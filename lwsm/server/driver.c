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

static
DWORD
LwSmDriverStart(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PWSTR pwszName = LwSmGetServiceObjectData(pObject);

    dwError = LwNtStatusToWin32Error(LwIoLoadDriver(pwszName));
    BAIL_ON_ERROR(dwError);

    LwSmNotifyServiceObjectStateChange(pObject, LW_SERVICE_STATE_RUNNING);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmDriverStop(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PWSTR pwszName = LwSmGetServiceObjectData(pObject);

    dwError = LwNtStatusToWin32Error(LwIoUnloadDriver(pwszName));
    BAIL_ON_ERROR(dwError);

    LwSmNotifyServiceObjectStateChange(pObject, LW_SERVICE_STATE_STOPPED);

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
    LWIO_DRIVER_STATUS driverStatus = 0;
    PWSTR pwszName = LwSmGetServiceObjectData(pObject);

    pStatus->home = LW_SERVICE_HOME_IO_MANAGER;

    dwError = LwNtStatusToWin32Error(LwIoGetPid(&pStatus->pid));
    
    if (!dwError)
    {
        dwError = LwNtStatusToWin32Error(LwIoGetDriverStatus(
                                             pwszName,
                                             &driverStatus));
    }

    if (dwError)
    {
        pStatus->state = LW_SERVICE_STATE_STOPPED;
        dwError = 0;
    }
    else switch(driverStatus)
    {
    case LWIO_DRIVER_LOADED:
        pStatus->state = LW_SERVICE_STATE_RUNNING;
        break;
    case LWIO_DRIVER_UNLOADED:
        pStatus->state = LW_SERVICE_STATE_STOPPED;
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
    }
    
cleanup:
    
    return dwError;

error:

    goto cleanup;
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
    PWSTR pwszName = NULL;

    dwError = LwSmCopyString(pInfo->pwszName, &pwszName);
    BAIL_ON_ERROR(dwError);

    *ppData = pwszName;

error:

    return dwError;
}

static
VOID
LwSmDriverDestruct(
    PLW_SERVICE_OBJECT pObject
    )
{
    LwFreeMemory(LwSmGetServiceObjectData(pObject));

    return;
}

static
LW_SERVICE_LOADER_VTBL gDriverVtbl =
{
    .pfnStart = LwSmDriverStart,
    .pfnStop = LwSmDriverStop,
    .pfnGetStatus = LwSmDriverGetStatus,
    .pfnRefresh = LwSmDriverRefresh,
    .pfnConstruct = LwSmDriverConstruct,
    .pfnDestruct = LwSmDriverDestruct
};

static
LW_SERVICE_LOADER_PLUGIN gPlugin =
{
    .dwInterfaceVersion = LW_SERVICE_LOADER_INTERFACE_VERSION,
    .pVtbl = &gDriverVtbl,
    .pszName = "driver",
    .pszAuthor = "Likewise",
    .pszLicense = "GPLv2"
};

DWORD
ServiceLoaderInit(
    DWORD dwInterfaceVersion,
    PLW_SERVICE_LOADER_PLUGIN* ppPlugin
    )
{
    *ppPlugin = &gPlugin;
    
    return LW_ERROR_SUCCESS;
}
