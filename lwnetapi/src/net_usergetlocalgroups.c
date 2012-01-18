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
 *        net_usergetlocalgroups.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetUserGetLocalGroups
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetUserGetLocalGroups(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername,
    DWORD   dwLevel,
    DWORD   dwFlags,
    PVOID  *ppBuffer,
    DWORD   dwMaxBufferSize,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalEntries
    )
{
    const DWORD dwBuiltinDomainAccess = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                      DOMAIN_ACCESS_ENUM_ACCOUNTS;

    const DWORD dwUserAccess = USER_ACCESS_GET_GROUP_MEMBERSHIP;
	
    NTSTATUS status = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBtinDomain = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    PSID pDomainSid = NULL;
    PSID pUserSid = NULL;
    DWORD dwUserRid = 0;
    DWORD dwSidLen = 0;
    DWORD i = 0;
    SID_PTR SidPtr = {0};
    SID_ARRAY Sids = {0};
    PDWORD pdwUserRids = NULL;
    PDWORD pdwBuiltinUserRids = NULL;
    DWORD dwRidsCount = 0;
    DWORD dwBuiltinRidsCount = 0;
    DWORD dwInfoLevelSize = 0;
    DWORD dwTotalNumEntries = 0;
    PWSTR *ppwszAliasNames = NULL;
    PWSTR *ppwszBuiltinAliasNames = NULL;
    PDWORD pdwAliasTypes = NULL;
    PDWORD pdwBuiltinAliasTypes = NULL;
    PWSTR *ppwszLocalGroupNames = NULL;
    PVOID pSourceBuffer = NULL;
    PVOID pBuffer = NULL;
    PVOID pBufferCursor = NULL;
    DWORD dwSize = 0;
    DWORD dwTotalSize = 0;
    DWORD dwNumEntries = 0;
    DWORD dwSpaceAvailable = 0;
    PIO_CREDS pCreds = NULL;
    NET_VALIDATION_LEVEL eValidation = NET_VALIDATION_NONE;

    BAIL_ON_INVALID_PTR(pwszUsername, err);
    BAIL_ON_INVALID_PTR(ppBuffer, err);
    BAIL_ON_INVALID_PTR(pdwNumEntries, err);
    BAIL_ON_INVALID_PTR(pdwTotalEntries, err);

    switch (dwLevel)
    {
    case 0:
        dwInfoLevelSize = sizeof(LOCALGROUP_USERS_INFO_0);
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
    }

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            0,
                            dwBuiltinDomainAccess,
                            pCreds);
    BAIL_ON_NT_STATUS(status);
    
    hSamrBinding = pConn->Rpc.Samr.hBinding;
    hDomain      = pConn->Rpc.Samr.hDomain;
    hBtinDomain  = pConn->Rpc.Samr.hBuiltin;
    pDomainSid   = pConn->Rpc.Samr.pDomainSid;

    status = NetOpenUser(pConn,
                         pwszUsername,
                         dwUserAccess,
                         &hUser,
                         &dwUserRid);
    BAIL_ON_NT_STATUS(status);

    dwSidLen = RtlLengthRequiredSid(pDomainSid->SubAuthorityCount + 1);
    err = LwAllocateMemory(dwSidLen,
                           OUT_PPVOID(&pUserSid));
    BAIL_ON_WIN_ERROR(err);

    status = RtlCopySid(dwSidLen,
                        pUserSid,
                        pDomainSid);
    BAIL_ON_NT_STATUS(status);

    status = RtlAppendRidSid(dwSidLen,
                             pUserSid,
                             dwUserRid);
    BAIL_ON_NT_STATUS(status);

    SidPtr.pSid    = pUserSid;
    Sids.pSids     = &SidPtr;
    Sids.dwNumSids = 1;

    status = SamrGetAliasMembership(hSamrBinding,
                                    hDomain,
                                    &pUserSid,
                                    1,
                                    &pdwUserRids,
                                    &dwRidsCount);
    BAIL_ON_NT_STATUS(status);

    status = SamrGetAliasMembership(hSamrBinding,
                                    hBtinDomain,
                                    &pUserSid,
                                    1,
                                    &pdwBuiltinUserRids,
                                    &dwBuiltinRidsCount);
    BAIL_ON_NT_STATUS(status);

    dwTotalNumEntries = dwRidsCount + dwBuiltinRidsCount;

    err = LwAllocateMemory(
                      sizeof(ppwszLocalGroupNames[0]) * dwTotalNumEntries,
                      OUT_PPVOID(&ppwszLocalGroupNames));
    BAIL_ON_WIN_ERROR(err);

    if (dwRidsCount > 0)
    {
        status = SamrLookupRids(hSamrBinding,
                                hDomain,
                                dwRidsCount,
                                pdwUserRids,
                                &ppwszAliasNames,
                                &pdwAliasTypes);
        BAIL_ON_NT_STATUS(status);

        for (i = 0; i < dwRidsCount; i++)
        {
            ppwszLocalGroupNames[i] = ppwszAliasNames[i];
        }
    }

    if (dwBuiltinRidsCount > 0)
    {
        status = SamrLookupRids(hSamrBinding,
                                hBtinDomain,
                                dwBuiltinRidsCount,
                                pdwBuiltinUserRids,
                                &ppwszBuiltinAliasNames,
                                &pdwBuiltinAliasTypes);
        BAIL_ON_NT_STATUS(status);

        for (i = 0; i < dwBuiltinRidsCount; i++)
        {
            ppwszLocalGroupNames[i + dwRidsCount] = ppwszBuiltinAliasNames[i];
        }
    }

    for (i = 0; i < dwTotalNumEntries; i++)
    {
        pSourceBuffer = ppwszLocalGroupNames[i];

        dwSize = 0;
        err = NetAllocateLocalGroupUsersInfo(NULL,
                                             NULL,
                                             dwLevel,
                                             pSourceBuffer,
                                             &dwSize,
                                             eValidation);
        BAIL_ON_WIN_ERROR(err);

        dwTotalSize += dwSize;
        dwNumEntries++;

        if (dwTotalSize > dwMaxBufferSize)
        {
            dwTotalSize -= dwSize;
            dwNumEntries--;
            break;
        }
    }

    if (dwTotalNumEntries > 0 && dwNumEntries == 0)
    {
        err = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_WIN_ERROR(err);
    }

    if (dwTotalSize)
    {
        status = NetAllocateMemory(OUT_PPVOID(&pBuffer),
                                   dwTotalSize);
        BAIL_ON_NT_STATUS(status);
    }

    dwSize           = 0;
    pBufferCursor    = pBuffer;
    dwSpaceAvailable = dwTotalSize;

    for (i = 0; i < dwNumEntries; i++)
    {
        pSourceBuffer = ppwszLocalGroupNames[i];
        pBufferCursor = pBuffer + (i * dwInfoLevelSize);

        err = NetAllocateLocalGroupUsersInfo(pBufferCursor,
                                             &dwSpaceAvailable,
                                             dwLevel,
                                             pSourceBuffer,
                                             &dwSize,
                                             eValidation);
        BAIL_ON_WIN_ERROR(err);
    }

    if (dwNumEntries < dwTotalNumEntries)
    {
        err = ERROR_MORE_DATA;
    }

    status = SamrClose(hSamrBinding, hUser);
    BAIL_ON_NT_STATUS(status);

    *ppBuffer        = pBuffer;
    *pdwNumEntries   = dwNumEntries;
    *pdwTotalEntries = dwTotalNumEntries;

cleanup:
    LW_SAFE_FREE_MEMORY(pUserSid);
    LW_SAFE_FREE_MEMORY(ppwszLocalGroupNames);

    if (pdwUserRids)
    {
        SamrFreeMemory(pdwUserRids);
    }

    if (pdwBuiltinUserRids)
    {
        SamrFreeMemory(pdwBuiltinUserRids);
    }

    if (ppwszAliasNames)
    {
        SamrFreeMemory(ppwszAliasNames);
    }

    if (pdwAliasTypes)
    {
        SamrFreeMemory(pdwAliasTypes);
    }

    if (ppwszBuiltinAliasNames)
    {
        SamrFreeMemory(ppwszBuiltinAliasNames);
    }

    if (pdwBuiltinAliasTypes)
    {
        SamrFreeMemory(pdwBuiltinAliasTypes);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    return err;

error:
    if (pBuffer)
    {
        NetFreeMemory(pBuffer);
    }

    *ppBuffer        = NULL;
    *pdwNumEntries   = 0;
    *pdwTotalEntries = 0;

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
