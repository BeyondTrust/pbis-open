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
 *        filelog.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Logging API
 * 
 *        Implemenation of logging to file
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#include "includes.h"

DWORD
LsaOpenFileLog(
    PCSTR       pszFilePath,
    LsaLogLevel maxAllowedLogLevel,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PLSA_FILE_LOG pFileLog = NULL;
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszFilePath))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        goto error;
    }
    
    dwError = LwAllocateMemory(
                    sizeof(LSA_FILE_LOG),
                    (PVOID*)&pFileLog);
    if (dwError)
    {
        goto error;
    }
    
    dwError = LwAllocateString(
                    pszFilePath,
                    &pFileLog->pszFilePath);
    if (dwError)
    {
        goto error;
    }
    
    pFileLog->fp = fopen(pFileLog->pszFilePath, "w");
    if (pFileLog->fp == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        goto error;
    }
    
    dwError = LsaSetupLogging(
                    (HANDLE)pFileLog,
                    maxAllowedLogLevel,
                    &LsaLogToFile
                    );
    if (dwError)
    {
        goto error;
    }
    
    *phLog = (HANDLE)pFileLog;

cleanup:

    return dwError;

error:

    *phLog = (HANDLE)NULL;
    
    if (pFileLog)
    {
        LsaFreeFileLogInfo(pFileLog);
    }

    goto cleanup;
}

DWORD
LsaGetFileLogInfo(
    HANDLE hLog,
    PLSA_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLSA_LOG_INFO pLogInfo = NULL;
    PLSA_FILE_LOG pFileLog = (PLSA_FILE_LOG)hLog;
    
    BAIL_ON_INVALID_HANDLE(hLog);
    
    if ((gLogTarget != LSA_LOG_TARGET_FILE) ||
        LW_IS_NULL_OR_EMPTY_STR(pFileLog->pszFilePath))
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwAllocateMemory(
                    sizeof(LSA_LOG_INFO),
                    (PVOID*)&pLogInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pLogInfo->logTarget = LSA_LOG_TARGET_FILE;
    pLogInfo->maxAllowedLogLevel = LwRtlLogGetLevel();
    
    dwError = LwAllocateString(
                    pFileLog->pszFilePath,
                    &pLogInfo->pszPath);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    if (pLogInfo)
    {
        LsaFreeLogInfo(pLogInfo);
    }
    
    *ppLogInfo = NULL;

    goto cleanup;
}

DWORD
LsaCloseFileLog(
    HANDLE hLog
    )
{
    PLSA_FILE_LOG pFileLog = (PLSA_FILE_LOG)hLog;
    
    LsaResetLogging();
    
    if (pFileLog)
    {
        LsaFreeFileLogInfo(pFileLog);
    }
    return 0;
}

VOID
LsaLogToFile(
    HANDLE      hLog,
    LsaLogLevel logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    PLSA_FILE_LOG pFileLog = (PLSA_FILE_LOG)hLog;
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];
    
    switch (logLevel)
    {
        case LSA_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = LSA_INFO_TAG;
            break;
        }
        case LSA_LOG_LEVEL_ERROR:
        {
            pszEntryType = LW_ERROR_TAG;
            break;
        }

        case LSA_LOG_LEVEL_WARNING:
        {
            pszEntryType = LSA_WARN_TAG;
            break;
        }

        case LSA_LOG_LEVEL_INFO:
        {
            pszEntryType = LSA_INFO_TAG;
            break;
        }

        case LSA_LOG_LEVEL_VERBOSE:
        {
            pszEntryType = LSA_VERBOSE_TAG;
            break;
        }

        case LSA_LOG_LEVEL_DEBUG:
        {
            pszEntryType = LSA_DEBUG_TAG;
            break;
        }

        case LSA_LOG_LEVEL_TRACE:
        {
            pszEntryType = LSA_TRACE_TAG;
            break;
        }

        default:
        {
            pszEntryType = LSA_VERBOSE_TAG;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LSA_LOG_TIME_FORMAT, &tmp);

    fprintf(pFileLog->fp, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(pFileLog->fp, pszFormat, msgList);
    fprintf(pFileLog->fp, "\n");
    fflush(pFileLog->fp);
}

VOID
LsaFreeFileLogInfo(
    PLSA_FILE_LOG pFileLog
    )
{
    if (pFileLog->fp)
    {
        fclose(pFileLog->fp);
    }

    LW_SAFE_FREE_STRING(pFileLog->pszFilePath);
    
    LwFreeMemory(pFileLog);
}
