/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        netr_wkstagetinfo.c
 *
 * Abstract:
 *
 *        BeyondTrust Workstation Service (wkssvc) rpc server
 *
 *        NetrWkstaGetInfo server API
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *          Rafal Szczesniak <rafal@likewise.com>
 */

#include "includes.h"


WINERROR
NetrSrvWkstaGetInfo(
    /* [in] */ handle_t          hBinding,
    /* [in] */ PWSTR             pwszServerName,
    /* [in] */ DWORD             dwLevel,
    /* [out] */ PNETR_WKSTA_INFO pInfo
    )
{
    const DWORD dwRequiredAccessRights = WKSSVC_ACCESS_GET_INFO_1;

    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                     LSA_ACCESS_VIEW_POLICY_INFO;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WKSS_SRV_CONTEXT SrvCtx = {0};
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = gpWkssSecDesc;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;
    PWKSTA_INFO_100 pInfo100 = NULL;
    CHAR szHostname[64] = {0};
    PWSTR pwszLpcProtSeq = NULL;
    PSTR pszLsaLpcSocketPath = NULL;
    PWSTR pwszLsaLpcSocketPath = NULL;
    LSA_BINDING hLsaBinding = NULL;
    PWSTR pwszLocalHost = NULL;
    POLICY_HANDLE hLocalPolicy = NULL;
    LsaPolicyInformation *pPolInfo = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszDomain = NULL;

    if (dwLevel != 100)
    {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = WkssSrvInitAuthInfo(hBinding,
                                  &SrvCtx);
    BAIL_ON_LSA_ERROR(dwError);

    if (!RtlAccessCheck(pSecDesc,
                        SrvCtx.pUserToken,
                        dwRequiredAccessRights,
                        0,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = WkssSrvAllocateMemory(OUT_PPVOID(&pInfo100),
                                    sizeof(*pInfo100));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s("ncalrpc", &pwszLpcProtSeq);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = WkssSrvConfigGetLsaLpcSocketPath(&pszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pszLsaLpcSocketPath, &pwszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaInitBindingFull(&hLsaBinding,
                                  pwszLpcProtSeq,
                                  NULL,
                                  pwszLsaLpcSocketPath,
                                  NULL,
                                  NULL,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwMbsToWc16s(szHostname, &pwszLocalHost);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaOpenPolicy2(hLsaBinding,
                              pwszLocalHost,
                              NULL,
                              dwPolicyAccessMask,
                              &hLocalPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaQueryInfoPolicy(hLsaBinding,
                                  hLocalPolicy,
                                  LSA_POLICY_INFO_DNS,
                                  &pPolInfo);
    if (ntStatus == STATUS_SUCCESS)
    {
        dwError = WkssSrvAllocateWC16StringFromUnicodeStringEx(
                                    &pwszDomain,
                                    &pPolInfo->dns.dns_domain);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (ntStatus == STATUS_INVALID_INFO_CLASS)
    {
        ntStatus = STATUS_SUCCESS;
        /*
         * Not joined to a domain, return our localhost as our name
         * and WORKGROUP for the dns domain.  This matches matches
         * what Windows XP does over the wire.
         */

        dwError = WkssSrvAllocateWC16StringFromCString(
                                    &pwszDomain,
                                    "WORKGROUP"
                                    );
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = WkssSrvAllocateWC16StringFromCString(
                                &pwszHostname,
                                szHostname);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo100->wksta100_domain        = pwszDomain;
    pInfo100->wksta100_name          = pwszHostname;
    pInfo100->wksta100_version_major = 5;
    pInfo100->wksta100_version_minor = 1;
    pInfo100->wksta100_platform_id   = 500;

    pInfo->pInfo100 = pInfo100;

    pwszDomain   = NULL;
    pwszHostname = NULL;

cleanup:
    if (hLsaBinding && hLocalPolicy)
    {
        LsaClose(hLsaBinding, hLocalPolicy);
    }

    if (pPolInfo)
    {
        LsaRpcFreeMemory(pPolInfo);
    }

    LsaFreeBinding(&hLsaBinding);

    WkssSrvFreeAuthInfo(&SrvCtx);

    LW_SAFE_FREE_MEMORY(pszLsaLpcSocketPath);
    LW_SAFE_FREE_MEMORY(pwszLsaLpcSocketPath);
    LW_SAFE_FREE_MEMORY(pwszLpcProtSeq);
    LW_SAFE_FREE_MEMORY(pwszLocalHost);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    if (pInfo100)
    {
        if (pInfo100->wksta100_domain)
        {
            WkssSrvFreeMemory(pInfo100->wksta100_domain);
        }

        if (pInfo100->wksta100_name)
        {
            WkssSrvFreeMemory(pInfo100->wksta100_name);
        }

        WkssSrvFreeMemory(pInfo100);
    }

    memset(pInfo, 0, sizeof(*pInfo));

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
