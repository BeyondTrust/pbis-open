/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwnet-logger.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Logger API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

static const PSTR ERROR_TAG   = "ERROR";
static const PSTR WARN_TAG    = "WARNING";
static const PSTR INFO_TAG    = "INFO";
static const PSTR VERBOSE_TAG = "VERBOSE";
static const PSTR DEBUG_TAG   = "DEBUG";
static const PSTR TRACE_TAG   = "TRACE";

static const PSTR LOG_TIME_FORMAT = "%Y%m%d%H%M%S";

#define LWNET_LOCK_LOGGER(bInLock) \
    do { \
        pthread_mutex_lock(&gLwnetLogInfo.lock); \
        bInLock = TRUE; \
    } while (0)

#define LWNET_UNLOCK_LOGGER(bInLock) \
    do { \
        if (bInLock) \
        { \
            pthread_mutex_unlock(&gLwnetLogInfo.lock); \
            bInLock = FALSE; \
        } \
    } while (0)

DWORD
lwnet_validate_log_level(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch (dwLogLevel)
    {
        case LWNET_LOG_LEVEL_ERROR:
        case LWNET_LOG_LEVEL_WARNING:
        case LWNET_LOG_LEVEL_INFO:
        case LWNET_LOG_LEVEL_VERBOSE:
        case LWNET_LOG_LEVEL_DEBUG:
        case LWNET_LOG_LEVEL_TRACE:
            dwError = 0;
            break;
        default:
            dwError = ERROR_INVALID_PARAMETER;
            break;
    }

    return dwError;
}

void
lwnet_set_syslogmask(
    DWORD dwLogLevel
    )
{
    DWORD dwSysLogLevel = 0;

    dwSysLogLevel = LOG_UPTO(LOG_DEBUG);

    setlogmask(dwSysLogLevel);
}

DWORD
lwnet_init_logging_to_syslog(
    DWORD   dwLogLevel,
    BOOLEAN bEnableDebug,
    PCSTR   pszIdentifier,
    DWORD   dwOption,
    DWORD   dwFacility
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    dwError = lwnet_validate_log_level(dwLogLevel);
    if (dwError)
    {
        // Since logging is not yet initialized.
        fprintf(stderr, "An invalid log level [%d] was specified.", dwLogLevel);
    }
    BAIL_ON_LWNET_ERROR(dwError);
    
    LWNET_LOCK_LOGGER(bInLock);

    gLwnetLogInfo.logTarget = LWNET_LOG_TARGET_SYSLOG;
    
    gLwnetLogInfo.bDebug = bEnableDebug;

    strncpy(gLwnetLogInfo.syslog.szIdentifier, pszIdentifier, PATH_MAX);
    *(gLwnetLogInfo.syslog.szIdentifier+PATH_MAX) = '\0';

    gLwnetLogInfo.syslog.dwOption = dwOption;
    gLwnetLogInfo.syslog.dwFacility = dwFacility;

    openlog(pszIdentifier, dwOption, dwFacility);

    lwnet_set_syslogmask(dwLogLevel);
    gLwnetLogInfo.dwLogLevel = dwLogLevel;

    gLwnetLogInfo.bLoggingInitiated = 1;

error:
    LWNET_UNLOCK_LOGGER(bInLock);

    return dwError;
}

DWORD
lwnet_init_logging_to_file(
    DWORD   dwLogLevel,
    BOOLEAN bEnableDebug,
    PSTR    pszLogFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    dwError = lwnet_validate_log_level(dwLogLevel);
    if (dwError)
    {
        // Since logging is not yet initialized.
        fprintf(stderr, "An invalid log level [%d] was specified.", dwLogLevel);
        goto error;
    }

    LWNET_LOCK_LOGGER(bInLock);

    gLwnetLogInfo.logTarget = LWNET_LOG_TARGET_FILE;
    gLwnetLogInfo.bDebug = bEnableDebug;

    if (IsNullOrEmptyString(pszLogFilePath))
    {
        gLwnetLogInfo.bLogToConsole = TRUE;
        gLwnetLogInfo.logfile.logHandle = stdout;
    }
    else
    {
        strncpy(gLwnetLogInfo.logfile.szLogPath, pszLogFilePath, PATH_MAX);
        *(gLwnetLogInfo.logfile.szLogPath+PATH_MAX) = '\0';

        gLwnetLogInfo.logfile.logHandle = NULL;
        if (gLwnetLogInfo.logfile.szLogPath[0] != '\0')
        {
            gLwnetLogInfo.logfile.logHandle = fopen(gLwnetLogInfo.logfile.szLogPath, "w");
            if (gLwnetLogInfo.logfile.logHandle == NULL)
            {
                dwError = LwMapErrnoToLwError(errno);
                fprintf(stderr, "Failed to redirect logging. %s", strerror(errno));
                goto error;
            }
        }
    }


    gLwnetLogInfo.dwLogLevel = dwLogLevel;

    gLwnetLogInfo.bLoggingInitiated = 1;

cleanup:
    LWNET_UNLOCK_LOGGER(bInLock);

    return dwError;

error:
    if (!gLwnetLogInfo.bLogToConsole &&
        (gLwnetLogInfo.logfile.logHandle != NULL))
    {
        fclose(gLwnetLogInfo.logfile.logHandle);
        gLwnetLogInfo.logfile.logHandle = NULL;
    }

    goto cleanup;
}

void
lwnet_close_log(
    void
    )
{
    BOOLEAN bInLock = FALSE;

    LWNET_LOCK_LOGGER(bInLock);

    if (!gLwnetLogInfo.bLoggingInitiated)
    {
        goto cleanup;
    }

    switch (gLwnetLogInfo.logTarget)
    {
        case LWNET_LOG_TARGET_SYSLOG:
            {
                /* close connection to syslog */
                closelog();
            }
            break;
        case LWNET_LOG_TARGET_FILE:
            {
                if (!gLwnetLogInfo.bLogToConsole &&
                    (gLwnetLogInfo.logfile.logHandle != NULL))
                {
                    fclose(gLwnetLogInfo.logfile.logHandle);
                    gLwnetLogInfo.logfile.logHandle = NULL;
                }
            }
            break;
    }

cleanup:
    LWNET_UNLOCK_LOGGER(bInLock);
}

static PSTR logLevel2EntryType(
    DWORD dwLogLevel
    )
{
    switch (dwLogLevel)
    {
        case LWNET_LOG_LEVEL_ALWAYS:
            return INFO_TAG;
        case LWNET_LOG_LEVEL_ERROR:
            return(ERROR_TAG);
        case LWNET_LOG_LEVEL_WARNING:
            return(WARN_TAG);
        case LWNET_LOG_LEVEL_INFO:
            return(INFO_TAG);
        case LWNET_LOG_LEVEL_VERBOSE:
            return(VERBOSE_TAG);
        case LWNET_LOG_LEVEL_DEBUG:
            return(DEBUG_TAG);
        case LWNET_LOG_LEVEL_TRACE:
            return(TRACE_TAG);
    }

    return "UNKNOWN";
}

static void
lwnet_log_to_file_mt_unsafe(
    LOGINFO *pgLwnetLogInfo,
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp;
    char timeBuf[1024];
    FILE* pTarget = pgLwnetLogInfo->logfile.logHandle;

    switch (dwLogLevel)
    {
        case LWNET_LOG_LEVEL_ERROR:
        case LWNET_LOG_LEVEL_WARNING:
            pTarget = pTarget ? pTarget : stderr;
            break;
        default:
            pTarget = pTarget ? pTarget : stdout;
            break;
    }

    pszEntryType = logLevel2EntryType(dwLogLevel);
    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LOG_TIME_FORMAT, &tmp);

    fprintf(pTarget, "%s:0x%lx:%s:", timeBuf, (unsigned long) pthread_self(), pszEntryType);
    vfprintf(pTarget, pszFormat, msgList);
    fprintf(pTarget, "\n");
    fflush(pTarget);
}

void
lwnet_log_to_syslog_mt_unsafe(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    switch (dwLogLevel)
    {
        case LWNET_LOG_LEVEL_ALWAYS:
        {
            lwnet_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
        case LWNET_LOG_LEVEL_ERROR:
        {
            lwnet_vsyslog(LOG_ERR, pszFormat, msgList);
            break;
        }

        case LWNET_LOG_LEVEL_WARNING:
        {
            lwnet_vsyslog(LOG_WARNING, pszFormat, msgList);
            break;
        }

        case LWNET_LOG_LEVEL_INFO:
        case LWNET_LOG_LEVEL_VERBOSE:
        {
            lwnet_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }

        default:
        {
            lwnet_vsyslog(LOG_DEBUG, pszFormat, msgList);
            break;
        }
    }
}

void
lwnet_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat,...
    )
{
    BOOLEAN bInLock = FALSE;
    va_list argp;
  
    LWNET_LOCK_LOGGER(bInLock);

    if ( !gLwnetLogInfo.bLoggingInitiated ||
         gLwnetLogInfo.logTarget == LWNET_LOG_TARGET_DISABLED )
    {
        goto cleanup;
    }

    if (gLwnetLogInfo.dwLogLevel < dwLogLevel)
    {
        goto cleanup;
    }

    va_start(argp, pszFormat);

    switch (gLwnetLogInfo.logTarget)
    {
        case LWNET_LOG_TARGET_SYSLOG:
        {
            lwnet_log_to_syslog_mt_unsafe(dwLogLevel, pszFormat, argp);
            break;
        }
        case LWNET_LOG_TARGET_FILE:
        {
            lwnet_log_to_file_mt_unsafe(&gLwnetLogInfo, dwLogLevel, pszFormat, argp);
            break;
        }
    }

    va_end(argp);

cleanup:
    LWNET_UNLOCK_LOGGER(bInLock);
}

DWORD
lwnet_set_log_level(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    dwError = lwnet_validate_log_level(dwLogLevel);
    BAIL_ON_LWNET_ERROR(dwError);

    LWNET_LOCK_LOGGER(bInLock);
    gLwnetLogInfo.dwLogLevel = dwLogLevel;

error:
    LWNET_UNLOCK_LOGGER(bInLock);

    return dwError;
}

DWORD
lwnet_get_log_info(
    OUT PDWORD pdwLogLevel,
    OUT PDWORD pdwLogTarget,
    OUT PSTR* ppszLogPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    DWORD dwLogLevel = 0;
    DWORD dwLogTarget = 0;
    PSTR pszLogPath = NULL;

    LWNET_LOCK_LOGGER(bInLock);

    dwLogLevel = gLwnetLogInfo.dwLogLevel;
    dwLogTarget = gLwnetLogInfo.logTarget;

    if (LWNET_LOG_TARGET_FILE == dwLogTarget)
    {
        if (gLwnetLogInfo.bLogToConsole)
        {
            dwLogTarget = LWNET_LOG_TARGET_CONSOLE;
        }
        else
        {
            dwError = LWNetAllocateString(
                            gLwnetLogInfo.logfile.szLogPath,
                            &pszLogPath);
            // cannot use bail macro because it logs and we are in the lock.
            if (dwError)
            {
                goto error;
            }
        }
    }

error:
    LWNET_UNLOCK_LOGGER(bInLock);

    if (dwError)
    {
        dwLogLevel = 0;
        dwLogTarget = 0;
        LWNET_SAFE_FREE_STRING(pszLogPath);
    }

    *pdwLogLevel = dwLogLevel;
    *pdwLogTarget = dwLogTarget;
    *ppszLogPath = pszLogPath;

    return dwError;
}

