/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        leave.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Leave from Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LsaNetLeaveDomain(
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    HANDLE hStore = (HANDLE)NULL;
    PSTR  pszHostname = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszDnsDomainName = NULL;
    DWORD dwOptions = (NETSETUP_ACCT_DELETE);
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PLSA_MACHINE_ACCT_INFO pAcct = NULL;
    PLSA_CREDS_FREE_INFO pAccessInfo = NULL;
    
    if (geteuid() != 0) {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsGetPasswordByHostName(
                hStore,
                pszHostname,
                &pPassInfo);
    if (dwError)
    {
        if (dwError == LWPS_ERROR_INVALID_ACCOUNT)
        {
            dwError = LW_ERROR_NOT_JOINED_TO_AD;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaBuildMachineAccountInfo(
                    pPassInfo,
                    &pAcct);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pAcct->pszDnsDomainName))
    {
        dwError = LsaMbsToWc16s(
                    pAcct->pszDnsDomainName,
                    &pwszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaMbsToWc16s(
                    pszHostname,
                    &pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pszUsername) &&
        !LW_IS_NULL_OR_EMPTY_STR(pszPassword))
    {
        dwError = LsaSetSMBCreds(
                    pAcct->pszDnsDomainName,
                    pszUsername,
                    pszPassword,
                    TRUE,
                    &pAccessInfo,
                    NULL);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = Win32ErrorToErrno(
            NetUnjoinDomainLocal(
                pwszHostname,
                pwszDnsDomainName,
                NULL,
                NULL,
                dwOptions));
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwpsDeleteEntriesInAllStores();
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pPassInfo)
    {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }

    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }
    
    if (pAcct)
    {
        LsaFreeMachineAccountInfo(pAcct);
    }

    LW_SAFE_FREE_STRING(pszHostname);

    LW_SAFE_FREE_MEMORY(pwszHostname);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LsaFreeSMBCreds(&pAccessInfo);

    return dwError;
    
error:

    goto cleanup;
}


DWORD
LsaDisableDomainGroupMembership(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszHostname = NULL;
    HANDLE hStore = NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PSTR pszDnsDomainName = NULL;

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsGetPasswordByHostName(
                hStore,
                pszHostname,
                &pPassInfo);
    if (dwError)
    {
        if (dwError == LWPS_ERROR_INVALID_ACCOUNT)
        {
            dwError = LW_ERROR_NOT_JOINED_TO_AD;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(pPassInfo->pwszDnsDomainName,
                           &pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangeDomainGroupMembership(pszDnsDomainName,
                                             FALSE);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (hStore && pPassInfo)
    {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }

    if (hStore)
    {
        LwpsClosePasswordStore(hStore);
    }

    LW_SAFE_FREE_MEMORY(pszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pszHostname);

    return dwError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
