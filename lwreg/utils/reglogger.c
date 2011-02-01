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
 *        reglogger.c
 *
 * Abstract:
 *
 *        Registry Logger
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
RegInitLogging(
    PCSTR         pszProgramName,
    RegLogTarget  logTarget,
    RegLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case REG_LOG_TARGET_DISABLED:

            break;

        case REG_LOG_TARGET_SYSLOG:

            dwError = RegOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_REG_ERROR(dwError);

            break;

        case REG_LOG_TARGET_CONSOLE:

            dwError = RegOpenConsoleLog(
                            maxAllowedLogLevel,
                            &hLog);
            BAIL_ON_REG_ERROR(dwError);

            break;

        case REG_LOG_TARGET_FILE:

            if (IsNullOrEmptyString(pszPath))
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_REG_ERROR(dwError);
            }

            dwError = RegOpenFileLog(
                          pszPath,
                          maxAllowedLogLevel,
                          &hLog);
            BAIL_ON_REG_ERROR(dwError);

            break;

        default:

            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
    }

    gRegLogTarget = logTarget;
    ghRegLog = hLog;
    LwRtlLogSetLevel(maxAllowedLogLevel);

 cleanup:

    return dwError;

 error:

    gRegLogTarget = REG_LOG_TARGET_DISABLED;
    ghRegLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
RegLogGetInfo(
    PREG_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PREG_LOG_INFO pLogInfo = NULL;

    switch(gRegLogTarget)
    {
        case REG_LOG_TARGET_DISABLED:
        case REG_LOG_TARGET_CONSOLE:
        case REG_LOG_TARGET_SYSLOG:

            dwError = RegAllocateMemory(sizeof(*pLogInfo), (PVOID*)&pLogInfo);
            BAIL_ON_REG_ERROR(dwError);

            pLogInfo->logTarget = gRegLogTarget;
            pLogInfo->maxAllowedLogLevel = LwRtlLogGetLevel();

            break;

        case REG_LOG_TARGET_FILE:

            dwError = RegGetFileLogInfo(
                            ghRegLog,
                            &pLogInfo);
            BAIL_ON_REG_ERROR(dwError);

            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
    }

    *ppLogInfo = pLogInfo;

cleanup:

    return dwError;

error:

    *ppLogInfo = NULL;

    if (pLogInfo)
    {
        RegFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
RegShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;

    if (ghRegLog != (HANDLE)NULL)
    {
        switch(gRegLogTarget)
        {
            case REG_LOG_TARGET_DISABLED:
                break;

            case REG_LOG_TARGET_CONSOLE:
                RegCloseConsoleLog(ghRegLog);
                break;

            case REG_LOG_TARGET_FILE:
                RegCloseFileLog(ghRegLog);
                break;

            case REG_LOG_TARGET_SYSLOG:
                RegCloseSyslog(ghRegLog);
            break;
        }
    }

    return dwError;
}

DWORD
RegSetupLogging(
	HANDLE              hLog,
	RegLogLevel         maxAllowedLogLevel,
	PFN_REG_LOG_MESSAGE pfnLogger
	)
{
	DWORD dwError = 0;

	if ((hLog == (HANDLE)NULL) ||
		!pfnLogger)
	{
		dwError = ERROR_INVALID_PARAMETER;
		goto error;
	}

	ghRegLog = hLog;
	gpfnRegLogger = pfnLogger;
	LwRtlLogSetLevel(maxAllowedLogLevel);

error:

	return dwError;
}

VOID
RegResetLogging(
    VOID
    )
{
    gpfnRegLogger = NULL;
	ghRegLog = (HANDLE)NULL;
	LwRtlLogSetLevel(REG_LOG_LEVEL_ERROR);
}

VOID
RegLogMessage(
	PFN_REG_LOG_MESSAGE pfnLogger,
	HANDLE hLog,
	RegLogLevel logLevel,
	PCSTR  pszFormat,
	...
	)
{
	va_list msgList;
	va_start(msgList, pszFormat);

	pfnLogger(hLog, logLevel, pszFormat, msgList);

	va_end(msgList);
}

DWORD
RegValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch(dwLogLevel)
    {
        case REG_LOG_LEVEL_ALWAYS:
        case REG_LOG_LEVEL_ERROR:
        case REG_LOG_LEVEL_WARNING:
        case REG_LOG_LEVEL_INFO:
        case REG_LOG_LEVEL_VERBOSE:
        case REG_LOG_LEVEL_DEBUG:
        case REG_LOG_LEVEL_TRACE:
            dwError = 0;
            break;
        default:
            dwError = LWREG_ERROR_INVALID_LOG_LEVEL;
            break;
    }

    return dwError;
}
