#include "prx/libSceAgcDriver/Graphics/include/PipelineWatch.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

namespace {

using AgcDriver::Graphics::PipelineWatchRegistry;
using AgcDriver::Graphics::PipelineWatchStage;
using namespace std::chrono_literals;

int failures = 0;

void Expect(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAILED: %s\n", what);
    ++failures;
}

bool Contains(const std::string& text, const char* part) {
    return text.find(part) != std::string::npos;
}

struct Collected {
    std::mutex mutex;
    std::vector<std::string> lines;
    PipelineWatchRegistry::Sink Sink() {
        return [this](const std::string& line) {
            std::lock_guard lock(mutex);
            lines.push_back(line);
        };
    }
};

std::vector<PipelineWatchStage> Stages() {
    PipelineWatchStage vertex;
    vertex.vulkanStage = 0x1;
    vertex.words = 1234;
    vertex.hashLow = 0xaabbccddull;
    vertex.hashHigh = 0x1122ull;
    vertex.specializations = 2;
    vertex.bindings = 5;
    PipelineWatchStage fragment;
    fragment.vulkanStage = 0x10;
    fragment.words = 98765;
    fragment.imageTableShader = 0x2165c0500ull;
    return {vertex, fragment};
}

void QuickCreateIsSilent() {
    Collected out;
    PipelineWatchRegistry registry(out.Sink(), 3s, false);
    const auto start = PipelineWatchRegistry::Clock::now();
    const auto id = registry.Begin("graphics", Stages(), 7, start);
    Expect(registry.Sweep(start + 1s) == 0, "a create under the threshold is not reported");
    registry.End(id, start + 2s);
    Expect(out.lines.empty(), "a quick create writes nothing without tracing");
    Expect(registry.Active() == 0, "the create is no longer active");
}

void TraceRecordsBeginAndEnd() {
    Collected out;
    PipelineWatchRegistry registry(out.Sink(), 3s, true);
    registry.SetDevice("Test GPU vendor=10de device=2d58 driver=0x00000001");
    const auto start = PipelineWatchRegistry::Clock::now();
    const auto id = registry.Begin("graphics library link", Stages(), 9, start);
    registry.End(id, start + 40ms);
    Expect(out.lines.size() == 2, "tracing writes a begin and an end record");
    if (out.lines.size() != 2) return;
    Expect(Contains(out.lines[0], "begin #1 graphics library link"), "begin names the operation");
    Expect(Contains(out.lines[0], "words=1234") && Contains(out.lines[0], "words=98765"), "begin lists each stage's SPIR-V size");
    Expect(Contains(out.lines[0], "stage=0x1 ") && Contains(out.lines[0], "stage=0x10 "), "begin lists the stage flags");
    Expect(Contains(out.lines[0], "000000000000112200000000aabbccdd"), "begin carries the SPIR-V hash");
    Expect(Contains(out.lines[0], "imageTable=0x2165c0500"), "begin carries the image table shader");
    Expect(Contains(out.lines[0], "Test GPU vendor=10de device=2d58"), "records name the host device and driver");
    Expect(Contains(out.lines[1], "end #1") && Contains(out.lines[1], "elapsed=0.040 s"), "end carries the elapsed time");
}

void StalledCreateIsReportedOnceThenRepeated() {
    Collected out;
    PipelineWatchRegistry registry(out.Sink(), 3s, false);
    const auto start = PipelineWatchRegistry::Clock::now();
    const auto id = registry.Begin("graphics", Stages(), 3, start);
    Expect(registry.Sweep(start + 2999ms) == 0, "just under the threshold nothing is reported");
    Expect(registry.Sweep(start + 3100ms) == 1, "past the threshold the create is reported");
    Expect(registry.Sweep(start + 4000ms) == 0, "a reported create is not reported on every sweep");
    Expect(registry.Sweep(start + 13200ms) == 1, "a create that keeps running is reported again after ten seconds");
    Expect(out.lines.size() == 2 && Contains(out.lines[0], "STALL #1 graphics") && Contains(out.lines[0], "words=98765"), "the report names the create and its stages");
    registry.End(id, start + 20s);
    Expect(out.lines.size() == 3 && Contains(out.lines[2], "end after stall #1"), "a create that was reported also reports its end");
}

void SlowCreateReportsItsEndWithoutTracing() {
    Collected out;
    PipelineWatchRegistry registry(out.Sink(), 3s, false);
    const auto start = PipelineWatchRegistry::Clock::now();
    const auto id = registry.Begin("compute", {}, 1, start);
    registry.End(id, start + 5s);
    Expect(out.lines.size() == 1 && Contains(out.lines[0], "end #1 compute") && Contains(out.lines[0], "elapsed=5.000 s"), "a slow create is recorded even when it ended before a sweep");
}

void ScopeUsesTheProcessWatch() {
    AgcDriver::Graphics::PipelineWatch::SetDevice("Test GPU", 0x10de, 0x2d58, 1);
    {
        const AgcDriver::Graphics::PipelineWatch::Scope outer("graphics", [] { return Stages(); });
        const AgcDriver::Graphics::PipelineWatch::Scope inner("graphics library part");
    }
    Expect(AgcDriver::Graphics::PipelineWatch::Enabled(), "the watch is on by default");
}

}

int main() {
    QuickCreateIsSilent();
    TraceRecordsBeginAndEnd();
    StalledCreateIsReportedOnceThenRepeated();
    SlowCreateReportsItsEndWithoutTracing();
    ScopeUsesTheProcessWatch();
    if (failures != 0) return 1;
    std::puts("pipeline watch tests passed");
    return 0;
}
