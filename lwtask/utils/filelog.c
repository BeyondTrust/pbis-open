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
 *        Likewise Task System (LWTASK)
 * 
 *        Logging API
 * 
 *        Implemenation of logging to file
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

#include "includes.h"

static
VOID
LwTaskLogToFile(
    HANDLE      hLog,
    LW_TASK_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

DWORD
LwTaskOpenFileLog(
    PCSTR       pszFilePath,
    LW_TASK_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PLW_TASK_FILE_LOG pFileLog = NULL;
    
    if (IsNullOrEmptyString(pszFilePath))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }
    
    dwError = LwAllocateMemory(sizeof(LW_TASK_FILE_LOG), (PVOID*)&pFileLog);
    if (dwError)
    {
        goto error;
    }
    
    dwError = LwAllocateString(pszFilePath, &pFileLog->pszFilePath);
    if (dwError)
    {
        goto error;
    }
    
    pFileLog->fp = fopen(pFileLog->pszFilePath, "w");
    if (pFileLog->fp == NULL) {
        dwError = errno;
        goto error;
    }
    
    dwError = LwTaskSetupLogging(
                    (HANDLE)pFileLog,
                    maxAllowedLogLevel,
                    &LwTaskLogToFile
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
        LwTaskFreeFileLogInfo(pFileLog);
    }

    goto cleanup;
}

DWORD
LwTaskGetFileLogInfo(
    HANDLE             hLog,
    PLW_TASK_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLW_TASK_LOG_INFO pLogInfo = NULL;
    PLW_TASK_FILE_LOG pFileLog = (PLW_TASK_FILE_LOG)hLog;
    
    BAIL_ON_INVALID_POINTER(hLog);
    
    if ((gLWTASK_LOG_TARGET != LW_TASK_LOG_TARGET_FILE) ||
        IsNullOrEmptyString(pFileLog->pszFilePath))
    {
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
    
    dwError = LwAllocateMemory(sizeof(LW_TASK_LOG_INFO), (PVOID*)&pLogInfo);
    BAIL_ON_LW_TASK_ERROR(dwError);
    
    pLogInfo->logTarget = LW_TASK_LOG_TARGET_FILE;
    pLogInfo->maxAllowedLogLevel = gLwTaskMaxLogLevel;
    
    dwError = LwAllocateString(pFileLog->pszFilePath, &pLogInfo->pszPath);
    BAIL_ON_LW_TASK_ERROR(dwError);
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    if (pLogInfo)
    {
        LwTaskFreeLogInfo(pLogInfo);
    }
    
    *ppLogInfo = NULL;

    goto cleanup;
}

DWORD
LwTaskCloseFileLog(
    HANDLE hLog
    )
{
    PLW_TASK_FILE_LOG pFileLog = (PLW_TASK_FILE_LOG)hLog;
    
    LwTaskResetLogging();
    
    if (pFileLog)
    {
        LwTaskFreeFileLogInfo(pFileLog);
    }
    return 0;
}

static
VOID
LwTaskLogToFile(
    HANDLE      hLog,
    LW_TASK_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    PLW_TASK_FILE_LOG pFileLog = (PLW_TASK_FILE_LOG)hLog;
    time_t currentTime = 0;
    char timeBuf[128];
    struct tm tmp = {0};

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);
    strftime(timeBuf, sizeof(timeBuf), LW_TASK_LOG_TIME_FORMAT, &tmp);

    fprintf(pFileLog->fp, "%s", timeBuf);

    vfprintf(pFileLog->fp, pszFormat, msgList);
    fprintf(pFileLog->fp, "\n");
    fflush(pFileLog->fp);
}

VOID
LwTaskFreeFileLogInfo(
    PLW_TASK_FILE_LOG pFileLog
    )
{
    if (pFileLog->fp)
    {
        fclose(pFileLog->fp);
    }

    LW_SAFE_FREE_STRING(pFileLog->pszFilePath);
    
    LwFreeMemory(pFileLog);
}
