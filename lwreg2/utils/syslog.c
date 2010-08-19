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
 *        Registry
 *
 *        Logging API
 *
 *        Implemenation of logging to syslog
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
RegOpenSyslog(
    PCSTR       pszIdentifier,
    RegLogLevel maxAllowedLogLevel,
    DWORD       dwOptions,
    DWORD       dwFacility,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PREG_SYS_LOG pSyslog = NULL;

    dwError = RegAllocateMemory(sizeof(*pSyslog), (PVOID*)&pSyslog);
    if (dwError)
    {
        goto error;
    }

    dwError = RegCStringDuplicate(
                  &pSyslog->pszIdentifier,
                  (IsNullOrEmptyString(pszIdentifier) ? "registry" : pszIdentifier));
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

    RegSetSyslogMask(maxAllowedLogLevel);

    dwError = RegSetupLogging(
                    (HANDLE)pSyslog,
                    maxAllowedLogLevel,
                    &RegLogToSyslog);
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
        RegFreeSysLogInfo(pSyslog);
    }

    goto cleanup;
}

VOID
RegSetSyslogMask(
    RegLogLevel logLevel
    )
{
    DWORD dwSysLogLevel;

    switch (logLevel)
    {
        case REG_LOG_LEVEL_ALWAYS:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
        case REG_LOG_LEVEL_ERROR:
        {
            dwSysLogLevel = LOG_UPTO(LOG_ERR);
            break;
        }

        case REG_LOG_LEVEL_WARNING:
        {
            dwSysLogLevel = LOG_UPTO(LOG_WARNING);
            break;
        }

        case REG_LOG_LEVEL_INFO:
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
RegLogToSyslog(
    HANDLE      hLog,
    RegLogLevel logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    switch (logLevel)
    {
        case REG_LOG_LEVEL_ALWAYS:
        {
            reg_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
        case REG_LOG_LEVEL_ERROR:
        {
            reg_vsyslog(LOG_ERR, pszFormat, msgList);
            break;
        }

        case REG_LOG_LEVEL_WARNING:
        {
            reg_vsyslog(LOG_WARNING, pszFormat, msgList);
            break;
        }

        case REG_LOG_LEVEL_INFO:
        {
            reg_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }

        default:
        {
            reg_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
    }
}

DWORD
RegCloseSyslog(
    HANDLE hLog
    )
{
    PREG_SYS_LOG pSysLog = (PREG_SYS_LOG)hLog;

    RegResetLogging();

    if (pSysLog)
    {
        RegFreeSysLogInfo(pSysLog);
    }
    return 0;
}

VOID
RegFreeSysLogInfo(
    PREG_SYS_LOG pSysLog
    )
{
    if (pSysLog->bOpened)
    {
        /* close connection to syslog */
        closelog();
    }

    LWREG_SAFE_FREE_STRING(pSysLog->pszIdentifier);

    RegMemoryFree(pSysLog);
}

