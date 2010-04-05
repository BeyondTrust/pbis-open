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
 *        Likewise IO (NFSSVC)
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
NfsSvcInitLogging(
    PCSTR         pszProgramName,
    NFSSVC_LOG_TARGET  logTarget,
    NFSSVC_LOG_LEVEL   maxAllowedLogLevel,
    PCSTR         pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case NFSSVC_LOG_TARGET_DISABLED:
            
            break;
            
        case NFSSVC_LOG_TARGET_SYSLOG:
        
            dwError = NfsSvcOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_NFSSVC_ERROR(dwError);
      
            break;
            
        case NFSSVC_LOG_TARGET_CONSOLE:

            dwError = NfsSvcOpenConsoleLog(
                            maxAllowedLogLevel,
                            &hLog);
            BAIL_ON_NFSSVC_ERROR(dwError);
              
            break;

        case NFSSVC_LOG_TARGET_FILE:
            
            if (IsNullOrEmptyString(pszPath))
            {
                dwError = NFSSVC_ERROR_INVALID_PARAMETER;
                BAIL_ON_NFSSVC_ERROR(dwError);
            }
                        
            dwError = NfsSvcOpenFileLog(
                          pszPath,
                          maxAllowedLogLevel,
                          &hLog);
            BAIL_ON_NFSSVC_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = NFSSVC_ERROR_INVALID_PARAMETER;
            BAIL_ON_NFSSVC_ERROR(dwError);      
    }
    
    gNFSSVC_LOG_TARGET = logTarget;
    gNfsSvcMaxLogLevel = maxAllowedLogLevel;
    ghNfsSvcLog = hLog;

 cleanup:
    
    return dwError;

 error:
 
    gNFSSVC_LOG_TARGET = NFSSVC_LOG_TARGET_DISABLED;
    ghNfsSvcLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
NfsSvcLogGetInfo(
    PNFSSVC_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PNFSSVC_LOG_INFO pLogInfo = NULL;
    
    switch(gNFSSVC_LOG_TARGET)
    {
        case NFSSVC_LOG_TARGET_DISABLED:
        case NFSSVC_LOG_TARGET_CONSOLE:
        case NFSSVC_LOG_TARGET_SYSLOG:
            
            dwError = LwAllocateMemory(
                            sizeof(NFSSVC_LOG_INFO),
                            (PVOID*)&pLogInfo);
            BAIL_ON_NFSSVC_ERROR(dwError);
            
            pLogInfo->logTarget = gNFSSVC_LOG_TARGET;
            pLogInfo->maxAllowedLogLevel = gNfsSvcMaxLogLevel;
            
            break;
            
        case NFSSVC_LOG_TARGET_FILE:
            
            dwError = NfsSvcGetFileLogInfo(
                            ghNfsSvcLog,
                            &pLogInfo);
            BAIL_ON_NFSSVC_ERROR(dwError);
            
            break;
            
        default:
            dwError = NFSSVC_ERROR_INVALID_PARAMETER;
            BAIL_ON_NFSSVC_ERROR(dwError);
    }
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    *ppLogInfo = NULL;
    
    if (pLogInfo)
    {
        NfsSvcFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
NfsSvcLogSetInfo(
    PNFSSVC_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    
    if (!pLogInfo)
    {
        dwError = NFSSVC_ERROR_INVALID_PARAMETER;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }
    
    // The only information that is allowed
    // to be set after the log is initialized
    // is the log level
    
    gNfsSvcMaxLogLevel = pLogInfo->maxAllowedLogLevel;
    
    switch (gNFSSVC_LOG_TARGET)
    {
        case NFSSVC_LOG_TARGET_SYSLOG:
            
            NfsSvcSetSyslogMask(gNfsSvcMaxLogLevel);
            
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
NfsSvcShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;
    
    if (ghNfsSvcLog != (HANDLE)NULL)
    {
        switch(gNFSSVC_LOG_TARGET)
        {
            case NFSSVC_LOG_TARGET_DISABLED:
                break;
                
            case NFSSVC_LOG_TARGET_CONSOLE:
                NfsSvcCloseConsoleLog(ghNfsSvcLog);
                break;
                
            case NFSSVC_LOG_TARGET_FILE:
                NfsSvcCloseFileLog(ghNfsSvcLog);
                break;
                
            case NFSSVC_LOG_TARGET_SYSLOG:
                NfsSvcCloseSyslog(ghNfsSvcLog);
            break;
        }
    }
    
    return dwError;
}

DWORD
NfsSvcSetupLogging(
	HANDLE              hLog,
	NFSSVC_LOG_LEVEL         maxAllowedLogLevel,
	PFN_NFSSVC_LOG_MESSAGE pfnLogger
	)
{
	DWORD dwError = 0;
	
	if ((hLog == (HANDLE)NULL) ||
		!pfnLogger)
	{
		dwError = NFSSVC_ERROR_INVALID_PARAMETER;
		goto error;
	}
	
	ghNfsSvcLog = hLog;
	gNfsSvcMaxLogLevel = maxAllowedLogLevel;
	gpfnNfsSvcLogger = pfnLogger;
	
error:

	return dwError;
}

VOID
NfsSvcResetLogging(
    VOID
    )
{
	gNfsSvcMaxLogLevel = NFSSVC_LOG_LEVEL_ERROR;
	gpfnNfsSvcLogger = NULL;
	ghNfsSvcLog = (HANDLE)NULL;
}

VOID
NfsSvcLogMessage(
	PFN_NFSSVC_LOG_MESSAGE pfnLogger,
	HANDLE hLog,
	NFSSVC_LOG_LEVEL logLevel,
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
NfsSvcValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch(dwLogLevel)
    {
        case NFSSVC_LOG_LEVEL_ALWAYS:
        case NFSSVC_LOG_LEVEL_ERROR:
        case NFSSVC_LOG_LEVEL_WARNING:
        case NFSSVC_LOG_LEVEL_INFO:
        case NFSSVC_LOG_LEVEL_VERBOSE:
        case NFSSVC_LOG_LEVEL_DEBUG:
            dwError = 0;
            break;
        default:
            dwError = NFSSVC_ERROR_INVALID_PARAMETER;
            break;
    }
    
    return dwError;
}
