/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 * Abstract:
 *
 * Authors:
 * 
 */
#include "includes.h"

VOID
UpFreeMachineInformationContentsA(
    PLWPS_PASSWORD_INFOA pInfo
    )
{
    if (pInfo)
    {
        LW_SAFE_FREE_STRING(pInfo->pszDomainName);
        LW_SAFE_FREE_STRING(pInfo->pszDnsDomainName);
        LW_SAFE_FREE_STRING(pInfo->pszSid);
        LW_SAFE_FREE_STRING(pInfo->pszHostname);
        LW_SAFE_FREE_STRING(pInfo->pszHostDnsDomain);
        LW_SAFE_FREE_STRING(pInfo->pszMachineAccount);
        LW_SAFE_FREE_STRING(pInfo->pszMachinePassword);
    }
}

DWORD
UpAllocateMachineInformationContentsW(
    PLWPS_PASSWORD_INFOA pInfo,
    PLWPS_PASSWORD_INFO pPasswordInfo
    )
{
    DWORD dwError = 0;

    PWSTR pwszMachineAccount = NULL;
    PWSTR pwszMachinePassword = NULL;
    PWSTR pwszSid = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszHostDnsDomain = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszDnsDomainName = NULL;

    dwError = LwMbsToWc16s(pInfo->pszMachineAccount, &pwszMachineAccount);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszMachinePassword,
                &pwszMachinePassword);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszSid,
                &pwszSid);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszDomainName,
                &pwszDomainName);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszDnsDomainName,
                &pwszDnsDomainName);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszHostname,
                &pwszHostname);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszHostDnsDomain,
                &pwszHostDnsDomain);
    BAIL_ON_UP_ERROR(dwError);

    pPasswordInfo->pwszDomainName = pwszDomainName;
    pPasswordInfo->pwszDnsDomainName = pwszDnsDomainName;
    pPasswordInfo->pwszSID = pwszSid;
    pPasswordInfo->pwszHostname = pwszHostname;
    pPasswordInfo->pwszHostDnsDomain = pwszHostDnsDomain;
    pPasswordInfo->pwszMachineAccount = pwszMachineAccount;
    pPasswordInfo->pwszMachinePassword = pwszMachinePassword;
    pPasswordInfo->last_change_time = pInfo->last_change_time;
    pPasswordInfo->dwSchannelType = pInfo->dwSchannelType;

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszMachineAccount);
    LW_SAFE_FREE_MEMORY(pwszMachinePassword);
    LW_SAFE_FREE_MEMORY(pwszSid);
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pwszHostname);
    LW_SAFE_FREE_MEMORY(pwszHostDnsDomain);
    goto cleanup;
}

VOID
UpFreeMachineInformationContentsW(
    PLWPS_PASSWORD_INFO pInfo
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
        LW_SAFE_FREE_MEMORY(pInfo->pwszMachinePassword);
    }
}

