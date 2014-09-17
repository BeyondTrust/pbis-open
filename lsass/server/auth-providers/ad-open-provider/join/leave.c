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
    IN PCWSTR pwszDCName,
    IN LW_PIO_CREDS pCreds,
    IN PCWSTR pwszMachineAccountName
    );

static
DWORD
LsaUnjoinDomain(
    IN PCWSTR pwszDnsDomainName,
    IN PCWSTR pwszMachineSamAccountName,
    IN OPTIONAL PCWSTR pwszUserName,
    IN OPTIONAL PCWSTR pwszUserDomain,
    IN OPTIONAL PCWSTR pwszUserPassword,
    IN DWORD dwUnjoinFlags
    );

DWORD
LsaLeaveDomain2(
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszUsername,
    IN OPTIONAL PCSTR pszPassword,
    IN LSA_NET_JOIN_FLAGS dwFlags
    )
{
    DWORD dwError = 0;
    PWSTR pwszDnsDomainName = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;
    PLSA_CREDS_FREE_INFO pAccessInfo = NULL;

    if (pszDnsDomainName)
    {
        dwError = LwMbsToWc16s(pszDnsDomainName, &pwszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaPstoreGetPasswordInfoW(pwszDnsDomainName, &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszUsername) &&
        !LW_IS_NULL_OR_EMPTY_STR(pszPassword))
    {
        // TODO-2011/01/13-dalmeida -- Ensure we use UPN.
        // Otherwise, whether this works depends on krb5.conf.
        // Can cons up UPN if needed since we have domain.
        dwError = LsaSetSMBCreds(
                    pszUsername,
                    pszPassword,
                    TRUE,
                    &pAccessInfo);
        BAIL_ON_LSA_ERROR(dwError);

        // TODO-2010/01/10-dalmeida -- We should try to leave the
        // domain w/machine creds too...  Note that we do not
        // pass in the usernmame/password below.  There is inconsistent
        // handling of the passed in leave credentials.
        // Also, the flag does not actually try to delete the account...

        dwError = LsaUnjoinDomain(
                    pPasswordInfo->Account.DnsDomainName,
                    pPasswordInfo->Account.SamAccountName,
                    NULL,
                    NULL,
                    NULL,
                    LSAJOIN_ACCT_DELETE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaPstoreDeleteTrustEnumerationWaitInfo(pwszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPstoreDeletePasswordInfoW(pwszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

error:
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LSA_PSTORE_FREE_PASSWORD_INFO_W(&pPasswordInfo);
    LsaFreeSMBCreds(&pAccessInfo);

    return dwError;
}


static
DWORD
LsaUnjoinDomain(
    IN PCWSTR pwszDnsDomainName,
    IN PCWSTR pwszMachineSamAccountName,
    IN OPTIONAL PCWSTR pwszUserName,
    IN OPTIONAL PCWSTR pwszUserDomain,
    IN OPTIONAL PCWSTR pwszUserPassword,
    IN DWORD dwUnjoinFlags
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszDCName = NULL;
    PIO_CREDS pCreds = NULL;

    dwError = LsaGetRwDcName(pwszDnsDomainName,
                             FALSE,
                             &pwszDCName);
    BAIL_ON_LSA_ERROR(dwError);

    /* disable the account only if requested */
    if (dwUnjoinFlags & LSAJOIN_ACCT_DELETE)
    {
        if (pwszUserName && pwszUserPassword)
        {
            ntStatus = LwIoCreatePlainCredsW(pwszUserName,
                                             pwszUserDomain,
                                             pwszUserPassword,
                                             &pCreds);
            dwError = LwNtStatusToWin32Error(ntStatus);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            ntStatus = LwIoGetActiveCreds(NULL,
                                          &pCreds);
            dwError = LwNtStatusToWin32Error(ntStatus);
            BAIL_ON_LSA_ERROR(dwError);
        }

        ntStatus = LsaDisableMachineAccount(pwszDCName,
                                            pCreds,
                                            pwszMachineSamAccountName);
        dwError = LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_LSA_ERROR(dwError);
    }

error:
    LSA_ASSERT(!ntStatus || dwError);

    LW_SAFE_FREE_MEMORY(pwszDCName);

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    return dwError;
}


static
NTSTATUS
LsaDisableMachineAccount(
    IN PCWSTR pwszDCName,
    IN LW_PIO_CREDS pCreds,
    IN PCWSTR pwszMachineAccountName
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

    ntStatus = RtlAllocateWellKnownSid(
                    WinBuiltinDomainSid,
                    NULL,
                    &pBuiltinSid);
    BAIL_ON_NT_STATUS(ntStatus);

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

            SamrFreeMemory(pSid);
            pSid = NULL;
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
                               (PWSTR*)&pwszMachineAccountName,
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

    RTL_FREE(&pBuiltinSid);
    RTL_FREE(&pDomainSid);

    return ntStatus;

error:
    goto cleanup;
}


DWORD
LsaDisableDomainGroupMembership(
    PCSTR pszDomainName,
    PCSTR pszDomainSID
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = LsaChangeDomainGroupMembership(pszDomainName,
                                             pszDomainSID,
                                             FALSE);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
