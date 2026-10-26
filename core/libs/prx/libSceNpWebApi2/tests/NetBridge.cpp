#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include "prx/libSceNpWebApi2/include/Api.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

using FakeNet::Require;
using FakeNet::Service;

constexpr int Unavailable = static_cast<int>(0x80553406u);
constexpr int InvalidArgument = static_cast<int>(0x80553402u);

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

}

int main() {
    const FakeNet::Bytes token = {0x10, 0x20, 0x30, 0x40, 0x50};
    Service service(token);
    const std::filesystem::path observePath = std::filesystem::temp_directory_path() / "anyps5-net-bridge-webapi2.jsonl";
    std::error_code removed;
    std::filesystem::remove(observePath, removed);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));
    FakeNet::SetEnvironment("ANYPS5_NET_OBSERVE", observePath.string());
    const std::uint8_t address[4] = {1, 0, 0, 0};
    service.SetState(true, 77, "Web_Tester", 2, address);

    const int library = sceNpWebApi2Initialize(0, 1 << 20);
    const int userContext = sceNpWebApi2CreateUserContext(library, 42);
    std::int64_t request = 0;
    Require(sceNpWebApi2CreateRequest(userContext, "group", "/v1/items?x=1", "POST", nullptr, &request) == 0 && request != 0, "create request");
    Require(sceNpWebApi2AddHttpRequestHeader(request, "Content-Type", "application/json") == 0, "first header");
    Require(sceNpWebApi2AddHttpRequestHeader(request, "X-A", "b") == 0, "second header");
    Require(sceNpWebApi2AddHttpRequestHeader(request, nullptr, "b") == InvalidArgument, "a header needs a name");

    NpWebApi2ResponseInformationOption info;
    std::memset(&info, 0x5A, sizeof(info));
    Require(sceNpWebApi2SendRequest(request, "payload", 7, &info) == 0, "send request");
    const std::string expected = "echo:POST:/v1/items?x=1:payload";
    Require(info.http_status == 200 && info.response_data_size == expected.size() && info.error_object == nullptr && info.error_object_size == 0, "response information");

    std::size_t length = 0;
    Require(sceNpWebApi2GetHttpResponseHeaderValueLength(request, "X-PATH", &length) == 0 && length == std::strlen("/v1/items?x=1"), "header length is case-insensitive");
    char header[64];
    std::memset(header, 0x5A, sizeof(header));
    Require(sceNpWebApi2GetHttpResponseHeaderValue(request, "x-path", header, sizeof(header)) == 0 && std::string(header) == "/v1/items?x=1", "header value");
    Require(sceNpWebApi2GetHttpResponseHeaderValue(request, "x-path", header, 3) == InvalidArgument, "header value does not fit");
    Require(sceNpWebApi2GetHttpResponseHeaderValueLength(request, "x-missing", &length) == Unavailable, "missing header");

    char first[5];
    char rest[64];
    Require(sceNpWebApi2ReadData(request, first, sizeof(first)) == 5 && std::string(first, 5) == expected.substr(0, 5), "first read");
    const int restCount = sceNpWebApi2ReadData(request, rest, sizeof(rest));
    Require(restCount == static_cast<int>(expected.size() - 5) && std::string(rest, static_cast<std::size_t>(restCount)) == expected.substr(5), "second read");
    Require(sceNpWebApi2ReadData(request, rest, sizeof(rest)) == 0, "end of the body");
    Require(sceNpWebApi2ReadData(request, nullptr, 4) == InvalidArgument, "read needs a buffer");

    const auto calls = service.WebCalls();
    Require(calls.size() == 1, "one call reached the service");
    Require(calls[0].userId == 42 && calls[0].apiGroup == "group" && calls[0].path == "/v1/items?x=1" && calls[0].method == "POST", "call fields");
    Require(calls[0].headers.size() == 2 && calls[0].headers[0].first == "Content-Type" && calls[0].headers[1].second == "b", "call headers");
    Require(std::string(calls[0].body.begin(), calls[0].body.end()) == "payload", "call body");

    Require(sceNpWebApi2DeleteRequest(request) == 0, "delete request");
    Require(sceNpWebApi2ReadData(request, rest, sizeof(rest)) == Unavailable, "no data after delete");

    std::int64_t offline = 0;
    Require(sceNpWebApi2CreateRequest(userContext, "group", "/offline", "GET", nullptr, &offline) == 0, "create offline request");
    std::memset(&info, 0x5A, sizeof(info));
    Require(sceNpWebApi2SendRequest(offline, nullptr, 0, &info) == Unavailable, "status 0 gives the offline result");
    Require(info.http_status == 0x5A5A5A5A, "response information untouched");
    Require(sceNpWebApi2ReadData(offline, rest, sizeof(rest)) == Unavailable, "no data without a reply");
    Require(sceNpWebApi2GetHttpResponseHeaderValueLength(offline, "x-path", &length) == Unavailable, "no headers without a reply");

    std::int64_t orphan = 0;
    Require(sceNpWebApi2CreateRequest(987654, "group", "/v1/orphan", "GET", nullptr, &orphan) == 0, "request on an unknown user context");
    Require(sceNpWebApi2SendRequest(orphan, nullptr, 0, nullptr) == Unavailable, "unknown user context is not forwarded");
    Require(sceNpWebApi2SendRequest(123456789, nullptr, 0, nullptr) == Unavailable, "unknown request");
    Require(service.WebCalls().size() == 2, "only the known requests reached the service");

    Require(sceNpWebApi2DeleteUserContext(userContext) == 0, "delete user context");
    std::int64_t afterDelete = 0;
    Require(sceNpWebApi2CreateRequest(userContext, "group", "/v1/gone", "GET", nullptr, &afterDelete) == 0, "request after the context is gone");
    Require(sceNpWebApi2SendRequest(afterDelete, nullptr, 0, nullptr) == Unavailable, "deleted user context is not forwarded");

    const int handle = sceNpWebApi2PushEventCreateHandle(library);
    const int filter = sceNpWebApi2PushEventCreateFilter(library, handle, "np:service", 3, nullptr, 2);
    Require(filter > 0, "push filter");

    const int userContext2 = sceNpWebApi2CreateUserContext(library, 55);
    const int dummyArg = 123;
    const int callbackId = sceNpWebApi2PushEventRegisterCallback(userContext2, filter, nullptr, (void*)&dummyArg);
    Require(callbackId > 0, "push callback registration");

    service.SendPushEvent(55, filter, "game:push", 100, 200, {0x11, 0x22, 0x33});
    Require(Service::WaitUntil([&] {
        sceNpWebApi2CheckTimeout();
        const std::string content = ReadFile(observePath);
        return content.find("\"fn\":\"sceNpWebApi2PushEventDeliver\"") != std::string::npos &&
               content.find("\"data_type\":\"game:push\"") != std::string::npos &&
               content.find("\"data_size\":3") != std::string::npos;
    }, 5000), "push event delivered to observation log");

    Require(sceNpWebApi2PushEventUnregisterCallback(userContext2, callbackId) == 0, "unregister push callback");
    service.SendPushEvent(55, filter, "game:push2", 100, 200, {0x44});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sceNpWebApi2CheckTimeout();
    const std::string afterUnregister = ReadFile(observePath);
    Require(afterUnregister.find("\"data_type\":\"game:push2\"") == std::string::npos, "no push delivery after unregister");

    Require(sceNpWebApi2PushEventDeleteFilter(library, filter) == 0, "push filter delete");
    Require(Service::WaitUntil([&] { return service.PushFilters().size() == 1 && service.PushGone().size() == 1; }, 5000), "push events reach the service");
    Require(service.PushFilters()[0].filter == static_cast<std::uint32_t>(filter) && service.PushFilters()[0].userId == -1 && service.PushFilters()[0].dataTypes.empty(), "push filter event");
    Require(service.PushGone()[0].second == static_cast<std::uint32_t>(filter), "push filter gone event");

    Require(Service::WaitUntil([&] {
        const std::string content = ReadFile(observePath);
        return content.find("\"fn\":\"sceNpWebApi2PushEventDeleteFilter\"") != std::string::npos && content.find("\"fn\":\"sceNpWebApi2PushEventRegisterCallback\"") != std::string::npos;
    }, 5000), "observation lines written");
    const std::string content = ReadFile(observePath);
    Require(content.find("\"fn\":\"sceNpWebApi2SendRequest\",\"in\":{\"request_id\":" + std::to_string(request) + ",\"user_id\":42,\"api_group\":\"group\",\"path\":\"/v1/items?x=1\",\"method\":\"POST\",\"headers\":{\"Content-Type\":\"application/json\",\"X-A\":\"b\"},\"body\":\"payload\"},\"out\":{\"status\":200,\"response_data_size\":31},\"ret\":\"0x0\"") != std::string::npos, "send request line");
    Require(content.find("\"out\":{\"status\":0},\"ret\":\"0x80553406\"") != std::string::npos, "offline send request line");
    Require(content.find("\"fn\":\"sceNpWebApi2CreateRequest\"") != std::string::npos && content.find("\"fn\":\"sceNpWebApi2AddHttpRequestHeader\"") != std::string::npos, "request creation lines");
    Require(content.find("\"fn\":\"sceNpWebApi2PushEventCreateFilter\"") != std::string::npos && content.find("\"filter_param_num\":2") != std::string::npos, "push filter line");

    std::filesystem::remove(observePath, removed);
    std::puts("webapi2 net bridge: ok");
    return 0;
}
