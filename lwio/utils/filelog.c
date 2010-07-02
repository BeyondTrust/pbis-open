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
 *        filelog.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
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
LwioOpenFileLog(
    PCSTR       pszFilePath,
    LWIO_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PSMB_FILE_LOG pFileLog = NULL;
    
    if (IsNullOrEmptyString(pszFilePath))
    {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        goto error;
    }
    
    dwError = SMBAllocateMemory(
                    sizeof(SMB_FILE_LOG),
                    (PVOID*)&pFileLog);
    if (dwError)
    {
        goto error;
    }
    
    dwError = SMBAllocateString(
                    pszFilePath,
                    &pFileLog->pszFilePath);
    if (dwError)
    {
        goto error;
    }
    
    pFileLog->fp = fopen(pFileLog->pszFilePath, "w");
    if (pFileLog->fp == NULL) {
        dwError = errno;
        goto error;
    }
    
    dwError = LwioSetupLogging(
                    (HANDLE)pFileLog,
                    maxAllowedLogLevel,
                    &SMBLogToFile
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
        SMBFreeFileLogInfo(pFileLog);
    }

    goto cleanup;
}

DWORD
LwioGetFileLogInfo(
    HANDLE hLog,
    PLWIO_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLWIO_LOG_INFO pLogInfo = NULL;
    PSMB_FILE_LOG pFileLog = (PSMB_FILE_LOG)hLog;
    
    BAIL_ON_INVALID_HANDLE(hLog);
    
    if ((gLWIO_LOG_TARGET != LWIO_LOG_TARGET_FILE) ||
        IsNullOrEmptyString(pFileLog->pszFilePath))
    {
        dwError = LWIO_ERROR_INTERNAL;
        BAIL_ON_LWIO_ERROR(dwError);
    }
    
    dwError = SMBAllocateMemory(
                    sizeof(LWIO_LOG_INFO),
                    (PVOID*)&pLogInfo);
    BAIL_ON_LWIO_ERROR(dwError);
    
    pLogInfo->logTarget = LWIO_LOG_TARGET_FILE;
    pLogInfo->maxAllowedLogLevel = gLwioMaxLogLevel;
    
    dwError = SMBAllocateString(
                    pFileLog->pszFilePath,
                    &pLogInfo->pszPath);
    BAIL_ON_LWIO_ERROR(dwError);
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    if (pLogInfo)
    {
        LwIoFreeLogInfo(pLogInfo);
    }
    
    *ppLogInfo = NULL;

    goto cleanup;
}

DWORD
LwioCloseFileLog(
    HANDLE hLog
    )
{
    PSMB_FILE_LOG pFileLog = (PSMB_FILE_LOG)hLog;
    
    LwioResetLogging();
    
    if (pFileLog)
    {
        SMBFreeFileLogInfo(pFileLog);
    }
    return 0;
}

VOID
SMBLogToFile(
    HANDLE      hLog,
    LWIO_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    PSMB_FILE_LOG pFileLog = (PSMB_FILE_LOG)hLog;
    PSTR pszEntryType = NULL;
    time_t currentTime = 0;
    struct tm tmp = {0};
    char timeBuf[128];
    
    switch (logLevel)
    {
        case LWIO_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = LWIO_INFO_TAG;
            break;
        }
        case LWIO_LOG_LEVEL_ERROR:
        {
            pszEntryType = LWIO_ERROR_TAG;
            break;
        }

        case LWIO_LOG_LEVEL_WARNING:
        {
            pszEntryType = LWIO_WARN_TAG;
            break;
        }

        case LWIO_LOG_LEVEL_INFO:
        {
            pszEntryType = LWIO_INFO_TAG;
            break;
        }

        case LWIO_LOG_LEVEL_VERBOSE:
        {
            pszEntryType = LWIO_VERBOSE_TAG;
            break;
        }

        case LWIO_LOG_LEVEL_DEBUG:
        {
            pszEntryType = LWIO_DEBUG_TAG;
            break;
        }

        case LWIO_LOG_LEVEL_TRACE:
        {
            pszEntryType = LWIO_TRACE_TAG;
            break;
        }

        default:
        {
            pszEntryType = LWIO_VERBOSE_TAG;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LWIO_LOG_TIME_FORMAT, &tmp);

    fprintf(pFileLog->fp, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(pFileLog->fp, pszFormat, msgList);
    fprintf(pFileLog->fp, "\n");
    fflush(pFileLog->fp);
}

VOID
SMBFreeFileLogInfo(
    PSMB_FILE_LOG pFileLog
    )
{
    if (pFileLog->fp)
    {
        fclose(pFileLog->fp);
    }

    LWIO_SAFE_FREE_STRING(pFileLog->pszFilePath);
    
    SMBFreeMemory(pFileLog);
}
