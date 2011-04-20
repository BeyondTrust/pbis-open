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

typedef struct _LOGINFO
{
    PSM_LOGGER pLogger;
    PVOID pLoggerData;
    LW_SM_LOG_LEVEL MaxLevel;
} LOGINFO, *PLOGINFO;

static SM_LOGGER gDefaultLogger = {.type = LW_SM_LOGGER_DEFAULT};
static pthread_mutex_t gLogLock = PTHREAD_MUTEX_INITIALIZER;
static LOGINFO gLogInfo =
{
    .pLogger = NULL,
    .pLoggerData = NULL,
    .MaxLevel = LW_SM_LOG_LEVEL_ALWAYS
};
static PLW_HASHMAP gpLogMap = NULL;

static PCSTR gpProcessName = NULL;

DWORD
LwSmLoggingInit(
    PCSTR pProcessName
    )
{
    DWORD dwError = 0;

    dwError = LwNtStatusToWin32Error(LwRtlCreateHashMap(
        &gpLogMap,
        LwRtlHashDigestPstr,
        LwRtlHashEqualPstr,
        NULL));
    BAIL_ON_ERROR(dwError);

    gpProcessName = pProcessName;

error:

    return dwError;
}

static
void
LogInfoClear(
    PLW_HASHMAP_PAIR pPair,
    PVOID pUserData
    )
{
    PLOGINFO pInfo = pPair->pValue;

    LwFreeMemory(pPair->pKey);

    if (pInfo->pLoggerData)
    {
        pInfo->pLogger->pfnClose(pInfo->pLoggerData);
    }

    LwFreeMemory(pInfo);
}

VOID
LwSmLoggingShutdown(
    VOID
    )
{
    if (gpLogMap)
    {
        LwRtlHashMapClear(gpLogMap, LogInfoClear, NULL);
        LwRtlFreeHashMap(&gpLogMap);
    }

    if (gLogInfo.pLoggerData)
    {
        gLogInfo.pLogger->pfnClose(gLogInfo.pLoggerData);
    }
}

static
DWORD
LwSmFindOrCreateLogInfo(
    PCSTR pFacility,
    PLOGINFO* ppInfo
    )
{
    DWORD dwError = 0;
    PLOGINFO pInfo = NULL;
    PSTR pDup = NULL;

    if (!pFacility)
    {
        pInfo = &gLogInfo;
    }
    else if (LwRtlHashMapFindKey(gpLogMap, OUT_PPVOID(&pInfo), pFacility) != ERROR_SUCCESS)
    {
        dwError = LwAllocateMemory(sizeof(*pInfo), OUT_PPVOID(&pInfo));
        BAIL_ON_ERROR(dwError);

        pInfo->pLogger = &gDefaultLogger;
        pInfo->MaxLevel = LW_SM_LOG_LEVEL_DEFAULT;

        dwError = LwAllocateString(pFacility, &pDup);
        BAIL_ON_ERROR(dwError);

        dwError = LwRtlHashMapInsert(gpLogMap, pDup, pInfo, NULL);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    *ppInfo = pInfo;

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pInfo);
    LW_SAFE_FREE_MEMORY(pDup);

    goto cleanup;
}

static
VOID
LwSmResolveLogInfo(
    PCSTR pFacility,
    PLW_SM_LOG_LEVEL pMaxLevel,
    PLOGINFO* ppInfo
    )
{
    PLOGINFO pInfo = NULL;
    LW_SM_LOG_LEVEL maxLevel = 0;

    if (!pFacility || LwRtlHashMapFindKey(gpLogMap, OUT_PPVOID(&pInfo), pFacility) != ERROR_SUCCESS)
    {
        pInfo = &gLogInfo;
    }

    /* Resolve defaults */
    maxLevel = pInfo->MaxLevel == LW_SM_LOG_LEVEL_DEFAULT ? gLogInfo.MaxLevel : pInfo->MaxLevel;
    pInfo = pInfo->pLogger == &gDefaultLogger ? &gLogInfo : pInfo;

    *pMaxLevel = maxLevel;
    *ppInfo = pInfo;
}


static
VOID
LwSmRtlLogCallback(
    LW_IN LW_OPTIONAL LW_PVOID Context,
    LW_IN LW_RTL_LOG_LEVEL Level,
    LW_IN LW_OPTIONAL LW_PCSTR ComponentName,
    LW_IN LW_PCSTR FunctionName,
    LW_IN LW_PCSTR FileName,
    LW_IN LW_ULONG LineNumber,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    )
{
    va_list ap;

    va_start(ap, Format);
    (void) LwSmLogPrintfv(
        Level,
        ComponentName,
        FunctionName,
        FileName,
        LineNumber,
        Format,
        ap);
    va_end(ap);
}

VOID
LwSmLogInit(
    VOID
    )
{
    LwRtlLogSetCallback(LwSmRtlLogCallback, NULL);
}

static
PCSTR
LwSmLogLevelToString(
    LW_SM_LOG_LEVEL level
    )
{
    switch (level)
    {
    case LW_SM_LOG_LEVEL_DEFAULT:
        return "default";
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
#if defined(TOP_SRCDIR) && defined(TOP_OBJDIR)
    if (!strncmp(pszFilename, TOP_SRCDIR "/", strlen(TOP_SRCDIR "/")))
    {
        return pszFilename + strlen(TOP_SRCDIR "/");
    }
    else if (!strncmp(pszFilename, TOP_OBJDIR "/", strlen(TOP_OBJDIR "/")))
    {
        return pszFilename + strlen(TOP_OBJDIR "/");
    }
    else
    {
        return pszFilename;
    }
#else
    PSTR pszSlash = strrchr(pszFilename, '/');

    if (pszSlash)
    {
        return pszSlash + 1;
    }
    else
    {
        return pszFilename;
    }
#endif
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
    PCSTR pszFacility,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszMessage
    )
{
    DWORD dwError = 0;
    LW_SM_LOG_LEVEL maxLevel = 0;
    PLOGINFO pInfo = NULL;

    LwSmResolveLogInfo(pszFacility, &maxLevel, &pInfo);

    if (pInfo->pLogger && level <= maxLevel)
    {
        dwError = pInfo->pLogger->pfnLog(
            level,
            maxLevel,
            pszFacility,
            pszFunctionName,
            pszSourceFile,
            dwLineNumber,
            pszMessage,
            pInfo->pLoggerData);
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
    PCSTR pszFacility,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszMessage
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gLogLock);

    dwError = LwSmLogMessageInLock(
        level,
        pszFacility,
        pszFunctionName,
        pszSourceFile,
        dwLineNumber,
        pszMessage);
    BAIL_ON_ERROR(dwError);

cleanup:

    UNLOCK(bLocked, &gLogLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmSetLogger(
    PCSTR pFacility,
    PSM_LOGGER pLogger,
    PVOID pData
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    PLOGINFO pInfo = NULL;
    LW_HASHMAP_PAIR pair = {0};

    LOCK(bLocked, &gLogLock);

    dwError = LwSmFindOrCreateLogInfo(pFacility, &pInfo);
    BAIL_ON_ERROR(dwError);

    if (pInfo->pLogger)
    {
        if (pLogger)
        {
            LwSmLogMessageInLock(
                LW_SM_LOG_LEVEL_ALWAYS,
                pFacility,
                __FUNCTION__,
                __FILE__,
                __LINE__,
                "Logging redirected");
        }
        else
        {
            LwSmLogMessageInLock(
                LW_SM_LOG_LEVEL_ALWAYS,
                pFacility,
                __FUNCTION__,
                __FILE__,
                __LINE__,
                "Logging stopped");
        }

        if (pInfo->pLogger->pfnClose)
        {
            pInfo->pLogger->pfnClose(pInfo->pLoggerData);
        }

        pInfo->pLogger = NULL;
        pInfo->pLoggerData = NULL;
    }

    if (pLogger == &gDefaultLogger)
    {
        if (pInfo->MaxLevel == LW_SM_LOG_LEVEL_DEFAULT)
        {
            LwRtlHashMapRemove(gpLogMap, pFacility, &pair);
            LogInfoClear(&pair, NULL);
        }
    }
    else if (pLogger)
    {
        dwError = pLogger->pfnOpen(pData);
        BAIL_ON_ERROR(dwError);

        pInfo->pLogger = pLogger;
        pInfo->pLoggerData = pData;

        LwSmLogMessageInLock(
            LW_SM_LOG_LEVEL_ALWAYS,
            pFacility,
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
LwSmLogPrintfv(
    LW_SM_LOG_LEVEL level,
    PCSTR pszFacility,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszFormat,
    va_list ap
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszMessage = NULL;

    dwError = LwAllocateStringPrintfV(
        &pszMessage,
        pszFormat,
        ap);
    va_end(ap);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmLogMessageInLock(
        level,
        pszFacility,
        pszFunctionName,
        pszSourceFile,
        dwLineNumber,
        pszMessage);
    BAIL_ON_ERROR(dwError);

error:

    LW_SAFE_FREE_MEMORY(pszMessage);

    return dwError;
}

DWORD
LwSmLogPrintf(
    LW_SM_LOG_LEVEL level,
    PCSTR pszFacility,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    va_list ap;

    LOCK(bLocked, &gLogLock);

    if (level <= gLogInfo.MaxLevel)
    {
        va_start(ap, pszFormat);
        dwError = LwSmLogPrintfv(
            level,
            pszFacility,
            pszFunctionName,
            pszSourceFile,
            dwLineNumber,
            pszFormat,
            ap);
        va_end(ap);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    UNLOCK(bLocked, &gLogLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmSetMaxLogLevel(
    PCSTR pFacility,
    LW_SM_LOG_LEVEL level
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN bLocked = FALSE;
    PLOGINFO pInfo = NULL;
    LW_SM_LOG_LEVEL maxLevel = LW_SM_LOG_LEVEL_ALWAYS;
    LW_HASHMAP_ITER iter = LW_HASHMAP_ITER_INIT;
    LW_HASHMAP_PAIR pair = {0};

    LOCK(bLocked, &gLogLock);

    dwError = LwSmFindOrCreateLogInfo(pFacility, &pInfo);
    BAIL_ON_ERROR(dwError);

    pInfo->MaxLevel = level;

    if (pInfo->MaxLevel == LW_SM_LOG_LEVEL_DEFAULT &&
        pInfo->pLogger == &gDefaultLogger)
    {
        LwRtlHashMapRemove(gpLogMap, pFacility, &pair);
        LogInfoClear(&pair, NULL);
    }

    maxLevel = gLogInfo.MaxLevel;

    while (LwRtlHashMapIterate(gpLogMap, &iter, &pair))
    {
        pInfo = pair.pValue;
        if (pInfo->MaxLevel > maxLevel)
        {
            maxLevel = pInfo->MaxLevel;
        }
    }

    UNLOCK(bLocked, &gLogLock);

    LwRtlLogSetLevel(maxLevel);

    LwSmLogPrintf(
        LW_SM_LOG_LEVEL_ALWAYS,
        pFacility,
        __FUNCTION__,
        __FILE__,
        __LINE__,
        "Log level changed to %s",
        LwSmLogLevelToString(level));

error:

    UNLOCK(bLocked, &gLogLock);

    return dwError;
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

        setvbuf(pContext->file, NULL, _IOLBF, 0);
    }

error:

    return dwError;
}

static
DWORD
LwSmLogFileLog (
    LW_SM_LOG_LEVEL level,
    LW_SM_LOG_LEVEL maxLevel,
    PCSTR pszFacility,
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

    switch (maxLevel)
    {
    case LW_SM_LOG_LEVEL_DEFAULT:
        assert(FALSE);
        break;
    case LW_SM_LOG_LEVEL_ALWAYS:
    case LW_SM_LOG_LEVEL_ERROR:
    case LW_SM_LOG_LEVEL_WARNING:
    case LW_SM_LOG_LEVEL_INFO:
    case LW_SM_LOG_LEVEL_VERBOSE:
        if (pszFacility)
        {
            fprintf(
                pContext->file,
                "%s:%s:%s: %s\n",
                timeBuf,
                LwSmLogLevelToString(level),
                pszFacility,
                pszMessage);
        }
        else
        {
            fprintf(
                pContext->file,
                "%s:%s: %s\n",
                timeBuf,
                LwSmLogLevelToString(level),
                pszMessage);
        }
        break;
    case LW_SM_LOG_LEVEL_DEBUG:
    case LW_SM_LOG_LEVEL_TRACE:
        if (pszFacility)
        {
            fprintf(
                pContext->file,
                "%s:%s:%s:%s():%s:%i: %s\n",
                timeBuf,
                LwSmLogLevelToString(level),
                pszFacility,
                pszFunctionName,
                LwSmBasename(pszSourceFile),
                (int) dwLineNumber,
                pszMessage);
        }
        else
        {
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
LwSmLogFileGetTargetName(
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

    openlog(LwSmBasename(gpProcessName), 0, LOG_DAEMON);

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
LwSmSyslogLog(
    LW_SM_LOG_LEVEL level,
    LW_SM_LOG_LEVEL maxLevel,
    PCSTR pszFacility,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszMessage,
    PVOID pData
    )
{
    DWORD dwError = 0;

    switch (maxLevel)
    {
    case LW_SM_LOG_LEVEL_DEFAULT:
        assert(FALSE);
        break;
    case LW_SM_LOG_LEVEL_ALWAYS:
    case LW_SM_LOG_LEVEL_ERROR:
    case LW_SM_LOG_LEVEL_WARNING:
    case LW_SM_LOG_LEVEL_INFO:
    case LW_SM_LOG_LEVEL_VERBOSE:
        if (pszFacility)
        {
            syslog(
                LwSmLogLevelToPriority(level) | LOG_DAEMON,
                "[%s] %s\n",
                pszFacility,
                pszMessage);
        }
        else
        {
            syslog(
                LwSmLogLevelToPriority(level) | LOG_DAEMON,
                "%s\n",
                pszMessage);
        }
        break;
    case LW_SM_LOG_LEVEL_DEBUG:
    case LW_SM_LOG_LEVEL_TRACE:
        if (pszFacility)
        {
            syslog(
                LwSmLogLevelToPriority(level) | LOG_DAEMON,
                "[%s] %s():%s:%i: %s\n",
                pszFacility,
                pszFunctionName,
                LwSmBasename(pszSourceFile),
                (int) dwLineNumber,
                pszMessage);
        }
        else
        {
            syslog(
                LwSmLogLevelToPriority(level) | LOG_DAEMON,
                "%s():%s:%i: %s\n",
                pszFunctionName,
                LwSmBasename(pszSourceFile),
                (int) dwLineNumber,
                pszMessage);
        }
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
LwSmSyslogGetTargetName(
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
    PCSTR pFacility,
    FILE* file
    )
{
    DWORD dwError = 0;
    PSM_FILE_LOG_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(sizeof(*pContext), OUT_PPVOID(&pContext));
    BAIL_ON_ERROR(dwError);

    pContext->file = file;

    dwError = LwSmSetLogger(pFacility, &gFileLogger, pContext);
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
    PCSTR pFacility,
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

    dwError = LwSmSetLogger(pFacility, &gFileLogger, pContext);
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
    PCSTR pFacility
    )
{
    return LwSmSetLogger(pFacility, &gSyslogLogger, NULL);
}

DWORD
LwSmSetLoggerToDefault(
    PCSTR pFacility
    )
{
    return LwSmSetLogger(pFacility, &gDefaultLogger, NULL);
}

DWORD
LwSmGetLoggerState(
    PCSTR pFacility,
    LW_SM_LOGGER_TYPE* pType,
    PSTR* ppszTargetName,
    PLW_SM_LOG_LEVEL pLevel
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    PLOGINFO pInfo = NULL;

    LOCK(bLocked, &gLogLock);

    if (!pFacility)
    {
        *pType = gLogInfo.pLogger ? gLogInfo.pLogger->type : LW_SM_LOGGER_NONE;
        if (gLogInfo.pLogger && gLogInfo.pLogger->pfnGetTargetName)
        {
            dwError = gLogInfo.pLogger->pfnGetTargetName(ppszTargetName, gLogInfo.pLoggerData);
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            *ppszTargetName = NULL;
        }
        *pLevel = gLogInfo.MaxLevel;
    }
    else if (LwRtlHashMapFindKey(gpLogMap, OUT_PPVOID(&pInfo), pFacility) != ERROR_SUCCESS)
    {
        *pType = LW_SM_LOGGER_DEFAULT;
        *ppszTargetName = NULL;
        *pLevel = LW_SM_LOG_LEVEL_DEFAULT;
    }
    else if (!pInfo->pLogger)
    {
        *pType = LW_SM_LOGGER_NONE;
        *ppszTargetName = NULL;
        *pLevel = LW_SM_LOG_LEVEL_ALWAYS;
    }
    else
    {
        *pType = pInfo->pLogger->type;
        if (pInfo->pLogger->pfnGetTargetName)
        {
            dwError = pInfo->pLogger->pfnGetTargetName(ppszTargetName, pInfo->pLoggerData);
            BAIL_ON_ERROR(dwError);
        }
        *pLevel = pInfo->MaxLevel;
    }

cleanup:

    UNLOCK(bLocked, &gLogLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmGetLogFacilityList(
    PWSTR** pppFacilities
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    DWORD count = 0;
    DWORD index = 0;
    PWSTR* ppFacilities = NULL;
    LW_HASHMAP_PAIR pair = {0};
    LW_HASHMAP_ITER iter = LW_HASHMAP_ITER_INIT;

    LOCK(bLocked, &gLogLock);

    count = LwRtlHashMapGetCount(gpLogMap);

    dwError = LwAllocateMemory(sizeof(*ppFacilities) * (count + 1), OUT_PPVOID(&ppFacilities));
    BAIL_ON_ERROR(dwError);

    while (LwRtlHashMapIterate(gpLogMap, &iter, &pair))
    {
        dwError = LwMbsToWc16s((PCSTR) pair.pKey, &ppFacilities[index++]);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    UNLOCK(bLocked, &gLogLock);

    *pppFacilities = ppFacilities;

    return dwError;

error:

    if (ppFacilities)
    {
        LwSmFreeStringList(ppFacilities);
    }

    goto cleanup;
}
