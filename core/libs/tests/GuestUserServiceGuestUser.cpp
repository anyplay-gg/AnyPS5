#include "SceTypes.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr int GuestUser = USER_SERVICE_INITIAL_USER_ID + 1;

std::string NameOf(int user) {
    char name[32];
    std::memset(name, 'A', sizeof(name));
    Require(sceUserServiceGetUserName(user, name, sizeof(name)) == USER_SERVICE_OK);
    return name;
}

std::string GuestNameFor(const char* given) {
    UserServiceSetGuestUser_nid_postfix(1, given);
    const std::string name = NameOf(GuestUser);
    UserServiceClearGuestUser_nid_postfix(1);
    return name;
}

void RequireEvent(std::uint32_t type, int user) {
    SceUserServiceEvent event{};
    Require(sceUserServiceGetEvent(&event) == USER_SERVICE_OK);
    Require(event.event_type == type && event.user_id == user);
}

void RequireNoEvent() {
    SceUserServiceEvent event{};
    Require(sceUserServiceGetEvent(&event) == USER_SERVICE_ERROR_NO_EVENT);
}

void TestClaimBeforeInitialize() {
    UserServiceSetGuestUser_nid_postfix(1, "Mira");
    Require(sceUserServiceInitialize(nullptr) == USER_SERVICE_OK);
    UserServiceLoginUserIdList list{};
    Require(sceUserServiceGetLoginUserIdList(&list) == USER_SERVICE_OK);
    Require(list.user_id[0] == USER_SERVICE_INITIAL_USER_ID && list.user_id[1] == GuestUser);
    RequireEvent(0, USER_SERVICE_INITIAL_USER_ID);
    RequireEvent(0, GuestUser);
    RequireNoEvent();
    Require(NameOf(GuestUser) == "Mira");
    UserServiceClearGuestUser_nid_postfix(1);
    RequireEvent(1, GuestUser);
    RequireNoEvent();
    Require(sceUserServiceTerminate() == USER_SERVICE_OK);
}

void TestClaimWhileRunning() {
    Require(sceUserServiceInitialize(nullptr) == USER_SERVICE_OK);
    RequireEvent(0, USER_SERVICE_INITIAL_USER_ID);
    RequireNoEvent();

    UserServiceLoginUserIdList list{};
    UserServiceRegisteredUserIdList registered{};
    UserServiceSetGuestUser_nid_postfix(1, "Mira");
    RequireEvent(0, GuestUser);
    RequireNoEvent();
    Require(sceUserServiceGetLoginUserIdList(&list) == USER_SERVICE_OK);
    Require(list.user_id[1] == GuestUser && list.user_id[2] == USER_SERVICE_USER_ID_INVALID);
    Require(sceUserServiceGetRegisteredUserIdList(&registered) == USER_SERVICE_OK);
    Require(registered.user_id[1] == GuestUser);
    Require(NameOf(GuestUser) == "Mira");

    UserServiceSetGuestUser_nid_postfix(1, "Mira 2");
    RequireNoEvent();
    Require(NameOf(GuestUser) == "Mira 2");

    std::int32_t number = 0;
    Require(sceUserServiceGetUserNumber(GuestUser, &number) == USER_SERVICE_OK && number == 2);
    std::uint32_t age = 0;
    Require(sceUserServiceGetAgeLevel(GuestUser, &age) == USER_SERVICE_OK);
    std::uint64_t account = 0x5A5A;
    Require(sceUserServiceGetNpAccountId(GuestUser, &account) == USER_SERVICE_OK && account == 0);

    UserServiceClearGuestUser_nid_postfix(1);
    RequireEvent(1, GuestUser);
    RequireNoEvent();
    Require(sceUserServiceGetLoginUserIdList(&list) == USER_SERVICE_OK);
    Require(list.user_id[1] == USER_SERVICE_USER_ID_INVALID);
    Require(sceUserServiceGetAgeLevel(GuestUser, &age) == USER_SERVICE_ERROR_NOT_LOGGED_IN);
    Require(NameOf(GuestUser) == USER_SERVICE_USER_NAMES[1]);

    UserServiceClearGuestUser_nid_postfix(1);
    RequireNoEvent();
    Require(sceUserServiceTerminate() == USER_SERVICE_OK);
}

void TestShortLivedClaimLeavesNoEvent() {
    Require(sceUserServiceInitialize(nullptr) == USER_SERVICE_OK);
    RequireEvent(0, USER_SERVICE_INITIAL_USER_ID);
    UserServiceSetGuestUser_nid_postfix(1, "Mira");
    UserServiceClearGuestUser_nid_postfix(1);
    RequireNoEvent();
    Require(sceUserServiceTerminate() == USER_SERVICE_OK);
}

void TestHotplugAndGuestShareASlot() {
    Require(sceUserServiceInitialize(nullptr) == USER_SERVICE_OK);
    RequireEvent(0, USER_SERVICE_INITIAL_USER_ID);

    UserServiceSetUserConnected_nid_postfix(1, true);
    RequireEvent(0, GuestUser);
    Require(NameOf(GuestUser) == USER_SERVICE_USER_NAMES[1]);
    UserServiceSetGuestUser_nid_postfix(1, "Mira");
    RequireNoEvent();
    Require(NameOf(GuestUser) == "Mira");
    UserServiceSetUserConnected_nid_postfix(1, false);
    RequireNoEvent();
    std::uint32_t age = 0;
    Require(sceUserServiceGetAgeLevel(GuestUser, &age) == USER_SERVICE_OK);
    UserServiceClearGuestUser_nid_postfix(1);
    RequireEvent(1, GuestUser);
    RequireNoEvent();

    UserServiceSetUserConnected_nid_postfix(1, true);
    RequireEvent(0, GuestUser);
    UserServiceSetGuestUser_nid_postfix(1, "Mira");
    UserServiceClearGuestUser_nid_postfix(1);
    RequireNoEvent();
    Require(sceUserServiceGetAgeLevel(GuestUser, &age) == USER_SERVICE_OK);
    UserServiceSetUserConnected_nid_postfix(1, false);
    RequireEvent(1, GuestUser);
    Require(sceUserServiceTerminate() == USER_SERVICE_OK);
}

void TestNames() {
    Require(GuestNameFor("Mira") == "Mira");
    Require(GuestNameFor(nullptr) == USER_SERVICE_USER_NAMES[1]);
    Require(GuestNameFor("") == USER_SERVICE_USER_NAMES[1]);
    Require(GuestNameFor("   ") == USER_SERVICE_USER_NAMES[1]);
    Require(GuestNameFor("  Mira  ") == "Mira");
    Require(GuestNameFor("Mi\tra\n") == "Mira");
    Require(GuestNameFor("0123456789abcdefXYZ") == "0123456789abcdef");
    Require(GuestNameFor("0123456789abcde\xC3\xA9") == "0123456789abcde");
    Require(GuestNameFor("0123456789abcd\xC3\xA9") == "0123456789abcd\xC3\xA9");
    Require(GuestNameFor("Mi\xFF" "ra\xC3") == "Mira");
    Require(GuestNameFor("\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC") == "\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC");
}

void TestOutOfRangeSlotsAreIgnored() {
    Require(sceUserServiceInitialize(nullptr) == USER_SERVICE_OK);
    RequireEvent(0, USER_SERVICE_INITIAL_USER_ID);
    for (int index : {-1, 0, USER_SERVICE_MAX_USERS, 99}) {
        UserServiceSetGuestUser_nid_postfix(index, "Mira");
        UserServiceClearGuestUser_nid_postfix(index);
    }
    RequireNoEvent();
    UserServiceLoginUserIdList list{};
    Require(sceUserServiceGetLoginUserIdList(&list) == USER_SERVICE_OK);
    Require(list.user_id[0] == USER_SERVICE_INITIAL_USER_ID && list.user_id[1] == USER_SERVICE_USER_ID_INVALID);
    Require(NameOf(USER_SERVICE_INITIAL_USER_ID) == USER_SERVICE_INITIAL_USER_NAME);
    Require(sceUserServiceTerminate() == USER_SERVICE_OK);
}

}

int main() {
    TestClaimBeforeInitialize();
    TestClaimWhileRunning();
    TestShortLivedClaimLeavesNoEvent();
    TestHotplugAndGuestShareASlot();
    TestNames();
    TestOutOfRangeSlotsAreIgnored();
}
