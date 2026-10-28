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
    Require(sceNpCheckPlus(request, plusParameter, plusResult) == 0 && plusResult[0] == 1 && plusResult[1] == 0x5A, "plus granted while signed in");
    int polled = 1;
    Require(sceNpPollAsync(request, &polled) == 0 && polled == 0, "poll reports the grant");
    NpCheckPremiumParameter premiumParameter{};
    NpCheckPremiumResult premiumResult;
    std::memset(&premiumResult, 0x5A, sizeof(premiumResult));
    Require(sceNpCheckPremium(request, &premiumParameter, &premiumResult) == 0 && premiumResult.opaque[0] == 1 && premiumResult.opaque[1] == 0 && premiumResult.opaque[63] == 0, "premium granted while signed in");
    Require(sceNpCheckPremium(request, &premiumParameter, nullptr) == InvalidArgument, "premium without a result");
    Require(sceNpCheckPlus(0, plusParameter, plusResult) == InvalidArgument && sceNpCheckPlus(request, nullptr, plusResult) == InvalidArgument, "plus arguments");
    Require(sceNpDeleteRequest(request) == 0, "delete request");
    Require(sceNpCheckPlus(request, plusParameter, plusResult) == RequestNotFound, "plus on a deleted request");
    Require(sceNpPollAsync(request, &polled) == RequestNotFound, "a deleted request is not found");

    const int identityRequest = sceNpCreateRequest();
    char country[4];
    std::memset(country, 0x5A, sizeof(country));
    Require(sceNpGetAccountCountryA(USER_SERVICE_INITIAL_USER_ID, country) == 0 && std::string(country) == "us" && country[3] == 0x5A, "country of the signed in user");
    std::memset(country, 0x5A, sizeof(country));
    Require(sceNpManagerIntGetAccountCountryA(USER_SERVICE_INITIAL_USER_ID, country) == 0 && std::string(country) == "us", "internal country of the signed in user");
    std::memset(country, 0x5A, sizeof(country));
    Require(sceNpGetAccountCountry(&onlineId, country) == 0 && std::string(country) == "us", "country by online id");
    NpOnlineId strangerId;
    std::memset(&strangerId, 0, sizeof(strangerId));
    std::memcpy(strangerId.data, "Somebody_Else", 13);
    Require(sceNpGetAccountCountry(&strangerId, country) == static_cast<int>(0x80550007u), "no country for an online id that is not the signed in player");
    Require(sceNpGetAccountCountryA(OtherUser, country) == SignedOut, "no country for another user");
    Require(sceNpGetAccountCountryA(USER_SERVICE_INITIAL_USER_ID, nullptr) == InvalidArgument, "null country");

    std::uint8_t accountAge = 0;
    Require(sceNpGetAccountAge(identityRequest, USER_SERVICE_INITIAL_USER_ID, &accountAge) == 0 && accountAge == 18, "account age");
    Require(sceNpPollAsync(identityRequest, &polled) == 0 && polled == 0, "poll reports the age");
    accountAge = 0;
    Require(sceNpManagerIntGetAccountAge(identityRequest, USER_SERVICE_INITIAL_USER_ID, &accountAge) == 0 && accountAge == 18, "internal account age");
    Require(sceNpGetAccountAge(identityRequest, OtherUser, &accountAge) == SignedOut, "no age for another user");
    Require(sceNpGetAccountAge(0, USER_SERVICE_INITIAL_USER_ID, &accountAge) == InvalidArgument, "age needs a request");
    Require(sceNpGetAccountAge(request, USER_SERVICE_INITIAL_USER_ID, &accountAge) == RequestNotFound, "age on a deleted request");
    Require(sceNpGetAccountAge(identityRequest, USER_SERVICE_INITIAL_USER_ID, nullptr) == InvalidArgument, "null age");

    char language[16];
    std::memset(language, 0x5A, sizeof(language));
    Require(sceNpGetAccountLanguageA(identityRequest, USER_SERVICE_INITIAL_USER_ID, language) == 0 && std::string(language) == "en" && language[5] == 0 && language[6] == 0x5A, "language of the signed in user");
    std::memset(language, 0x5A, sizeof(language));
    Require(sceNpGetAccountLanguage2(identityRequest, USER_SERVICE_INITIAL_USER_ID, language) == 0 && std::string(language) == "en", "language 2");
    FakeNet::SetEnvironment("ANYPS5_LANGUAGE", "4");
    Require(sceNpGetAccountLanguageA(identityRequest, USER_SERVICE_INITIAL_USER_ID, language) == 0 && std::string(language) == "de", "language follows the console language");
    FakeNet::SetEnvironment("ANYPS5_LANGUAGE", "");
    Require(sceNpGetAccountLanguage(identityRequest, &onlineId, language) == 0 && std::string(language) == "en", "language by online id");
    Require(sceNpGetAccountLanguage(identityRequest, &strangerId, language) == static_cast<int>(0x80550007u), "no language for a stranger");
    Require(sceNpGetAccountLanguageA(identityRequest, OtherUser, language) == SignedOut, "no language for another user");
    Require(sceNpGetAccountLanguageA(request, USER_SERVICE_INITIAL_USER_ID, language) == RequestNotFound, "language on a deleted request");

    std::int8_t parentalAge = 0;
    std::uint8_t parentalInfo[4];
    std::memset(parentalInfo, 0x5A, sizeof(parentalInfo));
    Require(sceNpGetParentalControlInfoA(identityRequest, USER_SERVICE_INITIAL_USER_ID, &parentalAge, parentalInfo) == 0 && parentalAge == 18, "parental control answers for the signed in user");
    Require(parentalInfo[0] == 0 && parentalInfo[1] == 0 && parentalInfo[2] == 0 && parentalInfo[3] == 0x5A, "no content, chat or user generated content restriction");
    Require(sceNpGetParentalControlInfo(identityRequest, &onlineId, &parentalAge, parentalInfo) == 0, "parental control by online id");
    Require(sceNpGetParentalControlInfo(identityRequest, &strangerId, &parentalAge, parentalInfo) == static_cast<int>(0x80550007u), "no parental control for a stranger");
    Require(sceNpGetParentalControlInfoA(identityRequest, OtherUser, &parentalAge, parentalInfo) == SignedOut, "no parental control for another user");
    Require(sceNpGetParentalControlInfoA(identityRequest, USER_SERVICE_INITIAL_USER_ID, nullptr, parentalInfo) == InvalidArgument, "null parental age");

    Require(sceNpCheckNpAvailabilityA(identityRequest, USER_SERVICE_INITIAL_USER_ID) == 0, "network is available");
    std::uint8_t available = 0;
    Require(sceNpCheckNpAvailability(identityRequest, &onlineId, &available) == 0 && available == 1, "network is available by online id");
    Require(sceNpCheckNpAvailability(identityRequest, &strangerId, &available) == static_cast<int>(0x80550007u) && available == 0, "network is not available for a stranger");
    Require(sceNpCheckNpAvailabilityA(identityRequest, OtherUser) == SignedOut, "network is not available for another user");
    Require(sceNpCheckNpReachability(identityRequest, USER_SERVICE_INITIAL_USER_ID) == 0, "reachability check");
    Require(sceNpCheckNpReachability(identityRequest, OtherUser) == SignedOut, "reachability check for another user");
    Require(sceNpSetGamePresenceOnline(identityRequest, &onlineId, 1) == 0, "presence set");
    Require(sceNpSetGamePresenceOnline(identityRequest, &strangerId, 1) == SignedOut, "presence of a stranger");

    std::uint32_t presence = 7;
    Require(sceNpGetGamePresenceStatusA(USER_SERVICE_INITIAL_USER_ID, &presence) == 0 && presence == 1, "presence online");
    Require(sceNpGetGamePresenceStatusA(OtherUser, &presence) == 0 && presence == 0, "presence of another user");
    Require(sceNpGetGamePresenceStatus(&onlineId, &presence) == 0 && presence == 1, "presence online by online id");
    Require(sceNpGetGamePresenceStatus(&strangerId, &presence) == 0 && presence == 0, "presence of a stranger");

    int foundUser = 0;
    Require(sceNpGetUserIdByOnlineId(&onlineId, &foundUser) == 0 && foundUser == USER_SERVICE_INITIAL_USER_ID, "user by online id");
    foundUser = 0;
    Require(sceNpManagerIntGetUserIdByOnlineId(&onlineId, &foundUser) == 0 && foundUser == USER_SERVICE_INITIAL_USER_ID, "internal user by online id");
    Require(sceNpGetUserIdByOnlineId(&strangerId, &foundUser) == SignedOut, "no user for a stranger");
    foundUser = 0;
    Require(sceNpGetUserIdByAccountId(AccountId, &foundUser) == 0 && foundUser == USER_SERVICE_INITIAL_USER_ID, "user by account id");
    Require(sceNpGetUserIdByAccountId(AccountId + 1, &foundUser) == SignedOut, "no user for another account id");
    std::uint64_t foundAccount = 0;
    Require(sceNpGetAccountId(&onlineId, &foundAccount) == 0 && foundAccount == AccountId, "account id by online id");
    Require(sceNpGetAccountId(&strangerId, &foundAccount) == SignedOut, "no account id for a stranger");
    bool plusMember = false;
    Require(sceNpIsPlusMember(USER_SERVICE_INITIAL_USER_ID, &plusMember) == 0 && plusMember, "plus member while signed in");
    Require(sceNpIsPlusMember(OtherUser, &plusMember) == 0 && !plusMember, "another user is no plus member");
    Require(sceNpDeleteRequest(identityRequest) == 0, "delete identity request");

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
    std::memset(country, 0x5A, sizeof(country));
    Require(sceNpGetAccountCountryA(USER_SERVICE_INITIAL_USER_ID, country) == SignedOut && country[0] == 0x5A, "no country while signed out");
    Require(sceNpGetAccountCountry(&onlineId, country) == static_cast<int>(0x80550007u), "no country by online id while signed out");
    Require(sceNpGetAccountAge(second, USER_SERVICE_INITIAL_USER_ID, &accountAge) == SignedOut, "no age while signed out");
    Require(sceNpGetAccountLanguageA(second, USER_SERVICE_INITIAL_USER_ID, language) == SignedOut, "no language while signed out");
    Require(sceNpGetParentalControlInfoA(second, USER_SERVICE_INITIAL_USER_ID, &parentalAge, parentalInfo) == SignedOut, "no parental control while signed out");
    Require(sceNpCheckNpAvailabilityA(second, USER_SERVICE_INITIAL_USER_ID) == SignedOut && sceNpCheckNpReachability(second, USER_SERVICE_INITIAL_USER_ID) == SignedOut, "network unavailable while signed out");
    Require(sceNpGetGamePresenceStatusA(USER_SERVICE_INITIAL_USER_ID, &presence) == 0 && presence == 0, "offline presence while signed out");
    Require(sceNpGetUserIdByOnlineId(&onlineId, &foundUser) == SignedOut && sceNpGetUserIdByAccountId(AccountId, &foundUser) == SignedOut, "no user lookup while signed out");
    Require(sceNpIsPlusMember(USER_SERVICE_INITIAL_USER_ID, &plusMember) == 0 && !plusMember, "no plus member while signed out");

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
    Require(content.find("\"fn\":\"sceNpCheckPlus\",\"in\":{\"req_id\":" + std::to_string(request) + "},\"out\":{\"authorized\":true},\"ret\":\"0x0\"") != std::string::npos, "sceNpCheckPlus authorized line");

    std::filesystem::remove(observePath, removed);
    std::puts("np manager net bridge: ok");
    return 0;
}
