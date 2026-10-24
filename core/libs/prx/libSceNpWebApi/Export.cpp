#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

static constexpr int SCE_NP_WEBAPI_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80552902);
static constexpr int SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND = static_cast<int>(0x80552904);
static constexpr int SCE_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND = static_cast<int>(0x8055290B);
static constexpr int SCE_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND = static_cast<int>(0x8055290D);
static constexpr int SCE_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND = static_cast<int>(0x8055290E);
static constexpr int SCE_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND = static_cast<int>(0x80552910);
static constexpr int SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_MAX = static_cast<int>(0x8055291A);

namespace {

struct LibraryContext {
    int libHttpCtxId;
    std::size_t poolSize;
    std::unordered_set<int> handles;
    std::unordered_set<int> pushEventFilters;
};

std::mutex contextsMutex;
std::unordered_map<int, LibraryContext> contexts;
int lastContext = 0;
int lastObject = 0;

int nextObject() {
    lastObject = lastObject % 0xefffffff + 1;
    return lastObject;
}

}

extern "C" {

int APS5_VABI sceNpWebApiAbortRequest() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiCreateContext() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiCreateHandle(int libCtxId) {
    NetBridgeCall netCall("libSceNpWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_ctx_id", libCtxId); });
    std::lock_guard lock(contextsMutex);
    const auto context = contexts.find(libCtxId);
    if (context == contexts.end()) return netCall.Ret(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND);
    const auto handle = nextObject();
    context->second.handles.insert(handle);
    return netCall.Ret(handle);
}

int APS5_VABI sceNpWebApiCreatePushEventFilter(int libCtxId, const void* filterParam, std::size_t filterParamNum) {
    NetBridgeCall netCall("libSceNpWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_ctx_id", libCtxId); netIn.Uint("filter_param_num", filterParamNum); });
    if (!filterParam || filterParamNum == 0) return netCall.Ret(SCE_NP_WEBAPI_ERROR_INVALID_ARGUMENT);
    std::lock_guard lock(contextsMutex);
    const auto context = contexts.find(libCtxId);
    if (context == contexts.end()) return netCall.Ret(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND);
    const auto filter = nextObject();
    context->second.pushEventFilters.insert(filter);
    return netCall.Ret(filter);
}

int APS5_VABI sceNpWebApiCreateRequest() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiCreateServicePushEventFilter(int libCtxId, int handleId, const char* npServiceName, std::uint32_t npServiceLabel, const void* filterParam, std::size_t filterParamNum) {
    NetBridgeCall netCall("libSceNpWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_ctx_id", libCtxId); netIn.Int("handle_id", handleId); netIn.Uint("np_service_label", npServiceLabel); netIn.Uint("filter_param_num", filterParamNum); });
    (void)npServiceLabel;
    if (!npServiceName || !filterParam || filterParamNum == 0) return netCall.Ret(SCE_NP_WEBAPI_ERROR_INVALID_ARGUMENT);
    std::lock_guard lock(contextsMutex);
    const auto context = contexts.find(libCtxId);
    if (context == contexts.end()) return netCall.Ret(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND);
    if (!context->second.handles.contains(handleId)) return netCall.Ret(SCE_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND);
}

int APS5_VABI sceNpWebApiDeleteContext() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiDeleteHandle(int libCtxId, int handleId) {
    NetBridgeCall netCall("libSceNpWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_ctx_id", libCtxId); netIn.Int("handle_id", handleId); });
    std::lock_guard lock(contextsMutex);
    const auto context = contexts.find(libCtxId);
    if (context == contexts.end()) return netCall.Ret(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND);
    return netCall.Ret(context->second.handles.erase(handleId) != 0 ? 0 : SCE_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND);
}

int APS5_VABI sceNpWebApiDeletePushEventFilter(int libCtxId, int filterId) {
    NetBridgeCall netCall("libSceNpWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_ctx_id", libCtxId); netIn.Int("filter_id", filterId); });
    std::lock_guard lock(contextsMutex);
    const auto context = contexts.find(libCtxId);
    if (context == contexts.end()) return netCall.Ret(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND);
    return netCall.Ret(context->second.pushEventFilters.erase(filterId) != 0 ? 0 : SCE_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND);
}

int APS5_VABI sceNpWebApiDeleteRequest() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiDeleteServicePushEventFilter(int libCtxId, int filterId) {
    NetBridgeCall netCall("libSceNpWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_ctx_id", libCtxId); netIn.Int("filter_id", filterId); });
    (void)filterId;
    std::lock_guard lock(contextsMutex);
    if (!contexts.contains(libCtxId)) return netCall.Ret(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND);
}

int APS5_VABI sceNpWebApiGetHttpResponseHeaderValue() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiGetHttpResponseHeaderValueLength() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiGetHttpStatusCode() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiInitialize(int libHttpCtxId, std::size_t poolSize) {
    NetBridgeCall netCall("libSceNpWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_http_ctx_id", libHttpCtxId); netIn.Uint("pool_size", poolSize); });
    std::lock_guard lock(contextsMutex);
    for (int attempt = 0; attempt < 0x7fff; ++attempt) {
        lastContext = lastContext % 0x7fff + 1;
        if (contexts.contains(lastContext)) continue;
        contexts[lastContext] = {libHttpCtxId, poolSize, {}, {}};
        return netCall.Ret(lastContext);
    }
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_MAX);
}

int APS5_VABI sceNpWebApiReadData() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiRegisterPushEventCallback() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiRegisterServicePushEventCallback() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiSendRequest() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiTerminate(int libCtxId) {
    NetBridgeCall netCall("libSceNpWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_ctx_id", libCtxId); });
    std::lock_guard lock(contextsMutex);
    return netCall.Ret(contexts.erase(libCtxId) != 0 ? 0 : SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND);
}

int APS5_VABI sceNpWebApiUnregisterPushEventCallback() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiUnregisterServicePushEventCallback() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpWebApiUtilityParseNpId() {
    NetBridgeCall netCall("libSceNpWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

}
