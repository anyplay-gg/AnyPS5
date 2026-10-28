#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_PIPELINEWATCH_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_PIPELINEWATCH_HPP

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace AgcDriver::Graphics {

struct PipelineWatchStage {
    std::uint32_t vulkanStage = 0;
    std::uint32_t words = 0;
    std::uint64_t hashLow = 0;
    std::uint64_t hashHigh = 0;
    std::uint32_t specializations = 0;
    std::uint32_t bindings = 0;
    std::uint64_t imageTableShader = 0;
};

class PipelineWatchRegistry {
public:
    using Clock = std::chrono::steady_clock;
    using Sink = std::function<void(const std::string&)>;

    PipelineWatchRegistry(Sink sink, Clock::duration threshold, bool trace) : sink(std::move(sink)), threshold(threshold), trace(trace) {}

    void SetDevice(std::string description) {
        std::lock_guard lock(mutex);
        device = std::move(description);
    }

    std::uint64_t Begin(const char* operation, std::vector<PipelineWatchStage> stages, std::uint64_t thread, Clock::time_point now) {
        std::string line;
        std::uint64_t id;
        {
            std::lock_guard lock(mutex);
            id = ++last;
            Entry entry{operation, std::move(stages), thread, now, now};
            if (trace) line = Format("begin", id, entry, now);
            active.emplace(id, std::move(entry));
        }
        if (!line.empty()) sink(line);
        return id;
    }

    void End(std::uint64_t id, Clock::time_point now) {
        std::string line;
        {
            std::lock_guard lock(mutex);
            const auto found = active.find(id);
            if (found == active.end()) return;
            if (trace || now - found->second.started >= threshold) line = Format(found->second.reported ? "end after stall" : "end", id, found->second, now);
            active.erase(found);
        }
        if (!line.empty()) sink(line);
    }

    std::size_t Sweep(Clock::time_point now) {
        std::vector<std::string> lines;
        {
            std::lock_guard lock(mutex);
            for (auto& [id, entry] : active) {
                if (now - entry.lastReport < (entry.reported ? RepeatEvery : threshold)) continue;
                entry.lastReport = now;
                entry.reported = true;
                lines.push_back(Format("STALL", id, entry, now));
            }
        }
        for (const auto& line : lines) sink(line);
        return lines.size();
    }

    std::size_t Active() const {
        std::lock_guard lock(mutex);
        return active.size();
    }

    static std::string Describe(const std::vector<PipelineWatchStage>& stages) {
        std::string text;
        char piece[192];
        for (const auto& stage : stages) {
            std::snprintf(piece, sizeof(piece), " stage=0x%x words=%u hash=%016llx%016llx specializations=%u bindings=%u imageTable=0x%llx", stage.vulkanStage, stage.words, static_cast<unsigned long long>(stage.hashHigh), static_cast<unsigned long long>(stage.hashLow), stage.specializations, stage.bindings, static_cast<unsigned long long>(stage.imageTableShader));
            text += piece;
            text += ';';
        }
        return text;
    }

private:
    static constexpr Clock::duration RepeatEvery = std::chrono::seconds(10);

    struct Entry {
        const char* operation;
        std::vector<PipelineWatchStage> stages;
        std::uint64_t thread;
        Clock::time_point started;
        Clock::time_point lastReport;
        bool reported = false;
    };

    std::string Format(const char* what, std::uint64_t id, const Entry& entry, Clock::time_point now) const {
        char head[256];
        const double seconds = std::chrono::duration<double>(now - entry.started).count();
        std::snprintf(head, sizeof(head), "[pipeline-watch] %s #%llu %s thread=%llu elapsed=%.3f s device=\"", what, static_cast<unsigned long long>(id), entry.operation, static_cast<unsigned long long>(entry.thread), seconds);
        return std::string(head) + device + "\"" + Describe(entry.stages);
    }

    Sink sink;
    Clock::duration threshold;
    bool trace;
    mutable std::mutex mutex;
    std::map<std::uint64_t, Entry> active;
    std::uint64_t last = 0;
    std::string device;
};

class PipelineWatch {
public:
    static bool Enabled() { return Registry() != nullptr; }

    static void SetDevice(const char* name, std::uint32_t vendor, std::uint32_t deviceId, std::uint32_t driver) {
        if (auto* registry = Registry()) {
            char text[320];
            std::snprintf(text, sizeof(text), "%s vendor=%04x device=%04x driver=0x%08x", name, vendor, deviceId, driver);
            registry->SetDevice(text);
        }
    }

    class Scope {
    public:
        template<typename TBuild>
        Scope(const char* operation, TBuild&& build) {
            auto* watched = Registry();
            if (watched == nullptr) return;
            std::vector<PipelineWatchStage> stages = Inherited();
            const bool nested = !stages.empty();
            if (!nested) stages = build();
            outermost = !nested && !stages.empty();
            if (outermost) Inherited() = stages;
            registry = watched;
            id = watched->Begin(operation, std::move(stages), std::hash<std::thread::id>{}(std::this_thread::get_id()), PipelineWatchRegistry::Clock::now());
        }
        explicit Scope(const char* operation) : Scope(operation, [] { return std::vector<PipelineWatchStage>{}; }) {}
        ~Scope() {
            if (registry == nullptr) return;
            registry->End(id, PipelineWatchRegistry::Clock::now());
            if (outermost) Inherited().clear();
        }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        PipelineWatchRegistry* registry = nullptr;
        std::uint64_t id = 0;
        bool outermost = false;
    };

private:
    struct Config {
        long long thresholdMs;
        bool trace;
    };

    static std::vector<PipelineWatchStage>& Inherited() {
        static thread_local std::vector<PipelineWatchStage> stages;
        return stages;
    }

    static const Config& Settings() {
        static const Config config = [] {
            Config result{3000, std::getenv("APS5_TRACE_PIPELINE_CREATE") != nullptr};
            if (const char* text = std::getenv("APS5_PIPELINE_WATCH_MS"); text != nullptr && text[0] >= '0' && text[0] <= '9') {
                char* end = nullptr;
                const auto value = std::strtoll(text, &end, 10);
                if (end != nullptr && *end == 0) result.thresholdMs = value;
            }
            return result;
        }();
        return config;
    }

    static PipelineWatchRegistry* Registry() {
        static PipelineWatchRegistry* const registry = []() -> PipelineWatchRegistry* {
            const auto& config = Settings();
            if (config.thresholdMs <= 0) return nullptr;
            auto* created = new PipelineWatchRegistry([](const std::string& line) { std::fprintf(stderr, "%s\n", line.c_str()); }, std::chrono::milliseconds(config.thresholdMs), config.trace);
            const auto period = std::chrono::milliseconds(std::clamp<long long>(config.thresholdMs / 2, 50, 500));
            std::thread([created, period] {
                for (;;) {
                    std::this_thread::sleep_for(period);
                    created->Sweep(PipelineWatchRegistry::Clock::now());
                }
            }).detach();
            return created;
        }();
        return registry;
    }
};

}

#endif
