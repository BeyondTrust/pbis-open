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
 *        consolelog.c
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
