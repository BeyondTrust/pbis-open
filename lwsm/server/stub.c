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
 *        stub.c
 *
 * Abstract:
 *
 *        Logic for managing stub services
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
LwSmStubStart(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_STATE pState = LwSmGetServiceObjectData(pObject);
    
    if (*pState != LW_SERVICE_STATE_RUNNING)
    {
        *pState = LW_SERVICE_STATE_RUNNING;
        LwSmNotifyServiceObjectStateChange(pObject, LW_SERVICE_STATE_RUNNING);
    }

    return dwError;
}

static
DWORD
LwSmStubStop(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_STATE pState = LwSmGetServiceObjectData(pObject);
    
    if (*pState != LW_SERVICE_STATE_STOPPED)
    {
        *pState = LW_SERVICE_STATE_STOPPED;
        LwSmNotifyServiceObjectStateChange(pObject, LW_SERVICE_STATE_RUNNING);
    }

    return dwError;
}

static
DWORD
LwSmStubGetStatus(
    PLW_SERVICE_OBJECT pObject,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_STATE pState = LwSmGetServiceObjectData(pObject);

    pStatus->home = LW_SERVICE_HOME_SERVICE_MANAGER;
    pStatus->pid = getpid();
    pStatus->state = *pState;
    
    return dwError;
}

static
DWORD
LwSmStubRefresh(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
DWORD
LwSmStubConstruct(
    PLW_SERVICE_OBJECT pObject,
    PCLW_SERVICE_INFO pInfo,
    PVOID* ppData
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_STATE pState = NULL;

    dwError = LwAllocateMemory(sizeof(*pState), OUT_PPVOID(&pState));
    BAIL_ON_ERROR(dwError);

    *ppData = pState;

error:

    return dwError;
}

static
VOID
LwSmStubDestruct(
    PLW_SERVICE_OBJECT pObject
    )
{
    LwFreeMemory(LwSmGetServiceObjectData(pObject));

    return;
}

LW_SERVICE_LOADER_VTBL gStubVtbl =
{
    .pfnStart = LwSmStubStart,
    .pfnStop = LwSmStubStop,
    .pfnGetStatus = LwSmStubGetStatus,
    .pfnRefresh = LwSmStubRefresh,
    .pfnConstruct = LwSmStubConstruct,
    .pfnDestruct = LwSmStubDestruct
};
