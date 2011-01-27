/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
LWNetSvcmParseArgs(
    ULONG ArgCount,
    PWSTR* ppArgs,
    PLWNETSERVERINFO pLWNetServerInfo
    );

NTSTATUS
LWNetSvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

VOID
LWNetSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}

NTSTATUS
LWNetSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    DWORD dwError = 0;

    dwError = LWNetSrvSetDefaults();
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSvcmParseArgs(ArgCount, ppArgs, &gServerInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvInitLogging("netlogon");
    BAIL_ON_LWNET_ERROR(dwError);

    LWNET_LOG_VERBOSE("Logging started");

    dwError = LWNetSrvInitialize();
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvStartListenThread();
    BAIL_ON_LWNET_ERROR(dwError);

    // Post service started event to eventlog
    LWNetSrvLogProcessStartedEvent();

cleanup:

    return LwWin32ErrorToNtStatus(dwError);

error:

    goto cleanup;
}

NTSTATUS
LWNetSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    LWNET_LOG_VERBOSE("LWNet main cleaning up");

    // Post service stopped event to eventlog
    LWNetSrvLogProcessStoppedEvent(ERROR_SUCCESS);

    LWNetSrvStopListenThread();

    LWNetSrvApiShutdown();

    LWNET_LOG_INFO("LWNET Service exiting...");

    lwnet_close_log();

    return STATUS_SUCCESS;
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = LWNetSvcmInit,
    .Destroy = LWNetSvcmDestroy,
    .Start = LWNetSvcmStart,
    .Stop = LWNetSvcmStop
};

PLW_SVCM_MODULE
(LW_RTL_SVCM_ENTRY_POINT_NAME)(
    VOID
    )
{
    return &gService;
}


DWORD
LWNetSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    gpServerInfo->dwLogLevel = LWNET_LOG_LEVEL_ERROR;

    *(gpServerInfo->szLogFilePath) = '\0';

    strcpy(gpServerInfo->szCachePath, LWNET_CACHE_DIR);
    strcpy(gpServerInfo->szPrefixPath, PREFIXDIR);

    setlocale(LC_ALL, "");

    return (dwError);
}

static
DWORD
LWNetSvcmParseArgs(
    ULONG ArgCount,
    PWSTR* ppArgs,
    PLWNETSERVERINFO pLWNetServerInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int iArg = 0;
    static const WCHAR paramLogFile[] = {'-','-','l','o','g','f','i','l','e','\0'};
    static const WCHAR paramLogLevel[] = {'-','-','l','o','g','l','e','v','e','l','\0'};
    static const WCHAR paramSyslog[] = {'-','-','s','y','s','l','o','g','\0'};
    static const WCHAR paramError[] = {'e','r','r','o','r','\0'};
    static const WCHAR paramWarning[] = {'w','a','r','n','i','n','g','\0'};
    static const WCHAR paramInfo[] = {'i','n','f','o','\0'};
    static const WCHAR paramVerbose[] = {'v','e','r','b','o','s','e','\0'};
    static const WCHAR paramDebug[] = {'d','e','b','u','g','\0'};
    static const WCHAR paramTrace[] = {'t','r','a','c','e','\0'};
    PSTR pConverted = NULL;

    for (iArg = 0; iArg < ArgCount; iArg++)
    {
        PWSTR pArg = ppArgs[iArg];

        if (LwRtlWC16StringIsEqual(pArg, paramLogFile, TRUE))
        {
            if (iArg + 1 >= ArgCount)
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_LWNET_ERROR(dwError);
            }
            pArg = ppArgs[++iArg];
            dwError = LwWc16sToMbs(pArg, &pConverted);
            strcpy(pLWNetServerInfo->szLogFilePath, pConverted);
            LW_SAFE_FREE_STRING(pConverted);
        }
        else if (LwRtlWC16StringIsEqual(pArg, paramSyslog, TRUE))
        {
            pLWNetServerInfo->bLogToSyslog = TRUE;
        }
        else if (LwRtlWC16StringIsEqual(pArg, paramLogLevel, TRUE))
        {
            if (iArg + 1 >= ArgCount)
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_LWNET_ERROR(dwError);
            }
            pArg = ppArgs[++iArg];
            if (LwRtlWC16StringIsEqual(pArg, paramError, TRUE))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_ERROR;
            }
            else if (LwRtlWC16StringIsEqual(pArg, paramWarning, TRUE))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_WARNING;
            }
            else if (LwRtlWC16StringIsEqual(pArg, paramInfo, TRUE))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_INFO;
            }
            else if (LwRtlWC16StringIsEqual(pArg, paramVerbose, TRUE))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_VERBOSE;
            }
            else if (LwRtlWC16StringIsEqual(pArg, paramDebug, TRUE))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_DEBUG;
            }
            else if (LwRtlWC16StringIsEqual(pArg, paramTrace, TRUE))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_TRACE;
            }
            else
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }
        else
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }

error:

    LW_SAFE_FREE_STRING(pConverted);

    return dwError;
}

DWORD
LWNetSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    
    dwError = LWNetSrvApiInit();
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWNetSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;
  
    LWNET_LOCK_SERVERINFO(bInLock);
    
    if (IsNullOrEmptyString(gpServerInfo->szCachePath)) {
      dwError = ERROR_PATH_NOT_FOUND;
      BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateString(gpServerInfo->szCachePath, &pszPath);
    BAIL_ON_LWNET_ERROR(dwError);

    *ppszPath = pszPath;
    
 cleanup:

    LWNET_UNLOCK_SERVERINFO(bInLock);
    
    return dwError;

 error:

    LWNET_SAFE_FREE_STRING(pszPath);
    
    *ppszPath = NULL;

    goto cleanup;
}

DWORD
LWNetSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;
  
    LWNET_LOCK_SERVERINFO(bInLock);
    
    if (IsNullOrEmptyString(gpServerInfo->szPrefixPath)) {
      dwError = ERROR_PATH_NOT_FOUND;
      BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateString(gpServerInfo->szPrefixPath, &pszPath);
    BAIL_ON_LWNET_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:
    
    LWNET_UNLOCK_SERVERINFO(bInLock);
    
    return dwError;

 error:

    LWNET_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
LWNetSrvInitLogging(
    PCSTR pszProgramName
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    LWNET_LOCK_SERVERINFO(bInLock);

    if ((gpServerInfo->dwStartAsDaemon &&
            gpServerInfo->szLogFilePath[0] == '\0') ||
            gpServerInfo->bLogToSyslog)
    {
      
      dwError = lwnet_init_logging_to_syslog(gpServerInfo->dwLogLevel,
                                           gpServerInfo->bEnableDebugLogs,
                                           pszProgramName,
                                           LOG_PID,
                                           LOG_DAEMON);
      BAIL_ON_LWNET_ERROR(dwError);
      
    } else {
      
      dwError = lwnet_init_logging_to_file(gpServerInfo->dwLogLevel,
                                         gpServerInfo->bEnableDebugLogs,
                                         gpServerInfo->szLogFilePath);
      BAIL_ON_LWNET_ERROR(dwError);
      
    }

 cleanup:

    LWNET_UNLOCK_SERVERINFO(bInLock);
    
    return dwError;

 error:

    goto cleanup;
}

VOID
LWNetSrvLogProcessStartedEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise site manager service was started.");
    BAIL_ON_LWNET_ERROR(dwError);

    LWNetSrvLogInformationEvent(
            LWNET_EVENT_INFO_SERVICE_STARTED,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LWNET_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LWNetSrvLogProcessStoppedEvent(
    DWORD dwExitCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise site manager service was stopped");
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetErrorMessageForLoggingEvent(
                         dwExitCode,
                         &pszData);
    BAIL_ON_LWNET_ERROR(dwError);

    if (dwExitCode)
    {
        LWNetSrvLogErrorEvent(
                LWNET_EVENT_ERROR_SERVICE_STOPPED,
                SERVICE_EVENT_CATEGORY,
                pszDescription,
                pszData);
    }
    else
    {
        LWNetSrvLogInformationEvent(
                LWNET_EVENT_INFO_SERVICE_STOPPED,
                SERVICE_EVENT_CATEGORY,
                pszDescription,
                pszData);
    }

cleanup:

    LWNET_SAFE_FREE_STRING(pszDescription);
    LWNET_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

VOID
LWNetSrvLogProcessFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise site manager service stopped running due to an error");
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
    BAIL_ON_LWNET_ERROR(dwError);

    LWNetSrvLogErrorEvent(
            LWNET_EVENT_ERROR_SERVICE_START_FAILURE,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            pszData);

cleanup:

    LWNET_SAFE_FREE_STRING(pszDescription);
    LWNET_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

DWORD
LWNetGetErrorMessageForLoggingEvent(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg
    )
{
    DWORD dwErrorBufferSize = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszErrorMsg = NULL;
    PSTR  pszErrorBuffer = NULL;

    dwErrorBufferSize = LwGetErrorString(dwErrCode, NULL, 0);

    if (!dwErrorBufferSize)
        goto cleanup;

    dwError = LWNetAllocateMemory(
                dwErrorBufferSize,
                (PVOID*)&pszErrorBuffer);
    BAIL_ON_LWNET_ERROR(dwError);

    dwLen = LwGetErrorString(dwErrCode, pszErrorBuffer, dwErrorBufferSize);

    if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
    {
        dwError = LwAllocateStringPrintf(
                     &pszErrorMsg,
                     "Error: %s [error code: %d]",
                     pszErrorBuffer,
                     dwErrCode);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    *ppszErrorMsg = pszErrorMsg;

cleanup:

    LWNET_SAFE_FREE_STRING(pszErrorBuffer);

    return dwError;

error:

    LWNET_SAFE_FREE_STRING(pszErrorMsg);

    *ppszErrorMsg = NULL;

    goto cleanup;
}

