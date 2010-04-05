#ifndef __GPASTRUCT_H__
#define __GPASTRUCT_H__

typedef struct _GROUP_POLICY_CLIENT_EXTENSION {
    PSTR pszName;
    uuid_t UUID;
    PSTR pszGUID;
    PSTR pszDllName;
    DWORD dwEnableAsynchronousProcessing;
    DWORD dwNoBackgroundPolicy;
    DWORD dwNoGPOListChanges;
    DWORD dwNoMachinePolicy;
    DWORD dwNoSlowLink;
    DWORD dwNoUserPolicy;
    DWORD dwPerUserSettings;
    DWORD dwRequireSuccessfulRegistry;
    PSTR  pszProcessGroupPolicyFunction;
    PSTR  pszResetGroupPolicyFunction;
    PVOID dlHandle;
    struct _GROUP_POLICY_CLIENT_EXTENSION * pNext;
    PFNPROCESSGROUPPOLICY pfnProcessGroupPolicy;
    PFNRESETGROUPPOLICY pfnResetGroupPolicy;
} GROUP_POLICY_CLIENT_EXTENSION, *PGROUP_POLICY_CLIENT_EXTENSION;


/* Lexer & Parser */
typedef struct _GPA_TOKEN {
    DWORD dwToken;
    char  szString[MAX_TOKEN_LENGTH+1];
    DWORD dwLength;
} GPA_TOKEN, *PGPA_TOKEN;

typedef enum {
    INTEGER = 0,
    STRING
} GPAParseValueType;

typedef struct _GPA_PARSE_VALUE
{
    DWORD dwType;
    union __data {
        DWORD dwInteger;
        PSTR * ppszString;
    } data;
} GPA_PARSE_VALUE, *PGPA_PARSE_VALUE;

typedef struct _GPA_PARSE_STATEMENT
{
    PSTR pszIdentifier;
    PGPA_PARSE_VALUE pValue;
    struct _GPA_PARSE_STATEMENT *pNext;
} GPA_PARSE_STATEMENT, *PGPA_PARSE_STATEMENT;

typedef struct _GPA_PARSE_SECTION
{
    uuid_t  UUID;
    PGPA_PARSE_STATEMENT pStatement;
    struct _GPA_PARSE_SECTION * pNext;
} GPA_PARSE_SECTION, *PGPA_PARSE_SECTION;

typedef struct __GPAPOLLERINFO {
    pthread_mutex_t lock;
    pthread_cond_t  pollCondition;
    DWORD           dwPollingInterval;
    DWORD           dwRefresh;
} GPAPOLLERINFO, *PGPAPOLLERINFO;

typedef struct __GPASERVERCONNECTIONCONTEXT {
    HANDLE handle;
    uid_t peerUID;
    gid_t peerGID;
    short bConnected;
} GPASERVERCONNECTIONCONTEXT, *PGPASERVERCONNECTIONCONTEXT;

typedef enum {
    LOOPBACK_PROCESSING_MODE_USER_ONLY = 0,
    LOOPBACK_PROCESSING_MODE_REPLACE_USER,
    LOOPBACK_PROCESSING_MODE_MERGED
} LoopbackProcessingMode;

/* This structure captures the state of the Group Policy Service */
typedef struct {
    /* MT safety */
    pthread_mutex_t lock;
    /* config file path */
    PSTR pszConfigFilePath;
    /* gp policy settings file path */
    PSTR pszPolicySettingsFilePath;
    /* Extension library path */
    PSTR pszExtensionLibPath;
    /* Prefix path */
    PSTR pszPrefixPath;
    /* KRB5 Cache Env Override
     * Note: Please do not remove this
     * as long as we are using putenv to
     * make this env setting.
     */
    PSTR pszKrb5CredCacheEnvSetting;
    /* Computer polling timeout */
    DWORD dwComputerPollingInterval;
    /* User polling timeout */
    DWORD dwUserPollingInterval;
    /* Enable EventLog */
    BOOLEAN bEnableEventLog;
    /* Monitor Sudoers */
    BOOLEAN bMonitorSudoers;
    /* User policy loopback processing mode */
    LoopbackProcessingMode lpmUserPolicyMode;
    /* Process termination flag */
    BOOLEAN bProcessShouldExit;
    /* Process Exit Code */
    DWORD dwExitCode;
    PSTR pszDistroName;
    PSTR pszDistroVersion;
    /* GP agent disabled mode flag */
    BOOLEAN bDisableGP;
} GPOSERVERINFO, *PGPOSERVERINFO;

/* These are the arguments to the GP agent */
typedef struct {
    /* If this flag is positive the agent does nothing useful */
    DWORD dwDoNothing;
    /* Should start as daemon */
    DWORD dwStartAsDaemon;
    /* How much logging do you want? */
    DWORD dwLogLevel;
    /* log file path (points to static buffer) */
    PSTR pszLogFilePath;
    /* config file path (points to static buffer) */
    PSTR pszConfigFilePath;
} GP_ARGS, *PGP_ARGS;

#endif /* __GPASTRUCT_H__ */
