#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/include/NetCtl.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceNet/include/Net.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

using FakeNet::Bytes;
using FakeNet::Require;
using FakeNet::Service;

constexpr int AfInet = 2;
constexpr int SockDgram = 2;
constexpr int SolSocket = 0xFFFF;
constexpr int SoReuseAddr = 0x4;
constexpr int SoReusePort = 0x200;
constexpr int SoBroadcast = 0x20;
constexpr int SoNbio = 0x1200;
constexpr int EAccess = 13;
constexpr int EAgain = 35;
constexpr int EAddrInUse = 48;
constexpr int ENetUnreach = 51;
constexpr int MsgTrunc = 0x10;
constexpr std::uint32_t Self = 0xC6120001u;
constexpr std::uint32_t Peer = 0xC6120002u;
constexpr std::uint32_t OtherPeer = 0xC6120003u;
constexpr std::uint32_t SubnetBroadcast = 0xC613FFFFu;
constexpr std::uint32_t LimitedBroadcast = 0xFFFFFFFFu;
constexpr std::uint16_t PortA = 40123;
constexpr std::uint16_t PortB = 40124;
constexpr std::uint16_t PortC = 40125;
constexpr std::uint16_t Remote = 40200;

std::vector<std::uint8_t> Address(std::uint32_t address, std::uint16_t port) {
    std::vector<std::uint8_t> bytes(16, 0);
    bytes[0] = 16;
    bytes[1] = AfInet;
    bytes[2] = static_cast<std::uint8_t>(port >> 8);
    bytes[3] = static_cast<std::uint8_t>(port);
    for (int index = 0; index < 4; ++index) bytes[4 + index] = static_cast<std::uint8_t>(address >> (24 - 8 * index));
    return bytes;
}

int Open() {
    const int socket = sceNetSocket("lan", AfInet, SockDgram, 0);
    Require(socket >= 0, "datagram socket");
    const int nonblocking = 1;
    Require(sceNetSetsockopt(socket, SolSocket, SoNbio, &nonblocking, sizeof(nonblocking)) == 0, "non-blocking");
    return socket;
}

void Option(int socket, int option) {
    const int on = 1;
    Require(sceNetSetsockopt(socket, SolSocket, option, &on, sizeof(on)) == 0, "socket option");
}

int Bind(int socket, std::uint32_t address, std::uint16_t port) {
    const auto local = Address(address, port);
    return sceNetBind_nid_postfix(socket, local.data(), 16);
}

int Fails(std::int64_t result) {
    return result < 0 ? *sceNetErrnoLoc() : 0;
}

std::int64_t SendTo(int socket, const Bytes& data, std::uint32_t address, std::uint16_t port) {
    const auto to = Address(address, port);
    return sceNetSendto(socket, data.data(), data.size(), 0, to.data(), 16);
}

std::int64_t ReceiveWithin(int socket, std::uint8_t* buffer, std::size_t size, std::uint8_t* from) {
    std::int64_t got = -1;
    Service::WaitUntil([&] {
        std::uint32_t fromLength = 16;
        got = sceNetRecvfrom(socket, buffer, size, 0, from, &fromLength);
        return got >= 0;
    }, 5000);
    return got;
}

bool NothingWaiting(int socket) {
    std::uint8_t buffer[64]{};
    std::uint8_t from[16]{};
    std::uint32_t fromLength = 16;
    return sceNetRecvfrom(socket, buffer, sizeof(buffer), 0, from, &fromLength) < 0 && *sceNetErrnoLoc() == EAgain;
}

std::size_t BindsOf(Service& service, std::uint16_t vport) {
    std::size_t count = 0;
    for (const auto& bind : service.LiveBinds()) count += bind.vport == vport ? 1 : 0;
    return count;
}

std::string InfoText(int code, int* result = nullptr) {
    NetCtlInfo info{};
    const int got = sceNetCtlGetInfo(code, &info);
    if (result != nullptr) *result = got;
    return got == 0 ? std::string(reinterpret_cast<const char*>(info.opaque)) : std::string();
}

std::uint32_t InfoWord(int code) {
    NetCtlInfo info{};
    Require(sceNetCtlGetInfo(code, &info) == 0, "netctl word");
    std::uint32_t value = 0;
    std::memcpy(&value, info.opaque, sizeof(value));
    return value;
}

void SetAddress(Service& service, std::uint32_t address) {
    const std::uint8_t bytes[4] = {static_cast<std::uint8_t>(address), static_cast<std::uint8_t>(address >> 8), static_cast<std::uint8_t>(address >> 16), static_cast<std::uint8_t>(address >> 24)};
    service.SetState(true, 0x0102030405060708ull, "Alice", 2, bytes);
    Require(Service::WaitUntil([&] { NetBridgeStateView view{}; NetBridgeState_nid_no_patch(&view); return view.address == address; }, 5000), "state with the address arrives");
}

void Inactive(Service& service) {
    SetAddress(service, 0);
    int result = 0;
    const std::string host = InfoText(14, &result);
    Require(result != 0 || host != "198.18.0.1", "no virtual address in the interface answer while no session is active");

    const int socket = Open();
    Require(Bind(socket, 0, 0) == 0, "bind the wildcard with port 0");
    std::uint8_t name[16]{};
    std::uint32_t nameLength = sizeof(name);
    Require(sceNetGetsockname(socket, name, &nameLength) == 0 && nameLength == 16, "getsockname");
    const std::uint16_t port = static_cast<std::uint16_t>((name[2] << 8) | name[3]);
    Require(port != 0, "the host picked a port");
    Require(service.LiveBinds().empty() && service.Binds().empty(), "no bind reaches the net service");

    const Bytes hello = {'h', 'i'};
    Require(SendTo(socket, hello, 0x7F000001u, port) == 2, "a loopback datagram goes the host way");
    std::uint8_t buffer[16]{};
    std::uint8_t from[16]{};
    Require(ReceiveWithin(socket, buffer, sizeof(buffer), from) == 2 && buffer[0] == 'h', "and arrives on the host socket");
    Require(service.Datagrams().empty(), "nothing reached the net service");
    Require(sceNetSocketClose(socket) == 0, "close");
}

void Interface(Service& service) {
    SetAddress(service, Self);
    Require(InfoText(14) == "198.18.0.1", "the interface address is the virtual address");
    Require(InfoText(15) == "255.254.0.0", "the netmask is the virtual subnet's");
    Require(InfoText(16) == "0.0.0.0", "no gateway to a real network");
    Require(InfoWord(3) == 1200, "the MTU is the tunnel's");
}

void Broadcasts(Service& service) {
    const int socket = Open();
    Option(socket, SoBroadcast);
    Require(Bind(socket, 0, PortA) == 0, "bind the wildcard");
    Require(Service::WaitUntil([&] { return BindsOf(service, PortA) == 1; }, 5000), "bind reached the net service");
    std::uint16_t loopbackPort = 0;
    for (const auto& bind : service.LiveBinds()) {
        if (bind.vport == PortA) {
            Require(bind.transport == 1 && bind.loopbackPort != 0, "datagram bind fields");
            loopbackPort = bind.loopbackPort;
        }
    }
    std::uint8_t name[16]{};
    std::uint32_t nameLength = sizeof(name);
    Require(sceNetGetsockname(socket, name, &nameLength) == 0 && std::memcmp(name, Address(0, PortA).data(), 16) == 0, "getsockname: wildcard address, the game's port");

    const Bytes ask = {0x01, 0x02, 0x03};
    Require(SendTo(socket, ask, LimitedBroadcast, Remote) == 3, "sendto the limited broadcast");
    Require(SendTo(socket, ask, SubnetBroadcast, Remote + 1) == 3, "sendto the subnet broadcast");
    Require(SendTo(socket, ask, 0xE00000FBu, 5353) == 3, "multicast is dropped without an error");
    Require(Fails(SendTo(socket, ask, 0xC0A801FFu, Remote)) == ENetUnreach, "the broadcast of a real network is unreachable");
    Require(Fails(SendTo(socket, ask, 0x08080808u, 3478)) == ENetUnreach, "so is a server on the internet");
    Require(SendTo(socket, ask, Peer, 7) == 3, "unicast to a peer");
    Require(Service::WaitUntil([&] { return service.Datagrams().size() >= 3; }, 5000), "datagrams reached the net service");
    const auto sent = service.Datagrams();
    Require(sent.size() == 3, "only the three carried datagrams left");
    Require(sent[0].address == LimitedBroadcast && sent[0].vport == Remote && sent[0].sourceVport == PortA && sent[0].payload == ask, "limited broadcast frame");
    Require(sent[1].address == SubnetBroadcast && sent[1].vport == Remote + 1 && sent[1].sourceVport == PortA, "subnet broadcast frame");
    Require(sent[2].address == Peer && sent[2].vport == 7 && sent[2].sourceVport == PortA, "unicast frame");
    for (const auto& datagram : sent) Require(datagram.fromPort == loopbackPort, "sent from the loopback socket the bind named");

    const int quiet = Open();
    Require(Bind(quiet, 0, PortB) == 0, "bind a second port");
    Require(Fails(SendTo(quiet, ask, LimitedBroadcast, Remote)) == EAccess, "no SO_BROADCAST, no broadcast");
    Require(service.Datagrams().size() == 3, "and nothing was carried");

    const Bytes reply = {0xAA, 0xBB, 0xCC, 0xDD};
    Require(service.InjectDatagram(Peer, PortA, 5555, reply), "the service delivers a datagram");
    std::uint8_t buffer[64]{};
    std::uint8_t from[16]{};
    Require(ReceiveWithin(socket, buffer, sizeof(buffer), from) == 4 && std::memcmp(buffer, reply.data(), 4) == 0, "payload unchanged");
    Require(std::memcmp(from, Address(Peer, 5555).data(), 16) == 0, "sender is the peer's virtual address and source port");
    Require(NothingWaiting(quiet), "another port heard nothing");
    Require(SendTo(socket, reply, Peer, 5555) == 4, "a reply goes to the address and port it came from");
    Require(Service::WaitUntil([&] { return service.Datagrams().size() == 4; }, 5000), "reply reached the net service");
    Require(service.Datagrams()[3].address == Peer && service.Datagrams()[3].vport == 5555, "reply frame");

    Require(service.InjectDatagram(Peer, PortA, 5555, {1, 2, 3, 4, 5, 6, 7, 8}), "a longer datagram");
    std::uint8_t small[3]{};
    Require(ReceiveWithin(socket, small, sizeof(small), from) == 3 && small[0] == 1 && small[2] == 3, "cut to the buffer");

    Require(sceNetSocketClose(quiet) == 0, "close the quiet socket");
    Require(sceNetSocketClose(socket) == 0, "close");
    Require(Service::WaitUntil([&] { return service.LiveBinds().empty(); }, 5000), "close unbinds the virtual port");
}

void VirtualAddressAndEphemeral(Service& service) {
    const int bound = Open();
    Require(Bind(bound, Self, PortB) == 0, "bind the virtual address");
    Require(Service::WaitUntil([&] { return BindsOf(service, PortB) == 1; }, 5000), "bind reached the net service");
    std::uint8_t name[16]{};
    std::uint32_t nameLength = sizeof(name);
    Require(sceNetGetsockname(bound, name, &nameLength) == 0 && std::memcmp(name, Address(Self, PortB).data(), 16) == 0, "getsockname reports the virtual address");
    Require(service.InjectDatagram(OtherPeer, PortB, 1000, {9, 9}), "deliver");
    std::uint8_t buffer[16]{};
    std::uint8_t from[16]{};
    Require(ReceiveWithin(bound, buffer, sizeof(buffer), from) == 2 && std::memcmp(from, Address(OtherPeer, 1000).data(), 16) == 0, "received on the virtual address");

    const int ephemeral = Open();
    Require(Bind(ephemeral, 0, 0) == 0, "bind port 0");
    nameLength = sizeof(name);
    Require(sceNetGetsockname(ephemeral, name, &nameLength) == 0, "getsockname");
    const std::uint16_t chosen = static_cast<std::uint16_t>((name[2] << 8) | name[3]);
    Require(chosen != 0 && BindsOf(service, chosen) == 1, "the chosen port is the one bound at the net service");

    const int implicit = Open();
    const std::size_t before = service.LiveBinds().size();
    const std::size_t implicitBefore = service.Datagrams().size();
    Require(SendTo(implicit, {1}, Peer, 7) == 1, "sendto on an unbound socket");
    Require(Service::WaitUntil([&] { return service.LiveBinds().size() == before + 1; }, 5000), "it was bound first");
    Require(Service::WaitUntil([&] { return service.Datagrams().size() == implicitBefore + 1; }, 5000), "and its datagram was carried");

    const int connected = Open();
    Require(Bind(connected, 0, PortC) == 0, "bind");
    Require(Fails(sceNetSend(connected, "x", 1, 0)) == 39, "send without a peer needs a destination");
    const auto target = Address(Peer, 9);
    Require(sceNetConnect(connected, target.data(), 16) == 0, "connect");
    nameLength = sizeof(name);
    Require(sceNetGetpeername(connected, name, &nameLength) == 0 && std::memcmp(name, target.data(), 16) == 0, "getpeername");
    const std::size_t sentBefore = service.Datagrams().size();
    Require(sceNetSend(connected, "ping", 4, 0) == 4, "send to the peer");
    Require(Service::WaitUntil([&] { return service.Datagrams().size() == sentBefore + 1; }, 5000), "frame reached the net service");
    Require(service.Datagrams().back().address == Peer && service.Datagrams().back().vport == 9 && service.Datagrams().back().sourceVport == PortC, "connected send frame");
    Require(service.InjectDatagram(OtherPeer, PortC, 9, {1}), "a stranger sends");
    Require(service.InjectDatagram(Peer, PortC, 9, {2}), "the peer sends");
    Require(ReceiveWithin(connected, buffer, sizeof(buffer), from) == 1 && buffer[0] == 2, "only the connected peer is heard");

    Require(sceNetSocketClose(connected) == 0 && sceNetSocketClose(implicit) == 0 && sceNetSocketClose(ephemeral) == 0 && sceNetSocketClose(bound) == 0, "close all");
    Require(Service::WaitUntil([&] { return service.LiveBinds().empty(); }, 5000), "all virtual ports unbound");
}

void SharedPorts(Service& service) {
    const int first = Open();
    const int second = Open();
    Option(first, SoReuseAddr);
    Option(second, SoReuseAddr);
    Require(Bind(first, 0, PortA) == 0 && Bind(second, 0, PortA) == 0, "two sockets share a port with SO_REUSEADDR");
    Require(Service::WaitUntil([&] { return BindsOf(service, PortA) == 1; }, 5000), "the port is bound once");
    const int stranger = Open();
    Require(Bind(stranger, 0, PortA) < 0 && *sceNetErrnoLoc() == EAddrInUse, "a socket without the option is refused");
    Require(sceNetSocketClose(stranger) == 0, "close the refused socket");

    Require(service.InjectDatagram(Peer, PortA, 4444, {7, 7, 7}), "deliver once");
    std::uint8_t buffer[16]{};
    std::uint8_t from[16]{};
    Require(ReceiveWithin(first, buffer, sizeof(buffer), from) == 3, "the first socket gets it");
    Require(ReceiveWithin(second, buffer, sizeof(buffer), from) == 3 && std::memcmp(from, Address(Peer, 4444).data(), 16) == 0, "and so does the second");

    Require(sceNetSocketClose(first) == 0, "close one");
    Require(BindsOf(service, PortA) == 1, "the port stays bound for the other");
    Require(service.InjectDatagram(Peer, PortA, 4444, {8}), "deliver again");
    Require(ReceiveWithin(second, buffer, sizeof(buffer), from) == 1 && buffer[0] == 8, "the remaining socket still hears");
    Require(sceNetSocketClose(second) == 0, "close the last");
    Require(Service::WaitUntil([&] { return service.LiveBinds().empty(); }, 5000), "the last close unbinds");
}

void ReuseCleared(Service& service) {
    const int first = Open();
    const int second = Open();
    Option(first, SoReuseAddr);
    Option(second, SoReuseAddr);
    const int off = 0;
    Require(sceNetSetsockopt(second, SolSocket, SoReuseAddr, &off, sizeof(off)) == 0, "clear SO_REUSEADDR");
    Require(Bind(first, 0, PortB) == 0, "bind the first");
    Require(Bind(second, 0, PortB) < 0 && *sceNetErrnoLoc() == EAddrInUse, "a cleared SO_REUSEADDR no longer shares the port");

    const int third = Open();
    const int fourth = Open();
    Option(third, SoReusePort);
    Option(fourth, SoReusePort);
    Require(Bind(third, 0, PortC) == 0 && Bind(fourth, 0, PortC) == 0, "SO_REUSEPORT shares a port");
    Require(BindsOf(service, PortC) == 1, "bound once");
    Require(sceNetSocketClose(first) == 0 && sceNetSocketClose(second) == 0 && sceNetSocketClose(third) == 0 && sceNetSocketClose(fourth) == 0, "close all");
    Require(Service::WaitUntil([&] { return service.LiveBinds().empty(); }, 5000), "all virtual ports unbound");
}

void HostSockets(Service& service) {
    const int socket = Open();
    Option(socket, SoBroadcast);
    Require(Bind(socket, 0x7F000001u, 0) == 0, "bind loopback");
    Require(service.LiveBinds().empty(), "a loopback socket stays on the host");
    std::uint8_t name[16]{};
    std::uint32_t nameLength = sizeof(name);
    Require(sceNetGetsockname(socket, name, &nameLength) == 0, "getsockname");
    const std::uint16_t port = static_cast<std::uint16_t>((name[2] << 8) | name[3]);
    const std::size_t sentBefore = service.Datagrams().size();
    const Bytes ask = {0x05};
    Require(Fails(SendTo(socket, ask, LimitedBroadcast, Remote)) == ENetUnreach, "a host socket's limited broadcast never reaches a real network");
    Require(Fails(SendTo(socket, ask, SubnetBroadcast, Remote)) == ENetUnreach, "nor its subnet broadcast");
    Require(Fails(SendTo(socket, ask, Peer, Remote)) == ENetUnreach, "nor a datagram to a virtual address");
    Require(SendTo(socket, ask, 0xE00000FBu, 5353) == 1, "its multicast is dropped without an error");
    const auto peer = Address(Peer, Remote);
    Require(sceNetConnect(socket, peer.data(), 16) < 0 && *sceNetErrnoLoc() == ENetUnreach, "connect to a virtual address");
    Require(SendTo(socket, ask, 0x7F000001u, port) == 1, "loopback still works");
    std::uint8_t buffer[16]{};
    std::uint8_t from[16]{};
    Require(ReceiveWithin(socket, buffer, sizeof(buffer), from) == 1 && buffer[0] == 0x05, "and arrives");
    Require(service.Datagrams().size() == sentBefore && service.LiveBinds().empty(), "nothing reached the net service");
    Require(sceNetSocketClose(socket) == 0, "close");
}

void ConcurrentImplicitBinds(Service& service) {
    for (int round = 0; round < 8; ++round) {
        const int socket = Open();
        const std::size_t before = service.LiveBinds().size();
        const std::size_t sentBefore = service.Datagrams().size();
        std::int64_t otherSent = -1;
        std::thread other([&] { otherSent = SendTo(socket, {1}, Peer, 7); });
        const std::int64_t sent = SendTo(socket, {2}, Peer, 7);
        other.join();
        Require(sent == 1 && otherSent == 1, "both first sends succeed");
        Require(Service::WaitUntil([&] { return service.Datagrams().size() == sentBefore + 2; }, 5000), "both sends carried");
        Require(Service::WaitUntil([&] { return service.LiveBinds().size() == before + 1; }, 5000), "two first sends at once bind the socket once");
        Require(sceNetSocketClose(socket) == 0, "close");
        Require(Service::WaitUntil([&] { return service.LiveBinds().size() == before; }, 5000), "close unbinds it");
    }
}

void ReceiveMessage(Service& service) {
    const int socket = Open();
    Require(Bind(socket, 0, PortA) == 0, "bind");
    Require(Service::WaitUntil([&] { return BindsOf(service, PortA) == 1; }, 5000), "bound");
    Require(service.InjectDatagram(Peer, PortA, 6000, {1, 2, 3, 4, 5, 6}), "deliver");
    std::uint8_t head[2]{};
    std::uint8_t tail[2]{};
    NetIovec vectors[2] = {{head, sizeof(head)}, {tail, sizeof(tail)}};
    std::uint8_t name[16]{};
    NetMsghdr message{};
    std::int64_t got = -1;
    Require(Service::WaitUntil([&] {
        message = {};
        message.name = name;
        message.name_length = sizeof(name);
        message.iov = vectors;
        message.iov_length = 2;
        got = sceNetRecvmsg(socket, &message, 0);
        return got >= 0;
    }, 5000), "recvmsg");
    Require(got == 4 && head[0] == 1 && head[1] == 2 && tail[0] == 3 && tail[1] == 4, "recvmsg fills the vectors in order");
    Require((message.flags & MsgTrunc) != 0, "and reports the cut");
    Require(message.name_length == 16 && std::memcmp(name, Address(Peer, 6000).data(), 16) == 0, "recvmsg names the peer");
    Require(sceNetSocketClose(socket) == 0, "close");
    Require(Service::WaitUntil([&] { return service.LiveBinds().empty(); }, 5000), "unbound");
}

void Ended(Service& service) {
    SetAddress(service, 0);
    const int socket = Open();
    Require(Bind(socket, 0, PortB) == 0, "bind after the session ended");
    Require(service.LiveBinds().empty(), "the host handles it again");
    Require(sceNetSocketClose(socket) == 0, "close");
}

}

int main() {
    const Bytes token = {0x70, 0x32, 0x70, 0x02};
    Service service(token);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));
    Require(NetBridgeEnabled_nid_no_patch() == 1, "bridge on");
    const std::uint8_t none[4] = {0, 0, 0, 0};
    service.SetState(true, 0x0102030405060708ull, "Alice", 2, none);
    Require(Service::WaitUntil([] { NetBridgeStateView view{}; NetBridgeState_nid_no_patch(&view); return view.signedIn == 1; }, 5000), "signed in");
    Require(sceNetCtlInit() == 0, "netctl init");
    Inactive(service);
    Interface(service);
    Broadcasts(service);
    VirtualAddressAndEphemeral(service);
    SharedPorts(service);
    ReuseCleared(service);
    HostSockets(service);
    ConcurrentImplicitBinds(service);
    ReceiveMessage(service);
    Ended(service);
    std::puts("net bridge lan: ok");
    return 0;
}
