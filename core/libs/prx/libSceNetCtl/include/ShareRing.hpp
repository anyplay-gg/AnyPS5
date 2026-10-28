#ifndef CORE_LIBS_PRX_LIBSCENETCTL_INCLUDE_SHARERING_HPP
#define CORE_LIBS_PRX_LIBSCENETCTL_INCLUDE_SHARERING_HPP

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

static_assert(std::endian::native == std::endian::little);

namespace ShareRing {

constexpr char Magic[8] = {'A', 'N', 'Y', 'S', 'H', 'R', '0', '0'};
constexpr std::uint32_t LayoutVersion = 0;
constexpr std::uint32_t SlotCount = 3;
constexpr std::uint32_t FormatBgra8 = 1;
constexpr std::size_t HeaderBytes = 64;
constexpr std::size_t SlotHeaderBytes = 32;
constexpr std::size_t AudioFrameBytes = 8;
constexpr std::size_t MaxRingBytes = std::size_t{512} * 1024 * 1024;
constexpr std::uint32_t MaxDimension = 8192;

constexpr std::size_t OffsetMagic = 0;
constexpr std::size_t OffsetLayout = 8;
constexpr std::size_t OffsetSlots = 12;
constexpr std::size_t OffsetWidth = 16;
constexpr std::size_t OffsetHeight = 20;
constexpr std::size_t OffsetStride = 24;
constexpr std::size_t OffsetFormat = 28;
constexpr std::size_t OffsetMaxFps = 32;
constexpr std::size_t OffsetAudioFrames = 36;
constexpr std::size_t OffsetAudioPosition = 40;

constexpr std::size_t SlotOffsetSeq = 0;
constexpr std::size_t SlotOffsetPresentUs = 8;
constexpr std::size_t SlotOffsetKind = 16;
constexpr std::size_t SlotOffsetFrame = 20;

struct Geometry {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t stride = 0;
    std::uint32_t audioFrames = 0;

    std::size_t SlotBytes() const {
        return SlotHeaderBytes + static_cast<std::size_t>(stride) * height;
    }

    std::size_t SlotOffset(std::uint32_t slot) const {
        return HeaderBytes + SlotBytes() * slot;
    }

    std::size_t AudioOffset() const {
        return HeaderBytes + SlotBytes() * SlotCount;
    }

    std::size_t TotalBytes() const {
        return AudioOffset() + static_cast<std::size_t>(audioFrames) * AudioFrameBytes;
    }
};

inline bool PowerOfTwoOrZero(std::uint32_t value) {
    return (value & (value - 1)) == 0;
}

inline void Store32(std::uint8_t* at, std::uint32_t value) {
    std::memcpy(at, &value, sizeof(value));
}

inline std::uint32_t Load32(const std::uint8_t* at) {
    std::uint32_t value = 0;
    std::memcpy(&value, at, sizeof(value));
    return value;
}

inline void Store64(std::uint8_t* at, std::uint64_t value) {
    std::memcpy(at, &value, sizeof(value));
}

inline std::uint64_t Load64(const std::uint8_t* at) {
    std::uint64_t value = 0;
    std::memcpy(&value, at, sizeof(value));
    return value;
}

inline void WriteHeader(std::uint8_t* base, const Geometry& geometry, std::uint32_t maxFps) {
    std::memset(base, 0, HeaderBytes);
    std::memcpy(base + OffsetMagic, Magic, sizeof(Magic));
    Store32(base + OffsetLayout, LayoutVersion);
    Store32(base + OffsetSlots, SlotCount);
    Store32(base + OffsetWidth, geometry.width);
    Store32(base + OffsetHeight, geometry.height);
    Store32(base + OffsetStride, geometry.stride);
    Store32(base + OffsetFormat, FormatBgra8);
    Store32(base + OffsetMaxFps, maxFps);
    Store32(base + OffsetAudioFrames, geometry.audioFrames);
}

struct Rectangle {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

inline Rectangle Fit(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t width, std::uint32_t height) {
    if (sourceWidth == 0 || sourceHeight == 0 || width == 0 || height == 0) return {};
    const std::uint64_t byWidth = static_cast<std::uint64_t>(width) * sourceHeight;
    const std::uint64_t byHeight = static_cast<std::uint64_t>(height) * sourceWidth;
    std::uint32_t fitWidth = width;
    std::uint32_t fitHeight = height;
    if (byWidth <= byHeight) fitHeight = (std::max<std::uint32_t>)(1, static_cast<std::uint32_t>(byWidth / sourceWidth));
    else fitWidth = (std::max<std::uint32_t>)(1, static_cast<std::uint32_t>(byHeight / sourceHeight));
    return Rectangle{(width - fitWidth) / 2, (height - fitHeight) / 2, fitWidth, fitHeight};
}

// The byte offset in a source row of each destination column of a fitted area. The divide runs
// once per pair of sizes, not once per pixel of every frame.
class ColumnTable {
public:
    void Prepare(std::uint32_t sourceWidth, std::uint32_t areaWidth) {
        if (sourceWidth == this->sourceWidth && areaWidth == this->areaWidth) return;
        this->sourceWidth = sourceWidth;
        this->areaWidth = areaWidth;
        offsets.resize(areaWidth);
        for (std::uint32_t column = 0; column < areaWidth; ++column) {
            offsets[column] = static_cast<std::uint32_t>(static_cast<std::uint64_t>(column) * sourceWidth / areaWidth) * 4;
        }
    }

    const std::uint32_t* Data() const {
        return offsets.data();
    }

private:
    std::uint32_t sourceWidth = 0;
    std::uint32_t areaWidth = 0;
    std::vector<std::uint32_t> offsets;
};

inline void FillBlack(std::uint8_t* line, std::uint32_t pixels) {
    const std::uint32_t black = 0xFF000000u;
    for (std::uint32_t column = 0; column < pixels; ++column) std::memcpy(line + static_cast<std::size_t>(column) * 4, &black, 4);
}

inline void Letterbox(std::uint8_t* destination, std::uint32_t destinationStride, std::uint32_t width, std::uint32_t height, const std::uint8_t* source, std::uint32_t sourceStride, std::uint32_t sourceWidth, std::uint32_t sourceHeight, ColumnTable& columns) {
    if (sourceWidth == width && sourceHeight == height) {
        for (std::uint32_t row = 0; row < height; ++row) {
            std::memcpy(destination + static_cast<std::size_t>(row) * destinationStride, source + static_cast<std::size_t>(row) * sourceStride, static_cast<std::size_t>(width) * 4);
        }
        return;
    }
    const Rectangle area = Fit(sourceWidth, sourceHeight, width, height);
    if (area.width != 0) columns.Prepare(sourceWidth, area.width);
    const std::uint32_t* offsets = columns.Data();
    for (std::uint32_t row = 0; row < height; ++row) {
        std::uint8_t* line = destination + static_cast<std::size_t>(row) * destinationStride;
        const bool inside = row >= area.y && row < area.y + area.height;
        if (!inside || area.width == 0) {
            FillBlack(line, width);
            continue;
        }
        const std::uint32_t sourceRow = static_cast<std::uint32_t>(static_cast<std::uint64_t>(row - area.y) * sourceHeight / area.height);
        const std::uint8_t* from = source + static_cast<std::size_t>(sourceRow) * sourceStride;
        FillBlack(line, area.x);
        std::uint8_t* inner = line + static_cast<std::size_t>(area.x) * 4;
        for (std::uint32_t column = 0; column < area.width; ++column) std::memcpy(inner + static_cast<std::size_t>(column) * 4, from + offsets[column], 4);
        FillBlack(inner + static_cast<std::size_t>(area.width) * 4, width - area.x - area.width);
    }
}

inline void Letterbox(std::uint8_t* destination, std::uint32_t destinationStride, std::uint32_t width, std::uint32_t height, const std::uint8_t* source, std::uint32_t sourceStride, std::uint32_t sourceWidth, std::uint32_t sourceHeight) {
    ColumnTable columns;
    Letterbox(destination, destinationStride, width, height, source, sourceStride, sourceWidth, sourceHeight, columns);
}

class Producer {
public:
    bool Attach(std::uint8_t* memory, std::size_t bytes, std::uint32_t width, std::uint32_t height) {
        base = nullptr;
        if (memory == nullptr || bytes < HeaderBytes) return false;
        if (std::memcmp(memory + OffsetMagic, Magic, sizeof(Magic)) != 0) return false;
        if (Load32(memory + OffsetLayout) != LayoutVersion || Load32(memory + OffsetSlots) != SlotCount) return false;
        if (Load32(memory + OffsetFormat) != FormatBgra8) return false;
        Geometry found;
        found.width = Load32(memory + OffsetWidth);
        found.height = Load32(memory + OffsetHeight);
        found.stride = Load32(memory + OffsetStride);
        found.audioFrames = Load32(memory + OffsetAudioFrames);
        if (found.width != width || found.height != height) return false;
        if (found.width == 0 || found.height == 0 || found.width > MaxDimension || found.height > MaxDimension) return false;
        if (found.stride < found.width * 4 || found.stride > MaxDimension * 4 || !PowerOfTwoOrZero(found.audioFrames)) return false;
        if (found.audioFrames > (1u << 24)) return false;
        if (found.TotalBytes() > bytes) return false;
        geometry = found;
        base = memory;
        next = 1;
        audioPosition = 0;
        return true;
    }

    bool Attached() const {
        return base != nullptr;
    }

    const Geometry& Layout() const {
        return geometry;
    }

    void Detach() {
        base = nullptr;
    }

    bool SubmitFrame(const std::uint8_t* bgra, std::uint32_t width, std::uint32_t height, std::uint32_t stride, std::uint64_t presentUs, std::uint32_t frame) {
        if (base == nullptr || bgra == nullptr || width == 0 || height == 0 || stride < width * 4) return false;
        const std::uint32_t index = static_cast<std::uint32_t>((next - 1) % SlotCount);
        std::uint8_t* slot = base + geometry.SlotOffset(index);
        Store64(slot + SlotOffsetSeq, 0);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        Letterbox(slot + SlotHeaderBytes, geometry.stride, geometry.width, geometry.height, bgra, stride, width, height, columns);
        Store64(slot + SlotOffsetPresentUs, presentUs);
        Store32(slot + SlotOffsetKind, 0);
        Store32(slot + SlotOffsetFrame, frame);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        Store64(slot + SlotOffsetSeq, next);
        ++next;
        return true;
    }

    bool SubmitAudio(const float* stereo, std::uint32_t frames) {
        if (base == nullptr || stereo == nullptr || geometry.audioFrames == 0 || frames == 0) return false;
        if (frames > geometry.audioFrames) {
            stereo += static_cast<std::size_t>(frames - geometry.audioFrames) * 2;
            audioPosition += frames - geometry.audioFrames;
            frames = geometry.audioFrames;
        }
        std::uint8_t* ring = base + geometry.AudioOffset();
        const std::uint32_t mask = geometry.audioFrames - 1;
        const std::uint32_t start = static_cast<std::uint32_t>(audioPosition & mask);
        const std::uint32_t first = (std::min)(frames, geometry.audioFrames - start);
        std::memcpy(ring + static_cast<std::size_t>(start) * AudioFrameBytes, stereo, static_cast<std::size_t>(first) * AudioFrameBytes);
        if (first < frames) std::memcpy(ring, stereo + static_cast<std::size_t>(first) * 2, static_cast<std::size_t>(frames - first) * AudioFrameBytes);
        audioPosition += frames;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        Store64(base + OffsetAudioPosition, audioPosition);
        return true;
    }

    std::uint32_t MaxFps() const {
        return base != nullptr ? Load32(base + OffsetMaxFps) : 0;
    }

    std::uint64_t FramesWritten() const {
        return next - 1;
    }

private:
    std::uint8_t* base = nullptr;
    Geometry geometry;
    std::uint64_t next = 1;
    std::uint64_t audioPosition = 0;
    ColumnTable columns;
};

}

#endif
