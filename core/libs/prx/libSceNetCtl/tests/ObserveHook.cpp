
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libc/include/CallTrace.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

extern "C" int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V111UserFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_4UserEEE();

namespace {

void Require(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "observe hook check failed: %s\n", what);
        std::exit(1);
    }
}

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

bool WaitFor(const std::filesystem::path& path, const std::string& needle) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (ReadFile(path).find(needle) != std::string::npos) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

std::size_t Occurrences(const std::string& text, const std::string& needle) {
    std::size_t count = 0;
    for (std::size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) ++count;
    return count;
}

int APS5_VABI FakeListen(int s, int backlog) {
    return s + backlog == 21 ? static_cast<int>(0x80410116) : 0;
}

int APS5_VABI FakeConnection(int templateId, const char* url, int keepAlive) {
    return templateId + keepAlive + static_cast<int>(std::strlen(url));
}

int APS5_VABI FakeDescribing(int user) {
    NetBridgeCall call("libSceNet", "sceNetSetsockopt", [&](NetBridgeJson& in) { in.Int("custom", user); });
    return call.Ret(7);
}

int APS5_VABI FakeUnknown(int value) {
    return value;
}

int APS5_VABI FakeElsewhere(int value) {
    return value;
}

}

int main() {
    const auto path = std::filesystem::temp_directory_path() / "anyps5-observe-hook.jsonl";
    std::error_code removed;
    std::filesystem::remove(path, removed);
#ifdef _WIN32
    _putenv_s("ANYPS5_NET_OBSERVE", path.string().c_str());
#else
    setenv("ANYPS5_NET_OBSERVE", path.string().c_str(), 1);
#endif
    Require(NetBridgeObserveEnabled_nid_no_patch() == 1, "observation is on");

    static constexpr struct {
        const char* library;
        std::size_t floor;
    } Floors[] = {
        {"libSceNet", 223}, {"libSceNetCtl", 84}, {"libSceNpAuth", 14}, {"libSceNpCommerce", 12}, {"libSceNpCppWebApi", 751},
        {"libSceNpEntitlementAccess", 25}, {"libSceNpGameIntent", 18}, {"libSceNpManager", 207}, {"libSceNpMatching2", 69},
        {"libSceNpSessionSignaling", 26}, {"libSceNpSignaling", 9}, {"libSceNpUniversalDataSystem", 84}, {"libSceNpWebApi", 23},
        {"libSceNpWebApi2", 14}, {"libSceHttp", 115}, {"libSceHttp2", 62}, {"libSceSsl", 55},
    };
    for (const auto& floor : Floors) {
        const std::size_t count = NetBridgeObservedExports_nid_no_patch(floor.library);
        std::printf("%s: %zu exports observed\n", floor.library, count);
        Require(count >= floor.floor, floor.library);
    }
    Require(NetBridgeObservedExports_nid_no_patch("libSceNotObserved") == 0, "a library that is not observed has no exports");
    Require(NetBridgeObservedExport_nid_no_patch("libSceNet", "sceNetSocket") == 1, "sceNetSocket is observed");
    Require(NetBridgeObservedExport_nid_no_patch("libSceNpManager", "sceNpGetState") == 1, "sceNpGetState is observed");
    Require(NetBridgeObservedExport_nid_no_patch("libSceNpCppWebApi.prx", "_ZN3sce2Np9CppWebApi12Leaderboards2V111UserFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_4UserEEE") == 1, "a C++ export is observed");
    Require(NetBridgeObservedExport_nid_no_patch("libSceNet", "sceNetHtonl_nid_postfix") == 1, "an export with a NID postfix is observed under its source name");

    using Create = int(APS5_VABI*)();
    const auto create = reinterpret_cast<Create>(NetBridgeObserveImport_nid_no_patch(
        reinterpret_cast<void*>(&_ZN3sce2Np9CppWebApi12Leaderboards2V111UserFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_4UserEEE),
        "libSceNpCppWebApi", "_ZN3sce2Np9CppWebApi12Leaderboards2V111UserFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_4UserEEE"));
    Require(create != &_ZN3sce2Np9CppWebApi12Leaderboards2V111UserFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_4UserEEE, "the import is routed through a thunk");
    Require(create() == static_cast<int>(0x80552907), "the thunk passes the result through");
    Require(WaitFor(path, "\"lib\":\"libSceNpCppWebApi\",\"fn\":\"_ZN3sce2Np9CppWebApi12Leaderboards2V111UserFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_4UserEEE\",\"in\":{},\"out\":{},\"ret\":\"0x80552907\"}"), "the hook wrote the line of an export without observation code");

    using Listen = int(APS5_VABI*)(int, int);
    const auto listen = reinterpret_cast<Listen>(NetBridgeObserveImport_nid_no_patch(reinterpret_cast<void*>(&FakeListen), "libSceNet", "sceNetListen"));
    Require(listen(5, 16) == static_cast<int>(0x80410116) && listen(1, 1) == 0, "listen passes the result through");
    using Connection = int(APS5_VABI*)(int, const char*, int);
    const auto connection = reinterpret_cast<Connection>(NetBridgeObserveImport_nid_no_patch(reinterpret_cast<void*>(&FakeConnection), "libSceHttp", "sceHttpCreateConnectionWithURL"));
    Require(connection(3, "https://example.test/", 1) == 3 + 1 + 21, "the string argument reaches the export intact");
    Require(WaitFor(path, "\"fn\":\"sceNetListen\",\"in\":{\"s\":5,\"backlog\":16},\"out\":{},\"ret\":\"0x80410116\"}"), "integer arguments and the result code");
    Require(WaitFor(path, "\"fn\":\"sceNetListen\",\"in\":{\"s\":1,\"backlog\":1},\"out\":{},\"ret\":\"0x0\"}"), "a second call is a second line");
    Require(WaitFor(path, "\"lib\":\"libSceHttp\",\"fn\":\"sceHttpCreateConnectionWithURL\",\"in\":{\"tmpl_id\":3,\"url\":\"https://example.test/\",\"enable_keep_alive\":1},\"out\":{},\"ret\":\"0x19\"}"), "a string argument");

    using Describing = int(APS5_VABI*)(int);
    const auto describing = reinterpret_cast<Describing>(NetBridgeObserveImport_nid_no_patch(reinterpret_cast<void*>(&FakeDescribing), "libSceNet", "sceNetSetsockopt"));
    Require(describing(9) == 7, "a describing export passes the result through");
    Require(WaitFor(path, "\"fn\":\"sceNetSetsockopt\",\"in\":{\"custom\":9},\"out\":{},\"ret\":\"0x7\"}"), "the explicit line is written");

    using Unknown = int(APS5_VABI*)(int);
    const auto unknown = reinterpret_cast<Unknown>(TraceImport_nid_no_patch(reinterpret_cast<void*>(&FakeUnknown), "ZZZZZZZZZZZ", "libSceNet.prx"));
    Require(unknown != &FakeUnknown && unknown(4) == 4, "an unknown NID of an observed library is hooked");
    Require(WaitFor(path, "\"lib\":\"libSceNet\",\"fn\":\"ZZZZZZZZZZZ\",\"in\":{},\"out\":{},\"ret\":\"0x4\"}"), "the unknown export is written under its NID");

    Require(TraceImport_nid_no_patch(reinterpret_cast<void*>(&FakeElsewhere), "ZZZZZZZZZZY", "libSceElsewhere.prx") == reinterpret_cast<void*>(&FakeElsewhere), "other libraries are not hooked");

    const std::string content = ReadFile(path);
    Require(Occurrences(content, "\"fn\":\"sceNetSetsockopt\"") == 1, "a describing export is written once");
    Require(content.find("\"src\":\"bridge\"") != std::string::npos && content.find("{\"v\":1,\"t_ns\":") == 0, "the envelope of the lines");
    std::filesystem::remove(path, removed);
    std::puts("observe hook: ok");
    return 0;
}
