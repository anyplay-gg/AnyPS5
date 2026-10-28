#include "prx/libSceSystemService/include/SystemService.hpp"
#include <cstddef>
#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <atomic>
#include <array>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include "prx/libSceUserService/UserService.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"

namespace {

std::atomic<bool> initialized{false};
std::mutex userMutex;

struct UserEvent {
    std::uint32_t eventType;
    int userId;
};

std::deque<UserEvent> eventQueue;
std::array<bool, USER_SERVICE_MAX_USERS> userConnected{true, false, false, false};
std::array<bool, USER_SERVICE_MAX_USERS> hotplugConnected{true, false, false, false};
std::array<bool, USER_SERVICE_MAX_USERS> guestClaimed{};
std::array<std::string, USER_SERVICE_MAX_USERS> guestNames;

std::string SanitizedUserName(const char* name) {
 std::string result;
 if (name == nullptr) {
  return result;
 }
 const auto* bytes = reinterpret_cast<const unsigned char*>(name);
 std::size_t i = 0;
 while (bytes[i] != 0) {
  std::size_t length = 1;
  if (bytes[i] >= 0xF0 && bytes[i] <= 0xF4) {
   length = 4;
  } else if (bytes[i] >= 0xE0) {
   length = bytes[i] <= 0xEF ? 3 : 0;
  } else if (bytes[i] >= 0xC2) {
   length = bytes[i] <= 0xDF ? 2 : 0;
  } else if (bytes[i] >= 0x80) {
   length = 0;
  }
  bool valid = length != 0;
  for (std::size_t k = 1; valid && k < length; ++k) {
   valid = bytes[i + k] != 0 && (bytes[i + k] & 0xC0) == 0x80;
  }
  if (!valid) {
   ++i;
   continue;
  }
  const bool control = length == 1 && (bytes[i] < 0x20 || bytes[i] == 0x7F);
  if (!control && result.size() + length <= USER_SERVICE_MAX_USER_NAME_LENGTH) {
   result.append(name + i, length);
  }
  i += length;
 }
 const auto first = result.find_first_not_of(' ');
 if (first == std::string::npos) {
  return {};
 }
 return result.substr(first, result.find_last_not_of(' ') - first + 1);
}

void ReconcileUser(int userIndex) {
 const bool connected = hotplugConnected[userIndex] || guestClaimed[userIndex];
 if (userConnected[userIndex] == connected) {
  return;
 }
 userConnected[userIndex] = connected;
 if (!initialized.load()) {
  return;
 }
 constexpr std::uint32_t EventTypeLogin = 0;
 constexpr std::uint32_t EventTypeLogout = 1;
 const std::uint32_t eventType = connected ? EventTypeLogin : EventTypeLogout;
 const int userId = USER_SERVICE_INITIAL_USER_ID + userIndex;
 for (auto it = eventQueue.begin(); it != eventQueue.end(); ++it) {
  if (it->userId == userId) {
   if (it->eventType != eventType) {
    eventQueue.erase(it);
   }
   return;
  }
 }
 eventQueue.push_back({eventType, userId});
}

int UserIndex(int userId) {
    if (userId >= USER_SERVICE_INITIAL_USER_ID && userId < USER_SERVICE_INITIAL_USER_ID + USER_SERVICE_MAX_USERS) {
        return userId - USER_SERVICE_INITIAL_USER_ID;
    }
    return -1;
}

bool IsUserLoggedIn(int userId) {
 const int index = UserIndex(userId);
 if (index < 0) {
  return false;
 }
 std::lock_guard lock(userMutex);
 return userConnected[index];
}

int ReportAccessibilitySetting(int user_id, int32_t* setting, int32_t value) {
 if (!IsUserLoggedIn(user_id)) {
  return USER_SERVICE_ERROR_NOT_LOGGED_IN;
 }
 if (setting == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 *setting = value;
 return USER_SERVICE_OK;
}

void PopulateInitialLoginEvents() {
 std::lock_guard lock(userMutex);
 eventQueue.clear();
 constexpr std::uint32_t EventTypeLogin = 0;
 eventQueue.push_back({EventTypeLogin, USER_SERVICE_INITIAL_USER_ID});
 for (int i = 1; i < USER_SERVICE_MAX_USERS; ++i) {
  if (userConnected[i]) {
   eventQueue.push_back({EventTypeLogin, USER_SERVICE_INITIAL_USER_ID + i});
  }
 }
}


// Per-user keyboard settings. Nothing in AnyPS5 stores a keyboard type or input variant, so every known user reports
// the keyboard type of the configured system language and 0 for the input variants (see
// docs/dev/debt/libSceUserService.md).
static int32_t KeyboardTypeForSystemLanguage() {
 constexpr int param_id_lang = 1;
 constexpr int32_t fallback = 20;
 constexpr int32_t keyboard_types[] = {37, 20, 26, 21, 17, 28, 29, 33, 34, 38, 39, 40, 23, 35, 16,
                                       30, 31, 32, 19, 36, 22, 41, 25, 44, 48, 43, 47, 42, 46, 45};
 int language = -1;
 if (sceSystemServiceParamGetInt(param_id_lang, &language) != 0 || language < 0 ||
     language >= static_cast<int>(sizeof(keyboard_types) / sizeof(keyboard_types[0]))) {
  return fallback;
 }
 return keyboard_types[language];
}

static int32_t NoInputVariant() {
 return 0;
}

static int ReportKeyboardSetting(int user_id, int32_t* type, int32_t (*value)()) {
 if (type == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 if (UserIndex(user_id) < 0) {
  return USER_SERVICE_ERROR_UNKNOWN_USER;
 }
 *type = value();
 return USER_SERVICE_OK;
}
}

extern "C" {

void UserServiceSetUserConnected_nid_postfix(int userIndex, bool connected) {
    if (userIndex <= 0 || userIndex >= USER_SERVICE_MAX_USERS) return;
    std::lock_guard lock(userMutex);
    hotplugConnected[userIndex] = connected;
    ReconcileUser(userIndex);
}

void UserServiceSetGuestUser_nid_postfix(int userIndex, const char* name) {
    if (userIndex <= 0 || userIndex >= USER_SERVICE_MAX_USERS) return;
    std::string clean = SanitizedUserName(name);
    std::lock_guard lock(userMutex);
    guestNames[userIndex] = std::move(clean);
    guestClaimed[userIndex] = true;
    ReconcileUser(userIndex);
}

void UserServiceClearGuestUser_nid_postfix(int userIndex) {
    if (userIndex <= 0 || userIndex >= USER_SERVICE_MAX_USERS) return;
    std::lock_guard lock(userMutex);
    guestClaimed[userIndex] = false;
    guestNames[userIndex].clear();
    ReconcileUser(userIndex);
}

int APS5_VABI sceUserServiceGetAccessibilityChatTranscription(int user_id, int32_t* chat_transcription) {
 return ReportAccessibilitySetting(user_id, chat_transcription, 0);
}

int APS5_VABI sceUserServiceGetAccessibilityKeyremapEnable(std::int32_t userId, std::int32_t* enabled) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceUserServiceGetAccessibilityPressAndHoldDelay(int user_id, int32_t* press_and_hold_delay) {
 return ReportAccessibilitySetting(user_id, press_and_hold_delay, 0);
}

int APS5_VABI sceUserServiceGetAccessibilityTriggerEffect(int user_id, int32_t* trigger_effect) {
 return ReportAccessibilitySetting(user_id, trigger_effect, 1);
}

int APS5_VABI sceUserServiceGetAccessibilityVibration(int user_id, int32_t* vibration) {
 return ReportAccessibilitySetting(user_id, vibration, 1);
}

int APS5_VABI sceUserServiceGetAccessibilityZoom(std::int32_t userId, std::int32_t* zoom) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceUserServiceGetAccessibilityZoomEnabled(int user_id, int32_t* zoom_enabled) {
 return ReportAccessibilitySetting(user_id, zoom_enabled, 0);
}

int APS5_VABI sceUserServiceGetAccessibilityZoomFollowFocus(int user_id, int32_t* zoom_follow_focus) {
 return ReportAccessibilitySetting(user_id, zoom_follow_focus, 1);
}

int APS5_VABI sceUserServiceGetAgeLevel(int user_id, uint32_t* age_level) {
 if (!IsUserLoggedIn(user_id)) {
  return USER_SERVICE_ERROR_NOT_LOGGED_IN;
 }
 if (age_level == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 constexpr std::uint32_t Unrestricted = 0xFFFFFFFFu;
 *age_level = Unrestricted;
 return USER_SERVICE_OK;
}

// The initial user is reported as logging in once per initialization; afterwards there are no user events.
int APS5_VABI sceUserServiceGetEvent(SceUserServiceEvent* event) {
 if (!initialized.load()) {
  return USER_SERVICE_ERROR_NOT_INITIALIZED;
 }
 if (event == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 std::lock_guard lock(userMutex);
 if (eventQueue.empty()) {
  return USER_SERVICE_ERROR_NO_EVENT;
 }
 const auto item = eventQueue.front();
 eventQueue.pop_front();
 event->event_type = item.eventType;
 event->user_id = item.userId;
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetGamePresets(int user_id, UserServiceGamePresets* presets) {
 // System-wide game presets (difficulty, view inversion, subtitles, audio language) of the user's
 // profile. A fresh console has none set: every field is 0, "not specified", and the title falls
 // back to its own defaults. this_size is the caller's.
 const int index = UserIndex(user_id);
 if (presets == nullptr || index < 0) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 presets->difficulty = 0;
 presets->priority = 0;
 presets->invert_vertical_view_for_1st_person_view = 0;
 presets->invert_horizontal_view_for_1st_person_view = 0;
 presets->invert_vertical_view_for_3rd_person_view = 0;
 presets->invert_horizontal_view_for_3rd_person_view = 0;
 presets->display_sub_titles = 0;
 presets->audio_language = 0;
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetInitialUser(int* user_id) {
 if (user_id == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 *user_id = USER_SERVICE_INITIAL_USER_ID;
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetLoginUserIdList(UserServiceLoginUserIdList* user_id_list) {
 if (!initialized.load()) {
  return USER_SERVICE_ERROR_NOT_INITIALIZED;
 }
 if (user_id_list == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 std::lock_guard lock(userMutex);
 for (int i = 0; i < 4; ++i) {
  if (i < USER_SERVICE_MAX_USERS && userConnected[i]) {
   user_id_list->user_id[i] = USER_SERVICE_INITIAL_USER_ID + i;
  } else {
   user_id_list->user_id[i] = USER_SERVICE_USER_ID_INVALID;
  }
 }
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetUserName(int user_id, char* name, size_t size) {
 if (name == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 const int index = UserIndex(user_id);
 if (index < 0) {
  return USER_SERVICE_ERROR_UNKNOWN_USER;
 }
 if (size < USER_SERVICE_MAX_USER_NAME_LENGTH + 1) {
  std::memset(name, 0, size);
  return USER_SERVICE_ERROR_BUFFER_TOO_SHORT;
 }
 std::string guestName;
 {
  std::lock_guard lock(userMutex);
  if (guestClaimed[index]) {
   guestName = guestNames[index];
  }
 }
 std::strncpy(name, guestName.empty() ? USER_SERVICE_USER_NAMES[index] : guestName.c_str(), size);
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetUserNumber(int user_id, int32_t* number) {
 const int index = UserIndex(user_id);
 if (number == nullptr || index < 0) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 *number = index + 1;
 return USER_SERVICE_OK;
}

static bool ValidThreadPriority(int priority) {
 return priority >= USER_SERVICE_THREAD_PRIORITY_MIN && priority <= USER_SERVICE_THREAD_PRIORITY_MAX;
}

static int MarkInitialized() {
 if (initialized.exchange(true)) {
  return USER_SERVICE_ERROR_ALREADY_INITIALIZED;
 }
 PopulateInitialLoginEvents();
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceInitialize(const UserServiceInitParam* params) {
 if (params != nullptr) {
  if (!ValidThreadPriority(params->priority)) {
   return USER_SERVICE_ERROR_INVALID_ARGUMENT;
  }
 }
 return MarkInitialized();
}

int APS5_VABI sceUserServiceInitialize2(std::int32_t priority, KernelCpumask affinityMask) {
 if (!ValidThreadPriority(priority)) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 return MarkInitialized();
}

int APS5_VABI sceUserServiceTerminate(void) {
 if (!initialized.exchange(false)) {
  return USER_SERVICE_ERROR_NOT_INITIALIZED;
 }
 std::lock_guard lock(userMutex);
 eventQueue.clear();
 return USER_SERVICE_OK;
}

// No PSN account exists, so the platform privacy setting reports the feature as not permitted.
int APS5_VABI sceUserServiceGetPlatformPrivacyWs1(int32_t user_id, int32_t* value) {
    (void)user_id;
    if (!value) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
    *value = 0;
    return 0;
}

int APS5_VABI sceUserServiceGetForegroundUser(int* user_id) {
 if (user_id == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 *user_id = USER_SERVICE_INITIAL_USER_ID;
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetRegisteredUserIdList(UserServiceRegisteredUserIdList* user_id_list) {
 if (user_id_list == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 std::lock_guard lock(userMutex);
 for (int i = 0; i < 16; ++i) {
  if (i < USER_SERVICE_MAX_USERS && userConnected[i]) {
   user_id_list->user_id[i] = USER_SERVICE_INITIAL_USER_ID + i;
  } else {
   user_id_list->user_id[i] = USER_SERVICE_USER_ID_INVALID;
  }
 }
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetUserColor(int user_id, int* color) {
 const int index = UserIndex(user_id);
 if (color == nullptr || index < 0) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 *color = index;
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetKeyboardType(int user_id, int32_t* type) {
 return ReportKeyboardSetting(user_id, type, KeyboardTypeForSystemLanguage);
}

int APS5_VABI sceUserServiceGetJapaneseInputType(int user_id, int32_t* type) {
 return ReportKeyboardSetting(user_id, type, NoInputVariant);
}

int APS5_VABI sceUserServiceGetTraditionalChineseInputType(int user_id, int32_t* type) {
 return ReportKeyboardSetting(user_id, type, NoInputVariant);
}

int APS5_VABI sceUserServiceGetNpAccountId(int user_id, uint64_t* account_id) {
 const int index = UserIndex(user_id);
 if (account_id == nullptr || index < 0) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 {
  std::lock_guard lock(userMutex);
  if (guestClaimed[index]) {
   *account_id = 0;
   return USER_SERVICE_OK;
  }
 }
 NetBridgeStateView view{};
 if (NetBridgeEnabled_nid_no_patch() != 0) {
  NetBridgeState_nid_no_patch(&view);
 }
 *account_id = view.signedIn != 0 ? view.accountId : 0;
 NetBridgeRecord("libSceUserService", "sceUserServiceGetNpAccountId", true, USER_SERVICE_OK, [&](NetBridgeJson& in, NetBridgeJson& out) {
  in.Int("user_id", user_id);
  out.Uint("account_id", *account_id);
 });
 return USER_SERVICE_OK;
}

}
