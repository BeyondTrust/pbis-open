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
 *        Event Collector
 *        Syslog Log Backend
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include "includes.h"

DWORD
CltrOpenSyslog(
    IN PCSTR       pszIdentifier,
    IN DWORD       dwOptions,
    IN DWORD       dwFacility,
    OUT PCLTR_LOG* ppLog
    )
{
    DWORD dwError = 0;
    PCLTR_LOG pLog = NULL;

    pLog = LwRtlMemoryAllocate(sizeof(*pLog));
    if (pLog == NULL)
    {
        dwError = ENOMEM;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    pLog->pfnWrite = CltrLogToSyslog;
    pLog->pfnClose = CltrCloseSyslog;
    
    *ppLog = pLog;

    if (pszIdentifier == NULL)
    {
        pszIdentifier = "lsass";
    }
    
    openlog(
        pszIdentifier,
        dwOptions,
        dwFacility);
    gdwCltrSyslogOpenCount++;
    
    *ppLog = pLog;

cleanup:
    return dwError;

error:
    LW_RTL_FREE(&pLog);
    goto cleanup;
}
    
DWORD
CltrLogToSyslog(
    IN PCLTR_LOG pThis,
    IN CltrLogLevel level,
    IN PSTR pszMessage
    )
{
    switch (level)
    {
        case CLTR_LOG_LEVEL_ALWAYS:
        {
            syslog(LOG_INFO, "%s", pszMessage);
            break;
        }
        case CLTR_LOG_LEVEL_ERROR:
        {
            syslog(LOG_ERR, "%s", pszMessage);
            break;
        }

        case CLTR_LOG_LEVEL_WARNING:
        {
            syslog(LOG_WARNING, "%s", pszMessage);
            break;
        }

        case CLTR_LOG_LEVEL_INFO:
        default:
        {
            syslog(LOG_INFO, "%s", pszMessage);
            break;
        }
    }

    return 0;
}

DWORD
CltrCloseSyslog(
    IN PCLTR_LOG pThis
    )
{
    LW_RTL_FREE(&pThis);

    if (--gdwCltrSyslogOpenCount == 0)
    {
        closelog();
    }

    return 0;
}
