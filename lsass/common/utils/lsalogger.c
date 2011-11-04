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
 *        lsalogger.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Logger
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LsaInitLogging(
    PCSTR         pszProgramName,
    LsaLogTarget  logTarget,
    LsaLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case LSA_LOG_TARGET_DISABLED:

            break;

        case LSA_LOG_TARGET_SYSLOG:

            dwError = LsaOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case LSA_LOG_TARGET_CONSOLE:

            dwError = LsaOpenConsoleLog(
                            maxAllowedLogLevel,
                            &hLog);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case LSA_LOG_TARGET_FILE:

            if (LW_IS_NULL_OR_EMPTY_STR(pszPath))
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaOpenFileLog(
                          pszPath,
                          maxAllowedLogLevel,
                          &hLog);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:

            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    gLogTarget = logTarget;
    gLsaMaxLogLevel = maxAllowedLogLevel;
    ghLog = hLog;

 cleanup:

    return dwError;

 error:

    gLogTarget = LSA_LOG_TARGET_DISABLED;
    ghLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LsaLogGetInfo(
    PLSA_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLSA_LOG_INFO pLogInfo = NULL;

    switch(gLogTarget)
    {
        case LSA_LOG_TARGET_DISABLED:
        case LSA_LOG_TARGET_CONSOLE:
        case LSA_LOG_TARGET_SYSLOG:

            dwError = LwAllocateMemory(
                            sizeof(LSA_LOG_INFO),
                            (PVOID*)&pLogInfo);
            BAIL_ON_LSA_ERROR(dwError);

            pLogInfo->logTarget = gLogTarget;
            pLogInfo->maxAllowedLogLevel = gLsaMaxLogLevel;

            break;

        case LSA_LOG_TARGET_FILE:

            dwError = LsaGetFileLogInfo(
                            ghLog,
                            &pLogInfo);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *ppLogInfo = pLogInfo;

cleanup:

    return dwError;

error:

    *ppLogInfo = NULL;

    if (pLogInfo)
    {
        LsaFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
LsaLogSetInfo(
    PLSA_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pLogInfo);

    // The only information that is allowed
    // to be set after the log is initialized
    // is the log level

    gLsaMaxLogLevel = pLogInfo->maxAllowedLogLevel;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;

    if (ghLog != (HANDLE)NULL)
    {
        switch(gLogTarget)
        {
            case LSA_LOG_TARGET_DISABLED:
                break;

            case LSA_LOG_TARGET_CONSOLE:
                LsaCloseConsoleLog(ghLog);
                break;

            case LSA_LOG_TARGET_FILE:
                LsaCloseFileLog(ghLog);
                break;

            case LSA_LOG_TARGET_SYSLOG:
                LsaCloseSyslog(ghLog);
            break;
        }
    }

    return dwError;
}

DWORD
LsaSetupLogging(
	HANDLE              hLog,
	LsaLogLevel         maxAllowedLogLevel,
	PFN_LSA_LOG_MESSAGE pfnLogger
	)
{
	DWORD dwError = 0;

	if ((hLog == (HANDLE)NULL) ||
		!pfnLogger)
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		goto error;
	}

	ghLog = hLog;
	gLsaMaxLogLevel = maxAllowedLogLevel;
	gpfnLogger = pfnLogger;

error:

	return dwError;
}

VOID
LsaResetLogging(
    VOID
    )
{
	gLsaMaxLogLevel = LSA_LOG_LEVEL_ERROR;
	gpfnLogger = NULL;
	ghLog = (HANDLE)NULL;
}

VOID
LsaLogMessage(
	PFN_LSA_LOG_MESSAGE pfnLogger,
	HANDLE hLog,
	LsaLogLevel logLevel,
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
LsaValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch(dwLogLevel)
    {
        case LSA_LOG_LEVEL_ALWAYS:
        case LSA_LOG_LEVEL_ERROR:
        case LSA_LOG_LEVEL_WARNING:
        case LSA_LOG_LEVEL_INFO:
        case LSA_LOG_LEVEL_VERBOSE:
        case LSA_LOG_LEVEL_DEBUG:
        case LSA_LOG_LEVEL_TRACE:
            dwError = 0;
            break;
        default:
            dwError = LW_ERROR_INVALID_LOG_LEVEL;
            break;
    }

    return dwError;
}

DWORD
LsaTraceInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    PLSA_BIT_VECTOR pTraceVector = NULL;

    dwError = LsaBitVectorCreate(
                    LSA_TRACE_FLAG_SENTINEL,
                    &pTraceVector);
    BAIL_ON_LSA_ERROR(dwError);

    if (gpTraceFlags)
    {
        LsaBitVectorFree(gpTraceFlags);
    }

    gpTraceFlags = pTraceVector;

cleanup:

    return dwError;

error:

    if (pTraceVector)
    {
        LsaBitVectorFree(pTraceVector);
    }

    goto cleanup;
}

BOOLEAN
LsaTraceIsFlagSet(
    DWORD dwTraceFlag
    )
{
    BOOLEAN bResult = FALSE;

    if (gpTraceFlags &&
        dwTraceFlag &&
        LsaBitVectorIsSet(gpTraceFlags, dwTraceFlag))
    {
        bResult = TRUE;
    }

    return bResult;
}

BOOLEAN
LsaTraceIsAllowed(
    DWORD dwTraceFlags[],
    DWORD dwNumFlags
    )
{
    BOOLEAN bResult = FALSE;
    DWORD   iFlag = 0;

    if (gpTraceFlags)
    {
        for (; !bResult && (iFlag < dwNumFlags); iFlag++)
        {
            if (LsaTraceIsFlagSet(dwTraceFlags[iFlag]))
            {
                bResult = TRUE;
            }
        }
    }

    return bResult;
}

DWORD
LsaTraceSetFlag(
    DWORD dwTraceFlag
    )
{
    DWORD dwError = 0;

    if (!gpTraceFlags)
    {
        dwError = LW_ERROR_TRACE_NOT_INITIALIZED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaBitVectorSetBit(
                    gpTraceFlags,
                    dwTraceFlag);

error:

    return dwError;
}

DWORD
LsaTraceUnsetFlag(
    DWORD dwTraceFlag
    )
{
    DWORD dwError = 0;

    if (!gpTraceFlags)
    {
        dwError = LW_ERROR_TRACE_NOT_INITIALIZED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaBitVectorUnsetBit(
                    gpTraceFlags,
                    dwTraceFlag);

error:

    return dwError;
}

VOID
LsaTraceShutdown(
    VOID
    )
{
    if (gpTraceFlags)
    {
        LsaBitVectorFree(gpTraceFlags);
        gpTraceFlags = NULL;
    }
}
