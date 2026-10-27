#include "prx/libSceHttp2/include/Http2.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cctype>
#include <cstdlib>
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
#include <string>
#include <string_view>
#include <array>
#include <algorithm>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>
#ifdef _WIN32
#include "prx/libkernel/File/include/TrustedRoots.hpp"
#endif

static constexpr int ERROR_NETWORK = static_cast<int>(0x80436063);
static constexpr int ERROR_INVALID_ID = static_cast<int>(0x817B1100);
static constexpr int ERROR_NULL_POINTER = static_cast<int>(0x817B1225);

enum class Http2Kind : std::uint8_t { Context, Template, Request, CookieBox };

struct Http2Handle {
    Http2Kind kind;
    int owner{0};
    std::size_t poolSize{0};
    std::shared_ptr<std::byte[]> cookieMemory;

    std::string userAgent;
    uint32_t timeoutUsec{30000000};
    uint32_t connectTimeoutUsec{30000000};
    uint32_t recvTimeoutUsec{30000000};
    uint32_t sendTimeoutUsec{30000000};
    uint32_t resolveTimeoutUsec{30000000};

    std::string method{"GET"};
    std::string url;
    uint64_t requestContentLength{0};
    bool hasRequestContentLength{false};
    std::vector<std::pair<std::string, std::string>> headers;

    bool completed{false};
    int responseStatusCode{0};
    std::string responseHeadersRaw;
    std::vector<uint8_t> responseBody;
    uint64_t responseContentLength{0};
    bool hasResponseContentLength{false};
    bool isChunked{false};
    size_t bodyReadOffset{0};
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

struct ParsedUrl {
    bool isTls{false};
    std::string host;
    std::string port;
    std::string path;
};

static bool ParseUrl(const std::string& url, ParsedUrl& out) {
    std::string_view sv(url);
    if (sv.starts_with("https://")) {
        out.isTls = true;
        out.port = "443";
        sv.remove_prefix(8);
    } else if (sv.starts_with("http://")) {
        out.isTls = false;
        out.port = "80";
        sv.remove_prefix(7);
    } else {
        return false;
    }

    const auto slash = sv.find_first_of("/?#");
    std::string_view hostPart = (slash == std::string_view::npos) ? sv : sv.substr(0, slash);
    out.path = (slash == std::string_view::npos) ? "/" : std::string(sv.substr(slash));

    if (hostPart.empty()) return false;

    if (hostPart.front() == '[') {
        const auto closeBracket = hostPart.find(']');
        if (closeBracket == std::string_view::npos) return false;
        out.host = std::string(hostPart.substr(1, closeBracket - 1));
        if (closeBracket + 1 < hostPart.size() && hostPart[closeBracket + 1] == ':') {
            out.port = std::string(hostPart.substr(closeBracket + 2));
        }
    } else {
        const auto colon = hostPart.rfind(':');
        if (colon != std::string_view::npos) {
            out.host = std::string(hostPart.substr(0, colon));
            out.port = std::string(hostPart.substr(colon + 1));
        } else {
            out.host = std::string(hostPart);
        }
    }
    return !out.host.empty() && !out.port.empty();
}

static bool LoadHostCaCertificates(mbedtls_x509_crt* chain) {
#ifdef _WIN32
    const std::string pem = HostTrustedTlsRoots_nid_no_patch();
    if (pem.empty()) return false;
    return mbedtls_x509_crt_parse(chain, reinterpret_cast<const unsigned char*>(pem.c_str()), pem.size() + 1) == 0;
#else
    const std::array<const char*, 5> candidates{
        std::getenv("SSL_CERT_FILE"),
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/ca-bundle.pem",
        "/etc/ssl/cert.pem"
    };
    for (const char* path : candidates) {
        if (path == nullptr || *path == '\0') continue;
        if (mbedtls_x509_crt_parse_file(chain, path) == 0) return true;
    }
    return false;
#endif
}

static std::mutex g_caMutex;
static mbedtls_x509_crt g_caChain;
static bool g_caLoaded = false;
static bool g_caLoadSuccess = false;

static bool EnsureCaCertificatesLoaded() {
    std::lock_guard lock(g_caMutex);
    if (!g_caLoaded) {
        mbedtls_x509_crt_init(&g_caChain);
        g_caLoadSuccess = LoadHostCaCertificates(&g_caChain);
        g_caLoaded = true;
    }
    return g_caLoadSuccess;
}

static std::vector<uint8_t> DecodeChunked(const uint8_t* data, size_t size) {
    std::vector<uint8_t> body;
    size_t pos = 0;
    while (pos < size) {
        size_t eol = pos;
        while (eol + 1 < size && !(data[eol] == '\r' && data[eol + 1] == '\n')) ++eol;
        if (eol + 1 >= size) break;
        std::string hexLen(reinterpret_cast<const char*>(data + pos), eol - pos);
        const auto semi = hexLen.find(';');
        if (semi != std::string::npos) hexLen.resize(semi);
        size_t chunkSize = 0;
        try {
            chunkSize = std::stoul(hexLen, nullptr, 16);
        } catch (...) {
            break;
        }
        if (chunkSize == 0) break;
        pos = eol + 2;
        if (pos + chunkSize > size) {
            body.insert(body.end(), data + pos, data + size);
            break;
        }
        body.insert(body.end(), data + pos, data + pos + chunkSize);
        pos += chunkSize;
        if (pos + 1 < size && data[pos] == '\r' && data[pos + 1] == '\n') pos += 2;
    }
    return body;
}

struct HttpResponse {
    int statusCode{0};
    std::string headersRaw;
    std::vector<uint8_t> body;
    uint64_t contentLength{0};
    bool hasContentLength{false};
    bool isChunked{false};
};

static bool ParseHttpResponse(const std::vector<uint8_t>& rawData, HttpResponse& out) {
    if (rawData.empty()) return false;
    std::string_view data(reinterpret_cast<const char*>(rawData.data()), rawData.size());

    size_t headerEnd = data.find("\r\n\r\n");
    size_t bodyStart = 0;
    if (headerEnd != std::string_view::npos) {
        bodyStart = headerEnd + 4;
        out.headersRaw = std::string(data.substr(0, headerEnd + 2));
    } else {
        headerEnd = data.find("\n\n");
        if (headerEnd == std::string_view::npos) return false;
        bodyStart = headerEnd + 2;
        out.headersRaw = std::string(data.substr(0, headerEnd + 1));
    }

    const auto firstLineEnd = data.find_first_of("\r\n");
    if (firstLineEnd != std::string_view::npos) {
        const std::string_view statusLine = data.substr(0, firstLineEnd);
        const auto firstSpace = statusLine.find(' ');
        if (firstSpace != std::string_view::npos) {
            const auto secondSpace = statusLine.find(' ', firstSpace + 1);
            const auto codeStr = statusLine.substr(firstSpace + 1, (secondSpace == std::string_view::npos) ? (statusLine.size() - firstSpace - 1) : (secondSpace - firstSpace - 1));
            try {
                out.statusCode = std::stoi(std::string(codeStr));
            } catch (...) {
                out.statusCode = 0;
            }
        }
    }

    bool isChunked = false;
    std::string_view headerView = data.substr(0, headerEnd);
    size_t lineStart = 0;
    while (lineStart < headerView.size()) {
        const auto lineEnd = headerView.find("\r\n", lineStart);
        const auto curEnd = (lineEnd == std::string_view::npos) ? headerView.size() : lineEnd;
        const std::string_view line = headerView.substr(lineStart, curEnd - lineStart);
        const auto colon = line.find(':');
        if (colon != std::string_view::npos) {
            std::string headerName;
            for (char c : line.substr(0, colon)) headerName += static_cast<char>(std::tolower(c));
            std::string_view headerVal = line.substr(colon + 1);
            while (!headerVal.empty() && (headerVal.front() == ' ' || headerVal.front() == '\t')) headerVal.remove_prefix(1);
            while (!headerVal.empty() && (headerVal.back() == ' ' || headerVal.back() == '\t')) headerVal.remove_suffix(1);

            if (headerName == "transfer-encoding") {
                if (headerVal.find("chunked") != std::string_view::npos) {
                    isChunked = true;
                }
            } else if (headerName == "content-length") {
                try {
                    out.contentLength = std::stoull(std::string(headerVal));
                    out.hasContentLength = true;
                } catch (...) {
                }
            }
        }
        if (lineEnd == std::string_view::npos) break;
        lineStart = lineEnd + 2;
    }

    const uint8_t* bodyBytes = rawData.data() + bodyStart;
    const size_t bodyBytesCount = rawData.size() - bodyStart;

    if (isChunked) {
        out.isChunked = true;
        out.body = DecodeChunked(bodyBytes, bodyBytesCount);
        out.contentLength = out.body.size();
    } else {
        out.body.assign(bodyBytes, bodyBytes + bodyBytesCount);
        if (!out.hasContentLength) {
            out.contentLength = out.body.size();
        }
    }
    return out.statusCode != 0;
}

static std::string BuildHttpRequest(
    const std::string& method,
    const std::string& host,
    const std::string& port,
    bool isTls,
    const std::string& path,
    const std::string& userAgent,
    const std::vector<std::pair<std::string, std::string>>& headers,
    const void* postData,
    size_t postSize,
    bool hasRequestContentLength,
    uint64_t requestContentLength
) {
    std::string req;
    req.reserve(512 + postSize);
    req += method.empty() ? "GET" : method;
    req += ' ';
    req += path.empty() ? "/" : path;
    req += " HTTP/1.1\r\nHost: ";
    req += host;
    if ((isTls && port != "443") || (!isTls && port != "80")) {
        req += ':';
        req += port;
    }
    req += "\r\nUser-Agent: ";
    req += userAgent.empty() ? "libSceHttp2" : userAgent;
    req += "\r\nConnection: close\r\n";

    bool hasContentLengthHeader = false;
    for (const auto& [name, value] : headers) {
        if (name == "Content-Length" || name == "content-length") hasContentLengthHeader = true;
        req += name;
        req += ": ";
        req += value;
        req += "\r\n";
    }

    if (!hasContentLengthHeader) {
        if (postData != nullptr && postSize > 0) {
            req += "Content-Length: " + std::to_string(postSize) + "\r\n";
        } else if (hasRequestContentLength) {
            req += "Content-Length: " + std::to_string(requestContentLength) + "\r\n";
        }
    }
    req += "\r\n";
    if (postData != nullptr && postSize > 0) {
        req.append(reinterpret_cast<const char*>(postData), postSize);
    }
    return req;
}

static int ExecuteHttpRequest(
    const std::string& method,
    const std::string& url,
    const std::string& userAgent,
    uint32_t timeoutUsec,
    const std::vector<std::pair<std::string, std::string>>& headers,
    const void* postData,
    size_t postSize,
    bool hasRequestContentLength,
    uint64_t requestContentLength,
    HttpResponse& outResponse
) {
    ParsedUrl parsed;
    if (!ParseUrl(url, parsed)) return ERROR_NETWORK;

    mbedtls_net_context serverFd;
    mbedtls_net_init(&serverFd);

    int ret = mbedtls_net_connect(&serverFd, parsed.host.c_str(), parsed.port.c_str(), MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        mbedtls_net_free(&serverFd);
        return ERROR_NETWORK;
    }

    const std::string requestStr = BuildHttpRequest(
        method, parsed.host, parsed.port, parsed.isTls, parsed.path,
        userAgent, headers, postData, postSize,
        hasRequestContentLength, requestContentLength
    );

    std::vector<uint8_t> responseRaw;
    responseRaw.reserve(4096);

    if (parsed.isTls) {
        if (!EnsureCaCertificatesLoaded()) {
            mbedtls_net_free(&serverFd);
            return ERROR_NETWORK;
        }

        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context drbg;
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config conf;

        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&conf);
        mbedtls_ctr_drbg_init(&drbg);
        mbedtls_entropy_init(&entropy);

        const char* seed = "libSceHttp2";
        if (mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy,
                                  reinterpret_cast<const unsigned char*>(seed), std::strlen(seed)) != 0 ||
            mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
                                        MBEDTLS_SSL_TRANSPORT_STREAM,
                                        MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
            mbedtls_ssl_free(&ssl);
            mbedtls_ssl_config_free(&conf);
            mbedtls_ctr_drbg_free(&drbg);
            mbedtls_entropy_free(&entropy);
            mbedtls_net_free(&serverFd);
            return ERROR_NETWORK;
        }

        mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(&conf, &g_caChain, nullptr);
        mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &drbg);
        if (timeoutUsec > 0) {
            mbedtls_ssl_conf_read_timeout(&conf, timeoutUsec / 1000);
        }

        if (mbedtls_ssl_setup(&ssl, &conf) != 0 ||
            mbedtls_ssl_set_hostname(&ssl, parsed.host.c_str()) != 0) {
            mbedtls_ssl_free(&ssl);
            mbedtls_ssl_config_free(&conf);
            mbedtls_ctr_drbg_free(&drbg);
            mbedtls_entropy_free(&entropy);
            mbedtls_net_free(&serverFd);
            return ERROR_NETWORK;
        }

        mbedtls_ssl_set_bio(&ssl, &serverFd, mbedtls_net_send, mbedtls_net_recv,
                            (timeoutUsec > 0) ? mbedtls_net_recv_timeout : nullptr);

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                mbedtls_ssl_free(&ssl);
                mbedtls_ssl_config_free(&conf);
                mbedtls_ctr_drbg_free(&drbg);
                mbedtls_entropy_free(&entropy);
                mbedtls_net_free(&serverFd);
                return ERROR_NETWORK;
            }
        }

        if (mbedtls_ssl_get_verify_result(&ssl) != 0) {
            mbedtls_ssl_free(&ssl);
            mbedtls_ssl_config_free(&conf);
            mbedtls_ctr_drbg_free(&drbg);
            mbedtls_entropy_free(&entropy);
            mbedtls_net_free(&serverFd);
            return ERROR_NETWORK;
        }

        size_t written = 0;
        while (written < requestStr.size()) {
            ret = mbedtls_ssl_write(&ssl,
                reinterpret_cast<const unsigned char*>(requestStr.data() + written),
                requestStr.size() - written);
            if (ret <= 0) {
                if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
                mbedtls_ssl_free(&ssl);
                mbedtls_ssl_config_free(&conf);
                mbedtls_ctr_drbg_free(&drbg);
                mbedtls_entropy_free(&entropy);
                mbedtls_net_free(&serverFd);
                return ERROR_NETWORK;
            }
            written += static_cast<size_t>(ret);
        }

        unsigned char buf[4096];
        for (;;) {
            ret = mbedtls_ssl_read(&ssl, buf, sizeof(buf));
            if (ret > 0) {
                responseRaw.insert(responseRaw.end(), buf, buf + ret);
            } else if (ret == 0 || ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                break;
            } else if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
                continue;
            } else {
                break;
            }
        }

        mbedtls_ssl_close_notify(&ssl);
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&conf);
        mbedtls_ctr_drbg_free(&drbg);
        mbedtls_entropy_free(&entropy);
        mbedtls_net_free(&serverFd);
    } else {
        size_t written = 0;
        while (written < requestStr.size()) {
            ret = mbedtls_net_send(&serverFd,
                reinterpret_cast<const unsigned char*>(requestStr.data() + written),
                requestStr.size() - written);
            if (ret <= 0) {
                if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
                mbedtls_net_free(&serverFd);
                return ERROR_NETWORK;
            }
            written += static_cast<size_t>(ret);
        }

        unsigned char buf[4096];
        for (;;) {
            ret = mbedtls_net_recv(&serverFd, buf, sizeof(buf));
            if (ret > 0) {
                responseRaw.insert(responseRaw.end(), buf, buf + ret);
            } else {
                break;
            }
        }
        mbedtls_net_free(&serverFd);
    }

    if (!ParseHttpResponse(responseRaw, outResponse)) {
        return ERROR_NETWORK;
    }
    return 0;
}

extern "C" {

int APS5_VABI sceHttp2AddRequestHeader(int id, const char* name, const char* value, uint32_t mode) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("mode", mode); });
    (void)mode;
    if (name == nullptr || value == nullptr) return netCall.Ret(ERROR_NULL_POINTER);
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindAnyHandle(id);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->headers.emplace_back(name, value);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2CreateRequestWithURL(int tmpl_id, const char* method, const char* url, uint64_t content_length) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("tmpl_id", tmpl_id); netIn.Text("method", method); netIn.Text("url", url); netIn.Uint("content_length", content_length); });
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* tmplHandle = FindHandle(tmpl_id, Http2Kind::Template);
    if (tmplHandle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    const int id = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    Http2Handle req{};
    req.kind = Http2Kind::Request;
    req.owner = tmpl_id;
    req.userAgent = tmplHandle->userAgent;
    req.timeoutUsec = tmplHandle->timeoutUsec;
    req.connectTimeoutUsec = tmplHandle->connectTimeoutUsec;
    req.recvTimeoutUsec = tmplHandle->recvTimeoutUsec;
    req.sendTimeoutUsec = tmplHandle->sendTimeoutUsec;
    req.resolveTimeoutUsec = tmplHandle->resolveTimeoutUsec;
    req.method = method ? method : "GET";
    req.url = url ? url : "";
    req.requestContentLength = content_length;
    req.hasRequestContentLength = (content_length > 0);
    g_handles[id] = std::move(req);
    return netCall.Ret(id);
}

int APS5_VABI sceHttp2CreateTemplate(int lib_http2_ctx_id, const char* user_agent, int http_ver, int is_auto_proxy_conf) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("lib_http2_ctx_id", lib_http2_ctx_id); netIn.Int("http_ver", http_ver); netIn.Int("is_auto_proxy_conf", is_auto_proxy_conf); });
    (void)http_ver;
    (void)is_auto_proxy_conf;
    std::lock_guard lock(g_handlesMutex);
    if (FindHandle(lib_http2_ctx_id, Http2Kind::Context) == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    const int id = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    Http2Handle tmpl{};
    tmpl.kind = Http2Kind::Template;
    tmpl.owner = lib_http2_ctx_id;
    if (user_agent != nullptr) tmpl.userAgent = user_agent;
    g_handles[id] = std::move(tmpl);
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
    if (header == nullptr || header_size == nullptr) return netCall.Ret(ERROR_NULL_POINTER);
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* req = FindHandle(req_id, Http2Kind::Request);
    if (req == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    if (!req->completed) return netCall.Ret(ERROR_NETWORK);
    *header = const_cast<char*>(req->responseHeadersRaw.c_str());
    *header_size = req->responseHeadersRaw.size();
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetResponseContentLength(int req_id, int* result, uint64_t* content_length) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    if (result == nullptr || content_length == nullptr) return netCall.Ret(ERROR_NULL_POINTER);
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* req = FindHandle(req_id, Http2Kind::Request);
    if (req == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    if (!req->completed) return netCall.Ret(ERROR_NETWORK);
    if (req->hasResponseContentLength) {
        *result = 1;
    } else if (req->isChunked) {
        *result = 2;
    } else {
        *result = 0;
    }
    *content_length = req->responseContentLength;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2GetStatusCode(int req_id, int* status_code) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); });
    if (status_code == nullptr) return netCall.Ret(ERROR_NULL_POINTER);
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* req = FindHandle(req_id, Http2Kind::Request);
    if (req == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    if (!req->completed) return netCall.Ret(ERROR_NETWORK);
    *status_code = req->responseStatusCode;
    return netCall.Ret(0);
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
    if (data == nullptr && size > 0) return netCall.Ret(ERROR_NULL_POINTER);
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* req = FindHandle(req_id, Http2Kind::Request);
    if (req == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    if (!req->completed) return netCall.Ret(ERROR_NETWORK);
    if (req->bodyReadOffset >= req->responseBody.size()) return netCall.Ret(0);
    const size_t available = req->responseBody.size() - req->bodyReadOffset;
    const size_t toRead = std::min(size, available);
    if (toRead > 0 && data != nullptr) {
        std::memcpy(data, req->responseBody.data() + req->bodyReadOffset, toRead);
        req->bodyReadOffset += toRead;
    }
    return netCall.Ret(static_cast<int>(toRead));
}

int APS5_VABI sceHttp2ReadDataAsync(int req_id, void* data, size_t size, void* kqueue_option, void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Uint("size", size); });
    (void)kqueue_option;
    (void)option;
    return netCall.Ret(sceHttp2ReadData(req_id, data, size));
}

int APS5_VABI sceHttp2SendRequest(int req_id, const void* post_data, size_t size) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Uint("size", size); });
    std::string method;
    std::string url;
    std::string userAgent;
    uint32_t timeoutUsec = 30000000;
    std::vector<std::pair<std::string, std::string>> headers;
    bool hasRequestContentLength = false;
    uint64_t requestContentLength = 0;
    {
        std::lock_guard lock(g_handlesMutex);
        Http2Handle* req = FindHandle(req_id, Http2Kind::Request);
        if (req == nullptr) return netCall.Ret(ERROR_INVALID_ID);
        method = req->method;
        url = req->url;
        userAgent = req->userAgent;
        timeoutUsec = req->timeoutUsec;
        headers = req->headers;
        hasRequestContentLength = req->hasRequestContentLength;
        requestContentLength = req->requestContentLength;
    }

    HttpResponse resp{};
    const int result = ExecuteHttpRequest(
        method, url, userAgent, timeoutUsec, headers,
        post_data, size, hasRequestContentLength, requestContentLength, resp
    );

    {
        std::lock_guard lock(g_handlesMutex);
        Http2Handle* req = FindHandle(req_id, Http2Kind::Request);
        if (req != nullptr) {
            if (result == 0) {
                req->completed = true;
                req->responseStatusCode = resp.statusCode;
                req->responseHeadersRaw = std::move(resp.headersRaw);
                req->responseBody = std::move(resp.body);
                req->responseContentLength = resp.contentLength;
                req->hasResponseContentLength = resp.hasContentLength;
                req->isChunked = resp.isChunked;
                req->bodyReadOffset = 0;
            } else {
                req->completed = false;
            }
        }
    }
    return netCall.Ret(result);
}

int APS5_VABI sceHttp2SendRequestAsync(int req_id, const void* post_data, size_t size, Http2AsyncOption* kqueue_option, void* option) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("req_id", req_id); netIn.Uint("size", size); });
    if (option != nullptr) NotImplemented_nid_no_patch(__func__);

    std::string method;
    std::string url;
    std::string userAgent;
    uint32_t timeoutUsec = 30000000;
    std::vector<std::pair<std::string, std::string>> headers;
    bool hasRequestContentLength = false;
    uint64_t requestContentLength = 0;
    {
        std::lock_guard lock(g_handlesMutex);
        Http2Handle* req = FindHandle(req_id, Http2Kind::Request);
        if (req == nullptr) return netCall.Ret(ERROR_INVALID_ID);
        method = req->method;
        url = req->url;
        userAgent = req->userAgent;
        timeoutUsec = req->timeoutUsec;
        headers = req->headers;
        hasRequestContentLength = req->hasRequestContentLength;
        requestContentLength = req->requestContentLength;
    }

    HttpResponse resp{};
    const int result = ExecuteHttpRequest(
        method, url, userAgent, timeoutUsec, headers,
        post_data, size, hasRequestContentLength, requestContentLength, resp
    );

    {
        std::lock_guard lock(g_handlesMutex);
        Http2Handle* req = FindHandle(req_id, Http2Kind::Request);
        if (req != nullptr) {
            if (result == 0) {
                req->completed = true;
                req->responseStatusCode = resp.statusCode;
                req->responseHeadersRaw = std::move(resp.headersRaw);
                req->responseBody = std::move(resp.body);
                req->responseContentLength = resp.contentLength;
                req->hasResponseContentLength = resp.hasContentLength;
                req->isChunked = resp.isChunked;
                req->bodyReadOffset = 0;
            } else {
                req->completed = false;
            }
        }
    }

    {
        std::lock_guard completionsLock(g_completionsMutex);
        Http2AsyncResult completion{};
        completion.req_id = req_id;
        completion.result = result;
        g_completions[req_id].push_back(completion);
    }

    if (kqueue_option != nullptr) {
        if (EqueueTriggerEvent_nid_postfix(kqueue_option->equeue, static_cast<uintptr_t>(kqueue_option->user_event_id), EVFILT_USER, kqueue_option->user_data) != 0) {
            {
                std::lock_guard lock(g_completionsMutex);
                const auto pending = g_completions.find(req_id);
                if (pending != g_completions.end() && !pending->second.empty()) pending->second.pop_back();
            }
            NotImplemented_nid_no_patch(__func__);
        }
    }
    return netCall.Ret(0);
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
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindAnyHandle(id);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->timeoutUsec = usec;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetConnectTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindAnyHandle(id);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->connectTimeoutUsec = usec;
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
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindAnyHandle(id);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->recvTimeoutUsec = usec;
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
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindHandle(id, Http2Kind::Request);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->requestContentLength = content_length;
    handle->hasRequestContentLength = true;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetResolveTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindAnyHandle(id);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->resolveTimeoutUsec = usec;
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetSendTimeOut(int id, uint32_t usec) {
    NetBridgeCall netCall("libSceHttp2", __func__, [&](NetBridgeJson& netIn) { netIn.Int("id", id); netIn.Uint("usec", usec); });
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindAnyHandle(id);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->sendTimeoutUsec = usec;
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
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindAnyHandle(id);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->timeoutUsec = usec;
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
    Http2Handle box{};
    box.kind = Http2Kind::CookieBox;
    box.poolSize = poolSize;
    box.cookieMemory = std::move(memory);
    g_handles[id] = std::move(box);
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
    std::lock_guard lock(g_handlesMutex);
    Http2Handle* handle = FindHandle(id, Http2Kind::Request);
    if (handle == nullptr) return netCall.Ret(ERROR_INVALID_ID);
    handle->requestContentLength = 0;
    handle->hasRequestContentLength = false;
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
