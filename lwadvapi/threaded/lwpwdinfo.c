/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwpwdinfo.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) 
 *                    
 *        Password Info Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
LwDupWStr(
    PCWSTR pwszInput,
    PWSTR* ppwszOutput
    )
{
    DWORD dwError = 0;
    size_t len = 0;
    PWSTR pwszOutput = NULL;

    dwError = LwWc16sLen(pwszInput, &len);
    BAIL_ON_LW_ERROR(dwError);

    if (len)
    {
        dwError = LwAllocateMemory(
                      (len + 1) * sizeof(*pwszOutput),
                      OUT_PPVOID(&pwszOutput));
        BAIL_ON_LW_ERROR(dwError);

        dwError = LwWc16sCpy(
                      pwszOutput,
                      pwszInput);
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppwszOutput = pwszOutput;
    pwszOutput = NULL;

error:

    if (dwError)
    {
        *ppwszOutput = NULL;
    } 

    LW_SAFE_FREE_MEMORY(pwszOutput);

    return dwError;
}

DWORD
LwDuplicatePasswordInfo(
    IN PLWPS_PASSWORD_INFO pInfo,
    OUT PLWPS_PASSWORD_INFO* ppInfoCopy
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pInfoCopy = NULL;

    if (!pInfo)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                  sizeof(*pInfoCopy),
                  OUT_PPVOID(&pInfoCopy));
    BAIL_ON_LW_ERROR(dwError);

    if (pInfo->pwszDomainName)
    {
        dwError = LwDupWStr(
                      pInfo->pwszDomainName,
                      &pInfoCopy->pwszDomainName);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszDnsDomainName)
    {
        dwError = LwDupWStr(
                      pInfo->pwszDnsDomainName,
                      &pInfoCopy->pwszDnsDomainName);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszSID)
    {
        dwError = LwDupWStr(
                      pInfo->pwszSID,
                      &pInfoCopy->pwszSID);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszHostname)
    {
        dwError = LwDupWStr(
                      pInfo->pwszHostname,
                      &pInfoCopy->pwszHostname);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszHostDnsDomain)
    {
        dwError = LwDupWStr(
                      pInfo->pwszHostDnsDomain,
                      &pInfoCopy->pwszHostDnsDomain);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszMachineAccount)
    {
        dwError = LwDupWStr(
                      pInfo->pwszMachineAccount,
                      &pInfoCopy->pwszMachineAccount);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszMachinePassword)
    {
        dwError = LwDupWStr(
                      pInfo->pwszMachinePassword,
                      &pInfoCopy->pwszMachinePassword);
        BAIL_ON_LW_ERROR(dwError);
    }

    pInfoCopy->last_change_time = pInfo->last_change_time;
    pInfoCopy->dwSchannelType = pInfo->dwSchannelType;

    *ppInfoCopy = pInfoCopy;
    pInfoCopy = NULL;

error:

    if (dwError)
    {
        *ppInfoCopy = NULL;
    }

    LwFreePasswordInfo(pInfoCopy);

    return dwError;
}

VOID
LwFreePasswordInfo(
    IN PLWPS_PASSWORD_INFO pInfo
    )
{
    if (pInfo)
    {
        LW_SAFE_FREE_MEMORY(pInfo->pwszDomainName);
        LW_SAFE_FREE_MEMORY(pInfo->pwszDnsDomainName);
        LW_SAFE_FREE_MEMORY(pInfo->pwszSID);
        LW_SAFE_FREE_MEMORY(pInfo->pwszHostname);
        LW_SAFE_FREE_MEMORY(pInfo->pwszHostDnsDomain);
        LW_SAFE_FREE_MEMORY(pInfo->pwszMachineAccount);
        LW_SECURE_FREE_WSTRING(pInfo->pwszMachinePassword);
        LW_SAFE_FREE_MEMORY(pInfo);
    }

    return;
}

DWORD
LwDuplicatePasswordInfoWToA(
    IN PLWPS_PASSWORD_INFO pInfo,
    OUT PLWPS_PASSWORD_INFO_A* ppInfoCopy
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO_A pInfoCopy = NULL;

    if (!pInfo)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                  sizeof(*pInfoCopy),
                  OUT_PPVOID(&pInfoCopy));
    BAIL_ON_LW_ERROR(dwError);

    if (pInfo->pwszDomainName)
    {
        dwError = LwWc16sToMbs(
                      pInfo->pwszDomainName,
                      &pInfoCopy->pszDomainName);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszDnsDomainName)
    {
        dwError = LwWc16sToMbs(
                      pInfo->pwszDnsDomainName,
                      &pInfoCopy->pszDnsDomainName);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszSID)
    {
        dwError = LwWc16sToMbs(
                      pInfo->pwszSID,
                      &pInfoCopy->pszSID);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszHostname)
    {
        dwError = LwWc16sToMbs(
                      pInfo->pwszHostname,
                      &pInfoCopy->pszHostname);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszHostDnsDomain)
    {
        dwError = LwWc16sToMbs(
                      pInfo->pwszHostDnsDomain,
                      &pInfoCopy->pszHostDnsDomain);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszMachineAccount)
    {
        dwError = LwWc16sToMbs(
                      pInfo->pwszMachineAccount,
                      &pInfoCopy->pszMachineAccount);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pwszMachinePassword)
    {
        dwError = LwWc16sToMbs(
                      pInfo->pwszMachinePassword,
                      &pInfoCopy->pszMachinePassword);
        BAIL_ON_LW_ERROR(dwError);
    }

    pInfoCopy->last_change_time = pInfo->last_change_time;
    pInfoCopy->dwSchannelType = pInfo->dwSchannelType;

    *ppInfoCopy = pInfoCopy;
    pInfoCopy = NULL;

error:

    if (dwError)
    {
        *ppInfoCopy = NULL;
    }

    LwFreePasswordInfoA(pInfoCopy);

    return dwError;
}

DWORD
LwDuplicatePasswordInfoA(
    IN PLWPS_PASSWORD_INFO_A pInfo,
    OUT PLWPS_PASSWORD_INFO_A* ppInfoCopy
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO_A pInfoCopy = NULL;

    if (!pInfo)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                  sizeof(*pInfoCopy),
                  OUT_PPVOID(&pInfoCopy));
    BAIL_ON_LW_ERROR(dwError);

    if (pInfo->pszDomainName)
    {
        dwError = LwAllocateString(
                      pInfo->pszDomainName,
                      &pInfoCopy->pszDomainName);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pszDnsDomainName)
    {
        dwError = LwAllocateString(
                      pInfo->pszDnsDomainName,
                      &pInfoCopy->pszDnsDomainName);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pszSID)
    {
        dwError = LwAllocateString(
                      pInfo->pszSID,
                      &pInfoCopy->pszSID);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pszHostname)
    {
        dwError = LwAllocateString(
                      pInfo->pszHostname,
                      &pInfoCopy->pszHostname);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pszHostDnsDomain)
    {
        dwError = LwAllocateString(
                      pInfo->pszHostDnsDomain,
                      &pInfoCopy->pszHostDnsDomain);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pszMachineAccount)
    {
        dwError = LwAllocateString(
                      pInfo->pszMachineAccount,
                      &pInfoCopy->pszMachineAccount);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pInfo->pszMachinePassword)
    {
        dwError = LwAllocateString(
                      pInfo->pszMachinePassword,
                      &pInfoCopy->pszMachinePassword);
        BAIL_ON_LW_ERROR(dwError);
    }

    pInfoCopy->last_change_time = pInfo->last_change_time;
    pInfoCopy->dwSchannelType = pInfo->dwSchannelType;

    *ppInfoCopy = pInfoCopy;
    pInfoCopy = NULL;

error:

    if (dwError)
    {
        *ppInfoCopy = NULL;
    }

    LwFreePasswordInfoA(pInfoCopy);

    return dwError;
}

VOID
LwFreePasswordInfoA(
    IN PLWPS_PASSWORD_INFO_A pInfo
    )
{
    if (pInfo)
    {
        LW_SAFE_FREE_STRING(pInfo->pszDomainName);
        LW_SAFE_FREE_STRING(pInfo->pszDnsDomainName);
        LW_SAFE_FREE_STRING(pInfo->pszSID);
        LW_SAFE_FREE_STRING(pInfo->pszHostname);
        LW_SAFE_FREE_STRING(pInfo->pszHostDnsDomain);
        LW_SAFE_FREE_STRING(pInfo->pszMachineAccount);
        LW_SECURE_FREE_STRING(pInfo->pszMachinePassword);
        LW_SAFE_FREE_MEMORY(pInfo);
    }

    return;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
