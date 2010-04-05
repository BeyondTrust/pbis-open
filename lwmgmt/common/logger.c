/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog logging utilities
 *
 */
#include "includes.h"

DWORD
LWMGMTInitBasicLogStream(
                PSTR  pszLogFilePath
                )
{

    DWORD dwError = 0;

    if (gBasicLogStreamFD != NULL)
    {
        LWMGMTCloseBasicLogStream();
    }

    if (IsNullOrEmptyString(pszLogFilePath))
    {
        gBasicLogStreamFD = stdout;
        return dwError;
    }

    gBasicLogStreamFD = fopen(pszLogFilePath, "w");
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (gBasicLogStreamFD == NULL)
    {
        gBasicLogStreamFD = stdout;
        return -1;
    }
    else
    {
        return dwError;
    }

 error:
    gBasicLogStreamFD = stdout;

    return dwError;

}

void
LWMGMTCloseBasicLogStream()
{
    if (gBasicLogStreamFD != NULL) {
    fflush(gBasicLogStreamFD);
    if (gBasicLogStreamFD != stdout) {
        fclose(gBasicLogStreamFD);
    }
    }
}



#ifdef _WIN32

DWORD
LWMGMTSetLogLevel(
    DWORD dwLogLevel
    )
{
    gLogLevel = dwLogLevel;
    if (gBasicLogStreamFD == NULL)
    {
        gBasicLogStreamFD = stdout;
    }
    return 0;
}


DWORD
LWMGMTInitLoggingToFile(
    DWORD dwLogLevel,
    PSTR  pszLogFilePath
    )
{
    return init_basic_log_stream(pszLogFilePath);
}

void
LWMGMTCloseLog()
{
    LWMGMTCloseBasicLogStream();
}

#else

static const PSTR ERROR_TAG   = "ERROR";
static const PSTR WARN_TAG    = "WARNING";
static const PSTR INFO_TAG    = "INFO";
static const PSTR VERBOSE_TAG = "VERBOSE";

static const PSTR LOG_TIME_FORMAT = "%Y%m%d%H%M%S";

#define LWMGMT__LOCK_LOGGER   pthread_mutex_lock(&gLWMGMTLogInfo.lock)
#define LWMGMT__UNLOCK_LOGGER pthread_mutex_unlock(&gLWMGMTLogInfo.lock)

DWORD
LWMGMTValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    if (dwLogLevel < 1) {
        dwError = EINVAL;
    }

    return (dwError);
}

void
LWMGMTSetSyslogMask(
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

DWORD
LWMGMTInitLoggingToSyslog(
    DWORD dwLogLevel,
    PSTR  pszIdentifier,
    DWORD dwOption,
    DWORD dwFacility
    )
{
    DWORD dwError = 0;

    LWMGMT__LOCK_LOGGER;

    dwError = LWMGMTValidateLogLevel(dwLogLevel);
    BAIL_ON_LWMGMT_ERROR(dwError);

    gLWMGMTLogInfo.logTarget = LOG_TO_SYSLOG;

    strncpy(gLWMGMTLogInfo.data.syslog.szIdentifier, pszIdentifier, PATH_MAX);
    *(gLWMGMTLogInfo.data.syslog.szIdentifier+PATH_MAX) = '\0';

    gLWMGMTLogInfo.data.syslog.dwOption = dwOption;
    gLWMGMTLogInfo.data.syslog.dwFacility = dwFacility;

    openlog(pszIdentifier, dwOption, dwFacility);

    LWMGMTSetSyslogMask(dwLogLevel);

    gLWMGMTLogInfo.bLoggingInitiated = 1;

    LWMGMT__UNLOCK_LOGGER;

    return (dwError);

    error:

    LWMGMT__UNLOCK_LOGGER;

    return (dwError);
}

DWORD
LWMGMTInitLoggingToFile(
             DWORD dwLogLevel,
             PSTR  pszLogFilePath
             )
{
    DWORD dwError = 0;

    LWMGMT__LOCK_LOGGER;

    if (IsNullOrEmptyString(pszLogFilePath))
    {
        gLWMGMTLogInfo.logTarget = LOG_TO_CONSOLE;
        gLWMGMTLogInfo.data.logfile.szLogPath[0] = '\0';
        gLWMGMTLogInfo.data.logfile.logHandle = stdout;
    }
    else
    {
        gLWMGMTLogInfo.logTarget = LOG_TO_FILE;
        strcpy(gLWMGMTLogInfo.data.logfile.szLogPath, pszLogFilePath);

        gLWMGMTLogInfo.data.logfile.logHandle = NULL;
        if (gLWMGMTLogInfo.data.logfile.szLogPath[0] != '\0') {
        gLWMGMTLogInfo.data.logfile.logHandle = freopen(gLWMGMTLogInfo.data.logfile.szLogPath, "w", stderr);
        if (gLWMGMTLogInfo.data.logfile.logHandle == NULL) {
            dwError = errno;
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        }
    }

    dwError = LWMGMTValidateLogLevel(dwLogLevel);
    BAIL_ON_LWMGMT_ERROR(dwError);

    gLWMGMTLogInfo.dwLogLevel = dwLogLevel;

    gLWMGMTLogInfo.bLoggingInitiated = 1;

    LWMGMT__UNLOCK_LOGGER;

    return (dwError);

 error:

    if (gLWMGMTLogInfo.data.logfile.logHandle != NULL) {
        fclose(gLWMGMTLogInfo.data.logfile.logHandle);
        gLWMGMTLogInfo.data.logfile.logHandle = NULL;
    }

    LWMGMT__UNLOCK_LOGGER;

    return (dwError);
}

void
LWMGMTCloseLog()
{
    LWMGMT__LOCK_LOGGER;

    if (!gLWMGMTLogInfo.bLoggingInitiated) {
        LWMGMT__UNLOCK_LOGGER;
        return;
    }

    switch (gLWMGMTLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
            {
                /* close connection to syslog */
                closelog();
            }
            break;
        case LOG_TO_FILE:
            {
                if (gLWMGMTLogInfo.data.logfile.logHandle != NULL) {
                    fclose(gLWMGMTLogInfo.data.logfile.logHandle);
                    gLWMGMTLogInfo.data.logfile.logHandle = NULL;
                }
            }
            break;
    }

    LWMGMT__UNLOCK_LOGGER;
}

void
LWMGMTLogToFileUnsafe(
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
    FILE* pTarget = NULL;

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

        default:
        {
            pszEntryType = VERBOSE_TAG;
            break;
        }
    }

    //Set pTarget
    if (logInfo->logHandle)
        pTarget = logInfo->logHandle;
    else
        pTarget = bUseErrorStream ? stderr : stdout;

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LOG_TIME_FORMAT, &tmp);

    fprintf(pTarget, "%s:0x%lx:%s:", timeBuf, (unsigned long)pthread_self(), pszEntryType);
    vfprintf(pTarget, pszFormat, msgList);
    fprintf(pTarget, "\n");
    fflush(pTarget);
}

void
LWMGMTLogToSyslogUnsafe(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            lwmgmt_sys_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            lwmgmt_sys_vsyslog(LOG_ERR, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            lwmgmt_sys_vsyslog(LOG_WARNING, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_INFO:
        {
            lwmgmt_sys_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }

        default:
        {
            lwmgmt_sys_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
    }
}

void
LWMGMTLogMessage(
    DWORD dwLogLevel,
    PSTR pszFormat,...
    )
{
    va_list argp;

    LWMGMT__LOCK_LOGGER;

    if ( !gLWMGMTLogInfo.bLoggingInitiated ||
        gLWMGMTLogInfo.logTarget == LOG_DISABLED ) {
        LWMGMT__UNLOCK_LOGGER;
        return;
    }

    if (gLWMGMTLogInfo.dwLogLevel < dwLogLevel) {
        LWMGMT__UNLOCK_LOGGER;
        return;
    }

    va_start(argp, pszFormat);

    switch (gLWMGMTLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
        {
            LWMGMTLogToSyslogUnsafe(dwLogLevel, pszFormat, argp);
            break;
        }
        case LOG_TO_FILE:
        case LOG_TO_CONSOLE:
        {
            LWMGMTLogToFileUnsafe(&gLWMGMTLogInfo.data.logfile, dwLogLevel, pszFormat, argp);
            break;
        }
    }

    va_end(argp);

    LWMGMT__UNLOCK_LOGGER;
}

DWORD
LWMGMTSetLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    LWMGMT__LOCK_LOGGER;

    dwError = LWMGMTValidateLogLevel(dwLogLevel);
    BAIL_ON_LWMGMT_ERROR(dwError);

    gLWMGMTLogInfo.dwLogLevel = dwLogLevel;

    LWMGMT__UNLOCK_LOGGER;

    return (dwError);

error:

    LWMGMT__UNLOCK_LOGGER;

    return (dwError);
}

#endif //not _WIN32
