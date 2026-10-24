#include <cstddef>
#include <cstdint>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"

static constexpr int SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80550C02);
static constexpr int SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED = static_cast<int>(0x80550C03);
static constexpr int SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80550C0A);

namespace {

struct NpMatching2InitializeParameter {
    std::uint64_t poolSize;
    std::uint64_t cpuAffinityMask;
    std::int32_t threadPriority;
    std::uint8_t padding[4];
    std::uint64_t threadStackSize;
    std::uint64_t size;
    std::uint64_t sslPoolSize;
};
static_assert(offsetof(NpMatching2InitializeParameter, size) == 0x20);
static_assert(sizeof(NpMatching2InitializeParameter) == 0x30);

std::mutex matching2Mutex;
bool matching2Initialized = false;

}

extern "C" {

int APS5_VABI sceNpMatching2ContextStart() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2ContextStop() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2CreateContext() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2CreateJoinRoom() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2DestroyContext() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
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

int APS5_VABI sceNpMatching2RegisterContextCallback() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2RegisterRoomEventCallback() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2RegisterRoomMessageCallback() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpMatching2RegisterSignalingCallback() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
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

int APS5_VABI sceNpMatching2SetDefaultRequestOptParam() {
    NetBridgeCall netCall("libSceNpMatching2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
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
    return netCall.Ret(0);
}

}
