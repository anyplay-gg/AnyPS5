#include "prx/libSceNpAuth/include/Api.hpp"
#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"

static constexpr int ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80550301);
static constexpr int SCE_NP_ERROR_SIGNED_OUT = static_cast<int>(0x80550006);
static constexpr int ORBIS_NP_AUTH_ERROR_ABORTED = static_cast<int>(0x80550304);
static constexpr int ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND = static_cast<int>(0x80550306);
static constexpr int ORBIS_NP_AUTH_ERROR_INVALID_ID = static_cast<int>(0x80550307);
static constexpr int ORBIS_NP_AUTH_ERROR_CODE_SERVICE_UNAVAILABLE = ORBIS_NP_AUTH_ERROR_ABORTED;
static constexpr int NP_POLL_ASYNC_FINISHED = 0;
static constexpr int NP_POLL_ASYNC_RUNNING = 1;

namespace {

struct RequestInfo {
 bool isAsync = false;
 bool processed = false;
 bool aborted = false;
 bool polled = false;
 int result = SCE_NP_ERROR_SIGNED_OUT;
 std::int32_t resolveRetry = 0;
 std::array<std::uint32_t, 4> timeouts{};
};

std::mutex g_requestMutex;
std::unordered_map<int, RequestInfo> g_requests;
std::atomic<int> g_nextRequest{1};

int CodeOrTokenAnswer() {
 if (NetBridgeEnabled_nid_no_patch() != 0) {
  NetBridgeStateView view{};
  NetBridgeState_nid_no_patch(&view);
  if (view.signedIn != 0) return ORBIS_NP_AUTH_ERROR_CODE_SERVICE_UNAVAILABLE;
 }
 return SCE_NP_ERROR_SIGNED_OUT;
}

int CreateRequest(bool isAsync) {
 const int reqId = g_nextRequest.fetch_add(1, std::memory_order_relaxed);
 std::lock_guard lock(g_requestMutex);
 g_requests[reqId] = {isAsync, false, false, false, SCE_NP_ERROR_SIGNED_OUT};
 return reqId;
}

}

extern "C" {

int APS5_VABI sceNpAuthAbortRequest(int req_id) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
 std::lock_guard lock(g_requestMutex);
 auto it = g_requests.find(req_id);
 if (it == g_requests.end()) return netCall.Ret(ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND);
 if (!it->second.processed) it->second.aborted = true;
 return netCall.Ret(0);
}

int APS5_VABI sceNpAuthCreateAsyncRequest(const void* param) {
 NetBridgeCall netCall("libSceNpAuth", __func__);
 (void)param;
 return netCall.Ret(CreateRequest(true));
}

int APS5_VABI sceNpAuthCreateRequest(void) {
 NetBridgeCall netCall("libSceNpAuth", __func__);
 return netCall.Ret(CreateRequest(false));
}

int APS5_VABI sceNpAuthDeleteRequest(int req_id) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
 std::lock_guard lock(g_requestMutex);
 g_requests.erase(req_id);
 return netCall.Ret(0);
}

int APS5_VABI sceNpAuthGetAuthorizedAppCode(std::int32_t requestId, const NpAuthAuthorizedAppCodeParameter* parameter, char* code, std::int32_t* issuerId, bool* authorizationRequired) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("request_id", requestId); });
 (void)issuerId;
 if (parameter == nullptr || code == nullptr || authorizationRequired == nullptr) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if (parameter->size != sizeof(NpAuthAuthorizedAppCodeParameter)) return netCall.Ret(static_cast<int>(0x80550302u));
 if (parameter->userId == -1 || parameter->clientId == nullptr || parameter->scope == nullptr || parameter->option > 1) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 std::lock_guard lock(g_requestMutex);
 auto it = g_requests.find(requestId);
 if (it == g_requests.end()) return netCall.Ret(ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND);
 if (it->second.processed) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if (it->second.aborted) return netCall.Ret(ORBIS_NP_AUTH_ERROR_ABORTED);
 it->second.processed = true;
 if (it->second.isAsync) {
  it->second.result = CodeOrTokenAnswer();
  return netCall.Ret(0);
 }
 return netCall.Ret(CodeOrTokenAnswer());
}

int APS5_VABI sceNpAuthGetAuthorizationCodeV3(int req_id, const void* param, void* auth_code, int* issuer_id) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
 (void)issuer_id;
 if (!param || !auth_code) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 std::lock_guard lock(g_requestMutex);
 auto it = g_requests.find(req_id);
 if (it == g_requests.end()) return netCall.Ret(ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND);
 if (it->second.processed) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if (it->second.aborted) {
  return netCall.Ret(ORBIS_NP_AUTH_ERROR_ABORTED);
 }
 it->second.processed = true;
 if (it->second.isAsync) {
  it->second.result = CodeOrTokenAnswer();
  return netCall.Ret(0);
 }
 return netCall.Ret(CodeOrTokenAnswer());
}

int APS5_VABI sceNpAuthGetIdTokenV3(int req_id, const void* param, void* id_token) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
 if (!param || !id_token) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 std::lock_guard lock(g_requestMutex);
 auto it = g_requests.find(req_id);
 if (it == g_requests.end()) return netCall.Ret(ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND);
 if (it->second.processed) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if (it->second.aborted) {
  return netCall.Ret(ORBIS_NP_AUTH_ERROR_ABORTED);
 }
 it->second.processed = true;
 if (it->second.isAsync) {
  it->second.result = CodeOrTokenAnswer();
  return netCall.Ret(0);
 }
 return netCall.Ret(CodeOrTokenAnswer());
}

int APS5_VABI sceNpAuthGetAuthorizationCode(int req_id, const void* param, void* auth_code) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
 return netCall.Ret(sceNpAuthGetAuthorizationCodeV3(req_id, param, auth_code, nullptr));
}

int APS5_VABI sceNpAuthGetIdToken(int req_id, const void* param, void* id_token) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
 return netCall.Ret(sceNpAuthGetIdTokenV3(req_id, param, id_token));
}

int APS5_VABI sceNpAuthPollAsync(int req_id, int* result) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
 std::lock_guard lock(g_requestMutex);
 auto it = g_requests.find(req_id);
 if (it == g_requests.end()) return netCall.Ret(ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND);
 if (!it->second.isAsync || !it->second.processed) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ID);
 if (!it->second.polled) {
  it->second.polled = true;
  return netCall.Ret(NP_POLL_ASYNC_RUNNING);
 }
 if (result) *result = it->second.result;
 return netCall.Ret(NP_POLL_ASYNC_FINISHED);
}

int APS5_VABI sceNpAuthSetTimeout(std::int32_t requestId, std::int32_t resolveRetry, std::uint32_t resolveTimeout, std::uint32_t connectTimeout, std::uint32_t sendTimeout, std::uint32_t receiveTimeout) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("request_id", requestId); netIn.Int("resolve_retry", resolveRetry); netIn.Uint("resolve_timeout", resolveTimeout); netIn.Uint("connect_timeout", connectTimeout); netIn.Uint("send_timeout", sendTimeout); netIn.Uint("receive_timeout", receiveTimeout); });
 if (resolveRetry < 0) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if ((static_cast<std::uint32_t>(resolveRetry) | resolveTimeout | connectTimeout | sendTimeout | receiveTimeout) == 0) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if (resolveTimeout != 0 && resolveTimeout < 1000000) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if (connectTimeout != 0 && connectTimeout < 10000000) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if (sendTimeout != 0 && sendTimeout < 10000000) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 if (receiveTimeout != 0 && receiveTimeout < 10000000) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 std::lock_guard lock(g_requestMutex);
 const auto found = g_requests.find(requestId);
 if (found == g_requests.end()) return netCall.Ret(ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND);
 found->second.resolveRetry = resolveRetry;
 found->second.timeouts = {resolveTimeout, connectTimeout, sendTimeout, receiveTimeout};
 return netCall.Ret(0);
}

int APS5_VABI sceNpAuthWaitAsync(int req_id, int* result) {
 NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
 if (!result) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT);
 std::lock_guard lock(g_requestMutex);
 auto it = g_requests.find(req_id);
 if (it == g_requests.end()) return netCall.Ret(ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND);
 if (!it->second.isAsync || !it->second.processed) return netCall.Ret(ORBIS_NP_AUTH_ERROR_INVALID_ID);
 *result = it->second.result;
 return netCall.Ret(NP_POLL_ASYNC_FINISHED);
}

int APS5_VABI sceNpAuthGetAuthorizationCodeA(int req_id, const void* param, void* auth_code, int* issuer_id) {
    NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    return netCall.Ret(sceNpAuthGetAuthorizationCodeV3(req_id, param, auth_code, issuer_id));
}

int APS5_VABI sceNpAuthGetIdTokenA(int req_id, const void* param, void* token) {
    NetBridgeCall netCall("libSceNpAuth", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    return netCall.Ret(sceNpAuthGetIdTokenV3(req_id, param, token));
}

}
