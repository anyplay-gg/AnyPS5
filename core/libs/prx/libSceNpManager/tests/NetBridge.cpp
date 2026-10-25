#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libSceNetCtl/tests/FakeNetService.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include "prx/libSceNpManager/include/Api.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using FakeNet::Require;
using FakeNet::Service;

constexpr int SignedOut = static_cast<int>(0x80550006u);
constexpr int InvalidArgument = static_cast<int>(0x80550003u);
constexpr int RequestNotFound = static_cast<int>(0x80550014u);
constexpr std::uint64_t AccountId = 0x0123456789ABCDEFull;
constexpr int OtherUser = 0x10000001;

struct Delivery {
    int calls = 0;
    int userId = 0;
    std::uint32_t state = 0;
    bool hadNpId = false;
    std::string npIdName;
};

Delivery g_legacy;
Delivery g_modern;
Delivery g_reachability;

void APS5_VABI ReachabilityCallback(int userId, std::uint32_t state, void*) {
    ++g_reachability.calls;
    g_reachability.userId = userId;
    g_reachability.state = state;
}

void APS5_VABI LegacyCallback(int userId, std::uint32_t state, NpId* npId, void*) {
    ++g_legacy.calls;
    g_legacy.userId = userId;
    g_legacy.state = state;
    g_legacy.hadNpId = npId != nullptr;
    g_legacy.npIdName = npId != nullptr ? npId->online_id.data : "";
}

void APS5_VABI ModernCallback(int userId, std::uint32_t state, void*) {
    ++g_modern.calls;
    g_modern.userId = userId;
    g_modern.state = state;
}

bool SignedInNow() {
    NetBridgeStateView view{};
    NetBridgeState_nid_no_patch(&view);
    return view.signedIn != 0;
}

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

}

int main() {
    const FakeNet::Bytes token = {0xAA, 0xBB, 0xCC, 0x01};
    Service service(token);
    const std::filesystem::path observePath = std::filesystem::temp_directory_path() / "anyps5-net-bridge-np.jsonl";
    std::error_code removed;
    std::filesystem::remove(observePath, removed);
    FakeNet::SetEnvironment("ANYPS5_NET_ENDPOINT", "127.0.0.1:" + std::to_string(service.Port()));
    FakeNet::SetEnvironment("ANYPS5_NET_TOKEN", FakeNet::HexOf(token));
    FakeNet::SetEnvironment("ANYPS5_NET_OBSERVE", observePath.string());
    const std::uint8_t address[4] = {0x05, 0x00, 0x12, 0xC6};
    service.SetState(true, AccountId, "Sixteen_Chars_16", 1, address);

    Require(sceNpRegisterStateCallback(reinterpret_cast<NpStateCallback>(&LegacyCallback), nullptr) == 0, "register legacy callback");
    Require(sceNpRegisterStateCallbackA(reinterpret_cast<NpStateCallbackA>(&ModernCallback), nullptr) == 1, "register modern callback");
    Require(sceNpRegisterNpReachabilityStateCallback(reinterpret_cast<void*>(&ReachabilityCallback), nullptr) == 0, "register reachability callback");

    std::uint32_t state = 0;
    Require(sceNpGetState(USER_SERVICE_INITIAL_USER_ID, &state) == 0 && state == 2, "signed in state");
    Require(sceNpGetState(OtherUser, &state) == 0 && state == 1, "another user is signed out");
    Require(sceNpGetState(USER_SERVICE_INITIAL_USER_ID, nullptr) == InvalidArgument, "null state");

    Require(sceNpCheckCallback() == 0, "check callback");
    Require(g_legacy.calls == 1 && g_modern.calls == 1, "each callback is called once");
    Require(g_legacy.state == 2 && g_modern.state == 2 && g_legacy.userId == USER_SERVICE_INITIAL_USER_ID, "callbacks see signed in");
    Require(g_legacy.hadNpId && g_legacy.npIdName == "Sixteen_Chars_16", "legacy callback gets the NpId");
    Require(sceNpCheckCallback() == 0 && g_legacy.calls == 1 && g_modern.calls == 1, "no change, no delivery");
    Require(g_reachability.calls == 1 && g_reachability.state == 2 && g_reachability.userId == USER_SERVICE_INITIAL_USER_ID, "reachability callback sees reachable once");
    std::uint32_t reachability = 7;
    Require(sceNpGetNpReachabilityState(USER_SERVICE_INITIAL_USER_ID, &reachability) == 0 && reachability == 2, "reachable while signed in");
    Require(sceNpGetNpReachabilityState(OtherUser, &reachability) == 0 && reachability == 0, "another user is unavailable");
    Require(sceNpGetNpReachabilityState(USER_SERVICE_INITIAL_USER_ID, nullptr) == InvalidArgument, "null reachability state");
    bool signedUp = false;
    Require(sceNpHasSignedUp(USER_SERVICE_INITIAL_USER_ID, &signedUp) == 0 && signedUp, "signed up while signed in");
    Require(sceNpHasSignedUp(OtherUser, &signedUp) == 0 && !signedUp, "another user has not signed up");
    Require(sceNpHasSignedUp(USER_SERVICE_INITIAL_USER_ID, nullptr) == InvalidArgument, "null sign-up flag");

    NpOnlineId onlineId;
    std::memset(&onlineId, 0x5A, sizeof(onlineId));
    Require(sceNpGetOnlineId(USER_SERVICE_INITIAL_USER_ID, &onlineId) == 0, "online id");
    Require(std::string(onlineId.data) == "Sixteen_Chars_16" && onlineId.data[16] == '\0' && onlineId.pad[0] == 0 && onlineId.pad[2] == 0, "online id fields");
    NpId npId;
    std::memset(&npId, 0x5A, sizeof(npId));
    Require(sceNpGetNpId(USER_SERVICE_INITIAL_USER_ID, &npId) == 0 && std::string(npId.online_id.data) == "Sixteen_Chars_16", "np id");
    for (const auto value : npId.opaque) Require(value == 0, "np id opaque cleared");
    std::uint64_t account = 0;
    Require(sceNpGetAccountIdA(USER_SERVICE_INITIAL_USER_ID, &account) == 0 && account == AccountId, "account id");
    Require(sceNpGetAccountIdA(OtherUser, &account) == SignedOut, "account id of another user");
    Require(sceNpGetOnlineId(OtherUser, &onlineId) == SignedOut && sceNpGetNpId(OtherUser, &npId) == SignedOut, "identity of another user");
    Require(sceNpGetAccountIdA(USER_SERVICE_INITIAL_USER_ID, nullptr) == InvalidArgument, "null account id");
    Require(sceNpGetOnlineId(USER_SERVICE_INITIAL_USER_ID, nullptr) == InvalidArgument && sceNpGetNpId(USER_SERVICE_INITIAL_USER_ID, nullptr) == InvalidArgument, "null identity");

    std::uint64_t serviceAccount = 1;
    Require(sceUserServiceGetNpAccountId(USER_SERVICE_INITIAL_USER_ID, &serviceAccount) == 0 && serviceAccount == AccountId, "user service account id");

    const int request = sceNpCreateRequest();
    std::uint8_t plusResult[40];
    std::memset(plusResult, 0x5A, sizeof(plusResult));
    const std::uint8_t plusParameter[16] = {};
    Require(sceNpCheckPlus(request, plusParameter, plusResult) == SignedOut && plusResult[0] == 0x5A, "plus stays signed out while signed in");
    int polled = 1;
    Require(sceNpPollAsync(request, &polled) == 0 && polled == SignedOut, "poll stays signed out while signed in");
    NpCheckPremiumParameter premiumParameter{};
    NpCheckPremiumResult premiumResult;
    std::memset(&premiumResult, 0x5A, sizeof(premiumResult));
    Require(sceNpCheckPremium(request, &premiumParameter, &premiumResult) == SignedOut && premiumResult.opaque[0] == 0x5A, "premium stays signed out while signed in");
    Require(sceNpCheckPlus(0, plusParameter, plusResult) == InvalidArgument && sceNpCheckPlus(request, nullptr, plusResult) == InvalidArgument, "plus arguments");
    Require(sceNpDeleteRequest(request) == 0, "delete request");

    int dummyFeature = 0;
    Require(sceNpNotifyPlusFeature(&dummyFeature) == 0, "plus feature accepted");
    Require(sceNpNotifyPlusFeature(nullptr) == InvalidArgument, "plus feature needs a parameter");
    Require(Service::WaitUntil([&] { return service.Features().size() == 1; }, 5000), "feature event reaches the service");

    service.SetState(false, 0, "", 0, address);
    Require(Service::WaitUntil([] { return !SignedInNow(); }, 5000), "signed out");
    Require(sceNpCheckCallback() == 0 && g_legacy.calls == 2 && g_modern.calls == 2, "sign out is delivered");
    Require(g_legacy.state == 1 && g_modern.state == 1 && !g_legacy.hadNpId, "callbacks see signed out");
    Require(g_reachability.calls == 2 && g_reachability.state == 0, "reachability callback sees unavailable");
    Require(sceNpGetNpReachabilityState(USER_SERVICE_INITIAL_USER_ID, &reachability) == 0 && reachability == 0, "unavailable while signed out");
    Require(sceNpHasSignedUp(USER_SERVICE_INITIAL_USER_ID, &signedUp) == 0 && !signedUp, "not signed up while signed out");
    Require(sceNpGetState(USER_SERVICE_INITIAL_USER_ID, &state) == 0 && state == 1, "signed out state");
    Require(sceNpGetOnlineId(USER_SERVICE_INITIAL_USER_ID, &onlineId) == SignedOut, "no online id while signed out");
    Require(sceNpGetNpId(USER_SERVICE_INITIAL_USER_ID, &npId) == SignedOut, "no np id while signed out");
    Require(sceNpGetAccountIdA(USER_SERVICE_INITIAL_USER_ID, &account) == SignedOut, "no account id while signed out");
    Require(sceUserServiceGetNpAccountId(USER_SERVICE_INITIAL_USER_ID, &serviceAccount) == 0 && serviceAccount == 0, "user service account id is 0 while signed out");
    const int second = sceNpCreateRequest();
    std::memset(plusResult, 0x5A, sizeof(plusResult));
    Require(sceNpCheckPlus(second, plusParameter, plusResult) == SignedOut && plusResult[0] == 0x5A, "plus refused while signed out");
    Require(sceNpCheckPremium(second, &premiumParameter, &premiumResult) == SignedOut, "premium refused while signed out");
    Require(sceNpPollAsync(second, &polled) == 0 && polled == SignedOut, "poll while signed out");

    service.SetState(true, AccountId, "Sixteen_Chars_16", 1, address);
    Require(Service::WaitUntil([] { return SignedInNow(); }, 5000), "signed in again");
    Require(sceNpCheckCallback() == 0 && g_legacy.calls == 3 && g_modern.calls == 3 && g_modern.state == 2, "sign in is delivered");
    Require(g_reachability.calls == 3 && g_reachability.state == 2, "reachability follows the second sign in");
    Require(sceNpUnregisterNpReachabilityStateCallback() == 0, "unregister reachability callback");
    Require(sceNpCheckCallback() == 0 && g_reachability.calls == 3, "no reachability delivery after unregistering");

    Require(Service::WaitUntil([&] {
        const std::string content = ReadFile(observePath);
        return content.find("\"fn\":\"sceNpCheckCallback\"") != std::string::npos && content.find("\"fn\":\"sceUserServiceGetNpAccountId\"") != std::string::npos &&
            content.find("\"fn\":\"sceNpPollAsync\"") != std::string::npos && content.find("\"fn\":\"sceNpNotifyPlusFeature\"") != std::string::npos;
    }, 5000), "observation lines written");
    const std::string content = ReadFile(observePath);
    Require(content.find("\"fn\":\"sceNpGetState\",\"in\":{\"user_id\":268435456},\"out\":{\"state\":2},\"ret\":\"0x0\"") != std::string::npos, "sceNpGetState line");
    Require(content.find("\"fn\":\"sceNpGetOnlineId\",\"in\":{\"user_id\":268435456},\"out\":{\"online_id\":\"Sixteen_Chars_16\"},\"ret\":\"0x0\"") != std::string::npos, "sceNpGetOnlineId line");
    Require(content.find("\"fn\":\"sceNpGetAccountIdA\",\"in\":{\"user_id\":268435456},\"out\":{},\"ret\":\"0x80550006\"") != std::string::npos, "signed out account line");

    std::filesystem::remove(observePath, removed);
    std::puts("np manager net bridge: ok");
    return 0;
}
