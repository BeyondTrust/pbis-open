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
 *        lsa_openpolicy2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaOpenPolicy2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvOpenPolicy2(
    IN  handle_t          hBinding,
    IN  PWSTR             pwszSysName,
    IN  ObjectAttribute  *pAttrib,
    IN  DWORD             dwAccessMask,
    OUT POLICY_HANDLE    *phPolicy
    )
{
    const DWORD dwSamrConnAccess     = SAMR_ACCESS_CONNECT_TO_SERVER |
                                       SAMR_ACCESS_ENUM_DOMAINS |
                                       SAMR_ACCESS_OPEN_DOMAIN;
    const DWORD dwSamrDomainAccess   = DOMAIN_ACCESS_LOOKUP_INFO_1 |
                                       DOMAIN_ACCESS_LOOKUP_ALIAS |
                                       DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                       DOMAIN_ACCESS_OPEN_ACCOUNT;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    RPCSTATUS rpcStatus = 0;
    PPOLICY_CONTEXT pPolCtx = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = gpLsaSecDesc;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;
    PSTR pszSamrLpcSocketPath = NULL;
    handle_t hSamrBinding = NULL;
    CHAR szHostname[64] = {0};
    PWSTR pwszSystemName = NULL;
    CONNECT_HANDLE hConn = (CONNECT_HANDLE)NULL;
    DWORD dwResume = 0;
    DWORD dwMaxSize = -1;
    DWORD dwCount = 0;
    PWSTR *ppwszDomainNames = NULL;
    DWORD i = 0;
    PWSTR pwszDomainName = NULL;
    DWORD dwSidLength = 0;
    PSID pDomainSid = NULL;
    DOMAIN_HANDLE hDomain = (DOMAIN_HANDLE)NULL;
    PSTR pszDomainFqdn = NULL;
    PSTR pszDcFqdn = NULL;
    HANDLE hPassStore = NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pPolCtx),
                               OUT_PPVOID(&pPolCtx));
    BAIL_ON_LSA_ERROR(dwError);

    pPolCtx->Type        = LsaContextPolicy;
    pPolCtx->refcount    = 1;

    ntStatus = LsaSrvInitAuthInfo(hBinding, pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (!RtlAccessCheck(pSecDesc,
                        pPolCtx->pUserToken,
                        dwAccessMask,
                        pPolCtx->dwAccessGranted,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pPolCtx->dwAccessGranted = dwAccessGranted;

    /*
     * Connect samr rpc server and get basic domain info
     */

    dwError = LsaSrvConfigGetSamrLpcSocketPath(&pszSamrLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);
    
    rpcStatus = InitSamrBindingFull(&hSamrBinding,
                                    "ncalrpc",
                                    szHostname,
                                    pszSamrLpcSocketPath,
                                    NULL,
                                    NULL,
                                    NULL);
    if (rpcStatus) {
        dwError = LW_ERROR_RPC_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pPolCtx->hSamrBinding = hSamrBinding;

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s((PSTR)szHostname, &pwszSystemName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrConnect2(hSamrBinding,
                            pwszSystemName,
                            dwSamrConnAccess,
                            &hConn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pPolCtx->hConn = hConn;

    /* Get local and builtin domains info */
    ntStatus = SamrEnumDomains(hSamrBinding,
                               hConn,
                               &dwResume,
                               dwMaxSize,
                               &ppwszDomainNames,
                               &dwCount);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < dwCount; i++)
    {
        pwszDomainName = ppwszDomainNames[i];

        ntStatus = SamrLookupDomain(hSamrBinding,
                                    hConn,
                                    pwszDomainName,
                                    &pDomainSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = SamrOpenDomain(hSamrBinding,
                                  hConn,
                                  dwSamrDomainAccess,
                                  pDomainSid,
                                  &hDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        if (pDomainSid &&
            pDomainSid->SubAuthorityCount == 1)
        {
            pPolCtx->hBuiltinDomain = hDomain;
        }
        else if (pDomainSid)
        {
            pPolCtx->hLocalDomain = hDomain;

            dwError = LwAllocateWc16String(&pPolCtx->pwszLocalDomainName,
                                           pwszDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            dwSidLength = RtlLengthSid(pDomainSid);
            dwError = LwAllocateMemory(dwSidLength,
                                       OUT_PPVOID(&pPolCtx->pLocalDomainSid));
            BAIL_ON_LSA_ERROR(dwError);

            ntStatus = RtlCopySid(dwSidLength,
                                  pPolCtx->pLocalDomainSid,
                                  pDomainSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        if (pDomainSid)
        {
            SamrFreeMemory(pDomainSid);
            pDomainSid = NULL;
        }
    }

    dwError = LWNetGetCurrentDomain(&pszDomainFqdn);
    if (dwError == ERROR_SUCCESS)
    {
        /*
         * LSASS is a domain member so get some basic information
         * about the domain
         */
        dwError = LWNetGetDomainController(pszDomainFqdn,
                                           &pszDcFqdn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMbsToWc16s(pszDcFqdn,
                               &pPolCtx->pwszDcName);
        BAIL_ON_LSA_ERROR(dwError)

            dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT,
                                            &hPassStore);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwpsGetPasswordByCurrentHostName(
                                            hPassStore,
                                            &pPassInfo);
        BAIL_ON_LSA_ERROR(dwError);

        if (pPassInfo)
        {
            dwError = LwAllocateWc16String(&pPolCtx->pwszDomainName,
                                           pPassInfo->pwszDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            ntStatus = RtlAllocateSidFromWC16String(&pPolCtx->pDomainSid,
                                                    pPassInfo->pwszSID);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
    }
    else if (dwError == ERROR_NOT_JOINED)
    {
        /*
         * LSASS is a standalone server
         */
        dwError = ERROR_SUCCESS;

        pPolCtx->pwszDcName     = NULL;
        pPolCtx->pwszDomainName = NULL;
        pPolCtx->pDomainSid     = NULL;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    ntStatus = LsaSrvCreateDomainsTable(&pPolCtx->pDomains);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Increase ref count because DCE/RPC runtime is about to use this
       pointer as well */
    InterlockedIncrement(&pPolCtx->refcount);

    *phPolicy = (POLICY_HANDLE)pPolCtx;

cleanup:
    if (pDomainSid)
    {
        SamrFreeMemory(pDomainSid);
    }

    if (ppwszDomainNames)
    {
        SamrFreeMemory(ppwszDomainNames);
    }

    if (pszDcFqdn)
    {
        LWNetFreeString(pszDcFqdn);
    }

    if (pszDomainFqdn)
    {
        LWNetFreeString(pszDomainFqdn);
    }

    if (hPassStore && pPassInfo)
    {
        LwpsFreePasswordInfo(hPassStore,
                             pPassInfo);
    }

    if (hPassStore)
    {
        LwpsClosePasswordStore(hPassStore);
    }

    LW_SAFE_FREE_MEMORY(pwszSystemName);
    LW_SAFE_FREE_MEMORY(pszSamrLpcSocketPath);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pPolCtx)
    {
        POLICY_HANDLE_rundown((POLICY_HANDLE)pPolCtx);
    }

    *phPolicy = NULL;
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
