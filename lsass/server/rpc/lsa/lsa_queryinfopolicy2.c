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
 *        lsa_queryinfopolicy2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaQueryInfoPolicy2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
LsaQueryDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    LsaDomainInfo *pInfo
    );


static
NTSTATUS
LsaQueryAccountDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    LsaDomainInfo *pInfo
    );


static
NTSTATUS
LsaQueryDnsDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    DnsDomainInfo *pInfo
    );


NTSTATUS
LsaSrvQueryInfoPolicy2(
    handle_t hBinding,
    POLICY_HANDLE hPolicy,
    UINT16 level,
    LsaPolicyInformation **ppInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPOLICY_CONTEXT pPolCtx = NULL;
    LsaPolicyInformation *pInfo = NULL;

    pPolCtx = (PPOLICY_CONTEXT)hPolicy;

    if (pPolCtx == NULL || pPolCtx->Type != LsaContextPolicy)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pPolCtx->dwAccessGranted & LSA_ACCESS_VIEW_POLICY_INFO))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = LsaSrvAllocateMemory((void**)&pInfo,
                                    sizeof(*pInfo));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    switch (level) {
    case LSA_POLICY_INFO_AUDIT_LOG:
    case LSA_POLICY_INFO_AUDIT_EVENTS:
        ntStatus = STATUS_INVALID_PARAMETER;
        break;

    case LSA_POLICY_INFO_DOMAIN:
        ntStatus = LsaQueryDomainInfo(hBinding, pPolCtx, &pInfo->domain);
        break;

    case LSA_POLICY_INFO_ACCOUNT_DOMAIN:
        ntStatus = LsaQueryAccountDomainInfo(hBinding, pPolCtx, &pInfo->domain);
        break;

    case LSA_POLICY_INFO_PD:
    case LSA_POLICY_INFO_ROLE:
    case LSA_POLICY_INFO_REPLICA:
    case LSA_POLICY_INFO_QUOTA:
    case LSA_POLICY_INFO_DB:
    case LSA_POLICY_INFO_AUDIT_FULL_SET:
    case LSA_POLICY_INFO_AUDIT_FULL_QUERY:
        ntStatus = STATUS_INVALID_PARAMETER;
        break;

    case LSA_POLICY_INFO_DNS:
        ntStatus = LsaQueryDnsDomainInfo(hBinding, pPolCtx, &pInfo->dns);
        break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppInfo = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo)
    {
        LsaSrvFreeMemory(pInfo);
    }

    *ppInfo = NULL;
    goto cleanup;
}


static
NTSTATUS
LsaQueryDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    LsaDomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = NULL;

    dwError = LsaSrvProviderGetMachineAccountInfoW(
                  LSA_PROVIDER_TAG_AD,
                  NULL,
                  &pAccountInfo);
    if (dwError == NERR_SetupNotJoined)
    {
        /* No password info means we're not joined */
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pAccountInfo)
    {
        ntStatus = LsaSrvInitUnicodeStringEx(&pInfo->name,
                                             pAccountInfo->NetbiosDomainName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus); 

        ntStatus = LsaSrvAllocateSidFromWC16String(&pInfo->sid,
                                                   pAccountInfo->DomainSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

cleanup:
    LsaSrvFreeMachineAccountInfoW(pAccountInfo);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaQueryAccountDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    LsaDomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LsaSrvInitUnicodeStringEx(&pInfo->name,
                                         pPolCtx->pwszLocalDomainName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvDuplicateSid(&pInfo->sid,
                                  pPolCtx->pLocalDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaQueryDnsDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    DnsDomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = NULL;
    PWSTR pwszDnsForest = NULL;
    PSTR pszDomainFqdn = NULL;
    PSTR pszDcFqdn = NULL;
    PSTR pszSiteName = NULL;
    DWORD dwFlags = 0;
    PLWNET_DC_INFO pDcInfo = NULL;


    dwError = LsaSrvProviderGetMachineAccountInfoW(
                  LSA_PROVIDER_TAG_AD,
                  NULL,
                  &pAccountInfo);
    if (dwError == NERR_SetupNotJoined)
    {
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pAccountInfo)
    {
        ntStatus = LsaSrvInitUnicodeStringEx(&pInfo->name,
                                             pAccountInfo->NetbiosDomainName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvInitUnicodeStringEx(&pInfo->dns_domain,
                                             pAccountInfo->DnsDomainName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvAllocateSidFromWC16String(&pInfo->sid,
                                                   pAccountInfo->DomainSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwWc16sToMbs(pAccountInfo->DnsDomainName,
                               &pszDomainFqdn);
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = LWNetGetDCName(pszDcFqdn,
                                 pszDomainFqdn,
                                 pszSiteName,
                                 dwFlags,
                                 &pDcInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMbsToWc16s(pDcInfo->pszDnsForestName,
                               &pwszDnsForest);

        ntStatus = LsaSrvInitUnicodeStringEx(&pInfo->dns_forest,
                                             pwszDnsForest);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        memcpy(&pInfo->domain_guid, pDcInfo->pucDomainGUID,
               sizeof(pInfo->domain_guid));
    }

cleanup:
    LsaSrvFreeMachineAccountInfoW(pAccountInfo);

    if (pDcInfo)
    {
        LWNetFreeDCInfo(pDcInfo);
    }

    LW_SAFE_FREE_MEMORY(pszDomainFqdn);
    LW_SAFE_FREE_MEMORY(pwszDnsForest);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

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
