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
 *        syslog.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Logging API
 *
 *        Implemenation of logging to syslog
 * 
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#include "includes.h"

DWORD
LwioOpenSyslog(
    PCSTR       pszIdentifier,
    LWIO_LOG_LEVEL maxAllowedLogLevel,
    DWORD       dwOptions,
    DWORD       dwFacility,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PSMB_SYS_LOG pSyslog = NULL;

    dwError = SMBAllocateMemory(
                sizeof(SMB_SYS_LOG),
                (PVOID*)&pSyslog);
    if (dwError)
    {
        goto error;
    }

    dwError = SMBAllocateString(
                  (IsNullOrEmptyString(pszIdentifier) ? "lwio" : pszIdentifier),
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

    LwioSetSyslogMask(maxAllowedLogLevel);
    
    dwError = LwioSetupLogging(
                    (HANDLE)pSyslog,
                    maxAllowedLogLevel,
                    &SMBLogToSyslog);
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
        SMBFreeSysLogInfo(pSyslog);
    }

    goto cleanup;
}
    
VOID
LwioSetSyslogMask(
    LWIO_LOG_LEVEL logLevel
    )
{
    DWORD dwSysLogLevel;

    switch (logLevel)
    {
        case LWIO_LOG_LEVEL_ALWAYS:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
        case LWIO_LOG_LEVEL_ERROR:
        {
            dwSysLogLevel = LOG_UPTO(LOG_ERR);
            break;
        }

        case LWIO_LOG_LEVEL_WARNING:
        {
            dwSysLogLevel = LOG_UPTO(LOG_WARNING);
            break;
        }

        case LWIO_LOG_LEVEL_INFO:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }

        default:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
    }

    setlogmask(dwSysLogLevel);
}

VOID
SMBLogToSyslog(
    HANDLE      hLog,
    LWIO_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    switch (logLevel)
    {
        case LWIO_LOG_LEVEL_ALWAYS:
        {
            lsmb_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
        case LWIO_LOG_LEVEL_ERROR:
        {
            lsmb_vsyslog(LOG_ERR, pszFormat, msgList);
            break;
        }

        case LWIO_LOG_LEVEL_WARNING:
        {
            lsmb_vsyslog(LOG_WARNING, pszFormat, msgList);
            break;
        }

        case LWIO_LOG_LEVEL_INFO:
        {
            lsmb_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }

        default:
        {
            lsmb_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
    }
}

DWORD
LwioCloseSyslog(
    HANDLE hLog
    )
{
    PSMB_SYS_LOG pSysLog = (PSMB_SYS_LOG)hLog;
    
    LwioResetLogging();
    
    if (pSysLog)
    {    
        SMBFreeSysLogInfo(pSysLog);
    }
    return 0;
}

VOID
SMBFreeSysLogInfo(
    PSMB_SYS_LOG pSysLog
    )
{
    if (pSysLog->bOpened)
    {
        /* close connection to syslog */
        closelog();
    }
    
    LWIO_SAFE_FREE_STRING(pSysLog->pszIdentifier);
    
    SMBFreeMemory(pSysLog);
}

