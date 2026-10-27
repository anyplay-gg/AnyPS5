#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "prx/libSceNetCtl/include/NetCtl.hpp"

namespace {

using FakeNet::Require;

void ExpectOff() {
    Require(NetBridgeEnabled_nid_no_patch() == 0, "bridge off");

    NetBridgeStateView view;
    std::memset(&view, 0x5A, sizeof(view));
    Require(NetBridgeState_nid_no_patch(&view) == 0, "no change counter");
    Require(view.signedIn == 0 && view.accountId == 0 && view.handle[0] == '\0' && view.natType == 0 && view.address == 0, "signed out state");
    Require(NetBridgeState_nid_no_patch(nullptr) == 0, "null view");

    NetBridgeWebRequestView request{};
    request.apiGroup = "group";
    request.path = "/p";
    request.method = "GET";
    NetBridgeWebReplyView reply;
    std::memset(&reply, 0x5A, sizeof(reply));
    Require(NetBridgeWebRequest_nid_no_patch(&request, &reply) == 0, "no web reply");
    Require(reply.status == 0 && reply.owner == nullptr && reply.body == nullptr && reply.bodySize == 0, "empty reply");
    NetBridgeWebReplyRelease_nid_no_patch(&reply);
    NetBridgeWebReplyRelease_nid_no_patch(nullptr);

    NetBridgeFeature_nid_no_patch(-1, 1);
    NetBridgePushFilter_nid_no_patch(-1, 1, nullptr, 0);
    NetBridgePushFilterGone_nid_no_patch(-1, 1);
}

}

int main(int argc, char** argv) {
    Require(argc == 2, "mode argument");
    const std::string mode = argv[1];
    const std::string goodToken = "00112233445566778899aabbccddeeff";
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "");
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", "");
    FakeNet::SetEnvironment("ANYPS5_NET_OBSERVE", "");
    if (mode == "none") {
    } else if (mode == "endpoint_only") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:4000");
    } else if (mode == "token_only") {
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", goodToken);
    } else if (mode == "remote_host") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "192.168.1.5:4000");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", goodToken);
    } else if (mode == "other_loopback") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.2:4000");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", goodToken);
    } else if (mode == "hostname") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "localhost:4000");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", goodToken);
    } else if (mode == "port_zero") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:0");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", goodToken);
    } else if (mode == "port_too_large") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:65536");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", goodToken);
    } else if (mode == "port_text") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:40x0");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", goodToken);
    } else if (mode == "uppercase_token") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:4000");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", "00112233445566778899AABBCCDDEEFF");
    } else if (mode == "odd_token") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:4000");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", "abc");
    } else if (mode == "empty_token") {
        FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:4000");
        FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", "");
    } else if (mode == "observe_only") {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / "anyps5-net-bridge-observe-only.jsonl";
        std::error_code removed;
        std::filesystem::remove(path, removed);
        FakeNet::SetEnvironment("ANYPS5_NET_OBSERVE", path.string());
        ExpectOff();
        Require(NetBridgeObserveEnabled_nid_no_patch() == 1, "observation works without a net service");
        NetBridgeRecord("libSceTest", "withoutService", true, 0, [](NetBridgeJson& in, NetBridgeJson& out) {
            in.Int("user_id", 1);
            out.Int("state", 1);
        });
        Require(FakeNet::Service::WaitUntil([&] {
            std::ifstream input(path, std::ios::binary);
            std::ostringstream text;
            text << input.rdbuf();
            const std::string content = text.str();
            return content.find("\"fn\":\"withoutService\"") != std::string::npos && content.find("\"in\":{\"user_id\":1}") != std::string::npos && content.back() == '\n';
        }, 5000), "observation line written");
        {
            NetBridgeCall call("libSceTest", "withCall", [](NetBridgeJson& in) { in.Int("ctx_id", 7); });
            Require(call.Ret(static_cast<int>(0x80550006)) == static_cast<int>(0x80550006), "call passes the return value through");
        }
        {
            NetBridgeCall call("libSceTest", "withPointer");
            int value = 0;
            Require(call.Ret(&value) == &value, "call passes a pointer through");
        }
        bool threw = false;
        try {
            NotImplemented_nid_no_patch("sceTestMissing");
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Require(threw, "not implemented still throws");
        {
            std::ifstream input(path, std::ios::binary);
            std::ostringstream text;
            text << input.rdbuf();
            const std::string content = text.str();
            Require(content.find("\"fn\":\"not_implemented\",\"in\":{\"export\":\"sceTestMissing\"},\"out\":{}}\n") != std::string::npos, "not implemented line is on disk before the throw");
            Require(content.find("\"fn\":\"withCall\",\"in\":{\"ctx_id\":7},\"out\":{},\"ret\":\"0x80550006\"}") != std::string::npos, "queued call line is flushed with it");
            Require(content.find("\"fn\":\"withPointer\",\"in\":{},\"out\":{}}") != std::string::npos, "a pointer return has no ret");
        }
        std::filesystem::remove(path, removed);
        std::puts("net bridge off: ok");
        return 0;
    } else {
        Require(false, "unknown mode");
    }
    ExpectOff();
    Require(NetBridgeObserveEnabled_nid_no_patch() == 0, "observation off");
    NetBridgeRecord("libSceTest", "ignored", true, 0, [](NetBridgeJson&, NetBridgeJson&) { Require(false, "builder must not run while observation is off"); });
    std::puts("net bridge off: ok");
    return 0;
}
