#include "prx/libSceHttp2/include/Http2.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/Equeue/Equeue.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include <memory>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// No network is emulated: contexts, templates and requests can be created, but any request
// that would touch the network fails with the library's network error.
static constexpr int ERROR_NETWORK = static_cast<int>(0x80436063);
static constexpr int ERROR_INVALID_ID = static_cast<int>(0x817B1100);
static constexpr int ERROR_NULL_POINTER = static_cast<int>(0x817B1225);

enum class Http2Kind : std::uint8_t { Context, Template, Request, CookieBox };

struct Http2Handle {
    Http2Kind kind;
    int owner;
    std::size_t poolSize;
    std::shared_ptr<std::byte[]> cookieMemory;
};

static std::atomic<int> g_nextHandle{1};
static std::mutex g_handlesMutex;
static std::unordered_map<int, Http2Handle> g_handles;

static Http2Handle* FindHandle(int id, Http2Kind kind) {
    const auto entry = g_handles.find(id);
    return entry != g_handles.end() && entry->second.kind == kind ? &entry->second : nullptr;
}

static Http2Handle* FindAnyHandle(int id) {
    const auto entry = g_handles.find(id);
    return entry != g_handles.end() ? &entry->second : nullptr;
}

static std::vector<int> ReleaseOwnedHandles(int owner) {
    std::vector<int> owned;
    for (const auto& entry : g_handles) {
        if (entry.second.owner == owner) owned.push_back(entry.first);
    }
    std::vector<int> removed;
    for (const int id : owned) {
        const std::vector<int> descendants = ReleaseOwnedHandles(id);
        removed.insert(removed.end(), descendants.begin(), descendants.end());
        g_handles.erase(id);
        removed.push_back(id);
    }
    return removed;
}

static bool HandleIsLive(int id) {
    std::lock_guard lock(g_handlesMutex);
    return FindAnyHandle(id) != nullptr;
}

static std::mutex g_completionsMutex;
static std::unordered_map<int, std::deque<Http2AsyncResult>> g_completions;

static void EraseCompletions(const std::vector<int>& ids) {
    std::lock_guard lock(g_completionsMutex);
    for (const int id : ids) g_completions.erase(id);
}

static int CompleteAsync(const char* function, int req_id, const Http2AsyncOption* kqueue_option, const void* option) {
    if (option != nullptr) NotImplemented_nid_no_patch(function);
    bool queued = false;
    {
        std::lock_guard handlesLock(g_handlesMutex);
        if (FindHandle(req_id, Http2Kind::Request) != nullptr) {
            std::lock_guard completionsLock(g_completionsMutex);
            Http2AsyncResult completion{};
            completion.req_id = req_id;
            completion.result = ERROR_NETWORK;
            g_completions[req_id].push_back(completion);
            queued = true;
        }
    }
    if (!queued) return ERROR_INVALID_ID;
    if (kqueue_option == nullptr) return 0;
    if (EqueueTriggerEvent_nid_postfix(kqueue_option->equeue, static_cast<uintptr_t>(kqueue_option->user_event_id), EVFILT_USER, kqueue_option->user_data) != 0) {
        {
            std::lock_guard lock(g_completionsMutex);
            const auto pending = g_completions.find(req_id);
            if (pending != g_completions.end() && !pending->second.empty()) pending->second.pop_back();
        }
        NotImplemented_nid_no_patch(function);
    }
    return 0;
}

extern "C" {

int APS5_VABI sceHttp2AddRequestHeader(int id, const char* name, const char* value, uint32_t mode) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("mode", mode); });
    (void)id;
    (void)name;
    (void)value;
    (void)mode;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2CreateRequestWithURL(int tmpl_id, const char* method, const char* url, uint64_t content_length) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("tmpl_id", tmpl_id); netIn.Text("method", method); netIn.Text("url", url); netIn.Uint("content_length", content_length); });
    (void)method;
    (void)url;
    (void)content_length;
    std::lock_guard lock(g_handlesMutex);
    if (FindHandle(tmpl_id, Http2Kind::Template) == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    const int id = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    g_handles[id] = {Http2Kind::Request, tmpl_id, 0};
    return netCall.Ret(id);
}

int APS5_VABI sceHttp2CreateTemplate(int lib_http2_ctx_id, const char* user_agent, int http_ver, int is_auto_proxy_conf) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_http2_ctx_id", lib_http2_ctx_id); netIn.Int("http_ver", http_ver); netIn.Int("is_auto_proxy_conf", is_auto_proxy_conf); });
    (void)user_agent;
    (void)http_ver;
    (void)is_auto_proxy_conf;
    std::lock_guard lock(g_handlesMutex);
    if (FindHandle(lib_http2_ctx_id, Http2Kind::Context) == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    const int id = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    g_handles[id] = {Http2Kind::Template, lib_http2_ctx_id, 0};
    return netCall.Ret(id);
}

int APS5_VABI sceHttp2DeleteRequest(int req_id) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    {
        std::lock_guard lock(g_handlesMutex);
        if (FindHandle(req_id, Http2Kind::Request) == nullptr) return netCall.Ret(ERROR_INVALID_ID);
        g_handles.erase(req_id);
    }
    EraseCompletions({req_id});
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2DeleteTemplate(int tmpl_id) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("tmpl_id", tmpl_id); });
    std::vector<int> removed;
    {
        std::lock_guard lock(g_handlesMutex);
        if (FindHandle(tmpl_id, Http2Kind::Template) == nullptr) return netCall.Ret(ERROR_INVALID_ID);
        removed = ReleaseOwnedHandles(tmpl_id);
        g_handles.erase(tmpl_id);
    }
    EraseCompletions(removed);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetAllResponseHeaders(int req_id, char** header, size_t* header_size) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    (void)req_id;
    (void)header;
    (void)header_size;
    return netCall.Ret(ERROR_NETWORK);
}

int APS5_VABI sceHttp2GetResponseContentLength(int req_id, int* result, uint64_t* content_length) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    (void)req_id;
    (void)result;
    (void)content_length;
    return netCall.Ret(ERROR_NETWORK);
}

int APS5_VABI sceHttp2GetStatusCode(int req_id, int* status_code) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    (void)req_id;
    (void)status_code;
    return netCall.Ret(ERROR_NETWORK);
}

int APS5_VABI sceHttp2Init(int libnet_mem_id, int libssl_ctx_id, size_t pool_size, int max_concurrent_request) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("libnet_mem_id", libnet_mem_id); netIn.Int("libssl_ctx_id", libssl_ctx_id); netIn.Uint("pool_size", pool_size); netIn.Int("max_concurrent_request", max_concurrent_request); });
    (void)libnet_mem_id;
    (void)libssl_ctx_id;
    (void)max_concurrent_request;
    const int id = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard lock(g_handlesMutex);
    g_handles[id] = {Http2Kind::Context, 0, pool_size};
    return netCall.Ret(id);
}

int APS5_VABI sceHttp2ReadData(int req_id, void* data, size_t size) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Uint("size", size); });
    (void)req_id;
    (void)data;
    (void)size;
    return netCall.Ret(ERROR_NETWORK);
}

int APS5_VABI sceHttp2ReadDataAsync(int req_id, void* data, size_t size, void* kqueue_option, void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Uint("size", size); });
    (void)req_id;
    (void)data;
    (void)size;
    (void)kqueue_option;
    (void)option;
    return netCall.Ret(ERROR_NETWORK);
}

int APS5_VABI sceHttp2SendRequest(int req_id, const void* post_data, size_t size) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Uint("size", size); });
    (void)req_id;
    (void)post_data;
    (void)size;
    return netCall.Ret(ERROR_NETWORK);
}

int APS5_VABI sceHttp2SendRequestAsync(int req_id, const void* post_data, size_t size, Http2AsyncOption* kqueue_option, void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Uint("size", size); });
    (void)post_data;
    (void)size;
    return netCall.Ret(CompleteAsync(__func__, req_id, kqueue_option, option));
}

int APS5_VABI sceHttp2SetAuthEnabled(int id, int is_enable) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Int("is_enable", is_enable); });
    (void)id;
    (void)is_enable;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetAutoRedirect(int id, int enable) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Int("enable", enable); });
    (void)id;
    (void)enable;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetConnectionWaitTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    (void)id;
    (void)usec;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetConnectTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    (void)id;
    (void)usec;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetInflateGZIPEnabled(int id, int enable) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Int("enable", enable); });
    (void)id;
    (void)enable;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetRecvTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    (void)id;
    (void)usec;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetRedirectCallback(int id, void* cb_func, void* user_arg) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); });
    (void)id;
    (void)cb_func;
    (void)user_arg;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetRequestContentLength(int id, uint64_t content_length) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("content_length", content_length); });
    (void)id;
    (void)content_length;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetResolveTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    (void)id;
    (void)usec;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetSendTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    (void)id;
    (void)usec;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetSslCallback(int id, void* cb_func, void* user_arg) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); });
    (void)id;
    (void)cb_func;
    (void)user_arg;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    (void)id;
    (void)usec;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SslDisableOption(int id, uint32_t ssl_flags) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("ssl_flags", ssl_flags); });
    (void)ssl_flags;
    return netCall.Ret(HandleIsLive(id) ? 0 : ERROR_INVALID_ID);
}

int APS5_VABI sceHttp2SslEnableOption(int id, uint32_t ssl_flags) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("ssl_flags", ssl_flags); });
    (void)ssl_flags;
    return netCall.Ret(HandleIsLive(id) ? 0 : ERROR_INVALID_ID);
}

int APS5_VABI sceHttp2SetMinSslVersion(int id, uint32_t ssl_version) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("ssl_version", ssl_version); });
    (void)ssl_version;
    return netCall.Ret(HandleIsLive(id) ? 0 : ERROR_INVALID_ID);
}

int APS5_VABI sceHttp2Term(int lib_http2_ctx_id) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_http2_ctx_id", lib_http2_ctx_id); });
    std::vector<int> removed;
    {
        std::lock_guard lock(g_handlesMutex);
        if (FindHandle(lib_http2_ctx_id, Http2Kind::Context) == nullptr) return netCall.Ret(ERROR_INVALID_ID);
        removed = ReleaseOwnedHandles(lib_http2_ctx_id);
        g_handles.erase(lib_http2_ctx_id);
    }
    EraseCompletions(removed);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WaitAsync(int req_id, Http2AsyncResult* result, uint32_t* timeout, void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    (void)timeout;
    if (result == nullptr || option != nullptr) NotImplemented_nid_no_patch(__func__);
    std::lock_guard lock(g_completionsMutex);
    const auto pending = g_completions.find(req_id);
    if (pending == g_completions.end() || pending->second.empty()) NotImplemented_nid_no_patch("sceHttp2WaitAsync: waiting for an operation that has not completed");
    *result = pending->second.front();
    pending->second.pop_front();
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2AbortRequest(int req_id) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    (void)req_id;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetMemoryPoolStats(int lib_http2_ctx_id, Http2MemoryPoolStats* stats) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_http2_ctx_id", lib_http2_ctx_id); });
    if (stats == nullptr) return netCall.Ret(ERROR_NULL_POINTER);
    std::lock_guard lock(g_handlesMutex);
    const Http2Handle* handle = FindHandle(lib_http2_ctx_id, Http2Kind::Context);
    if (handle == nullptr) {
        std::memset(stats, 0, sizeof(*stats));
        return netCall.Ret(ERROR_INVALID_ID);
    }
    *stats = {handle->poolSize, 0, 0, 0};
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2CookieFlush(int id) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); });
    (void)id;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2CreateCookieBox(std::size_t poolSize) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Uint("pool_size", poolSize); });
    if (poolSize < 0x10000) return netCall.Ret(static_cast<int>(0x817b121du));
    if (poolSize > std::numeric_limits<std::size_t>::max() - 0x3fff) throw std::overflow_error("sceHttp2CreateCookieBox: pool size overflow");
    const auto allocationSize = (poolSize + 0x3fff) & ~std::size_t{0x3fff};
    auto memory = std::make_shared<std::byte[]>(allocationSize);
    std::lock_guard lock(g_handlesMutex);
    const int id = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    g_handles[id] = {Http2Kind::CookieBox, 0, poolSize, std::move(memory)};
    return netCall.Ret(id);
}

int APS5_VABI sceHttp2DeleteCookieBox(int cookie_box_id) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("cookie_box_id", cookie_box_id); });
    std::lock_guard lock(g_handlesMutex);
    if (FindHandle(cookie_box_id, Http2Kind::CookieBox) == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    g_handles.erase(cookie_box_id);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetCookieBox(int id, int cookie_box_id) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Int("cookie_box_id", cookie_box_id); });
    (void)id;
    (void)cookie_box_id;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetRequestNoContentLength(int id) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); });
    (void)id;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetResolveRetry(int id, int32_t retry) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Int("retry", retry); });
    (void)id;
    (void)retry;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketCreateRequest(std::int32_t templateId, const char* url, Http2WebSocketTextCallback textCallback, Http2WebSocketDataCallback dataCallback, Http2WebSocketCloseCallback closeCallback, Http2WebSocketErrorCallback errorCallback, void* receiveBuffer, std::size_t receiveBufferSize, void* userData, const void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("template_id", templateId); netIn.Text("url", url); netIn.Uint("receive_buffer_size", receiveBufferSize); });
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketCloseAsync(std::int32_t requestId, const std::uint16_t* status, const char* reason, std::uint8_t flags, const Http2AsyncOption* asyncOption, const void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("request_id", requestId); netIn.Uint("flags", flags); });
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketSendTextMessageAsync(std::int32_t requestId, const char* text, std::uint8_t flags, const Http2AsyncOption* asyncOption, const void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("request_id", requestId); netIn.Uint("flags", flags); });
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketSendDataMessageAsync(std::int32_t requestId, const void* data, std::size_t dataSize, std::uint8_t flags, const Http2AsyncOption* asyncOption, const void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("request_id", requestId); netIn.Uint("data_size", dataSize); netIn.Uint("flags", flags); });
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketSetPingInterval(std::int32_t requestId, std::uint32_t intervalMicroseconds) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("request_id", requestId); netIn.Uint("interval_microseconds", intervalMicroseconds); });
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketSetPingTimeout(std::int32_t requestId, std::uint32_t timeoutMicroseconds) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("request_id", requestId); netIn.Uint("timeout_microseconds", timeoutMicroseconds); });
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}
int APS5_VABI sceHttp2AddCookie(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2AuthCacheFlush(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2CookieExport(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2CookieImport(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetAuthEnabled(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetAutoRedirect(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetCookie(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetCookieBox(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetCookieStats(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2RedirectCacheFlush(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2RemoveRequestHeader(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetAuthInfoCallback(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetCookieMaxNum(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetCookieMaxNumPerDomain(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetCookieMaxSize(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetCookieRecvCallback(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetCookieSendCallback(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetPreSendCallback(void) {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

}
