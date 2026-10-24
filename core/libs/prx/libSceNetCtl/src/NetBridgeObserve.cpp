#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libc/include/General.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace {

constexpr std::size_t MaxQueuedLines = 8192;
constexpr std::size_t MaxQueuedBytes = 16u * 1024u * 1024u;
constexpr auto FlushInterval = std::chrono::milliseconds(250);

class ObservationWriter {
public:
    explicit ObservationWriter(std::filesystem::path path) : started(std::chrono::steady_clock::now()) {
        output.open(path, std::ios::binary | std::ios::app);
        if (!output.is_open()) return;
        open = true;
        std::thread(&ObservationWriter::run, this).detach();
    }

    bool Open() const {
        return open;
    }

    void Push(const NetBridgeObservation& observation) {
        std::string line = buildLine(observation);
        std::lock_guard lock(mutex);
        if (lines.size() >= MaxQueuedLines || queuedBytes + line.size() > MaxQueuedBytes) {
            ++dropped;
            return;
        }
        queuedBytes += line.size();
        lines.push_back(std::move(line));
        wake.notify_one();
    }

    void PushNow(const NetBridgeObservation& observation) {
        std::string text;
        std::uint64_t lost = 0;
        {
            std::lock_guard lock(mutex);
            for (const auto& queued : lines) text += queued;
            lines.clear();
            queuedBytes = 0;
            lost = std::exchange(dropped, 0);
        }
        if (lost != 0) text += droppedLine(lost);
        text += buildLine(observation);
        write(text);
    }

private:
    std::int64_t elapsedNanos() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started).count();
    }

    static void appendString(std::string& out, const char* text) {
        const std::string_view view = text != nullptr ? std::string_view(text) : std::string_view();
        const auto* data = reinterpret_cast<const unsigned char*>(view.data());
        out.push_back('"');
        if (NetBridgeText::ValidUtf8(data, view.size())) {
            NetBridgeText::AppendEscaped(out, data, view.size());
        } else {
            out += "b64:";
            NetBridgeText::AppendBase64(out, data, view.size());
        }
        out.push_back('"');
    }

    static void appendObject(std::string& out, const char* json) {
        if (json == nullptr) {
            out += "{}";
            return;
        }
        const std::string_view view(json);
        if (view.size() >= 2 && view.front() == '{' && view.back() == '}') {
            out += view;
        } else {
            out += "{}";
        }
    }

    std::string buildLine(const NetBridgeObservation& observation) const {
        std::string line = "{\"v\":1,\"t_ns\":" + std::to_string(elapsedNanos()) + ",\"src\":\"bridge\",\"lib\":";
        appendString(line, observation.lib);
        line += ",\"fn\":";
        appendString(line, observation.fn);
        line += ",\"in\":";
        appendObject(line, observation.inJson);
        line += ",\"out\":";
        appendObject(line, observation.outJson);
        if (observation.hasRet != 0) {
            char text[32];
            std::snprintf(text, sizeof(text), "0x%x", static_cast<unsigned>(static_cast<std::uint32_t>(observation.ret)));
            line += ",\"ret\":\"";
            line += text;
            line += "\"";
        }
        if (observation.cut != 0) line += ",\"cut\":" + std::to_string(observation.cut);
        line += "}\n";
        return line;
    }

    std::string droppedLine(std::uint64_t lost) const {
        return "{\"v\":1,\"t_ns\":" + std::to_string(elapsedNanos()) + ",\"src\":\"bridge\",\"lib\":\"libSceNetCtl\",\"fn\":\"dropped\",\"dropped\":" + std::to_string(lost) + "}\n";
    }

    void write(const std::string& text) {
        std::lock_guard lock(outputMutex);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.flush();
    }

    void run() {
        try {
            while (true) {
                std::deque<std::string> batch;
                std::uint64_t lost = 0;
                {
                    std::unique_lock lock(mutex);
                    wake.wait_for(lock, FlushInterval, [this] { return !lines.empty() || dropped != 0; });
                    batch.swap(lines);
                    queuedBytes = 0;
                    lost = std::exchange(dropped, 0);
                }
                std::string text;
                for (const auto& line : batch) text += line;
                if (lost != 0) text += droppedLine(lost);
                if (text.empty()) continue;
                write(text);
            }
        } catch (...) {
            std::fputs("[netbridge] observation writer stopped\n", stderr);
        }
    }

    std::chrono::steady_clock::time_point started;
    std::ofstream output;
    bool open = false;
    std::mutex mutex;
    std::mutex outputMutex;
    std::condition_variable wake;
    std::deque<std::string> lines;
    std::size_t queuedBytes = 0;
    std::uint64_t dropped = 0;
};

ObservationWriter* CreateWriter() {
#ifdef _WIN32
    const wchar_t* wide = _wgetenv(L"ANYPS5_NET_OBSERVE");
    if (wide == nullptr || wide[0] == L'\0') return nullptr;
    std::filesystem::path path(wide);
#else
    const char* narrow = std::getenv("ANYPS5_NET_OBSERVE");
    if (narrow == nullptr || narrow[0] == '\0') return nullptr;
    std::filesystem::path path(narrow);
#endif
    auto* writer = new ObservationWriter(std::move(path));
    if (!writer->Open()) {
        std::fputs("[netbridge] cannot open the observation file\n", stderr);
        delete writer;
        return nullptr;
    }
    return writer;
}

ObservationWriter* Writer() {
    static ObservationWriter* const writer = CreateWriter();
    return writer;
}

void ObserveNotImplemented(const char* funcName) {
    if (Writer() == nullptr) return;
    NetBridgeJson in;
    in.Text("export", funcName);
    const std::string inText = in.Finish();
    const NetBridgeObservation observation{"libSceNetCtl", "not_implemented", inText.c_str(), "{}", 0, 0, in.Cut()};
    NetBridgeObserveNow_nid_no_patch(&observation);
}

const bool g_notImplementedHooked = [] {
    SetNotImplementedHook_nid_no_patch(&ObserveNotImplemented);
    return true;
}();

}

extern "C" int NetBridgeObserveEnabled_nid_no_patch(void) {
    return Writer() != nullptr ? 1 : 0;
}

extern "C" void NetBridgeObserveNow_nid_no_patch(const NetBridgeObservation* observation) {
    if (observation == nullptr) return;
    auto* writer = Writer();
    if (writer == nullptr) return;
    try {
        writer->PushNow(*observation);
    } catch (...) {
    }
}

extern "C" void NetBridgeObserve_nid_no_patch(const NetBridgeObservation* observation) {
    if (observation == nullptr) return;
    auto* writer = Writer();
    if (writer == nullptr) return;
    try {
        writer->Push(*observation);
    } catch (...) {
    }
}
