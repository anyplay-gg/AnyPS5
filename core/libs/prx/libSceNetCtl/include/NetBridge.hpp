#ifndef CORE_LIBS_PRX_LIBSCENETCTL_INCLUDE_NETBRIDGE_HPP
#define CORE_LIBS_PRX_LIBSCENETCTL_INCLUDE_NETBRIDGE_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

struct NetBridgeStateView {
    std::int32_t signedIn;
    std::uint32_t address;
    std::uint64_t accountId;
    char handle[17];
    std::uint8_t natType;
    std::uint8_t reserved[6];
};

struct NetBridgeWebRequestView {
    std::int32_t userId;
    const char* apiGroup;
    const char* path;
    const char* method;
    std::uint32_t headerCount;
    const char* const* headerNames;
    const char* const* headerValues;
    const void* body;
    std::uint64_t bodySize;
};

struct NetBridgeWebReplyView {
    void* owner;
    std::uint16_t status;
    std::uint32_t headerCount;
    const char* const* headerNames;
    const char* const* headerValues;
    const std::uint8_t* body;
    std::uint64_t bodySize;
};

struct NetBridgeTrafficView {
    std::uint16_t udpPort;
    std::uint16_t tcpPort;
    std::uint8_t streamKey[16];
};

struct NetBridgePeerView {
    std::uint64_t accountId;
    char handle[17];
    std::uint8_t peerState;
    std::uint8_t reserved[2];
    std::uint32_t address;
};

struct NetBridgePeerInfoView {
    std::uint8_t known;
    std::uint8_t relayed;
    std::uint8_t reserved[2];
    std::uint32_t rttUs;
};

struct NetBridgePushEventView {
    std::int32_t userId;
    std::uint32_t filter;
    const char* dataType;
    std::uint64_t from;
    std::uint64_t to;
    const void* data;
    std::uint64_t dataSize;
    std::uint32_t extendedCount;
    const char* const* extendedNames;
    const char* const* extendedValues;
};

using NetBridgePushHandler = void (*)(const NetBridgePushEventView* event, void* userArg);
using NetBridgePushPump = void (*)(void* userArg);

struct NetBridgeObservation {
    const char* lib;
    const char* fn;
    const char* inJson;
    const char* outJson;
    std::int32_t ret;
    std::int32_t hasRet;
    std::uint64_t cut;
};

extern "C" {

int NetBridgeEnabled_nid_no_patch(void);
std::uint64_t NetBridgeState_nid_no_patch(NetBridgeStateView* out);
int NetBridgeWebRequest_nid_no_patch(const NetBridgeWebRequestView* in, NetBridgeWebReplyView* out);
void NetBridgeWebReplyRelease_nid_no_patch(NetBridgeWebReplyView* reply);
void NetBridgeFeature_nid_no_patch(std::int32_t userId, std::uint32_t feature);
int NetBridgeTraffic_nid_no_patch(NetBridgeTrafficView* out);
int NetBridgeActivate_nid_no_patch(std::int32_t userId, const char* session, std::uint8_t sessionKind);
void NetBridgeDeactivate_nid_no_patch(std::int32_t userId, const char* session);
std::uint32_t NetBridgePeers_nid_no_patch(const char* session, NetBridgePeerView* out, std::uint32_t capacity, std::uint64_t* counter);
int NetBridgePeerInfo_nid_no_patch(std::uint32_t address, NetBridgePeerInfoView* out);
int NetBridgeBind_nid_no_patch(std::uint16_t vport, std::uint8_t transport, std::uint16_t loopbackPort);
void NetBridgeUnbind_nid_no_patch(std::uint16_t vport, std::uint8_t transport);
void NetBridgePushFilter_nid_no_patch(std::int32_t userId, std::uint32_t filter, const char* const* dataTypes, std::uint32_t count);
void NetBridgePushFilterGone_nid_no_patch(std::int32_t userId, std::uint32_t filter);
int NetBridgeRegisterPushHandler_nid_no_patch(NetBridgePushHandler handler, void* userArg);
void NetBridgeUnregisterPushHandler_nid_no_patch(int registrationId);
int NetBridgeRegisterPushPump_nid_no_patch(NetBridgePushPump pump, void* userArg);
void NetBridgeUnregisterPushPump_nid_no_patch(int registrationId);
void NetBridgeDeliverPushEvents_nid_no_patch(void);
int NetBridgeObserveEnabled_nid_no_patch(void);
void NetBridgeObserve_nid_no_patch(const NetBridgeObservation* observation);
void NetBridgeObserveNow_nid_no_patch(const NetBridgeObservation* observation);
int NetBridgeObserveClaim_nid_no_patch(const char* library, const char* function);
void* NetBridgeObserveImport_nid_no_patch(void* function, const char* library, const char* name);
std::size_t NetBridgeObservedExports_nid_no_patch(const char* library);
int NetBridgeObservedExport_nid_no_patch(const char* library, const char* name);

}

namespace NetBridgeText {

inline bool ValidUtf8(const unsigned char* data, std::size_t size) {
    std::size_t index = 0;
    while (index < size) {
        const unsigned char lead = data[index];
        std::size_t extra = 0;
        std::uint32_t minimum = 0;
        std::uint32_t point = 0;
        if (lead < 0x80) {
            ++index;
            continue;
        }
        if (lead >= 0xC2 && lead <= 0xDF) {
            extra = 1;
            minimum = 0x80;
            point = lead & 0x1Fu;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            extra = 2;
            minimum = 0x800;
            point = lead & 0x0Fu;
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            extra = 3;
            minimum = 0x10000;
            point = lead & 0x07u;
        } else {
            return false;
        }
        if (size - index <= extra) return false;
        for (std::size_t offset = 1; offset <= extra; ++offset) {
            const unsigned char next = data[index + offset];
            if ((next & 0xC0u) != 0x80u) return false;
            point = (point << 6) | (next & 0x3Fu);
        }
        if (point < minimum || point > 0x10FFFFu || (point >= 0xD800u && point <= 0xDFFFu)) return false;
        index += extra + 1;
    }
    return true;
}

inline void AppendEscaped(std::string& out, const unsigned char* data, std::size_t size) {
    static constexpr char Hex[] = "0123456789abcdef";
    for (std::size_t index = 0; index < size; ++index) {
        const unsigned char value = data[index];
        if (value == '"' || value == '\\') {
            out.push_back('\\');
            out.push_back(static_cast<char>(value));
        } else if (value < 0x20 || value == 0x7F) {
            out += "\\u00";
            out.push_back(Hex[value >> 4]);
            out.push_back(Hex[value & 0x0F]);
        } else {
            out.push_back(static_cast<char>(value));
        }
    }
}

inline void AppendBase64(std::string& out, const unsigned char* data, std::size_t size) {
    static constexpr char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::size_t index = 0;
    for (; index + 2 < size; index += 3) {
        const std::uint32_t block = (static_cast<std::uint32_t>(data[index]) << 16) | (static_cast<std::uint32_t>(data[index + 1]) << 8) | data[index + 2];
        out.push_back(Alphabet[(block >> 18) & 0x3F]);
        out.push_back(Alphabet[(block >> 12) & 0x3F]);
        out.push_back(Alphabet[(block >> 6) & 0x3F]);
        out.push_back(Alphabet[block & 0x3F]);
    }
    if (size - index == 1) {
        const std::uint32_t block = static_cast<std::uint32_t>(data[index]) << 16;
        out.push_back(Alphabet[(block >> 18) & 0x3F]);
        out.push_back(Alphabet[(block >> 12) & 0x3F]);
        out += "==";
    } else if (size - index == 2) {
        const std::uint32_t block = (static_cast<std::uint32_t>(data[index]) << 16) | (static_cast<std::uint32_t>(data[index + 1]) << 8);
        out.push_back(Alphabet[(block >> 18) & 0x3F]);
        out.push_back(Alphabet[(block >> 12) & 0x3F]);
        out.push_back(Alphabet[(block >> 6) & 0x3F]);
        out.push_back('=');
    }
}

}

class NetBridgeJson {
public:
    static constexpr std::size_t ValueLimit = 65536;

    NetBridgeJson() : commaNeeded(1, false) {
    }

    NetBridgeJson& Int(std::string_view name, std::int64_t value) {
        key(name);
        char text[32];
        std::snprintf(text, sizeof(text), "%lld", static_cast<long long>(value));
        body += text;
        return *this;
    }

    NetBridgeJson& Uint(std::string_view name, std::uint64_t value) {
        key(name);
        char text[32];
        std::snprintf(text, sizeof(text), "%llu", static_cast<unsigned long long>(value));
        body += text;
        return *this;
    }

    NetBridgeJson& Bool(std::string_view name, bool value) {
        key(name);
        body += value ? "true" : "false";
        return *this;
    }

    NetBridgeJson& Null(std::string_view name) {
        key(name);
        body += "null";
        return *this;
    }

    NetBridgeJson& Text(std::string_view name, const char* value) {
        if (value == nullptr) return Null(name);
        std::size_t length = 0;
        while (length < ValueLimit && value[length] != '\0') ++length;
        const bool truncated = length == ValueLimit && value[length] != '\0';
        if (truncated) cut = std::max<std::uint64_t>(cut, ValueLimit + 1);
        return Bytes(name, value, length);
    }

    NetBridgeJson& Text(std::string_view name, const std::string& value) {
        return Bytes(name, value.data(), value.size());
    }

    NetBridgeJson& Bytes(std::string_view name, const void* data, std::size_t size) {
        key(name);
        value(static_cast<const unsigned char*>(data), size);
        return *this;
    }

    NetBridgeJson& Begin(std::string_view name) {
        key(name);
        body.push_back('{');
        commaNeeded.push_back(false);
        return *this;
    }

    NetBridgeJson& End() {
        if (commaNeeded.size() > 1) {
            commaNeeded.pop_back();
            body.push_back('}');
        }
        return *this;
    }

    std::string Finish() {
        while (commaNeeded.size() > 1) End();
        return "{" + body + "}";
    }

    std::uint64_t Cut() const {
        return cut;
    }

private:
    void key(std::string_view name) {
        if (commaNeeded.back()) body.push_back(',');
        commaNeeded.back() = true;
        body.push_back('"');
        const auto* data = reinterpret_cast<const unsigned char*>(name.data());
        if (NetBridgeText::ValidUtf8(data, name.size())) {
            NetBridgeText::AppendEscaped(body, data, name.size());
        } else {
            body += "b64:";
            NetBridgeText::AppendBase64(body, data, name.size());
        }
        body += "\":";
    }

    void value(const unsigned char* data, std::size_t size) {
        std::size_t used = size;
        if (size > ValueLimit) {
            used = ValueLimit;
            while (used > ValueLimit - 4 && (data[used] & 0xC0u) == 0x80u) --used;
            cut = std::max<std::uint64_t>(cut, size);
        }
        if (NetBridgeText::ValidUtf8(data, used)) {
            body.push_back('"');
            NetBridgeText::AppendEscaped(body, data, used);
            body.push_back('"');
        } else {
            body += "{\"b64\":\"";
            NetBridgeText::AppendBase64(body, data, used);
            body += "\"}";
        }
    }

    std::string body;
    std::vector<bool> commaNeeded;
    std::uint64_t cut = 0;
};

template <typename TBuild>
inline void NetBridgeRecord(const char* lib, const char* fn, bool hasRet, std::int32_t ret, TBuild&& build) {
    if (NetBridgeObserveEnabled_nid_no_patch() == 0) return;
    NetBridgeObserveClaim_nid_no_patch(lib, fn);
    NetBridgeJson in;
    NetBridgeJson out;
    build(in, out);
    const std::string inText = in.Finish();
    const std::string outText = out.Finish();
    const NetBridgeObservation observation{lib, fn, inText.c_str(), outText.c_str(), ret, hasRet ? 1 : 0, std::max(in.Cut(), out.Cut())};
    NetBridgeObserve_nid_no_patch(&observation);
}

class NetBridgeCall {
public:
    NetBridgeCall(const char* lib, const char* fn) : lib(lib), fn(fn), on(NetBridgeObserveEnabled_nid_no_patch() != 0) {
        if (on) NetBridgeObserveClaim_nid_no_patch(lib, fn);
    }

    template <typename TBuild>
    NetBridgeCall(const char* lib, const char* fn, TBuild&& build) : NetBridgeCall(lib, fn) {
        if (on) build(in);
    }

    NetBridgeCall(const NetBridgeCall&) = delete;
    NetBridgeCall& operator=(const NetBridgeCall&) = delete;

    bool On() const {
        return on;
    }

    NetBridgeJson& Out() {
        return out;
    }

    template <typename T>
    T Ret(T value) {
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool> && sizeof(T) <= sizeof(std::int32_t)) {
            ret = static_cast<std::int32_t>(value);
            hasRet = true;
        } else if constexpr (std::is_same_v<T, bool>) {
            if (on) out.Bool("value", value);
        }
        return value;
    }

    ~NetBridgeCall() {
        if (!on) return;
        try {
            const std::string inText = in.Finish();
            const std::string outText = out.Finish();
            const NetBridgeObservation observation{lib, fn, inText.c_str(), outText.c_str(), ret, hasRet ? 1 : 0, std::max(in.Cut(), out.Cut())};
            NetBridgeObserve_nid_no_patch(&observation);
        } catch (...) {
        }
    }

private:
    const char* lib;
    const char* fn;
    bool on;
    bool hasRet = false;
    std::int32_t ret = 0;
    NetBridgeJson in;
    NetBridgeJson out;
};

#endif
