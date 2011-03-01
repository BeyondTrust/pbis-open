/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        svcm.c
 *
 * Abstract:
 *
 *        Logic for managing service modules
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

typedef struct
{
    LW_SERVICE_STATE State;
    PLW_SVCM_INSTANCE pInstance;
    PWSTR* ppArgs;
    DWORD FdLimit;
} SVCM_STATE, *PSVCM_STATE;

pthread_mutex_t gLimitMutex = PTHREAD_MUTEX_INITIALIZER;

static
DWORD
SetLimits(
    PSVCM_STATE pState
    )
{
    DWORD dwError = 0;
    struct rlimit limit = {0};

    pthread_mutex_lock(&gLimitMutex);

    if (pState->FdLimit)
    {
        (void) getrlimit(RLIMIT_NOFILE, &limit);

        if (pState->FdLimit > limit.rlim_cur)
        {
            limit.rlim_cur = pState->FdLimit;
        }

        if (pState->FdLimit > limit.rlim_max)
        {
            limit.rlim_max = pState->FdLimit;
        }

        (void) setrlimit(RLIMIT_NOFILE, &limit);
    }

    pthread_mutex_unlock(&gLimitMutex);

    return dwError;
}

static
VOID
NotifyStart(
    PLW_SVCM_INSTANCE pInstance,
    NTSTATUS Status,
    PVOID pContext
    )
{
    PLW_SERVICE_OBJECT pObject = pContext;
    PSVCM_STATE pState = LwSmGetServiceObjectData(pObject);

    if (Status == STATUS_SUCCESS)
    {
        pState->State = LW_SERVICE_STATE_RUNNING;
    }
    else
    {
        pState->State = LW_SERVICE_STATE_DEAD;
    }

    LwSmNotifyServiceObjectStateChange(pObject, pState->State);
}

static
VOID
NotifyStop(
    PLW_SVCM_INSTANCE pInstance,
    NTSTATUS Status,
    PVOID pContext
    )
{
    PLW_SERVICE_OBJECT pObject = pContext;
    PSVCM_STATE pState = LwSmGetServiceObjectData(pObject);

    if (Status == STATUS_SUCCESS)
    {
        pState->State = LW_SERVICE_STATE_STOPPED;
    }
    else
    {
        pState->State = LW_SERVICE_STATE_DEAD;
    }

    LwSmNotifyServiceObjectStateChange(pObject, pState->State);
}

static
DWORD
LwSmSvcmStart(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSVCM_STATE pState = LwSmGetServiceObjectData(pObject);
    ULONG ArgCount = 0;

    for (ArgCount = 0; pState->ppArgs[ArgCount]; ArgCount++);

    pState->State = LW_SERVICE_STATE_STARTING;

    dwError = SetLimits(pState);
    BAIL_ON_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        LwRtlSvcmStart(
            pState->pInstance,
            ArgCount,
            pState->ppArgs,
            0,
            NULL,
            NotifyStart,
            pObject));
    BAIL_ON_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmSvcmStop(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSVCM_STATE pState = LwSmGetServiceObjectData(pObject);

    pState->State = LW_SERVICE_STATE_STOPPING;

    dwError = LwNtStatusToWin32Error(
           LwRtlSvcmStop(
               pState->pInstance,
               NotifyStop,
               pObject));
    BAIL_ON_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmSvcmGetStatus(
    PLW_SERVICE_OBJECT pObject,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    PSVCM_STATE pState = LwSmGetServiceObjectData(pObject);

    pStatus->home = LW_SERVICE_HOME_SERVICE_MANAGER;
    pStatus->pid = getpid();
    pStatus->state = pState->State;
    
    return dwError;
}

static
DWORD
LwSmSvcmRefresh(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSVCM_STATE pState = LwSmGetServiceObjectData(pObject);

    dwError = LwNtStatusToWin32Error(
        LwRtlSvcmRefresh(
            pState->pInstance,
            NULL,
            NULL));
    BAIL_ON_ERROR(dwError);

cleanup:

     return dwError;

error:

     goto cleanup;
}

static
DWORD
LwSmSvcmConstruct(
    PLW_SERVICE_OBJECT pObject,
    PCLW_SERVICE_INFO pInfo,
    PVOID* ppData
    )
{
    DWORD dwError = 0;
    PSVCM_STATE pState = NULL;

    dwError = LwAllocateMemory(sizeof(*pState), OUT_PPVOID(&pState));
    BAIL_ON_ERROR(dwError);

    pState->State = LW_SERVICE_STATE_STOPPED;
    pState->FdLimit = pInfo->dwFdLimit;

    dwError = LwSmCopyStringList(pInfo->ppwszArgs, &pState->ppArgs);
    BAIL_ON_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        LwRtlSvcmLoad(
            pInfo->pwszName,
            pInfo->pwszPath,
            &pState->pInstance));
    BAIL_ON_ERROR(dwError);

    *ppData = pState;

error:

    return dwError;
}

static
VOID
LwSmSvcmDestruct(
    PLW_SERVICE_OBJECT pObject
    )
{
    PSVCM_STATE pState = LwSmGetServiceObjectData(pObject);

    if (pState)
    {
        LwRtlSvcmUnload(pState->pInstance);
        LwSmFreeStringList(pState->ppArgs);
        LwFreeMemory(pState);
    }

    return;
}

LW_SERVICE_LOADER_VTBL gSvcmVtbl =
{
    .pfnStart = LwSmSvcmStart,
    .pfnStop = LwSmSvcmStop,
    .pfnGetStatus = LwSmSvcmGetStatus,
    .pfnRefresh = LwSmSvcmRefresh,
    .pfnConstruct = LwSmSvcmConstruct,
    .pfnDestruct = LwSmSvcmDestruct
};

