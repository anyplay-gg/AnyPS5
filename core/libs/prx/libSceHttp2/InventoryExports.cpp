#include "prx/libSceHttp2/include/InventoryExports.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceHttp2GetAllTrailingHeaders() {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetProxyWithURL() {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetRequestServerName() {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2SetStripGZIPHeadersEnabled() {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketClose() {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketSendDataMessage() {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketSendTextMessage() {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI sceHttp2WebSocketSetCloseTimeout() {
    NetBridgeCall netCall("libSceHttp2", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

}
