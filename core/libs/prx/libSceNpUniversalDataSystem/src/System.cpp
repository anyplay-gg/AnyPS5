#include "prx/libSceNpUniversalDataSystem/include/Api.hpp"
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include "NpUniversalDataSystem.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceNpUniversalDataSystemInitialize(const NpUniversalDataSystemInitParam* param) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    if (param == nullptr) {
        APS5_INVALID_ARG_EX;
    }
    return netCall.Ret(SCE_NP_UNIVERSAL_DATA_SYSTEM_OK);
}

int APS5_VABI sceNpUniversalDataSystemTerminate(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    return netCall.Ret(SCE_NP_UNIVERSAL_DATA_SYSTEM_OK);
}

int APS5_VABI sceNpUniversalDataSystemGetMemoryStat(NpUniversalDataSystemMemoryStat* stat) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    if (stat == nullptr) {
        APS5_INVALID_ARG_EX;
    }
    *stat = {};
    return netCall.Ret(SCE_NP_UNIVERSAL_DATA_SYSTEM_OK);
}

int APS5_VABI sceNpUniversalDataSystemGetStorageStat(int context, NpUniversalDataSystemStorageStat* stat) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__, [&](NetBridgeJson& netIn) { netIn.Int("context", context); });
    if (stat == nullptr) {
        APS5_INVALID_ARG_EX;
    }
    *stat = {};
    return netCall.Ret(SCE_NP_UNIVERSAL_DATA_SYSTEM_OK);
}

}
