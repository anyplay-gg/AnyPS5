#ifndef CORE_LIBS_PRX_LIBSCENETCTL_SRC_NETBRIDGEWIRE_HPP
#define CORE_LIBS_PRX_LIBSCENETCTL_SRC_NETBRIDGEWIRE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace NetBridgeWire {

constexpr std::size_t HeaderSize = 12;
constexpr std::uint32_t MaxPayload = 16u * 1024u * 1024u;
constexpr std::uint16_t ProtocolVersion = 1;
constexpr std::uint32_t MaxHandshakePayload = 1024;
constexpr std::uint16_t ReplyBit = 0x8000;

constexpr std::uint16_t KindHello = 0x0001;
constexpr std::uint16_t KindWelcome = 0x0002;
constexpr std::uint16_t KindReject = 0x0003;
constexpr std::uint16_t KindChallenge = 0x0004;
constexpr std::uint16_t KindAuth = 0x0005;
constexpr std::uint16_t KindUnsupported = 0x8000;
constexpr std::uint16_t KindState = 0x0101;
constexpr std::uint16_t KindFeature = 0x0102;
constexpr std::uint16_t KindWebRequest = 0x0201;
constexpr std::uint16_t KindPushFilter = 0x0202;
constexpr std::uint16_t KindPushFilterGone = 0x0203;
constexpr std::uint16_t KindPushEvent = 0x0204;
constexpr std::uint16_t KindActivate = 0x0301;
constexpr std::uint16_t KindDeactivate = 0x0302;
constexpr std::uint16_t KindPeer = 0x0303;
constexpr std::uint16_t KindPeerInfo = 0x0304;
constexpr std::uint16_t KindBind = 0x0305;
constexpr std::uint16_t KindUnbind = 0x0306;
constexpr std::uint16_t KindShareOpen = 0x0501;
constexpr std::uint16_t KindShareOpened = 0x8501;
constexpr std::uint16_t KindShareClose = 0x0502;
constexpr std::uint16_t KindShareMode = 0x0503;
constexpr std::uint16_t KindSharePad = 0x0504;
constexpr std::uint16_t KindOverride = 0x0505;
constexpr std::uint16_t KindShareStatus = 0x0506;

using Pairs = std::vector<std::pair<std::string, std::string>>;

struct FrameHeader {
    std::uint32_t length;
    std::uint16_t kind;
    std::uint16_t flags;
    std::uint32_t request;
};

inline std::uint16_t Load16(const std::uint8_t* data) {
    return static_cast<std::uint16_t>(data[0] | (data[1] << 8));
}

inline std::uint32_t Load32(const std::uint8_t* data) {
    return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8) | (static_cast<std::uint32_t>(data[2]) << 16) | (static_cast<std::uint32_t>(data[3]) << 24);
}

inline FrameHeader ParseHeader(const std::uint8_t* data) {
    return FrameHeader{Load32(data), Load16(data + 4), Load16(data + 6), Load32(data + 8)};
}

class Reader {
public:
    Reader(const std::uint8_t* data, std::size_t size) : cursor(data), remaining(size) {
    }

    bool Ok() const {
        return good;
    }

    std::size_t Remaining() const {
        return remaining;
    }

    std::uint8_t U8() {
        const auto* bytes = take(1);
        return bytes != nullptr ? bytes[0] : 0;
    }

    std::uint16_t U16() {
        const auto* bytes = take(2);
        return bytes != nullptr ? Load16(bytes) : 0;
    }

    std::uint32_t U32() {
        const auto* bytes = take(4);
        return bytes != nullptr ? Load32(bytes) : 0;
    }

    std::int32_t I32() {
        return static_cast<std::int32_t>(U32());
    }

    std::uint64_t U64() {
        const auto* bytes = take(8);
        if (bytes == nullptr) return 0;
        return static_cast<std::uint64_t>(Load32(bytes)) | (static_cast<std::uint64_t>(Load32(bytes + 4)) << 32);
    }

    void Skip(std::size_t size) {
        take(size);
    }

    std::string Str() {
        const std::size_t length = U16();
        const auto* bytes = take(length);
        return bytes != nullptr ? std::string(reinterpret_cast<const char*>(bytes), length) : std::string();
    }

    std::pair<const std::uint8_t*, std::size_t> Bytes() {
        const std::size_t length = U32();
        const auto* bytes = take(length);
        if (bytes == nullptr) return {nullptr, 0};
        return {bytes, length};
    }

    Pairs ReadPairs() {
        Pairs pairs;
        const std::size_t count = U16();
        for (std::size_t index = 0; index < count && good; ++index) {
            std::string name = Str();
            std::string value = Str();
            if (good) pairs.emplace_back(std::move(name), std::move(value));
        }
        return pairs;
    }

private:
    const std::uint8_t* take(std::size_t size) {
        if (!good || size > remaining) {
            good = false;
            return nullptr;
        }
        const std::uint8_t* start = cursor;
        cursor += size;
        remaining -= size;
        return start;
    }

    const std::uint8_t* cursor;
    std::size_t remaining;
    bool good = true;
};

class Builder {
public:
    bool Ok() const {
        return good;
    }

    std::vector<std::uint8_t> Take() {
        return std::move(data);
    }

    Builder& U8(std::uint8_t value) {
        data.push_back(value);
        return *this;
    }

    Builder& U16(std::uint16_t value) {
        data.push_back(static_cast<std::uint8_t>(value));
        data.push_back(static_cast<std::uint8_t>(value >> 8));
        return *this;
    }

    Builder& U32(std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) data.push_back(static_cast<std::uint8_t>(value >> shift));
        return *this;
    }

    Builder& I32(std::int32_t value) {
        return U32(static_cast<std::uint32_t>(value));
    }

    Builder& Raw(const void* bytes, std::size_t size) {
        if (size == 0) return *this;
        const auto* begin = static_cast<const std::uint8_t*>(bytes);
        data.insert(data.end(), begin, begin + size);
        return *this;
    }

    Builder& Str(std::string_view text) {
        if (text.size() > 0xFFFF) {
            good = false;
            return U16(0);
        }
        U16(static_cast<std::uint16_t>(text.size()));
        return Raw(text.data(), text.size());
    }

    Builder& Bytes(const void* bytes, std::uint64_t size) {
        if (size > MaxPayload) {
            good = false;
            return U32(0);
        }
        U32(static_cast<std::uint32_t>(size));
        return Raw(bytes, static_cast<std::size_t>(size));
    }

    Builder& ListOfStrings(const std::vector<std::string>& items) {
        if (items.size() > 0xFFFF) {
            good = false;
            return U16(0);
        }
        U16(static_cast<std::uint16_t>(items.size()));
        for (const auto& item : items) Str(item);
        return *this;
    }

    Builder& WritePairs(const Pairs& pairs) {
        if (pairs.size() > 0xFFFF) {
            good = false;
            return U16(0);
        }
        U16(static_cast<std::uint16_t>(pairs.size()));
        for (const auto& [name, value] : pairs) {
            Str(name);
            Str(value);
        }
        return *this;
    }

private:
    std::vector<std::uint8_t> data;
    bool good = true;
};

inline std::vector<std::uint8_t> EncodeFrame(std::uint16_t kind, std::uint32_t request, const std::vector<std::uint8_t>& payload) {
    Builder frame;
    frame.U32(static_cast<std::uint32_t>(payload.size())).U16(kind).U16(0).U32(request).Raw(payload.data(), payload.size());
    return frame.Take();
}

}

#endif
