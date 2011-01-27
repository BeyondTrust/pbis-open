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
 *        netr_joindomain2.c
 *
 * Abstract:
 *
 *        Likewise Workstation Service (wkssvc) rpc server
 *
 *        NetrJoinDomain2 server API
 *
 * Authors: Rafal Szczesniak <rafal@likewise.com>
 */

#include "includes.h"


WINERROR
NetrSrvJoinDomain2(
    /* [in] */ handle_t                  hBinding,
    /* [in] */ PWSTR                     pwszServerName,
    /* [in] */ PWSTR                     pwszDomainName,
    /* [in] */ PWSTR                     pwszAccountOu,
    /* [in] */ PWSTR                     pwszAccountName,
    /* [in] */ PENC_JOIN_PASSWORD_BUFFER pPassword,
    /* [in] */ DWORD                     dwJoinFlags
    )
{
    const DWORD dwRequiredAccessRights = WKSSVC_ACCESS_JOIN_DOMAIN;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WKSS_SRV_CONTEXT SrvCtx = {0};
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = gpWkssSecDesc;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;
    PWSTR pwszPassword = NULL;
    size_t sPasswordLen = 0;
    PSTR szHostname[255] = {0};
    PSTR pszDomain = NULL;
    PSTR pszAccountOU = NULL;
    PSTR pszUsername = NULL;
    PSTR pszPassword = NULL;
    PSTR pszOSName = "";
    PSTR pszOSVersion = "";
    PSTR pszOSServicePack = "";
    LSA_AD_IPC_JOIN_DOMAIN_REQ Request = {0};
    HANDLE hServer = NULL;
    LWMsgDataContext *pDataCtx = NULL;
    size_t sInputBlobSize = 0;
    PVOID pInputBlob = NULL;
    DWORD dwOutputBlobSize = 0;
    PVOID pOutputBlob = NULL;

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

    dwError = WkssSrvDecryptPasswordBlob(&SrvCtx,
                                         pPassword,
                                         NULL,
                                         0,
                                         &pwszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszDomainName, &pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszAccountName, &pszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszPassword, &pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszAccountOu)
    {
        dwError = LwWc16sToMbs(pwszAccountOu, &pszAccountOU);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (gethostname((PSTR)&(szHostname[0]),
                    sizeof(szHostname)/sizeof(szHostname[0])) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    Request.pszHostname        = (PSTR)&(szHostname[0]);
    Request.pszHostDnsDomain   = pszDomain;
    Request.pszDomain          = pszDomain;
    Request.pszOU              = pszAccountOU;
    Request.pszUsername        = pszUsername;
    Request.pszPassword        = pszPassword;
    Request.pszOSName          = pszOSName;
    Request.pszOSVersion       = pszOSVersion;
    Request.pszOSServicePack   = pszOSServicePack;

    if (dwJoinFlags & NETSETUP_ACCT_CREATE)
    {
        Request.dwFlags |= LSA_NET_JOIN_DOMAIN_ACCT_CREATE;
    }
    if (dwJoinFlags & NETSETUP_DOMAIN_JOIN_IF_JOINED)
    {
        Request.dwFlags |= LSA_NET_JOIN_DOMAIN_JOIN_IF_JOINED;
    }
    if (dwJoinFlags & NETSETUP_JOIN_UNSECURE)
    {
        Request.dwFlags |= LSA_NET_JOIN_DOMAIN_JOIN_UNSECURE;
    }
    if (dwJoinFlags & NETSETUP_MACHINE_PWD_PASSED)
    {
        Request.dwFlags |= LSA_NET_JOIN_DOMAIN_MACHINE_PWD_PASSED;
    }
    if (dwJoinFlags & NETSETUP_DEFER_SPN_SET)
    {
        Request.dwFlags |= LSA_NET_JOIN_DOMAIN_DEFER_SPN_SET;
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataCtx));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                      pDataCtx,
                                      LsaAdIPCGetJoinDomainReqSpec(),
                                      &Request,
                                      &pInputBlob,
                                      &sInputBlobSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvOpenServer(geteuid(),
                               getegid(),
                               getpid(),
                               &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvProviderIoControl(hServer,
                                      LSA_PROVIDER_TAG_AD,
                                      LSA_AD_IO_JOINDOMAIN,
                                      sInputBlobSize,
                                      pInputBlob,
                                      &dwOutputBlobSize,
                                      &pOutputBlob);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (hServer)
    {
        LsaSrvCloseServer(hServer);
    }

    if (pDataCtx)
    {
        lwmsg_data_context_delete(pDataCtx);
    }

    WkssSrvFreeAuthInfo(&SrvCtx);

    LW_SECURE_FREE_STRING(pszPassword);
    LW_SECURE_FREE_WSTRING(pwszPassword);
    LW_SECURE_FREE_MEMORY(pInputBlob, sInputBlobSize);
    LW_SAFE_FREE_MEMORY(pszDomain);
    LW_SAFE_FREE_MEMORY(pszAccountOU);
    LW_SAFE_FREE_MEMORY(pszUsername);

    return (WINERROR)dwError;

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
