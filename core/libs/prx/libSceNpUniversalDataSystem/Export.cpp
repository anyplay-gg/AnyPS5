#include "prx/libSceNpUniversalDataSystem/include/InventoryExports.hpp"
#include "prx/libSceNpUniversalDataSystem/include/Api.hpp"
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceNpUniversalDataSystemIntCloseStream() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpUniversalDataSystemIntDeleteUserData() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpUniversalDataSystemIntGetNpCommIdByNpTitleId() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpUniversalDataSystemIntGetNpTitleIdListByNpCommId() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpUniversalDataSystemIntGetObjectState() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpUniversalDataSystemIntOpenObjectsDefinitionConfStream() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpUniversalDataSystemIntOpenObjectsDefinitionConfStreamByObjectId() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpUniversalDataSystemIntOpenStreamByUri() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpUniversalDataSystemIntReadStream() {
    NotImplemented_nid_no_patch(__func__);
}


int APS5_VABI sceNpUniversalDataSystemIntAbortHandle(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntCheckNetSyncTitles(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntCreateContext(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntCreateHandle(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntDeleteAllData(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntDeleteUserDataWithServerData(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntDestroyContext(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntDestroyHandle(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntInitialize(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntNetSyncTitles(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceNpUniversalDataSystemIntTerminate(void) {
    NetBridgeCall netCall("libSceNpUniversalDataSystem", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

}
