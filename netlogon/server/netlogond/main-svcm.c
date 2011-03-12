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

#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(netlogon)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
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

    strcpy(gpLwnetServerInfo->szCachePath, LWNET_CACHE_DIR);
    strcpy(gpLwnetServerInfo->szPrefixPath, PREFIXDIR);

    return (dwError);
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
    
    if (IsNullOrEmptyString(gpLwnetServerInfo->szCachePath)) {
      dwError = ERROR_PATH_NOT_FOUND;
      BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateString(gpLwnetServerInfo->szCachePath, &pszPath);
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
    
    if (IsNullOrEmptyString(gpLwnetServerInfo->szPrefixPath)) {
      dwError = ERROR_PATH_NOT_FOUND;
      BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateString(gpLwnetServerInfo->szPrefixPath, &pszPath);
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

