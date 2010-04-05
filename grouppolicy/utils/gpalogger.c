#include "includes.h"

#define ERROR_TAG   "ERROR"
#define WARN_TAG    "WARNING"
#define INFO_TAG    "INFO"
#define VERBOSE_TAG "VERBOSE"
#define LOCK_TAG    "LOCK"

#define LOG_TIME_FORMAT "%Y%m%d%H%M%S"

/*
 * Logging targets
 */
#define LOG_DISABLED  0
#define LOG_TO_SYSLOG 1
#define LOG_TO_FILE   2

typedef struct _LOGFILEINFO {
    PSTR pszLogFilePath;
    FILE* logFileStream;
} LOGFILEINFO, *PLOGFILEINFO;

typedef struct _SYSLOGINFO {
    PSTR pszIdentifier;
    DWORD dwOption;
    DWORD dwFacility;
} SYSLOGINFO, *PSYSLOGINFO;

typedef struct _LOGINFO_INTERNAL {
    DWORD logTarget;
    union _logdata {
        LOGFILEINFO logfile;
        SYSLOGINFO syslog;
    } data;
} LOGINFO_INTERNAL, *PLOGINFO_INTERNAL;


LOGINFO gLogInfo =
{
    PTHREAD_MUTEX_INITIALIZER,
    LOG_LEVEL_ERROR,
    NULL
};

#define GPA_LOCK_LOGGER   pthread_mutex_lock(&gLogInfo.lock)
#define GPA_UNLOCK_LOGGER pthread_mutex_unlock(&gLogInfo.lock)

/* It is not safe to use BAIL_ON_CENTERIS_ERROR in here because that is re-defined to
   log and it can use the logging mutex.  So, we use an alternate logging method
   after releasing the mutex. */

#define GPA_LOGGER_LOG_INIT_ERROR(ceError, EE) \
    do { \
        if (ceError) \
        { \
            if (gLogInfo.internal && (gLogInfo.internal->logTarget != LOG_DISABLED)) \
            { \
                GPA_LOG_ALWAYS("Error at %s:%d. Error code [0x%8x]", __FILE__, EE, ceError); \
            } \
            else \
            { \
                fprintf(stderr, "Error at %s:%d. Error code [0x%8x]\n", __FILE__, EE, ceError); \
            } \
        } \
    } while (0)

static
void
gpa_close_log_internal(PLOGINFO_INTERNAL internal)
{
    if (internal)
    {
        switch (internal->logTarget)
        {
        case LOG_TO_SYSLOG:
        {
            /* close connection to syslog */
            closelog();
            LW_SAFE_FREE_STRING(internal->data.syslog.pszIdentifier);
            break;
        }
        case LOG_TO_FILE:
        {
            if (internal->data.logfile.logFileStream)
            {
                fclose(internal->data.logfile.logFileStream);
                internal->data.logfile.logFileStream = NULL;
            }
            LW_SAFE_FREE_STRING(internal->data.logfile.pszLogFilePath);
            break;
        }
        case LOG_DISABLED:
            break;
        default:
            /* ASSERT */
            break;
        }
        LwFreeMemory(internal);
    }
}

static
CENTERROR
gpa_validate_log_level(
    DWORD dwLogLevel
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (dwLogLevel < LOG_LEVEL_NOTHING) {
        ceError = CENTERROR_INVALID_VALUE;
    }

    return ceError;
}

/* UNUSED
static
void
gpa_set_syslogmask(
    DWORD dwLogLevel
    )
{
    DWORD dwSysLogLevel;

    switch (dwLogLevel)
    {
    case LOG_LEVEL_ALWAYS:
    {
        dwSysLogLevel = LOG_UPTO(LOG_INFO);
        break;
    }
    case LOG_LEVEL_ERROR:
    {
        dwSysLogLevel = LOG_UPTO(LOG_ERR);
        break;
    }

    case LOG_LEVEL_WARNING:
    {
        dwSysLogLevel = LOG_UPTO(LOG_WARNING);
        break;
    }

    case LOG_LEVEL_INFO:
    {
        dwSysLogLevel = LOG_UPTO(LOG_INFO);
        break;
    }

    default:
    {
        dwSysLogLevel = LOG_UPTO(LOG_INFO);
        break;
    }
    }

    setlogmask(dwSysLogLevel);
}
*/

CENTERROR
gpa_init_logging_to_syslog(
    DWORD dwLogLevel,
    PSTR  pszIdentifier,
    DWORD dwOption,
    DWORD dwFacility
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int EE = 0;
    PLOGINFO_INTERNAL internal = NULL;

    GPA_LOCK_LOGGER;

    gpa_close_log_internal(gLogInfo.internal);
    gLogInfo.internal = NULL;

    ceError = gpa_validate_log_level(dwLogLevel);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = LwAllocateMemory(sizeof(*internal), (PVOID*)&internal);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    internal->logTarget = LOG_TO_SYSLOG;

    ceError = LwAllocateString(pszIdentifier, &internal->data.syslog.pszIdentifier);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    internal->data.syslog.dwOption = dwOption;
    internal->data.syslog.dwFacility = dwFacility;

    openlog(pszIdentifier, dwOption, dwFacility);

    /* By default, syslog logging is enabled for all priorities.  We do our own masking
       before calling into syslog, so we do not need to set the syslog mask. */
    // gpa_set_syslogmask(dwLogLevel);

    gLogInfo.dwLogLevel = dwLogLevel;

cleanup:
    if (ceError)
    {
        gpa_close_log_internal(internal);
        internal = NULL;
    }

    gLogInfo.internal = internal;

    GPA_UNLOCK_LOGGER;

    GPA_LOGGER_LOG_INIT_ERROR(ceError, EE);
    return ceError;
}

CENTERROR
gpa_init_logging_to_file(
    DWORD dwLogLevel,
    PSTR  pszLogFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int EE = 0;
    PLOGINFO_INTERNAL internal = NULL;

    GPA_LOCK_LOGGER;

    gpa_close_log_internal(gLogInfo.internal);
    gLogInfo.internal = NULL;

    ceError = gpa_validate_log_level(dwLogLevel);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = LwAllocateMemory(sizeof(*internal), (PVOID*)&internal);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    internal->logTarget = LOG_TO_FILE;

    if (pszLogFilePath && pszLogFilePath[0])
    {
        ceError = LwAllocateString(pszLogFilePath, &internal->data.logfile.pszLogFilePath);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);

        internal->data.logfile.logFileStream = fopen(internal->data.logfile.pszLogFilePath, "w");
        if (internal->data.logfile.logFileStream == NULL)
        {
            ceError = LwMapErrnoToLwError(errno);
            CLEANUP_ON_CENTERROR_EE(ceError, EE);
        }
    }

    gLogInfo.dwLogLevel = dwLogLevel;

cleanup:
    if (ceError)
    {
        gpa_close_log_internal(internal);
        internal = NULL;
    }

    gLogInfo.internal = internal;

    GPA_UNLOCK_LOGGER;

    GPA_LOGGER_LOG_INIT_ERROR(ceError, EE);
    return ceError;
}

void
gpa_close_log()
{
    GPA_LOCK_LOGGER;

    gpa_close_log_internal(gLogInfo.internal);
    gLogInfo.internal = NULL;

    GPA_UNLOCK_LOGGER;
}

static
void
gpa_log_to_file_mt_unsafe(
    PLOGFILEINFO logInfo,
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    PSTR pszEntryType = NULL;
    BOOLEAN bUseErrorStream = FALSE;
    time_t currentTime;
    struct tm tmp;
    char timeBuf[1024];
    FILE* logStream;

    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            pszEntryType = INFO_TAG;
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            pszEntryType = ERROR_TAG;
            bUseErrorStream = TRUE;
            break;
        }
        case LOG_LEVEL_WARNING:
        {
            pszEntryType = WARN_TAG;
            bUseErrorStream = TRUE;
            break;
        }
        case LOG_LEVEL_INFO:
        {
            pszEntryType = INFO_TAG;
            break;
        }
        case LOG_LEVEL_LOCKS:
        {
            pszEntryType = LOCK_TAG;
            break;
        }
        default:
        {
            pszEntryType = VERBOSE_TAG;
            break;
        }
    }

    if (logInfo->logFileStream)
    {
        logStream = logInfo->logFileStream;
    }
    else
    {
        logStream = bUseErrorStream ? stderr : stdout;
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LOG_TIME_FORMAT, &tmp);

    fprintf(logStream, "%s:0x%lx:%s:", timeBuf, (unsigned long)pthread_self(), pszEntryType);
    vfprintf(logStream, pszFormat, msgList);
    fprintf(logStream, "\n");
    fflush(logStream);
}

static
void
gpa_log_to_syslog_mt_unsafe(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    switch (dwLogLevel)
    {
    case LOG_LEVEL_ALWAYS:
    {
        gp_vsyslog(LOG_INFO, pszFormat, msgList);
        break;
    }
    case LOG_LEVEL_ERROR:
    {
        gp_vsyslog(LOG_ERR, pszFormat, msgList);
        break;
    }

    case LOG_LEVEL_WARNING:
    {
        gp_vsyslog(LOG_WARNING, pszFormat, msgList);
        break;
    }

    case LOG_LEVEL_INFO:
    {
        gp_vsyslog(LOG_INFO, pszFormat, msgList);
        break;
    }

    default:
    {
        gp_vsyslog(LOG_INFO, pszFormat, msgList);
        break;
    }
    }
}

void
gpa_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat,...
    )
{
    va_list argp;

    GPA_LOCK_LOGGER;

    if ( !gLogInfo.internal || gLogInfo.internal->logTarget == LOG_DISABLED )
    {
        goto cleanup;
    }

    if (gLogInfo.dwLogLevel < dwLogLevel)
    {
        goto cleanup;
    }

    va_start(argp, pszFormat);

    switch (gLogInfo.internal->logTarget)
    {
    case LOG_TO_SYSLOG:
    {
        gpa_log_to_syslog_mt_unsafe(dwLogLevel, pszFormat, argp);
        break;
    }
    case LOG_TO_FILE:
    {
        gpa_log_to_file_mt_unsafe(&gLogInfo.internal->data.logfile, dwLogLevel, pszFormat, argp);
        break;
    }
    }

    va_end(argp);

cleanup:
    GPA_UNLOCK_LOGGER;
}

CENTERROR
gpa_set_log_level(
    DWORD dwLogLevel
    )
{
    DWORD ceError = CENTERROR_SUCCESS;
    DWORD dwSysLogLevel = 0;

    ceError = gpa_validate_log_level(dwLogLevel);
    CLEANUP_ON_CENTERROR(ceError);

    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            dwSysLogLevel = LOG_UPTO(LOG_ERR);
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            dwSysLogLevel = LOG_UPTO(LOG_WARNING);
            break;
        }

        case LOG_LEVEL_INFO:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }

        default:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
    }

    GPA_LOCK_LOGGER;

    gLogInfo.dwLogLevel = dwLogLevel;

    setlogmask(dwSysLogLevel);

    GPA_UNLOCK_LOGGER;

cleanup:
    return ceError;
}
