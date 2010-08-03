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


static
NTSTATUS
LsaDisableMachineAccount(
    PWSTR          pwszDCName,
    LW_PIO_CREDS   pCreds,
    PWSTR          pwszMachineAccountName,
    DWORD          dwUnjoinFlags
    );


static
DWORD
LsaUnjoinDomain(
    PCWSTR  pwszMachineName, 
    PCWSTR  pwszDomainName,
    PCWSTR  pwszAccountName,
    PCWSTR  pwszPassword,
    DWORD   dwUnjoinFlags
    );


DWORD
LsaLeaveDomain(
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    HANDLE hStore = (HANDLE)NULL;
    PSTR  pszHostname = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszDnsDomainName = NULL;
    DWORD dwOptions = LSAJOIN_ACCT_DELETE;
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
        dwError = LwMbsToWc16s(
                    pAcct->pszDnsDomainName,
                    &pwszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwMbsToWc16s(
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
                    &pAccessInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaUnjoinDomain(
                    pwszHostname,
                    pwszDnsDomainName,
                    NULL,
                    NULL,
                    dwOptions);
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


static
DWORD
LsaUnjoinDomain(
    PCWSTR  pwszMachineName, 
    PCWSTR  pwszDomainName,
    PCWSTR  pwszAccountName,
    PCWSTR  pwszPassword,
    DWORD   dwUnjoinFlags
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PSTR pszLocalname = NULL;
    PWSTR pwszDCName = NULL;
    PWSTR pwszMachine = NULL;   
    PIO_CREDS pCreds = NULL;
    size_t sMachinePasswordLen = 0;

    BAIL_ON_INVALID_POINTER(pwszMachineName);
    BAIL_ON_INVALID_POINTER(pwszDomainName);

    dwError = LwAllocateWc16String(&pwszMachine,
                                   pwszMachineName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetHostInfo(&pszLocalname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetRwDcName(pwszDomainName,
                             FALSE,
                             &pwszDCName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT,
                                     &hStore);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwpsGetPasswordByHostName(hStore,
                                         pszLocalname,
                                         &pPassInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    /* disable the account only if requested */
    if (dwUnjoinFlags & LSAJOIN_ACCT_DELETE)
    {
        if (pwszAccountName && pwszPassword)
        {
            ntStatus = LwIoCreatePlainCredsW(pwszAccountName,
                                             pwszDomainName,
                                             pwszPassword,
                                             &pCreds);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ntStatus = LwIoGetActiveCreds(NULL,
                                          &pCreds);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = LsaDisableMachineAccount(pwszDCName,
                                            pCreds,
                                            pPassInfo->pwszMachineAccount,
                                            dwUnjoinFlags);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = LwWc16sLen(pPassInfo->pwszMachinePassword,
                         &sMachinePasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* zero the machine password */
    memset(pPassInfo->pwszMachinePassword,
           0,
           sMachinePasswordLen);

    pPassInfo->last_change_time = time(NULL);

    ntStatus = LwpsWritePasswordToAllStores(pPassInfo);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    LW_SAFE_FREE_MEMORY(pszLocalname);
    LW_SAFE_FREE_MEMORY(pwszDCName);
    LW_SAFE_FREE_MEMORY(pwszMachine);

    if (pPassInfo)
    {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }

    if (hStore != (HANDLE)NULL)
    {
        LwpsClosePasswordStore(hStore);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
LsaGetHostInfo(
    PSTR* ppszHostname
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CHAR szBuffer[256] = {0};
    PSTR pszLocal = NULL;
    PSTR pszDot = NULL;
    size_t len = 0;
    PSTR pszHostname = NULL;

    *ppszHostname = NULL;

    if (gethostname(szBuffer, sizeof(szBuffer)) != 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    len = strlen(szBuffer);
    if (len > strlen(".local"))
    {
        pszLocal = &szBuffer[len - strlen(".local")];
        if (!strcasecmp(pszLocal, ".local"))
        {
            pszLocal[0] = '\0';
        }
    }
    
    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    pszDot = strchr(szBuffer, '.');
    if ( pszDot )
    {
        pszDot[0] = '\0';
    }

    len = strlen(szBuffer) + 1;
    ntStatus = LwAllocateMemory(len,
                                OUT_PPVOID(&pszHostname));
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy((void *)pszHostname, szBuffer, len);

    if (ppszHostname)
    {
        *ppszHostname = pszHostname;
        pszHostname = NULL;
    }
        
cleanup:
    LW_SAFE_FREE_MEMORY(pszHostname);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


static
NTSTATUS
LsaDisableMachineAccount(
    PWSTR          pwszDCName,
    LW_PIO_CREDS   pCreds,
    PWSTR          pwszMachineAccountName,
    DWORD          dwUnjoinFlags
    )
{
    const DWORD dwConnAccess = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;

    const DWORD dwDomainAccess = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2;

    const DWORD dwUserAccess = USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    SAMR_BINDING hSamrBinding = NULL;
    CONNECT_HANDLE hConnect = NULL;
    PSID pBuiltinSid = NULL;
    DWORD dwResume = 0;
    DWORD dwSize = 256;
    PWSTR *ppwszDomainNames = NULL;
    DWORD i = 0;
    DWORD dwNumEntries = 0;
    PSID pSid = NULL;
    PSID pDomainSid = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    PDWORD pdwRids = NULL;
    PDWORD pdwTypes = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwLevel = 0;
    UserInfo *pInfo = NULL;
    DWORD dwFlagsDisable = 0;
    UserInfo Info;

    memset(&Info, 0, sizeof(Info));

    ntStatus = SamrInitBindingDefault(&hSamrBinding,
                                      pwszDCName,
                                      pCreds);
    BAIL_ON_NT_STATUS(ntStatus);
    
    ntStatus = SamrConnect2(hSamrBinding,
                            pwszDCName,
                            dwConnAccess,
                            &hConnect);
    BAIL_ON_NT_STATUS(ntStatus);    

    dwError = LwCreateWellKnownSid(WinBuiltinDomainSid,
                                   NULL,
                                   &pBuiltinSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        ntStatus = SamrEnumDomains(hSamrBinding,
                                   hConnect,
                                   &dwResume,
                                   dwSize,
                                   &ppwszDomainNames,
                                   &dwNumEntries);
        BAIL_ON_NT_STATUS(ntStatus);

        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        for (i = 0; pDomainSid == NULL && i < dwNumEntries; i++)
        {
            ntStatus = SamrLookupDomain(hSamrBinding,
                                        hConnect,
                                        ppwszDomainNames[i],
                                        &pSid);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!RtlEqualSid(pSid, pBuiltinSid))
            {
                ntStatus = RtlDuplicateSid(&pDomainSid, pSid);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (pSid)
            {
                SamrFreeMemory(pSid);
                pSid = NULL;
            }
        }

        if (ppwszDomainNames)
        {
            SamrFreeMemory(ppwszDomainNames);
            ppwszDomainNames = NULL;
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    ntStatus = SamrOpenDomain(hSamrBinding,
                              hConnect,
                              dwDomainAccess,
                              pDomainSid,
                              &hDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrLookupNames(hSamrBinding,
                               hDomain,
                               1,
                               &pwszMachineAccountName,
                               &pdwRids,
                               &pdwTypes,
                               NULL);
    if (ntStatus == STATUS_NONE_MAPPED)
    {
        BAIL_ON_LSA_ERROR(NERR_SetupAlreadyJoined);
    }

    ntStatus = SamrOpenUser(hSamrBinding,
                            hDomain,
                            dwUserAccess,
                            pdwRids[0],
                            &hUser);
    BAIL_ON_NT_STATUS(ntStatus);

    dwLevel = 16;

    ntStatus = SamrQueryUserInfo(hSamrBinding,
                                 hUser,
                                 dwLevel,
                                 &pInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    dwFlagsDisable = pInfo->info16.account_flags | ACB_DISABLED;

    Info.info16.account_flags = dwFlagsDisable;
    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &Info);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hSamrBinding && hUser)
    {
        SamrClose(hSamrBinding, hUser);
    }

    if (hSamrBinding && hDomain)
    {
        SamrClose(hSamrBinding, hDomain);
    }

    if (hSamrBinding && hConnect)
    {
        SamrClose(hSamrBinding, hConnect);
    }

    if (hSamrBinding)
    {
        SamrFreeBinding(&hSamrBinding);
    }

    if (pInfo)
    {
        SamrFreeMemory(pInfo);
    }

    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
    }

    if (pdwTypes)
    {
        SamrFreeMemory(pdwTypes);
    }

    if (ppwszDomainNames)
    {
        SamrFreeMemory(ppwszDomainNames);
    }

    LW_SAFE_FREE_MEMORY(pBuiltinSid);
    RTL_FREE(&pDomainSid);

    return ntStatus;

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
