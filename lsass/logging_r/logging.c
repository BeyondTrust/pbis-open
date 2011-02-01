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
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Logging API (Private Header)
 *
 *        Thread Safe
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"
#include <lw/rtllog.h>

static
VOID
LsaLwLogMessage(
    LwLogLevel level,
    PVOID pUserData,
    PCSTR pszMessage
    )
{
    LSA_LOCK_LOGGER;
    if (gpfnLogger)
    {
        LW_RTL_LOG_AT_LEVEL(level, "%s", pszMessage);
    }
    LSA_UNLOCK_LOGGER;
}

static
VOID
LsaRtlLogCallback(
    IN OPTIONAL LW_PVOID Context,
    IN LW_RTL_LOG_LEVEL Level,
    IN OPTIONAL PCSTR ComponentName,
    IN PCSTR FunctionName,
    IN PCSTR FileName,
    IN ULONG LineNumber,
    IN PCSTR Format,
    IN ...
    )
{
    DWORD dwError = 0;
    PSTR formattedMessage = NULL;
    LW_RTL_LOG_LEVEL maxLevel = LwRtlLogGetLevel();

    LSA_LOCK_LOGGER;

    if (gpfnLogger)
    {
        va_list argList;
        va_start(argList, Format);
        dwError = LwAllocateStringPrintfV(&formattedMessage, Format, argList);
        va_end(argList);
        if (dwError)
        {
            goto error;
        }
        else
        {
            if (maxLevel >= LW_RTL_LOG_LEVEL_DEBUG)
            {
                LsaLogMessage(
                    gpfnLogger,
                    ghLog,
                    Level,
                    "0x%lx:[%s() %s:%d] %s",
                    (unsigned long)pthread_self(),
                    FunctionName,
                    FileName,
                    LineNumber,
                    formattedMessage);
            }
            else
            {
                LsaLogMessage(
                    gpfnLogger,
                    ghLog,
                    Level,
                    "0x%lx:%s",
                    (unsigned long)pthread_self(),
                    formattedMessage);
            }
        }
    }

error:
    LSA_UNLOCK_LOGGER;

    if (dwError)
    {
        LSA_LOG_ERROR("Failed to format log message");
    }

    LW_SAFE_FREE_STRING(formattedMessage);
}

DWORD
LsaInitLogging_r(
    PCSTR         pszProgramName,
    LsaLogTarget  logTarget,
    LsaLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    )
{
    DWORD dwError = 0;

    LSA_LOCK_LOGGER;

    dwError = LsaInitLogging(
                    pszProgramName,
                    logTarget,
                    maxAllowedLogLevel,
                    pszPath);

    LwSetLogFunction(LW_LOG_LEVEL_DEBUG, LsaLwLogMessage, NULL);

    LwRtlLogSetCallback(LsaRtlLogCallback, NULL);
    LwRtlLogSetLevel(maxAllowedLogLevel);

    LSA_UNLOCK_LOGGER;

    return dwError;
}

DWORD
LsaLogGetInfo_r(
    PLSA_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;

    LSA_LOCK_LOGGER;

    dwError = LsaLogGetInfo(ppLogInfo);

    LSA_UNLOCK_LOGGER;

    return dwError;
}

DWORD
LsaLogSetInfo_r(
    PLSA_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;

    LSA_LOCK_LOGGER;

    dwError = LsaLogSetInfo(pLogInfo);
    LwRtlLogSetLevel(pLogInfo->maxAllowedLogLevel);

    LSA_UNLOCK_LOGGER;

    return dwError;
}

DWORD
LsaShutdownLogging_r(
    VOID
    )
{
    DWORD dwError = 0;

    LSA_LOCK_LOGGER;

    dwError = LsaShutdownLogging();

    LSA_UNLOCK_LOGGER;

    return dwError;
}

