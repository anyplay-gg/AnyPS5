#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/src/NetBridgeWire.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

using FakeNet::Bytes;
using FakeNet::Require;
using FakeNet::Service;
using namespace NetBridgeWire;

constexpr std::uint16_t KindWebReply = 0x8201;
constexpr std::uint16_t KindActivated = 0x8301;
constexpr std::uint16_t KindPeerInfoReply = 0x8304;
constexpr std::uint16_t KindBound = 0x8305;

std::string BodyOf(const NetBridgeWebReplyView& reply) {
    return std::string(reinterpret_cast<const char*>(reply.body), static_cast<std::size_t>(reply.bodySize));
}

std::string HeaderOf(const NetBridgeWebReplyView& reply, const std::string& name) {
    for (std::uint32_t index = 0; index < reply.headerCount; ++index) {
        if (name == reply.headerNames[index]) return reply.headerValues[index];
    }
    return "<missing>";
}

void TestHeaderLayoutMatchesSpec() {
    const std::vector<std::uint8_t> payload = {0xAA, 0xBB};
    const auto frame = EncodeFrame(KindWebRequest, 7, payload);

    const std::vector<std::uint8_t> expected = {
        0x02, 0x00, 0x00, 0x00,
        0x01, 0x02,
        0x00, 0x00,
        0x07, 0x00, 0x00, 0x00,
        0xAA, 0xBB
    };
    Require(frame == expected, "frame wire format must match spec layout exactly");

    const FrameHeader parsed = ParseHeader(frame.data());
    Require(parsed.length == 2, "parsed length matches");
    Require(parsed.kind == KindWebRequest, "parsed kind matches");
    Require(parsed.flags == 0, "parsed flags matches");
    Require(parsed.request == 7, "parsed request matches");
}

void TestUnsupportedRoundTrip() {
    const auto frame = EncodeFrame(KindUnsupported, 42, {});
    const FrameHeader hdr = ParseHeader(frame.data());
    Require(hdr.kind == KindUnsupported, "unsupported kind");
    Require(hdr.request == 42, "unsupported request");
    Require(hdr.length == 0, "unsupported empty payload");
}

void TestTruncatedPayloadHandled() {
    const std::vector<std::uint8_t> incomplete = {0x01, 0x00, 0x00};
    Reader r(incomplete.data(), incomplete.size());
    r.U32();
    Require(!r.Ok(), "reading past buffer marks reader not ok");

    const std::vector<std::uint8_t> badStr = {0x05, 0x00, 'a', 'b'};
    Reader r2(badStr.data(), badStr.size());
    r2.Str();
    Require(!r2.Ok(), "truncated string marks reader not ok");
}

void TestTrailingFieldsIgnored() {
    Builder b;
    b.I32(7).Str("forward").U32(999);
    const auto payload = b.Take();

    Reader r(payload.data(), payload.size());
    const std::int32_t id = r.I32();
    const std::string s = r.Str();
    Require(r.Ok(), "base fields parsed ok");
    Require(id == 7 && s == "forward", "base values correct");
    Require(r.Remaining() == 4, "trailing fields preserved for forward compatibility");
}

void TestRejectRoundTrip() {
    const std::string reason = "bad token";
    Builder b;
    b.Raw(reason.data(), reason.size());
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {'b', 'a', 'd', ' ', 't', 'o', 'k', 'e', 'n'};
    Require(payload == expected, "reject fixed byte vector matches raw utf-8");
    Require(payload.size() == reason.size(), "reject has no u16 length prefix");

    const std::string back(payload.begin(), payload.end());
    Require(back == reason, "reject reason decoded from raw payload");
}

void TestPayloadLengthLimit() {
    Require(MaxPayload == 16 * 1024 * 1024, "MaxPayload constant is 16 MiB");

    Builder bBytes;
    const std::uint8_t dummy = 0;
    bBytes.Bytes(&dummy, static_cast<std::uint64_t>(MaxPayload) + 1ULL);
    Require(!bBytes.Ok(), "Bytes exceeding MaxPayload fails");
}

void TestShareKindsMatchSpec() {
    Require(KindShareOpen == 0x0501 && KindShareOpened == (KindShareOpen | ReplyBit), "share open and its reply");
    Require(KindShareClose == 0x0502 && KindShareMode == 0x0503 && KindSharePad == 0x0504, "share close, mode and pad");
    Require(KindOverride == 0x0505 && KindShareStatus == 0x0506, "override and status");

    Builder pad;
    pad.U8(1).U16(0x0102).U32(0x00004011).U8(1).U8(2).U8(3).U8(4).U8(5).U8(6);
    const Bytes expected = {
        0x0D, 0x00, 0x00, 0x00,
        0x04, 0x05,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01,
        0x02, 0x01,
        0x11, 0x40, 0x00, 0x00,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };
    Require(EncodeFrame(KindSharePad, 0, pad.Take()) == expected, "share pad frame matches the spec byte for byte");

    Builder open;
    open.Str("r1").U32(4096).U32(1920).U32(1080).U32(60).U8(2);
    const Bytes expectedOpen = {
        0x02, 0x00, 'r', '1',
        0x00, 0x10, 0x00, 0x00,
        0x80, 0x07, 0x00, 0x00,
        0x38, 0x04, 0x00, 0x00,
        0x3C, 0x00, 0x00, 0x00,
        0x02
    };
    Require(open.Take() == expectedOpen, "share open payload matches the spec byte for byte");

    // The bridge-to-service direction: written out, not built with the code under test.
    const Bytes expectedOpened = {
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x85,
        0x00, 0x00,
        0x07, 0x00, 0x00, 0x00,
        0x02
    };
    Require(EncodeFrame(KindShareOpened, 7, Bytes{2}) == expectedOpened, "share opened frame matches the spec byte for byte");
    const Bytes expectedOverride = {
        0x01, 0x00, 0x00, 0x00,
        0x05, 0x05,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01
    };
    Require(EncodeFrame(KindOverride, 0, Bytes{1}) == expectedOverride, "override frame matches the spec byte for byte");
    Builder status;
    status.U32(0x0102).U32(3).U32(4);
    const Bytes expectedStatus = {
        0x0C, 0x00, 0x00, 0x00,
        0x06, 0x05,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00
    };
    Require(EncodeFrame(KindShareStatus, 0, status.Take()) == expectedStatus, "share status frame matches the spec byte for byte");
}

void TestRealBridgeWireVectors() {
    const Bytes token = {0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x01, 0xFE};
    Service service(token);
    const std::filesystem::path observePath = std::filesystem::temp_directory_path() / "anyps5-bridge-protocol-test.jsonl";
    std::error_code ec;
    std::filesystem::remove(observePath, ec);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));
    FakeNet::SetEnvironment("ANYPS5_NET_OBSERVE", observePath.string());

    Require(NetBridgeEnabled_nid_no_patch() == 1, "bridge enabled");
    Require(Service::WaitUntil([&] { return service.Welcomed() == 1; }, 5000), "bridge welcomed");

    const Bytes hello = service.LastPayload(KindHello);
    Require(hello.size() == 34 && hello[0] == 0x01 && hello[1] == 0x00, "hello wire payload matches spec");
    Require(std::any_of(hello.begin() + 2, hello.end(), [](std::uint8_t byte) { return byte != 0; }), "hello nonce is not empty");
    Require(service.LastPayload(KindAuth).size() == 32, "auth wire payload is one 32-byte proof");

    NetBridgeTrafficView traffic{};
    Require(NetBridgeTraffic_nid_no_patch(&traffic) == 1, "traffic view answers");
    Require(traffic.udpPort == service.DatagramPort(), "welcome udp port exposed");
    Require(traffic.tcpPort == service.StreamPort(), "welcome tcp port exposed");
    for (int i = 0; i < 16; ++i) {
        Require(traffic.streamKey[i] == static_cast<std::uint8_t>(i), "welcome stream key exposed");
    }

    const Bytes stateVector = {
        0x01,
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
        0x03, 0x00, 'A', 'n', 'y',
        0x02,
        0x01, 0x00, 0x12, 0xC6
    };
    service.SendRaw(FakeNet::MakeFrame(KindState, 0, stateVector));
    NetBridgeStateView stateView{};
    Require(Service::WaitUntil([&] {
        NetBridgeState_nid_no_patch(&stateView);
        return stateView.signedIn == 1 && std::string(stateView.handle) == "Any";
    }, 5000), "state vector applied");
    Require(stateView.accountId == 0x0102030405060708ULL, "state account id exposed");
    Require(stateView.natType == 2, "state nat type exposed");
    Require(stateView.address == 0xC6120001u, "state address exposed in host byte order");

    NetBridgeFeature_nid_no_patch(-1, 0x1234);
    Require(Service::WaitUntil([&] { return !service.LastPayload(KindFeature).empty(); }, 5000), "feature frame received");
    const Bytes expectedFeature = {0xFF, 0xFF, 0xFF, 0xFF, 0x34, 0x12, 0x00, 0x00};
    Require(service.LastPayload(KindFeature) == expectedFeature, "feature wire payload matches spec byte for byte");

    const Bytes webReplyVector = {
        0xC8, 0x00,
        0x01, 0x00, 0x06, 0x00, 'x', '-', 'r', 'e', 's', 'p', 0x02, 0x00, 'o', 'k',
        0x04, 0x00, 0x00, 0x00, 'p', 'o', 'n', 'g'
    };
    service.SetNextWebReply(webReplyVector);

    const char* reqHeaderNames[] = {"h"};
    const char* reqHeaderValues[] = {"v"};
    const std::string reqBody = "ping";
    NetBridgeWebRequestView req{};
    req.userId = 7;
    req.apiGroup = "group";
    req.path = "/test";
    req.method = "POST";
    req.headerCount = 1;
    req.headerNames = reqHeaderNames;
    req.headerValues = reqHeaderValues;
    req.body = reqBody.data();
    req.bodySize = reqBody.size();
    NetBridgeWebReplyView webReply{};
    Require(NetBridgeWebRequest_nid_no_patch(&req, &webReply) == 1, "web request offered");

    const Bytes expectedWebRequest = {
        0x07, 0x00, 0x00, 0x00,
        0x05, 0x00, 'g', 'r', 'o', 'u', 'p',
        0x05, 0x00, '/', 't', 'e', 's', 't',
        0x04, 0x00, 'P', 'O', 'S', 'T',
        0x01, 0x00, 0x01, 0x00, 'h', 0x01, 0x00, 'v',
        0x04, 0x00, 0x00, 0x00, 'p', 'i', 'n', 'g'
    };
    Require(service.LastPayload(KindWebRequest) == expectedWebRequest, "web request wire payload matches spec byte for byte");
    Require(webReply.status == 200, "web reply status exposed");
    Require(HeaderOf(webReply, "x-resp") == "ok", "web reply headers exposed");
    Require(BodyOf(webReply) == "pong", "web reply body exposed");
    NetBridgeWebReplyRelease_nid_no_patch(&webReply);

    const char* filterTypes[] = {"a.b", "c"};
    NetBridgePushFilter_nid_no_patch(-1, 77, filterTypes, 2);
    Require(Service::WaitUntil([&] { return !service.LastPayload(KindPushFilter).empty(); }, 5000), "push filter arrived");
    const Bytes expectedPushFilter = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0x4D, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x03, 0x00, 'a', '.', 'b', 0x01, 0x00, 'c'
    };
    Require(service.LastPayload(KindPushFilter) == expectedPushFilter, "push filter wire payload matches spec byte for byte");

    NetBridgePushFilterGone_nid_no_patch(-1, 77);
    Require(Service::WaitUntil([&] { return !service.LastPayload(KindPushFilterGone).empty(); }, 5000), "push filter gone arrived");
    const Bytes expectedPushFilterGone = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0x4D, 0x00, 0x00, 0x00
    };
    Require(service.LastPayload(KindPushFilterGone) == expectedPushFilterGone, "push filter gone wire payload matches spec byte for byte");

    Require(NetBridgeActivate_nid_no_patch(7, "s1", 1) == 0, "activate succeeded");
    const Bytes expectedActivate = {
        0x07, 0x00, 0x00, 0x00,
        0x02, 0x00, 's', '1',
        0x01
    };
    Require(service.LastPayload(KindActivate) == expectedActivate, "activate wire payload matches spec byte for byte");

    NetBridgeDeactivate_nid_no_patch(7, "s1");
    Require(Service::WaitUntil([&] { return !service.LastPayload(KindDeactivate).empty(); }, 5000), "deactivate arrived");
    const Bytes expectedDeactivate = {
        0x07, 0x00, 0x00, 0x00,
        0x02, 0x00, 's', '1'
    };
    Require(service.LastPayload(KindDeactivate) == expectedDeactivate, "deactivate wire payload matches spec byte for byte");

    const Bytes peerVector = {
        0x02, 0x00, 's', '1',
        0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x04, 0x00, 'P', 'e', 'e', 'r',
        0x02, 0x00, 0x12, 0xC6,
        0x02
    };
    service.SendRaw(FakeNet::MakeFrame(KindPeer, 0, peerVector));
    NetBridgePeerView peers[4]{};
    std::uint64_t peerCounter = 0;
    Require(Service::WaitUntil([&] {
        return NetBridgePeers_nid_no_patch("s1", peers, 4, &peerCounter) == 1;
    }, 5000), "peer vector applied to session");
    Require(peers[0].accountId == 0x22, "peer account id exposed");
    Require(std::string(peers[0].handle) == "Peer", "peer handle exposed");
    Require(peers[0].address == 0xC6120002u, "peer address exposed in host byte order");
    Require(peers[0].peerState == 2, "peer state exposed");

    const Bytes peerInfoReplyVector = {
        0x01,
        0xDC, 0x05, 0x00, 0x00,
        0x00
    };
    service.SetNextPeerInfoReply(peerInfoReplyVector);
    NetBridgePeerInfoView info{};
    Require(NetBridgePeerInfo_nid_no_patch(0xC6120005u, &info) == 1, "peer info succeeded");
    const Bytes expectedPeerInfo = {0x05, 0x00, 0x12, 0xC6};
    Require(service.LastPayload(KindPeerInfo) == expectedPeerInfo, "peer info wire payload matches spec byte for byte");
    Require(info.known == 1, "peer info reply known exposed");
    Require(info.rttUs == 1500, "peer info reply rtt_us exposed");
    Require(info.relayed == 0, "peer info reply relayed exposed");

    Require(NetBridgeBind_nid_no_patch(9999, 1, 54321) == 0, "bind succeeded");
    const Bytes expectedBind = {0x0F, 0x27, 0x01, 0x31, 0xD4};
    Require(service.LastPayload(KindBind) == expectedBind, "bind wire payload matches spec byte for byte");

    NetBridgeUnbind_nid_no_patch(9999, 1);
    Require(Service::WaitUntil([&] { return !service.LastPayload(KindUnbind).empty(); }, 5000), "unbind arrived");
    const Bytes expectedUnbind = {0x0F, 0x27, 0x01};
    Require(service.LastPayload(KindUnbind) == expectedUnbind, "unbind wire payload matches spec byte for byte");

    std::filesystem::remove(observePath, ec);
}

}

int main() {
    TestHeaderLayoutMatchesSpec();
    TestUnsupportedRoundTrip();
    TestRejectRoundTrip();
    TestTruncatedPayloadHandled();
    TestTrailingFieldsIgnored();
    TestPayloadLengthLimit();

    TestShareKindsMatchSpec();
    TestRealBridgeWireVectors();

    std::puts("bridge protocol frame tests: ok (frame kinds verified against spec and real bridge, share play included)");
    return 0;
}
