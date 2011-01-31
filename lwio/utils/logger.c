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
 *        logger.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Utilities
 *
 *        Logger
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LwioInitLogging(
    PCSTR           pszProgramName,
    LWIO_LOG_TARGET logTarget,
    LWIO_LOG_LEVEL  maxAllowedLogLevel,
    PCSTR           pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case LWIO_LOG_TARGET_DISABLED:
            
            break;
            
        case LWIO_LOG_TARGET_SYSLOG:
        
            dwError = LwioOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_LWIO_ERROR(dwError);
      
            break;
            
        case LWIO_LOG_TARGET_CONSOLE:

            dwError = LwioOpenConsoleLog(
                            maxAllowedLogLevel,
                            &hLog);
            BAIL_ON_LWIO_ERROR(dwError);
              
            break;

        case LWIO_LOG_TARGET_FILE:
            
            if (IsNullOrEmptyString(pszPath))
            {
                dwError = LWIO_ERROR_INVALID_PARAMETER;
                BAIL_ON_LWIO_ERROR(dwError);
            }
                        
            dwError = LwioOpenFileLog(
                          pszPath,
                          maxAllowedLogLevel,
                          &hLog);
            BAIL_ON_LWIO_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = LWIO_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWIO_ERROR(dwError);      
    }
    
    gLWIO_LOG_TARGET = logTarget;
    gLwioMaxLogLevel = maxAllowedLogLevel;
    ghLwioLog = hLog;

#ifdef LW_SUPPORT_NANOSECOND_TIMESTAMP

#if defined(HAVE_CLOCK_GETRES) && defined(HAVE_CLOCK_GETTIME)
    {
        struct timespec ts;

        if (!clock_getres(CLOCK_REALTIME, &ts) && (ts.tv_nsec == 1))
        {
            gbLwioLogDoNanoSecondTime = TRUE;
        }
    }
#endif /* defined(HAVE_CLOCK_GETRES) && defined(HAVE_CLOCK_GETTIME) */

#endif /* LW_SUPPORT_NANOSECOND_TIMESTAMP */

 cleanup:
    
    return dwError;

 error:
 
    gLWIO_LOG_TARGET = LWIO_LOG_TARGET_DISABLED;
    ghLwioLog = (HANDLE)NULL;

    goto cleanup;
}

PCSTR
LwioLogLevelGetLabel(
    LWIO_LOG_LEVEL logLevel
    )
{
    switch (logLevel)
    {
        case LWIO_LOG_LEVEL_ALWAYS:

            return LWIO_ALWAYS_TAG;

        case LWIO_LOG_LEVEL_ERROR:

            return LWIO_ERROR_TAG;

        case LWIO_LOG_LEVEL_WARNING:

            return LWIO_WARN_TAG;

        case LWIO_LOG_LEVEL_INFO:

            return LWIO_INFO_TAG;

        case LWIO_LOG_LEVEL_VERBOSE:

            return LWIO_VERBOSE_TAG;

        case LWIO_LOG_LEVEL_DEBUG:

            return LWIO_DEBUG_TAG;

        case LWIO_LOG_LEVEL_TRACE:

            return LWIO_TRACE_TAG;

        default:

            return LWIO_UNKNOWN_TAG;
    }
}

DWORD
LwioLogGetInfo(
    PLWIO_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLWIO_LOG_INFO pLogInfo = NULL;
    
    switch(gLWIO_LOG_TARGET)
    {
        case LWIO_LOG_TARGET_DISABLED:
        case LWIO_LOG_TARGET_CONSOLE:
        case LWIO_LOG_TARGET_SYSLOG:
            
            dwError = LwIoAllocateMemory(
                            sizeof(LWIO_LOG_INFO),
                            (PVOID*)&pLogInfo);
            BAIL_ON_LWIO_ERROR(dwError);
            
            pLogInfo->logTarget = gLWIO_LOG_TARGET;
            pLogInfo->maxAllowedLogLevel = gLwioMaxLogLevel;
            
            break;
            
        case LWIO_LOG_TARGET_FILE:
            
            dwError = LwioGetFileLogInfo(
                            ghLwioLog,
                            &pLogInfo);
            BAIL_ON_LWIO_ERROR(dwError);
            
            break;
            
        default:
            dwError = LWIO_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWIO_ERROR(dwError);
    }
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    *ppLogInfo = NULL;
    
    if (pLogInfo)
    {
        LwIoFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
LwioLogSetInfo(
    PLWIO_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    
    BAIL_ON_INVALID_POINTER(pLogInfo);
    
    // The only information that is allowed
    // to be set after the log is initialized
    // is the log level
    
    gLwioMaxLogLevel = pLogInfo->maxAllowedLogLevel;
    
    switch (gLWIO_LOG_TARGET)
    {
        case LWIO_LOG_TARGET_SYSLOG:
            
            LwioSetSyslogMask(LWIO_LOG_LEVEL_DEBUG);
            
            break;
            
        default:
            
            break;
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;    
}

DWORD
LwioShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;
    
    if (ghLwioLog != (HANDLE)NULL)
    {
        switch(gLWIO_LOG_TARGET)
        {
            case LWIO_LOG_TARGET_DISABLED:
                break;
                
            case LWIO_LOG_TARGET_CONSOLE:
                LwioCloseConsoleLog(ghLwioLog);
                break;
                
            case LWIO_LOG_TARGET_FILE:
                LwioCloseFileLog(ghLwioLog);
                break;
                
            case LWIO_LOG_TARGET_SYSLOG:
                LwioCloseSyslog(ghLwioLog);
            break;
        }
    }
    
    return dwError;
}

DWORD
LwioSetupLogging(
	HANDLE              hLog,
	LWIO_LOG_LEVEL         maxAllowedLogLevel,
	PFN_LWIO_LOG_MESSAGE pfnLogger
	)
{
	DWORD dwError = 0;
	
	if ((hLog == (HANDLE)NULL) ||
		!pfnLogger)
	{
		dwError = LWIO_ERROR_INVALID_PARAMETER;
		goto error;
	}
	
	ghLwioLog = hLog;
	gLwioMaxLogLevel = maxAllowedLogLevel;
	gpfnLwioLogger = pfnLogger;
	
error:

	return dwError;
}

VOID
LwioResetLogging(
    VOID
    )
{
	gLwioMaxLogLevel = LWIO_LOG_LEVEL_ERROR;
	gpfnLwioLogger = NULL;
	ghLwioLog = (HANDLE)NULL;
}

VOID
LwioLogMessage(
	PFN_LWIO_LOG_MESSAGE pfnLogger,
	HANDLE hLog,
	LWIO_LOG_LEVEL logLevel,
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
LwioValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch(dwLogLevel)
    {
        case LWIO_LOG_LEVEL_ALWAYS:
        case LWIO_LOG_LEVEL_ERROR:
        case LWIO_LOG_LEVEL_WARNING:
        case LWIO_LOG_LEVEL_INFO:
        case LWIO_LOG_LEVEL_VERBOSE:
        case LWIO_LOG_LEVEL_DEBUG:
        case LWIO_LOG_LEVEL_TRACE:
            dwError = 0;
            break;
        default:
            dwError = LWIO_ERROR_INVALID_LOG_LEVEL;
            break;
    }
    
    return dwError;
}
