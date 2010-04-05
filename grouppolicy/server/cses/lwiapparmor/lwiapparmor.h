#ifndef __LWIAPPARMOR_H__
#define __LWIAPPARMOR_H__

#define LWIAPPARMOR_CLIENT_GUID "{5554B0EB-ABE5-4654-A123-3B7818B2A48A}"
#define LWIAPPARMOR_ITEM_GUID   "{BB392F21-B75B-41af-BDFD-A3378EB51C98}"

#define SUSE_RELEASE_FILE       "/etc/SuSE-release"

#define LWIAPPARMOR_FILE        "File"
#define LWIAPPARMOR_FILE_NAME   "name"
#define LWIAPPARMOR_MODE_NAME   "mode"

#define LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH     "/sys/kernel/security/apparmor/profiles"
#define LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH_GP  CACHEDIR "/profiles.lwidentity.orig"

static const PSTR LWI_APPARMOR_PATH         = "/etc/apparmor.d/";
static const PSTR APPARMOR_INIT_FILE_SUSE   = "/etc/init.d/boot.apparmor";
static const PSTR APPARMOR_INIT_FILE_UBUNTU = "/etc/init.d/apparmor";
static const PSTR CENTERIS_GP_DIRECTORY     = CACHEDIR "/";

typedef struct __APPARMORPOLICYLIST {
  PSTR pszFileName;
  PSTR pszFilePath;
  PSTR pszMode;
  PSTR pszPolicyIdentifier;
  struct __APPARMORPOLICYLIST * pNext;
} APPARMORPOLICYLIST, *PAPPARMORPOLICYLIST;

static PAPPARMORPOLICYLIST g_PolicyHierarchyList = NULL;

#endif

