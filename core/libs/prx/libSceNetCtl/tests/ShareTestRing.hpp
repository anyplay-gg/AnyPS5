#ifndef CORE_LIBS_PRX_LIBSCENETCTL_TESTS_SHARETESTRING_HPP
#define CORE_LIBS_PRX_LIBSCENETCTL_TESTS_SHARETESTRING_HPP

#include "prx/libSceNetCtl/tests/FakeNetService.hpp"

#ifdef _WIN32
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

#include "prx/libSceNetCtl/include/ShareRing.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"

#include <cstdint>
#include <string>

namespace ShareTest {

class Ring {
public:
    Ring(const std::string& ringName, std::uint32_t width, std::uint32_t height, std::uint32_t audioFrames, std::uint32_t maxFps = 60) : name(ringName) {
        geometry.width = width;
        geometry.height = height;
        geometry.stride = width * 4;
        geometry.audioFrames = audioFrames;
        size = geometry.TotalBytes();
#ifdef _WIN32
        const std::string path = "Local\\" + name;
        const std::wstring wide(path.begin(), path.end());
        mapping = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(size), wide.c_str());
        FakeNet::Require(mapping != nullptr, "create the ring mapping");
        view = ::MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, size);
        FakeNet::Require(view != nullptr, "map the ring");
#else
        const std::string path = "/" + name;
        descriptor = ::shm_open(path.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
        FakeNet::Require(descriptor >= 0, "create the ring object");
        FakeNet::Require(::ftruncate(descriptor, static_cast<off_t>(size)) == 0, "size the ring object");
        view = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
        FakeNet::Require(view != MAP_FAILED, "map the ring");
#endif
        ShareRing::WriteHeader(Data(), geometry, maxFps);
    }

    Ring(const Ring&) = delete;
    Ring& operator=(const Ring&) = delete;

    ~Ring() {
#ifdef _WIN32
        ::UnmapViewOfFile(view);
        ::CloseHandle(mapping);
#else
        ::munmap(view, size);
        ::close(descriptor);
        ::shm_unlink(("/" + name).c_str());
#endif
    }

    std::uint8_t* Data() const {
        return static_cast<std::uint8_t*>(view);
    }

    std::size_t Size() const {
        return size;
    }

    const ShareRing::Geometry& Layout() const {
        return geometry;
    }

    const std::string& Name() const {
        return name;
    }

    std::uint64_t Seq(std::uint32_t slot) const {
        return ShareRing::Load64(Data() + geometry.SlotOffset(slot) + ShareRing::SlotOffsetSeq);
    }

    std::uint32_t NewestSlot() const {
        std::uint32_t best = 0;
        for (std::uint32_t slot = 1; slot < ShareRing::SlotCount; ++slot) {
            if (Seq(slot) > Seq(best)) best = slot;
        }
        return best;
    }

    const std::uint8_t* Pixels(std::uint32_t slot) const {
        return Data() + geometry.SlotOffset(slot) + ShareRing::SlotHeaderBytes;
    }

    std::uint64_t PresentUs(std::uint32_t slot) const {
        return ShareRing::Load64(Data() + geometry.SlotOffset(slot) + ShareRing::SlotOffsetPresentUs);
    }

    std::uint32_t FrameNumber(std::uint32_t slot) const {
        return ShareRing::Load32(Data() + geometry.SlotOffset(slot) + ShareRing::SlotOffsetFrame);
    }

    std::uint64_t AudioPosition() const {
        return ShareRing::Load64(Data() + ShareRing::OffsetAudioPosition);
    }

    const float* AudioFrames() const {
        return reinterpret_cast<const float*>(Data() + geometry.AudioOffset());
    }

private:
    std::string name;
    ShareRing::Geometry geometry;
    std::size_t size = 0;
    void* view = nullptr;
#ifdef _WIN32
    HANDLE mapping = nullptr;
#else
    int descriptor = -1;
#endif
};

inline FakeNet::Bytes OpenPayload(const std::string& ring, std::uint32_t ringBytes, std::uint32_t width, std::uint32_t height, std::uint32_t maxFps, std::uint8_t mode) {
    FakeNet::Bytes payload;
    FakeNet::PutText(payload, ring);
    FakeNet::Put32(payload, ringBytes);
    FakeNet::Put32(payload, width);
    FakeNet::Put32(payload, height);
    FakeNet::Put32(payload, maxFps);
    payload.push_back(mode);
    return payload;
}

inline FakeNet::Bytes PadPayload(std::uint8_t slot, std::uint16_t sequence, std::uint32_t buttons, std::uint8_t lx, std::uint8_t ly, std::uint8_t rx, std::uint8_t ry, std::uint8_t l2, std::uint8_t r2) {
    FakeNet::Bytes payload;
    payload.push_back(slot);
    FakeNet::Put16(payload, sequence);
    FakeNet::Put32(payload, buttons);
    for (const std::uint8_t axis : {lx, ly, rx, ry, l2, r2}) payload.push_back(axis);
    return payload;
}

}

#endif
