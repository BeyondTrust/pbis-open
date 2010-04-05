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
SMBInitLogging(
    PCSTR         pszProgramName,
    LWIO_LOG_TARGET  logTarget,
    LWIO_LOG_LEVEL   maxAllowedLogLevel,
    PCSTR         pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case LWIO_LOG_TARGET_DISABLED:
            
            break;
            
        case LWIO_LOG_TARGET_SYSLOG:
        
            dwError = SMBOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_LWIO_ERROR(dwError);
      
            break;
            
        case LWIO_LOG_TARGET_CONSOLE:

            dwError = SMBOpenConsoleLog(
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
                        
            dwError = SMBOpenFileLog(
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
    gSMBMaxLogLevel = maxAllowedLogLevel;
    ghSMBLog = hLog;

 cleanup:
    
    return dwError;

 error:
 
    gLWIO_LOG_TARGET = LWIO_LOG_TARGET_DISABLED;
    ghSMBLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
SMBLogGetInfo(
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
            
            dwError = SMBAllocateMemory(
                            sizeof(LWIO_LOG_INFO),
                            (PVOID*)&pLogInfo);
            BAIL_ON_LWIO_ERROR(dwError);
            
            pLogInfo->logTarget = gLWIO_LOG_TARGET;
            pLogInfo->maxAllowedLogLevel = gSMBMaxLogLevel;
            
            break;
            
        case LWIO_LOG_TARGET_FILE:
            
            dwError = SMBGetFileLogInfo(
                            ghSMBLog,
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
SMBLogSetInfo(
    PLWIO_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    
    BAIL_ON_INVALID_POINTER(pLogInfo);
    
    // The only information that is allowed
    // to be set after the log is initialized
    // is the log level
    
    gSMBMaxLogLevel = pLogInfo->maxAllowedLogLevel;
    
    switch (gLWIO_LOG_TARGET)
    {
        case LWIO_LOG_TARGET_SYSLOG:
            
            SMBSetSyslogMask(gSMBMaxLogLevel);
            
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
SMBShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;
    
    if (ghSMBLog != (HANDLE)NULL)
    {
        switch(gLWIO_LOG_TARGET)
        {
            case LWIO_LOG_TARGET_DISABLED:
                break;
                
            case LWIO_LOG_TARGET_CONSOLE:
                SMBCloseConsoleLog(ghSMBLog);
                break;
                
            case LWIO_LOG_TARGET_FILE:
                SMBCloseFileLog(ghSMBLog);
                break;
                
            case LWIO_LOG_TARGET_SYSLOG:
                SMBCloseSyslog(ghSMBLog);
            break;
        }
    }
    
    return dwError;
}

DWORD
SMBSetupLogging(
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
	
	ghSMBLog = hLog;
	gSMBMaxLogLevel = maxAllowedLogLevel;
	gpfnSMBLogger = pfnLogger;
	
error:

	return dwError;
}

VOID
SMBResetLogging(
    VOID
    )
{
	gSMBMaxLogLevel = LWIO_LOG_LEVEL_ERROR;
	gpfnSMBLogger = NULL;
	ghSMBLog = (HANDLE)NULL;
}

VOID
SMBLogMessage(
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
SMBValidateLogLevel(
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
