#include "prx/libSceNpManager/include/Api.hpp"
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include <array>
#include <cstring>
#include <atomic>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// PSN is not emulated: the user is reported as signed out and online queries fail.
static constexpr int SCE_NP_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80550003);
static constexpr int SCE_NP_ERROR_INVALID_PLATFORM_TYPE = static_cast<int>(0x80550004);
static constexpr int SCE_NP_ERROR_OUT_OF_MEMORY = static_cast<int>(0x80550005);
static constexpr int SCE_NP_ERROR_SIGNED_OUT = static_cast<int>(0x80550006);
static constexpr int SCE_NP_ERROR_USER_NOT_FOUND = static_cast<int>(0x80550007);
static constexpr int SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED = static_cast<int>(0x80550008);
static constexpr int SCE_NP_ERROR_CALLBACK_NOT_REGISTERED = static_cast<int>(0x80550009);
static constexpr int SCE_NP_ERROR_INVALID_SIZE = static_cast<int>(0x80550011);
static constexpr int SCE_NP_ERROR_REQUEST_NOT_FOUND = static_cast<int>(0x80550014);
static constexpr int SCE_NP_ERROR_INVALID_ID = static_cast<int>(0x80550015);
static constexpr int SCE_NP_ERROR_CALLBACK_MAX = static_cast<int>(0x8055001D);
static constexpr int NP_ID_MAPPER_REFUSED_ARGUMENT = static_cast<int>(0x80553083);
static constexpr int NP_ID_MAPPER_REFUSED_REQUEST_ID = static_cast<int>(0x80553085);
static constexpr int NP_MANAGER_INT_REQUEST_DUPLICATE = static_cast<int>(0x80559D03);
static constexpr int NP_MANAGER_INT_REQUEST_TABLE_FULL = static_cast<int>(0x80559D05);
static constexpr int NP_MANAGER_INT_REQUEST_NOT_IN_TABLE = static_cast<int>(0x80559D0A);
static constexpr uint32_t NP_STATE_SIGNED_OUT = 1;
static constexpr uint32_t NP_STATE_SIGNED_IN = 2;
static constexpr char NP_LIBRARY[] = "libSceNpManager";
static constexpr int NP_POLL_ASYNC_FINISHED = 0;
static constexpr uint32_t NP_REACHABILITY_STATE_UNAVAILABLE = 0;
static constexpr uint32_t NP_REACHABILITY_STATE_REACHABLE = 2;
static constexpr uint32_t NP_GAME_PRESENCE_STATUS_OFFLINE = 0;

static std::atomic<int> g_nextRequest{1};

namespace {
std::mutex reachabilityMutex;
void* reachabilityCallback = nullptr;
void* reachabilityUserdata = nullptr;
std::uint32_t deliveredReachability = NP_REACHABILITY_STATE_UNAVAILABLE;
std::mutex requestMutex;
std::unordered_set<int> liveRequests;
std::unordered_map<int, int> requestResults;

bool BridgeSignedIn(int userId, NetBridgeStateView& view) {
    if (userId != USER_SERVICE_INITIAL_USER_ID || NetBridgeEnabled_nid_no_patch() == 0) return false;
    NetBridgeState_nid_no_patch(&view);
    return view.signedIn != 0;
}

void FillOnlineId(NpOnlineId& onlineId, const NetBridgeStateView& view) {
    std::memset(&onlineId, 0, sizeof(onlineId));
    std::memcpy(onlineId.data, view.handle, sizeof(onlineId.data) - 1);
}

void FillNpId(NpId& npId, const NetBridgeStateView& view) {
    std::memset(&npId, 0, sizeof(npId));
    FillOnlineId(npId.online_id, view);
}

int GrantOnline(const char* fn, int reqId, bool argumentsValid, std::uint8_t* result, std::size_t resultSize) {
    NetBridgeStateView view{};
    bool signedIn = false;
    if (NetBridgeEnabled_nid_no_patch() != 0) {
        NetBridgeState_nid_no_patch(&view);
        signedIn = view.signedIn != 0;
    }
    int ret = SCE_NP_ERROR_SIGNED_OUT;
    if (signedIn && !argumentsValid) {
        ret = SCE_NP_ERROR_INVALID_ARGUMENT;
    } else if (signedIn) {
        std::lock_guard lock(requestMutex);
        if (!liveRequests.count(reqId)) {
            ret = SCE_NP_ERROR_REQUEST_NOT_FOUND;
        } else {
            requestResults[reqId] = 0;
            if (result != nullptr && resultSize > 0) {
                std::memset(result, 0, resultSize);
                result[0] = 1;
            }
            ret = 0;
        }
    }
    NetBridgeRecord(NP_LIBRARY, fn, true, ret, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("req_id", reqId);
        if (ret == 0) out.Bool("authorized", true);
    });
    return ret;
}

int AddRequest() {
    const int reqId = g_nextRequest.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard lock(requestMutex);
    liveRequests.insert(reqId);
    return reqId;
}

std::mutex idMapperMutex;
std::uint32_t nextIdMapperRequest = 1;
std::unordered_set<int> idMapperRequests;
constexpr std::size_t NP_MANAGER_INT_REQUEST_SLOTS = 64;
std::mutex managerIntMutex;
std::uint32_t nextManagerIntRequest = 1;
std::unordered_set<int> managerIntRequests;

int SignedOutRequest(int reqId) {
    if (reqId == 0) return SCE_NP_ERROR_INVALID_ARGUMENT;
    std::lock_guard lock(requestMutex);
    return liveRequests.count(reqId) ? SCE_NP_ERROR_SIGNED_OUT : SCE_NP_ERROR_REQUEST_NOT_FOUND;
}

using NpReachabilityCallback = void (APS5_VABI *)(int userId, uint32_t state, void* userdata);

constexpr int NP_STATE_CALLBACK_MAX = 8;

struct StateCallbackA {
    NpStateCallbackA callback = nullptr;
    void* userdata = nullptr;
    bool pending = false;
};

struct StateCallback {
    NpStateCallback callback = nullptr;
    void* userdata = nullptr;
    bool pending = false;
};

std::mutex stateMutex;
std::array<StateCallbackA, NP_STATE_CALLBACK_MAX> stateCallbacksA;
StateCallback stateCallback;
std::uint64_t deliveredStateChanges = 0;
}

extern "C" {

int APS5_VABI sceNpAsmAbort(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientAbortRequest(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientClearNpTitleToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientClearNpTitleTokenA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientCreateRequest2(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientCreateResourceContext(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientCreateResourceContext2(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientDeleteRequest(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientDeleteResourceContext(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientDeleteResourceContext2(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetAppId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetCacheControlMaxAge(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetGameNpTitleInfo(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetGameNpTitleToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetGameTitleBanInfo(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetNpComInfo2(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetNpComInfo2A(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetNpComInfo3(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetNpComInfo4(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetNpTitleId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetNpTitleToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetNpTitleToken2(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetNpTitleTokenA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetRelatedGameNpTitleIds(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetRelatedGameNpTitleIdsA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetRelatedGameNpTitleIdsResult(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetServiceBaseUrl(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetServiceBaseUrlA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetServiceBaseUrlWithNpTitleId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetServiceBaseUrlWithNpTitleIdA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetServiceIdInfo(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientGetServiceIdInfoA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientInitialize(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientSetNpTitleId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmClientTerminate(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmCreateConnection(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmCreateRequest(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmDeleteConnection(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmDeleteRequest(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmGenerateNpTitleToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmGenerateNpTitleToken2(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmGetNpCommInfo(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmGetNpCommInfo2(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmGetRelatedGameNpTitleIds(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmGetServiceBaseUrl(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmGetServiceIdInfo(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmInitialize(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpAsmTerminate(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpCheckNpAvailabilityA(int reqId, int userId) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", reqId); netIn.Int("user_id", userId); });
    if (userId == USER_SERVICE_USER_ID_INVALID) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SignedOutRequest(reqId));
}
int APS5_VABI sceNpGetAccountDateOfBirthA(int userId, void* dateOfBirth) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("user_id", userId); });
    if (!dateOfBirth || userId == USER_SERVICE_USER_ID_INVALID) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpGetAccountLanguageA(int reqId, int userId, void* language) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", reqId); netIn.Int("user_id", userId); });
    if (!language || userId == USER_SERVICE_USER_ID_INVALID) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SignedOutRequest(reqId));
}
int APS5_VABI sceNpGetGamePresenceStatus(const NpOnlineId* onlineId, uint32_t* status) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!onlineId || !status) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    *status = NP_GAME_PRESENCE_STATUS_OFFLINE;
    return netCall.Ret(0);
}
int APS5_VABI sceNpGetGamePresenceStatusA(int userId, uint32_t* status) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("user_id", userId); });
    if (!status || userId == USER_SERVICE_USER_ID_INVALID) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    *status = NP_GAME_PRESENCE_STATUS_OFFLINE;
    return netCall.Ret(0);
}
int APS5_VABI sceNpGetParentalControlInfoA(int reqId, int userId, int8_t* age, void* info) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", reqId); netIn.Int("user_id", userId); });
    if (!age || !info || userId == USER_SERVICE_USER_ID_INVALID) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SignedOutRequest(reqId));
}
int APS5_VABI sceNpIdMapperAbortRequest(int reqId) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (reqId <= 0) return netCall.Ret(NP_ID_MAPPER_REFUSED_REQUEST_ID);
    std::lock_guard lock(idMapperMutex);
    return netCall.Ret(idMapperRequests.count(reqId) ? 0 : SCE_NP_ERROR_REQUEST_NOT_FOUND);
}
int APS5_VABI sceNpIdMapperAccountIdToNpId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIdMapperAccountIdToOnlineId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIdMapperCreateRequest(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    std::lock_guard lock(idMapperMutex);
    const int reqId = static_cast<int>(nextIdMapperRequest++);
    idMapperRequests.insert(reqId);
    return netCall.Ret(reqId);
}
int APS5_VABI sceNpIdMapperDeleteRequest(int reqId) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (reqId <= 0) return netCall.Ret(NP_ID_MAPPER_REFUSED_REQUEST_ID);
    std::lock_guard lock(idMapperMutex);
    return netCall.Ret(idMapperRequests.erase(reqId) ? 0 : SCE_NP_ERROR_REQUEST_NOT_FOUND);
}
int APS5_VABI sceNpIdMapperNpIdToAccountId(int reqId, int userId, const void* input, void* output, std::uint32_t value, std::uint8_t flag, std::uint64_t extra) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)flag;
    (void)extra;
    if (reqId <= 0) return netCall.Ret(NP_ID_MAPPER_REFUSED_REQUEST_ID);
    if (userId == USER_SERVICE_USER_ID_INVALID || !input || !output || value < 1 || value > 1000) return netCall.Ret(NP_ID_MAPPER_REFUSED_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpIdMapperOnlineIdToAccountId(int reqId, int userId, const void* input, void* output, std::uint32_t value, std::uint8_t flag, std::uint64_t extra) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)flag;
    (void)extra;
    if (reqId <= 0) return netCall.Ret(NP_ID_MAPPER_REFUSED_REQUEST_ID);
    if (userId == USER_SERVICE_USER_ID_INVALID || !input || !output || value < 1 || value > 1000) return netCall.Ret(NP_ID_MAPPER_REFUSED_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpInGameMessageAbortHandle(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpInGameMessageCheckCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpInGameMessageGetMemoryPoolStatistics(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpInGameMessagePrepareA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpInGameMessageSendDataA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIntGetAppType(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIntGetGamePresenceStatus(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIntGetNpTitleId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIntGetNpTitleIdSecret(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIntRegisterCheckCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIntRegisterGamePresenceCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpIsPlusMember(int, bool* isMember) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!isMember) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    *isMember = false;
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntAbortRequest(int reqId) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    std::lock_guard lock(managerIntMutex);
    return netCall.Ret(managerIntRequests.count(reqId) ? 0 : SCE_NP_ERROR_REQUEST_NOT_FOUND);
}
int APS5_VABI sceNpManagerIntAddActiveSigninStateCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntAddOnlineIdChangeCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntAddPlusMemberTypeCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntAddSigninStateCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntAuthGetAuthorizationCode(int reqId, int userId, const void* parameter, const void* optionalParameter, std::uint32_t firstValue, std::uint32_t secondValue, std::uint8_t flag, void* output) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)reqId;
    (void)optionalParameter;
    (void)flag;
    if (userId == USER_SERVICE_USER_ID_INVALID || !parameter || !output || firstValue > 3 || secondValue < 1 || secondValue > 3) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntAuthGetIdToken(int reqId, int userId, const void* firstParameter, const void* secondParameter, const void* thirdParameter, std::uint64_t value, void* output) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)reqId;
    (void)value;
    if (userId == USER_SERVICE_USER_ID_INVALID || !firstParameter || !secondParameter || !thirdParameter || !output) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntCheckNpAvailability(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntCheckNpAvailabilityByPid(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntCheckNpState(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntCheckNpStateA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntClearGameAccessToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntClearOnlineIdChangeFlag(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntClearTicket(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntClearUsedFlag(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntClearVshAccessToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntCreateAuthenticationTicket(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntCreateLoginContext(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntCreateLoginRequest(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntCreateRequest(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    std::lock_guard lock(managerIntMutex);
    if (managerIntRequests.size() >= NP_MANAGER_INT_REQUEST_SLOTS) return netCall.Ret(NP_MANAGER_INT_REQUEST_TABLE_FULL);
    const int reqId = static_cast<int>(nextManagerIntRequest++);
    if (reqId <= 0) return netCall.Ret(reqId);
    if (!managerIntRequests.insert(reqId).second) return netCall.Ret(NP_MANAGER_INT_REQUEST_DUPLICATE);
    return netCall.Ret(reqId);
}
int APS5_VABI sceNpManagerIntDeleteLoginContext(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntDeleteRequest(int reqId) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    std::lock_guard lock(managerIntMutex);
    return netCall.Ret(managerIntRequests.erase(reqId) ? 0 : NP_MANAGER_INT_REQUEST_NOT_IN_TABLE);
}
int APS5_VABI sceNpManagerIntGetAccessTokenViaImplicitFlow(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAccountAge(int reqId, int userId, std::uint8_t* age) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)reqId;
    if (userId == USER_SERVICE_USER_ID_INVALID || !age) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetAccountCountry(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAccountCountryA(int userId, void* countryCode) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (userId == USER_SERVICE_USER_ID_INVALID || !countryCode) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetAccountCountrySdk(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAccountDateOfBirthSdk(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAccountId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAccountIdSdk(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAccountLanguage(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAccountLanguageA(int reqId, int userId, void* language) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)reqId;
    if (userId == USER_SERVICE_USER_ID_INVALID || !language) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetAccountNpEnv(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAccountType(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetActiveSigninState(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAuthServerErrorFlag(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetAuthorizationCode(int reqId, int userId, const void* firstParameter, const void* secondParameter, const void* optionalParameter, std::uint32_t mode, std::uint8_t flag, void* optionalOutput, void* output) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)reqId;
    (void)optionalParameter;
    (void)flag;
    (void)optionalOutput;
    if (userId == USER_SERVICE_USER_ID_INVALID || !firstParameter || !secondParameter || !output) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    if (mode != 0 && mode != 3 && mode != 5) return netCall.Ret(SCE_NP_ERROR_INVALID_PLATFORM_TYPE);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetAuthorizationCodeWithPsnoUri(int reqId, int userId, const char* uri, std::uint32_t mode, void* output, std::size_t size) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)reqId;
    if (userId == USER_SERVICE_USER_ID_INVALID || !uri || !output) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    if (size < 1 || size > 0x801) return netCall.Ret(SCE_NP_ERROR_INVALID_SIZE);
    if (mode != 0 && mode != 3 && mode != 5) return netCall.Ret(SCE_NP_ERROR_INVALID_PLATFORM_TYPE);
    if (!std::memchr(uri, 0, 0x2000)) return netCall.Ret(SCE_NP_ERROR_OUT_OF_MEMORY);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetClientCredentialAccessToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetCommunicationRestrictionStatus(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetGameAccessToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetIssuerId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetNpEnv(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetNpId(int userId, NpId* npId) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (userId == USER_SERVICE_USER_ID_INVALID || !npId) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetNpIdByOnlineId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetNpIdSdk(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetOfflineAccountId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetOnlineId(int userId, NpOnlineId* onlineId) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (userId == USER_SERVICE_USER_ID_INVALID || !onlineId) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetOnlineIdChangeFlag(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetOnlineIdSdk(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetParentalControlFlag(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetParentalControlInfo(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetPlusMemberType(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetPlusMemberTypeNB(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetServerError(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetSigninState(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetSigninTelemetryInfo(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetTicket(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetTicketA(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetTitleToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetTitleTokenWithCheck(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetUserIdByAccountId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetUserIdByNpId(const NpId* npId, int* userId) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!npId || !userId) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetUserIdByOnlineId(const NpOnlineId* onlineId, int* userId) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!onlineId || !userId) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}
int APS5_VABI sceNpManagerIntGetUserIdByOnlineIdSdk(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetUserList(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetUserNum(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetVshAccessToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetVshAccessTokenWithCheck(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntGetVshClientId(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntIsSubAccount(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntIsTemporarySignout(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntIsUnregisteredClientError(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntLoginAuthenticate(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntLoginBind(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntLoginCreateAuthenticationTicket(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceNpManagerIntLoginGetAccessToken(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpAbortRequest(int req_id) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    std::lock_guard lock(requestMutex);
    return netCall.Ret(liveRequests.count(req_id) ? 0 : SCE_NP_ERROR_REQUEST_NOT_FOUND);
}

int APS5_VABI sceNpCheckCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NetBridgeStateView view{};
    std::uint64_t changes = 0;
    const bool bridge = NetBridgeEnabled_nid_no_patch() != 0;
    if (bridge) changes = NetBridgeState_nid_no_patch(&view);
    const bool signedIn = bridge && view.signedIn != 0;
    const uint32_t state = signedIn ? NP_STATE_SIGNED_IN : NP_STATE_SIGNED_OUT;
    NpId npId{};
    if (signedIn) FillNpId(npId, view);
    std::array<StateCallbackA, NP_STATE_CALLBACK_MAX> callbacksA;
    StateCallback legacy;
    {
        std::lock_guard lock(stateMutex);
        if (bridge && changes != deliveredStateChanges) {
            deliveredStateChanges = changes;
            for (auto& entry : stateCallbacksA) entry.pending = entry.pending || entry.callback != nullptr;
            stateCallback.pending = stateCallback.pending || stateCallback.callback != nullptr;
        }
        callbacksA = stateCallbacksA;
        legacy = stateCallback;
        for (auto& entry : stateCallbacksA) entry.pending = false;
        stateCallback.pending = false;
    }
    if (legacy.pending) {
        legacy.callback(USER_SERVICE_INITIAL_USER_ID, state, signedIn ? &npId : nullptr, legacy.userdata);
        NetBridgeRecord(NP_LIBRARY, "sceNpCheckCallback", false, 0, [&](NetBridgeJson& in, NetBridgeJson& out) {
            in.Text("callback", "state");
            out.Int("user_id", USER_SERVICE_INITIAL_USER_ID).Uint("state", state);
        });
    }
    for (const auto& entry : callbacksA) {
        if (!entry.pending) continue;
        entry.callback(USER_SERVICE_INITIAL_USER_ID, state, entry.userdata);
        NetBridgeRecord(NP_LIBRARY, "sceNpCheckCallback", false, 0, [&](NetBridgeJson& in, NetBridgeJson& out) {
            in.Text("callback", "stateA");
            out.Int("user_id", USER_SERVICE_INITIAL_USER_ID).Uint("state", state);
        });
    }
    const uint32_t reachability = signedIn ? NP_REACHABILITY_STATE_REACHABLE : NP_REACHABILITY_STATE_UNAVAILABLE;
    NpReachabilityCallback reachabilityTarget = nullptr;
    void* reachabilityArgument = nullptr;
    {
        std::lock_guard lock(reachabilityMutex);
        if (bridge && reachabilityCallback != nullptr && reachability != deliveredReachability) {
            deliveredReachability = reachability;
            reachabilityTarget = reinterpret_cast<NpReachabilityCallback>(reachabilityCallback);
            reachabilityArgument = reachabilityUserdata;
        }
    }
    if (reachabilityTarget != nullptr) {
        reachabilityTarget(USER_SERVICE_INITIAL_USER_ID, reachability, reachabilityArgument);
        NetBridgeRecord(NP_LIBRARY, "sceNpCheckCallback", false, 0, [&](NetBridgeJson& in, NetBridgeJson& out) {
            in.Text("callback", "reachability");
            out.Int("user_id", USER_SERVICE_INITIAL_USER_ID).Uint("state", reachability);
        });
    }
    NetBridgeDeliverPushEvents_nid_no_patch();
    return netCall.Ret(0);
}

int APS5_VABI sceNpCheckNpAvailability(std::int32_t requestId, const NpOnlineId* onlineId, std::uint8_t* available) {
 NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("request_id", requestId); });
 if (requestId == 0 || onlineId == nullptr) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
 if (available != nullptr) *available = 0;
 return netCall.Ret(SCE_NP_ERROR_USER_NOT_FOUND);
}

int APS5_VABI sceNpCheckNpReachability(int req_id, int user_id) {
 NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Int("user_id", user_id); });
 (void)req_id;
 (void)user_id;
 return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpCheckPremium(int req_id, const NpCheckPremiumParameter* param, NpCheckPremiumResult* result) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (NetBridgeEnabled_nid_no_patch() != 0) {
        return netCall.Ret(GrantOnline("sceNpCheckPremium", req_id, req_id != 0 && param != nullptr && result != nullptr, result != nullptr ? result->opaque : nullptr, sizeof(result->opaque)));
    }
    (void)param;
    (void)result;
    NetBridgeRecord(NP_LIBRARY, "sceNpCheckPremium", true, SCE_NP_ERROR_SIGNED_OUT, [&](NetBridgeJson& in, NetBridgeJson&) { in.Int("req_id", req_id); });
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpCreateAsyncRequest(const NpCreateAsyncRequestParameter* param) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)param;
    return netCall.Ret(AddRequest());
}

int APS5_VABI sceNpCreateRequest(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    return netCall.Ret(AddRequest());
}

int APS5_VABI sceNpDeleteRequest(int req_id) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    std::lock_guard lock(requestMutex);
    if (!liveRequests.count(req_id)) return netCall.Ret(SCE_NP_ERROR_REQUEST_NOT_FOUND);
    liveRequests.erase(req_id);
    requestResults.erase(req_id);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGetAccountAge(int req_id, int user_id, uint8_t* age) {
 NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Int("user_id", user_id); });
 (void)req_id;
 (void)user_id;
 if (!age) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
 return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpGetAccountCountry(const NpOnlineId* online_id, void* country_code) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!online_id || !country_code) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_USER_NOT_FOUND);
}

int APS5_VABI sceNpGetAccountDateOfBirth(const NpOnlineId* online_id, void* date_of_birth) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!online_id || !date_of_birth) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_USER_NOT_FOUND);
}

int APS5_VABI sceNpGetAccountLanguage(int req_id, const NpOnlineId* online_id, void* language) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    (void)req_id;
    if (!online_id || !language) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_USER_NOT_FOUND);
}

int APS5_VABI sceNpGetParentalControlInfo(int req_id, const NpOnlineId* online_id, int8_t* age, void* info) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    (void)req_id;
    if (!online_id || !age || !info) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_USER_NOT_FOUND);
}

int APS5_VABI sceNpGetUserIdByOnlineId(const NpOnlineId* online_id, int* user_id) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!online_id || !user_id) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpCheckPlus(int req_id, const void* param, void* result) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (req_id == 0 || !param || !result) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    if (NetBridgeEnabled_nid_no_patch() != 0) return netCall.Ret(GrantOnline("sceNpCheckPlus", req_id, true, static_cast<std::uint8_t*>(result), 1));
    NetBridgeRecord(NP_LIBRARY, "sceNpCheckPlus", true, SCE_NP_ERROR_SIGNED_OUT, [&](NetBridgeJson& in, NetBridgeJson&) { in.Int("req_id", req_id); });
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpNotifyPlusFeature(const void* param) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (NetBridgeEnabled_nid_no_patch() != 0) {
        if (!param) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
        NetBridgeFeature_nid_no_patch(-1, 0);
        NetBridgeRecord(NP_LIBRARY, "sceNpNotifyPlusFeature", true, 0, [&](NetBridgeJson& in, NetBridgeJson&) { in.Bool("param_null", false); });
        return netCall.Ret(0);
    }
    NetBridgeRecord(NP_LIBRARY, "sceNpNotifyPlusFeature", false, 0, [&](NetBridgeJson& in, NetBridgeJson&) { in.Bool("param_null", param == nullptr); });
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGetAccountId(const NpOnlineId* online_id, uint64_t* account_id) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!online_id || !account_id) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpSetGamePresenceOnline(int req_id, const NpOnlineId* online_id, uint32_t presence) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Uint("presence", presence); });
    (void)presence;
    if (req_id == 0 || !online_id) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    {
        std::lock_guard lock(requestMutex);
        if (!liveRequests.count(req_id)) return netCall.Ret(SCE_NP_ERROR_REQUEST_NOT_FOUND);
    }
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpGetAccountCountryA(int user_id, void* country_code) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("user_id", user_id); });
    (void)user_id;
    if (!country_code) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpGetAccountIdA(int user_id, uint64_t* account_id) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NetBridgeStateView view{};
    int ret = SCE_NP_ERROR_SIGNED_OUT;
    if (BridgeSignedIn(user_id, view)) {
        ret = account_id != nullptr ? 0 : SCE_NP_ERROR_INVALID_ARGUMENT;
        if (ret == 0) *account_id = view.accountId;
    }
    NetBridgeRecord(NP_LIBRARY, "sceNpGetAccountIdA", true, ret, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("user_id", user_id);
        if (ret == 0) out.Uint("account_id", view.accountId);
    });
    return netCall.Ret(ret);
}

int APS5_VABI sceNpGetNpId(int user_id, NpId* np_id) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!np_id) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    NetBridgeStateView view{};
    int ret = SCE_NP_ERROR_SIGNED_OUT;
    if (BridgeSignedIn(user_id, view)) {
        FillNpId(*np_id, view);
        ret = 0;
    }
    NetBridgeRecord(NP_LIBRARY, "sceNpGetNpId", true, ret, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("user_id", user_id);
        if (ret == 0) out.Text("online_id", np_id->online_id.data);
    });
    return netCall.Ret(ret);
}

int APS5_VABI sceNpGetNpReachabilityState(int user_id, uint32_t* state) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("user_id", user_id); });
    (void)user_id;
    if (!state) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    NetBridgeStateView view{};
    *state = BridgeSignedIn(user_id, view) ? NP_REACHABILITY_STATE_REACHABLE : NP_REACHABILITY_STATE_UNAVAILABLE;
    if (netCall.On()) netCall.Out().Uint("state", *state);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGetOnlineId(int user_id, NpOnlineId* online_id) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!online_id) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    NetBridgeStateView view{};
    int ret = SCE_NP_ERROR_SIGNED_OUT;
    if (BridgeSignedIn(user_id, view)) {
        FillOnlineId(*online_id, view);
        ret = 0;
    }
    NetBridgeRecord(NP_LIBRARY, "sceNpGetOnlineId", true, ret, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("user_id", user_id);
        if (ret == 0) out.Text("online_id", online_id->data);
    });
    return netCall.Ret(ret);
}

int APS5_VABI sceNpGetState(int user_id, uint32_t* state) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!state) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    NetBridgeStateView view{};
    *state = BridgeSignedIn(user_id, view) ? NP_STATE_SIGNED_IN : NP_STATE_SIGNED_OUT;
    NetBridgeRecord(NP_LIBRARY, "sceNpGetState", true, 0, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("user_id", user_id);
        out.Uint("state", *state);
    });
    return netCall.Ret(0);
}

int APS5_VABI sceNpHasSignedUp(int user_id, bool* has_signed_up) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("user_id", user_id); });
    if (!has_signed_up) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    NetBridgeStateView view{};
    *has_signed_up = BridgeSignedIn(user_id, view);
    if (netCall.On()) netCall.Out().Bool("has_signed_up", *has_signed_up);
    return netCall.Ret(0);
}

int APS5_VABI sceNpPollAsync(int req_id, int* result) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!result) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    int value = SCE_NP_ERROR_SIGNED_OUT;
    {
        std::lock_guard lock(requestMutex);
        if (!liveRequests.count(req_id)) return netCall.Ret(SCE_NP_ERROR_REQUEST_NOT_FOUND);
        const auto found = requestResults.find(req_id);
        if (found != requestResults.end()) value = found->second;
    }
    *result = value;
    NetBridgeRecord(NP_LIBRARY, "sceNpPollAsync", true, NP_POLL_ASYNC_FINISHED, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("req_id", req_id);
        out.Int("result", *result);
    });
    return netCall.Ret(NP_POLL_ASYNC_FINISHED);
}

int APS5_VABI sceNpWaitAsync(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI sceNpRegisterGamePresenceCallback(void* callback, void* userdata) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)callback;
    (void)userdata;
}

int APS5_VABI sceNpRegisterNpReachabilityStateCallback(void* callback, void* userdata) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!callback) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    std::lock_guard lock(reachabilityMutex);
    if (reachabilityCallback) return netCall.Ret(SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED);
    reachabilityCallback = callback;
    reachabilityUserdata = userdata;
    return netCall.Ret(0);
}

int APS5_VABI sceNpUnregisterNpReachabilityStateCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    std::lock_guard lock(reachabilityMutex);
    if (!reachabilityCallback) return netCall.Ret(SCE_NP_ERROR_CALLBACK_NOT_REGISTERED);
    reachabilityCallback = nullptr;
    reachabilityUserdata = nullptr;
    deliveredReachability = NP_REACHABILITY_STATE_UNAVAILABLE;
    return netCall.Ret(0);
}

int APS5_VABI sceNpRegisterPlusEventCallback(void* callback, void* userdata) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)userdata;
    if (!callback) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(0);
}

int APS5_VABI sceNpRegisterPremiumEventCallback(void* callback, void* userdata) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)userdata;
    if (!callback) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(0);
}

int APS5_VABI sceNpRegisterStateCallback(NpStateCallback callback, void* userdata) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!callback) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    std::lock_guard lock(stateMutex);
    if (stateCallback.callback) return netCall.Ret(SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED);
    stateCallback = {callback, userdata, true};
    return netCall.Ret(0);
}

int APS5_VABI sceNpSetContentRestriction(const NpContentRestriction* restriction) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    struct Fields {
        std::uint64_t size;
        std::int8_t defaultAgeRestriction;
        std::uint8_t padding[3];
        std::int32_t ageRestrictionCount;
        const void* ageRestrictions;
    };
    if (!restriction) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    const auto* fields = reinterpret_cast<const Fields*>(restriction);
    if (fields->size != sizeof(Fields)) return netCall.Ret(SCE_NP_ERROR_INVALID_SIZE);
    if (fields->defaultAgeRestriction < 0 || fields->ageRestrictionCount > 0x100) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    if (fields->ageRestrictionCount > 0 && !fields->ageRestrictions) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(0);
}

int APS5_VABI sceNpSetNpTitleId(const NpTitleId* title_id, const NpTitleSecret* title_secret) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)title_id;
    (void)title_secret;
    return netCall.Ret(0);
}

int APS5_VABI sceNpSetTimeout(int reqId, int32_t resolveRetry, uint32_t resolveTimeout, uint32_t connTimeout,
    uint32_t sendTimeout, uint32_t recvTimeout) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", reqId); netIn.Int("resolve_retry", resolveRetry); netIn.Uint("resolve_timeout", resolveTimeout); netIn.Uint("conn_timeout", connTimeout); netIn.Uint("send_timeout", sendTimeout); netIn.Uint("recv_timeout", recvTimeout); });
    constexpr uint32_t minResolveTimeout = 1000000;
    constexpr uint32_t minTransferTimeout = 10000000;
    auto belowMinimum = [](uint32_t timeout, uint32_t minimum) { return timeout != 0 && timeout < minimum; };
    if (reqId <= 0 || resolveRetry < 0) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    if (resolveRetry == 0 && resolveTimeout == 0 && connTimeout == 0 && sendTimeout == 0 && recvTimeout == 0) {
        return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    }
    if (belowMinimum(resolveTimeout, minResolveTimeout) || belowMinimum(connTimeout, minTransferTimeout) ||
        belowMinimum(sendTimeout, minTransferTimeout) || belowMinimum(recvTimeout, minTransferTimeout)) {
        return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    }
    std::lock_guard lock(requestMutex);
    return netCall.Ret(liveRequests.count(reqId) ? 0 : SCE_NP_ERROR_REQUEST_NOT_FOUND);
}

int APS5_VABI sceNpUnregisterStateCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    std::lock_guard lock(stateMutex);
    if (!stateCallback.callback) return netCall.Ret(SCE_NP_ERROR_CALLBACK_NOT_REGISTERED);
    stateCallback = {};
    return netCall.Ret(0);
}

int APS5_VABI sceNpGetAccountLanguage2(int req_id, int user_id, void* language) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Int("user_id", user_id); });
    (void)req_id;
    (void)user_id;
    (void)language;
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpNotifyPremiumFeature(const void* param) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)param;
    return netCall.Ret(0);
}

int APS5_VABI sceNpRegisterStateCallbackA(NpStateCallbackA callback, void* userdata) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    if (!callback) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    std::lock_guard lock(stateMutex);
    for (const auto& entry : stateCallbacksA) {
        if (entry.callback == callback) return netCall.Ret(SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED);
    }
    for (int index = 0; index < NP_STATE_CALLBACK_MAX; ++index) {
        if (stateCallbacksA[index].callback) continue;
        stateCallbacksA[index] = {callback, userdata, true};
        return netCall.Ret(index + 1);
    }
    return netCall.Ret(SCE_NP_ERROR_CALLBACK_MAX);
}

int APS5_VABI sceNpUnregisterStateCallbackA(int callbackId) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("callback_id", callbackId); });
    if (static_cast<uint32_t>(callbackId) - 1u >= static_cast<uint32_t>(NP_STATE_CALLBACK_MAX)) return netCall.Ret(SCE_NP_ERROR_INVALID_ID);
    std::lock_guard lock(stateMutex);
    auto& entry = stateCallbacksA[callbackId - 1];
    if (!entry.callback) return netCall.Ret(SCE_NP_ERROR_CALLBACK_NOT_REGISTERED);
    entry = {};
    return netCall.Ret(0);
}

int APS5_VABI sceNpUnregisterPremiumEventCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUnregisterPlusEventCallback(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGetUserIdByAccountId(uint64_t account_id, int* user_id) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Uint("account_id", account_id); });
    if (account_id == 0 || !user_id) return netCall.Ret(SCE_NP_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(SCE_NP_ERROR_SIGNED_OUT);
}

int APS5_VABI sceNpGetSandboxInfo(void* sandbox_info) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    (void)sandbox_info;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

APS5_EXPORT("8weiQyIdI3A", sceNpUnknown_8weiQyIdI3A);
int APS5_VABI sceNpUnknown_8weiQyIdI3A(int userId, const NpOnlineId* buffer) {
    NetBridgeCall netCall("libSceNpManager", __func__, [&](NetBridgeJson& netIn) { netIn.Int("user_id", userId); });
    (void)userId;
    (void)buffer;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

APS5_EXPORT("WxLcN76q19o", sceNpUnknown_WxLcN76q19o);
int APS5_VABI sceNpUnknown_WxLcN76q19o(void) {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpCheckCallbackForLib() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    return netCall.Ret(sceNpCheckCallback());
}

}

extern "C" {

int APS5_VABI sceNpInGameMessageCreateHandle() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpInGameMessageDeleteHandle() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpInGameMessageInitialize() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpInGameMessagePrepare() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpInGameMessageSendData() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpInGameMessageTerminate() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpSnsFacebookCreateRequest() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpSnsFacebookDeleteRequest() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpSnsFacebookGetAccessToken() {
    NetBridgeCall netCall("libSceNpManager", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

}
