#ifndef __LWINETWORK_H__
#define __LWINETWORK_H__

#define LWI_NETWORK_MACHINE_CLIENT_GUID    "{5FB45FF0-A68C-430b-8C6E-347B14AEB975}"
#define LWI_NETWORK_MACHINE_ITEM_GUID      "{5E8BFB9F-8725-4399-AB83-590B3EEE7C12}"
#define NETWORK_SERVICE_RESTART            "/etc/init.d/network restart"
#define NETWORK_CONF_FILE                  "/etc/resolv.conf"
#define NETWORK_CONF_FILE_GP               "/etc/resolv.conf.gp"

#define STATIC_PATH_BUFFER_SIZE             256

static const PSTR LWI_NETWORK_OLD_FILE       = "/etc/resolv.conf.lwidentity.orig";
static const PSTR CENTERIS_GP_DIRECTORY      = CACHEDIR "/";
static const PSTR CENTERIS_SYSVOL_PATH       = "\\Machine\\Centeris\\Identity\\";

typedef struct __DNS
{
    PSTR pszDNSSettingName;
    PSTR pszDNSSettingValue;
    struct __DNS *pNext;
} DNSSETTINGS, *PDNSSETTINGS;

typedef struct __NETWORK
{
    PSTR pszDisplayName;
    PDNSSETTINGS pDNSSettings;
    struct __NETWORK *pNext;
} NETWORKSETTINGS, *PNETWORKSETTINGS;

PNETWORKSETTINGS g_PolicyHierarchyList = NULL;
PDNSSETTINGS g_TargetDNSSettings = NULL;

CENTERROR
ProcessNetworkSettingsGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    );

CENTERROR
ResetNetworkSettingsGroupPolicy();


#endif

