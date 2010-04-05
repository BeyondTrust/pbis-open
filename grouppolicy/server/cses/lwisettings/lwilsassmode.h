#ifndef __LWILSASSMODE_H__
#define __LWILSASSMODE_H__

/* lsassd.conf file sections  */
#define GLOBAL_SECTION                  "global"
#define PAM_SECTION                     "pam"
#define AD_PROVIDER_SECTION             "auth provider:lsa-activedirectory-provider"
#define LOCAL_PROVIDER_SECTION          "auth provider:lsa-local-provider"

/* Auth settings */
#define OFFLINE_LOGON                   "offline-logon"
#define CACHE_ENTRY_EXPIRY              "cache-entry-expiry"
#define MACHINE_PASSWD_TIMEOUT          "machine-password-timeout"
#define EXPANDED_GROUP_DEPTH            "expanded-group-depth"
#define SPACE_REPLACEMENT               "space-replacement"
#define CREATE_K5LOGIN                  "create-k5login"
#define CREATE_HOMEDIR                  "create-homedir"
#define SKEL                            "skeleton-dirs"
#define HOMEDIR_UMASK                   "homedir-umask"
#define ENABLE_EVENTLOG                 "enable-eventlog"
#define LOG_NETWORK_CONN_EVENTS         "log-network-connection-events"
#define DISPLAY_MOTD                    "display-motd"
#define ASSUME_DEFAULT_DOMAIN           "assume-default-domain"
#define SYNC_SYSTEM_TIME                "sync-system-time"
#define HOMEDIR_PREFIX_PATH             "homedir-prefix"
#define HOMEDIR_TEMPLATE                "homedir-template"
#define LOGIN_SHELL_TEMPLATE            "login-shell-template"
#define LDAP_SIGN_AND_SEAL              "ldap-sign-and-seal"
#define REFRESH_USER_CREDENTIALS        "refresh-user-credentials"
#define TRIM_USER_MEMBERSHIP            "trim-user-membership"
#define NSS_GROUP_QUERY_CACHE_ONLY      "nss-group-members-query-cache-only"
#define NSS_USER_QUERY_CACHE_ONLY       "nss-user-membership-query-cache-only"
#define NSS_ENUMERATION_ENABLED         "nss-enumeration-enabled"
#define LIKEWISE_CELL_SUPPORT           "cell-support"

/* Logon settings */
#define REQUIRE_MEMSHIP_OF              "require-membership-of"
#define USER_NOT_ALLOW_ERROR            "user-not-allowed-error"
#define LOG_LEVEL                       "log-level"
#define PASSWD_EXPIRATION_WARNING       "password-change-warning-time"
#define PASSWORD_LIFESPAN               "password-lifespan"

typedef struct __lsass_global_policy_settings {
    BOOL bUseDefaultEnableEventLog;
    BOOL bEnableEventLog;

    BOOL bUseDefaultLogNetworkConnectionEvents;
    BOOL bLogNetworkConnectionEvents;

} LSASS_GLOBAL_POLICY_SETTINGS, *PLSASS_GLOBAL_POLICY_SETTINGS;

typedef struct __lsass_pam_policy_settings {
    BOOL bUseDefaultLogLevel;
    PSTR pszLogLevel;

    BOOL bUseDefaultDisplayMotd;
    BOOL bDisplayMotd;

    BOOL bUseDefaultUserNotAllowedError;
    PSTR pszUserNotAllowedError;

} LSASS_PAM_POLICY_SETTINGS, *PLSASS_PAM_POLICY_SETTINGS;

typedef struct __lsass_ad_policy_settings {
    BOOL bUseDefaultLoginShellTemplate;
    PSTR pszLoginShellTemplate;

    BOOL bUseDefaultHomeDirPrefix;
    PSTR pszHomeDirPrefix;

    BOOL bUseDefaultHomeDirTemplate;
    PSTR pszHomeDirTemplate;

    BOOL bUseDefaultLdapSignAndSeal;
    BOOL bLdapSignAndSeal;

    BOOL bUseDefaultCacheEntryExpiry;
    DWORD dwCacheEntryExpiry; 

    BOOL bUseDefaultMachinePasswordLifespan;
    DWORD dwMachinePasswordLifespan;

    BOOL bUseDefaultSpaceReplacement;
    PSTR pszSpaceReplacement;

    BOOL bUseDefaultDomain;
    BOOL bAssumeDefaultDomain;

    BOOL bUseDefaultSyncSystemTime;
    BOOL bSyncSystemTime;

    BOOL bUseDefaultRequireMembershipOf;
    PSTR pszRequireMembershipOf;

    BOOL bUseDefaultCreateK5Login;
    BOOL bCreateK5Login;

    BOOL bUseDefaultCreateHomeDir;
    BOOL bCreateHomeDir;

    BOOL bUseDefaultHomeDirUMask;
    PSTR pszHomeDirUMask;

    BOOL bUseDefaultSkeletonDirs;
    PSTR pszSkeletonDirs;

    BOOL bUseDefaultRefreshUserCredentials;
    BOOL bRefreshUserCredentials;

    BOOL bUseDefaultCellSupport;
    PSTR pszCellSupport;

    BOOL bUseDefaultTrimUserMembership;
    BOOL bTrimUserMembership;

    BOOL bUseDefaultNssGroupMembersQueryCacheOnly;
    BOOL bNssGroupMembersQueryCacheOnly;

    BOOL bUseDefaultNssUserMembersQueryCacheOnly;
    BOOL bNssUserMembersQueryCacheOnly;

    BOOL bUseDefaultNssEnumerationEnabled;
    BOOL bNssEnumerationEnabled;

    BOOL bUseDefaultDomainManagerCheckDomainOnlineInterval;
    DWORD dwDomainManagerCheckDomainOnlineInterval; 

    BOOL bUseDefaultDomainManagerUnknownDomainCacheTimeout;
    DWORD dwDomainManagerUnknownDomainCacheTimeout; 

    BOOL bUseDefaultCacheType;
    PSTR pszCacheType;

    BOOL bUseDefaultMemoryCacheSizeCap;
    DWORD dwMemoryCacheSizeCap;
} LSASS_AD_POLICY_SETTINGS, *PLSASS_AD_POLICY_SETTINGS;

typedef struct __lsass_local_policy_settings {
    BOOL bUseDefaultLoginShellTemplate;
    PSTR pszLoginShellTemplate;

    BOOL bUseDefaultHomeDirPrefix;
    PSTR pszHomeDirPrefix;

    BOOL bUseDefaultHomeDirTemplate;
    PSTR pszHomeDirTemplate;

    BOOL bUseDefaultCreateHomeDir;
    BOOL bCreateHomeDir;

    BOOL bUseDefaultHomeDirUMask;
    PSTR pszHomeDirUMask;

    BOOL bUseDefaultSkeletonDirs;
    PSTR pszSkeletonDirs;
} LSASS_LOCAL_POLICY_SETTINGS, *PLSASS_LOCAL_POLICY_SETTINGS;

typedef struct __lsass_policy_settings {
    LSASS_GLOBAL_POLICY_SETTINGS    globalSettings;
    LSASS_PAM_POLICY_SETTINGS       pamSettings;
    LSASS_LOCAL_POLICY_SETTINGS     localSettings;
    LSASS_AD_POLICY_SETTINGS        adSettings;
} LSASS_POLICY_SETTINGS, *PLSASS_POLICY_SETTINGS;

CENTERROR
ProcessLsassSettingsMode(
    PGPOLWIGPITEM pRsopGPAuthItem,
    PGPOLWIGPITEM pRsopGPLogonItem
    );

CENTERROR
ResetLsassAuthSettings();

#endif

