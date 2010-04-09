/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        netr_wkstagetinfo.c
 *
 * Abstract:
 *
 *        Likewise Workstation Service (wkssvc) rpc server
 *
 *        NetrWkstaGetInfo server API
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *          Rafal Szczesniak <rafal@likewise.com>
 */

#include "includes.h"


WINERROR
NetrSrvWkstaGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ DWORD dwLevel,
    /* [out] */ PNETR_WKSTA_INFO pInfo
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                     LSA_ACCESS_VIEW_POLICY_INFO;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    PWKSTA_INFO_100 pInfo100 = NULL;
    CHAR szHostname[64] = {0};
    PSTR pszLsaLpcSocketPath = NULL;
    handle_t hLsaBinding = NULL;
    PWSTR pwszLocalHost = NULL;
    POLICY_HANDLE hLocalPolicy = NULL;
    LsaPolicyInformation *pPolInfo = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszDnsDomain = NULL;

    if (dwLevel != 100)
    {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = WkssSrvAllocateMemory(OUT_PPVOID(&pInfo100),
                                    sizeof(*pInfo100));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = WkssSrvConfigGetLsaLpcSocketPath(&pszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    rpcStatus = InitLsaBindingFull(&hLsaBinding,
                                   "ncalrpc",
                                   NULL,
                                   pszLsaLpcSocketPath,
                                   NULL,
                                   NULL,
                                   NULL);
    if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
        BAIL_ON_NT_STATUS(ntStatus);
    }

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
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = WkssSrvAllocateWC16StringFromUnicodeStringEx(
                                &pwszDnsDomain,
                                (PUNICODE_STRING)&pPolInfo->dns.dns_domain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = WkssSrvAllocateWC16StringFromUnicodeStringEx(
                                  &pwszHostname,
                                  (PUNICODE_STRING)&pPolInfo->dns.name);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo100->wksta100_domain        = pwszDnsDomain;
    pInfo100->wksta100_name          = pwszHostname;
    pInfo100->wksta100_version_major = 5;
    pInfo100->wksta100_version_minor = 1;
    pInfo100->wksta100_platform_id   = 500;

    pInfo->pInfo100 = pInfo100;

    pwszDnsDomain = NULL;
    pwszHostname  = NULL;

cleanup:
    if (hLsaBinding && hLocalPolicy)
    {
        LsaClose(hLsaBinding, hLocalPolicy);
    }

    if (pPolInfo)
    {
        LsaRpcFreeMemory(pPolInfo);
    }

    FreeLsaBinding(&hLsaBinding);

    LW_SAFE_FREE_MEMORY(pszLsaLpcSocketPath);
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
