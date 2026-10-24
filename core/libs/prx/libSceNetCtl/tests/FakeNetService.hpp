#ifndef CORE_LIBS_PRX_LIBSCENETCTL_TESTS_FAKENETSERVICE_HPP
#define CORE_LIBS_PRX_LIBSCENETCTL_TESTS_FAKENETSERVICE_HPP

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace FakeNet {

using Bytes = std::vector<std::uint8_t>;

#ifdef _WIN32
using Handle = SOCKET;
constexpr Handle InvalidHandle = INVALID_SOCKET;
#else
using Handle = int;
constexpr Handle InvalidHandle = -1;
#endif

inline void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "fake net service test: %s\n", message);
        std::fflush(stderr);
        std::abort();
    }
}

inline void Put16(Bytes& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}

inline void Put32(Bytes& out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) out.push_back(static_cast<std::uint8_t>(value >> shift));
}

inline void Put64(Bytes& out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) out.push_back(static_cast<std::uint8_t>(value >> shift));
}

inline void PutText(Bytes& out, const std::string& text) {
    Put16(out, static_cast<std::uint16_t>(text.size()));
    out.insert(out.end(), text.begin(), text.end());
}

inline void PutBlob(Bytes& out, const Bytes& blob) {
    Put32(out, static_cast<std::uint32_t>(blob.size()));
    out.insert(out.end(), blob.begin(), blob.end());
}

inline Bytes MakeFrame(std::uint16_t kind, std::uint32_t request, const Bytes& payload) {
    Bytes frame;
    Put32(frame, static_cast<std::uint32_t>(payload.size()));
    Put16(frame, kind);
    Put16(frame, 0);
    Put32(frame, request);
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

class Cursor {
public:
    explicit Cursor(const Bytes& data) : data(data) {
    }

    bool Ok() const {
        return good;
    }

    std::size_t Left() const {
        return data.size() - position;
    }

    std::uint64_t Number(std::size_t width) {
        if (!good || Left() < width) {
            good = false;
            return 0;
        }
        std::uint64_t value = 0;
        for (std::size_t index = 0; index < width; ++index) value |= static_cast<std::uint64_t>(data[position + index]) << (8 * index);
        position += width;
        return value;
    }

    std::string Text() {
        const std::size_t length = static_cast<std::size_t>(Number(2));
        if (!good || Left() < length) {
            good = false;
            return {};
        }
        std::string text(reinterpret_cast<const char*>(data.data() + position), length);
        position += length;
        return text;
    }

    Bytes Blob() {
        const std::size_t length = static_cast<std::size_t>(Number(4));
        if (!good || Left() < length) {
            good = false;
            return {};
        }
        Bytes blob(data.begin() + static_cast<std::ptrdiff_t>(position), data.begin() + static_cast<std::ptrdiff_t>(position + length));
        position += length;
        return blob;
    }

private:
    const Bytes& data;
    std::size_t position = 0;
    bool good = true;
};

inline void CloseHandleSafe(Handle handle) {
#ifdef _WIN32
    ::closesocket(handle);
#else
    ::close(handle);
#endif
}

inline void ShutdownHandle(Handle handle) {
#ifdef _WIN32
    ::shutdown(handle, SD_BOTH);
#else
    ::shutdown(handle, SHUT_RDWR);
#endif
}

inline bool ReadExact(Handle handle, std::uint8_t* data, std::size_t size) {
    while (size > 0) {
        const auto got = ::recv(handle, reinterpret_cast<char*>(data), static_cast<int>(size), 0);
        if (got <= 0) return false;
        data += got;
        size -= static_cast<std::size_t>(got);
    }
    return true;
}

inline bool WriteExact(Handle handle, const Bytes& frame) {
#ifdef MSG_NOSIGNAL
    constexpr int flags = MSG_NOSIGNAL;
#else
    constexpr int flags = 0;
#endif
    std::size_t done = 0;
    while (done < frame.size()) {
        const auto sent = ::send(handle, reinterpret_cast<const char*>(frame.data() + done), static_cast<int>(frame.size() - done), flags);
        if (sent <= 0) return false;
        done += static_cast<std::size_t>(sent);
    }
    return true;
}

struct WebCall {
    std::int32_t userId = 0;
    std::string apiGroup;
    std::string path;
    std::string method;
    std::vector<std::pair<std::string, std::string>> headers;
    Bytes body;
};

struct PushFilterCall {
    std::int32_t userId = 0;
    std::uint32_t filter = 0;
    std::vector<std::string> dataTypes;
};

class Service {
public:
    explicit Service(Bytes expectedToken) : token(std::move(expectedToken)) {
#ifdef _WIN32
        WSADATA data{};
        Require(WSAStartup(MAKEWORD(2, 2), &data) == 0, "WSAStartup");
#endif
        listener = ::socket(AF_INET, SOCK_STREAM, 0);
        Require(listener != InvalidHandle, "socket");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        Require(::bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0, "bind");
        Require(::listen(listener, 4) == 0, "listen");
        socklen_t length = sizeof(address);
        Require(::getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) == 0, "getsockname");
        port = ntohs(address.sin_port);
        worker = std::thread([this] { acceptLoop(); });
    }

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    ~Service() {
        stopping.store(true);
        ShutdownHandle(listener);
        CloseHandleSafe(listener);
        DropConnection();
        if (worker.joinable()) worker.join();
    }

    std::uint16_t Port() const {
        return port;
    }

    void SetState(bool signedIn, std::uint64_t account, const std::string& handle, std::uint8_t natType, const std::uint8_t (&address)[4]) {
        Bytes payload;
        payload.push_back(signedIn ? 1 : 0);
        Put64(payload, account);
        PutText(payload, handle);
        payload.push_back(natType);
        payload.insert(payload.end(), address, address + 4);
        {
            std::lock_guard lock(mutex);
            statePayload = payload;
        }
        sendToClient(MakeFrame(0x0101, 0, payload));
    }

    void SendRaw(const Bytes& frame) {
        sendToClient(frame);
    }

    void DropConnection() {
        std::lock_guard lock(sendMutex);
        if (client != InvalidHandle) ShutdownHandle(client);
    }

    void HoldReplies(int count) {
        hold.store(count);
    }

    int Welcomed() const {
        return welcomed.load();
    }

    int Rejected() const {
        return rejected.load();
    }

    int Unsupported() const {
        return unsupported.load();
    }

    int Malformed() const {
        return malformed.load();
    }

    std::vector<WebCall> WebCalls() {
        std::lock_guard lock(mutex);
        return webCalls;
    }

    std::vector<std::pair<std::int32_t, std::uint32_t>> Features() {
        std::lock_guard lock(mutex);
        return features;
    }

    std::vector<PushFilterCall> PushFilters() {
        std::lock_guard lock(mutex);
        return pushFilters;
    }

    std::vector<std::pair<std::int32_t, std::uint32_t>> PushGone() {
        std::lock_guard lock(mutex);
        return pushGone;
    }

    static bool WaitUntil(const std::function<bool()>& condition, int milliseconds) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
        while (std::chrono::steady_clock::now() < deadline) {
            if (condition()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return condition();
    }

private:
    void sendToClient(const Bytes& frame) {
        std::lock_guard lock(sendMutex);
        if (client != InvalidHandle) WriteExact(client, frame);
    }

    void acceptLoop() {
        while (!stopping.load()) {
            const Handle accepted = ::accept(listener, nullptr, nullptr);
            if (accepted == InvalidHandle) {
                if (stopping.load()) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            serve(accepted);
            {
                std::lock_guard lock(sendMutex);
                client = InvalidHandle;
            }
            CloseHandleSafe(accepted);
        }
    }

    bool readFrame(Handle handle, std::uint16_t& kind, std::uint32_t& request, Bytes& payload) {
        std::uint8_t header[12];
        if (!ReadExact(handle, header, sizeof(header))) return false;
        const Bytes headerBytes(header, header + sizeof(header));
        Cursor parsed(headerBytes);
        const std::uint32_t length = static_cast<std::uint32_t>(parsed.Number(4));
        kind = static_cast<std::uint16_t>(parsed.Number(2));
        const std::uint16_t flags = static_cast<std::uint16_t>(parsed.Number(2));
        request = static_cast<std::uint32_t>(parsed.Number(4));
        if (flags != 0 || length > 16u * 1024u * 1024u) {
            malformed.fetch_add(1);
            return false;
        }
        payload.assign(length, 0);
        return length == 0 || ReadExact(handle, payload.data(), payload.size());
    }

    void serve(Handle handle) {
        std::uint16_t kind = 0;
        std::uint32_t request = 0;
        Bytes payload;
        if (!readFrame(handle, kind, request, payload)) return;
        const bool helloOk = kind == 0x0001 && request == 0 && payload.size() == 2 + token.size() && payload[0] == 0 && payload[1] == 0 &&
            std::equal(token.begin(), token.end(), payload.begin() + 2);
        if (!helloOk) {
            rejected.fetch_add(1);
            const std::string reason = "wrong hello";
            WriteExact(handle, MakeFrame(0x0003, 0, Bytes(reason.begin(), reason.end())));
            return;
        }
        welcomed.fetch_add(1);
        Bytes welcome;
        Put16(welcome, 0);
        Put16(welcome, 0);
        Put16(welcome, 0);
        for (int index = 0; index < 16; ++index) welcome.push_back(static_cast<std::uint8_t>(index));
        Bytes state;
        {
            std::lock_guard lock(mutex);
            state = statePayload;
        }
        {
            std::lock_guard lock(sendMutex);
            client = handle;
            if (!WriteExact(handle, MakeFrame(0x0002, 0, welcome))) return;
            if (!state.empty() && !WriteExact(handle, MakeFrame(0x0101, 0, state))) return;
        }
        std::vector<std::pair<std::uint32_t, Bytes>> held;
        while (readFrame(handle, kind, request, payload)) {
            if (kind == 0x0201 && request != 0) {
                answerWeb(request, payload, held);
            } else if (kind == 0x0102) {
                Cursor cursor(payload);
                const auto user = static_cast<std::int32_t>(cursor.Number(4));
                const auto feature = static_cast<std::uint32_t>(cursor.Number(4));
                if (!cursor.Ok()) malformed.fetch_add(1);
                std::lock_guard lock(mutex);
                features.emplace_back(user, feature);
            } else if (kind == 0x0202) {
                Cursor cursor(payload);
                PushFilterCall call;
                call.userId = static_cast<std::int32_t>(cursor.Number(4));
                call.filter = static_cast<std::uint32_t>(cursor.Number(4));
                const auto count = static_cast<std::size_t>(cursor.Number(2));
                for (std::size_t index = 0; index < count && cursor.Ok(); ++index) call.dataTypes.push_back(cursor.Text());
                if (!cursor.Ok()) malformed.fetch_add(1);
                std::lock_guard lock(mutex);
                pushFilters.push_back(std::move(call));
            } else if (kind == 0x0203) {
                Cursor cursor(payload);
                const auto user = static_cast<std::int32_t>(cursor.Number(4));
                const auto filter = static_cast<std::uint32_t>(cursor.Number(4));
                if (!cursor.Ok()) malformed.fetch_add(1);
                std::lock_guard lock(mutex);
                pushGone.emplace_back(user, filter);
            } else if (kind == 0x8000) {
                unsupported.fetch_add(1);
            } else if (request != 0) {
                sendToClient(MakeFrame(0x8000, request, {}));
            }
        }
    }

    void answerWeb(std::uint32_t request, const Bytes& payload, std::vector<std::pair<std::uint32_t, Bytes>>& held) {
        Cursor cursor(payload);
        WebCall call;
        call.userId = static_cast<std::int32_t>(cursor.Number(4));
        call.apiGroup = cursor.Text();
        call.path = cursor.Text();
        call.method = cursor.Text();
        const auto headerCount = static_cast<std::size_t>(cursor.Number(2));
        for (std::size_t index = 0; index < headerCount && cursor.Ok(); ++index) {
            std::string name = cursor.Text();
            std::string value = cursor.Text();
            call.headers.emplace_back(std::move(name), std::move(value));
        }
        call.body = cursor.Blob();
        if (!cursor.Ok() || cursor.Left() != 0) {
            malformed.fetch_add(1);
            sendToClient(MakeFrame(0x8000, request, {}));
            return;
        }
        Bytes reply;
        if (call.path == "/offline") {
            Put16(reply, 0);
            Put16(reply, 0);
            PutBlob(reply, {});
        } else {
            Put16(reply, 200);
            Put16(reply, 2);
            PutText(reply, "content-type");
            PutText(reply, "text/plain");
            PutText(reply, "x-path");
            PutText(reply, call.path);
            std::string echo = "echo:" + call.method + ":" + call.path + ":";
            Bytes body(echo.begin(), echo.end());
            body.insert(body.end(), call.body.begin(), call.body.end());
            PutBlob(reply, body);
        }
        {
            std::lock_guard lock(mutex);
            webCalls.push_back(std::move(call));
        }
        held.emplace_back(request, std::move(reply));
        if (static_cast<int>(held.size()) < hold.load()) return;
        for (auto it = held.rbegin(); it != held.rend(); ++it) sendToClient(MakeFrame(0x8201, it->first, it->second));
        held.clear();
    }

    const Bytes token;
    Handle listener = InvalidHandle;
    Handle client = InvalidHandle;
    std::uint16_t port = 0;
    std::thread worker;
    std::atomic<bool> stopping{false};
    std::atomic<int> hold{1};
    std::atomic<int> welcomed{0};
    std::atomic<int> rejected{0};
    std::atomic<int> malformed{0};
    std::atomic<int> unsupported{0};
    std::mutex mutex;
    std::mutex sendMutex;
    Bytes statePayload;
    std::vector<WebCall> webCalls;
    std::vector<std::pair<std::int32_t, std::uint32_t>> features;
    std::vector<PushFilterCall> pushFilters;
    std::vector<std::pair<std::int32_t, std::uint32_t>> pushGone;
};

inline void SetEnvironment(const char* name, const std::string& value) {
#ifdef _WIN32
    Require(_putenv_s(name, value.c_str()) == 0, "putenv");
#else
    Require(setenv(name, value.c_str(), 1) == 0, "setenv");
#endif
}

inline std::string HexOf(const Bytes& bytes) {
    static constexpr char Digits[] = "0123456789abcdef";
    std::string text;
    for (const auto byte : bytes) {
        text.push_back(Digits[byte >> 4]);
        text.push_back(Digits[byte & 0x0F]);
    }
    return text;
}

}

#endif
