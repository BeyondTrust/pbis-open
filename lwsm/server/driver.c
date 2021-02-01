/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Module Name:
 *
 *        driver.c
 *
 * Abstract:
 *
 *        Logic for managing lwio drivers
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

typedef struct
{
    LONG volatile State;
    PWSTR pName;
} DRIVER_STATE, *PDRIVER_STATE;

static
DWORD
LwSmDriverStart(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PDRIVER_STATE pState = LwSmGetServiceObjectData(pObject);

    LwInterlockedCompareExchange(&pState->State, LW_SERVICE_STATE_STOPPED, LW_SERVICE_STATE_DEAD);

    if (LwInterlockedCompareExchange(&pState->State, LW_SERVICE_STATE_STARTING, LW_SERVICE_STATE_STOPPED) == LW_SERVICE_STATE_STOPPED)
    {
        dwError = LwNtStatusToWin32Error(LwIoLoadDriver(pState->pName));

        pState->State = dwError ? LW_SERVICE_STATE_DEAD : LW_SERVICE_STATE_RUNNING;
        LwSmNotifyServiceObjectStateChange(pObject, pState->State);
    }

    return 0;
}

static
DWORD
LwSmDriverStop(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PDRIVER_STATE pState = LwSmGetServiceObjectData(pObject);

    if (LwInterlockedCompareExchange(&pState->State, LW_SERVICE_STATE_STOPPING, LW_SERVICE_STATE_RUNNING) == LW_SERVICE_STATE_RUNNING)
    {
        dwError = LwNtStatusToWin32Error(LwIoUnloadDriver(pState->pName));
        BAIL_ON_ERROR(dwError);

        pState->State = LW_SERVICE_STATE_STOPPED;
        LwSmNotifyServiceObjectStateChange(pObject, LW_SERVICE_STATE_STOPPED);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmDriverGetStatus(
    PLW_SERVICE_OBJECT pObject,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    LWIO_DRIVER_STATE driverState = 0;
    PDRIVER_STATE pState = LwSmGetServiceObjectData(pObject);

    pStatus->home = LW_SERVICE_HOME_IO_MANAGER;

    dwError = LwNtStatusToWin32Error(LwIoGetPid(&pStatus->pid));
    if (dwError)
    {
        pStatus->pid = -1;
    }
    
    dwError = LwNtStatusToWin32Error(LwIoQueryStateDriver(pState->pName, &driverState));
    if (dwError || driverState != LWIO_DRIVER_STATE_LOADED)
    {
        LwInterlockedCompareExchange(&pState->State, LW_SERVICE_STATE_DEAD, LW_SERVICE_STATE_RUNNING);
    }

    pStatus->state = LwInterlockedRead(&pState->State);
    
    return 0;
}

static
DWORD
LwSmDriverRefresh(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
DWORD
LwSmDriverConstruct(
    PLW_SERVICE_OBJECT pObject,
    PCLW_SERVICE_INFO pInfo,
    PVOID* ppData
    )
{
    DWORD dwError = 0;
    PDRIVER_STATE pState = NULL;

    dwError = LwAllocateMemory(sizeof(*pState), OUT_PPVOID(&pState));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(pInfo->pwszName, &pState->pName);
    BAIL_ON_ERROR(dwError);

    pState->State = LW_SERVICE_STATE_STOPPED;

    *ppData = pState;

error:

    return dwError;
}

static
VOID
LwSmDriverDestruct(
    PLW_SERVICE_OBJECT pObject
    )
{
    PDRIVER_STATE pState = LwSmGetServiceObjectData(pObject);

    LW_SAFE_FREE_MEMORY(pState->pName);
    LW_SAFE_FREE_MEMORY(pState);

    return;
}

LW_SERVICE_LOADER_VTBL gDriverVtbl =
{
    .pfnStart = LwSmDriverStart,
    .pfnStop = LwSmDriverStop,
    .pfnGetStatus = LwSmDriverGetStatus,
    .pfnRefresh = LwSmDriverRefresh,
    .pfnConstruct = LwSmDriverConstruct,
    .pfnDestruct = LwSmDriverDestruct
};
