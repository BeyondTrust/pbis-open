/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        server-api.c
 *
 * Abstract:
 *
 *        Server-side API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static pthread_mutex_t gLogLock = PTHREAD_MUTEX_INITIALIZER;
static PSM_LOGGER gpLogger = NULL;
static PVOID gpLoggerData = NULL;
static LW_SM_LOG_LEVEL gMaxLevel = LW_SM_LOG_LEVEL_ALWAYS;

static
PCSTR
LwSmLogLevelToString(
    LW_SM_LOG_LEVEL level
    )
{
    switch (level)
    {
    case LW_SM_LOG_LEVEL_ALWAYS:
        return "ALWAYS";
    case LW_SM_LOG_LEVEL_ERROR:
        return "ERROR";
    case LW_SM_LOG_LEVEL_WARNING:
        return "WARNING";
    case LW_SM_LOG_LEVEL_INFO:
        return "INFO";
    case LW_SM_LOG_LEVEL_VERBOSE:
        return "VERBOSE";
    case LW_SM_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case LW_SM_LOG_LEVEL_TRACE:
        return "TRACE";
    default:
        return "UNKNOWN";
    }
}

static
PCSTR
LwSmBasename(
    PCSTR pszFilename
    )
{
    PSTR pszSlash = strrchr(pszFilename, '/');

    if (pszSlash)
    {
        return pszSlash + 1;
    }
    else
    {
        return pszFilename;
    }
}

DWORD
LwSmLogLevelNameToLogLevel(
    PCSTR pszName,
    PLW_SM_LOG_LEVEL pLevel
    )
{
    DWORD dwError = 0;

    if (!strcasecmp(pszName, "always"))
    {
        *pLevel = LW_SM_LOG_LEVEL_ALWAYS;
    }
    else if (!strcasecmp(pszName, "error"))
    {
        *pLevel = LW_SM_LOG_LEVEL_ERROR;
    }
    else if (!strcasecmp(pszName, "warning"))
    {
        *pLevel = LW_SM_LOG_LEVEL_WARNING;
    }
    else if (!strcasecmp(pszName, "info"))
    {
        *pLevel = LW_SM_LOG_LEVEL_INFO;
    }
    else if (!strcasecmp(pszName, "verbose"))
    {
        *pLevel = LW_SM_LOG_LEVEL_VERBOSE;
    }
    else if (!strcasecmp(pszName, "debug"))
    {
        *pLevel = LW_SM_LOG_LEVEL_DEBUG;
    }
    else if (!strcasecmp(pszName, "trace"))
    {
        *pLevel = LW_SM_LOG_LEVEL_TRACE;
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

error:

    return dwError;
}

static
DWORD
LwSmLogMessageInLock(
    LW_SM_LOG_LEVEL level,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszMessage
    )
{
    DWORD dwError = 0;

    if (gpLogger)
    {
        dwError = gpLogger->pfnLog(
            level,
            pszFunctionName,
            pszSourceFile,
            dwLineNumber,
            pszMessage,
            gpLoggerData
            );
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmLogMessage(
    LW_SM_LOG_LEVEL level,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszMessage
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gLogLock);

    if (level <= gMaxLevel)
    {
        dwError = LwSmLogMessageInLock(
            level,
            pszFunctionName,
            pszSourceFile,
            dwLineNumber,
            pszMessage);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    UNLOCK(bLocked, &gLogLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmSetLogger(
    PSM_LOGGER pLogger,
    PVOID pData
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gLogLock);

    if (gpLogger)
    {
        if (pLogger)
        {
            LwSmLogMessageInLock(
                LW_SM_LOG_LEVEL_ALWAYS,
                __FUNCTION__,
                __FILE__,
                __LINE__,
                "Logging redirected");
        }
        else
        {
            LwSmLogMessageInLock(
                LW_SM_LOG_LEVEL_ALWAYS,
                __FUNCTION__,
                __FILE__,
                __LINE__,
                "Logging stopped");
        }
        gpLogger->pfnClose(gpLoggerData);
        gpLogger = NULL;
        gpLoggerData = NULL;
    }

    if (pLogger)
    {
        dwError = pLogger->pfnOpen(pData);
        BAIL_ON_ERROR(dwError);

        gpLogger = pLogger;
        gpLoggerData = pData;

        LwSmLogMessageInLock(
            LW_SM_LOG_LEVEL_ALWAYS,
            __FUNCTION__,
            __FILE__,
            __LINE__,
            "Logging started");
    }

cleanup:

    UNLOCK(bLocked, &gLogLock);

    return dwError;

error:

    goto cleanup; 
}

DWORD
LwSmLogPrintf(
    LW_SM_LOG_LEVEL level,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    PSTR pszMessage = NULL;
    va_list ap;

    LOCK(bLocked, &gLogLock);

    if (level <= gMaxLevel)
    {
        va_start(ap, pszFormat);
        dwError = LwAllocateStringPrintfV(
            &pszMessage,
            pszFormat,
            ap);
        va_end(ap);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmLogMessageInLock(
            level,
            pszFunctionName,
            pszSourceFile,
            dwLineNumber,
            pszMessage);
        BAIL_ON_ERROR(dwError);
    }

    UNLOCK(bLocked, &gLogLock);

cleanup:

    LW_SAFE_FREE_MEMORY(pszMessage);

    return dwError;

error:

    goto cleanup;
}

VOID
LwSmSetMaxLogLevel(
    LW_SM_LOG_LEVEL level
    )
{
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gLogLock);

    gMaxLevel = level;

    UNLOCK(bLocked, &gLogLock);

    SM_LOG_ALWAYS("Log level changed to %s", LwSmLogLevelToString(level));
}

LW_SM_LOG_LEVEL
LwSmGetMaxLogLevel(
    VOID
    )
{
    BOOLEAN bLocked = FALSE;
    LW_SM_LOG_LEVEL level;

    LOCK(bLocked, &gLogLock);

    level = gMaxLevel;

    UNLOCK(bLocked, &gLogLock);

    return level;
}

typedef struct _SM_FILE_LOG_CONTEXT
{
    PSTR pszPath;
    int fd;
    FILE* file;
} SM_FILE_LOG_CONTEXT, *PSM_FILE_LOG_CONTEXT;

static
DWORD
LwSmLogFileOpen (
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_FILE_LOG_CONTEXT pContext = pData;
    int fd = -1;

    if (pContext->fd < 0)
    {
        fd = open(pContext->pszPath, O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (fd < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }

        pContext->fd = fd;
        fd = -1;
    }

    if (!pContext->file)
    {
        pContext->file = fdopen(pContext->fd, "w");

        if (!pContext->file)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
    }

error:

    return dwError;
}

static
DWORD
LwSmLogFileLog (
    LW_SM_LOG_LEVEL level,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszMessage,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_FILE_LOG_CONTEXT pContext = pData;
    time_t currentTime = time(NULL);
    struct tm tmp = {0};
    char timeBuf[128];

    localtime_r(&currentTime, &tmp);
    strftime(timeBuf, sizeof(timeBuf), "%Y%m%d%H%M%S", &tmp);

    switch (gMaxLevel)
    {
    case LW_SM_LOG_LEVEL_ALWAYS:
    case LW_SM_LOG_LEVEL_ERROR:
    case LW_SM_LOG_LEVEL_WARNING:
    case LW_SM_LOG_LEVEL_INFO:
    case LW_SM_LOG_LEVEL_VERBOSE:
        fprintf(
            pContext->file,
            "%s:%s: %s\n",
            timeBuf,
            LwSmLogLevelToString(level),
            pszMessage);
        break;
    case LW_SM_LOG_LEVEL_DEBUG:
    case LW_SM_LOG_LEVEL_TRACE:
        fprintf(
            pContext->file,
            "%s:%s:%s():%s:%i: %s\n",
            timeBuf,
            LwSmLogLevelToString(level),
            pszFunctionName,
            LwSmBasename(pszSourceFile),
            (int) dwLineNumber,
            pszMessage);
    }

    fflush(pContext->file);
   
    return dwError;
}

static
void
LwSmLogFileClose(
    PVOID pData
    )
{
    PSM_FILE_LOG_CONTEXT pContext = pData;

    if (pContext->file && pContext->fd >= 0)
    {
        fclose(pContext->file);
    }
    else if (pContext->fd >= 0)
    {
        close(pContext->fd);
    }
    
    LW_SAFE_FREE_MEMORY(pContext->pszPath);
    LW_SAFE_FREE_MEMORY(pContext);
}

static
DWORD
LwSmLogFileGetTargetName (
    PSTR* ppszTargetName,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_FILE_LOG_CONTEXT pContext = pData;

    if (pContext->pszPath)
    {
        dwError = LwAllocateString(
            pContext->pszPath,
            ppszTargetName);
        BAIL_ON_ERROR(dwError);
    }
    else if (pContext->file)
    {
        dwError = LwAllocateStringPrintf(
            ppszTargetName,
            "fd %i",
            fileno(pContext->file));
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateString(
            "none",
            ppszTargetName);
        BAIL_ON_ERROR(dwError);
    }

error:

    return dwError;
}

static SM_LOGGER gFileLogger =
{
    .type = LW_SM_LOGGER_FILE,
    .pfnOpen = LwSmLogFileOpen,
    .pfnLog = LwSmLogFileLog,
    .pfnClose = LwSmLogFileClose,
    .pfnGetTargetName = LwSmLogFileGetTargetName
};

static
DWORD
LwSmSyslogOpen (
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCSTR pszProgramName = pData;

    openlog(LwSmBasename(pszProgramName), 0, LOG_DAEMON);

    return dwError;
}

static
int
LwSmLogLevelToPriority(
    LW_SM_LOG_LEVEL level
    )
{
    switch (level)
    {
    case LW_SM_LOG_LEVEL_ALWAYS:
        return LOG_NOTICE;
    case LW_SM_LOG_LEVEL_ERROR:
        return LOG_ERR;
    case LW_SM_LOG_LEVEL_WARNING:
        return LOG_WARNING;
    case LW_SM_LOG_LEVEL_INFO:
        return LOG_INFO;
    case LW_SM_LOG_LEVEL_VERBOSE:
    case LW_SM_LOG_LEVEL_DEBUG:
    case LW_SM_LOG_LEVEL_TRACE:
        return LOG_DEBUG;
    default:
        return LOG_ERR;
    }
}

static
DWORD
LwSmSyslogLog (
    LW_SM_LOG_LEVEL level,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszMessage,
    PVOID pData
    )
{
    DWORD dwError = 0;

    switch (gMaxLevel)
    {
    case LW_SM_LOG_LEVEL_ALWAYS:
    case LW_SM_LOG_LEVEL_ERROR:
    case LW_SM_LOG_LEVEL_WARNING:
    case LW_SM_LOG_LEVEL_INFO:
    case LW_SM_LOG_LEVEL_VERBOSE:
        syslog(
            LwSmLogLevelToPriority(level) | LOG_DAEMON,
            "%s\n",
            pszMessage);
        break;
    case LW_SM_LOG_LEVEL_DEBUG:
    case LW_SM_LOG_LEVEL_TRACE:
        syslog(
            LwSmLogLevelToPriority(level) | LOG_DAEMON,
            "%s():%s:%i: %s\n",
            pszFunctionName,
            LwSmBasename(pszSourceFile),
            (int) dwLineNumber,
            pszMessage);
    }
   
    return dwError;
}

static
void
LwSmSyslogClose(
    PVOID pData
    )
{
    closelog();
}

static
DWORD
LwSmSyslogGetTargetName (
    PSTR* ppszTargetName,
    PVOID pData
    )
{
    DWORD dwError = 0;

    dwError = LwAllocateString(
        "LOG_DAEMON",
        ppszTargetName);
    BAIL_ON_ERROR(dwError);

error:

    return dwError;
}

static SM_LOGGER gSyslogLogger =
{
    .type = LW_SM_LOGGER_SYSLOG,
    .pfnOpen = LwSmSyslogOpen,
    .pfnLog = LwSmSyslogLog,
    .pfnClose = LwSmSyslogClose,
    .pfnGetTargetName = LwSmSyslogGetTargetName
};

DWORD
LwSmSetLoggerToFile(
    FILE* file
    )
{
    DWORD dwError = 0;
    PSM_FILE_LOG_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(sizeof(*pContext), OUT_PPVOID(&pContext));
    BAIL_ON_ERROR(dwError);

    pContext->file = file;

    dwError = LwSmSetLogger(&gFileLogger, pContext);
    BAIL_ON_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (pContext)
    {
        LwSmLogFileClose(pContext);
    }

    goto cleanup;
}


DWORD
LwSmSetLoggerToPath(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;
    PSM_FILE_LOG_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(sizeof(*pContext), OUT_PPVOID(&pContext));
    BAIL_ON_ERROR(dwError);

    pContext->fd = -1;

    dwError = LwAllocateString(pszPath, &pContext->pszPath);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmSetLogger(&gFileLogger, pContext);
    BAIL_ON_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (pContext)
    {
        LwSmLogFileClose(pContext);
    }

    goto cleanup;
}

DWORD
LwSmSetLoggerToSyslog(
    PCSTR pszProgramName
    )
{
    return LwSmSetLogger(&gSyslogLogger, (PVOID) pszProgramName);
}

DWORD
LwSmSrvGetLogInfo(
    LW_SM_LOGGER_TYPE* pType,
    PSTR* ppszTargetName
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gLogLock);

    if (!gpLogger)
    {
        *pType = LW_SM_LOGGER_NONE;
        *ppszTargetName = NULL;
    }
    else
    {
        *pType = gpLogger->type;
        dwError = gpLogger->pfnGetTargetName(ppszTargetName, gpLoggerData);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    UNLOCK(bLocked, &gLogLock);

    return dwError;

error:

    goto cleanup;
}
