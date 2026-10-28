#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libSceNetCtl/tests/ShareTestRing.hpp"
#include "prx/libScePad/include/Pad.hpp"
#include "prx/libScePad/include/PadState.hpp"
#include "prx/libScePad/include/SharePadMapping.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>

namespace {

using FakeNet::Bytes;
using FakeNet::Require;
using FakeNet::Service;
using ShareTest::OpenPayload;
using ShareTest::PadPayload;

constexpr std::uint32_t Width = 8;
constexpr std::uint32_t Height = 4;

void TestMapping() {
    using namespace Pad::Share;
    Require(EngineButtons(DpadUp) == 0x0010 && EngineButtons(DpadDown) == 0x0040 && EngineButtons(DpadLeft) == 0x0080 && EngineButtons(DpadRight) == 0x0020, "directions");
    Require(EngineButtons(South) == 0x4000 && EngineButtons(East) == 0x2000 && EngineButtons(West) == 0x8000 && EngineButtons(North) == 0x1000, "face buttons by position");
    Require(EngineButtons(L1) == 0x0400 && EngineButtons(R1) == 0x0800 && EngineButtons(L3) == 0x0002 && EngineButtons(R3) == 0x0004, "shoulders and sticks");
    Require(EngineButtons(Start) == 0x0008 && EngineButtons(PadClick) == 0x100000, "start and pad click");
    Require(EngineButtons(Select) == 0, "select is not delivered to the game");
    Require(EngineButtons(0x7FFF) == 0x4000 + 0x2000 + 0x8000 + 0x1000 + 0x0010 + 0x0040 + 0x0080 + 0x0020 + 0x0400 + 0x0800 + 0x0002 + 0x0004 + 0x0008 + 0x100000, "all buttons together");

    NetBridgeSharePadView view{};
    view.sticks[0] = 1;
    view.sticks[1] = 2;
    view.sticks[2] = 3;
    view.sticks[3] = 4;
    view.l2 = 9;
    view.r2 = 0;
    const PadInputState state = ToInputState(view);
    Require(state.sticks[0] == 1 && state.sticks[1] == 2 && state.sticks[2] == 3 && state.sticks[3] == 4, "sticks");
    Require(state.analogButtonsL2 == 9 && (state.buttons & 0x0100) != 0, "a pulled L2 sets its digital bit");
    Require(state.analogButtonsR2 == 0 && (state.buttons & 0x0200) == 0, "a released R2 does not");
    Require(!state.hasMotion && !state.touchLeft && !state.touchRight && !state.touch[0].active, "no motion and no touch from a guest");
    Require(PortOf(ModeHandOver) == 0 && PortOf(ModeTogether) == 1, "hand_over is pad 1, together is pad 2");
}

bool Reads(int port, std::uint32_t buttons, std::uint8_t leftX) {
    const PadData data = Pad::ReadState(port);
    return data.buttons == buttons && data.left_stick_x == leftX;
}

}

int main() {
    TestMapping();

    const Bytes token = {0x70, 0x61, 0x64, 0x01};
    Service service(token);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));
    Require(NetBridgeEnabled_nid_no_patch() == 1, "bridge on");
    NetBridgeStateView state{};
    NetBridgeState_nid_no_patch(&state);
    Require(Service::WaitUntil([&] { return service.Welcomed() == 1; }, 5000), "bridge welcomed");

    Pad::Initialize();
    PadInputState host;
    host.buttons = 0x4000;
    host.sticks = {10, 10, 10, 10};
    PadPublishInputForPort_nid_postfix(0, host);
    PadInputState hostTwo;
    hostTwo.buttons = 0x0008;
    hostTwo.sticks = {20, 20, 20, 20};
    PadPublishInputForPort_nid_postfix(1, hostTwo);

    Require(Reads(0, 0x4000, 10), "without a share the pad is the host's");
    Require(!Pad::IsConnected(1), "pad 2 is not connected without a share");
    Require(Pad::ReadState(0).connected_count == 1, "one pad connected");

    const std::string name = "anyplay-pad-" + std::to_string(static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count() & 0xFFFFFF));
    ShareTest::Ring ring(name, Width, Height, 0);
    service.SendRaw(FakeNet::MakeFrame(0x0501, 5, OpenPayload(name, static_cast<std::uint32_t>(ring.Size()), Width, Height, 60, 1)));
    Require(Service::WaitUntil([&] { return NetBridgeShareActive_nid_no_patch() == 1; }, 5000), "share opened in hand_over");

    Require(Service::WaitUntil([&] { return Reads(0, 0, 128); }, 5000), "hand_over replaces pad 1 with the guest's neutral pad");
    Require(Pad::ReadState(0).left_stick_y == 128 && Pad::ReadState(0).analog_buttons_l2 == 0, "neutral sticks and triggers");
    Require(!Pad::IsConnected(1), "hand_over adds no second pad");
    service.SendRaw(FakeNet::MakeFrame(0x0504, 0, PadPayload(0, 1, 0x0101, 200, 60, 128, 128, 255, 0)));
    Require(Service::WaitUntil([&] { return Reads(0, 0x0010 | 0x0400 | 0x0100, 200); }, 5000), "the guest's state reaches pad 1");
    const PadData guest = Pad::ReadState(0);
    Require(guest.left_stick_y == 60 && guest.analog_buttons_l2 == 255 && (guest.buttons & 0x4000) == 0, "the host's cross is not in the guest's state");
    PadData queued[4]{};
    Require(Pad::Read(queued, 4, 0) == 1 && (queued[0].buttons & 0x0410) == 0x0410 && (queued[0].buttons & 0x4000) == 0, "Read returns the guest's state too");

    service.SendRaw(FakeNet::MakeFrame(0x0503, 0, Bytes{2}));
    Require(Service::WaitUntil([&] { return Pad::IsConnected(1); }, 5000), "together connects pad 2");
    Require(Reads(0, 0x4000, 10), "together leaves pad 1 to the host");
    Require(Service::WaitUntil([&] { return Reads(1, 0, 128); }, 5000), "pad 2 is the guest's neutral pad");
    Require(Pad::ReadState(0).connected_count == 2, "two pads connected");
    service.SendRaw(FakeNet::MakeFrame(0x0504, 0, PadPayload(1, 2, 0x0020, 33, 128, 128, 128, 0, 0)));
    Require(Service::WaitUntil([&] { return Reads(1, 0x2000, 33); }, 5000), "the guest's East button reaches pad 2");

    service.SendRaw(FakeNet::MakeFrame(0x0503, 0, Bytes{0}));
    Require(Service::WaitUntil([&] { return !Pad::IsConnected(1); }, 5000), "watch unplugs pad 2");
    Require(Reads(0, 0x4000, 10), "the host's pad again on pad 1");
    Require(Pad::ReadState(1).buttons == 0x0008, "pad 2's own state is untouched by the share");

    service.SendRaw(FakeNet::MakeFrame(0x0503, 0, Bytes{1}));
    Require(Service::WaitUntil([&] { return Reads(0, 0, 128); }, 5000), "hand_over again");
    Require(NetBridgeShareOverride_nid_no_patch(0) == 1, "the host takes control back");
    Require(Reads(0, 0x4000, 10), "the host's pad is back at once");

    service.DropConnection();
    Require(Service::WaitUntil([&] { return NetBridgeShareActive_nid_no_patch() == 0; }, 5000), "a lost connection ends the share");
    Require(Reads(0, 0x4000, 10), "and the host plays on");

    std::puts("pad share: ok");
    return 0;
}
