#include "prx/libSceNetCtl/include/NetBridge.hpp"

extern "C" int NET_BRIDGE_ANCHOR() {
    return NetBridgeObserveEnabled_nid_no_patch();
}
