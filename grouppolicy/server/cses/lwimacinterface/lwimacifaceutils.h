#include "lwimaciface_errcodes.h"

#define RIGHTS_TAG                              "rights"
#define SYSTEM_PREFERENCES_TAG                  "system.preferences"
#define SHARED_TAG                              "shared"

#define ETC_FOLDER_PATH                         "/etc"
#define LIBRARY_PREFERENCES_PATH                "/Library/Preferences"
#define GLOBAL_PREFERENCES_PLIST_FILE_PATH      LIBRARY_PREFERENCES_PATH "/.GlobalPreferences.plist"
#define SYSTEM_VERSION_PLIST_FILE_PATH          "/System/Library/CoreServices/SystemVersion.plist"

#define SYSTEM_VERSION_KEY                      "ProductVersion"

#define ASSERT_QUERY(__ceError__)                           \
    do {                                                    \
        if ((__ceError__) == NULL) {                        \
            result = GP_MAC_ITF_FAILED_QUERY_DICTIONARY;    \
            goto cleanup;                                   \
        }                                                   \
    } while (0)

#define CHECK_SETTING_SUPPORT                               \
    int nValue = 0;                                         \
    IsLeopardSystem(&nValue);                               \
    if(!nValue) {                                           \
	    result = GP_MAC_ITF_SETTING_NOT_SUPPORTED;          \
	    goto cleanup;                                       \
    }

char*
StripWhiteSpaceFromFront(
    char* pszLine
    );

void
NotifyLoginWindow();

int
IsLeopardSystem(
    int *pbIsLeopard
    );
