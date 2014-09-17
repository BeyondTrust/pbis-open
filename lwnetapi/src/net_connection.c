/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        net_connection.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Rpc connection functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
NetConnectSamr(
    PNET_CONN   *ppConn,
    PCWSTR       pwszHostname,
    DWORD        dwReqDomainAccess,
    DWORD        dwReqBuiltinAccess,
    PIO_CREDS    pCreds
    )
{
    const DWORD dwDefConnAccess = SAMR_ACCESS_OPEN_DOMAIN |
                                  SAMR_ACCESS_ENUM_DOMAINS;

    const DWORD dwDefDomainAccess = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                    DOMAIN_ACCESS_OPEN_ACCOUNT |
                                    DOMAIN_ACCESS_LOOKUP_INFO_2;
			     
    const DWORD dwDefBuiltinAccess = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                  DOMAIN_ACCESS_OPEN_ACCOUNT |
                                  DOMAIN_ACCESS_LOOKUP_INFO_2;
    const DWORD dwSize = 128;
    const char *builtin = "BUILTIN";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    unsigned32 rpcStatus = 0;
    SAMR_BINDING hSamrBinding = NULL;
    PNET_CONN pConn = NULL;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBuiltin = NULL;
    PSID pBuiltinSid = NULL;
    PSID pDomainSid = NULL;
    DWORD dwConnAccess = 0;
    DWORD dwDomainAccess = 0;
    DWORD dwBuiltinAccess = 0;
    DWORD dwResume = 0;
    DWORD dwNumEntries = 0;
    DWORD i = 0;
    PSTR pszHost = NULL;
    PWSTR *ppwszDomainNames = NULL;
    PWSTR pwszDomainName = NULL;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    unsigned char *pszSessionKey = NULL;
    unsigned16 swSessionKeyLen = 0;

    BAIL_ON_INVALID_PTR(ppConn, err);

    if ((*ppConn) == NULL)
    {
        /* Create a new connection */
        ntStatus = NetAllocateMemory(OUT_PPVOID(&pConn), sizeof(*pConn));
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->eType = NET_CONN_SAMR;
    }
    else
    {
        /* Use existing connection (usually to gain more access rights) */
        pConn = (*ppConn);

        if (pConn->eType != NET_CONN_SAMR)
        {
            err = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN_ERROR(err);
        }
    }

    if (pConn->Rpc.Samr.dwConnAccess == 0 ||
        pConn->Rpc.Samr.hBinding == NULL)
    {
        dwConnAccess = dwDefConnAccess;

        if (pwszHostname)
        {
            /* remote host */
            err = LwWc16sToMbs(pwszHostname, &pszHost);
            BAIL_ON_WIN_ERROR(err);
        }
        else
        {
            /* local host (ncalrpc) */
            pszHost = NULL;
        }

        ntStatus = SamrInitBindingDefault(&hSamrBinding,
                                          pwszHostname,
                                          pCreds);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrConnect2(hSamrBinding, pwszHostname, dwConnAccess, &hConn);
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->Rpc.Samr.dwConnAccess = dwConnAccess;
        pConn->Rpc.Samr.hBinding     = hSamrBinding;
        pConn->Rpc.Samr.hConn        = hConn;

        pszSessionKey   = NULL;
        swSessionKeyLen = 0;

        rpc_binding_inq_transport_info(hSamrBinding,
                                       &hTransportInfo,
                                       &rpcStatus);
        if (rpcStatus)
        {
            ntStatus = STATUS_CONNECTION_INVALID;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /* Check if there's transport info (there may be none) and copy
           the session key */
        if (hTransportInfo)
        {
            rpc_smb_transport_info_inq_session_key(hTransportInfo,
                                                   &pszSessionKey,
                                                   &swSessionKeyLen);
            if (swSessionKeyLen > 0)
            {
                memcpy(pConn->SessionKey, pszSessionKey, sizeof(pConn->SessionKey));
                pConn->dwSessionKeyLen = (DWORD)swSessionKeyLen;
            }
        }

    }
    else
    {
        hSamrBinding = pConn->Rpc.Samr.hBinding;
        hConn        = pConn->Rpc.Samr.hConn;
    }

    /* check if requested builtin domain access flags have been
       specified and whether they match already opened handle's
       access rights */
    if (dwReqBuiltinAccess != 0 &&
        pConn->Rpc.Samr.dwBuiltinAccess != 0 &&
        (pConn->Rpc.Samr.dwBuiltinAccess & dwReqBuiltinAccess) != dwReqBuiltinAccess)
    {
        ntStatus = SamrClose(hSamrBinding,
                             pConn->Rpc.Samr.hBuiltin);
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->Rpc.Samr.hBuiltin        = NULL;
        pConn->Rpc.Samr.dwBuiltinAccess = 0;
    }

    if (pConn->Rpc.Samr.dwBuiltinAccess == 0)
    {
        dwBuiltinAccess = dwDefBuiltinAccess | dwReqBuiltinAccess;
        hConn           = pConn->Rpc.Samr.hConn;

        ntStatus = RtlAllocateSidFromCString(&pBuiltinSid, "S-1-5-32");
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrOpenDomain(hSamrBinding,
                                  hConn,
                                  dwBuiltinAccess,
                                  pBuiltinSid,
                                  &hBuiltin);
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->Rpc.Samr.hBuiltin        = hBuiltin;
        pConn->Rpc.Samr.dwBuiltinAccess = dwBuiltinAccess;

        RTL_FREE(&pBuiltinSid);
    }

    /* check if requested host's domain access flags have been
       specified and whether they match already opened handle's
       access rights */
    if (dwReqDomainAccess != 0 &&
        pConn->Rpc.Samr.dwDomainAccess != 0 &&
        (pConn->Rpc.Samr.dwDomainAccess & dwReqDomainAccess) != dwReqDomainAccess)
    {
        ntStatus = SamrClose(hSamrBinding,
                             pConn->Rpc.Samr.hDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->Rpc.Samr.hDomain        = NULL;
        pConn->Rpc.Samr.dwDomainAccess = 0;
        RTL_FREE(&pConn->Rpc.Samr.pDomainSid);
    }

    if (pConn->Rpc.Samr.dwDomainAccess == 0)
    {
        dwResume         = 0;
        dwNumEntries     = 0;
        ppwszDomainNames = NULL;

        do {
            ntStatus = SamrEnumDomains(hSamrBinding,
                                       hConn,
                                       &dwResume,
                                       dwSize,
                                       &ppwszDomainNames,
                                       &dwNumEntries);
            if (ntStatus != STATUS_SUCCESS &&
                ntStatus != STATUS_MORE_ENTRIES)
            {
                BAIL_ON_NT_STATUS(ntStatus);
            }

            for (i = 0; i < dwNumEntries; i++)
            {
                CHAR pszName[32]; /* any netbios name can fit here */

                wc16stombs(pszName, ppwszDomainNames[i], sizeof(pszName));

                /* pick up first domain name that is not a builtin domain */
                if (strcasecmp(pszName, builtin))
                {
                    err = LwAllocateWc16String(&pwszDomainName,
                                               ppwszDomainNames[i]);
                    BAIL_ON_WIN_ERROR(err);

                    SamrFreeMemory(ppwszDomainNames);
                    ppwszDomainNames = NULL;
                    goto domain_name_found;
                }
            }

            if (ppwszDomainNames)
            {
                SamrFreeMemory(ppwszDomainNames);
                ppwszDomainNames = NULL;
            }
        }
        while (ntStatus == STATUS_MORE_ENTRIES);
        
domain_name_found:
        ntStatus = SamrLookupDomain(hSamrBinding,
                                    hConn,
                                    pwszDomainName,
                                    &pDomainSid);
        BAIL_ON_NT_STATUS(ntStatus);

        dwDomainAccess = dwDefDomainAccess | dwReqDomainAccess;

        ntStatus = SamrOpenDomain(hSamrBinding,
                                hConn,
                                dwDomainAccess,
                                pDomainSid,
                                &hDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->Rpc.Samr.hDomain        = hDomain;
        pConn->Rpc.Samr.dwDomainAccess = dwDomainAccess;

        err = LwAllocateWc16String(&pConn->Rpc.Samr.pwszDomainName,
                                   pwszDomainName);
        BAIL_ON_WIN_ERROR(err);

        if (pDomainSid)
        {
            ntStatus = RtlDuplicateSid(&pConn->Rpc.Samr.pDomainSid, pDomainSid);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ntStatus = STATUS_INVALID_SID;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    /* set the host name if it's completely new connection */
    if (pwszHostname &&
        pConn->pwszHostname == NULL)
    {
        err = LwAllocateWc16String(&pConn->pwszHostname, pwszHostname);
        BAIL_ON_WIN_ERROR(err);
    }

    /* return initialised connection and status code */
    *ppConn = pConn;

cleanup:
    RTL_FREE(&pBuiltinSid);

    if (pDomainSid) {
        SamrFreeMemory((void*)pDomainSid);
    }

    if (ppwszDomainNames)
    {
        SamrFreeMemory(ppwszDomainNames);
    }

    LW_SAFE_FREE_MEMORY(pszHost);
    LW_SAFE_FREE_MEMORY(pwszDomainName);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }       

    return err;

error:
    if (pConn)
    {
        NetDisconnectSamr(&pConn);
    }

    *ppConn = NULL;
    goto cleanup;
}


DWORD
NetConnectLsa(
    PNET_CONN  *ppConn,
    PCWSTR      pwszHostname,
    DWORD       dwReqPolicyAccess,
    PIO_CREDS   pCreds
    )
{
    const DWORD dwDefPolicyAccess = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PSTR pszHost = NULL;
    LSA_BINDING hLsaBinding = NULL;
    PNET_CONN pConn = NULL;
    POLICY_HANDLE hPolicy = NULL;
    DWORD dwPolicyAccess = 0;

    BAIL_ON_INVALID_PTR(ppConn, err);

    if ((*ppConn) == NULL)
    {
        /* create a new connection */
        ntStatus = NetAllocateMemory(OUT_PPVOID(&pConn), sizeof(*pConn));
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->eType = NET_CONN_LSA;
    }
    else
    {
        pConn = (*ppConn);

        if (pConn->eType != NET_CONN_LSA)
        {
            err = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN_ERROR(err);
        }
    }

    if (!(pConn->Rpc.Lsa.dwPolicyAccess & dwReqPolicyAccess) &&
        pConn->Rpc.Lsa.hBinding)
    {
        ntStatus = LsaClose(hLsaBinding, pConn->Rpc.Lsa.hPolicy);
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->Rpc.Lsa.hPolicy        = NULL;
        pConn->Rpc.Lsa.dwPolicyAccess = 0;
    }

    if (pConn->Rpc.Lsa.dwPolicyAccess == 0 ||
        pConn->Rpc.Lsa.hBinding == NULL)
    {
        dwPolicyAccess = dwDefPolicyAccess | dwReqPolicyAccess;

        err = LwWc16sToMbs(pwszHostname, &pszHost);
        BAIL_ON_WIN_ERROR(err);

        ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                         pwszHostname,
                                         pCreds);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LsaOpenPolicy2(hLsaBinding,
                                  pwszHostname,
                                  NULL,
                                  dwPolicyAccess,
                                  &hPolicy);
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->Rpc.Lsa.hBinding       = hLsaBinding;
        pConn->Rpc.Lsa.hPolicy        = hPolicy;
        pConn->Rpc.Lsa.dwPolicyAccess = dwPolicyAccess;
    }

    /* set the host name if it's completely new connection */
    if (pwszHostname &&
        pConn->pwszHostname == NULL)
    {
        err = LwAllocateWc16String(&pConn->pwszHostname, pwszHostname);
        BAIL_ON_WIN_ERROR(err);
    }

    /* return initialised connection and status code */
    if ((*ppConn) == NULL)
    {
        *ppConn = pConn;
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pszHost);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }       

    return err;

error:
    if (pConn)
    {
        NetDisconnectLsa(&pConn);
    }

    *ppConn = NULL;
    goto cleanup;
}


DWORD
NetConnectWkssvc(
    PNET_CONN  *ppConn,
    PCWSTR      pwszHostname,
    PIO_CREDS   pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    unsigned32 rpcStatus = 0;
    PWSTR pwszServerName = NULL;
    WKSS_BINDING hWkssBinding = NULL;
    PNET_CONN pConn = NULL;
    NETR_WKSTA_INFO WkstaInfo = {0};
    DWORD dwLevel = 100;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    unsigned32 ProtSeq = 0;
    unsigned char *SessionKey = NULL;
    unsigned16 SessionKeyLen = 0;

    BAIL_ON_INVALID_PTR(ppConn, err);

    if ((*ppConn) == NULL)
    {
        /* create a new connection */
        ntStatus = NetAllocateMemory(OUT_PPVOID(&pConn), sizeof(*pConn));
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->eType = NET_CONN_WKSSVC;
    }
    else
    {
        pConn = (*ppConn);

        if (pConn->eType != NET_CONN_WKSSVC)
        {
            err = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN_ERROR(err);
        }
    }

    if (pConn->Rpc.WksSvc.hBinding == NULL)
    {
        err = WkssInitBindingDefault(&hWkssBinding,
                                     pwszHostname,
                                     pCreds);
        BAIL_ON_WIN_ERROR(err);

        if (pwszHostname)
        {
            err = LwAllocateWc16String(&pwszServerName,
                                       pwszHostname);
            BAIL_ON_WIN_ERROR(err);
        }

        /*
         * This function is called only to establish a valid connection and
         * get the session key
         */
        err = NetrWkstaGetInfo(hWkssBinding,
                               pwszServerName,
                               dwLevel,
                               &WkstaInfo);
        BAIL_ON_WIN_ERROR(err);

        pConn->Rpc.WksSvc.hBinding = hWkssBinding;

        rpc_binding_inq_transport_info(hWkssBinding,
                                       &hTransportInfo,
                                       &rpcStatus);
        if (rpcStatus)
        {
            ntStatus = STATUS_CONNECTION_INVALID;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (hTransportInfo)
        {
            rpc_binding_inq_prot_seq(hWkssBinding,
                                     &ProtSeq,
                                     &rpcStatus);

            switch (ProtSeq)
            {
            case rpc_c_protseq_id_ncacn_np:
                rpc_smb_transport_info_inq_session_key(
                                           hTransportInfo,
                                           &SessionKey,
                                           &SessionKeyLen);
                break;

            case rpc_c_protseq_id_ncalrpc:
                rpc_lrpc_transport_info_inq_session_key(
                                           hTransportInfo,
                                           &SessionKey,
                                           &SessionKeyLen);
                break;
            }

            if (SessionKeyLen > 0)
            {
                memcpy(pConn->SessionKey,
                       SessionKey,
                       sizeof(pConn->SessionKey));
                pConn->dwSessionKeyLen = SessionKeyLen;
            }
        }
    }

    /* set the host name if it's completely new connection */
    if (pwszHostname &&
        pConn->pwszHostname == NULL)
    {
        err = LwAllocateWc16String(&pConn->pwszHostname, pwszHostname);
        BAIL_ON_WIN_ERROR(err);
    }

    /* return initialised connection and status code */
    if ((*ppConn) == NULL)
    {
        *ppConn = pConn;
    }

cleanup:
    if (WkstaInfo.pInfo100)
    {
        WkssFreeMemory(WkstaInfo.pInfo100);
    }

    LW_SAFE_FREE_MEMORY(pwszServerName);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }       

    return err;

error:
    if (pConn)
    {
        NetDisconnectWkssvc(&pConn);
    }

    *ppConn = NULL;
    goto cleanup;
}


VOID
NetDisconnectSamr(
    PNET_CONN  *ppConn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err ATTRIBUTE_UNUSED = ERROR_SUCCESS;
    SAMR_BINDING hSamrBinding = NULL;
    PNET_CONN pConn = NULL;

    BAIL_ON_INVALID_PTR(ppConn, err);

    pConn = (*ppConn);

    if (pConn == NULL ||
        pConn->eType != NET_CONN_SAMR)
    {
        goto cleanup;
    }

    hSamrBinding = pConn->Rpc.Samr.hBinding;

    if (hSamrBinding)
    {
        if (pConn->Rpc.Samr.hDomain)
        {
            ntStatus = SamrClose(hSamrBinding, pConn->Rpc.Samr.hDomain);
            BAIL_ON_NT_STATUS(ntStatus);

            pConn->Rpc.Samr.hDomain        = NULL;
            pConn->Rpc.Samr.dwDomainAccess = 0;

        }

        if (pConn->Rpc.Samr.hBuiltin)
        {
            ntStatus = SamrClose(hSamrBinding, pConn->Rpc.Samr.hBuiltin);
            BAIL_ON_NT_STATUS(ntStatus);

            pConn->Rpc.Samr.hBuiltin        = NULL;
            pConn->Rpc.Samr.dwBuiltinAccess = 0;
        }

        if (pConn->Rpc.Samr.hConn)
        {
            ntStatus = SamrClose(hSamrBinding, pConn->Rpc.Samr.hConn);
            BAIL_ON_NT_STATUS(ntStatus);

            pConn->Rpc.Samr.hConn        = NULL;
            pConn->Rpc.Samr.dwConnAccess = 0;
        }

        SamrFreeBinding(&hSamrBinding);
        pConn->Rpc.Samr.hBinding = NULL;
    }

    *ppConn = NULL;

cleanup:
    if (pConn)
    {
        LW_SAFE_FREE_MEMORY(pConn->Rpc.Samr.pwszDomainName);
        RTL_FREE(&pConn->Rpc.Samr.pDomainSid);
        LW_SAFE_FREE_MEMORY(pConn->pwszHostname);

        NetFreeMemory(pConn);
    }

    return;

error:
    goto cleanup;
}


VOID
NetDisconnectLsa(
    PNET_CONN  *ppConn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err ATTRIBUTE_UNUSED = ERROR_SUCCESS;
    LSA_BINDING hLsaBinding = NULL;
    PNET_CONN pConn = NULL;

    BAIL_ON_INVALID_PTR(ppConn, err);

    pConn = (*ppConn);

    if (pConn == NULL ||
        pConn->eType != NET_CONN_LSA)
    {
        goto cleanup;
    }

    hLsaBinding = pConn->Rpc.Lsa.hBinding;

    if (hLsaBinding &&
        pConn->Rpc.Lsa.hPolicy)
    {
        ntStatus = LsaClose(hLsaBinding, pConn->Rpc.Lsa.hPolicy);
        BAIL_ON_NT_STATUS(ntStatus);

        pConn->Rpc.Lsa.hPolicy        = NULL;
        pConn->Rpc.Lsa.dwPolicyAccess = 0;

        LsaFreeBinding(&hLsaBinding);
        pConn->Rpc.Lsa.hBinding = NULL;
    }

cleanup:
    if (pConn)
    {
        LW_SAFE_FREE_MEMORY(pConn->pwszHostname);

        NetFreeMemory(pConn);
    }

    return;

error:
    goto cleanup;
}


VOID
NetDisconnectWkssvc(
    PNET_CONN  *ppConn
    )
{
    WINERROR err ATTRIBUTE_UNUSED = ERROR_SUCCESS;
    WKSS_BINDING hWkssBinding = NULL;
    PNET_CONN pConn = NULL;

    BAIL_ON_INVALID_PTR(ppConn, err);

    pConn = (*ppConn);

    if (pConn == NULL ||
        pConn->eType != NET_CONN_WKSSVC)
    {
        goto cleanup;
    }

    hWkssBinding = pConn->Rpc.WksSvc.hBinding;

    if (hWkssBinding)
    {
        WkssFreeBinding(&hWkssBinding);
        pConn->Rpc.WksSvc.hBinding = NULL;
    }

cleanup:
    if (pConn)
    {
        LW_SAFE_FREE_MEMORY(pConn->pwszHostname);

        NetFreeMemory(pConn);
    }

    return;

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
