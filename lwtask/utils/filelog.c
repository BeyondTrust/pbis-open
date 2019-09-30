/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        filelog.c
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
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
