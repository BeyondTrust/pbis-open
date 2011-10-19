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
        _LSA_LOG_WITH_THREAD(level, "%s", pszMessage);
    }
    LSA_UNLOCK_LOGGER;
}

static
VOID
LsaLwpsLogMessage(
    LwpsLogLevel level,
    PVOID pUserData,
    PCSTR pszMessage
    )
{
    LSA_LOCK_LOGGER;
    if (gpfnLogger && ((int)gLsaMaxLogLevel >= (int)level))
    {
        _LSA_LOG_WITH_THREAD(level, "%s", pszMessage);
    }
    LSA_UNLOCK_LOGGER;
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
    LwpsSetLogFunction(LWPS_LOG_LEVEL_DEBUG, LsaLwpsLogMessage, NULL);

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

