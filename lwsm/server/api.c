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

DWORD
LwSmPopulateTable(
    VOID
    )
{
    DWORD dwError = 0;
    HANDLE hReg = NULL;
    PWSTR* ppwszNames = NULL;
    PWSTR pwszName = NULL;
    PLW_SERVICE_INFO pInfo = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;
    size_t i = 0;

    SM_LOG_VERBOSE("Populating service table");

    dwError = RegOpenServer(&hReg);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryEnumServices(hReg, &ppwszNames);
    switch (dwError)
    {
    case LWREG_ERROR_NO_SUCH_KEY_OR_VALUE:
        /* No services in registry */
        dwError = 0;
        goto cleanup;
    }
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszNames[i]; i++)
    {
        pwszName = ppwszNames[i];

        LwSmCommonFreeServiceInfo(pInfo);
        pInfo = NULL;

        dwError = LwSmRegistryReadServiceInfo(hReg, pwszName, &pInfo);
        switch (dwError)
        {
        case LWREG_ERROR_NO_SUCH_KEY_OR_VALUE:
            dwError = 0;
            continue;
        default:
            break;
        }
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableGetEntry(pwszName, &pEntry);
        if (!dwError)
        {
            dwError = LwSmTableUpdateEntry(pEntry, pInfo, LW_SERVICE_INFO_MASK_ALL);
            BAIL_ON_ERROR(dwError);
        }
        else if (dwError == LW_ERROR_NO_SUCH_SERVICE)
        {
            dwError = LwSmTableAddEntry(pInfo, &pEntry);
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            BAIL_ON_ERROR(dwError);
        }

        LwSmTableReleaseEntry(pEntry);
        pEntry = NULL;
    }

cleanup:

    LwSmFreeStringList(ppwszNames);
    LwSmCommonFreeServiceInfo(pInfo);

    if (hReg)
    {
        RegCloseServer(hReg);
    }

    return dwError;

error:

    goto cleanup;
}

