/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include <lwerror.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lwsm/lwsm.h>

#define BAIL_ON_ERROR(dwError) if (dwError) { goto error; }
DWORD
DJGetServiceStatus(
    PCSTR pszServiceName,
    PBOOLEAN pbStarted
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_STATUS status = {0};
    BOOLEAN bStarted = FALSE;

    dwError = LwMbsToWc16s(pszServiceName, &pwszServiceName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmQueryServiceStatus(hHandle, &status);
    BAIL_ON_ERROR(dwError);

    switch (status.state)
    {
        case LW_SERVICE_STATE_RUNNING:
            bStarted = TRUE;
            break;
        default:
            break;
    }

    *pbStarted = bStarted;

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
DJStartService(
    PCSTR pszServiceName
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    LW_SERVICE_STATUS status = {0};
    PWSTR* ppwszDependencies = NULL;
    PSTR pszTemp = NULL;
    size_t i = 0;

    dwError = LwMbsToWc16s(pszServiceName, &pwszServiceName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmQueryServiceDependencyClosure(hHandle, &ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDependencies[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszDependencies[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceStatus(hDepHandle, &status);
        BAIL_ON_ERROR(dwError);

        if (status.state != LW_SERVICE_STATE_RUNNING)
        {
            dwError = LwSmStartService(hDepHandle);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmStartService(hHandle);
    BAIL_ON_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (ppwszDependencies)
    {
        LwSmFreeServiceNameList(ppwszDependencies);
    }

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
DJStopService(
    PCSTR pszServiceName
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    LW_SERVICE_STATUS status = {0};
    PWSTR* ppwszDependencies = NULL;
    PSTR pszTemp = NULL;
    size_t i = 0;

    dwError = LwMbsToWc16s(pszServiceName, &pwszServiceName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmQueryServiceReverseDependencyClosure(hHandle, &ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDependencies[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszDependencies[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceStatus(hDepHandle, &status);
        BAIL_ON_ERROR(dwError);

        if (status.state != LW_SERVICE_STATE_STOPPED)
        {
            dwError = LwSmStopService(hDepHandle);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmStopService(hHandle);
    BAIL_ON_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (ppwszDependencies)
    {
        LwSmFreeServiceNameList(ppwszDependencies);
    }

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
    }

    return dwError;

error:

    goto cleanup;
}

