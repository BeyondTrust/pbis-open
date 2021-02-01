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
 *        syslog.c
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
 *
 *        Logging API
 *
 *        Implemenation of logging to syslog
 * 
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

#include "includes.h"

static
VOID
LwTaskLogToSyslog(
    HANDLE            hLog,
    LW_TASK_LOG_LEVEL logLevel,
    PCSTR             pszFormat,
    va_list           msgList
    );

DWORD
LwTaskOpenSyslog(
    PCSTR             pszIdentifier,
    LW_TASK_LOG_LEVEL maxAllowedLogLevel,
    DWORD             dwOptions,
    DWORD             dwFacility,
    PHANDLE           phLog
    )
{
    DWORD dwError = 0;
    PLW_TASK_SYS_LOG pSyslog = NULL;

    dwError = LwAllocateMemory(sizeof(LW_TASK_SYS_LOG), (PVOID*)&pSyslog);
    if (dwError)
    {
        goto error;
    }

    dwError = LwAllocateString(
                  (IsNullOrEmptyString(pszIdentifier) ? "lwtask" : pszIdentifier),
                  &pSyslog->pszIdentifier);
    if (dwError)
    {
        goto error;
    }
    
    pSyslog->dwOptions = dwOptions;
    pSyslog->dwFacility = dwFacility;

    openlog(
        pSyslog->pszIdentifier,
        pSyslog->dwOptions,
        pSyslog->dwFacility);
    
    pSyslog->bOpened = TRUE;

    LwTaskSetSyslogMask(LW_TASK_LOG_LEVEL_DEBUG);
    
    dwError = LwTaskSetupLogging(
                    (HANDLE)pSyslog,
                    maxAllowedLogLevel,
                    &LwTaskLogToSyslog);
    if (dwError)
    {
        goto error;
    }
    
    *phLog = (HANDLE)pSyslog;

cleanup:

    return dwError;

error:

    *phLog = (HANDLE)NULL;
    
    if (pSyslog)
    {
        LwTaskFreeSysLogInfo(pSyslog);
    }

    goto cleanup;
}

VOID
LwTaskSetSyslogMask(
    LW_TASK_LOG_LEVEL maxLogLevel
    )
{
    int mask = LOG_UPTO(LOG_INFO);

    switch (maxLogLevel)
    {
        case LW_TASK_LOG_LEVEL_ERROR:

            mask = LOG_UPTO(LOG_ERR);

            break;

        case LW_TASK_LOG_LEVEL_WARNING:

            mask = LOG_UPTO(LOG_WARNING);

            break;

        case LW_TASK_LOG_LEVEL_DEBUG:

            mask = LOG_UPTO(LOG_DEBUG);

            break;

        default:

            mask = LOG_UPTO(LOG_INFO);

            break;
    }

    setlogmask(mask);
}

static
VOID
LwTaskLogToSyslog(
    HANDLE            hLog,
    LW_TASK_LOG_LEVEL logLevel,
    PCSTR             pszFormat,
    va_list           msgList
    )
{
    switch (logLevel)
    {
        case LW_TASK_LOG_LEVEL_ALWAYS:

            lwtask_vsyslog(LOG_INFO, pszFormat, msgList);
            break;

        case LW_TASK_LOG_LEVEL_ERROR:

            lwtask_vsyslog(LOG_ERR, pszFormat, msgList);
            break;

        case LW_TASK_LOG_LEVEL_WARNING:

            lwtask_vsyslog(LOG_WARNING, pszFormat, msgList);
            break;

        case LW_TASK_LOG_LEVEL_INFO:

            lwtask_vsyslog(LOG_INFO, pszFormat, msgList);
            break;

        case LW_TASK_LOG_LEVEL_DEBUG:

            lwtask_vsyslog(LOG_DEBUG, pszFormat, msgList);
            break;

        default:

            lwtask_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
    }
}

DWORD
LwTaskCloseSyslog(
    HANDLE hLog
    )
{
    PLW_TASK_SYS_LOG pSysLog = (PLW_TASK_SYS_LOG)hLog;
    
    LwTaskResetLogging();
    
    if (pSysLog)
    {    
        LwTaskFreeSysLogInfo(pSysLog);
    }
    return 0;
}

VOID
LwTaskFreeSysLogInfo(
    PLW_TASK_SYS_LOG pSysLog
    )
{
    if (pSysLog->bOpened)
    {
        /* close connection to syslog */
        closelog();
    }
    
    LW_SAFE_FREE_STRING(pSysLog->pszIdentifier);
    
    LwFreeMemory(pSysLog);
}

