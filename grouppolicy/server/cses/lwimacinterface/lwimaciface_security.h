#ifndef __LWIMACINTERFACE_SECURITY_H__
#define __LWIMACINTERFACE_SECURITY_H__

#define AUTHORIZATION_PLIST_FILE_PATH           ETC_FOLDER_PATH "/authorization"
#define HOST_CONFIG_FILE_PATH                   ETC_FOLDER_PATH "/hostconfig"
#define GLOBAL_PREFERENCES_PLIST_FILE_PATH      LIBRARY_PREFERENCES_PATH "/.GlobalPreferences.plist"
#define LOGINWINDOW_PLIST_FILE_PATH             LIBRARY_PREFERENCES_PATH "/com.apple.loginwindow.plist"
#define VIRTUAL_MEMORY_FILE_PATH_LEOPARD        LIBRARY_PREFERENCES_PATH "/com.apple.virtualMemory.plist"

#define AUTOLOGIN_USER_UID_TAG                  "autoLoginUserUID"
#define AUTOLOGIN_USER_TAG                      "autoLoginUser"
#define MINUTES_OF_INACTIVITY_TAG               "com.apple.autologout.AutoLogOutDelay"
#define DISABLE_AUTO_LOGIN_KEY_TAG              "com.apple.userspref.DisableAutoLogin"
#define ENCRYPTSWAP_TAG                         "ENCRYPTSWAP"
#define SECURE_VIRTUAL_MEMORY_TAG_LEOPARD       "UseEncryptedSwap"
#define RIGHTS_TAG                              "rights"
#define SYSTEM_PREFERENCES_TAG                  "system.preferences"
#define SHARED_TAG                              "shared"

int
RequirePasswordForEachSecureSystemPreference(
    int bValue
    );

int
IsPasswordForEachSecureSystemPreferenceRequired(
    int* pbValue
    );

int
DisableAutomaticLogin(
    int bValue
    );

int
IsAutomaticLoginDisabled(
    int* pbValue
    );

int
UseSecureVirtualMemory(
    int bValue
    );

int
IsSecureVirtualMemoryUsed(
    int* pbValue
    );

int
LogoutInMinutesOfInactivity(
    int nMinutes
    );

int
GetMinutesOfInactivityToLogout(
    int* pNMinutes
    );

#endif

