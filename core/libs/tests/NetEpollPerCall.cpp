// A managed runtime (Mono behind Socket.Poll) answers every poll with a throwaway epoll:
// create, add, wait with a zero timeout, delete, destroy. These checks drive that pattern.
#include "../prx/libSceNet/Export.cpp"
#include <filesystem>
#include <fstream>
#include <future>
#include <source_location>

static void Require(bool value, const std::source_location location = std::source_location::current()) {
    if (value) return;
    std::fprintf(stderr, "requirement failed at %s:%u\n", location.file_name(), location.line());
    std::abort();
}

static constexpr std::uint32_t In = 1, Out = 2, Err = 8, Hup = 16;
static constexpr int NonBlockingOption = 0x1200;

// One Socket.Poll(0, mode): returns the events the wait reported, or -1 for none.
static std::int64_t PollOnce(int descriptor, std::uint32_t mask) {
    const int epoll = sceNetEpollCreate("per-call", 0);
    Require(epoll >= 0);
    NetEpollEvent request{};
    request.events = mask;
    request.data.u64 = 0x1234;
    Require(sceNetEpollControl(epoll, 1, descriptor, &request) == 0);
    NetEpollEvent ready{};
    const int count = sceNetEpollWait(epoll, &ready, 16, 0);
    Require(count == 0 || count == 1);
    if (count == 1) Require(ready.ident == static_cast<std::uint64_t>(descriptor) && ready.data.u64 == 0x1234);
    Require(sceNetEpollControl(epoll, 3, descriptor, nullptr) == 0);
    Require(sceNetEpollDestroy(epoll) == 0);
    return count == 0 ? -1 : static_cast<std::int64_t>(ready.events);
}

static std::int64_t PollUntil(int descriptor, std::uint32_t mask, std::chrono::milliseconds limit) {
    const auto deadline = std::chrono::steady_clock::now() + limit;
    for (;;) {
        const auto seen = PollOnce(descriptor, mask);
        if (seen != -1 || std::chrono::steady_clock::now() >= deadline) return seen;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

static std::array<std::uint8_t, 16> LoopbackAddress(int descriptor) {
    std::array<std::uint8_t, 16> address{16, 2, 0, 0, 127, 0, 0, 1};
    Require(sceNetBind_nid_postfix(descriptor, address.data(), address.size()) == 0);
    std::uint32_t size = address.size();
    Require(sceNetGetsockname(descriptor, address.data(), &size) == 0 && size == address.size());
    return address;
}

static void SetNonBlocking(int descriptor) {
    const int on = 1;
    Require(sceNetSetsockopt(descriptor, 0xffff, NonBlockingOption, &on, sizeof(on)) == 0);
}

static void CheckUdp() {
    const int sender = sceNetSocket("per-call-udp", 2, 2, 17);
    const int receiver = sceNetSocket("per-call-udp", 2, 2, 17);
    Require(sender >= 0 && receiver >= 0);
    SetNonBlocking(sender);
    const auto address = LoopbackAddress(receiver);
    // A fresh epoll must report a writable datagram socket at once, every time, and nothing readable.
    for (int round = 0; round != 2000; ++round) {
        Require(PollOnce(sender, Out) == Out);
        Require(PollOnce(sender, In | Out) == Out);
    }
    for (int round = 0; round != 300; ++round) Require(PollOnce(receiver, In) == -1);
    const char payload[] = "per-call";
    Require(sceNetSendto(sender, payload, sizeof(payload), 0, address.data(), address.size()) == sizeof(payload));
    Require(PollUntil(receiver, In, std::chrono::seconds(5)) == In);
    Require(PollOnce(receiver, In) == In);  // level triggered: still readable until read
    Require(PollOnce(receiver, Out) == Out);
    Require(sceNetSocketClose(sender) == 0 && sceNetSocketClose(receiver) == 0);
}

static void CheckTcpConnect() {
    const int listener = sceNetSocket("per-call-tcp", 2, 1, 6);
    Require(listener >= 0);
    const auto address = LoopbackAddress(listener);
    Require(sceNetListen(listener, 4) == 0);
    const int client = sceNetSocket("per-call-tcp", 2, 1, 6);
    Require(client >= 0);
    SetNonBlocking(client);
    const int result = sceNetConnect(client, address.data(), address.size());
    Require(result == 0 || (result == static_cast<int>(0x80410124) && *sceNetErrnoLoc() == 36));
    // Mid-connect (or just connected): writable is the connect-complete signal and must arrive.
    const auto writable = PollUntil(client, Out, std::chrono::seconds(5));
    Require(writable > 0 && (writable & Out) && !(writable & Err));
    int error = -1;
    std::uint32_t size = sizeof(error);
    Require(sceNetGetsockopt(client, 0xffff, 0x1007, &error, &size) == 0 && error == 0);
    const int accepted = sceNetAccept(listener, nullptr, nullptr);
    Require(accepted >= 0);
    Require(PollOnce(client, In) == -1);
    const char byte = 'x';
    Require(sceNetSend(accepted, &byte, 1, 0) == 1);
    Require(PollUntil(client, In, std::chrono::seconds(5)) == In);
    // The peer going away is reported as readable or hang-up, never as silence.
    char received = 0;
    Require(sceNetRecv(client, &received, 1, 0) == 1);
    Require(sceNetSocketClose(accepted) == 0);
    const auto closed = PollUntil(client, In, std::chrono::seconds(5));
    Require(closed > 0 && (closed & (In | Hup)));
    for (const int descriptor : {client, listener}) Require(sceNetSocketClose(descriptor) == 0);
}

static void CheckTcpRefused() {
    const int probe = sceNetSocket("per-call-closed", 2, 1, 6);
    Require(probe >= 0);
    const auto address = LoopbackAddress(probe);
    Require(sceNetSocketClose(probe) == 0);  // nothing listens on this port any more
    const int client = sceNetSocket("per-call-closed", 2, 1, 6);
    Require(client >= 0);
    SetNonBlocking(client);
    const int result = sceNetConnect(client, address.data(), address.size());
    if (result != 0) {
        // A failed connect must wake a poll that asks for writable or for errors.
        const auto seen = PollUntil(client, Out | Err, std::chrono::seconds(10));
        Require(seen > 0 && (seen & (Err | Hup | Out)));
    }
    Require(sceNetSocketClose(client) == 0);
}

static void CheckClosedDescriptor() {
    const int socket = sceNetSocket("per-call-close", 2, 2, 17);
    Require(socket >= 0);
    const int epoll = sceNetEpollCreate("per-call-close", 0);
    NetEpollEvent request{};
    request.events = Out;
    Require(sceNetEpollControl(epoll, 1, socket, &request) == 0);
    Require(sceNetSocketClose(socket) == 0);
    NetEpollEvent ready{};
    // The closed descriptor leaves the epoll: no event, and no later control succeeds on it.
    Require(sceNetEpollWait(epoll, &ready, 1, 0) == 0);
    for (const int operation : {1, 2, 3}) {
        Require(sceNetEpollControl(epoll, operation, socket, operation == 3 ? nullptr : &request) == static_cast<int>(0x80410109));
        Require(*sceNetErrnoLoc() == 9);
    }
    Require(sceNetEpollDestroy(epoll) == 0);
    Require(sceNetEpollDestroy(epoll) == static_cast<int>(0x80410109));
}

// Seven threads that never sleep must all finish, and they are paced instead of spinning at
// full speed (the pacing starts after a few hundred hot empty polls from one thread).
static void CheckSpinningThreads() {
    std::array<int, 7> sockets{};
    for (int& socket : sockets) {
        socket = sceNetSocket("per-call-spin", 2, 2, 17);
        Require(socket >= 0);
    }
    const auto start = std::chrono::steady_clock::now();
    std::vector<std::future<int>> workers;
    for (const int socket : sockets)
        workers.push_back(std::async(std::launch::async, [socket] {
            int polls = 0;
            while (polls < 1500) {
                Require(PollOnce(socket, In) == -1);
                ++polls;
            }
            return polls;
        }));
    for (auto& worker : workers) Require(worker.get() == 1500);
    Require(std::chrono::steady_clock::now() - start > std::chrono::milliseconds(200));
    for (const int socket : sockets) Require(sceNetSocketClose(socket) == 0);
}

// The observation log must show what the game asks an epoll for and what the wait reported.
static void CheckObservation(const std::filesystem::path& path) {
    const int socket = sceNetSocket("per-call-observe", 2, 2, 17);
    Require(socket >= 0);
    Require(PollOnce(socket, In | Out) == Out);
    Require(sceNetSocketClose(socket) == 0);
    std::string text;
    for (int attempt = 0; attempt != 40 && text.find("sceNetEpollWait") == std::string::npos; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::ifstream input(path, std::ios::binary);
        text.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }
    Require(text.find("\"fn\":\"sceNetEpollControl\",\"in\":{\"eid\":") != std::string::npos);
    Require(text.find("\"events\":3}") != std::string::npos);
    Require(text.find("\"ready\":{\"" + std::to_string(socket) + "\":2}") != std::string::npos);
}

int main(int argc, char** argv) {
    Require(argc == 2);
    const std::string mode = argv[1];
    const auto observePath = std::filesystem::temp_directory_path() / "anyps5-net-epoll-per-call.jsonl";
    if (mode == "observe") {
        std::error_code ignored;
        std::filesystem::remove(observePath, ignored);
#ifdef _WIN32
        Require(_putenv_s("ANYPS5_NET_OBSERVE", observePath.string().c_str()) == 0);
#else
        Require(setenv("ANYPS5_NET_OBSERVE", observePath.c_str(), 1) == 0);
#endif
    }
    Require(sceNetInit_nid_postfix() == 0);
    if (mode == "udp") CheckUdp();
    else if (mode == "tcp") CheckTcpConnect();
    else if (mode == "refused") CheckTcpRefused();
    else if (mode == "closed") CheckClosedDescriptor();
    else if (mode == "spin") CheckSpinningThreads();
    else if (mode == "observe") CheckObservation(observePath);
    else Require(false);
    std::printf("epoll per call %s passed\n", mode.c_str());
}
