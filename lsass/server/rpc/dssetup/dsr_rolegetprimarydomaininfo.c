/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
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
 *        dsr_rolegetprimarydomaininfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        DsrRoleGetPrimaryDomainInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
DsrSrvRoleGetPDCInfoBasic(
    PDSR_ROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo,
    void *pParent
    );


static
NTSTATUS
DsrSrvRoleGetPDCInfoUpgrade(
    PDSR_ROLE_UPGRADE_STATUS pInfo
    );


static
NTSTATUS
DsrSrvRoleGetPDCInfoOpStatus(
    PDSR_ROLE_OP_STATUS pInfo
    );


WINERROR
DsrSrvRoleGetPrimaryDomainInformation(
    IN  handle_t          hBinding,
    IN  WORD              swLevel,
    OUT PDSR_ROLE_INFO   *ppInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = gpDsrSecDesc;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessMask = LSA_ACCESS_VIEW_POLICY_INFO;
    DWORD dwAccessGranted = 0;
    PACCESS_TOKEN pUserToken = NULL;
    PDSR_ROLE_INFO pInfo = NULL;

    /*
     * Get an access token and perform access check before
     * handling any info level
     */
    ntStatus = DsrSrvInitAuthInfo(hBinding, &pUserToken);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (pUserToken == NULL)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!RtlAccessCheck(pSecDesc,
                        pUserToken,
                        dwAccessMask,
                        dwAccessGranted,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = DsrSrvAllocateMemory(OUT_PPVOID(&pInfo),
                                    sizeof(*pInfo));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    switch (swLevel)
    {
    case DS_ROLE_BASIC_INFORMATION:
        ntStatus = DsrSrvRoleGetPDCInfoBasic(&pInfo->Basic,
                                             pInfo);
        break;

    case DS_ROLE_UPGRADE_STATUS:
        ntStatus = DsrSrvRoleGetPDCInfoUpgrade(&pInfo->Upgrade);
        break;

    case DS_ROLE_OP_STATUS:
        ntStatus = DsrSrvRoleGetPDCInfoOpStatus(&pInfo->OpStatus);
        break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppInfo = pInfo;

cleanup:
    if (pUserToken)
    {
        RtlReleaseAccessToken(&pUserToken);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return (WINERROR)dwError;

error:
    if (pInfo)
    {
        DsrSrvFreeMemory(pInfo);
    }

    *ppInfo = NULL;

    goto cleanup;
}


static
NTSTATUS
DsrSrvRoleGetPDCInfoBasic(
    PDSR_ROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo,
    void *pParent
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_VIEW_POLICY_INFO;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;
    PWSTR pwszProtSeq = NULL;
    PSTR pszLsaLpcSocketPath = NULL;
    PWSTR pwszLsaLpcSocketPath = NULL;
    PSTR pszDcName = NULL;
    PWSTR pwszDcName = NULL;
    PIO_CREDS pCreds = NULL;
    CHAR szHostname[64];
    LSA_BINDING hLsaBinding = NULL;
    POLICY_HANDLE hLocalPolicy = NULL;
    LsaPolicyInformation *pPolInfo = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszDnsDomain = NULL;
    PWSTR pwszForest = NULL;

    memset(szHostname, 0, sizeof(szHostname));

    dwError = LsaSrvProviderGetMachineAccountInfoA(
                    LSA_PROVIDER_TAG_AD,
                    NULL,
                    &pAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDomainController(pAccountInfo->DnsDomainName,
                                       &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LwIoGetThreadCreds(&pCreds);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DsrSrvConfigGetLsaLpcSocketPath(&pszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s("ncalrpc",
                           &pwszProtSeq);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pszLsaLpcSocketPath,
                           &pwszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaInitBindingFull(&hLsaBinding,
                                  pwszProtSeq,
                                  NULL,
                                  pwszLsaLpcSocketPath,
                                  NULL,
                                  NULL,
                                  NULL);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwMbsToWc16s(pszDcName, &pwszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaOpenPolicy2(hLsaBinding,
                              pwszDcName,
                              NULL,
                              dwPolicyAccessMask,
                              &hLocalPolicy);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaQueryInfoPolicy(hLsaBinding,
                                  hLocalPolicy,
                                  LSA_POLICY_INFO_DNS,
                                  &pPolInfo);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = DsrSrvGetFromUnicodeStringEx(&pwszDomain,
                                            &pPolInfo->dns.name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = DsrSrvGetFromUnicodeStringEx(&pwszDnsDomain,
                                            &pPolInfo->dns.dns_domain);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = DsrSrvGetFromUnicodeStringEx(&pwszForest,
                                            &pPolInfo->dns.dns_forest);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(&pInfo->DomainGuid, &pPolInfo->dns.domain_guid,
           sizeof(pInfo->DomainGuid));

    ntStatus = LsaClose(hLsaBinding, hLocalPolicy);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pInfo->dwRole        = DS_ROLE_MEMBER_SERVER;
    pInfo->dwFlags       = DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT;
    pInfo->pwszDomain    = pwszDomain;
    pInfo->pwszDnsDomain = pwszDnsDomain;
    pInfo->pwszForest    = pwszForest;

cleanup:
    if (pInfo)
    {
        LsaRpcFreeMemory(pPolInfo);
    }

    if (pAccountInfo)
    {
        LsaSrvFreeMachineAccountInfoA(pAccountInfo);
    }

    if (pszDcName)
    {
        LWNetFreeString(pszDcName);
    }

    LW_SAFE_FREE_MEMORY(pwszDcName);
    LW_SAFE_FREE_MEMORY(pszLsaLpcSocketPath);
    LW_SAFE_FREE_MEMORY(pwszLsaLpcSocketPath);
    LW_SAFE_FREE_MEMORY(pwszProtSeq);

    LsaFreeBinding(&hLsaBinding);

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
DsrSrvRoleGetPDCInfoUpgrade(
    PDSR_ROLE_UPGRADE_STATUS pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pInfo->swUpgradeStatus = DS_ROLE_NOT_UPGRADING;
    pInfo->dwPrevious      = DS_ROLE_PREVIOUS_UNKNOWN;

    return ntStatus;
}


static
NTSTATUS
DsrSrvRoleGetPDCInfoOpStatus(
    PDSR_ROLE_OP_STATUS pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pInfo->swStatus = DS_ROLE_OP_IDLE;

    return ntStatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
