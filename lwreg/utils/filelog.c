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
 *        Registry
 *
 *        Logging API
 *
 *        Implemenation of logging to file
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
RegOpenFileLog(
    PCSTR       pszFilePath,
    RegLogLevel maxAllowedLogLevel,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PREG_FILE_LOG pFileLog = NULL;

    if (IsNullOrEmptyString(pszFilePath))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

    dwError = RegAllocateMemory(sizeof(*pFileLog), (PVOID*)&pFileLog);
    if (dwError)
    {
        goto error;
    }

    dwError = RegCStringDuplicate(
                    &pFileLog->pszFilePath,
                    pszFilePath);
    if (dwError)
    {
        goto error;
    }

    pFileLog->fp = fopen(pFileLog->pszFilePath, "w");
    if (pFileLog->fp == NULL) {
        dwError = errno;
        goto error;
    }

    dwError = RegSetupLogging(
                    (HANDLE)pFileLog,
                    maxAllowedLogLevel,
                    &RegLogToFile
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
        RegFreeFileLogInfo(pFileLog);
    }

    goto cleanup;
}

DWORD
RegGetFileLogInfo(
    HANDLE hLog,
    PREG_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PREG_LOG_INFO pLogInfo = NULL;
    PREG_FILE_LOG pFileLog = (PREG_FILE_LOG)hLog;

    BAIL_ON_INVALID_HANDLE(hLog);

    if ((gRegLogTarget != REG_LOG_TARGET_FILE) ||
        IsNullOrEmptyString(pFileLog->pszFilePath))
    {
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegAllocateMemory(sizeof(*pLogInfo), (PVOID*)&pLogInfo);
    BAIL_ON_REG_ERROR(dwError);

    pLogInfo->logTarget = REG_LOG_TARGET_FILE;
    pLogInfo->maxAllowedLogLevel = LwRtlLogGetLevel();

    dwError = RegCStringDuplicate(
                    &pLogInfo->pszPath,
                    pFileLog->pszFilePath);
    BAIL_ON_REG_ERROR(dwError);

    *ppLogInfo = pLogInfo;

cleanup:

    return dwError;

error:

    if (pLogInfo)
    {
        RegFreeLogInfo(pLogInfo);
    }

    *ppLogInfo = NULL;

    goto cleanup;
}

DWORD
RegCloseFileLog(
    HANDLE hLog
    )
{
    PREG_FILE_LOG pFileLog = (PREG_FILE_LOG)hLog;

    RegResetLogging();

    if (pFileLog)
    {
        RegFreeFileLogInfo(pFileLog);
    }
    return 0;
}

VOID
RegLogToFile(
    HANDLE      hLog,
    RegLogLevel logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    PREG_FILE_LOG pFileLog = (PREG_FILE_LOG)hLog;
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];

    switch (logLevel)
    {
        case REG_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = REG_INFO_TAG;
            break;
        }
        case REG_LOG_LEVEL_ERROR:
        {
            pszEntryType = REG_ERROR_TAG;
            break;
        }

        case REG_LOG_LEVEL_WARNING:
        {
            pszEntryType = REG_WARN_TAG;
            break;
        }

        case REG_LOG_LEVEL_INFO:
        {
            pszEntryType = REG_INFO_TAG;
            break;
        }

        case REG_LOG_LEVEL_VERBOSE:
        {
            pszEntryType = REG_VERBOSE_TAG;
            break;
        }

        case REG_LOG_LEVEL_DEBUG:
        {
            pszEntryType = REG_DEBUG_TAG;
            break;
        }

        case REG_LOG_LEVEL_TRACE:
        {
            pszEntryType = REG_TRACE_TAG;
            break;
        }

        default:
        {
            pszEntryType = REG_VERBOSE_TAG;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), REG_LOG_TIME_FORMAT, &tmp);

    fprintf(pFileLog->fp, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(pFileLog->fp, pszFormat, msgList);
    fprintf(pFileLog->fp, "\n");
    fflush(pFileLog->fp);
}

VOID
RegFreeFileLogInfo(
    PREG_FILE_LOG pFileLog
    )
{
    if (pFileLog->fp)
    {
        fclose(pFileLog->fp);
    }

    LW_RTL_FREE(&pFileLog->pszFilePath);

    RegMemoryFree(pFileLog);
}
