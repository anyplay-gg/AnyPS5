#include "prx/libSceNpMatching2/include/Api.hpp"
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"

static constexpr int SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80550C02);
static constexpr int SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED = static_cast<int>(0x80550C03);
static constexpr int SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80550C0A);
static constexpr int SCE_NP_MATCHING2_ERROR_CONTEXT_MAX = static_cast<int>(0x80550C04);
static constexpr int SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED = static_cast<int>(0x80550C08);
static constexpr int SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID = static_cast<int>(0x80550C0B);
static constexpr int SCE_NET_ERROR_RESOLVER_ETIMEDOUT = static_cast<int>(0x804101E2);
static constexpr int MAX_CONTEXTS = 10;
static constexpr std::uint64_t CREATE_CONTEXT_PARAMETER_SIZE = 0x28;

namespace {

static_assert(offsetof(NpMatching2InitializeParameter, size) == 0x20);
static_assert(sizeof(NpMatching2InitializeParameter) == 0x30);

static_assert(sizeof(NpMatching2CreateContextParameter) == CREATE_CONTEXT_PARAMETER_SIZE);

std::mutex matching2Mutex;
bool matching2Initialized = false;
std::unordered_set<std::uint16_t> contexts;
int nextContextId = 1;

int CheckContext(std::uint16_t ctxId) {
    if (!matching2Initialized) return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    return contexts.contains(ctxId) ? 0 : SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
}

}

extern "C" {

int APS5_VABI sceNpMatching2ContextStart(std::uint16_t ctxId, std::uint64_t timeout) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    (void)ctxId;
    (void)timeout;
    std::lock_guard lock(matching2Mutex);
    if (!matching2Initialized) return netCall.Ret(SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED);
    return netCall.Ret(SCE_NET_ERROR_RESOLVER_ETIMEDOUT);
}

int APS5_VABI sceNpMatching2ContextStop(std::uint16_t ctxId) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    std::lock_guard lock(matching2Mutex);
    if (const int error = CheckContext(ctxId)) return netCall.Ret(error);
    return netCall.Ret(SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED);
}

int APS5_VABI sceNpMatching2CreateContext(const NpMatching2CreateContextParameter* param, std::uint16_t* ctxId) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    std::lock_guard lock(matching2Mutex);
    if (!matching2Initialized) return netCall.Ret(SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED);
    if (!param || param->size != CREATE_CONTEXT_PARAMETER_SIZE || !ctxId) return netCall.Ret(SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT);
    for (int tried = 0; tried < MAX_CONTEXTS; ++tried) {
        const auto candidate = static_cast<std::uint16_t>((nextContextId - 1 + tried) % MAX_CONTEXTS + 1);
        if (contexts.contains(candidate)) continue;
        contexts.insert(candidate);
        nextContextId = candidate % MAX_CONTEXTS + 1;
        *ctxId = candidate;
        return netCall.Ret(0);
    }
    return netCall.Ret(SCE_NP_MATCHING2_ERROR_CONTEXT_MAX);
}

int APS5_VABI sceNpMatching2CreateJoinRoom() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2DestroyContext(std::uint16_t ctxId) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    std::lock_guard lock(matching2Mutex);
    if (const int error = CheckContext(ctxId)) return netCall.Ret(error);
    contexts.erase(ctxId);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetRoomDataExternalList() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetRoomDataInternal() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetRoomMemberDataInternal() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetServerId() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetUserInfoList() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetWorldInfoList() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2Initialize(const NpMatching2InitializeParameter* param) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    std::lock_guard lock(matching2Mutex);
    if (matching2Initialized) return netCall.Ret(SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED);
    if (!param || (param->size != offsetof(NpMatching2InitializeParameter, sslPoolSize) && param->size != sizeof(NpMatching2InitializeParameter))) return netCall.Ret(SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT);
    matching2Initialized = true;
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2JoinRoom() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2KickoutRoomMember() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2LeaveRoom() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2RegisterContextCallback(void* callback, void* userdata) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    (void)callback;
    (void)userdata;
    std::lock_guard lock(matching2Mutex);
    return netCall.Ret(matching2Initialized ? 0 : SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED);
}

int APS5_VABI sceNpMatching2RegisterRoomEventCallback(std::uint16_t ctxId, void* callback, void* userdata) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    (void)callback;
    (void)userdata;
    std::lock_guard lock(matching2Mutex);
    return netCall.Ret(CheckContext(ctxId));
}

int APS5_VABI sceNpMatching2RegisterRoomMessageCallback(std::uint16_t ctxId, void* callback, void* userdata) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    (void)callback;
    (void)userdata;
    std::lock_guard lock(matching2Mutex);
    return netCall.Ret(CheckContext(ctxId));
}

int APS5_VABI sceNpMatching2RegisterSignalingCallback(std::uint16_t ctxId, void* callback, void* userdata) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    (void)callback;
    (void)userdata;
    std::lock_guard lock(matching2Mutex);
    return netCall.Ret(CheckContext(ctxId));
}

int APS5_VABI sceNpMatching2SearchRoom() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SendRoomChatMessage() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SendRoomMessage() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SetDefaultRequestOptParam(std::uint16_t ctxId, const void* requestOpt) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    std::lock_guard lock(matching2Mutex);
    if (!matching2Initialized) return netCall.Ret(SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED);
    if (!requestOpt) return netCall.Ret(SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT);
    return netCall.Ret(CheckContext(ctxId));
}

int APS5_VABI sceNpMatching2SetRoomDataExternal() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SetRoomDataInternal() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SetRoomMemberDataInternal() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SetUserInfo() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingGetConnectionInfo() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2Terminate(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    std::lock_guard lock(matching2Mutex);
    if (!matching2Initialized) return netCall.Ret(SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED);
    matching2Initialized = false;
    contexts.clear();
    nextContextId = 1;
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2AbortContextStart(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2CreateContextA(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2CreateContextInternal(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2CreateJoinRoomA(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetLobbyInfoList(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetLobbyMemberDataInternal(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetLobbyMemberDataInternalList(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetMemoryInfo(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetRoomJoinedSlotMaskLocal(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetRoomMemberDataExternalList(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetRoomMemberIdListLocal(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetRoomPasswordLocal(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetSignalingOptParamLocal(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetSslMemoryInfo(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetUserInfoListA(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GetWorldIdArrayForAllServers(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2GrantRoomOwner(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2JoinLobby(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2JoinRoomA(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2LeaveLobby(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2RegisterLobbyEventCallback(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2RegisterLobbyMessageCallback(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2RegisterManualUdpSignalingCallback(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SendLobbyChatMessage(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SetExtraInitParam(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SetLobbyMemberDataInternal(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SetRoomDataInternalExt(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SetSignalingOptParam(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingAbortConnection(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingCancelPeerNetInfo(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingEnableManualUdpMode(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingEstablishConnection(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingGetConnectionInfoA(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingGetConnectionStatus(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingGetLocalNetInfo(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingGetPeerNetInfo(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingGetPeerNetInfoResult(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingGetPingInfo(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingGetPort(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2SignalingSetPort(void) {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

}
