#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/include/ShareRing.hpp"
#include "NetBridgeShare.hpp"
#include "NetBridgeWire.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using namespace NetBridgeWire;
using namespace NetBridgeShare;

constexpr std::size_t MaxNameLength = 30;
constexpr std::uint32_t DefaultFps = 60;
constexpr std::uint32_t MaxFps = 240;
constexpr auto StatusInterval = std::chrono::seconds(1);
constexpr std::uint32_t ButtonMask = 0x7FFFu;

bool ValidName(const std::string& name) {
    static constexpr char Prefix[] = "anyplay-";
    if (name.size() <= sizeof(Prefix) - 1 || name.size() > MaxNameLength || name.rfind(Prefix, 0) != 0) return false;
    for (const char c : name) {
        const bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (!plain) return false;
    }
    return true;
}

class Mapping {
public:
    Mapping() = default;
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;

    ~Mapping() {
        Release();
    }

    bool Open(const std::string& name, std::size_t bytes) {
        Release();
#ifdef _WIN32
        // The local namespace of the user's session, written out; the share worker makes the
        // mapping with the bare name, which means the same.
        const std::string path = "Local\\" + name;
        std::wstring wide(path.begin(), path.end());
        mapping = ::OpenFileMappingW(FILE_MAP_WRITE, FALSE, wide.c_str());
        if (mapping == nullptr) return false;
        view = ::MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, bytes);
        if (view == nullptr) {
            Release();
            return false;
        }
#else
        const std::string path = "/" + name;
        descriptor = ::shm_open(path.c_str(), O_RDWR, 0);
        if (descriptor < 0) return false;
        struct stat info{};
        // Only the user who runs the game may be able to read the ring.
        if (::fstat(descriptor, &info) != 0 || info.st_size < 0 || static_cast<std::size_t>(info.st_size) < bytes || info.st_uid != ::geteuid() || (info.st_mode & 077) != 0) {
            Release();
            return false;
        }
        void* mapped = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
        if (mapped == MAP_FAILED) {
            Release();
            return false;
        }
        view = mapped;
#endif
        size = bytes;
        return true;
    }

    std::uint8_t* Data() const {
        return static_cast<std::uint8_t*>(view);
    }

    std::size_t Size() const {
        return size;
    }

    void Release() {
#ifdef _WIN32
        if (view != nullptr) ::UnmapViewOfFile(view);
        if (mapping != nullptr) ::CloseHandle(mapping);
        mapping = nullptr;
#else
        if (view != nullptr) ::munmap(view, size);
        if (descriptor >= 0) ::close(descriptor);
        descriptor = -1;
#endif
        view = nullptr;
        size = 0;
    }

private:
    void* view = nullptr;
    std::size_t size = 0;
#ifdef _WIN32
    HANDLE mapping = nullptr;
#else
    int descriptor = -1;
#endif
};

// Three locks, never held together except in one order, ring then audio (Open and Close only):
// - ringMutex: the frame side of the producer (a full-frame copy under it);
// - audioMutex: the audio side, so that a frame copy never costs an audio block;
// - padMutex: the guest pad, the mode, the sender and the open flag of the share. A pad read from
//   libScePad (under its stateMutex) takes only this one and never waits for a copy.
// The frame and the audio fields of the producer are disjoint; Attach and Detach change the
// mapping under both ring locks, and a submit reads it under its own.
struct State {
    std::mutex ringMutex;
    std::mutex audioMutex;
    std::mutex padMutex;
    Sender sender;
    bool open = false;
    bool statusThread = false;
    std::uint8_t mode = ModeWatch;
    std::atomic<std::uint32_t> maxFps{DefaultFps};
    Mapping mapping;
    ShareRing::Producer ring;
    NetBridgeSharePadView pad{};
    std::uint64_t serial = 0;
    std::atomic<bool> active{false};
    std::atomic<std::uint8_t> padMode{ModeWatch};
    std::atomic<std::int64_t> lastFrameNs{0};
    std::atomic<std::uint32_t> frames{0};
    std::atomic<std::uint32_t> dropped{0};
    std::atomic<std::uint32_t> audioUnderruns{0};
};

State& Share() {
    static State* const state = new State;
    return *state;
}

std::uint8_t SlotOf(std::uint8_t mode) {
    return mode == ModeTogether ? 1 : 0;
}

// With padMutex held.
void Neutralize(State& state) {
    std::memset(&state.pad, 0, sizeof(state.pad));
    std::memset(state.pad.sticks, 0x80, sizeof(state.pad.sticks));
    state.pad.mode = state.mode;
    state.pad.slot = SlotOf(state.mode);
    state.pad.serial = ++state.serial;
    state.padMode.store(state.open ? state.mode : ModeWatch);
}

void StatusLoop() {
    State& state = Share();
    while (true) {
        std::this_thread::sleep_for(StatusInterval);
        Sender send;
        Builder payload;
        {
            std::lock_guard lock(state.padMutex);
            if (!state.open) continue;
            send = state.sender;
            payload.U32(state.frames.load()).U32(state.dropped.load()).U32(state.audioUnderruns.load());
        }
        if (send) send(KindShareStatus, payload.Take());
    }
}

std::int64_t NowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}

namespace NetBridgeShare {

void Attach(Sender sender) {
    State& state = Share();
    std::lock_guard lock(state.padMutex);
    state.sender = std::move(sender);
}

std::uint8_t Open(const std::vector<std::uint8_t>& payload) {
    Reader reader(payload.data(), payload.size());
    const std::string name = reader.Str();
    const std::uint32_t ringBytes = reader.U32();
    const std::uint32_t width = reader.U32();
    const std::uint32_t height = reader.U32();
    const std::uint32_t maxFps = reader.U32();
    const std::uint8_t mode = reader.U8();
    if (!reader.Ok()) return OpenUnsupported;
    if (!ValidName(name) || mode > ModeTogether || ringBytes < ShareRing::HeaderBytes || ringBytes > ShareRing::MaxRingBytes) return OpenCannot;
    State& state = Share();
    std::scoped_lock ringLocks(state.ringMutex, state.audioMutex);
    if (state.ring.Attached()) return OpenRunning;
    if (!state.mapping.Open(name, ringBytes)) return OpenCannot;
    if (!state.ring.Attach(state.mapping.Data(), state.mapping.Size(), width, height)) {
        state.mapping.Release();
        return OpenCannot;
    }
    state.maxFps.store(maxFps == 0 ? DefaultFps : (maxFps > MaxFps ? MaxFps : maxFps));
    state.frames.store(0);
    state.dropped.store(0);
    state.audioUnderruns.store(0);
    state.lastFrameNs.store(0);
    {
        std::lock_guard lock(state.padMutex);
        state.open = true;
        state.mode = mode;
        Neutralize(state);
    }
    state.active.store(true);
    if (!state.statusThread) {
        state.statusThread = true;
        std::thread(StatusLoop).detach();
    }
    return OpenMapped;
}

void Close() {
    State& state = Share();
    state.active.store(false);
    {
        std::lock_guard lock(state.padMutex);
        state.open = false;
        state.mode = ModeWatch;
        Neutralize(state);
    }
    std::scoped_lock ringLocks(state.ringMutex, state.audioMutex);
    state.ring.Detach();
    state.mapping.Release();
}

void Mode(const std::vector<std::uint8_t>& payload) {
    Reader reader(payload.data(), payload.size());
    const std::uint8_t mode = reader.U8();
    if (!reader.Ok() || mode > ModeTogether) return;
    State& state = Share();
    std::lock_guard lock(state.padMutex);
    if (!state.open) return;
    state.mode = mode;
    Neutralize(state);
}

void Pad(const std::vector<std::uint8_t>& payload) {
    Reader reader(payload.data(), payload.size());
    const std::uint8_t slot = reader.U8();
    const std::uint16_t sequence = reader.U16();
    const std::uint32_t buttons = reader.U32();
    std::uint8_t axes[6];
    for (auto& axis : axes) axis = reader.U8();
    if (!reader.Ok() || (buttons & ~ButtonMask) != 0) return;
    State& state = Share();
    std::lock_guard lock(state.padMutex);
    if (!state.open || state.mode == ModeWatch || slot != SlotOf(state.mode)) return;
    static_cast<void>(sequence);
    state.pad.buttons = buttons;
    std::memcpy(state.pad.sticks, axes, 4);
    state.pad.l2 = axes[4];
    state.pad.r2 = axes[5];
    state.pad.mode = state.mode;
    state.pad.slot = slot;
    state.pad.serial = ++state.serial;
}

}

extern "C" int NetBridgeShareActive_nid_no_patch(void) {
    return Share().active.load() ? 1 : 0;
}

// The frame-rate cap is not a drop: a game that presents faster than the stream wants loses
// nothing the stream asked for, so `dropped` counts only refused and busy frames.
extern "C" int NetBridgeShareWantsFrame_nid_no_patch(void) {
    State& state = Share();
    if (!state.active.load()) return 0;
    const std::int64_t now = NowNs();
    const std::int64_t interval = 1000000000ll / state.maxFps.load();
    if (now - state.lastFrameNs.load() < interval * 9 / 10) return 0;
    state.lastFrameNs.store(now);
    return 1;
}

extern "C" int NetBridgeShareSubmitFrame_nid_no_patch(const void* bgra, std::uint32_t width, std::uint32_t height, std::uint32_t stride, std::uint64_t presentUs, std::uint32_t frame) {
    State& state = Share();
    if (!state.active.load()) return 0;
    try {
        std::unique_lock lock(state.ringMutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            state.dropped.fetch_add(1);
            return 0;
        }
        if (!state.ring.SubmitFrame(static_cast<const std::uint8_t*>(bgra), width, height, stride, presentUs, frame)) {
            state.dropped.fetch_add(1);
            return 0;
        }
        state.frames.fetch_add(1);
        return 1;
    } catch (...) {
        return 0;
    }
}

extern "C" int NetBridgeShareSubmitAudio_nid_no_patch(const float* stereo, std::uint32_t frames) {
    State& state = Share();
    if (!state.active.load()) return 0;
    try {
        std::unique_lock lock(state.audioMutex, std::try_to_lock);
        if (!lock.owns_lock() || !state.ring.SubmitAudio(stereo, frames)) {
            state.audioUnderruns.fetch_add(1);
            return 0;
        }
        return 1;
    } catch (...) {
        return 0;
    }
}

extern "C" int NetBridgeSharePad_nid_no_patch(NetBridgeSharePadView* out) {
    State& state = Share();
    if (out != nullptr) std::memset(out, 0, sizeof(*out));
    if (state.padMode.load() == ModeWatch || out == nullptr) return 0;
    std::lock_guard lock(state.padMutex);
    if (!state.open || state.mode == ModeWatch) return 0;
    *out = state.pad;
    return 1;
}

extern "C" int NetBridgeShareOverride_nid_no_patch(std::uint8_t trigger) {
    State& state = Share();
    std::uint8_t action = 0;
    Sender send;
    {
        std::lock_guard lock(state.padMutex);
        if (!state.open) return 0;
        const bool chord = trigger == 0;
        action = chord && state.mode != ModeWatch ? ActionTookControl : ActionStop;
        if (state.mode != ModeWatch) {
            state.mode = ModeWatch;
            Neutralize(state);
        }
        send = state.sender;
    }
    if (send) send(KindOverride, std::vector<std::uint8_t>{action});
    return action;
}
