/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        nfssvc_netservergetinfo.c
 *
 * Abstract:
 *
 *        Likewise Server Service (nfssvc) RPC client and server
 *
 *        NfsSvcNetServerGetInfo server API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NfsSvcNetrServerGetInfo(
    /* [in] */ handle_t b,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ UINT32 level,
    /* [out] */ nfssvc_NetNfsInfo *info
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                     LSA_ACCESS_VIEW_POLICY_INFO;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    RPCSTATUS rpcStatus = RPC_S_OK;
    PSTR pszLsaLpcSocketPath = NULL;
    handle_t hLsaBinding = NULL;
    CHAR szHostname[64] = {0};
    PWSTR pwszLocalHost = NULL;
    POLICY_HANDLE hLocalPolicy = NULL;
    LsaPolicyInformation *pPolInfo = NULL;
    PCSTR pszComment = "Likewise RPC";
    PCSTR pszUserPath = "/testpath";
    PWSTR pwszHostname = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszUserPath = NULL;
    SERVER_INFO_101 *pInfo101 = NULL;
    SERVER_INFO_102 *pInfo102 = NULL;

    dwError = NfsSvcConfigGetLsaLpcSocketPath(&pszLsaLpcSocketPath);
    BAIL_ON_NFSSVC_ERROR(dwError);

    rpcStatus = InitLsaBindingFull(&hLsaBinding,
                                   "ncalrpc",
                                   NULL,
                                   pszLsaLpcSocketPath,
                                   NULL,
                                   NULL,
                                   NULL);
    if (rpcStatus)
    {
        dwError = NERR_InternalError;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwMbsToWc16s(szHostname, &pwszLocalHost);
    BAIL_ON_NFSSVC_ERROR(dwError);

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

    if (level == 101)
    {
        dwError = NfsSvcNfsAllocateMemory(sizeof(*pInfo101),
                                          OUT_PPVOID(&pInfo101));
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwError = NfsSvcNfsAllocateWC16StringFromUnicodeStringEx(
                                              &pwszHostname,
                                              &pPolInfo->dns.name);
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwError = NfsSvcNfsAllocateWC16StringFromCString(
                                              &pwszComment,
                                              pszComment);
        BAIL_ON_NFSSVC_ERROR(dwError);

        pInfo101->sv101_platform_id    = 500;
        pInfo101->sv101_name           = pwszHostname;
        pInfo101->sv101_version_major  = 5;
        pInfo101->sv101_version_minor  = 1;
        pInfo101->sv101_type           = 0x0001003;
        pInfo101->sv101_comment        = pwszComment;

        info->info101 = pInfo101;

        pwszHostname = NULL;
        pwszComment  = NULL;
    }
    else if (level == 102)
    {
        dwError = NfsSvcNfsAllocateMemory(sizeof(*pInfo102),
                                          OUT_PPVOID(&pInfo102));
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwError = NfsSvcNfsAllocateMemory(sizeof(*pInfo101),
                                          OUT_PPVOID(&pInfo101));
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwError = NfsSvcNfsAllocateWC16StringFromUnicodeStringEx(
                                              &pwszHostname,
                                              &pPolInfo->dns.name);
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwError = NfsSvcNfsAllocateWC16StringFromCString(
                                              &pwszComment,
                                              pszComment);
        BAIL_ON_NFSSVC_ERROR(dwError);

        pInfo102->sv102_platform_id    = 500;
        pInfo102->sv102_name           = pwszHostname;
        pInfo102->sv102_version_major  = 5;
        pInfo102->sv102_version_minor  = 1;
        pInfo102->sv102_type           = 0x0001003;
        pInfo102->sv102_comment        = pwszComment;
        pInfo102->sv102_users          = 0;
        pInfo102->sv102_disc           = 0;
        pInfo102->sv102_hidden         = 0;
        pInfo102->sv102_announce       = 0;
        pInfo102->sv102_anndelta       = 0;
        pInfo102->sv102_licenses       = 5;
        pInfo102->sv102_userpath       = pwszUserPath;

        info->info102 = pInfo102;

        pwszHostname = NULL;
        pwszComment  = NULL;
        pwszUserPath  = NULL;
    }
    else
    {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

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
    if (pInfo101)
    {
        if (pInfo101->sv101_name)
        {
            NfsSvcNfsFreeMemory(pInfo101->sv101_name);
        }

        if (pInfo101->sv101_comment)
        {
            NfsSvcNfsFreeMemory(pInfo101->sv101_comment);
        }

        NfsSvcNfsFreeMemory(pInfo101);
    }
    else if (pInfo102)
    {
        if (pInfo102->sv102_name)
        {
            NfsSvcNfsFreeMemory(pInfo102->sv102_name);
        }

        if (pInfo102->sv102_comment)
        {
            NfsSvcNfsFreeMemory(pInfo102->sv102_comment);
        }

        if (pInfo102->sv102_userpath)
        {
            NfsSvcNfsFreeMemory(pInfo102->sv102_userpath);
        }

        NfsSvcNfsFreeMemory(pInfo102);
    }

    memset(info, 0, sizeof(*info));

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
