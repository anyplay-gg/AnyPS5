#include "prx/libSceNetCtl/src/NetBridgeWire.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

using namespace NetBridgeWire;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "bridge protocol test failure: %s\n", message);
        std::fflush(stderr);
        std::abort();
    }
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

void TestAddressEncoding() {
    Builder b;
    b.Addr(0xC6120001);
    const auto bytes = b.Take();
    Require(bytes.size() == 4, "address is 4 bytes");
    Require(bytes[0] == 0x01 && bytes[1] == 0x00 && bytes[2] == 0x12 && bytes[3] == 0xC6, "address matches little-endian spec");

    Reader r(bytes.data(), bytes.size());
    Require(r.Addr() == 0xC6120001, "address decodes correctly");
    Require(r.Ok(), "reader ok after address");
}

void TestHelloRoundTrip() {
    const std::vector<std::uint8_t> token = {0xAA, 0xBB, 0xCC, 0xDD};
    Builder b;
    b.U16(ProtocolVersion).Raw(token.data(), token.size());
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {0x00, 0x00, 0xAA, 0xBB, 0xCC, 0xDD};
    Require(payload == expected, "hello fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint16_t version = r.U16();
    Require(r.Ok() && version == ProtocolVersion, "hello version matches");
    Require(r.Remaining() == token.size(), "hello token size matches");
    std::vector<std::uint8_t> back(r.Remaining());
    for (std::size_t i = 0; i < back.size(); ++i) back[i] = r.U8();
    Require(back == token, "hello token bytes match");
}

void TestWelcomeRoundTrip() {
    const std::uint8_t key[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    Builder b;
    b.U16(ProtocolVersion).U16(9999).U16(8888).Raw(key, sizeof(key));
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x00, 0x00,
        0x0F, 0x27,
        0xB8, 0x22,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };
    Require(payload == expected, "welcome fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint16_t version = r.U16();
    const std::uint16_t udpPort = r.U16();
    const std::uint16_t tcpPort = r.U16();
    Require(r.Remaining() >= 16, "welcome has 16-byte key");
    std::uint8_t keyBack[16];
    for (std::size_t i = 0; i < 16; ++i) keyBack[i] = r.U8();

    Require(r.Ok(), "welcome decodes ok");
    Require(version == ProtocolVersion, "welcome version");
    Require(udpPort == 9999, "welcome udp port");
    Require(tcpPort == 8888, "welcome tcp port");
    Require(std::memcmp(key, keyBack, 16) == 0, "welcome stream key");
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

void TestUnsupportedRoundTrip() {
    const auto frame = EncodeFrame(KindUnsupported, 42, {});
    const FrameHeader hdr = ParseHeader(frame.data());
    Require(hdr.kind == KindUnsupported, "unsupported kind");
    Require(hdr.request == 42, "unsupported request");
    Require(hdr.length == 0, "unsupported empty payload");
}

void TestStateRoundTrip() {
    Builder b;
    b.U8(1)
     .U64(0x0102030405060708ULL)
     .Str("Any")
     .U8(2)
     .Addr(0xC6120001);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01,
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
        0x03, 0x00, 'A', 'n', 'y',
        0x02,
        0x01, 0x00, 0x12, 0xC6
    };
    Require(payload == expected, "state fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint8_t signedIn = r.U8();
    const std::uint64_t accountId = r.U64();
    const std::string handle = r.Str();
    const std::uint8_t natType = r.U8();
    const std::uint32_t addr = r.Addr();

    Require(r.Ok(), "state reader ok");
    Require(signedIn == 1, "state signedIn");
    Require(accountId == 0x0102030405060708ULL, "state accountId");
    Require(handle == "Any", "state handle");
    Require(natType == 2, "state natType");
    Require(addr == 0xC6120001, "state address");
}

void TestFeatureRoundTrip() {
    Builder b;
    b.I32(1).U32(0x12345678);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01, 0x00, 0x00, 0x00,
        0x78, 0x56, 0x34, 0x12
    };
    Require(payload == expected, "feature fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::uint32_t feature = r.U32();
    Require(r.Ok(), "feature reader ok");
    Require(userId == 1, "feature userId");
    Require(feature == 0x12345678, "feature flag");
}

void TestWebRequestRoundTrip() {
    const std::vector<std::uint8_t> body = {0xAA, 0xBB};
    const Pairs headers = {{"k", "v"}};
    Builder b;
    b.I32(1)
     .Str("g")
     .Str("/p")
     .Str("GET")
     .WritePairs(headers)
     .Bytes(body.data(), body.size());
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 'g',
        0x02, 0x00, '/', 'p',
        0x03, 0x00, 'G', 'E', 'T',
        0x01, 0x00, 0x01, 0x00, 'k', 0x01, 0x00, 'v',
        0x02, 0x00, 0x00, 0x00, 0xAA, 0xBB
    };
    Require(payload == expected, "web request fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::string apiGroup = r.Str();
    const std::string path = r.Str();
    const std::string method = r.Str();
    const Pairs parsedHeaders = r.ReadPairs();
    const auto parsedBody = r.Bytes();

    Require(r.Ok(), "web request reader ok");
    Require(userId == 1, "web request userId");
    Require(apiGroup == "g", "web request apiGroup");
    Require(path == "/p", "web request path");
    Require(method == "GET", "web request method");
    Require(parsedHeaders == headers, "web request headers");
    Require(parsedBody.second == 2 && parsedBody.first[0] == 0xAA && parsedBody.first[1] == 0xBB, "web request body");
}

void TestWebReplyRoundTrip() {
    const std::vector<std::uint8_t> body = {0x11, 0x22, 0x33};
    const Pairs headers = {{"h", "1"}};
    Builder b;
    b.U16(200).WritePairs(headers).Bytes(body.data(), body.size());
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0xC8, 0x00,
        0x01, 0x00, 0x01, 0x00, 'h', 0x01, 0x00, '1',
        0x03, 0x00, 0x00, 0x00, 0x11, 0x22, 0x33
    };
    Require(payload == expected, "web reply fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint16_t status = r.U16();
    const Pairs parsedHeaders = r.ReadPairs();
    const auto parsedBody = r.Bytes();

    Require(r.Ok(), "web reply reader ok");
    Require(status == 200, "web reply status");
    Require(parsedHeaders == headers, "web reply headers");
    Require(parsedBody.second == 3 && parsedBody.first[0] == 0x11, "web reply body");
}

void TestPushFilterRoundTrip() {
    const std::vector<std::string> types = {"a"};
    Builder b;
    b.I32(-1).U32(77).ListOfStrings(types);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0x4D, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 'a'
    };
    Require(payload == expected, "push filter fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::uint32_t filter = r.U32();
    const std::vector<std::string> parsedTypes = r.ListOfStrings();

    Require(r.Ok(), "push filter reader ok");
    Require(userId == -1, "push filter userId");
    Require(filter == 77, "push filter id");
    Require(parsedTypes == types, "push filter types");
}

void TestPushFilterGoneRoundTrip() {
    Builder b;
    b.I32(42).U32(99);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x2A, 0x00, 0x00, 0x00,
        0x63, 0x00, 0x00, 0x00
    };
    Require(payload == expected, "push filter gone fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::uint32_t filter = r.U32();
    Require(r.Ok(), "push filter gone reader ok");
    Require(userId == 42, "push filter gone userId");
    Require(filter == 99, "push filter gone id");
}

void TestPushEventRoundTrip() {
    const std::vector<std::uint8_t> data = {0x55};
    const Pairs extended = {{"x", "y"}};
    Builder b;
    b.I32(1)
     .U32(10)
     .Str("t")
     .U64(100)
     .U64(200)
     .Bytes(data.data(), data.size())
     .WritePairs(extended);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01, 0x00, 0x00, 0x00,
        0x0A, 0x00, 0x00, 0x00,
        0x01, 0x00, 't',
        0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x55,
        0x01, 0x00, 0x01, 0x00, 'x', 0x01, 0x00, 'y'
    };
    Require(payload == expected, "push event fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::uint32_t filter = r.U32();
    const std::string dataType = r.Str();
    const std::uint64_t from = r.U64();
    const std::uint64_t to = r.U64();
    const auto parsedData = r.Bytes();
    const Pairs parsedExt = r.ReadPairs();

    Require(r.Ok(), "push event reader ok");
    Require(userId == 1 && filter == 10 && dataType == "t", "push event header fields");
    Require(from == 100 && to == 200, "push event routing");
    Require(parsedData.second == 1 && parsedData.first[0] == 0x55, "push event payload");
    Require(parsedExt == extended, "push event extended");
}

void TestActivateRoundTrip() {
    Builder b;
    b.I32(1).Str("s").U8(2);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 's',
        0x02
    };
    Require(payload == expected, "activate fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::string session = r.Str();
    const std::uint8_t sessionKind = r.U8();
    Require(r.Ok(), "activate reader ok");
    Require(userId == 1, "activate userId");
    Require(session == "s", "activate session");
    Require(sessionKind == 2, "activate sessionKind");
}

void TestActivatedRoundTrip() {
    Builder b;
    b.U8(0);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {0x00};
    Require(payload == expected, "activated fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint8_t result = r.U8();
    Require(r.Ok() && result == 0, "activated result ok");
}

void TestDeactivateRoundTrip() {
    Builder b;
    b.I32(1).Str("s");
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 's'
    };
    Require(payload == expected, "deactivate fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::string session = r.Str();
    Require(r.Ok(), "deactivate reader ok");
    Require(userId == 1 && session == "s", "deactivate fields");
}

void TestPeerRoundTrip() {
    Builder b;
    b.Str("s")
     .U64(1)
     .Str("h")
     .Addr(0xC6120002)
     .U8(3);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01, 0x00, 's',
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 'h',
        0x02, 0x00, 0x12, 0xC6,
        0x03
    };
    Require(payload == expected, "peer fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::string session = r.Str();
    const std::uint64_t accountId = r.U64();
    const std::string handle = r.Str();
    const std::uint32_t addr = r.Addr();
    const std::uint8_t peerState = r.U8();

    Require(r.Ok(), "peer reader ok");
    Require(session == "s" && accountId == 1 && handle == "h", "peer identities");
    Require(addr == 0xC6120002 && peerState == 3, "peer address and state");
}

void TestPeerInfoRoundTrip() {
    Builder b;
    b.Addr(0xC6120005);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {0x05, 0x00, 0x12, 0xC6};
    Require(payload == expected, "peer info fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint32_t addr = r.Addr();
    Require(r.Ok() && addr == 0xC6120005, "peer info address");
}

void TestPeerInfoReplyRoundTrip() {
    Builder b;
    b.U8(1).U8(0).U32(1500);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01,
        0x00,
        0xDC, 0x05, 0x00, 0x00
    };
    Require(payload == expected, "peer info reply fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint8_t known = r.U8();
    const std::uint8_t relayed = r.U8();
    const std::uint32_t rttUs = r.U32();
    Require(r.Ok(), "peer info reply reader ok");
    Require(known == 1 && relayed == 0 && rttUs == 1500, "peer info reply fields");
}

void TestBindRoundTrip() {
    Builder b;
    b.U16(9999).U8(1).U16(54321);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {0x0F, 0x27, 0x01, 0x31, 0xD4};
    Require(payload == expected, "bind fixed byte vector matches spec exactly");

    Reader r(payload.data(), payload.size());
    const std::uint16_t vport = r.U16();
    const std::uint8_t transport = r.U8();
    const std::uint16_t loopback = r.U16();
    Require(r.Ok(), "bind reader ok");
    Require(vport == 9999, "bind vport");
    Require(transport == 1, "bind transport");
    Require(loopback == 54321, "bind loopback");
}

void TestBoundRoundTrip() {
    Builder b;
    b.U8(0);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {0x00};
    Require(payload == expected, "bound fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint8_t result = r.U8();
    Require(r.Ok() && result == 0, "bound result ok");
}

void TestUnbindRoundTrip() {
    Builder b;
    b.U16(9999).U8(1);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {0x0F, 0x27, 0x01};
    Require(payload == expected, "unbind fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint16_t vport = r.U16();
    const std::uint8_t transport = r.U8();
    Require(r.Ok(), "unbind reader ok");
    Require(vport == 9999 && transport == 1, "unbind fields");
}

void TestIntentRoundTrip() {
    const Pairs properties = {{"s", "1"}};
    Builder b;
    b.I32(-1).Str("join").WritePairs(properties);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0x04, 0x00, 'j', 'o', 'i', 'n',
        0x01, 0x00, 0x01, 0x00, 's', 0x01, 0x00, '1'
    };
    Require(payload == expected, "intent fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::string intentType = r.Str();
    const Pairs parsedProps = r.ReadPairs();
    Require(r.Ok(), "intent reader ok");
    Require(userId == -1 && intentType == "join", "intent type");
    Require(parsedProps == properties, "intent properties");
}

void TestInvitationDialogRoundTrip() {
    Builder b;
    b.I32(1).Str("s").U16(4);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 's',
        0x04, 0x00
    };
    Require(payload == expected, "invitation dialog fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::int32_t userId = r.I32();
    const std::string session = r.Str();
    const std::uint16_t max = r.U16();
    Require(r.Ok(), "invitation dialog reader ok");
    Require(userId == 1 && session == "s" && max == 4, "invitation dialog fields");
}

void TestInvitationDialogReplyRoundTrip() {
    Builder b;
    b.U8(0).U16(3);
    const auto payload = b.Take();

    const std::vector<std::uint8_t> expected = {0x00, 0x03, 0x00};
    Require(payload == expected, "invitation dialog reply fixed byte vector matches");

    Reader r(payload.data(), payload.size());
    const std::uint8_t result = r.U8();
    const std::uint16_t sent = r.U16();
    Require(r.Ok(), "invitation dialog reply reader ok");
    Require(result == 0 && sent == 3, "invitation dialog reply fields");
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

void TestPayloadLengthLimit() {
    Require(MaxPayload == 16 * 1024 * 1024, "MaxPayload constant is 16 MiB");

    Builder bBytes;
    const std::uint8_t dummy = 0;
    bBytes.Bytes(&dummy, static_cast<std::uint64_t>(MaxPayload) + 1ULL);
    Require(!bBytes.Ok(), "Bytes exceeding MaxPayload fails");

    Builder bRaw;
    bRaw.Raw(&dummy, static_cast<std::size_t>(MaxPayload) + 1UL);
    Require(!bRaw.Ok(), "Raw exceeding MaxPayload fails");
}

}

int main() {
    TestHeaderLayoutMatchesSpec();
    TestAddressEncoding();

    TestHelloRoundTrip();
    TestWelcomeRoundTrip();
    TestRejectRoundTrip();
    TestUnsupportedRoundTrip();

    TestStateRoundTrip();
    TestFeatureRoundTrip();

    TestWebRequestRoundTrip();
    TestWebReplyRoundTrip();
    TestPushFilterRoundTrip();
    TestPushFilterGoneRoundTrip();
    TestPushEventRoundTrip();

    TestActivateRoundTrip();
    TestActivatedRoundTrip();
    TestDeactivateRoundTrip();
    TestPeerRoundTrip();
    TestPeerInfoRoundTrip();
    TestPeerInfoReplyRoundTrip();
    TestBindRoundTrip();
    TestBoundRoundTrip();
    TestUnbindRoundTrip();

    TestIntentRoundTrip();
    TestInvitationDialogRoundTrip();
    TestInvitationDialogReplyRoundTrip();

    TestTruncatedPayloadHandled();
    TestTrailingFieldsIgnored();
    TestPayloadLengthLimit();

    std::puts("bridge protocol frame tests: ok (23 frame kinds verified against spec)");
    return 0;
}
