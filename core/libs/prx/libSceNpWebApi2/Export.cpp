#include "prx/libSceNpWebApi2/include/Api.hpp"
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include <algorithm>
#include <cctype>
#include <atomic>
#include <climits>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// The PSN web API has no service behind it: handles can be created, but requests fail.
static constexpr int SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80553402);
static constexpr int SCE_NP_WEBAPI2_ERROR_UNAVAILABLE = static_cast<int>(0x80553406);
static constexpr int SCE_NP_WEBAPI2_ERROR_INVALID_CONTEXT = static_cast<int>(0x80553404);
static constexpr int SCE_NP_WEBAPI2_ERROR_INVALID_HANDLE = static_cast<int>(0x8055340D);
static std::atomic<int> g_nextHandle{1};

static_assert(sizeof(NpWebApi2PushContextId) == 37);

static_assert(sizeof(NpWebApi2MemoryPoolStats) == 32);

static std::mutex g_poolsMutex;
static std::unordered_map<int, size_t> g_pools;

static int CreateLibraryContext(size_t poolSize) {
    const int libCtxId = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard lock(g_poolsMutex);
    g_pools[libCtxId] = poolSize;
    return libCtxId;
}

struct PushHandle {
    std::int32_t libraryContextId;
    std::uint32_t timeout = 0;
    bool aborted = false;
};

static std::unordered_map<int, PushHandle> pushHandles;

namespace {

constexpr char WEB_API_LIBRARY[] = "libSceNpWebApi2";
constexpr std::size_t MAX_TRACKED_USER_CONTEXTS = 256;
constexpr std::size_t MAX_TRACKED_REQUESTS = 256;
constexpr std::size_t MAX_REQUEST_HEADERS = 64;
constexpr std::size_t MAX_WIRE_STRING = 0xFFFF;
constexpr std::uint64_t MAX_REQUEST_BODY = 8ull * 1024 * 1024;
constexpr std::uint64_t MAX_RETAINED_REPLY_BYTES = 64ull * 1024 * 1024;

struct PushCallbackRegistration {
    int callbackId;
    int userContextId;
    int filterId;
    void* callback;
    void* userArg;
};

struct QueuedPushEvent {
    int userContextId;
    int filterId;
    std::string dataType;
    std::vector<std::uint8_t> data;
    std::vector<std::pair<std::string, std::string>> pairs;
    void* callback;
    void* userArg;
};

std::mutex g_pushCallbackMutex;
std::unordered_map<int, PushCallbackRegistration> g_pushCallbacks;
std::vector<QueuedPushEvent> g_pendingPushEvents;
int g_bridgePushRegistrationId = 0;
int g_bridgePushPumpId = 0;

struct TrackedRequest {
    bool userKnown = false;
    bool complete = true;
    int userId = 0;
    std::string apiGroup;
    std::string path;
    std::string method;
    std::vector<std::pair<std::string, std::string>> headers;
    bool hasReply = false;
    NetBridgeWebReplyView reply{};
    std::size_t readOffset = 0;
};

std::mutex g_webMutex;
std::unordered_map<int, int> g_userContextUsers;
std::unordered_map<std::int64_t, TrackedRequest> g_trackedRequests;
std::uint64_t g_retainedReplyBytes = 0;

bool Tracking() {
    return NetBridgeEnabled_nid_no_patch() != 0 || NetBridgeObserveEnabled_nid_no_patch() != 0;
}

bool AssignWireString(const char* source, std::string& target) {
    if (source == nullptr) return false;
    std::size_t length = 0;
    while (source[length] != '\0' && length <= MAX_WIRE_STRING) ++length;
    if (length > MAX_WIRE_STRING) return false;
    target.assign(source, length);
    return true;
}

void ReleaseReply(TrackedRequest& request) {
    if (!request.hasReply) return;
    g_retainedReplyBytes -= request.reply.bodySize;
    NetBridgeWebReplyRelease_nid_no_patch(&request.reply);
    request.hasReply = false;
    request.readOffset = 0;
}

bool SameHeaderName(const char* left, const char* right) {
    for (; *left != '\0' && *right != '\0'; ++left, ++right) {
        const auto a = static_cast<unsigned char>(*left);
        const auto b = static_cast<unsigned char>(*right);
        if (std::tolower(a) != std::tolower(b)) return false;
    }
    return *left == *right;
}

const char* FindReplyHeader(const TrackedRequest& request, const char* name) {
    for (std::uint32_t index = 0; index < request.reply.headerCount; ++index) {
        if (SameHeaderName(request.reply.headerNames[index], name)) return request.reply.headerValues[index];
    }
    return nullptr;
}

void DeliverPendingPushEvents() {
    std::vector<QueuedPushEvent> events;
    {
        std::lock_guard lock(g_pushCallbackMutex);
        if (g_pendingPushEvents.empty()) return;
        events.swap(g_pendingPushEvents);
    }
    for (const auto& event : events) {
        NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2PushEventDeliver", false, 0, [&](NetBridgeJson& in, NetBridgeJson&) {
            in.Int("user_context_id", event.userContextId).Int("filter_id", event.filterId).Text("data_type", event.dataType.c_str());
            in.Uint("data_size", event.data.size());
            if (!event.pairs.empty()) {
                in.Begin("pairs");
                for (const auto& pair : event.pairs) in.Text(pair.first, pair.second);
                in.End();
            }
        });
    }
}

void OnBridgePushPump(void*) {
    DeliverPendingPushEvents();
}

void OnBridgePushEvent(const NetBridgePushEventView* event, void*) {
    if (event == nullptr) return;
    std::lock_guard lock(g_pushCallbackMutex);
    std::lock_guard webLock(g_webMutex);
    for (const auto& [id, reg] : g_pushCallbacks) {
        if (reg.filterId == static_cast<int>(event->filter)) {
            int regUser = -1;
            const auto userIt = g_userContextUsers.find(reg.userContextId);
            if (userIt != g_userContextUsers.end()) regUser = userIt->second;
            if (regUser != -1 && (event->userId == -1 || event->userId == regUser)) {
                QueuedPushEvent queued;
                queued.userContextId = reg.userContextId;
                queued.filterId = reg.filterId;
                queued.dataType = event->dataType != nullptr ? event->dataType : "";
                if (event->data != nullptr && event->dataSize > 0) {
                    const auto* bytes = static_cast<const std::uint8_t*>(event->data);
                    queued.data.assign(bytes, bytes + event->dataSize);
                }
                if (event->extendedCount > 0 && event->extendedNames != nullptr && event->extendedValues != nullptr) {
                    queued.pairs.reserve(event->extendedCount);
                    for (std::uint32_t i = 0; i < event->extendedCount; ++i) {
                        queued.pairs.emplace_back(
                            event->extendedNames[i] != nullptr ? event->extendedNames[i] : "",
                            event->extendedValues[i] != nullptr ? event->extendedValues[i] : ""
                        );
                    }
                }
                queued.callback = reg.callback;
                queued.userArg = reg.userArg;
                g_pendingPushEvents.push_back(std::move(queued));
            }
        }
    }
}

void EnsureBridgePushHandler() {
    if (g_bridgePushRegistrationId != 0 || NetBridgeEnabled_nid_no_patch() == 0) return;
    g_bridgePushRegistrationId = NetBridgeRegisterPushHandler_nid_no_patch(OnBridgePushEvent, nullptr);
    g_bridgePushPumpId = NetBridgeRegisterPushPump_nid_no_patch(OnBridgePushPump, nullptr);
}

}

extern "C" {

int APS5_VABI sceNpWebApi2AbortRequest(int64_t request_id) {
    (void)request_id;
    return 0;
}

int APS5_VABI sceNpWebApi2AddHttpRequestHeader(int64_t request_id, const char* field_name, const char* value) {
    int ret = 0;
    if (Tracking()) {
        const bool bridge = NetBridgeEnabled_nid_no_patch() != 0;
        std::lock_guard lock(g_webMutex);
        const auto found = g_trackedRequests.find(request_id);
        if (found != g_trackedRequests.end()) {
            std::string name;
            std::string content;
            if (field_name == nullptr || value == nullptr) {
                if (bridge) ret = SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
            } else if (!AssignWireString(field_name, name) || !AssignWireString(value, content) || found->second.headers.size() >= MAX_REQUEST_HEADERS) {
                found->second.complete = false;
            } else {
                found->second.headers.emplace_back(std::move(name), std::move(content));
            }
        }
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2AddHttpRequestHeader", true, ret, [&](NetBridgeJson& in, NetBridgeJson&) {
        in.Int("request_id", request_id).Text("field_name", field_name).Text("value", value);
    });
    return ret;
}

int APS5_VABI sceNpWebApi2AddWebTraceTag(std::int64_t requestId, const char* value) {
    (void)requestId;
    if (!value) return SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    return 0;
}

void APS5_VABI sceNpWebApi2CheckTimeout(void) {
    DeliverPendingPushEvents();
}

int APS5_VABI sceNpWebApi2CreateRequest(int user_context_id, const char* api_group, const char* path, const char* method, const void* content_parameter, int64_t* request_id) {
    (void)content_parameter;
    if (!request_id) return SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    *request_id = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    if (Tracking()) {
        TrackedRequest entry;
        entry.complete = AssignWireString(api_group, entry.apiGroup) && AssignWireString(path, entry.path) && AssignWireString(method, entry.method);
        std::lock_guard lock(g_webMutex);
        const auto user = g_userContextUsers.find(user_context_id);
        if (user != g_userContextUsers.end()) {
            entry.userKnown = true;
            entry.userId = user->second;
        }
        if (g_trackedRequests.size() < MAX_TRACKED_REQUESTS) g_trackedRequests.emplace(*request_id, std::move(entry));
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2CreateRequest", true, 0, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("user_context_id", user_context_id).Text("api_group", api_group).Text("path", path).Text("method", method);
        out.Int("request_id", *request_id);
    });
    return 0;
}

int APS5_VABI sceNpWebApi2CreateUserContext(int lib_ctx_id, int user_id) {
    const int userContextId = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    if (Tracking()) {
        std::lock_guard lock(g_webMutex);
        if (g_userContextUsers.size() < MAX_TRACKED_USER_CONTEXTS) g_userContextUsers[userContextId] = user_id;
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2CreateUserContext", true, userContextId, [&](NetBridgeJson& in, NetBridgeJson&) {
        in.Int("lib_ctx_id", lib_ctx_id).Int("user_id", user_id);
    });
    return userContextId;
}

int APS5_VABI sceNpWebApi2DeleteRequest(int64_t request_id) {
    {
        std::lock_guard lock(g_webMutex);
        const auto found = g_trackedRequests.find(request_id);
        if (found != g_trackedRequests.end()) {
            ReleaseReply(found->second);
            g_trackedRequests.erase(found);
        }
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2DeleteRequest", true, 0, [&](NetBridgeJson& in, NetBridgeJson&) { in.Int("request_id", request_id); });
    return 0;
}

int APS5_VABI sceNpWebApi2DeleteUserContext(int user_context_id) {
    {
        std::lock_guard pushLock(g_pushCallbackMutex);
        std::lock_guard lock(g_webMutex);
        g_userContextUsers.erase(user_context_id);
        std::erase_if(g_pushCallbacks, [user_context_id](const auto& item) {
            return item.second.userContextId == user_context_id;
        });
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2DeleteUserContext", true, 0, [&](NetBridgeJson& in, NetBridgeJson&) { in.Int("user_context_id", user_context_id); });
    return 0;
}

int APS5_VABI sceNpWebApi2GetHttpResponseHeaderValue(int64_t request_id, const char* field_name, char* value, size_t value_size) {
    int ret = SCE_NP_WEBAPI2_ERROR_UNAVAILABLE;
    std::string found;
    if (NetBridgeEnabled_nid_no_patch() != 0) {
        std::lock_guard lock(g_webMutex);
        const auto request = g_trackedRequests.find(request_id);
        if (request != g_trackedRequests.end() && request->second.hasReply) {
            if (field_name == nullptr || value == nullptr) {
                ret = SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
            } else if (const char* header = FindReplyHeader(request->second, field_name)) {
                const std::size_t length = std::strlen(header);
                if (value_size < length) {
                    ret = SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
                } else {
                    std::memcpy(value, header, length);
                    if (value_size > length) value[length] = '\0';
                    found.assign(header, length);
                    ret = 0;
                }
            }
        }
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2GetHttpResponseHeaderValue", true, ret, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("request_id", request_id).Text("field_name", field_name).Uint("value_size", value_size);
        if (ret == 0) out.Text("value", found);
    });
    return ret;
}

int APS5_VABI sceNpWebApi2GetHttpResponseHeaderValueLength(int64_t request_id, const char* field_name, size_t* value_length) {
    int ret = SCE_NP_WEBAPI2_ERROR_UNAVAILABLE;
    std::size_t length = 0;
    if (NetBridgeEnabled_nid_no_patch() != 0) {
        std::lock_guard lock(g_webMutex);
        const auto request = g_trackedRequests.find(request_id);
        if (request != g_trackedRequests.end() && request->second.hasReply) {
            if (field_name == nullptr || value_length == nullptr) {
                ret = SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
            } else if (const char* header = FindReplyHeader(request->second, field_name)) {
                length = std::strlen(header);
                *value_length = length;
                ret = 0;
            }
        }
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2GetHttpResponseHeaderValueLength", true, ret, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("request_id", request_id).Text("field_name", field_name);
        if (ret == 0) out.Uint("value_length", length);
    });
    return ret;
}

int APS5_VABI sceNpWebApi2Initialize(int lib_http_ctx_id, size_t pool_size) {
    (void)lib_http_ctx_id;
    return CreateLibraryContext(pool_size);
}

int APS5_VABI sceNpWebApi2GetMemoryPoolStats(int libraryContextId, NpWebApi2MemoryPoolStats* stats) {
    std::lock_guard lock(g_poolsMutex);
    const auto pool = g_pools.find(libraryContextId);
    if (pool == g_pools.end()) return SCE_NP_WEBAPI2_ERROR_INVALID_CONTEXT;
    if (!stats) return 0;
    std::memset(stats, 0, sizeof(*stats));
    stats->pool_size = pool->second;
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventCreateFilter(int lib_ctx_id, int handle_id, const char* np_service_name, uint32_t np_service_label, const void* filter_param, size_t filter_param_num) {
    const int filterId = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    if (NetBridgeEnabled_nid_no_patch() != 0) NetBridgePushFilter_nid_no_patch(-1, static_cast<std::uint32_t>(filterId), nullptr, 0);
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2PushEventCreateFilter", true, filterId, [&](NetBridgeJson& in, NetBridgeJson&) {
        in.Int("lib_ctx_id", lib_ctx_id).Int("handle_id", handle_id).Text("np_service_name", np_service_name).Uint("np_service_label", np_service_label);
        in.Bool("filter_param_null", filter_param == nullptr).Uint("filter_param_num", filter_param_num);
    });
    return filterId;
}

int APS5_VABI sceNpWebApi2PushEventCreateHandle(int libraryContextId) {
    std::lock_guard lock(g_poolsMutex);
    if (!g_pools.contains(libraryContextId)) return SCE_NP_WEBAPI2_ERROR_INVALID_CONTEXT;
    const int handleId = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    pushHandles.emplace(handleId, PushHandle{libraryContextId});
    return handleId;
}

int APS5_VABI sceNpWebApi2PushEventAbortHandle(int libraryContextId, int handleId) {
    std::lock_guard lock(g_poolsMutex);
    if (!g_pools.contains(libraryContextId)) return SCE_NP_WEBAPI2_ERROR_INVALID_CONTEXT;
    const auto handle = pushHandles.find(handleId);
    if (handle == pushHandles.end() || handle->second.libraryContextId != libraryContextId) return SCE_NP_WEBAPI2_ERROR_INVALID_HANDLE;
    handle->second.aborted = true;
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventDeleteHandle(int libraryContextId, int handleId) {
    std::lock_guard lock(g_poolsMutex);
    if (!g_pools.contains(libraryContextId)) return SCE_NP_WEBAPI2_ERROR_INVALID_CONTEXT;
    const auto handle = pushHandles.find(handleId);
    if (handle == pushHandles.end() || handle->second.libraryContextId != libraryContextId) return SCE_NP_WEBAPI2_ERROR_INVALID_HANDLE;
    pushHandles.erase(handle);
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventDeletePushContext(int user_context_id, const void* push_context_id) {
    (void)user_context_id;
    (void)push_context_id;
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventRegisterCallback(int user_context_id, int filter_id, void* callback, void* user_arg) {
    const int callbackId = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard lock(g_pushCallbackMutex);
        EnsureBridgePushHandler();
        PushCallbackRegistration reg;
        reg.callbackId = callbackId;
        reg.userContextId = user_context_id;
        reg.filterId = filter_id;
        reg.callback = callback;
        reg.userArg = user_arg;
        g_pushCallbacks[callbackId] = std::move(reg);
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2PushEventRegisterCallback", true, callbackId, [&](NetBridgeJson& in, NetBridgeJson&) {
        in.Int("user_context_id", user_context_id).Int("filter_id", filter_id).Bool("callback_null", callback == nullptr);
    });
    return callbackId;
}

int APS5_VABI sceNpWebApi2ReadData(int64_t request_id, void* data, size_t size) {
    int ret = SCE_NP_WEBAPI2_ERROR_UNAVAILABLE;
    std::string received;
    if (NetBridgeEnabled_nid_no_patch() != 0) {
        std::lock_guard lock(g_webMutex);
        const auto request = g_trackedRequests.find(request_id);
        if (request != g_trackedRequests.end() && request->second.hasReply) {
            TrackedRequest& tracked = request->second;
            if (data == nullptr && size != 0) {
                ret = SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
            } else {
                const std::size_t remaining = static_cast<std::size_t>(tracked.reply.bodySize) - tracked.readOffset;
                const std::size_t count = std::min<std::size_t>({size, remaining, static_cast<std::size_t>(INT_MAX)});
                if (count != 0) std::memcpy(data, tracked.reply.body + tracked.readOffset, count);
                received.assign(reinterpret_cast<const char*>(tracked.reply.body) + tracked.readOffset, count);
                tracked.readOffset += count;
                ret = static_cast<int>(count);
            }
        }
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2ReadData", true, ret, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("request_id", request_id).Uint("size", size);
        if (ret >= 0) out.Bytes("data", received.data(), received.size());
    });
    return ret;
}

int APS5_VABI sceNpWebApi2SendRequest(int64_t request_id, const void* data, size_t data_size, NpWebApi2ResponseInformationOption* response_info_option) {
    int ret = SCE_NP_WEBAPI2_ERROR_UNAVAILABLE;
    std::uint32_t status = 0;
    std::uint64_t responseSize = 0;
    bool tracked = false;
    TrackedRequest snapshot;
    if (Tracking()) {
        std::lock_guard lock(g_webMutex);
        const auto found = g_trackedRequests.find(request_id);
        if (found != g_trackedRequests.end()) {
            tracked = true;
            snapshot.userKnown = found->second.userKnown;
            snapshot.complete = found->second.complete;
            snapshot.userId = found->second.userId;
            snapshot.apiGroup = found->second.apiGroup;
            snapshot.path = found->second.path;
            snapshot.method = found->second.method;
            snapshot.headers = found->second.headers;
        }
    }
    if (tracked && NetBridgeEnabled_nid_no_patch() != 0 && snapshot.userKnown && snapshot.complete && (data_size == 0 || data != nullptr) && data_size <= MAX_REQUEST_BODY) {
        std::vector<const char*> names;
        std::vector<const char*> values;
        for (const auto& header : snapshot.headers) {
            names.push_back(header.first.c_str());
            values.push_back(header.second.c_str());
        }
        NetBridgeWebRequestView request{};
        request.userId = snapshot.userId;
        request.apiGroup = snapshot.apiGroup.c_str();
        request.path = snapshot.path.c_str();
        request.method = snapshot.method.c_str();
        request.headerCount = static_cast<std::uint32_t>(names.size());
        request.headerNames = names.data();
        request.headerValues = values.data();
        request.body = data;
        request.bodySize = data_size;
        NetBridgeWebReplyView reply{};
        if (NetBridgeWebRequest_nid_no_patch(&request, &reply) != 0) {
            std::lock_guard lock(g_webMutex);
            const auto found = g_trackedRequests.find(request_id);
            if (found == g_trackedRequests.end()) {
                NetBridgeWebReplyRelease_nid_no_patch(&reply);
            } else {
                ReleaseReply(found->second);
                if (g_retainedReplyBytes + reply.bodySize > MAX_RETAINED_REPLY_BYTES) {
                    NetBridgeWebReplyRelease_nid_no_patch(&reply);
                } else {
                    found->second.reply = reply;
                    found->second.hasReply = true;
                    found->second.readOffset = 0;
                    g_retainedReplyBytes += reply.bodySize;
                    status = reply.status;
                    responseSize = reply.bodySize;
                    ret = 0;
                }
            }
        }
    }
    if (ret == 0 && response_info_option != nullptr) {
        response_info_option->http_status = static_cast<std::int32_t>(status);
        response_info_option->error_object = nullptr;
        response_info_option->error_object_size = 0;
        response_info_option->response_data_size = static_cast<std::size_t>(responseSize);
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2SendRequest", true, ret, [&](NetBridgeJson& in, NetBridgeJson& out) {
        in.Int("request_id", request_id);
        if (tracked) {
            if (snapshot.userKnown) in.Int("user_id", snapshot.userId);
            in.Text("api_group", snapshot.apiGroup).Text("path", snapshot.path).Text("method", snapshot.method);
            in.Begin("headers");
            for (const auto& header : snapshot.headers) in.Text(header.first, header.second);
            in.End();
        }
        if (data != nullptr) in.Bytes("body", data, data_size);
        out.Uint("status", status);
        if (ret == 0) out.Uint("response_data_size", responseSize);
    });
    return ret;
}

int APS5_VABI sceNpWebApi2Terminate(int lib_ctx_id) {
    std::lock_guard lock(g_poolsMutex);
    std::erase_if(pushHandles, [lib_ctx_id](const auto& item) { return item.second.libraryContextId == lib_ctx_id; });
    g_pools.erase(lib_ctx_id);
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventCreatePushContext(std::int32_t userContextId, NpWebApi2PushContextId* pushContextId) {
    return SCE_NP_WEBAPI2_ERROR_UNAVAILABLE;
}

int APS5_VABI sceNpWebApi2PushEventDeleteFilter(std::int32_t libraryContextId, std::int32_t filterId) {
    if (NetBridgeEnabled_nid_no_patch() != 0) NetBridgePushFilterGone_nid_no_patch(-1, static_cast<std::uint32_t>(filterId));
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2PushEventDeleteFilter", true, 0, [&](NetBridgeJson& in, NetBridgeJson&) {
        in.Int("lib_ctx_id", libraryContextId).Int("filter_id", filterId);
    });
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventSetHandleTimeout(int libraryContextId, int handleId, std::uint32_t timeout) {
    std::lock_guard lock(g_poolsMutex);
    if (!g_pools.contains(libraryContextId)) return SCE_NP_WEBAPI2_ERROR_INVALID_CONTEXT;
    const auto handle = pushHandles.find(handleId);
    if (handle == pushHandles.end() || handle->second.libraryContextId != libraryContextId) return SCE_NP_WEBAPI2_ERROR_INVALID_HANDLE;
    handle->second.timeout = timeout;
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventRegisterPushContextCallback(std::int32_t userContextId, std::int32_t filterId, void* callback, void* userArg) {
    return g_nextHandle.fetch_add(1, std::memory_order_relaxed);
}

int APS5_VABI sceNpWebApi2PushEventStartPushContextCallback(std::int32_t userContextId, const NpWebApi2PushContextId* pushContextId) {
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventUnregisterCallback(std::int32_t userContextId, std::int32_t callbackId) {
    {
        std::lock_guard lock(g_pushCallbackMutex);
        g_pushCallbacks.erase(callbackId);
    }
    NetBridgeRecord(WEB_API_LIBRARY, "sceNpWebApi2PushEventUnregisterCallback", true, 0, [&](NetBridgeJson& in, NetBridgeJson&) {
        in.Int("user_context_id", userContextId).Int("callback_id", callbackId);
    });
    return 0;
}

int APS5_VABI sceNpWebApi2PushEventUnregisterPushContextCallback(std::int32_t userContextId, std::int32_t callbackId) {
    return 0;
}

int APS5_VABI sceNpWebApi2SetRequestTimeout(int64_t request_id, uint32_t timeout) {
    (void)request_id;
    (void)timeout;
    return 0;
}

int APS5_VABI sceNpWebApi2AddMultipartPart() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpWebApi2CreateMultipartRequest(int user_ctx_id, const char* api_group, const char* path, const char* method, int64_t* request_id) {
    (void)user_ctx_id;
    if (!api_group || !path || !method) return SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    const int64_t created = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    if (request_id) *request_id = created;
    return 0;
}

int APS5_VABI sceNpWebApi2InitializeForPresence(int lib_http_ctx_id, uint64_t pool_size) {
    (void)lib_http_ctx_id;
    return CreateLibraryContext(pool_size);
}

int APS5_VABI sceNpWebApi2IntCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpWebApi2IntInitialize(const NpWebApi2IntInitializeArgs* args) {
    if (args == nullptr || args->struct_size != sizeof(NpWebApi2IntInitializeArgs)) return SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    return CreateLibraryContext(args->pool_size);
}

int APS5_VABI sceNpWebApi2IntInitialize2(const NpWebApi2IntInitialize2Args* args) {
    if (args == nullptr || args->struct_size != sizeof(NpWebApi2IntInitialize2Args)) return SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    return CreateLibraryContext(args->pool_size);
}

int APS5_VABI sceNpWebApi2IntPushEventCreateCtxIndFilter() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpWebApi2IntPushEventCreateDestPs4CtxIndFilter() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpWebApi2SendMultipartRequest(int64_t request_id, int part_index, void* data, uint64_t data_size, void* resp_info_option) {
    (void)request_id;
    (void)resp_info_option;
    if (part_index <= 0 || !data || data_size == 0) return SCE_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    return SCE_NP_WEBAPI2_ERROR_UNAVAILABLE;
}

int APS5_VABI sceNpWebApi2SetMultipartContentType() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
