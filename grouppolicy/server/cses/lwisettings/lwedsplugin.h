#ifndef __LWEDSPLUGIN_H__
#define __LWEDSPLUGIN_H__

//TODO: to include new GUID given by server side team 
#define LWEDSPLUGIN_SETTINGS_ITEM_GUID      "{C697619F-177A-47c2-9B22-5B84BAE78018}"
#define LWEDSPLUGIN_SETTINGS_CLIENT_GUID    "{9024A863-BA1E-4ad9-BF5E-C0C33BE3BFCF}"

/* lwedsplugin settings */
#define MERGE_MODE_MCX                      "lwedsplugin-merge-mode-mcx"
#define FORCE_HOMEDIR_ON_STARTUP_DISK       "lwedsplugin-force-homedir-on-startup-disk"
#define USE_AD_UNC_PATH                     "lwedsplugin-use-ad-network-home-location-unc-path"
#define ALLOW_ADMINISTRATION_BY             "lwedsplugin-allow-administration-by"
#define MERGE_ADMINS                        "lwedsplugin-merge-admins"

typedef struct __lwed_policy_settings {
    BOOL bUseDefaultEnableMergeModeMCX;
    BOOL bEnableMergeModeMCX;

    BOOL bUseDefaultEnableForceHomedirOnStartupDisk;
    BOOL bEnableForceHomedirOnStartupDisk;

    BOOL bUseDefaultUseADUncForHomeLocation; 
    BOOL bUseADUncForHomeLocation;

    BOOL bUseDefaultUncProtocolForHomeLocation;
    PSTR pszUncProtocolForHomeLocation;

    BOOL bUseDefaultAllowAdministrationBy; 
    PSTR pszAllowAdministrationBy; 

    BOOL bUseDefaultEnableMergeAdmins; 
    BOOL bEnableMergeAdmins; 

} LWED_POLICY_SETTINGS, *PLWED_POLICY_SETTINGS;

CENTERROR
ApplyLWEDSPluginSettingsPolicy(
    PGPOLWIGPITEM pRsopLWEDSPLUGINSettingsItem
    );

CENTERROR
ResetLWEDSPluginSettings();

#endif //__LWEDPLUGIN_H__
