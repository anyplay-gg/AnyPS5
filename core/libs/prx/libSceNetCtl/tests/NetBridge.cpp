#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"

#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "prx/libSceNetCtl/include/NetCtl.hpp"

namespace {

using FakeNet::Bytes;
using FakeNet::Require;
using FakeNet::Service;

class JsonCheck {
public:
    explicit JsonCheck(const std::string& text) : text(text) {
    }

    bool Valid() {
        skip();
        if (!value()) return false;
        skip();
        return position == text.size();
    }

private:
    void skip() {
        while (position < text.size() && (text[position] == ' ' || text[position] == '\t')) ++position;
    }

    bool literal(const char* word) {
        const std::size_t length = std::strlen(word);
        if (text.compare(position, length, word) != 0) return false;
        position += length;
        return true;
    }

    bool string() {
        if (position >= text.size() || text[position] != '"') return false;
        ++position;
        while (position < text.size()) {
            const unsigned char c = static_cast<unsigned char>(text[position++]);
            if (c == '"') return true;
            if (c < 0x20) return false;
            if (c == '\\') {
                if (position >= text.size()) return false;
                const char escape = text[position++];
                if (escape == 'u') {
                    for (int digit = 0; digit < 4; ++digit) {
                        if (position >= text.size() || !std::isxdigit(static_cast<unsigned char>(text[position++]))) return false;
                    }
                } else if (std::strchr("\"\\/bfnrt", escape) == nullptr) {
                    return false;
                }
            }
        }
        return false;
    }

    bool number() {
        const std::size_t start = position;
        if (position < text.size() && text[position] == '-') ++position;
        while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position]))) ++position;
        return position > start;
    }

    bool object() {
        ++position;
        skip();
        if (position < text.size() && text[position] == '}') {
            ++position;
            return true;
        }
        while (true) {
            skip();
            if (!string()) return false;
            skip();
            if (position >= text.size() || text[position++] != ':') return false;
            skip();
            if (!value()) return false;
            skip();
            if (position >= text.size()) return false;
            const char next = text[position++];
            if (next == '}') return true;
            if (next != ',') return false;
        }
    }

    bool value() {
        if (position >= text.size()) return false;
        const char c = text[position];
        if (c == '{') return object();
        if (c == '"') return string();
        if (c == 't') return literal("true");
        if (c == 'f') return literal("false");
        if (c == 'n') return literal("null");
        return number();
    }

    const std::string& text;
    std::size_t position = 0;
};

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

std::vector<std::string> SplitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < text.size()) {
        const std::size_t end = text.find('\n', start);
        if (end == std::string::npos) break;
        lines.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return lines;
}

NetBridgeStateView CurrentState(std::uint64_t* counter = nullptr) {
    NetBridgeStateView view{};
    const std::uint64_t value = NetBridgeState_nid_no_patch(&view);
    if (counter != nullptr) *counter = value;
    return view;
}

struct WebResult {
    int offered = 0;
    NetBridgeWebReplyView reply{};
};

WebResult SendWeb(std::int32_t user, const std::string& path, const std::string& method, const std::string& body) {
    const char* names[] = {"Accept", "X-Test"};
    const char* values[] = {"application/json", "1"};
    NetBridgeWebRequestView request{};
    request.userId = user;
    request.apiGroup = "group";
    request.path = path.c_str();
    request.method = method.c_str();
    request.headerCount = 2;
    request.headerNames = names;
    request.headerValues = values;
    request.body = body.data();
    request.bodySize = body.size();
    WebResult result;
    result.offered = NetBridgeWebRequest_nid_no_patch(&request, &result.reply);
    return result;
}

std::string BodyOf(const NetBridgeWebReplyView& reply) {
    return std::string(reinterpret_cast<const char*>(reply.body), static_cast<std::size_t>(reply.bodySize));
}

std::string HeaderOf(const NetBridgeWebReplyView& reply, const std::string& name) {
    for (std::uint32_t index = 0; index < reply.headerCount; ++index) {
        if (name == reply.headerNames[index]) return reply.headerValues[index];
    }
    return "<missing>";
}

}

int main() {
    const Bytes token = {0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x01, 0xFE};
    Service service(token);
    const std::filesystem::path observePath = std::filesystem::temp_directory_path() / "anyps5-net-bridge-core.jsonl";
    std::error_code removed;
    std::filesystem::remove(observePath, removed);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));
    FakeNet::SetEnvironment("ANYPS5_NET_OBSERVE", observePath.string());

    const std::uint8_t address[4] = {0x01, 0x00, 0x12, 0xC6};
    service.SetState(true, 0x1122334455667788ull, "Alec_PS5", 2, address);

    Require(NetBridgeEnabled_nid_no_patch() == 1, "bridge on");
    std::uint64_t counter = 0;
    NetBridgeStateView view = CurrentState(&counter);
    Require(counter > 0 && view.signedIn == 1, "signed in after the first use");
    Require(view.accountId == 0x1122334455667788ull && std::string(view.handle) == "Alec_PS5" && view.natType == 2, "state fields");
    Require(view.address == 0xC6120001u, "address is the little-endian number");
    Require(service.Welcomed() == 1 && service.Rejected() == 0, "one welcomed connection");

    NetCtlNatInfo nat{};
    nat.size = sizeof(nat);
    Require(sceNetCtlGetNatInfo(&nat) == 0, "nat info answers");
    const std::int32_t stunStatus = nat.stun_status;
    const std::int32_t natType = nat.nat_type;
    Require(stunStatus == 2 && natType == 2, "nat info fields");
    const std::uint8_t expectedMapped[4] = {0xC6, 0x12, 0x00, 0x01};
    Require(std::memcmp(&nat.mapped_addr, expectedMapped, 4) == 0, "mapped address in network order");

    service.HoldReplies(4);
    std::vector<WebResult> results(4);
    std::vector<std::thread> callers;
    for (int index = 0; index < 4; ++index) {
        callers.emplace_back([&results, index] {
            results[index] = SendWeb(7, "/v1/item" + std::to_string(index), "POST", "body-" + std::to_string(index));
        });
    }
    for (auto& caller : callers) caller.join();
    service.HoldReplies(1);
    for (int index = 0; index < 4; ++index) {
        const std::string path = "/v1/item" + std::to_string(index);
        Require(results[index].offered == 1 && results[index].reply.status == 200, "concurrent reply");
        Require(BodyOf(results[index].reply) == "echo:POST:" + path + ":body-" + std::to_string(index), "reply matched to its caller");
        Require(HeaderOf(results[index].reply, "x-path") == path && HeaderOf(results[index].reply, "content-type") == "text/plain", "reply headers");
        NetBridgeWebReplyRelease_nid_no_patch(&results[index].reply);
        Require(results[index].reply.owner == nullptr && results[index].reply.body == nullptr, "reply released");
    }
    const auto calls = service.WebCalls();
    Require(calls.size() == 4, "four web calls arrived");
    for (const auto& call : calls) {
        Require(call.userId == 7 && call.apiGroup == "group" && call.method == "POST" && call.headers.size() == 2, "web call fields");
        Require(call.headers[0].first == "Accept" && call.headers[1].second == "1", "web call headers");
    }

    WebResult offline = SendWeb(7, "/offline", "GET", "");
    Require(offline.offered == 0 && offline.reply.status == 0 && offline.reply.owner == nullptr, "status 0 means not offered");
    WebResult empty = SendWeb(7, "/v1/empty", "GET", "");
    Require(empty.offered == 1 && BodyOf(empty.reply) == "echo:GET:/v1/empty:", "empty body round trip");
    NetBridgeWebReplyRelease_nid_no_patch(&empty.reply);

    NetBridgeFeature_nid_no_patch(-1, 0x1234);
    const char* dataTypes[] = {"a.b", "c"};
    NetBridgePushFilter_nid_no_patch(-1, 77, dataTypes, 2);
    NetBridgePushFilterGone_nid_no_patch(-1, 77);
    Require(Service::WaitUntil([&] { return service.Features().size() == 1 && service.PushFilters().size() == 1 && service.PushGone().size() == 1; }, 5000), "events arrive");
    Require(service.Features()[0].first == -1 && service.Features()[0].second == 0x1234, "feature event");
    Require(service.PushFilters()[0].filter == 77 && service.PushFilters()[0].dataTypes == std::vector<std::string>({"a.b", "c"}), "push filter event");
    Require(service.PushGone()[0].second == 77, "push filter gone event");

    struct PushRecord {
        std::int32_t userId = 0;
        std::uint32_t filter = 0;
        std::string dataType;
        std::uint64_t from = 0;
        std::uint64_t to = 0;
        std::vector<std::uint8_t> data;
        std::vector<std::pair<std::string, std::string>> extended;
    };
    std::vector<PushRecord> pushRecords;
    const int handlerId = NetBridgeRegisterPushHandler_nid_no_patch([](const NetBridgePushEventView* event, void* userArg) {
        auto* records = static_cast<std::vector<PushRecord>*>(userArg);
        PushRecord record;
        record.userId = event->userId;
        record.filter = event->filter;
        record.dataType = event->dataType != nullptr ? event->dataType : "";
        record.from = event->from;
        record.to = event->to;
        if (event->data != nullptr && event->dataSize > 0) {
            const auto* bytes = static_cast<const std::uint8_t*>(event->data);
            record.data.assign(bytes, bytes + event->dataSize);
        }
        for (std::uint32_t i = 0; i < event->extendedCount; ++i) {
            record.extended.emplace_back(
                event->extendedNames[i] != nullptr ? event->extendedNames[i] : "",
                event->extendedValues[i] != nullptr ? event->extendedValues[i] : ""
            );
        }
        records->push_back(std::move(record));
    }, &pushRecords);
    Require(handlerId > 0, "push handler registered");

    const FakeNet::Pairs extendedPairs = {{"key1", "val1"}, {"key2", "val2"}};
    service.SendPushEvent(12, 77, "custom.type", 1001, 2002, {0xAA, 0xBB, 0xCC}, extendedPairs);
    Require(Service::WaitUntil([&] { return pushRecords.size() == 1; }, 5000), "push event received");
    Require(pushRecords[0].userId == 12 && pushRecords[0].filter == 77 && pushRecords[0].dataType == "custom.type", "push event fields");
    Require(pushRecords[0].from == 1001 && pushRecords[0].to == 2002, "push event sender/receiver");
    Require(pushRecords[0].data == std::vector<std::uint8_t>({0xAA, 0xBB, 0xCC}), "push event payload");
    Require(pushRecords[0].extended == extendedPairs, "push event extended pairs");

    NetBridgeUnregisterPushHandler_nid_no_patch(handlerId);
    service.SendPushEvent(12, 77, "custom.type2", 1001, 2002, {0xDD});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Require(pushRecords.size() == 1, "push handler unregistered stops receiving events");

    std::uint64_t junkBefore = 0;
    CurrentState(&junkBefore);
    service.SendRaw(FakeNet::MakeFrame(0x0101, 0, Bytes{1, 2, 3}));
    Bytes longHandle;
    longHandle.push_back(1);
    FakeNet::Put64(longHandle, 5);
    FakeNet::PutText(longHandle, "ABCDEFGHIJKLMNOPQ");
    longHandle.push_back(1);
    FakeNet::Put32(longHandle, 0);
    service.SendRaw(FakeNet::MakeFrame(0x0101, 0, longHandle));
    service.SendRaw(FakeNet::MakeFrame(0x7777, 0, Bytes{9, 9}));
    service.SendRaw(FakeNet::MakeFrame(0x7001, 5, Bytes{}));
    const std::uint8_t second[4] = {0x02, 0x00, 0x12, 0xC6};
    service.SetState(true, 99, "Other", 3, second);
    Require(Service::WaitUntil([&] { return std::string(CurrentState().handle) == "Other"; }, 5000), "valid state after junk frames");
    Require(CurrentState().accountId == 99 && CurrentState().natType == 3, "state replaced");
    Require(Service::WaitUntil([&] { return service.Unsupported() == 1; }, 5000), "unknown request is answered with unsupported");

    service.SetState(false, 99, "Ghost", 1, second);
    Require(Service::WaitUntil([&] { return CurrentState().signedIn == 0; }, 5000), "signed out state");
    view = CurrentState();
    Require(view.accountId == 0 && view.handle[0] == '\0', "signed out clears account and handle");
    service.SetState(true, 0x1122334455667788ull, "Alec_PS5", 2, address);
    Require(Service::WaitUntil([&] { return CurrentState().signedIn == 1; }, 5000), "signed in again");

    std::uint64_t beforeDrop = 0;
    CurrentState(&beforeDrop);
    service.DropConnection();
    Require(Service::WaitUntil([&] { return CurrentState().signedIn == 0; }, 5000), "signed out after a dropped connection");
    std::uint64_t duringOutage = 0;
    CurrentState(&duringOutage);
    Require(duringOutage > beforeDrop, "the change counter moves on disconnect");
    const auto outageStart = std::chrono::steady_clock::now();
    WebResult during = SendWeb(7, "/v1/during", "GET", "");
    Require(during.offered == 0, "no web request while disconnected");
    Require(std::chrono::steady_clock::now() - outageStart < std::chrono::seconds(2), "a disconnected web request does not wait");
    Require(Service::WaitUntil([&] { return CurrentState().signedIn == 1; }, 20000), "signed in again after the reconnect");
    Require(service.Welcomed() == 2, "second connection welcomed");
    Require(std::string(CurrentState().handle) == "Alec_PS5", "state restored after the reconnect");
    WebResult after = SendWeb(7, "/v1/after", "GET", "");
    Require(after.offered == 1 && after.reply.status == 200, "web request after the reconnect");
    NetBridgeWebReplyRelease_nid_no_patch(&after.reply);

    Require(NetBridgeObserveEnabled_nid_no_patch() == 1, "observation on");
    const std::string big(70000, 'x');
    const unsigned char raw[] = {0xFF, 0x00};
    NetBridgeRecord("libSceTest", "testFn", true, static_cast<std::int32_t>(0x80550006u), [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Text("quote", "a\"b\\c\n\x01");
        in.Bytes("raw", raw, sizeof(raw));
        in.Text("utf8", "caf\xC3\xA9");
        in.Begin("headers").Text("X-A", "v").End();
        in.Bytes("body", big.data(), big.size());
        out.Uint("n", 5).Bool("ok", true).Null("nothing");
    });
    NetBridgeRecord("libSceTest", "noReturn", false, 0, [&](NetBridgeJson&, NetBridgeJson&) {});
    constexpr int Burst = 20000;
    for (int index = 0; index < Burst; ++index) {
        NetBridgeRecord("libSceTest", "burst", true, 0, [&](NetBridgeJson& in, NetBridgeJson&) { in.Int("i", index); });
    }
    std::vector<std::string> lines;
    long long burstLines = 0;
    long long droppedTotal = 0;
    Require(Service::WaitUntil([&] {
        lines = SplitLines(ReadFile(observePath));
        burstLines = 0;
        droppedTotal = 0;
        for (const auto& line : lines) {
            if (line.find("\"fn\":\"burst\"") != std::string::npos) ++burstLines;
            static const std::string marker = "\"fn\":\"dropped\",\"dropped\":";
            const auto dropped = line.find(marker);
            if (dropped != std::string::npos) droppedTotal += std::atoll(line.c_str() + dropped + marker.size());
        }
        return burstLines + droppedTotal == Burst;
    }, 15000), "every observation is written or counted as dropped");

    bool sawTest = false;
    bool sawNat = false;
    bool sawNoReturn = false;
    for (const auto& line : lines) {
        JsonCheck check(line);
        Require(check.Valid(), "every observation line is valid JSON");
        Require(line.rfind("{\"v\":1,\"t_ns\":", 0) == 0 && line.find("\"src\":\"bridge\"") != std::string::npos, "line envelope");
        if (line.find("\"fn\":\"testFn\"") != std::string::npos) {
            sawTest = true;
            Require(line.find("\"quote\":\"a\\\"b\\\\c\\u000a\\u0001\"") != std::string::npos, "escaping");
            Require(line.find("\"raw\":{\"b64\":\"/wA=\"}") != std::string::npos, "non-UTF-8 as base64");
            Require(line.find("\"utf8\":\"caf\xC3\xA9\"") != std::string::npos, "UTF-8 kept");
            Require(line.find("\"headers\":{\"X-A\":\"v\"}") != std::string::npos, "nested object");
            Require(line.find("\"body\":\"" + std::string(65536, 'x') + "\"") != std::string::npos, "body cut at 64 KiB");
            Require(line.find("\"cut\":70000") != std::string::npos, "cut length");
            Require(line.find("\"out\":{\"n\":5,\"ok\":true,\"nothing\":null}") != std::string::npos, "out fields");
            Require(line.find("\"ret\":\"0x80550006\"") != std::string::npos, "ret in hex");
            Require(line.find("\"lib\":\"libSceTest\"") != std::string::npos, "lib");
        }
        if (line.find("\"fn\":\"noReturn\"") != std::string::npos) {
            sawNoReturn = true;
            Require(line.find("\"ret\"") == std::string::npos && line.find("\"in\":{},\"out\":{}") != std::string::npos, "no ret for exports without one");
        }
        if (line.find("\"fn\":\"sceNetCtlGetNatInfo\"") != std::string::npos) {
            sawNat = true;
            Require(line.find("\"nat_type\":2") != std::string::npos && line.find("\"ret\":\"0x0\"") != std::string::npos, "nat observation");
        }
    }
    Require(sawTest && sawNat && sawNoReturn, "expected observation lines present");
    Require(service.Malformed() == 0, "the bridge sent only well-formed frames");

    std::filesystem::remove(observePath, removed);
    std::puts("net bridge: ok");
    return 0;
}
