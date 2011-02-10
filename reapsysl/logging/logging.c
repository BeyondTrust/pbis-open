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
 *        Reaper for syslog
 *        Global log accesser functions
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include "includes.h"

VOID
RSysLoadLockingSymbols(
    VOID
    )
{
    if (!gpRsysCheckedLockSymbols)
    {
#ifdef RTLD_DEFAULT
        gpRSysLogLock = (PFN_RSYS_LOGGING_LOCK)dlsym(
                                RTLD_DEFAULT,
                                "RSysLogLock");
        gpRSysLogUnlock = (PFN_RSYS_LOGGING_UNLOCK)dlsym(
                                RTLD_DEFAULT,
                                "RSysLogUnlock");
#else
        // Old versions of AIX use this branch
        void *pCurrentImage = dlopen(NULL, RTLD_LAZY);

        if (pCurrentImage)
        {
            gpRSysLogLock = (PFN_RSYS_LOGGING_LOCK)dlsym(
                                    pCurrentImage,
                                    "RSysLogLock");
            gpRSysLogUnlock = (PFN_RSYS_LOGGING_UNLOCK)dlsym(
                                    pCurrentImage,
                                    "RSysLogUnlock");
            dlclose(pCurrentImage);
        }
#endif
        gpRsysCheckedLockSymbols = TRUE;
    }
}

DWORD
RSysCloseLog(
    IN PRSYS_LOG pLog
    )
{
    if (pLog)
    {
        return pLog->pfnClose(pLog);
    }
    return 0;
}

DWORD
RSysWriteToLogV(
    IN PRSYS_LOG pLog,
    IN RSysLogLevel level,
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
RSysSetGlobalLog(
    IN PRSYS_LOG pLog,
    IN RSysLogLevel level
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD dwError = 0;

    RSysLoadLockingSymbols();
    if (gpRSysLogLock)
    {
        dwError = gpRSysLogLock(&bInLock);
        BAIL_ON_RSYS_ERROR(dwError);
    }

    if (gpRsysLog)
    {
        dwError = RSysCloseLog(gpRsysLog);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    gpRsysLog = pLog;
    gRsysLogLevel = level;

cleanup:
    if (gpRSysLogUnlock)
    {
        gpRSysLogUnlock(bInLock);
    }
    
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysCloseGlobalLog()
{
    return RSysSetGlobalLog(
                NULL,
                RSYS_LOG_LEVEL_ALWAYS);
}

DWORD
RSysLogMessage(
    RSysLogLevel level,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    va_list args;

    RSysLoadLockingSymbols();
    if (gpRSysLogLock)
    {
        dwError = gpRSysLogLock(&bInLock);
        BAIL_ON_RSYS_ERROR(dwError);
    }

    if (level <= gRsysLogLevel)
    {
        va_start(args, pszFormat);

        dwError = RSysWriteToLogV(
                        gpRsysLog,
                        level,
                        pszFormat,
                        args);

        va_end(args);
    }

cleanup:
    if (gpRSysLogUnlock)
    {
        gpRSysLogUnlock(bInLock);
    }
    
    return dwError;

error:
    goto cleanup;
}
