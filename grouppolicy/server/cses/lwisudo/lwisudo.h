#ifndef __LWISUDO_H__
#define __LWISUDO_H__

#define LWISUDO_CLIENT_GUID "{20D139DE-D892-419f-96E5-0C3A997CB9C4}"
#define LWISUDO_ITEM_GUID "{8F413F51-94F1-4945-B37F-A04BF4E7AC73}"

static const PSTR SUDOERS_FILE          = "/etc/sudoers";
static const PSTR SUDO_TMP_FILE         = CACHEDIR "/likewise_sudoers.new";
static const PSTR CENTERIS_GP_DIRECTORY = CACHEDIR "/";
static const PSTR GP_SETTINGS_FILE      = CONFIGDIR "/grouppolicy-settings.conf";
static const PSTR CENTERIS_SYSVOL_PATH  = "\\Machine\\Centeris\\Identity\\";

typedef struct __POLICYLIST
{
    PSTR FileName;
    struct __POLICYLIST *next;
} POLICYLIST, * PPOLICYLIST;

#endif /* __LWISUDO_H__ */

