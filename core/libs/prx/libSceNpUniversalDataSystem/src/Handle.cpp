#include "prx/libSceNpUniversalDataSystem/include/Api.hpp"
#include "NpUniversalDataSystem.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceNpUniversalDataSystemCreateHandle(int* handle) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    if (handle == nullptr) {
        APS5_INVALID_ARG_EX;
    }
    *handle = NP_UNIVERSAL_DATA_SYSTEM_HANDLE_DEFAULT;
    return netCall.Ret(SCE_NP_UNIVERSAL_DATA_SYSTEM_OK);
}

int APS5_VABI sceNpUniversalDataSystemDestroyHandle(int handle) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__, [&](NetBridgeJson& netIn) { netIn.Int("handle", handle); });
    return netCall.Ret(SCE_NP_UNIVERSAL_DATA_SYSTEM_OK);
}

int APS5_VABI sceNpUniversalDataSystemAbortHandle(int handle) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__, [&](NetBridgeJson& netIn) { netIn.Int("handle", handle); });
    return netCall.Ret(SCE_NP_UNIVERSAL_DATA_SYSTEM_OK);
}

}
