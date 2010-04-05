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
 *        logging.c
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 *        Global log accesser functions
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include "includes.h"

DWORD
EfdCloseLog(
    IN PEFD_LOG pLog
    )
{
    if (pLog)
    {
        return pLog->pfnClose(pLog);
    }
    return 0;
}

DWORD
EfdWriteToLogV(
    IN PEFD_LOG pLog,
    IN EfdLogLevel level,
    IN PCSTR pszFormat,
    IN va_list args
    )
{
    DWORD dwError = 0;
    PSTR pszFormatted = NULL;

    dwError = RtlCStringAllocatePrintfV(
                      &pszFormatted,
                      pszFormat,
                      args);
    if (dwError == STATUS_INSUFFICIENT_RESOURCES)
    {
        // There isn't enough memory to allocate the formatted string on the
        // stack. So instead print a truncated message using the stack.
        CHAR szLimited[200];
        vsnprintf(
                szLimited,
                sizeof(szLimited),
                pszFormat,
                args);
        szLimited[sizeof(szLimited)-1] = 0;

        if (pLog == NULL)
        {
            printf("%s\n", szLimited);
        }
        else
        {
            dwError = pLog->pfnWrite(pLog, level, szLimited);
        }
    }
    else if (dwError)
    {
        goto error;
    }
    else
    {
        if (pLog == NULL)
        {
            printf("%s\n", pszFormatted);
        }
        else
        {
            dwError = pLog->pfnWrite(pLog, level, pszFormatted);
        }
    }
    
cleanup:
    LwRtlCStringFree(&pszFormatted);
    return dwError;

error:
    goto cleanup;
}

DWORD
EfdSetGlobalLog(
    IN PEFD_LOG pLog,
    IN EfdLogLevel level
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD dwError = 0;

    if (EfdLogLock)
    {
        dwError = EfdLogLock(&bInLock);
        BAIL_ON_EFD_ERROR(dwError);
    }

    if (gpLog)
    {
        dwError = EfdCloseLog(gpLog);
        BAIL_ON_EFD_ERROR(dwError);
    }
    gpLog = pLog;
    gLogLevel = level;

cleanup:
    if (EfdLogLock)
    {
        EfdLogUnlock(bInLock);
    }
    
    return dwError;

error:
    goto cleanup;
}

DWORD
EfdCloseGlobalLog()
{
    return EfdSetGlobalLog(
                NULL,
                EFD_LOG_LEVEL_ALWAYS);
}

DWORD
EfdLogMessage(
    EfdLogLevel level,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    va_list args;

    if (EfdLogLock)
    {
        dwError = EfdLogLock(&bInLock);
        BAIL_ON_EFD_ERROR(dwError);
    }

    if (level <= gLogLevel)
    {
        va_start(args, pszFormat);

        dwError = EfdWriteToLogV(
                        gpLog,
                        level,
                        pszFormat,
                        args);

        va_end(args);
    }

cleanup:
    if (EfdLogLock)
    {
        EfdLogUnlock(bInLock);
    }
    
    return dwError;

error:
    goto cleanup;
}
