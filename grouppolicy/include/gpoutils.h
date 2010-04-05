#ifndef __GPOUTILS_H__
#define __GPOUTILS_H__

#define BAIL_ON_NON_LWREG_ERROR(dwError) \
        if (!(40700 <= dwError && dwError <= 41200)) {  \
           BAIL_ON_CENTERIS_ERROR(dwError);            \
        }
/*
 * Log levels
 */
#define LOG_LEVEL_NOTHING 0
#define LOG_LEVEL_ALWAYS  1
#define LOG_LEVEL_ERROR   2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_INFO    4
#define LOG_LEVEL_VERBOSE 5
#define LOG_LEVEL_LOCKS   6

struct _LOGINFO_INTERNAL;

typedef struct _LOGINFO {
    pthread_mutex_t lock;
    DWORD dwLogLevel;
    struct _LOGINFO_INTERNAL* internal;
} LOGINFO, *PLOGINFO;

typedef enum {
    ERROR                  =  0,
    REQUEST_CONNECT        =  1,
    CONNECTED              =  2,
    DISCONNECT             =  3,
    REQUEST_LICENSE        =  4, /* No longer used */
    LICENSE_VALID          =  5, /* No longer used */
    LICENSE_EXPIRED        =  6, /* No longer used */
    LICENSE_ERROR          =  7, /* No longer used */
    REFRESH_GPO            =  8,
    GPO_REFRESH_FAILED     =  9,
    GPO_REFRESH_SUCCESSFUL = 10,
    JOIN_DOMAIN            = 11,
    JOIN_SUCCESSFUL        = 12,
    JOIN_FAILED            = 13,
    LEAVE_DOMAIN           = 14,
    LEAVE_SUCCESSFUL       = 15,
    LEAVE_FAILED           = 16,
    PROCESS_LOGIN          = 17,
    PROCESS_LOGIN_FAILED   = 18,
    PROCESS_LOGIN_SUCCESS  = 19,
    PROCESS_LOGOUT         = 20,
    PROCESS_LOGOUT_SUCCESS = 21,
    PROCESS_LOGOUT_FAILED  = 22,
    SET_LOG_LEVEL          = 23,
    SET_LOG_LEVEL_FAILED   = 24,
    SET_LOG_LEVEL_SUCCESSFUL   = 25
} GroupPolicyMessageType;

typedef struct GroupPolicyMessageHeaderTag {
    /* type of group policy message */
    uint8_t   messageType;
    /* protocol version */
    uint8_t   version;
    /* This may be used for sequencing
     * For instance, 1 of 10, 2 of 10
     */
    uint16_t  reserved[2];
    /* The length of the attached message
     * This is in network format
     */
    uint32_t  messageLength;
} GPOMESSAGEHEADER, *PGPOMESSAGEHEADER;

typedef struct GroupPolicyMessageTag {
    GPOMESSAGEHEADER header;
    PSTR pData;
} GPOMESSAGE, *PGPOMESSAGE;

typedef struct __GPA_CONFIG_REG GPA_CONFIG_REG, *PGPA_CONFIG_REG;

typedef enum
{
    GPATypeString,
    GPATypeDword,
    GPATypeBoolean,
    GPATypeChar,
    GPATypeEnum
} GPA_CONFIG_TYPE;

typedef struct __GPA_CONFIG_TABLE
{
    PCSTR   pszName;
    BOOLEAN bUsePolicy;
    GPA_CONFIG_TYPE Type;
    DWORD dwMin;
    DWORD dwMax;
    const PCSTR *ppszEnumNames;
    PVOID pValue;
} GPA_CONFIG_TABLE, *PGPA_CONFIG_TABLE;

CENTERROR
gpa_init_logging_to_syslog(
    DWORD dwLogLevel,
    PSTR  pszIdentifier,
    DWORD dwOption,
    DWORD dwFacility
    );

CENTERROR
gpa_set_log_level(
    DWORD dwLogLevel
    );

CENTERROR
gpa_init_logging_to_file(
    DWORD dwLogLevel,
    PSTR  pszLogFilePath
    );

void
gpa_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat, ...
    );

void
gpa_close_log();

extern LOGINFO gLogInfo;

#define GPA_LOG_ALWAYS(szFmt...)                        \
    gpa_log_message(LOG_LEVEL_ALWAYS, ## szFmt);

#define GPA_LOG_ERROR(szFmt...)                         \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {       \
        gpa_log_message(LOG_LEVEL_ERROR, ## szFmt);     \
    }

#define GPA_LOG_WARNING(szFmt...)                       \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {     \
        gpa_log_message(LOG_LEVEL_WARNING, ## szFmt);   \
    }

#define GPA_LOG_INFO(szFmt...)                          \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {     \
        gpa_log_message(LOG_LEVEL_INFO, ## szFmt);      \
    }

#define GPA_LOG_VERBOSE(szFmt...)                       \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {     \
        gpa_log_message(LOG_LEVEL_VERBOSE, ## szFmt);   \
    }

#define GPA_LOG_LOCK(szFmt...)                          \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_LOCKS) {       \
        gpa_log_message(LOG_LEVEL_VERBOSE, ## szFmt);   \
    }

#define GPA_LOG_FUNCTION_ENTER()                        \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {     \
        gpa_log_message(LOG_LEVEL_VERBOSE, "Entering function (%s)", __FUNCTION__);   \
    }

#define GPA_LOG_FUNCTION_LEAVE(ceError)                 \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {     \
        gpa_log_message(LOG_LEVEL_VERBOSE, "Exit function (%s) with status: %d (%s)", __FUNCTION__, ceError, CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");   \
    }

CENTERROR
GetDnsSystemNames(
    char** hostname,
    char** machinename,
    char** domain
    );

/* Builds a message object with the data field allocated - but, not filled in */
CENTERROR
GPOBuildMessage(
    GroupPolicyMessageType msgType,
    uint32_t msgLen,
    uint16_t iData,
    uint16_t nData,
    PGPOMESSAGE *ppMessage
    );

void
GPOFreeMessage(
    PGPOMESSAGE *ppMessage
    );

CENTERROR
GPOReadNextMessage(
    PHANDLE pHandle,
    PGPOMESSAGE *ppMessage
    );

CENTERROR
GPOSecureReadNextMessage(
    PHANDLE pHandle,
    PGPOMESSAGE *ppMessage,
    uid_t*  ppeerUID
    );

CENTERROR
GPOWriteMessage(
    PHANDLE pHandle,
    const PGPOMESSAGE pMessage
    );

CENTERROR
GPOSendCreds(
    int fd
    );

CENTERROR
GPORecvCreds(
    int fd,
    uid_t* pUid,
    uid_t* pGid
    );

CENTERROR
WriteData(
    DWORD dwFd,
    PSTR pszBuf,
    DWORD dwLen
    );

CENTERROR
ReadData(
    DWORD  dwFd,
    PSTR   pszBuf,
    DWORD  dwBytesToRead,
    PDWORD pdwBytesRead
    );

CENTERROR
GPOCrackFileSysPath(
    PSTR pszFileSysPath,
    PSTR * ppszDomainName,
    PSTR * ppszSourcePath,
    PSTR * ppszPolicyIdentifier );

CENTERROR
GPOGetSysDirBase(
    PSTR pszFileSysPath,
    PSTR *ppszSysDir,
    PSTR *ppszBase );

CENTERROR
GPAReadConfigString(
    PGPA_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    );

CENTERROR
GPAReadConfigBoolean(
    PGPA_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    );

CENTERROR
GPAReadConfigDword(
    PGPA_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    );

CENTERROR
GPAOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PGPA_CONFIG_REG *ppReg
    );
VOID
GPACloseConfig(
    PGPA_CONFIG_REG pReg
    );

CENTERROR
GPAProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PGPA_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries
    );
#endif /* __GPOUTILS_H__ */
