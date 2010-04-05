#ifndef __LWIMACPOLICY_H__
#define __LWIMACPOLICY_H__

#define LWI_MAC_POLICY_GUID     "{A36E3651-9910-4183-859B-2E077140FBCB}"

#define MAC_GP_BUNDLE_PATH      LIBDIR "/grouppolicy/liblwimacinterface.so"
#define MAC_GP_CACHE_DIR_PATH   CACHEDIR "/scratch/" LWI_MAC_POLICY_GUID
#define MAC_GP_SYSTEM_BACKUP    MAC_GP_CACHE_DIR_PATH "/mac_policy_system_default.xml"

#define SETTING_TAG             "setting"
#define MACPOLICIES_TAG         "macpolicies"
#define NAME_TAG                "name"
#define TYPE_TAG                "type"
#define TYPE_BOOLEAN            "boolean"
#define TYPE_NUMBER             "number"
#define TYPE_CUSTOM             "custom"
#define TRUE_TAG                "true"
#define FALSE_TAG               "false"

#define GP_MAC_ITF_SETTING_NOT_SUPPORTED                        15

#define REQUIRE_PASSWORD_FOR_EACH_SECURE_SYSTEM_PREFERENCE_TAG  "mac require password for each secure system preference"
#define ENABLE_AUTOMATIC_LOGIN_TAG                              "mac disable automatic login"
#define USE_SECURE_VIRTUAL_MEMORY_TAG                           "mac use secure virtual memory"
#define MINUTES_OF_INACTIVITY_TO_LOGOUT_TAG                     "mac logout in minutes of inactivity"
#define ENABLE_FIREWALL_TAG                                     "mac enable firewall"
#define ENABLE_FIREWALL_LOGGING_TAG                             "mac enable firewall logging"
#define BLOCK_UDP_TRAFFIC_TAG                                   "mac block udp traffic"
#define ENABLE_STEALTH_MODE_TAG                                 "mac enable stealth mode"
#define ENABLE_BLUETOOTH_TAG                                    "bluetooth controller state"
#define BLUETOOTH_SETUP_ASSISTANT_TAG                           "BluetoothAutoSeekHIDDevices"
#define BLUETOOTH_SHARE_INTERNET_CONNECTION_TAG                 "PANServices"
#define DNS_SETTING_TAG                                         "DNS Settings"
#define DNS_SERVERS_TAG                                         "ServerAddresses"
#define DNS_SEARCH_DOMAINS_TAG                                  "SearchDomains"
#define APPLETALK_TAG                                           "Apple Talk"
#define APPLETALK_NODEID_TAG                                    "NodeId"
#define APPLETALK_NETID_TAG                                     "NetworkId"
#define APPLETALK_MODE_TAG                                      "AppleTalkMode"
#define ENERGY_SAVER_SYSTEM_SLEEP_TIMER                         "System Sleep Timer"
#define ENERGY_SAVER_DISPLAY_SLEEP_TIMER                        "Display Sleep Timer"
#define ENERGY_SAVER_DISK_SLEEP_TIMER                           "Disk Sleep Timer"
#define ENERGY_SAVER_WAKE_ON_MODEM_RING                         "Wake On Modem Ring"
#define ENERGY_SAVER_WAKE_ON_LAN                                "Wake On LAN"
#define ENERGY_SAVER_SLEEP_ON_POWER_BUTTON                      "Sleep On Power Button"
#define ENERGY_SAVER_AUTO_RESTART_ON_POWER_LOSS                 "Automatic Restart On Power Loss"

static PCSTR g_pszGPItemGUIDList[] = { "{83857CE8-1FA8-43C6-AD07-00B807874087}",
                                       "{0B06B245-FE4C-4808-A100-D52F4D74B7CB}",
                                       "{6642CE9C-B2F4-4bdb-9F07-890939B0FE86}",
                                       "{3BCB95AD-2DA2-438a-BD5C-4CA5BFBFDD88}",
                                       "{5842C39F-D134-4a3c-ABD1-D6D269ADE954}",
                                       "{2B068E9E-5BC6-4fb7-B292-539B7A4D92A3}",
                                       "{5E8BFB9F-8725-4399-AB83-590B3EEE7C12}"};

static PCSTR g_pszGPItemQueryList[] = { "macSecuritySettings/section/setting",
                                        "macFirewallSettings/section/setting",
                                        "macBluetoothSettings/section/setting",
                                        "macEnergySaverSleep/section/setting",
                                        "macEnergySaverOption/section/setting",
                                        "macInternetNetworkSettings/section/setting",
                                        "NetworkSettings/section/setting"};

#if 0
static PCSTR g_pszClientGUIDList[] = { "{A36E3651-9910-4183-859B-2E077140FBCB}",
                                       "{5FB45FF0-A68C-430b-8C6E-347B14AEB975}"};
#endif

typedef enum
{
    MAC_SETTING_BOOLEAN = 0,
    MAC_SETTING_NUMBER,
    MAC_SETTING_FIREWALL,
    MAC_SETTING_APPLETALK,
    MAC_SETTING_DNS
} MacSettingType;

typedef struct __MAC_SETTING
{
    MacSettingType settingType;
    void*          pValue;
} MAC_SETTING, *PMAC_SETTING;

typedef struct __APPLETALK_SETTING
{
    int    mode;
    int    nodeId;
    int    networkId;
} APPLETALK_SETTING, *PAPPLETALK_SETTING;

typedef struct __DNS_SETTING
{
    PSTR pszServerAddresses;
    PSTR pszSearchDomains;
} DNS_SETTING, *PDNS_SETTING;


typedef int (*PFNRequirePasswordForEachSecureSystemPreference)(int bValue);
typedef int (*PFNIsPasswordRequiredForEachSecureSystemPreference)(int* pbValue);

typedef int (*PFNDisableAutomaticLogin)(int bValue);
typedef int (*PFNIsAutomaticLoginDisabled)(int* pbValue);

typedef int (*PFNUseSecureVirtualMemory)(int bValue);
typedef int (*PFNIsSecureVirtualMemoryUsed)(int* pbValue);

typedef int (*PFNLogoutInMinutesOfInactivity)(int nMinutes);
typedef int (*PFNGetMinutesOfInactivityToLogout)(int* pNMinutes);

typedef int (*PFNEditFirewallState)(int bValue);
typedef int (*PFNIsFirewallEnabled)(int* pbValue);

typedef int (*PFNEnableFirewallLogging)(int bValue);
typedef int (*PFNIsFirewallLoggingEnabled)(int* pbValue);

typedef int (*PFNBlockUDPTraffic)(int bValue);
typedef int (*PFNIsUDPTrafficBlocked)(int* pbValue);

typedef int (*PFNEnableStealthMode)(int bValue);
typedef int (*PFNIsStealthModeEnabled)(int* pbValue);

typedef int (*PFNEnableBluetoothController)(int bValue);
typedef int (*PFNIsBluetoothEnabled)(int* pbValue);

typedef int (*PFNApplyBluetoothSetupAssistantSetting)(int bValue);
typedef int (*PFNIsBluetoothSetupAssistantEnabled)(int* pbValue);

typedef int (*PFNApplyBluetoothShareInternetConnectionSetting)(int bValue);
typedef int (*PFNIsBluetoothShareInternetConnectionEnabled)(int* pbValue);

typedef int (*PFNApplyDNSSettings)(PSTR, PSTR);
typedef int (*PFNApplyAppleTalkSettings)(int,int,int );
typedef int (*PFNApplyEnergySaverSettings)(int bValue, char *pszSettingName);

typedef struct __MAC_GP_BUNDLE_FUNCTIONS
{
    PFNRequirePasswordForEachSecureSystemPreference     GPRequirePasswordForEachSecureSystemPreference;
    PFNIsPasswordRequiredForEachSecureSystemPreference  GPIsPasswordRequiredForEachSecureSystemPreference;
    PFNDisableAutomaticLogin                            GPDisableAutomaticLogin;
    PFNIsAutomaticLoginDisabled                         GPIsAutomaticLoginDisabled;
    PFNUseSecureVirtualMemory                           GPUseSecureVirtualMemory;
    PFNIsSecureVirtualMemoryUsed                        GPIsSecureVirtualMemoryUsed;
    PFNLogoutInMinutesOfInactivity                      GPLogoutInMinutesOfInactivity;
    PFNGetMinutesOfInactivityToLogout                   GPGetMinutesOfInactivityToLogout;
    PFNEditFirewallState                                GPEditFirewallState;
    PFNIsFirewallEnabled                                GPIsFirewallEnabled;
    PFNEnableFirewallLogging                            GPEnableFirewallLogging;
    PFNIsFirewallLoggingEnabled                         GPIsFirewallLoggingEnabled;
    PFNBlockUDPTraffic                                  GPBlockUDPTraffic;
    PFNIsUDPTrafficBlocked                              GPIsUDPTrafficBlocked;
    PFNEnableStealthMode                                GPEnableStealthMode;
    PFNIsStealthModeEnabled                             GPIsStealthModeEnabled;
    PFNEnableBluetoothController                        GPEnableBluetoothControllerState;
    PFNApplyBluetoothSetupAssistantSetting              GPApplyBluetoothSetupAssistantSetting;
    PFNApplyBluetoothShareInternetConnectionSetting     GPApplyBluetoothShareInternetConnectionSetting;
    PFNApplyAppleTalkSettings                           GPApplyAppleTalkSettings;
    PFNApplyDNSSettings                                 GPApplyDNSSettings;
    PFNApplyEnergySaverSettings                         GPApplyEnergySaverSettings;

} MAC_GP_BUNDLE_FUNCTIONS;

typedef struct __MAC_GP_BUNDLE_MODULE
{
    void* LibHandle;
    char* pszError;
    MAC_GP_BUNDLE_FUNCTIONS functions;
} MAC_GP_BUNDLE_MODULE;

#endif /* __LWIMACPOLICY_H__ */
