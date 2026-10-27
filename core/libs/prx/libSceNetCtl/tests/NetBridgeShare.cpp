#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/include/ShareRing.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libSceNetCtl/tests/ShareTestRing.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

using FakeNet::Bytes;
using FakeNet::Require;
using FakeNet::Service;
using ShareTest::OpenPayload;
using ShareTest::PadPayload;
using ShareTest::Ring;

constexpr std::uint16_t KindShareOpen = 0x0501;
constexpr std::uint16_t KindShareOpened = 0x8501;
constexpr std::uint16_t KindShareClose = 0x0502;
constexpr std::uint16_t KindShareMode = 0x0503;
constexpr std::uint16_t KindSharePad = 0x0504;
constexpr std::uint16_t KindOverride = 0x0505;
constexpr std::uint16_t KindShareStatus = 0x0506;

constexpr std::uint32_t RingWidth = 16;
constexpr std::uint32_t RingHeight = 8;
constexpr std::uint32_t RingAudioFrames = 64;

std::size_t CountOf(Service& service, std::uint16_t kind) {
    std::size_t count = 0;
    for (const auto& frame : service.ReceivedFrames()) {
        if (frame.kind == kind) ++count;
    }
    return count;
}

std::uint8_t OpenAndWait(Service& service, std::uint32_t request, const Bytes& payload) {
    service.SendRaw(FakeNet::MakeFrame(KindShareOpen, request, payload));
    std::uint8_t result = 0xEE;
    Require(Service::WaitUntil([&] {
        for (const auto& frame : service.ReceivedFrames()) {
            if (frame.kind == KindShareOpened && frame.request == request && frame.payload.size() == 1) {
                result = frame.payload[0];
                return true;
            }
        }
        return false;
    }, 5000), "share open is answered with the same request number");
    return result;
}

void SendMode(Service& service, std::uint8_t mode) {
    service.SendRaw(FakeNet::MakeFrame(KindShareMode, 0, Bytes{mode}));
}

void SendPad(Service& service, const Bytes& payload) {
    service.SendRaw(FakeNet::MakeFrame(KindSharePad, 0, payload));
}

NetBridgeSharePadView PadNow(int* active = nullptr) {
    NetBridgeSharePadView view{};
    const int result = NetBridgeSharePad_nid_no_patch(&view);
    if (active != nullptr) *active = result;
    return view;
}

bool PadIs(std::uint32_t buttons, std::uint8_t lx, std::uint8_t l2) {
    int active = 0;
    const auto view = PadNow(&active);
    return active == 1 && view.buttons == buttons && view.sticks[0] == lx && view.l2 == l2;
}

std::vector<std::uint8_t> Pattern(std::uint32_t width, std::uint32_t height, std::uint8_t seed) {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
    for (std::size_t index = 0; index < pixels.size(); ++index) pixels[index] = static_cast<std::uint8_t>(seed + index * 7);
    return pixels;
}

void TestProducerOnMemory() {
    ShareRing::Geometry geometry;
    geometry.width = 8;
    geometry.height = 4;
    geometry.stride = 32;
    geometry.audioFrames = 16;
    std::vector<std::uint8_t> memory(geometry.TotalBytes() + 128);
    ShareRing::Producer producer;
    Require(!producer.Attach(memory.data(), memory.size(), 8, 4), "an empty mapping is refused");
    ShareRing::WriteHeader(memory.data(), geometry, 30);
    Require(!producer.Attach(memory.data(), geometry.TotalBytes() - 1, 8, 4), "a mapping smaller than the layout is refused");
    Require(!producer.Attach(memory.data(), memory.size(), 8, 8), "a size that differs from share open is refused");
    memory[0] = 'X';
    Require(!producer.Attach(memory.data(), memory.size(), 8, 4), "a wrong magic is refused");
    ShareRing::WriteHeader(memory.data(), geometry, 30);
    ShareRing::Store32(memory.data() + ShareRing::OffsetAudioFrames, 12);
    Require(!producer.Attach(memory.data(), memory.size(), 8, 4), "an audio ring that is not a power of two is refused");
    ShareRing::WriteHeader(memory.data(), geometry, 30);
    Require(producer.Attach(memory.data(), memory.size(), 8, 4) && producer.MaxFps() == 30, "a valid mapping attaches");

    const auto first = Pattern(8, 4, 1);
    Require(producer.SubmitFrame(first.data(), 8, 4, 32, 1000, 7), "first frame");
    Require(ShareRing::Load64(memory.data() + geometry.SlotOffset(0) + ShareRing::SlotOffsetSeq) == 1, "first frame is sequence 1 in slot 0");
    Require(std::memcmp(memory.data() + geometry.SlotOffset(0) + ShareRing::SlotHeaderBytes, first.data(), first.size()) == 0, "pixels copied");
    Require(ShareRing::Load64(memory.data() + geometry.SlotOffset(0) + ShareRing::SlotOffsetPresentUs) == 1000, "present_us stored");
    Require(ShareRing::Load32(memory.data() + geometry.SlotOffset(0) + ShareRing::SlotOffsetFrame) == 7, "frame stored");
    Require(ShareRing::Load32(memory.data() + geometry.SlotOffset(0) + ShareRing::SlotOffsetKind) == 0, "slot kind is pixels");
    for (int index = 2; index <= 4; ++index) Require(producer.SubmitFrame(first.data(), 8, 4, 32, 1000 + index, 7 + index), "more frames");
    Require(ShareRing::Load64(memory.data() + geometry.SlotOffset(0) + ShareRing::SlotOffsetSeq) == 4, "the fourth frame overwrites the oldest slot");
    Require(ShareRing::Load64(memory.data() + geometry.SlotOffset(1) + ShareRing::SlotOffsetSeq) == 2 && ShareRing::Load64(memory.data() + geometry.SlotOffset(2) + ShareRing::SlotOffsetSeq) == 3, "the other slots keep their sequence");

    std::vector<float> audio(40 * 2);
    for (std::size_t index = 0; index < audio.size(); ++index) audio[index] = static_cast<float>(index);
    Require(producer.SubmitAudio(audio.data(), 10), "audio");
    Require(ShareRing::Load64(memory.data() + ShareRing::OffsetAudioPosition) == 10, "audio position counts frames");
    Require(producer.SubmitAudio(audio.data(), 10), "audio wraps");
    Require(ShareRing::Load64(memory.data() + ShareRing::OffsetAudioPosition) == 20, "audio position after the wrap");
    float wrapped = 0;
    std::memcpy(&wrapped, memory.data() + geometry.AudioOffset(), sizeof(wrapped));
    Require(wrapped == audio[6 * 2], "frame 16 of the stream wraps to the start of the ring");
    Require(producer.SubmitAudio(audio.data(), 40), "a block larger than the ring keeps its newest frames");
    Require(ShareRing::Load64(memory.data() + ShareRing::OffsetAudioPosition) == 60, "audio position after an oversized block");
}

void TestLetterbox() {
    const auto source = Pattern(4, 4, 9);
    std::vector<std::uint8_t> destination(8 * 4 * 4, 0x55);
    ShareRing::Letterbox(destination.data(), 32, 8, 4, source.data(), 16, 4, 4);
    const std::uint32_t black = 0xFF000000u;
    std::uint32_t pixel = 0;
    std::memcpy(&pixel, destination.data(), 4);
    Require(pixel == black, "the bars are opaque black");
    std::memcpy(&pixel, destination.data() + 7 * 4, 4);
    Require(pixel == black, "the right bar is black");
    const auto area = ShareRing::Fit(4, 4, 8, 4);
    Require(area.x == 2 && area.y == 0 && area.width == 4 && area.height == 4, "a square picture is centred in a wide ring");
    Require(std::memcmp(destination.data() + 2 * 4, source.data(), 4) == 0, "the picture's first pixel lands in the fitted area");
    const auto tall = ShareRing::Fit(2, 8, 8, 4);
    Require(tall.width == 1 && tall.height == 4 && tall.y == 0, "a tall picture keeps its shape");
    const auto none = ShareRing::Fit(0, 4, 8, 4);
    Require(none.width == 0 && none.height == 0, "an empty source fits nowhere");
}

}

int main() {
    TestProducerOnMemory();
    TestLetterbox();

    const Bytes token = {0x73, 0x68, 0x61, 0x72, 0x65, 0x01};
    Service service(token);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));

    Require(NetBridgeEnabled_nid_no_patch() == 1, "bridge on");
    Require(NetBridgeShareActive_nid_no_patch() == 0 && PadNow().buttons == 0, "no share before share open");
    Require(NetBridgeShareOverride_nid_no_patch(0) == 0, "override without a share does nothing");
    NetBridgeStateView state{};
    NetBridgeState_nid_no_patch(&state);
    Require(Service::WaitUntil([&] { return service.Welcomed() == 1; }, 5000), "bridge welcomed");

    const std::string name = "anyplay-test-" + std::to_string(static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count() & 0xFFFFFF));
    Ring ring(name, RingWidth, RingHeight, RingAudioFrames, 60);
    const std::uint32_t ringBytes = static_cast<std::uint32_t>(ring.Size());

    Require(OpenAndWait(service, 11, OpenPayload("anyplay-no-such-ring", ringBytes, RingWidth, RingHeight, 60, 0)) == 1, "a ring that cannot be opened answers 1");
    Require(OpenAndWait(service, 12, OpenPayload(name, ringBytes, RingWidth + 2, RingHeight, 60, 0)) == 1, "a ring of another size answers 1");
    Require(OpenAndWait(service, 13, OpenPayload(name, ringBytes + 4096, RingWidth, RingHeight, 60, 0)) == 1, "a ring_bytes larger than the mapping answers 1");
    Require(OpenAndWait(service, 14, OpenPayload("../" + name, ringBytes, RingWidth, RingHeight, 60, 0)) == 1, "a name with a path answers 1");
    Require(OpenAndWait(service, 15, OpenPayload(name, ringBytes, RingWidth, RingHeight, 60, 3)) == 1, "an unknown mode answers 1");
    Require(NetBridgeShareActive_nid_no_patch() == 0, "refused opens leave no share");

    Require(OpenAndWait(service, 21, OpenPayload(name, ringBytes, RingWidth, RingHeight, 60, 0)) == 0, "share open maps the ring");
    Require(NetBridgeShareActive_nid_no_patch() == 1, "the share is active");
    Require(OpenAndWait(service, 22, OpenPayload(name, ringBytes, RingWidth, RingHeight, 60, 0)) == 2, "a second open answers 2");

    const Bytes expectedOpened = {0x00};
    for (const auto& frame : service.ReceivedFrames()) {
        if (frame.kind == KindShareOpened && frame.request == 21) Require(frame.payload == expectedOpened, "reply payload is one result byte");
    }

    int active = 1;
    SendPad(service, PadPayload(0, 1, 0x11, 10, 20, 30, 40, 50, 60));
    SendMode(service, 0);
    SendPad(service, PadPayload(0, 2, 0x11, 10, 20, 30, 40, 50, 60));
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    PadNow(&active);
    Require(active == 0, "a pad state in watch is never accepted");

    SendMode(service, 1);
    Require(Service::WaitUntil([&] { return PadIs(0, 128, 0); }, 5000), "hand_over starts with a neutral guest pad");
    auto view = PadNow();
    Require(view.mode == 1 && view.slot == 0 && view.sticks[1] == 128 && view.sticks[2] == 128 && view.sticks[3] == 128 && view.r2 == 0, "neutral state is no buttons, sticks in the middle, triggers 0");
    SendPad(service, PadPayload(1, 3, 0x01, 1, 2, 3, 4, 5, 6));
    SendPad(service, PadPayload(0, 4, 0x8000, 1, 2, 3, 4, 5, 6));
    SendPad(service, PadPayload(0, 5, 0x80000011u, 1, 2, 3, 4, 5, 6));
    SendPad(service, PadPayload(0, 6, 0x4011, 200, 100, 50, 25, 7, 9));
    Require(Service::WaitUntil([&] { return PadIs(0x4011, 200, 7); }, 5000), "a valid state for slot 0 is applied");
    view = PadNow();
    Require(view.sticks[1] == 100 && view.sticks[2] == 50 && view.sticks[3] == 25 && view.r2 == 9, "all axes arrive");
    const std::uint64_t serialBefore = view.serial;
    SendPad(service, PadPayload(0, 7, 0x4011, 200, 100, 50, 25, 7, 9));
    Require(Service::WaitUntil([&] { return PadNow().serial > serialBefore; }, 5000), "every accepted state advances the serial");
    SendPad(service, Bytes{0, 1, 2});
    SendPad(service, PadPayload(0, 8, 0x0008, 128, 128, 128, 128, 0, 0));
    Require(Service::WaitUntil([&] { return PadIs(0x0008, 128, 0); }, 5000), "a short state is skipped, the next valid one applies");

    SendMode(service, 2);
    Require(Service::WaitUntil([&] { return PadIs(0, 128, 0) && PadNow().slot == 1 && PadNow().mode == 2; }, 5000), "together switches to slot 1 and a neutral pad");
    SendPad(service, PadPayload(0, 9, 0x0010, 1, 1, 1, 1, 1, 1));
    SendPad(service, PadPayload(1, 10, 0x0020, 60, 128, 128, 128, 0, 255));
    Require(Service::WaitUntil([&] { return PadIs(0x0020, 60, 0); }, 5000), "together takes slot 1 and ignores slot 0");
    Require(PadNow().r2 == 255, "trigger value arrives");

    SendMode(service, 0);
    Require(Service::WaitUntil([&] {
        int now = 1;
        PadNow(&now);
        return now == 0;
    }, 5000), "a mode back to watch removes the guest pad in the same step");
    SendMode(service, 1);
    Require(Service::WaitUntil([&] { return PadIs(0, 128, 0); }, 5000), "the guest pad is neutral again, not the old state");

    Require(NetBridgeShareWantsFrame_nid_no_patch() == 1, "the first frame is wanted");
    Require(NetBridgeShareWantsFrame_nid_no_patch() == 0, "a second frame at once exceeds the stream's rate");
    const auto picture = Pattern(RingWidth, RingHeight, 3);
    Require(NetBridgeShareSubmitFrame_nid_no_patch(picture.data(), RingWidth, RingHeight, RingWidth * 4, 424242, 99) == 1, "a frame is copied into the ring");
    Require(ring.Seq(0) == 1 && ring.NewestSlot() == 0, "the first frame is sequence 1");
    Require(std::memcmp(ring.Pixels(0), picture.data(), picture.size()) == 0, "pixels are in the slot");
    Require(ring.PresentUs(0) == 424242 && ring.FrameNumber(0) == 99, "present_us and frame number are in the slot header");
    for (int index = 0; index < 3; ++index) {
        Require(NetBridgeShareSubmitFrame_nid_no_patch(picture.data(), RingWidth, RingHeight, RingWidth * 4, 500000 + index, 100 + index) == 1, "more frames");
    }
    Require(ring.Seq(0) == 4 && ring.NewestSlot() == 0 && ring.Seq(1) == 2 && ring.Seq(2) == 3, "the ring overwrites the oldest slot");
    const auto small = Pattern(RingHeight, RingHeight, 5);
    Require(NetBridgeShareSubmitFrame_nid_no_patch(small.data(), RingHeight, RingHeight, RingHeight * 4, 600000, 200) == 1, "a smaller picture is letterboxed");
    std::uint32_t bar = 0;
    std::memcpy(&bar, ring.Pixels(ring.NewestSlot()), 4);
    Require(bar == 0xFF000000u, "the bar of a letterboxed picture is opaque black");
    Require(NetBridgeShareSubmitFrame_nid_no_patch(nullptr, RingWidth, RingHeight, RingWidth * 4, 1, 1) == 0, "a null picture is dropped");

    std::vector<float> samples(RingAudioFrames * 2);
    for (std::size_t index = 0; index < samples.size(); ++index) samples[index] = static_cast<float>(index) * 0.5f;
    Require(NetBridgeShareSubmitAudio_nid_no_patch(samples.data(), 48) == 1, "audio is copied into the ring");
    Require(NetBridgeShareSubmitAudio_nid_no_patch(samples.data(), 48) == 1, "audio wraps around the ring");
    Require(ring.AudioPosition() == 96, "the audio write position counts frames");
    for (std::uint32_t frame = 0; frame < 32; ++frame) {
        const std::uint32_t at = (48 + frame) & (RingAudioFrames - 1);
        Require(ring.AudioFrames()[at * 2] == samples[frame * 2] && ring.AudioFrames()[at * 2 + 1] == samples[frame * 2 + 1], "wrapped audio is where the consumer reads it");
    }

    Require(Service::WaitUntil([&] { return CountOf(service, KindShareStatus) >= 2; }, 6000), "status goes out about once a second");
    Bytes lastStatus;
    for (const auto& frame : service.ReceivedFrames()) {
        if (frame.kind == KindShareStatus) {
            Require(frame.request == 0 && frame.payload.size() == 12, "status is an event of three u32");
            lastStatus = frame.payload;
        }
    }
    Require(lastStatus[0] == 5 && lastStatus[1] == 0 && lastStatus[2] == 0 && lastStatus[3] == 0, "status counts the frames copied");
    Require(lastStatus[4] >= 1, "status counts the frames dropped by the rate cap and the refused one");

    const std::size_t overridesBefore = CountOf(service, KindOverride);
    Require(NetBridgeShareOverride_nid_no_patch(0) == 1, "the chord in hand_over takes control back");
    int now = 1;
    PadNow(&now);
    Require(now == 0, "the guest pad is gone at once, before the net service answers");
    Require(Service::WaitUntil([&] { return CountOf(service, KindOverride) == overridesBefore + 1; }, 5000), "override reaches the net service");
    Require(service.LastPayload(KindOverride) == Bytes{1}, "override action 1");
    Require(NetBridgeShareOverride_nid_no_patch(0) == 2, "the chord in watch asks to stop");
    Require(Service::WaitUntil([&] { return service.LastPayload(KindOverride) == Bytes{2}; }, 5000), "override action 2");
    SendMode(service, 2);
    Require(Service::WaitUntil([&] { return PadNow().mode == 2; }, 5000), "together again");
    Require(NetBridgeShareOverride_nid_no_patch(1) == 2, "the stop key asks to stop");
    PadNow(&now);
    Require(now == 0, "the stop key also ends the guest pad");

    service.SendRaw(FakeNet::MakeFrame(KindShareClose, 0, {}));
    Require(Service::WaitUntil([&] { return NetBridgeShareActive_nid_no_patch() == 0; }, 5000), "share close ends the share");
    Require(NetBridgeShareSubmitFrame_nid_no_patch(picture.data(), RingWidth, RingHeight, RingWidth * 4, 1, 1) == 0, "no frames after the close");
    Require(NetBridgeShareWantsFrame_nid_no_patch() == 0, "no frame is wanted after the close");
    PadNow(&now);
    Require(now == 0, "no guest pad after the close");
    SendPad(service, PadPayload(0, 11, 0x0001, 1, 1, 1, 1, 1, 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PadNow(&now);
    Require(now == 0, "a pad state without a share is dropped");

    Require(OpenAndWait(service, 31, OpenPayload(name, ringBytes, RingWidth, RingHeight, 0, 2)) == 0, "a share opens again after a close, with the mode given");
    Require(Service::WaitUntil([&] { return PadIs(0, 128, 0) && PadNow().slot == 1; }, 5000), "the opening mode starts the guest pad");
    SendPad(service, PadPayload(1, 1, 0x0004, 128, 128, 128, 128, 0, 0));
    Require(Service::WaitUntil([&] { return PadIs(0x0004, 128, 0); }, 5000), "a pad state of the second share");
    service.DropConnection();
    Require(Service::WaitUntil([&] { return NetBridgeShareActive_nid_no_patch() == 0; }, 5000), "a lost net service connection closes the share");
    PadNow(&now);
    Require(now == 0, "and neutralises and disconnects the guest pad");

    Require(service.Malformed() == 0, "the bridge sent only well-formed frames");
    std::puts("net bridge share: ok");
    return 0;
}
