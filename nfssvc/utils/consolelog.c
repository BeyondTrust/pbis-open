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
 *        Likewise Server Service (NFSSVC)
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
NfsSvcOpenConsoleLog(
    NFSSVC_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PNFSSVC_CONSOLE_LOG pConsoleLog = NULL;
    
    dwError = LwAllocateMemory(
                    sizeof(NFSSVC_CONSOLE_LOG),
                    (PVOID*)&pConsoleLog);
    if (dwError)
    {
        goto error;
    }
    
    pConsoleLog->fp_out = stdout;
    pConsoleLog->fp_err = stderr;
    
    dwError = NfsSvcSetupLogging(
                    (HANDLE)pConsoleLog,
                    maxAllowedLogLevel,
                    &NfsSvcLogToConsole
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
        NfsSvcFreeConsoleLogInfo(pConsoleLog);
    }

    goto cleanup;
}

DWORD
NfsSvcCloseConsoleLog(
    HANDLE hLog
    )
{
    PNFSSVC_CONSOLE_LOG pConsoleLog = (PNFSSVC_CONSOLE_LOG)hLog;
    
    NfsSvcResetLogging();
    
    if (pConsoleLog)
    {
        NfsSvcFreeConsoleLogInfo(pConsoleLog);
    }
    return 0;
}

VOID
NfsSvcLogToConsole(
    HANDLE      hLog,
    NFSSVC_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    PNFSSVC_CONSOLE_LOG pConsoleLog = (PNFSSVC_CONSOLE_LOG)hLog;
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];
    FILE* pTarget = NULL;
    
    switch (logLevel)
    {
        case NFSSVC_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = NFSSVC_INFO_TAG;
            pTarget = pConsoleLog->fp_out;
            break;
        }
        case NFSSVC_LOG_LEVEL_ERROR:
        {
            pszEntryType = NFSSVC_ERROR_TAG;
            pTarget = pConsoleLog->fp_err;
            break;
        }

        case NFSSVC_LOG_LEVEL_WARNING:
        {
            pszEntryType = NFSSVC_WARN_TAG;
            pTarget = pConsoleLog->fp_err;
            break;
        }

        case NFSSVC_LOG_LEVEL_INFO:
        {
            pszEntryType = NFSSVC_INFO_TAG;
            pTarget = pConsoleLog->fp_out;
            break;
        }

        case NFSSVC_LOG_LEVEL_VERBOSE:
        {
            pszEntryType = NFSSVC_VERBOSE_TAG;
            pTarget = pConsoleLog->fp_out;
            break;
        }

        case NFSSVC_LOG_LEVEL_DEBUG:
        {
            pszEntryType = NFSSVC_DEBUG_TAG;
            pTarget = pConsoleLog->fp_out;
            break;
        }

        default:
        {
            pszEntryType = NFSSVC_VERBOSE_TAG;
            pTarget = pConsoleLog->fp_out;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), NFSSVC_LOG_TIME_FORMAT, &tmp);

    fprintf(pTarget, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(pTarget, pszFormat, msgList);
    fprintf(pTarget, "\n");
    fflush(pTarget);
}

VOID
NfsSvcFreeConsoleLogInfo(
    PNFSSVC_CONSOLE_LOG pConsoleLog
    )
{
    LwFreeMemory(pConsoleLog);
}
