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
 *        consolelog.c
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
LwTaskLogToConsole(
    HANDLE      hLog,
    LW_TASK_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

DWORD
LwTaskOpenConsoleLog(
    LW_TASK_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE           phLog
    )
{
    DWORD dwError = 0;
    PLW_TASK_CONSOLE_LOG pConsoleLog = NULL;
    
    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_CONSOLE_LOG),
                    (PVOID*)&pConsoleLog);
    if (dwError)
    {
        goto error;
    }
    
    pConsoleLog->fp_out = stdout;
    pConsoleLog->fp_err = stderr;
    
    dwError = LwTaskSetupLogging(
                    (HANDLE)pConsoleLog,
                    maxAllowedLogLevel,
                    &LwTaskLogToConsole
                    );
    if (dwError)
    {
        goto error;
    }
    
    *phLog = (HANDLE)pConsoleLog;

cleanup:

    return dwError;

error:

    *phLog = (HANDLE)NULL;
    
    if (pConsoleLog)
    {
        LwTaskFreeConsoleLogInfo(pConsoleLog);
    }

    goto cleanup;
}

DWORD
LwTaskCloseConsoleLog(
    HANDLE hLog
    )
{
    PLW_TASK_CONSOLE_LOG pConsoleLog = (PLW_TASK_CONSOLE_LOG)hLog;
    
    LwTaskResetLogging();
    
    if (pConsoleLog)
    {
        LwTaskFreeConsoleLogInfo(pConsoleLog);
    }
    return 0;
}

static
VOID
LwTaskLogToConsole(
    HANDLE      hLog,
    LW_TASK_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    PLW_TASK_CONSOLE_LOG pConsoleLog = (PLW_TASK_CONSOLE_LOG)hLog;
    FILE* pTarget = NULL;
    time_t currentTime = 0;
    char timeBuf[128];
    struct tm tmp = {0};
    
    switch (logLevel)
    {
        case LW_TASK_LOG_LEVEL_ERROR:
        case LW_TASK_LOG_LEVEL_WARNING:

            pTarget = pConsoleLog->fp_err;
            break;

        default:

            pTarget = pConsoleLog->fp_out;
            break;
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);
    strftime(timeBuf, sizeof(timeBuf), LW_TASK_LOG_TIME_FORMAT, &tmp);

    fprintf(pTarget, "%s", timeBuf);

    vfprintf(pTarget, pszFormat, msgList);
    fprintf(pTarget, "\n");
    fflush(pTarget);
}

VOID
LwTaskFreeConsoleLogInfo(
    PLW_TASK_CONSOLE_LOG pConsoleLog
    )
{
    LwFreeMemory(pConsoleLog);
}
