#include "prx/libSceNetCtl/include/NetBridge.hpp"

namespace {

__attribute__((used)) int (*const g_netBridgeAnchor)() = &NetBridgeObserveEnabled_nid_no_patch;

}
