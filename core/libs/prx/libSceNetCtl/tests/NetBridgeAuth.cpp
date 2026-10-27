#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/src/NetBridgeAuth.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>


namespace {

using FakeNet::Bytes;
using FakeNet::Require;
using FakeNet::Service;
using NetBridgeAuth::Nonce;
using NetBridgeAuth::Proof;

std::string Hex(const std::uint8_t* data, std::size_t size) {
    static const char digits[] = "0123456789abcdef";
    std::string text;
    for (std::size_t index = 0; index < size; ++index) {
        text.push_back(digits[data[index] >> 4]);
        text.push_back(digits[data[index] & 15]);
    }
    return text;
}

std::string Hex(const Proof& proof) {
    return Hex(proof.data(), proof.size());
}

Nonce Counting(std::uint8_t from) {
    Nonce nonce{};
    for (std::size_t index = 0; index < nonce.size(); ++index) nonce[index] = static_cast<std::uint8_t>(from + index);
    return nonce;
}

std::vector<std::uint8_t> Counting32(std::uint8_t from) {
    const Nonce nonce = Counting(from);
    return std::vector<std::uint8_t>(nonce.begin(), nonce.end());
}

const std::uint8_t* Raw(const std::string& text) {
    return reinterpret_cast<const std::uint8_t*>(text.data());
}

void TestSha256AndHmacVectors() {
    const std::string abc = "abc";
    Require(Hex(NetBridgeAuth::Sha256Digest(Raw(abc), abc.size())) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "sha-256 of abc");
    Require(Hex(NetBridgeAuth::Sha256Digest(nullptr, 0)) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "sha-256 of nothing");
    const std::string twoBlocks = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    Require(Hex(NetBridgeAuth::Sha256Digest(Raw(twoBlocks), twoBlocks.size())) == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
            "sha-256 of the two-block message");

    const std::vector<std::uint8_t> key1(20, 0x0b);
    const std::string hiThere = "Hi There";
    Require(Hex(NetBridgeAuth::Hmac(key1.data(), key1.size(), Raw(hiThere), hiThere.size())) ==
                "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7",
            "hmac rfc 4231 case 1");
    const std::string jefe = "Jefe";
    const std::string question = "what do ya want for nothing?";
    Require(Hex(NetBridgeAuth::Hmac(Raw(jefe), jefe.size(), Raw(question), question.size())) ==
                "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843",
            "hmac rfc 4231 case 2");
    const std::vector<std::uint8_t> key6(131, 0xaa);
    const std::string hashFirst = "Test Using Larger Than Block-Size Key - Hash Key First";
    Require(Hex(NetBridgeAuth::Hmac(key6.data(), key6.size(), Raw(hashFirst), hashFirst.size())) ==
                "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54",
            "hmac rfc 4231 case 6");
}

void TestProofsMatchTheSpecVectors() {
    const Nonce bridgeNonce = Counting(0x20);
    const Nonce serviceNonce = Counting(0x40);
    const std::vector<std::uint8_t> counting = Counting32(0x00);
    Require(Hex(NetBridgeAuth::MakeProof(counting, NetBridgeAuth::RoleBridge, bridgeNonce, serviceNonce)) ==
                "13d46f7dc5c37ff12cf336554acb4e1981ffbcc22ce374d68289bed0fd8f9651",
            "bridge proof, 32-byte token");
    Require(Hex(NetBridgeAuth::MakeProof(counting, NetBridgeAuth::RoleService, bridgeNonce, serviceNonce)) ==
                "f4ec03176babba0072aaf8625da4e41230e772613a0062121d5597db8cffbf69",
            "service proof, 32-byte token");
    const std::vector<std::uint8_t> shortToken = {0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x01, 0xFE};
    Require(Hex(NetBridgeAuth::MakeProof(shortToken, NetBridgeAuth::RoleBridge, bridgeNonce, serviceNonce)) ==
                "c561da91325fb75af59d36d89ff0057ffa6f832333ef35ce9719a3e991ec2fa3",
            "bridge proof, short token");
    Require(Hex(NetBridgeAuth::MakeProof(shortToken, NetBridgeAuth::RoleService, bridgeNonce, serviceNonce)) ==
                "1a6117546f912bebe685c274f3bfc080292dee261310c8941a6bd530567bd792",
            "service proof, short token");
    const std::vector<std::uint8_t> longToken(100, 0xaa);
    Require(Hex(NetBridgeAuth::MakeProof(longToken, NetBridgeAuth::RoleBridge, bridgeNonce, serviceNonce)) ==
                "a68cfdba801506b73275bad33fdb8b28a8d36e4e7d5f14ca82178ea2593f122f",
            "bridge proof, token longer than the hmac block");

    const Proof first = NetBridgeAuth::MakeProof(counting, NetBridgeAuth::RoleBridge, bridgeNonce, serviceNonce);
    Proof second = first;
    Require(NetBridgeAuth::SameProof(first, second), "equal proofs compare equal");
    second[31] ^= 1;
    Require(!NetBridgeAuth::SameProof(first, second), "a proof differing in the last byte differs");
    second = first;
    second[0] ^= 0x80;
    Require(!NetBridgeAuth::SameProof(first, second), "a proof differing in the first byte differs");
    Require(!NetBridgeAuth::SameProof(first, NetBridgeAuth::MakeProof(counting, NetBridgeAuth::RoleService, bridgeNonce, serviceNonce)),
            "the roles give different proofs");
}

void TestBridgeAnswersOnlyAServiceThatProvesTheToken() {
    const Bytes token = {0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x01, 0xFE};
    Service service(token);
    service.ActAsImpostor(true);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));

    Require(NetBridgeEnabled_nid_no_patch() == 1, "bridge enabled");
    Require(Service::WaitUntil([&] { return service.Unanswered() >= 1; }, 8000), "the bridge closed without an auth frame");
    Require(service.Welcomed() == 0 && service.Rejected() == 0, "no handshake finished with the impostor");
    for (const auto& frame : service.ReceivedFrames()) Require(frame.kind != 0x0005, "no auth frame reached the impostor");

    service.ActAsImpostor(false);
    Require(Service::WaitUntil([&] { return service.Welcomed() == 1; }, 8000), "the bridge connects to the real service");
    const Bytes hello = service.LastPayload(0x0001);
    Require(hello.size() == 34 && hello[0] == 1 && hello[1] == 0, "hello is the version and a 32-byte nonce");
    for (const auto& frame : service.ReceivedFrames()) {
        Require(std::search(frame.payload.begin(), frame.payload.end(), token.begin(), token.end()) == frame.payload.end(),
                "the token is in no frame the bridge sent");
    }
    Require(service.LastPayload(0x0005).size() == 32, "auth is one 32-byte proof");
}

}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "vectors";
    if (mode == "vectors") {
        TestSha256AndHmacVectors();
        TestProofsMatchTheSpecVectors();
    } else if (mode == "impostor") {
        TestBridgeAnswersOnlyAServiceThatProvesTheToken();
    } else {
        std::fprintf(stderr, "unknown mode %s\n", mode.c_str());
        return 2;
    }
    std::printf("bridge handshake tests (%s): ok\n", mode.c_str());
    return 0;
}
