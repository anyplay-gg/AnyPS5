#include "prx/libSceNpGameIntent/include/Api.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"

namespace {

constexpr int NpGameIntentErrorInvalidArgument = -2141898748;
constexpr int NpGameIntentErrorIntentNotFound = -2141898746;
constexpr int NpGameIntentErrorValueNotFound = -2141898745;
constexpr int NpGameIntentUserIdInvalid = -1;

}

extern "C" {

int APS5_VABI sceNpGameIntentGetPropertyValueString(const NpGameIntentData* intentData, const char* key, char* valueBuf, size_t bufSize) {
 NetBridgeCall netCall("libSceNpGameIntent", __func__, [&](NetBridgeJson& netIn) { netIn.Uint("buf_size", bufSize); });
 if (intentData == nullptr || key == nullptr || valueBuf == nullptr || bufSize == 0) {
  APS5_INVALID_ARG_EX;
 }

 valueBuf[0] = '\0';
 return netCall.Ret(NpGameIntentErrorValueNotFound);
}

int APS5_VABI sceNpGameIntentInitialize(const void* initParam) {
 NetBridgeCall netCall("libSceNpGameIntent", __func__);
 (void)initParam;
 return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentReceiveIntent(NpGameIntentInfo* intentInfo) {
 NetBridgeCall netCall("libSceNpGameIntent", __func__);
 if (intentInfo == nullptr) {
  APS5_INVALID_ARG_EX;
 }

 intentInfo->user_id = NpGameIntentUserIdInvalid;
 std::memset(intentInfo->intent_type, 0, sizeof(intentInfo->intent_type));
 std::memset(&intentInfo->intent_data, 0, sizeof(intentInfo->intent_data));

 return netCall.Ret(NpGameIntentErrorIntentNotFound);
}

int APS5_VABI sceNpGameIntentTerminate(void) {
 NetBridgeCall netCall("libSceNpGameIntent", __func__);
 return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentInitParamInit(NpGameIntentInitParam* param) {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    if (param == nullptr) {
        APS5_INVALID_ARG_EX;
    }
    std::memset(param, 0, sizeof(NpGameIntentInitParam));
    param->size = sizeof(NpGameIntentInitParam);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentInfoInit(NpGameIntentInfo* info) {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    if (info == nullptr) {
        APS5_INVALID_ARG_EX;
    }
    std::memset(info, 0, sizeof(NpGameIntentInfo));
    info->size = sizeof(NpGameIntentInfo);
    info->user_id = NpGameIntentUserIdInvalid;
    return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentVshInitialize(void) {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentVshTerminate(void) {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpAppLauncherInitialize(const void* initParam) {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    (void)initParam;
    return netCall.Ret(0);
}

int APS5_VABI sceNpAppLauncherLaunchApp() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpAppLauncherTerminate() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentGetUdsNpComId() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentLaunchApp() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentLaunchApp2() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpGameIntentNotifyAppLaunched() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpSessionServiceGetJoinedPartySession() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpSessionServiceGetRunningGamePlayerSession() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpSessionServiceSendPlayerSessionInvitations() {
    NetBridgeCall netCall("libSceNpGameIntent", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

}
