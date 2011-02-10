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
 *        Reaper for syslog
 *        Syslog Log Backend
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include "includes.h"

DWORD
RSysOpenSyslog(
    IN PCSTR       pszIdentifier,
    IN DWORD       dwOptions,
    IN DWORD       dwFacility,
    OUT PRSYS_LOG* ppLog
    )
{
    DWORD dwError = 0;
    PRSYS_LOG pLog = NULL;

    dwError = LwNtStatusToWin32Error(
                  LW_RTL_ALLOCATE(
                      &pLog,
                      RSYS_LOG,
                      sizeof(*pLog)));
    BAIL_ON_RSYS_ERROR(dwError);

    pLog->pfnWrite = RSysLogToSyslog;
    pLog->pfnClose = RSysCloseSyslog;
    
    *ppLog = pLog;

    if (pszIdentifier == NULL)
    {
        pszIdentifier = "lsass";
    }
    
    openlog(
        pszIdentifier,
        dwOptions,
        dwFacility);
    dwSyslogOpenCount++;
    
    *ppLog = pLog;

cleanup:
    return dwError;

error:
    LW_RTL_FREE(&pLog);
    goto cleanup;
}
    
DWORD
RSysLogToSyslog(
    IN PRSYS_LOG pThis,
    IN RSysLogLevel level,
    IN PSTR pszMessage
    )
{
    switch (level)
    {
        case RSYS_LOG_LEVEL_ALWAYS:
        {
            syslog(LOG_INFO, "%s", pszMessage);
            break;
        }
        case RSYS_LOG_LEVEL_ERROR:
        {
            syslog(LOG_ERR, "%s", pszMessage);
            break;
        }

        case RSYS_LOG_LEVEL_WARNING:
        {
            syslog(LOG_WARNING, "%s", pszMessage);
            break;
        }

        case RSYS_LOG_LEVEL_INFO:
        default:
        {
            syslog(LOG_INFO, "%s", pszMessage);
            break;
        }
    }

    return 0;
}

DWORD
RSysCloseSyslog(
    IN PRSYS_LOG pThis
    )
{
    LW_RTL_FREE(&pThis);

    if (--dwSyslogOpenCount == 0)
    {
        closelog();
    }

    return 0;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

