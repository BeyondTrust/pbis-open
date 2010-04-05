#ifndef __LWIGPSETTINGS_H__
#define __LWIGPSETTINGS_H__

#define LWIGPSETTINGS_ITEM_GUID     "{DEA51259-1884-4298-9D34-8AFBFA3F10A0}"
#define LWIGPSETTINGS_CLIENT_GUID   "{0EED766B-2404-46A6-A6B6-F8971164A920}"

typedef struct __gpagent_policy_settings {
    BOOL bUseDefaultComputerPolicyRefreshInterval;     // Set to true if no value was specified in a GPO
    DWORD dwComputerPolicyRefreshInterval;             // Time value always in seconds

    BOOL bUseDefaultUserPolicyRefreshInterval;         // Set to true if no value was specified in a GPO
    DWORD dwUserPolicyRefreshInterval;                 // Time value always in seconds

    BOOL bUseDefaultUserLoopbackMode;                  // Set to true if no value was specified in a GPO
    DWORD uplm;                                        // An enum of the possible values

    BOOL bUseDefaultMonitorSudoers;                    // Set to true if no value was specified in a GPO
    BOOL bMonitorSudoers;

    BOOL bUseDefaultEnableEventLog;                    // Set to true if no value was specified in a GPO
    BOOL bEnableEventLog;

} GPAGENT_POLICY_SETTINGS, *PGPAGENT_POLICY_SETTINGS;


CENTERROR
ApplyLwiGPSettingsPolicy(
    PGPOLWIGPITEM *ppRsopGPSettingsItem
    );


#endif /* __LWIGPSETTINGS_H__ */

