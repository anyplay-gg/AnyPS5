#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "prx/libSceNetCtl/include/NetCtl.hpp"
#include "prx/libSceNpManager/include/Api.hpp"
#include "prx/libSceNpWebApi2/include/Api.hpp"

namespace {

constexpr int INITIAL_USER_ID = 0x10000000;
constexpr int SCE_NP_ERROR_SIGNED_OUT = static_cast<int>(0x80550006);
constexpr std::uint64_t EXPECTED_ACCOUNT_ID = 10001;

template <typename F>
bool WaitUntil(F predicate, int timeoutMs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    return predicate();
}

int TestSignedOut() {
    std::uint32_t npState = 0;
    if (sceNpGetState(INITIAL_USER_ID, &npState) != 0 || npState == 2) {
        std::fprintf(stderr, "FAIL: player reached SignedIn unexpectedly in signed-out test (state=%u)\n", npState);
        return 1;
    }

    const int plusReq = sceNpCreateRequest();
    if (plusReq <= 0) {
        std::fprintf(stderr, "FAIL: sceNpCreateRequest failed in signed-out test\n");
        return 1;
    }
    const std::uint8_t plusParam[16] = {};
    std::uint8_t plusAuthorized = 0x5A;
    const int plusRet = sceNpCheckPlus(plusReq, plusParam, &plusAuthorized);
    sceNpDeleteRequest(plusReq);

    if (plusRet != SCE_NP_ERROR_SIGNED_OUT) {
        std::fprintf(stderr, "FAIL: sceNpCheckPlus expected SCE_NP_ERROR_SIGNED_OUT (0x%x), got 0x%x\n",
                     static_cast<unsigned>(SCE_NP_ERROR_SIGNED_OUT), static_cast<unsigned>(plusRet));
        return 1;
    }
    if (plusAuthorized != 0x5A) {
        std::fprintf(stderr, "FAIL: sceNpCheckPlus touched result pointer while signed out (got 0x%x)\n", plusAuthorized);
        return 1;
    }

    std::puts("bridge e2e: signed-out assertions passed");
    return 0;
}

int TestSignedIn() {
    std::uint32_t npState = 0;
    if (!WaitUntil([&] { return sceNpGetState(INITIAL_USER_ID, &npState) == 0 && npState == 2; }, 10000)) {
        std::fprintf(stderr, "FAIL: player did not reach SignedIn (state=%u)\n", npState);
        return 1;
    }

    NpOnlineId onlineId{};
    if (sceNpGetOnlineId(INITIAL_USER_ID, &onlineId) != 0 || onlineId.data[0] == '\0') {
        std::fprintf(stderr, "FAIL: sceNpGetOnlineId failed\n");
        return 1;
    }

    std::uint64_t accountId = 0;
    if (sceNpGetAccountIdA(INITIAL_USER_ID, &accountId) != 0 || accountId != EXPECTED_ACCOUNT_ID) {
        std::fprintf(stderr, "FAIL: sceNpGetAccountIdA failed (got %llu)\n", static_cast<unsigned long long>(accountId));
        return 1;
    }

    if (sceNetCtlInit() != 0) {
        std::fprintf(stderr, "FAIL: sceNetCtlInit failed\n");
        return 1;
    }
    NetCtlNatInfo natInfo{};
    natInfo.size = sizeof(NetCtlNatInfo);
    if (sceNetCtlGetNatInfo(&natInfo) != 0) {
        std::fprintf(stderr, "FAIL: sceNetCtlGetNatInfo failed\n");
        return 1;
    }

    const int plusReq = sceNpCreateRequest();
    if (plusReq <= 0) {
        std::fprintf(stderr, "FAIL: sceNpCreateRequest failed\n");
        return 1;
    }
    const std::uint8_t plusParam[16] = {};
    std::uint8_t plusAuthorized = 0;
    const int plusRet = sceNpCheckPlus(plusReq, plusParam, &plusAuthorized);
    int pollResult = -1;
    sceNpPollAsync(plusReq, &pollResult);
    sceNpDeleteRequest(plusReq);
    if (plusRet != 0 || plusAuthorized != 1) {
        std::fprintf(stderr, "FAIL: sceNpCheckPlus failed (ret=%d, auth=%u)\n", plusRet, plusAuthorized);
        return 1;
    }

    const int library = sceNpWebApi2Initialize(0, 1 << 20);
    const int userContext = sceNpWebApi2CreateUserContext(library, INITIAL_USER_ID);
    std::int64_t requestId = 0;
    if (sceNpWebApi2CreateRequest(userContext, "anyplay", "/v1/sessions", "POST", nullptr, &requestId) != 0 || requestId == 0) {
        std::fprintf(stderr, "FAIL: sceNpWebApi2CreateRequest failed\n");
        return 1;
    }
    sceNpWebApi2AddHttpRequestHeader(requestId, "Content-Type", "application/json");
    const char* sessionBody = "{\"max_members\":4,\"visibility\":\"friends\"}";
    NpWebApi2ResponseInformationOption info{};
    if (sceNpWebApi2SendRequest(requestId, sessionBody, std::strlen(sessionBody), &info) != 0 || info.http_status != 200) {
        std::fprintf(stderr, "FAIL: sceNpWebApi2SendRequest failed (status=%d)\n", info.http_status);
        return 1;
    }
    sceNpWebApi2DeleteRequest(requestId);
    sceNpWebApi2DeleteUserContext(userContext);

    std::puts("bridge e2e: signed-in assertions passed");
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    const char* endpoint = std::getenv("ANYPS5_NET_ENDPOINT");
    const char* token = std::getenv("ANYPS5_NET_TOKEN");
    if (endpoint == nullptr || token == nullptr || endpoint[0] == '\0' || token[0] == '\0') {
        std::puts("bridge_e2e_game: ANYPS5_NET_ENDPOINT or ANYPS5_NET_TOKEN not set; skipping live service tests");
        return 77;
    }

    const bool expectSignedOut = (argc > 1 && std::strcmp(argv[1], "--signed-out") == 0);

    if (expectSignedOut) {
        return TestSignedOut();
    }
    return TestSignedIn();
}
