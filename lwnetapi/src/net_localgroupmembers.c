/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        net_localgroupmembers.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetLocalGroupAddMembers and NetLocalGroupDelMembers functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NET_API_STATUS
NetLocalGroupChangeMembers(
    PCWSTR pwszHostname,
    PCWSTR pwszAliasname,
    DWORD  dwLevel,
    PVOID  pBuffer,
    DWORD  dwNumEntries,
    DWORD  dwAliasAccess
    );


NET_API_STATUS
NetLocalGroupAddMembers(
    PCWSTR pwszHostname,
    PCWSTR pwszAliasname,
    DWORD  dwLevel,
    PVOID  pBuffer,
    DWORD  dwNumEntries
    )
{
    return NetLocalGroupChangeMembers(pwszHostname,
                                      pwszAliasname,
                                      dwLevel,
                                      pBuffer,
                                      dwNumEntries,
                                      ALIAS_ACCESS_ADD_MEMBER);
}



NET_API_STATUS
NetLocalGroupDelMembers(
    PCWSTR pwszHostname,
    PCWSTR pwszAliasname,
    DWORD  dwLevel,
    PVOID  pBuffer,
    DWORD  dwNumEntries
    )
{
    return NetLocalGroupChangeMembers(pwszHostname,
                                      pwszAliasname,
                                      dwLevel,
                                      pBuffer,
                                      dwNumEntries,
                                      ALIAS_ACCESS_REMOVE_MEMBER);
}


static
NET_API_STATUS
NetLocalGroupChangeMembers(
    PCWSTR pwszHostname,
    PCWSTR pwszAliasname,
    DWORD  dwLevel,
    PVOID  pBuffer,
    DWORD  dwNumEntries,
    DWORD  dwAliasAccess
    )
{
    const DWORD dwLsaAccessFlags = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const DWORD dwBuiltinDomainAccessFlags = DOMAIN_ACCESS_OPEN_ACCOUNT;
    const WORD wLsaLookupLevel = LSA_LOOKUP_NAMES_ALL;

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwAliasAccessFlags = 0;
    PNET_CONN pSamrConn = NULL;
    PNET_CONN pLsaConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    LSA_BINDING hLsaBinding = NULL;
    ACCOUNT_HANDLE hAlias = NULL;
    DWORD dwAliasRid = 0;
    DWORD i = 0;
    LOCALGROUP_MEMBERS_INFO_0 *pInfo0 = NULL;
    LOCALGROUP_MEMBERS_INFO_3 *pInfo3 = NULL;
    PSID *ppSids = NULL;
    POLICY_HANDLE hLsaPolicy = NULL;
    PWSTR *ppNames = NULL;
    RefDomainList *pDomains = NULL;
    TranslatedSid3 *pTransSids = NULL;
    DWORD dwSidsCount = 0;
    PSID pMemberSid = NULL;
    PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pwszAliasname, err);
    BAIL_ON_INVALID_PTR(pBuffer, err);

    switch (dwLevel)
    {
    case 0:
        pInfo0 = (PLOCALGROUP_MEMBERS_INFO_0)pBuffer;
        break;

    case 3:
        pInfo3 = (PLOCALGROUP_MEMBERS_INFO_3)pBuffer;
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
    }

    dwAliasAccessFlags = dwAliasAccess;

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&pSamrConn,
                            pwszHostname,
                            dwAliasAccess,
                            dwBuiltinDomainAccessFlags,
                            pCreds);
    BAIL_ON_NT_STATUS(status);

    hSamrBinding = pSamrConn->Rpc.Samr.hBinding;

    status = NetOpenAlias(pSamrConn,
                          pwszAliasname,
                          dwAliasAccessFlags,
                          &hAlias,
                          &dwAliasRid);
    if (status == STATUS_NONE_MAPPED)
    {
        /* No such alias in host's domain.
           Try to look in builtin domain. */
        status = NetOpenAlias(pSamrConn,
                              pwszAliasname,
                              dwAliasAccessFlags,
                              &hAlias,
                              &dwAliasRid);
        BAIL_ON_NT_STATUS(status);
	
    }
    else if (status != STATUS_SUCCESS)
    {
        BAIL_ON_NT_STATUS(status);
    }

    status = NetAllocateMemory(OUT_PPVOID(&ppSids),
                               sizeof(ppSids[0]) * dwNumEntries);
    BAIL_ON_NT_STATUS(status);

    if (dwLevel == 0)
    {
        for (i = 0; i < dwNumEntries; i++)
        {
            ppSids[i] = pInfo0[i].lgrmi0_sid;
        }
    }
    else if (dwLevel == 3)
    {
        status = NetConnectLsa(&pLsaConn,
                               pwszHostname,
                               dwLsaAccessFlags,
                               pCreds);
        BAIL_ON_NT_STATUS(status);

        hLsaBinding = pLsaConn->Rpc.Lsa.hBinding;
        hLsaPolicy  = pLsaConn->Rpc.Lsa.hPolicy;

        status = NetAllocateMemory(OUT_PPVOID(&ppNames),
                                   sizeof(ppNames[0]) * dwNumEntries);
        BAIL_ON_NT_STATUS(status);

        for (i = 0; i < dwNumEntries; i++)
        {
            err = LwAllocateWc16String(&(ppNames[i]),
                                       pInfo3[i].lgrmi3_domainandname);
            BAIL_ON_WIN_ERROR(err);
        }

        status = LsaLookupNames3(hLsaBinding,
                                 hLsaPolicy,
                                 dwNumEntries,
                                 ppNames,
                                 &pDomains,
                                 &pTransSids,
                                 wLsaLookupLevel,
                                 &dwSidsCount);
        BAIL_ON_NT_STATUS(status);

        for (i = 0; i < dwSidsCount; i++)
        {
            ppSids[i] = pTransSids[i].sid;
        }
    }

    for (i = 0; i < dwNumEntries; i++)
    {
        pMemberSid = ppSids[i];

        if (dwAliasAccessFlags == ALIAS_ACCESS_ADD_MEMBER)
        {
            status = SamrAddAliasMember(hSamrBinding,
                                        hAlias,
                                        pMemberSid);
        }
        else if (dwAliasAccessFlags == ALIAS_ACCESS_REMOVE_MEMBER)
        {
            status = SamrDeleteAliasMember(hSamrBinding,
                                           hAlias,
                                           pMemberSid);
        }
        BAIL_ON_NT_STATUS(status);
    }

    status = SamrClose(hSamrBinding, hAlias);
    BAIL_ON_NT_STATUS(status);

cleanup:
    NetDisconnectSamr(&pSamrConn);
    NetDisconnectLsa(&pLsaConn);

    if (ppSids)
    {
        NetFreeMemory(ppSids);
    }

    if (ppNames)
    {
        for (i = 0; i < dwNumEntries; i++)
        {
            LW_SAFE_FREE_MEMORY(ppNames[i]);
        }

        NetFreeMemory(ppNames);
    }

    if (pDomains)
    {
        LsaRpcFreeMemory(pDomains);
    }

    if (pTransSids)
    {
        LsaRpcFreeMemory(pTransSids);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

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
