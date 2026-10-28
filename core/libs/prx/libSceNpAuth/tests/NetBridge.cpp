#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include "prx/libSceNpAuth/include/Api.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace {

using FakeNet::Require;
using FakeNet::Service;

constexpr int SignedOut = static_cast<int>(0x80550006u);
constexpr int CodeUnavailable = static_cast<int>(0x80550304u);

bool SignedInNow() {
    NetBridgeStateView view{};
    NetBridgeState_nid_no_patch(&view);
    return view.signedIn != 0;
}

void RequireAnswers(int expected) {
    Require(expected != 0, "never success");
    std::array<char, 136> client{};
    std::array<char, 256> param{};
    std::array<char, 4096> output{};
    output.fill('x');
    const auto original = output;
    bool authorizationRequired = true;
    std::int32_t issuerId = 7;
    NpAuthAuthorizedAppCodeParameter appCode{40, 1, reinterpret_cast<const NpAuthClientId*>(client.data()), "", 0};

    const auto check = [&](const std::function<int(int)>& call, const char* message) {
        const int request = sceNpAuthCreateRequest();
        Require(request > 0, "create request");
        Require(call(request) == expected, message);
        Require(output == original, "nothing written");
        Require(sceNpAuthDeleteRequest(request) == 0, "delete request");
    };
    char* const p = param.data();
    char* const o = output.data();
    check([&](int r) { return sceNpAuthGetAuthorizedAppCode(r, &appCode, o, &issuerId, &authorizationRequired); }, "authorized app code answer");
    check([&](int r) { return sceNpAuthGetAuthorizationCodeV3(r, p, o, &issuerId); }, "authorization code V3 answer");
    check([&](int r) { return sceNpAuthGetAuthorizationCodeA(r, p, o, &issuerId); }, "authorization code A answer");
    check([&](int r) { return sceNpAuthGetAuthorizationCode(r, p, o); }, "authorization code answer");
    check([&](int r) { return sceNpAuthGetIdTokenV3(r, p, o); }, "ID token V3 answer");
    check([&](int r) { return sceNpAuthGetIdTokenA(r, p, o); }, "ID token A answer");
    check([&](int r) { return sceNpAuthGetIdToken(r, p, o); }, "ID token answer");
    Require(authorizationRequired && issuerId == 7, "flags untouched");

    const int async = sceNpAuthCreateAsyncRequest(nullptr);
    Require(async > 0, "create async request");
    Require(sceNpAuthGetAuthorizedAppCode(async, &appCode, output.data(), &issuerId, &authorizationRequired) == 0, "async app code starts");
    int result = 0;
    Require(sceNpAuthPollAsync(async, &result) == 1, "async app code pending once");
    Require(sceNpAuthPollAsync(async, &result) == 0 && result == expected, "async app code ends with the expected error");
    Require(output == original, "async writes nothing");
    Require(sceNpAuthDeleteRequest(async) == 0, "delete async request");
}

}

int main() {
    const FakeNet::Bytes token = {0xAA, 0xBB, 0xCC, 0x02};
    Service service(token);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));
    const std::uint8_t address[4] = {0x05, 0x00, 0x12, 0xC6};
    service.SetState(true, 0x0123456789ABCDEFull, "Sixteen_Chars_16", 1, address);
    Require(Service::WaitUntil([] { return SignedInNow(); }, 5000), "signed in");
    Require(NetBridgeEnabled_nid_no_patch() != 0, "bridge on");
    RequireAnswers(CodeUnavailable);

    service.SetState(false, 0, "", 0, address);
    Require(Service::WaitUntil([] { return !SignedInNow(); }, 5000), "signed out");
    RequireAnswers(SignedOut);
    return 0;
}
