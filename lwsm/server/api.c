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
 *        server-api.c
 *
 * Abstract:
 *
 *        Server-side API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

DWORD
LwSmSrvAcquireServiceHandle(
    PCWSTR pwszName,
    PLW_SERVICE_HANDLE phHandle
    )
{
    DWORD dwError = 0;
    PSM_TABLE_ENTRY pEntry = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;

    dwError = LwSmTableGetEntry(pwszName, &pEntry);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*hHandle), OUT_PPVOID(&hHandle));
    BAIL_ON_ERROR(dwError);

    hHandle->pEntry = pEntry;
    *phHandle = hHandle;
    
cleanup:
    
    return dwError;
    
error:
    
    *phHandle = NULL;
    
    if (pEntry)
    {
        LwSmTableReleaseEntry(pEntry);
    }
    
    goto cleanup;
}

VOID
LwSmSrvReleaseHandle(
    LW_SERVICE_HANDLE hHandle
    )
{
    LwSmTableReleaseEntry(hHandle->pEntry);
    
    LwFreeMemory(hHandle);

    return;
}

DWORD
LwSmSrvStartService(
    LW_SERVICE_HANDLE hHandle
    )
{
    return LwSmTableStartEntry(hHandle->pEntry);
}

DWORD
LwSmSrvStopService(
    LW_SERVICE_HANDLE hHandle
    )
{
    return LwSmTableStopEntry(hHandle->pEntry);
}

DWORD
LwSmSrvEnumerateServices(
    PWSTR** pppwszServiceNames
    )
{
    return LwSmTableEnumerateEntries(pppwszServiceNames);
}

DWORD
LwSmSrvGetServiceStatus(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_STATUS pStatus
    )
{
    return LwSmTableGetEntryStatus(hHandle->pEntry, pStatus);
}

DWORD
LwSmSrvRefreshService(
    LW_SERVICE_HANDLE hHandle
    );

DWORD
LwSmSrvGetServiceInfo(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, hHandle->pEntry->pLock);

    dwError = LwSmCopyServiceInfo(hHandle->pEntry->pInfo, ppInfo);
    BAIL_ON_ERROR(dwError);

error:

    UNLOCK(bLocked, hHandle->pEntry->pLock);

    return dwError;
}
