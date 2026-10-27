#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <bcrypt.h>
#else
#include <cstdio>
#endif

#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libc/include/general/LogMacros.hpp"
#include "NetBridgeShare.hpp"
#include "NetBridgeAuth.hpp"
#include "NetBridgeWire.hpp"

#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using namespace NetBridgeWire;

#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket InvalidSocket = INVALID_SOCKET;
#else
using Socket = int;
constexpr Socket InvalidSocket = -1;
#endif

constexpr std::size_t MaxOpenRequests = 64;
constexpr std::size_t MaxHandleLength = 16;
constexpr std::size_t MaxTokenBytes = 512;
constexpr std::size_t MaxPushDataTypes = 1024;
constexpr auto ReconnectDelay = std::chrono::seconds(2);
constexpr auto WebTimeout = std::chrono::seconds(30);
constexpr auto ControlTimeout = std::chrono::seconds(10);
constexpr std::size_t MaxSessionLength = 1024;
constexpr std::size_t MaxPeers = 4096;
constexpr std::uint8_t PeerConnecting = 1;
constexpr std::uint8_t PeerFailed = 4;
constexpr std::uint8_t ActivateSignedOut = 2;
constexpr auto StartupGrace = std::chrono::seconds(2);
constexpr int HandshakeTimeoutMs = 10000;
constexpr int SendTimeoutMs = 5000;

struct Config {
    std::uint16_t port = 0;
    std::vector<std::uint8_t> token;
};

struct BridgeState {
    bool signedIn = false;
    std::uint64_t accountId = 0;
    std::string handle;
    std::uint8_t natType = 0;
    std::uint32_t address = 0;

    bool operator==(const BridgeState& other) const {
        return signedIn == other.signedIn && accountId == other.accountId && handle == other.handle && natType == other.natType && address == other.address;
    }
};

struct Traffic {
    std::uint16_t udpPort = 0;
    std::uint16_t tcpPort = 0;
    std::uint8_t streamKey[16]{};
};

struct Peer {
    std::uint64_t accountId = 0;
    std::string handle;
    std::uint8_t peerState = 0;
};

using BindKey = std::pair<std::uint16_t, std::uint8_t>;
using Activation = std::tuple<std::int32_t, std::string, std::uint8_t>;
using PeerKey = std::pair<std::string, std::uint32_t>;

struct Waiter {
    std::condition_variable done;
    std::uint16_t expectedKind = 0;
    bool finished = false;
    bool ok = false;
    std::vector<std::uint8_t> payload;
};

struct ReplyHolder {
    std::vector<std::string> names;
    std::vector<std::string> values;
    std::vector<const char*> namePointers;
    std::vector<const char*> valuePointers;
    std::vector<std::uint8_t> body;
};

int HexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

bool ParseToken(const char* text, std::vector<std::uint8_t>& token) {
    const std::size_t length = std::strlen(text);
    if (length == 0 || length % 2 != 0 || length / 2 > MaxTokenBytes) return false;
    token.clear();
    for (std::size_t index = 0; index < length; index += 2) {
        const int high = HexValue(text[index]);
        const int low = HexValue(text[index + 1]);
        if (high < 0 || low < 0) return false;
        token.push_back(static_cast<std::uint8_t>(high * 16 + low));
    }
    return true;
}

bool ParseEndpoint(const char* text, std::uint16_t& port) {
    static constexpr char Host[] = "127.0.0.1:";
    if (std::strncmp(text, Host, sizeof(Host) - 1) != 0) return false;
    const char* digits = text + sizeof(Host) - 1;
    if (*digits == '\0') return false;
    unsigned value = 0;
    for (const char* cursor = digits; *cursor != '\0'; ++cursor) {
        if (*cursor < '0' || *cursor > '9') return false;
        value = value * 10 + static_cast<unsigned>(*cursor - '0');
        if (value > 65535) return false;
    }
    if (value == 0) return false;
    port = static_cast<std::uint16_t>(value);
    return true;
}

bool ReadConfig(Config& config) {
    const char* endpoint = std::getenv("ANYPS5_NET_ENDPOINT");
    const char* token = std::getenv("ANYPS5_NET_TOKEN");
    if (endpoint == nullptr || token == nullptr) return false;
    return ParseEndpoint(endpoint, config.port) && ParseToken(token, config.token);
}

void CloseSocket(Socket socketHandle) {
#ifdef _WIN32
    ::closesocket(socketHandle);
#else
    ::close(socketHandle);
#endif
}

void ShutdownSocket(Socket socketHandle) {
#ifdef _WIN32
    ::shutdown(socketHandle, SD_BOTH);
#else
    ::shutdown(socketHandle, SHUT_RDWR);
#endif
}

void SetTimeout(Socket socketHandle, int option, int milliseconds) {
#ifdef _WIN32
    const DWORD value = static_cast<DWORD>(milliseconds);
    ::setsockopt(socketHandle, SOL_SOCKET, option, reinterpret_cast<const char*>(&value), sizeof(value));
#else
    timeval value{};
    value.tv_sec = milliseconds / 1000;
    value.tv_usec = (milliseconds % 1000) * 1000;
    ::setsockopt(socketHandle, SOL_SOCKET, option, &value, sizeof(value));
#endif
}

void StartSockets() {
#ifdef _WIN32
    static const int status = [] {
        WSADATA data{};
        return WSAStartup(MAKEWORD(2, 2), &data);
    }();
    (void)status;
#endif
}

Socket ConnectLoopback(std::uint16_t port) {
    const Socket socketHandle = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle == InvalidSocket) return InvalidSocket;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(socketHandle, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        CloseSocket(socketHandle);
        return InvalidSocket;
    }
    const int enable = 1;
    ::setsockopt(socketHandle, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&enable), sizeof(enable));
#ifdef SO_NOSIGPIPE
    ::setsockopt(socketHandle, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<const char*>(&enable), sizeof(enable));
#endif
    SetTimeout(socketHandle, SO_SNDTIMEO, SendTimeoutMs);
    return socketHandle;
}

bool Interrupted() {
#ifdef _WIN32
    return false;
#else
    return errno == EINTR;
#endif
}

bool SendAll(Socket socketHandle, const std::uint8_t* data, std::size_t size) {
#ifdef MSG_NOSIGNAL
    constexpr int flags = MSG_NOSIGNAL;
#else
    constexpr int flags = 0;
#endif
    while (size > 0) {
        const auto chunk = static_cast<int>(size > 1u << 20 ? 1u << 20 : size);
        const auto sent = ::send(socketHandle, reinterpret_cast<const char*>(data), chunk, flags);
        if (sent < 0 && Interrupted()) continue;
        if (sent <= 0) return false;
        data += sent;
        size -= static_cast<std::size_t>(sent);
    }
    return true;
}

bool ReceiveAll(Socket socketHandle, std::uint8_t* data, std::size_t size) {
    while (size > 0) {
        const auto chunk = static_cast<int>(size > 1u << 20 ? 1u << 20 : size);
        const auto received = ::recv(socketHandle, reinterpret_cast<char*>(data), chunk, 0);
        if (received < 0 && Interrupted()) continue;
        if (received <= 0) return false;
        data += received;
        size -= static_cast<std::size_t>(received);
    }
    return true;
}

bool ReceiveFrame(Socket socketHandle, FrameHeader& header, std::vector<std::uint8_t>& payload, std::uint32_t limit = MaxPayload) {
    std::uint8_t raw[HeaderSize];
    if (!ReceiveAll(socketHandle, raw, sizeof(raw))) return false;
    header = ParseHeader(raw);
    if (header.length > limit) return false;
    payload.assign(header.length, 0);
    return header.length == 0 || ReceiveAll(socketHandle, payload.data(), payload.size());
}

bool RandomBytes(std::uint8_t* out, std::size_t size) {
#ifdef _WIN32
    return BCRYPT_SUCCESS(BCryptGenRandom(nullptr, out, static_cast<ULONG>(size), BCRYPT_USE_SYSTEM_PREFERRED_RNG));
#else
    std::FILE* source = std::fopen("/dev/urandom", "rb");
    if (source == nullptr) return false;
    const bool ok = std::fread(out, 1, size, source) == size;
    std::fclose(source);
    return ok;
#endif
}

bool ValidHandle(const std::string& handle) {
    if (handle.size() > MaxHandleLength) return false;
    for (const char c : handle) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

class Bridge {
public:
    explicit Bridge(Config configuration) : config(std::move(configuration)) {
    }

    void Start() {
        StartSockets();
        std::thread(&Bridge::run, this).detach();
        std::unique_lock lock(mutex);
        resolved.wait_for(lock, StartupGrace, [this] { return attemptResolved; });
    }

    std::uint64_t State(NetBridgeStateView* out) {
        std::lock_guard lock(mutex);
        if (out != nullptr) {
            std::memset(out, 0, sizeof(*out));
            out->signedIn = state.signedIn ? 1 : 0;
            out->address = state.address;
            out->accountId = state.accountId;
            std::memcpy(out->handle, state.handle.data(), state.handle.size());
            out->natType = state.natType;
        }
        return counter;
    }

    bool Request(std::uint16_t kind, std::vector<std::uint8_t> payload, std::chrono::seconds timeout, std::vector<std::uint8_t>& reply) {
        if (payload.size() > MaxPayload) return false;
        Waiter waiter;
        waiter.expectedKind = static_cast<std::uint16_t>(kind | ReplyBit);
        std::uint32_t number = 0;
        std::uint64_t connection = 0;
        {
            std::lock_guard lock(mutex);
            if (!connected || waiters.size() >= MaxOpenRequests) return false;
            number = nextRequestNumber();
            waiters.emplace(number, &waiter);
            connection = generation;
        }
        const bool sent = send(connection, EncodeFrame(kind, number, payload));
        std::unique_lock lock(mutex);
        if (sent) waiter.done.wait_for(lock, timeout, [&waiter] { return waiter.finished; });
        waiters.erase(number);
        if (!sent || !waiter.finished || !waiter.ok) return false;
        reply = std::move(waiter.payload);
        return true;
    }

    void Event(std::uint16_t kind, const std::vector<std::uint8_t>& payload) {
        std::uint64_t connection = 0;
        {
            std::lock_guard lock(mutex);
            if (!connected) return;
            connection = generation;
        }
        send(connection, EncodeFrame(kind, 0, payload));
    }

    bool TrafficPorts(NetBridgeTrafficView* out) {
        std::lock_guard lock(mutex);
        if (out != nullptr) {
            std::memset(out, 0, sizeof(*out));
            if (connected) {
                out->udpPort = traffic.udpPort;
                out->tcpPort = traffic.tcpPort;
                std::memcpy(out->streamKey, traffic.streamKey, sizeof(out->streamKey));
            }
        }
        return connected;
    }

    int Activate(std::int32_t userId, const std::string& session, std::uint8_t sessionKind) {
        if (session.size() > MaxSessionLength) return -1;
        Builder payload;
        payload.I32(userId).Str(session).U8(sessionKind);
        if (!payload.Ok()) return -1;
        std::vector<std::uint8_t> reply;
        if (!Request(KindActivate, payload.Take(), ControlTimeout, reply)) return Connected() ? -1 : ActivateSignedOut;
        Reader reader(reply.data(), reply.size());
        const std::uint8_t result = reader.U8();
        if (!reader.Ok()) return -1;
        if (result == 0) {
            std::lock_guard lock(mutex);
            activations.emplace(userId, session, sessionKind);
        }
        return result;
    }

    void Deactivate(std::int32_t userId, const std::string& session) {
        if (session.size() > MaxSessionLength) return;
        {
            std::lock_guard lock(mutex);
            std::erase_if(activations, [&](const Activation& item) { return std::get<0>(item) == userId && std::get<1>(item) == session; });
            if (std::erase_if(peers, [&](const auto& item) { return item.first.first == session; }) != 0) ++peerCounter;
        }
        Builder payload;
        payload.I32(userId).Str(session);
        if (payload.Ok()) Event(KindDeactivate, payload.Take());
    }

    std::uint32_t Peers(const std::string& session, NetBridgePeerView* out, std::uint32_t capacity, std::uint64_t* changes) {
        std::lock_guard lock(mutex);
        if (changes != nullptr) *changes = peerCounter;
        std::uint32_t count = 0;
        for (const auto& [key, peer] : peers) {
            if (key.first != session) continue;
            if (out != nullptr && count < capacity) {
                NetBridgePeerView& view = out[count];
                std::memset(&view, 0, sizeof(view));
                view.accountId = peer.accountId;
                std::memcpy(view.handle, peer.handle.data(), peer.handle.size());
                view.peerState = peer.peerState;
                view.address = key.second;
            }
            ++count;
        }
        return count;
    }

    bool PeerInfo(std::uint32_t address, NetBridgePeerInfoView* out) {
        Builder payload;
        payload.U32(address);
        std::vector<std::uint8_t> reply;
        if (!Request(KindPeerInfo, payload.Take(), ControlTimeout, reply)) return false;
        Reader reader(reply.data(), reply.size());
        const std::uint8_t known = reader.U8();
        const std::uint32_t rttUs = reader.U32();
        const std::uint8_t relayed = reader.U8();
        if (!reader.Ok() || known > 1 || relayed > 1) return false;
        out->known = known;
        out->rttUs = rttUs;
        out->relayed = relayed;
        return true;
    }

    int Bind(std::uint16_t vport, std::uint8_t transport, std::uint16_t loopbackPort) {
        if (!Connected()) {
            std::lock_guard lock(mutex);
            binds[BindKey{vport, transport}] = loopbackPort;
            return 0;
        }
        Builder payload;
        payload.U16(vport).U8(transport).U16(loopbackPort);
        std::vector<std::uint8_t> reply;
        if (!Request(KindBind, payload.Take(), ControlTimeout, reply)) return -1;
        Reader reader(reply.data(), reply.size());
        const std::uint8_t result = reader.U8();
        if (!reader.Ok()) return -1;
        if (result == 0) {
            std::lock_guard lock(mutex);
            binds[BindKey{vport, transport}] = loopbackPort;
        }
        return result;
    }

    void Unbind(std::uint16_t vport, std::uint8_t transport) {
        {
            std::lock_guard lock(mutex);
            binds.erase(BindKey{vport, transport});
        }
        Builder payload;
        payload.U16(vport).U8(transport);
        Event(KindUnbind, payload.Take());
    }

    int RegisterPushHandler(NetBridgePushHandler handler, void* userArg) {
        if (handler == nullptr) return 0;
        std::lock_guard lock(pushMutex);
        const int id = nextPushHandlerId++;
        pushHandlers.push_back(PushHandlerSlot{id, handler, userArg});
        return id;
    }

    void UnregisterPushHandler(int registrationId) {
        std::lock_guard lock(pushMutex);
        std::erase_if(pushHandlers, [registrationId](const PushHandlerSlot& slot) {
            return slot.id == registrationId;
        });
    }

    int RegisterPushPump(NetBridgePushPump pump, void* userArg) {
        if (pump == nullptr) return 0;
        std::lock_guard lock(pumpMutex);
        const int id = nextPushPumpId++;
        pushPumps.push_back(PushPumpSlot{id, pump, userArg});
        return id;
    }

    void UnregisterPushPump(int registrationId) {
        std::lock_guard lock(pumpMutex);
        std::erase_if(pushPumps, [registrationId](const PushPumpSlot& slot) {
            return slot.id == registrationId;
        });
    }

    void DeliverPushEvents() {
        std::vector<PushPumpSlot> slots;
        {
            std::lock_guard lock(pumpMutex);
            slots = pushPumps;
        }
        for (const auto& slot : slots) {
            try {
                if (slot.pump != nullptr) slot.pump(slot.userArg);
            } catch (...) {
            }
        }
    }

private:
    bool Connected() {
        std::lock_guard lock(mutex);
        return connected;
    }

    std::uint32_t nextRequestNumber() {
        std::uint32_t number = 0;
        do {
            number = ++lastRequest;
        } while (number == 0 || waiters.count(number) != 0);
        return number;
    }

    void replay(std::uint64_t connection) {
        std::vector<std::vector<std::uint8_t>> frames;
        {
            std::lock_guard lock(mutex);
            for (const auto& [key, loopbackPort] : binds) {
                Builder payload;
                payload.U16(key.first).U8(key.second).U16(loopbackPort);
                frames.push_back(EncodeFrame(KindBind, nextRequestNumber(), payload.Take()));
            }
            for (const auto& [userId, session, sessionKind] : activations) {
                Builder payload;
                payload.I32(userId).Str(session).U8(sessionKind);
                if (payload.Ok()) frames.push_back(EncodeFrame(KindActivate, nextRequestNumber(), payload.Take()));
            }
        }
        for (const auto& frame : frames) {
            if (!send(connection, frame)) return;
        }
    }

    bool send(std::uint64_t connection, const std::vector<std::uint8_t>& frame) {
        std::lock_guard sendLock(sendMutex);
        Socket target = InvalidSocket;
        {
            std::lock_guard lock(mutex);
            if (!connected || generation != connection) return false;
            target = socketHandle;
        }
        if (SendAll(target, frame.data(), frame.size())) return true;
        ShutdownSocket(target);
        return false;
    }

    void resolve() {
        attemptResolved = true;
        resolved.notify_all();
    }

    void run() {
        bool reportedOutage = false;
        while (true) {
            try {
                const Socket target = ConnectLoopback(config.port);
                if (target == InvalidSocket) {
                    if (!reportedOutage) {
                        APS5_LOG_CHARS_ERR("the net service is not reachable; retrying every 2 seconds");
                        reportedOutage = true;
                    }
                } else if (serve(target)) {
                    reportedOutage = false;
                } else {
                    reportedOutage = true;
                }
            } catch (...) {
                reportedOutage = true;
            }
            {
                std::lock_guard lock(mutex);
                resolve();
            }
            std::this_thread::sleep_for(ReconnectDelay);
        }
    }

    bool serve(Socket target) {
        SetTimeout(target, SO_RCVTIMEO, HandshakeTimeoutMs);
        NetBridgeAuth::Nonce bridgeNonce{};
        if (!RandomBytes(bridgeNonce.data(), bridgeNonce.size())) {
            APS5_LOG_CHARS_ERR("no secure random numbers; the bridge cannot connect");
            CloseSocket(target);
            return false;
        }
        Builder hello;
        hello.U16(ProtocolVersion).Raw(bridgeNonce.data(), bridgeNonce.size());
        const auto helloFrame = EncodeFrame(KindHello, 0, hello.Take());
        FrameHeader header{};
        std::vector<std::uint8_t> payload;
        if (!SendAll(target, helloFrame.data(), helloFrame.size()) || !ReceiveFrame(target, header, payload, MaxHandshakePayload)) {
            CloseSocket(target);
            return false;
        }
        if (header.kind == KindReject) {
            const std::string reason(payload.begin(), payload.end());
            APS5_LOG_ERR("the net service refused the connection: %s", reason.c_str());
            CloseSocket(target);
            return false;
        }
        Reader challenge(payload.data(), payload.size());
        const std::uint16_t challengeVersion = challenge.U16();
        NetBridgeAuth::Nonce serviceNonce{};
        NetBridgeAuth::Proof serviceProof{};
        for (auto& byte : serviceNonce) byte = challenge.U8();
        for (auto& byte : serviceProof) byte = challenge.U8();
        if (header.kind != KindChallenge || !challenge.Ok() || challengeVersion != ProtocolVersion) {
            CloseSocket(target);
            return false;
        }
        const auto expected = NetBridgeAuth::MakeProof(config.token, NetBridgeAuth::RoleService, bridgeNonce, serviceNonce);
        if (!NetBridgeAuth::SameProof(expected, serviceProof)) {
            APS5_LOG_CHARS_ERR("the net service did not prove that it knows the token; closing the connection");
            CloseSocket(target);
            return false;
        }
        const auto mine = NetBridgeAuth::MakeProof(config.token, NetBridgeAuth::RoleBridge, bridgeNonce, serviceNonce);
        const auto authFrame = EncodeFrame(KindAuth, 0, std::vector<std::uint8_t>(mine.begin(), mine.end()));
        if (!SendAll(target, authFrame.data(), authFrame.size()) || !ReceiveFrame(target, header, payload, MaxHandshakePayload)) {
            CloseSocket(target);
            return false;
        }
        if (header.kind == KindReject) {
            const std::string reason(payload.begin(), payload.end());
            APS5_LOG_ERR("the net service refused the connection: %s", reason.c_str());
            CloseSocket(target);
            return false;
        }
        Reader welcome(payload.data(), payload.size());
        const std::uint16_t version = welcome.U16();
        Traffic offered;
        offered.udpPort = welcome.U16();
        offered.tcpPort = welcome.U16();
        for (auto& byte : offered.streamKey) byte = welcome.U8();
        if (header.kind != KindWelcome || !welcome.Ok() || version != ProtocolVersion) {
            CloseSocket(target);
            return false;
        }
        SetTimeout(target, SO_RCVTIMEO, 0);
        std::uint64_t connection = 0;
        {
            std::lock_guard lock(mutex);
            socketHandle = target;
            connected = true;
            traffic = offered;
            connection = ++generation;
        }
        APS5_LOG_CHARS_OUT("connected to the net service");
        replay(connection);
        while (ReceiveFrame(target, header, payload)) handle(header, payload);
        {
            std::lock_guard sendLock(sendMutex);
            std::lock_guard lock(mutex);
            connected = false;
            socketHandle = InvalidSocket;
            for (auto& [number, waiter] : waiters) {
                waiter->finished = true;
                waiter->ok = false;
                waiter->done.notify_all();
            }
            if (!peers.empty()) {
                peers.clear();
                ++peerCounter;
            }
            setState(BridgeState{});
            resolve();
        }
        CloseSocket(target);
        NetBridgeShare::Close();
        APS5_LOG_CHARS_OUT("disconnected from the net service");
        return true;
    }

    void handle(const FrameHeader& header, const std::vector<std::uint8_t>& payload) {
        if (header.request == 0) {
            if (header.kind == KindState) applyState(payload);
            else if (header.kind == KindPeer) applyPeer(payload);
            else if (header.kind == KindPushEvent) applyPushEvent(payload);
            else if (header.kind == KindShareClose) NetBridgeShare::Close();
            else if (header.kind == KindShareMode) NetBridgeShare::Mode(payload);
            else if (header.kind == KindSharePad) NetBridgeShare::Pad(payload);
            return;
        }
        if (header.kind == KindShareOpen) {
            std::uint64_t connection = 0;
            {
                std::lock_guard lock(mutex);
                connection = generation;
            }
            const std::uint8_t result = NetBridgeShare::Open(payload);
            if (result == NetBridgeShare::OpenUnsupported) send(connection, EncodeFrame(KindUnsupported, header.request, {}));
            else send(connection, EncodeFrame(KindShareOpened, header.request, std::vector<std::uint8_t>{result}));
            return;
        }
        if ((header.kind & ReplyBit) == 0) {
            std::uint64_t connection = 0;
            {
                std::lock_guard lock(mutex);
                connection = generation;
            }
            send(connection, EncodeFrame(KindUnsupported, header.request, {}));
            return;
        }
        std::lock_guard lock(mutex);
        const auto found = waiters.find(header.request);
        if (found == waiters.end() || found->second->finished) return;
        Waiter& waiter = *found->second;
        waiter.finished = true;
        waiter.ok = header.kind == waiter.expectedKind;
        if (waiter.ok) waiter.payload = payload;
        waiter.done.notify_all();
    }

    void applyState(const std::vector<std::uint8_t>& payload) {
        Reader reader(payload.data(), payload.size());
        const std::uint8_t signedIn = reader.U8();
        const std::uint64_t accountId = reader.U64();
        std::string handle = reader.Str();
        const std::uint8_t natType = reader.U8();
        const std::uint32_t address = reader.U32();
        if (!reader.Ok() || signedIn > 1 || !ValidHandle(handle)) return;
        BridgeState next;
        next.signedIn = signedIn == 1;
        next.accountId = next.signedIn ? accountId : 0;
        next.handle = next.signedIn ? std::move(handle) : std::string();
        next.natType = natType <= 3 ? natType : 0;
        next.address = address;
        std::lock_guard lock(mutex);
        setState(std::move(next));
        resolve();
    }

    void applyPeer(const std::vector<std::uint8_t>& payload) {
        Reader reader(payload.data(), payload.size());
        std::string session = reader.Str();
        const std::uint64_t accountId = reader.U64();
        std::string handle = reader.Str();
        const std::uint32_t address = reader.U32();
        const std::uint8_t peerState = reader.U8();
        if (!reader.Ok() || session.size() > MaxSessionLength || !ValidHandle(handle) || peerState < PeerConnecting || peerState > PeerFailed) return;
        std::lock_guard lock(mutex);
        PeerKey key{std::move(session), address};
        auto found = peers.find(key);
        if (found == peers.end()) {
            if (peers.size() >= MaxPeers) return;
            found = peers.emplace(std::move(key), Peer{}).first;
        }
        found->second = Peer{accountId, std::move(handle), peerState};
        ++peerCounter;
    }

    void applyPushEvent(const std::vector<std::uint8_t>& payload) {
        Reader reader(payload.data(), payload.size());
        const std::int32_t userId = reader.I32();
        const std::uint32_t filter = reader.U32();
        std::string dataType = reader.Str();
        const std::uint64_t from = reader.U64();
        const std::uint64_t to = reader.U64();
        const auto data = reader.Bytes();
        Pairs extended = reader.ReadPairs();
        if (!reader.Ok()) return;
        std::vector<const char*> extendedNames;
        std::vector<const char*> extendedValues;
        extendedNames.reserve(extended.size());
        extendedValues.reserve(extended.size());
        for (const auto& [name, value] : extended) {
            extendedNames.push_back(name.c_str());
            extendedValues.push_back(value.c_str());
        }
        NetBridgePushEventView view{};
        view.userId = userId;
        view.filter = filter;
        view.dataType = dataType.c_str();
        view.from = from;
        view.to = to;
        view.data = data.first;
        view.dataSize = data.second;
        view.extendedCount = static_cast<std::uint32_t>(extended.size());
        view.extendedNames = extendedNames.data();
        view.extendedValues = extendedValues.data();
        dispatchPushEvent(&view);
    }

    void dispatchPushEvent(const NetBridgePushEventView* event) {
        std::vector<PushHandlerSlot> slots;
        {
            std::lock_guard lock(pushMutex);
            slots = pushHandlers;
        }
        for (const auto& slot : slots) {
            try {
                if (slot.handler != nullptr) slot.handler(event, slot.userArg);
            } catch (...) {
            }
        }
    }

    void setState(BridgeState next) {
        if (state == next) return;
        state = std::move(next);
        ++counter;
    }

    const Config config;
    std::mutex mutex;
    std::mutex sendMutex;
    std::condition_variable resolved;
    bool attemptResolved = false;
    bool connected = false;
    Socket socketHandle = InvalidSocket;
    std::uint64_t generation = 0;
    std::uint64_t counter = 0;
    std::uint32_t lastRequest = 0;
    BridgeState state;
    Traffic traffic;
    std::map<BindKey, std::uint16_t> binds;
    std::set<Activation> activations;
    std::map<PeerKey, Peer> peers;
    std::uint64_t peerCounter = 0;
    std::unordered_map<std::uint32_t, Waiter*> waiters;
    struct PushHandlerSlot {
        int id;
        NetBridgePushHandler handler;
        void* userArg;
    };
    std::mutex pushMutex;
    std::vector<PushHandlerSlot> pushHandlers;
    int nextPushHandlerId = 1;
    struct PushPumpSlot {
        int id;
        NetBridgePushPump pump;
        void* userArg;
    };
    std::mutex pumpMutex;
    std::vector<PushPumpSlot> pushPumps;
    int nextPushPumpId = 1;
};

Bridge* Create() {
    Config config;
    if (!ReadConfig(config)) return nullptr;
    auto* bridge = new Bridge(std::move(config));
    NetBridgeShare::Attach([bridge](std::uint16_t kind, const std::vector<std::uint8_t>& payload) { bridge->Event(kind, payload); });
    bridge->Start();
    return bridge;
}

Bridge* Instance() {
    static Bridge* const bridge = Create();
    return bridge;
}

std::vector<std::string> StringList(const char* const* items, std::uint32_t count) {
    std::vector<std::string> list;
    if (items == nullptr) return list;
    for (std::uint32_t index = 0; index < count && index < MaxPushDataTypes; ++index) {
        list.emplace_back(items[index] != nullptr ? items[index] : "");
    }
    return list;
}

}

extern "C" int NetBridgeEnabled_nid_no_patch(void) {
    try {
        return Instance() != nullptr ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

extern "C" std::uint64_t NetBridgeState_nid_no_patch(NetBridgeStateView* out) {
    try {
        if (Bridge* bridge = Instance()) return bridge->State(out);
    } catch (...) {
    }
    if (out != nullptr) std::memset(out, 0, sizeof(*out));
    return 0;
}

extern "C" int NetBridgeWebRequest_nid_no_patch(const NetBridgeWebRequestView* in, NetBridgeWebReplyView* out) {
    if (out != nullptr) std::memset(out, 0, sizeof(*out));
    if (in == nullptr || out == nullptr) return 0;
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return 0;
        if (in->bodySize != 0 && in->body == nullptr) return 0;
        Builder request;
        request.I32(in->userId).Str(in->apiGroup != nullptr ? in->apiGroup : "").Str(in->path != nullptr ? in->path : "").Str(in->method != nullptr ? in->method : "");
        Pairs headers;
        if (in->headerNames != nullptr && in->headerValues != nullptr) {
            for (std::uint32_t index = 0; index < in->headerCount; ++index) {
                headers.emplace_back(in->headerNames[index] != nullptr ? in->headerNames[index] : "", in->headerValues[index] != nullptr ? in->headerValues[index] : "");
            }
        }
        request.WritePairs(headers).Bytes(in->body, in->bodySize);
        if (!request.Ok()) return 0;
        std::vector<std::uint8_t> reply;
        if (!bridge->Request(KindWebRequest, request.Take(), WebTimeout, reply)) return 0;
        Reader reader(reply.data(), reply.size());
        const std::uint16_t status = reader.U16();
        Pairs replyHeaders = reader.ReadPairs();
        const auto body = reader.Bytes();
        if (!reader.Ok() || status == 0) return 0;
        auto holder = std::make_unique<ReplyHolder>();
        holder->names.reserve(replyHeaders.size());
        holder->values.reserve(replyHeaders.size());
        for (auto& [name, value] : replyHeaders) {
            holder->names.push_back(std::move(name));
            holder->values.push_back(std::move(value));
        }
        for (std::size_t index = 0; index < holder->names.size(); ++index) {
            holder->namePointers.push_back(holder->names[index].c_str());
            holder->valuePointers.push_back(holder->values[index].c_str());
        }
        holder->body.assign(body.first, body.first + body.second);
        out->status = status;
        out->headerCount = static_cast<std::uint32_t>(holder->names.size());
        out->headerNames = holder->namePointers.data();
        out->headerValues = holder->valuePointers.data();
        out->body = holder->body.data();
        out->bodySize = holder->body.size();
        out->owner = holder.release();
        return 1;
    } catch (...) {
        std::memset(out, 0, sizeof(*out));
        return 0;
    }
}

extern "C" void NetBridgeWebReplyRelease_nid_no_patch(NetBridgeWebReplyView* reply) {
    if (reply == nullptr) return;
    delete static_cast<ReplyHolder*>(reply->owner);
    std::memset(reply, 0, sizeof(*reply));
}

extern "C" void NetBridgeFeature_nid_no_patch(std::int32_t userId, std::uint32_t feature) {
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return;
        Builder payload;
        payload.I32(userId).U32(feature);
        bridge->Event(KindFeature, payload.Take());
    } catch (...) {
    }
}

extern "C" void NetBridgePushFilter_nid_no_patch(std::int32_t userId, std::uint32_t filter, const char* const* dataTypes, std::uint32_t count) {
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return;
        Builder payload;
        payload.I32(userId).U32(filter).ListOfStrings(StringList(dataTypes, count));
        if (payload.Ok()) bridge->Event(KindPushFilter, payload.Take());
    } catch (...) {
    }
}

extern "C" int NetBridgeTraffic_nid_no_patch(NetBridgeTrafficView* out) {
    if (out != nullptr) std::memset(out, 0, sizeof(*out));
    try {
        Bridge* bridge = Instance();
        return bridge != nullptr && bridge->TrafficPorts(out) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

extern "C" int NetBridgeActivate_nid_no_patch(std::int32_t userId, const char* session, std::uint8_t sessionKind) {
    if (session == nullptr) return -1;
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return ActivateSignedOut;
        return bridge->Activate(userId, session, sessionKind);
    } catch (...) {
        return -1;
    }
}

extern "C" void NetBridgeDeactivate_nid_no_patch(std::int32_t userId, const char* session) {
    if (session == nullptr) return;
    try {
        if (Bridge* bridge = Instance()) bridge->Deactivate(userId, session);
    } catch (...) {
    }
}

extern "C" std::uint32_t NetBridgePeers_nid_no_patch(const char* session, NetBridgePeerView* out, std::uint32_t capacity, std::uint64_t* counter) {
    if (counter != nullptr) *counter = 0;
    if (session == nullptr) return 0;
    try {
        Bridge* bridge = Instance();
        return bridge != nullptr ? bridge->Peers(session, out, capacity, counter) : 0;
    } catch (...) {
        return 0;
    }
}

extern "C" int NetBridgePeerInfo_nid_no_patch(std::uint32_t address, NetBridgePeerInfoView* out) {
    if (out == nullptr) return 0;
    std::memset(out, 0, sizeof(*out));
    try {
        Bridge* bridge = Instance();
        return bridge != nullptr && bridge->PeerInfo(address, out) ? 1 : 0;
    } catch (...) {
        std::memset(out, 0, sizeof(*out));
        return 0;
    }
}

extern "C" int NetBridgeBind_nid_no_patch(std::uint16_t vport, std::uint8_t transport, std::uint16_t loopbackPort) {
    try {
        Bridge* bridge = Instance();
        return bridge != nullptr ? bridge->Bind(vport, transport, loopbackPort) : -1;
    } catch (...) {
        return -1;
    }
}

extern "C" void NetBridgeUnbind_nid_no_patch(std::uint16_t vport, std::uint8_t transport) {
    try {
        if (Bridge* bridge = Instance()) bridge->Unbind(vport, transport);
    } catch (...) {
    }
}

extern "C" void NetBridgePushFilterGone_nid_no_patch(std::int32_t userId, std::uint32_t filter) {
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return;
        Builder payload;
        payload.I32(userId).U32(filter);
        bridge->Event(KindPushFilterGone, payload.Take());
    } catch (...) {
    }
}

extern "C" int NetBridgeRegisterPushHandler_nid_no_patch(NetBridgePushHandler handler, void* userArg) {
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return 0;
        return bridge->RegisterPushHandler(handler, userArg);
    } catch (...) {
        return 0;
    }
}

extern "C" void NetBridgeUnregisterPushHandler_nid_no_patch(int registrationId) {
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return;
        bridge->UnregisterPushHandler(registrationId);
    } catch (...) {
    }
}

extern "C" int NetBridgeRegisterPushPump_nid_no_patch(NetBridgePushPump pump, void* userArg) {
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return 0;
        return bridge->RegisterPushPump(pump, userArg);
    } catch (...) {
        return 0;
    }
}

extern "C" void NetBridgeUnregisterPushPump_nid_no_patch(int registrationId) {
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return;
        bridge->UnregisterPushPump(registrationId);
    } catch (...) {
    }
}

extern "C" void NetBridgeDeliverPushEvents_nid_no_patch(void) {
    try {
        Bridge* bridge = Instance();
        if (bridge == nullptr) return;
        bridge->DeliverPushEvents();
    } catch (...) {
    }
}
