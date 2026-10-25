#include "SceTypes.hpp"
#include <array>
#include <cstdlib>

extern "C" {
int APS5_VABI sceNpAuthCreateRequest();
int APS5_VABI sceNpAuthCreateAsyncRequest(const void* param);
int APS5_VABI sceNpAuthPollAsync(int req_id, int* result);
int APS5_VABI sceNpAuthDeleteRequest(int);
int APS5_VABI sceNpAuthGetAuthorizedAppCode(std::int32_t, const NpAuthAuthorizedAppCodeParameter*, char*, std::int32_t*, bool*);
}

static void Require(bool condition) {
    if (!condition) std::abort();
}

int main() {
    constexpr auto invalidArgument = static_cast<int>(0x80550301u);
    constexpr auto invalidSize = static_cast<int>(0x80550302u);
    constexpr auto requestNotFound = static_cast<int>(0x80550306u);
    constexpr auto signedOut = static_cast<int>(0x80550006u);
    std::array<char, 136> client{};
    std::array<char, 128> code{};
    code.fill('x');
    const auto original = code;
    bool authorizationRequired = true;
    NpAuthAuthorizedAppCodeParameter parameter{40, 1, reinterpret_cast<const NpAuthClientId*>(client.data()), "", 0};
    Require(sceNpAuthGetAuthorizedAppCode(0, nullptr, code.data(), nullptr, &authorizationRequired) == invalidArgument);
    parameter.size = 39;
    Require(sceNpAuthGetAuthorizedAppCode(0, &parameter, code.data(), nullptr, &authorizationRequired) == invalidSize);
    parameter.size = 40;
    Require(sceNpAuthGetAuthorizedAppCode(0, &parameter, nullptr, nullptr, &authorizationRequired) == invalidArgument);
    Require(sceNpAuthGetAuthorizedAppCode(0, &parameter, code.data(), nullptr, nullptr) == invalidArgument);
    parameter.option = 2;
    Require(sceNpAuthGetAuthorizedAppCode(0, &parameter, code.data(), nullptr, &authorizationRequired) == invalidArgument);
    parameter.option = 0;
    parameter.userId = -1;
    Require(sceNpAuthGetAuthorizedAppCode(0, &parameter, code.data(), nullptr, &authorizationRequired) == invalidArgument);
    parameter.userId = 1;
    Require(sceNpAuthGetAuthorizedAppCode(0, &parameter, code.data(), nullptr, &authorizationRequired) == requestNotFound);
    const auto requestId = sceNpAuthCreateRequest();
    Require(requestId > 0);
    Require(sceNpAuthGetAuthorizedAppCode(requestId, &parameter, code.data(), nullptr, &authorizationRequired) == signedOut);
    Require(code == original && authorizationRequired);
    Require(sceNpAuthGetAuthorizedAppCode(requestId, &parameter, code.data(), nullptr, &authorizationRequired) == invalidArgument);
    Require(sceNpAuthDeleteRequest(requestId) == 0);
    const auto asyncId = sceNpAuthCreateAsyncRequest(nullptr);
    Require(asyncId > 0);
    Require(sceNpAuthGetAuthorizedAppCode(asyncId, &parameter, code.data(), nullptr, &authorizationRequired) == 0);
    int result = 0;
    Require(sceNpAuthPollAsync(asyncId, &result) == 1);
    Require(sceNpAuthPollAsync(asyncId, &result) == 0 && result == signedOut);
    Require(code == original && authorizationRequired);
    Require(sceNpAuthDeleteRequest(asyncId) == 0);
}
