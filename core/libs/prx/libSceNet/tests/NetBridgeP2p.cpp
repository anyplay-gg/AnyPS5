#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
int APS5_VABI sceNetSocket(const char*, int, int, int);
int APS5_VABI sceNetBind_nid_postfix(int, const void*, std::uint32_t);
int APS5_VABI sceNetGetsockname(int, void*, std::uint32_t*);
int APS5_VABI sceNetSetsockopt(int, int, int, const void*, std::uint32_t);
std::int64_t APS5_VABI sceNetSendto(int, const void*, std::size_t, int, const void*, std::uint32_t);
std::int64_t APS5_VABI sceNetRecvfrom(int, void*, std::size_t, int, void*, std::uint32_t*);
int APS5_VABI sceNetSocketClose(int);
int* APS5_VABI sceNetErrnoLoc();
int APS5_VABI sceNetListen(int, int);
int APS5_VABI sceNetAccept(int, void*, std::uint32_t*);
int APS5_VABI sceNetConnect(int, const void*, std::uint32_t);
int APS5_VABI sceNetGetpeername(int, void*, std::uint32_t*);
std::int64_t APS5_VABI sceNetSend(int, const void*, std::size_t, int);
std::int64_t APS5_VABI sceNetRecv(int, void*, std::size_t, int);
}

namespace {

using FakeNet::Bytes;
using FakeNet::Require;
using FakeNet::Service;

constexpr int AfInet = 2;
constexpr int SockDgramP2p = 6;
constexpr int SockStreamP2p = 10;
constexpr int EConnAborted = 53;
constexpr int ENotConn = 57;
constexpr int SolSocket = 0xFFFF;
constexpr int SoNbio = 0x1200;
constexpr int EAgain = 35;
constexpr int EAddrInUse = 48;
constexpr std::uint32_t Self = 0xC6120001u;
constexpr std::uint32_t Peer = 0xC6120002u;
constexpr std::uint16_t GamePort = 3658;

std::vector<std::uint8_t> P2pAddress(std::uint32_t address, std::uint16_t port, std::uint16_t vport) {
    std::vector<std::uint8_t> bytes(16, 0);
    bytes[0] = 16;
    bytes[1] = AfInet;
    bytes[2] = static_cast<std::uint8_t>(port >> 8);
    bytes[3] = static_cast<std::uint8_t>(port);
    for (int index = 0; index < 4; ++index) bytes[4 + index] = static_cast<std::uint8_t>(address >> (24 - 8 * index));
    bytes[8] = static_cast<std::uint8_t>(vport >> 8);
    bytes[9] = static_cast<std::uint8_t>(vport);
    return bytes;
}

std::int64_t ReceiveWithin(int socket, std::uint8_t* buffer, std::size_t size, std::uint8_t* from, std::uint32_t* fromLength) {
    std::int64_t got = -1;
    Service::WaitUntil([&] {
        *fromLength = 16;
        got = sceNetRecvfrom(socket, buffer, size, 0, from, fromLength);
        return got >= 0;
    }, 5000);
    return got;
}

void BridgeCalls(Service& service) {
    NetBridgeTrafficView traffic{};
    Require(NetBridgeTraffic_nid_no_patch(&traffic) == 1, "traffic ports while connected");
    Require(traffic.udpPort == service.DatagramPort() && traffic.tcpPort == service.StreamPort(), "traffic ports from welcome");
    for (int index = 0; index < 16; ++index) Require(traffic.streamKey[index] == index, "stream key from welcome");

    Require(NetBridgeActivate_nid_no_patch(268435456, "session-a", 1) == 0, "activate a known session");
    Require(NetBridgeActivate_nid_no_patch(268435456, "missing", 2) == 1, "activate an unknown session");
    const auto activations = service.Activations();
    Require(activations.size() == 2 && activations[0].session == "session-a" && activations[0].sessionKind == 1 && activations[1].sessionKind == 2, "activate frames");

    std::uint64_t before = 0;
    Require(NetBridgePeers_nid_no_patch("session-a", nullptr, 0, &before) == 0, "no peers yet");
    service.SendPeer("session-a", 77, "Bobby", Peer, 1);
    service.SendPeer("session-b", 78, "Carol", 0xC6120003u, 2);
    Require(Service::WaitUntil([&] { std::uint64_t now = 0; NetBridgePeers_nid_no_patch("session-b", nullptr, 0, &now); return now >= before + 2; }, 5000), "peer frames arrive");
    service.SendPeer("session-a", 77, "Bobby", Peer, 2);
    Require(Service::WaitUntil([&] {
        NetBridgePeerView view{};
        return NetBridgePeers_nid_no_patch("session-a", &view, 1, nullptr) == 1 && view.peerState == 2;
    }, 5000), "peer state changes");
    NetBridgePeerView peers[4]{};
    Require(NetBridgePeers_nid_no_patch("session-a", peers, 4, nullptr) == 1, "peers of one session only");
    Require(peers[0].accountId == 77 && std::string(peers[0].handle) == "Bobby" && peers[0].address == Peer, "peer fields");
    service.SendPeer("session-a", 79, "Bad\x01Name", 0xC6120004u, 2);
    service.SendPeer("session-a", 80, "Dan", 0xC6120005u, 9);
    service.SendPeer("session-c", 81, "Eve", 0xC6120006u, 1);
    Require(Service::WaitUntil([] { return NetBridgePeers_nid_no_patch("session-c", nullptr, 0, nullptr) == 1; }, 5000), "later peer frame arrives");
    Require(NetBridgePeers_nid_no_patch("session-a", nullptr, 0, nullptr) == 1, "malformed peers are skipped");

    NetBridgePeerInfoView info{};
    Require(NetBridgePeerInfo_nid_no_patch(Peer, &info) == 1 && info.known == 1 && info.rttUs == 1234 && info.relayed == 1, "peer info of a known peer");
    Require(NetBridgePeerInfo_nid_no_patch(0xC6120009u, &info) == 1 && info.known == 0, "peer info of an unknown address");

    NetBridgeDeactivate_nid_no_patch(268435456, "session-a");
    Require(Service::WaitUntil([&] { return service.Deactivations().size() == 1; }, 5000), "deactivate frame");
    Require(service.Deactivations()[0].second == "session-a", "deactivate fields");
    Require(NetBridgePeers_nid_no_patch("session-a", nullptr, 0, nullptr) == 0, "deactivate drops the session's peers");

    Require(NetBridgeBind_nid_no_patch(900, 1, 40000) == 0, "bind a virtual port");
    Require(NetBridgeBind_nid_no_patch(900, 1, 40001) == 1, "bind the same virtual port again");
    NetBridgeUnbind_nid_no_patch(900, 1);
    Require(Service::WaitUntil([&] { return service.Unbinds().size() == 1 && service.LiveBinds().empty(); }, 5000), "unbind frame");
}

void Sockets(Service& service) {
    const int socket = sceNetSocket("p2p", AfInet, SockDgramP2p, 0);
    Require(socket >= 0, "peer-to-peer datagram socket with the bridge on");
    const auto local = P2pAddress(0, GamePort, 3);
    Require(sceNetBind_nid_postfix(socket, local.data(), 16) == 0, "bind a virtual port");
    Require(Service::WaitUntil([&] { return service.LiveBinds().size() == 1; }, 5000), "bind reached the service");
    const auto bind = service.LiveBinds()[0];
    Require(bind.vport == 3 && bind.transport == 1 && bind.loopbackPort != 0, "bind fields");

    std::uint8_t name[16]{};
    std::uint32_t nameLength = sizeof(name);
    Require(sceNetGetsockname(socket, name, &nameLength) == 0 && nameLength == 16, "getsockname");
    Require(std::memcmp(name, P2pAddress(Self, GamePort, 3).data(), 16) == 0, "getsockname reports the virtual address and port");

    const int second = sceNetSocket("p2p", AfInet, SockDgramP2p, 0);
    Require(second >= 0, "second socket");
    Require(sceNetBind_nid_postfix(second, local.data(), 16) < 0 && *sceNetErrnoLoc() == EAddrInUse, "a bound virtual port is in use");
    Require(sceNetSocketClose(second) == 0, "close the second socket");

    const auto peer = P2pAddress(Peer, GamePort, 7);
    const char message[] = "hello peer";
    Require(sceNetSendto(socket, message, sizeof(message), 0, peer.data(), 16) == static_cast<std::int64_t>(sizeof(message)), "sendto returns the game's length");
    Require(Service::WaitUntil([&] { return service.Datagrams().size() == 1; }, 5000), "datagram reached the service");
    const auto sent = service.Datagrams()[0];
    Require(sent.fromPort == bind.loopbackPort, "sent from the bound loopback port");
    Require(sent.address == Peer && sent.vport == 7 && sent.sourceVport == 3, "datagram header");
    Require(sent.payload == Bytes(message, message + sizeof(message)), "datagram payload");

    const Bytes reply = {0x10, 0x20, 0x30, 0x40, 0x50};
    Require(service.InjectDatagram(Peer, 3, 7, reply), "service delivers a datagram");
    std::uint8_t buffer[64]{};
    std::uint8_t from[16]{};
    std::uint32_t fromLength = 16;
    Require(ReceiveWithin(socket, buffer, sizeof(buffer), from, &fromLength) == static_cast<std::int64_t>(reply.size()), "recvfrom returns the game's bytes");
    Require(std::memcmp(buffer, reply.data(), reply.size()) == 0, "received payload");
    Require(fromLength == 16 && std::memcmp(from, peer.data(), 16) == 0, "sender address, port and virtual port");

    Require(service.InjectDatagram(Peer, 3, 7, {1, 2, 3, 4, 5, 6, 7, 8}), "a longer datagram");
    std::uint8_t small[3]{};
    Require(ReceiveWithin(socket, small, sizeof(small), from, &fromLength) == 3 && small[0] == 1 && small[2] == 3, "a datagram is cut to the buffer");

    const int nonblocking = 1;
    Require(sceNetSetsockopt(socket, SolSocket, SoNbio, &nonblocking, sizeof(nonblocking)) == 0, "non-blocking");
    fromLength = 16;
    Require(sceNetRecvfrom(socket, buffer, sizeof(buffer), 0, from, &fromLength) < 0 && *sceNetErrnoLoc() == EAgain, "nothing waiting");

    Require(NetBridgeActivate_nid_no_patch(268435456, "session-r", 1) == 0, "activate before a reconnect");
    const auto replayed = [&] {
        std::size_t count = 0;
        for (const auto& call : service.Activations()) count += call.session == "session-r" ? 1 : 0;
        return count;
    };
    Require(replayed() == 1, "one activation before the reconnect");
    service.DropConnection();
    Require(Service::WaitUntil([&] { return service.Welcomed() == 2 && service.LiveBinds().size() == 1; }, 10000), "binds are replayed after a reconnect");
    Require(Service::WaitUntil([&] { return replayed() == 2; }, 5000), "activations are replayed after a reconnect");

    Require(sceNetSocketClose(socket) == 0, "close");
    Require(Service::WaitUntil([&] { return service.LiveBinds().empty(); }, 5000), "close unbinds the virtual port");
}

std::int64_t ReceiveAll(int socket, std::uint8_t* buffer, std::size_t size) {
    std::size_t total = 0;
    Service::WaitUntil([&] {
        const auto got = sceNetRecv(socket, buffer + total, size - total, 0);
        if (got > 0) total += static_cast<std::size_t>(got);
        return total == size;
    }, 5000);
    return static_cast<std::int64_t>(total);
}

Bytes ReadFromService(FakeNet::Handle handle, std::size_t size) {
    Bytes bytes(size, 0);
    Require(FakeNet::ReadExact(handle, bytes.data(), bytes.size()), "service reads the stream");
    return bytes;
}

void Streams(Service& service) {
    const int listener = sceNetSocket("p2p-stream", AfInet, SockStreamP2p, 0);
    Require(listener >= 0, "peer-to-peer stream socket with the bridge on");
    const auto listenerAddress = P2pAddress(0, GamePort, 4);
    Require(sceNetBind_nid_postfix(listener, listenerAddress.data(), 16) == 0, "bind a stream virtual port");
    Require(service.LiveBinds().empty(), "a stream bind registers nothing before listen");
    Require(sceNetListen(listener, 4) == 0, "listen");
    Require(Service::WaitUntil([&] { return service.LiveBinds().size() == 1; }, 5000), "listen registered the virtual port");
    const auto streamBind = service.LiveBinds()[0];
    Require(streamBind.vport == 4 && streamBind.transport == 2 && streamBind.loopbackPort != 0, "stream bind fields");

    const FakeNet::Handle incoming = service.OpenStreamTo(Peer, 4, 9);
    Require(incoming != FakeNet::InvalidHandle, "service opens a stream to the listener");
    std::uint8_t from[16]{};
    std::uint32_t fromLength = 16;
    const int accepted = sceNetAccept(listener, from, &fromLength);
    Require(accepted >= 0, "accept a peer stream");
    Require(fromLength == 16 && std::memcmp(from, P2pAddress(Peer, GamePort, 9).data(), 16) == 0, "accept reports the peer's virtual address and port");
    std::uint8_t peerName[16]{};
    std::uint32_t peerLength = 16;
    Require(sceNetGetpeername(accepted, peerName, &peerLength) == 0 && std::memcmp(peerName, from, 16) == 0, "getpeername of the accepted stream");
    Require(FakeNet::WriteExact(incoming, Bytes{'h', 'i'}), "peer writes");
    std::uint8_t greeting[2]{};
    Require(ReceiveAll(accepted, greeting, sizeof(greeting)) == 2 && greeting[0] == 'h' && greeting[1] == 'i', "game reads the peer's bytes");
    Require(sceNetSend(accepted, "ok", 2, 0) == 2, "game writes back");
    Require(ReadFromService(incoming, 2) == Bytes({'o', 'k'}), "peer reads the game's bytes");
    FakeNet::CloseHandleSafe(incoming);
    Require(sceNetSocketClose(accepted) == 0, "close the accepted stream");

    const FakeNet::Handle forged = service.OpenStreamTo(Peer, 4, 9, false);
    Require(forged != FakeNet::InvalidHandle, "a stream with the wrong key");
    Require(sceNetAccept(listener, nullptr, nullptr) < 0 && *sceNetErrnoLoc() == EConnAborted, "a wrong key is refused");
    FakeNet::CloseHandleSafe(forged);

    const int client = sceNetSocket("p2p-stream", AfInet, SockStreamP2p, 0);
    Require(client >= 0, "client stream socket");
    const auto clientAddress = P2pAddress(0, GamePort, 5);
    Require(sceNetBind_nid_postfix(client, clientAddress.data(), 16) == 0, "bind the client");
    std::uint8_t unknownPeer[16]{};
    std::uint32_t unknownLength = 16;
    Require(sceNetGetpeername(client, unknownPeer, &unknownLength) < 0 && *sceNetErrnoLoc() == ENotConn, "no peer before connect");
    const auto target = P2pAddress(Peer, GamePort, 6);
    Require(sceNetConnect(client, target.data(), 16) == 0, "connect to a peer");
    Require(Service::WaitUntil([&] { return service.Streams().size() == 1; }, 5000), "stream reached the service");
    const auto call = service.Streams()[0];
    Require(call.keyMatches && call.address == Peer && call.vport == 6 && call.sourceVport == 5, "stream preamble");
    Require(sceNetSend(client, "ping", 4, 0) == 4, "send on the stream");
    std::uint8_t echo[4]{};
    Require(ReceiveAll(client, echo, sizeof(echo)) == 4 && std::memcmp(echo, "ping", 4) == 0, "receive the echo");
    Require(sceNetGetpeername(client, peerName, &peerLength) == 0 && std::memcmp(peerName, target.data(), 16) == 0, "getpeername of the connected stream");
    Require(sceNetSocketClose(client) == 0, "close the client");

    Require(sceNetSocketClose(listener) == 0, "close the listener");
    Require(Service::WaitUntil([&] { return service.LiveBinds().empty(); }, 5000), "closing the listener unbinds its virtual port");
    Require(service.Unbinds().back() == std::make_pair(std::uint16_t{4}, std::uint8_t{2}), "unbind names the stream transport");
}

}

int main() {
    const Bytes token = {0x70, 0x32, 0x70, 0x01};
    Service service(token);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));
    const std::uint8_t address[4] = {0x01, 0x00, 0x12, 0xC6};
    service.SetState(true, 0x0102030405060708ull, "Alice", 2, address);
    Require(NetBridgeEnabled_nid_no_patch() == 1, "bridge on");
    Require(Service::WaitUntil([] { NetBridgeStateView view{}; NetBridgeState_nid_no_patch(&view); return view.signedIn == 1; }, 5000), "signed in");
    BridgeCalls(service);
    Sockets(service);
    Streams(service);
    std::puts("net bridge p2p: ok");
    return 0;
}
