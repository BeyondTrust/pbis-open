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
 *        Likewise Task System (LWTASK)
 *
 *        Utilities
 *
 *        Logger
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

DWORD
LwTaskInitLogging(
    PCSTR              pszProgramName,
    LW_TASK_LOG_TARGET logTarget,
    LW_TASK_LOG_LEVEL  maxAllowedLogLevel,
    PCSTR              pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case LW_TASK_LOG_TARGET_DISABLED:
            
            break;
            
        case LW_TASK_LOG_TARGET_SYSLOG:
        
            dwError = LwTaskOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_LW_TASK_ERROR(dwError);
      
            break;
            
        case LW_TASK_LOG_TARGET_CONSOLE:

            dwError = LwTaskOpenConsoleLog(
                            maxAllowedLogLevel,
                            &hLog);
            BAIL_ON_LW_TASK_ERROR(dwError);
              
            break;

        case LW_TASK_LOG_TARGET_FILE:
            
            if (IsNullOrEmptyString(pszPath))
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_LW_TASK_ERROR(dwError);
            }
                        
            dwError = LwTaskOpenFileLog(
                          pszPath,
                          maxAllowedLogLevel,
                          &hLog);
            BAIL_ON_LW_TASK_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_TASK_ERROR(dwError);
    }
    
    gLWTASK_LOG_TARGET = logTarget;
    gLwTaskMaxLogLevel = maxAllowedLogLevel;
    ghLwTaskLog = hLog;

 cleanup:
    
    return dwError;

 error:
 
    gLWTASK_LOG_TARGET = LW_TASK_LOG_TARGET_DISABLED;
    ghLwTaskLog = (HANDLE)NULL;

    goto cleanup;
}

PCSTR
LwTaskLogLevelGetLabel(
    LW_TASK_LOG_LEVEL logLevel
    )
{
    switch (logLevel)
    {
        case LW_TASK_LOG_LEVEL_ALWAYS:

            return LW_TASK_ALWAYS_TAG;

        case LW_TASK_LOG_LEVEL_ERROR:

            return LW_TASK_ERROR_TAG;

        case LW_TASK_LOG_LEVEL_WARNING:

            return LW_TASK_WARN_TAG;

        case LW_TASK_LOG_LEVEL_INFO:

            return LW_TASK_INFO_TAG;

        case LW_TASK_LOG_LEVEL_VERBOSE:

            return LW_TASK_VERBOSE_TAG;

        case LW_TASK_LOG_LEVEL_DEBUG:

            return LW_TASK_DEBUG_TAG;

        default:

            return LW_TASK_UNKNOWN_TAG;
    }
}

DWORD
LwTaskLogGetInfo(
    PLW_TASK_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLW_TASK_LOG_INFO pLogInfo = NULL;
    
    switch(gLWTASK_LOG_TARGET)
    {
        case LW_TASK_LOG_TARGET_DISABLED:
        case LW_TASK_LOG_TARGET_CONSOLE:
        case LW_TASK_LOG_TARGET_SYSLOG:
            
            dwError = LwAllocateMemory(
                            sizeof(LW_TASK_LOG_INFO),
                            (PVOID*)&pLogInfo);
            BAIL_ON_LW_TASK_ERROR(dwError);
            
            pLogInfo->logTarget = gLWTASK_LOG_TARGET;
            pLogInfo->maxAllowedLogLevel = gLwTaskMaxLogLevel;
            
            break;
            
        case LW_TASK_LOG_TARGET_FILE:
            
            dwError = LwTaskGetFileLogInfo(
                            ghLwTaskLog,
                            &pLogInfo);
            BAIL_ON_LW_TASK_ERROR(dwError);
            
            break;
            
        default:
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_TASK_ERROR(dwError);
    }
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    *ppLogInfo = NULL;
    
    if (pLogInfo)
    {
        LwTaskFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
LwTaskLogSetInfo(
    PLW_TASK_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    
    BAIL_ON_INVALID_POINTER(pLogInfo);
    
    // The only information that is allowed
    // to be set after the log is initialized
    // is the log level
    
    gLwTaskMaxLogLevel = pLogInfo->maxAllowedLogLevel;
    
    switch (gLWTASK_LOG_TARGET)
    {
        case LW_TASK_LOG_TARGET_SYSLOG:
            
            LwTaskSetSyslogMask(LW_TASK_LOG_LEVEL_DEBUG);
            
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
LwTaskShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;
    
    if (ghLwTaskLog != (HANDLE)NULL)
    {
        switch(gLWTASK_LOG_TARGET)
        {
            case LW_TASK_LOG_TARGET_DISABLED:
                break;
                
            case LW_TASK_LOG_TARGET_CONSOLE:
                LwTaskCloseConsoleLog(ghLwTaskLog);
                break;
                
            case LW_TASK_LOG_TARGET_FILE:
                LwTaskCloseFileLog(ghLwTaskLog);
                break;
                
            case LW_TASK_LOG_TARGET_SYSLOG:
                LwTaskCloseSyslog(ghLwTaskLog);
            break;
        }
    }
    
    return dwError;
}

DWORD
LwTaskSetupLogging(
	HANDLE                  hLog,
	LW_TASK_LOG_LEVEL       maxAllowedLogLevel,
	PFN_LW_TASK_LOG_MESSAGE pfnLogger
	)
{
	DWORD dwError = 0;
	
	if ((hLog == (HANDLE)NULL) || !pfnLogger)
	{
		dwError = ERROR_INVALID_PARAMETER;
		goto error;
	}
	
	ghLwTaskLog        = hLog;
	gLwTaskMaxLogLevel = maxAllowedLogLevel;
	gpfnLwTaskLogger   = pfnLogger;
	
error:

	return dwError;
}

VOID
LwTaskResetLogging(
    VOID
    )
{
	gLwTaskMaxLogLevel = LW_TASK_LOG_LEVEL_ERROR;
	gpfnLwTaskLogger = NULL;
	ghLwTaskLog = (HANDLE)NULL;
}

VOID
LwTaskLogMessage(
	PFN_LW_TASK_LOG_MESSAGE pfnLogger,
	HANDLE                  hLog,
	LW_TASK_LOG_LEVEL       logLevel,
	PCSTR                   pszFormat,
	...
	)
{
	va_list msgList;
	va_start(msgList, pszFormat);
	
	pfnLogger(hLog, logLevel, pszFormat, msgList);
	
	va_end(msgList);
}

DWORD
LwTaskValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch(dwLogLevel)
    {
        case LW_TASK_LOG_LEVEL_ALWAYS:
        case LW_TASK_LOG_LEVEL_ERROR:
        case LW_TASK_LOG_LEVEL_WARNING:
        case LW_TASK_LOG_LEVEL_INFO:
        case LW_TASK_LOG_LEVEL_VERBOSE:
        case LW_TASK_LOG_LEVEL_DEBUG:
            dwError = 0;
            break;
        default:
            dwError = ERROR_INVALID_PARAMETER;
            break;
    }
    
    return dwError;
}
