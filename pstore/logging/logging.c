/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        Global log accesser functions
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include "includes.h"

DWORD
LwpsLogMessage(
    IN LwpsLogLevel level,
    IN PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    PSTR pszFormatted = NULL;
    va_list args;
    BOOLEAN bArgsStarted = FALSE;

    if (level <= gLwpsLogLevel && gpLwpsLogCallback)
    {
        va_start(args, pszFormat);
        bArgsStarted = TRUE;

        dwError = LwpsAllocateStringPrintfV(
                          &pszFormatted,
                          pszFormat,
                          args);
        if (dwError == LWPS_ERROR_OUT_OF_MEMORY)
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

            gpLwpsLogCallback(level, gpLwpsLogUserData, szLimited);
            dwError = 0;
        }
        else if (dwError)
        {
            goto error;
        }
        else
        {
            gpLwpsLogCallback(level, gpLwpsLogUserData, pszFormatted);
        }
    }

cleanup:
    if (bArgsStarted)
    {
        va_end(args);
    }
    LWPS_SAFE_FREE_STRING(pszFormatted);
    
    return dwError;

error:
    goto cleanup;
}

DWORD
LwpsSetLogFunction(
    IN LwpsLogLevel maxLevel,
    IN PLWPS_LOG_CALLBACK pCallback,
    IN PVOID pUserData
    )
{
    gpLwpsLogUserData = pUserData;
    gpLwpsLogCallback = pCallback;
    gLwpsLogLevel = maxLevel;
    return 0;
}
