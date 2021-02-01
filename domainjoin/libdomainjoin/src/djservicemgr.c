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

