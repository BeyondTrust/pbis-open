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
    if (gpfnLogger && ((int)gLsaMaxLogLevel >= (int)level))
    {
        LW_RTL_LOG_AT_LEVEL(level, "%s", pszMessage);
    }
    LSA_UNLOCK_LOGGER;
}

static
inline
LsaLogLevel
LsaRtlToLsaLogLevel(
    IN LW_RTL_LOG_LEVEL Level,
    IN LsaLogLevel DefaultLevel
    )
{
    switch (Level)
    {
        case LW_RTL_LOG_LEVEL_ALWAYS:
            return LSA_LOG_LEVEL_ALWAYS;
        case LW_RTL_LOG_LEVEL_ERROR:
            return LSA_LOG_LEVEL_ERROR;
        case LW_RTL_LOG_LEVEL_WARNING:
            return LSA_LOG_LEVEL_WARNING;
        case LW_RTL_LOG_LEVEL_INFO:
            return LSA_LOG_LEVEL_INFO;
        case LW_RTL_LOG_LEVEL_VERBOSE:
            return LSA_LOG_LEVEL_VERBOSE;
        case LW_RTL_LOG_LEVEL_DEBUG:
            return LSA_LOG_LEVEL_DEBUG;
        case LW_RTL_LOG_LEVEL_TRACE:
            return LSA_LOG_LEVEL_TRACE;
        default:
            return DefaultLevel;
    }
}

static
inline
LW_RTL_LOG_LEVEL
LsaLsaToRtlLogLevel(
    IN LsaLogLevel Level,
    IN LW_RTL_LOG_LEVEL DefaultLevel
    )
{
    switch (Level)
    {
        case LSA_LOG_LEVEL_ALWAYS:
            return LW_RTL_LOG_LEVEL_ALWAYS;
        case LSA_LOG_LEVEL_ERROR:
            return LW_RTL_LOG_LEVEL_ERROR;
        case LSA_LOG_LEVEL_WARNING:
            return LW_RTL_LOG_LEVEL_WARNING;
        case LSA_LOG_LEVEL_INFO:
            return LW_RTL_LOG_LEVEL_INFO;
        case LSA_LOG_LEVEL_VERBOSE:
            return LW_RTL_LOG_LEVEL_VERBOSE;
        case LSA_LOG_LEVEL_DEBUG:
            return LW_RTL_LOG_LEVEL_DEBUG;
        case LSA_LOG_LEVEL_TRACE:
            return LW_RTL_LOG_LEVEL_TRACE;
        default:
            return DefaultLevel;
    }
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
    LsaLogLevel lsaLevel = LsaRtlToLsaLogLevel(Level, LSA_LOG_LEVEL_DEBUG);

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
                    lsaLevel,
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
                    lsaLevel,
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
    LwRtlLogSetLevel(LsaLsaToRtlLogLevel(maxAllowedLogLevel, LW_RTL_LOG_LEVEL_TRACE));

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
    LwRtlLogSetLevel(LsaLsaToRtlLogLevel(pLogInfo->maxAllowedLogLevel, LW_RTL_LOG_LEVEL_TRACE));

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

