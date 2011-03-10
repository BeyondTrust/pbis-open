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
