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
 *        net_localgroupenum.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetLocalGroupEnum function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetLocalGroupEnum(
    PCWSTR  pwszHostname,
    DWORD   dwLevel,
    PVOID  *ppBuffer,
    DWORD   dwMaxBufferSize,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalNumEntries,
    PDWORD  pdwResume
    )
{
    const DWORD dwAccountFlags = 0;
    const DWORD dwAliasAccessFlags = ALIAS_ACCESS_LOOKUP_INFO;
    const WORD wInfoLevel = ALIAS_INFO_ALL;
	
    NTSTATUS status = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    DWORD dwResume = 0;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBtinDomain = NULL;
    DWORD dwSamrResume = 0;
    PWSTR *ppwszDomainAliases = NULL;
    PDWORD pdwDomainRids = NULL;
    DWORD dwNumDomainEntries = 0;
    DWORD dwTotalNumDomainEntries = 0;
    PWSTR *ppwszBtinDomainAliases = NULL;
    PDWORD pdwBtinDomainRids = NULL;
    DWORD dwNumBtinDomainEntries = 0;
    DWORD dwTotalNumBtinDomainEntries = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD dwNumEntries = 0;
    DWORD i = 0;
    PDWORD pdwRids = NULL;
    PWSTR *ppwszAliases = NULL;
    ACCOUNT_HANDLE hAlias = NULL;
    AliasInfo *pSamrAliasInfo = NULL;
    AliasInfoAll **ppAliasInfo = NULL;
    DWORD dwInfoLevelSize = 0;
    PVOID pSourceBuffer = NULL;
    DWORD dwSize = 0;
    DWORD dwTotalSize = 0;
    DWORD dwSpaceAvailable = 0;
    PVOID pBuffer = NULL;
    PVOID pBufferCursor = NULL;
    PIO_CREDS pCreds = NULL;
    NET_VALIDATION_LEVEL eValidation = NET_VALIDATION_NONE;

    BAIL_ON_INVALID_PTR(ppBuffer, err);
    BAIL_ON_INVALID_PTR(pdwNumEntries, err);
    BAIL_ON_INVALID_PTR(pdwTotalNumEntries, err);
    BAIL_ON_INVALID_PTR(pdwResume, err);

    switch (dwLevel)
    {
    case 0: dwInfoLevelSize = sizeof(LOCALGROUP_INFO_0);
        break;

    case 1: dwInfoLevelSize = sizeof(LOCALGROUP_INFO_1);
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
    }

    dwResume = *pdwResume;

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            0,
                            0,
                            pCreds);
    BAIL_ON_NT_STATUS(status);

    hSamrBinding = pConn->Rpc.Samr.hBinding;
    hDomain      = pConn->Rpc.Samr.hDomain;
    hBtinDomain  = pConn->Rpc.Samr.hBuiltin;

    do
    {
        status = SamrEnumDomainAliases(hSamrBinding,
                                       hDomain,
                                       &dwSamrResume,
                                       dwAccountFlags,
                                       &ppwszDomainAliases,
                                       &pdwDomainRids,
                                       &dwNumDomainEntries);
        if (status != STATUS_SUCCESS &&
            status != STATUS_MORE_ENTRIES)
        {
            BAIL_ON_NT_STATUS(status);
        }

        if (ppwszDomainAliases)
        {
            SamrFreeMemory(ppwszDomainAliases);
            ppwszDomainAliases = NULL;
        }

        if (pdwDomainRids)
        {
            SamrFreeMemory(pdwDomainRids);
            pdwDomainRids = NULL;
        }

        dwTotalNumDomainEntries += dwNumDomainEntries;
        dwNumDomainEntries       = 0;
    }
    while (status == STATUS_MORE_ENTRIES);

    dwSamrResume = 0;

    do
    {
        status = SamrEnumDomainAliases(hSamrBinding,
                                       hBtinDomain,
                                       &dwSamrResume,
                                       dwAccountFlags,
                                       &ppwszBtinDomainAliases,
                                       &pdwBtinDomainRids,
                                       &dwNumBtinDomainEntries);
        if (status != STATUS_SUCCESS &&
            status != STATUS_MORE_ENTRIES)
        {
            BAIL_ON_NT_STATUS(status);
        }

        if (ppwszBtinDomainAliases)
        {
            SamrFreeMemory(ppwszBtinDomainAliases);
            ppwszBtinDomainAliases = NULL;
        }

        if (pdwBtinDomainRids)
        {
            SamrFreeMemory(pdwBtinDomainRids);
            pdwBtinDomainRids = NULL;
        }

        dwTotalNumBtinDomainEntries += dwNumBtinDomainEntries;
        dwNumBtinDomainEntries       = 0;
    }
    while (status == STATUS_MORE_ENTRIES);

    dwTotalNumEntries = dwTotalNumDomainEntries + dwTotalNumBtinDomainEntries;

    status = NetAllocateMemory(OUT_PPVOID(&pdwRids),
                               sizeof(pdwRids[0]) * dwTotalNumEntries);
    BAIL_ON_NT_STATUS(status);

    status = NetAllocateMemory(OUT_PPVOID(&ppwszAliases),
                               sizeof(ppwszAliases[0]) * dwTotalNumEntries);
    BAIL_ON_NT_STATUS(status);

    status = NetAllocateMemory(OUT_PPVOID(&ppAliasInfo),
                               sizeof(ppAliasInfo[0]) * dwTotalNumEntries);
    BAIL_ON_NT_STATUS(status);

    dwTotalNumDomainEntries     = 0;
    dwTotalNumBtinDomainEntries = 0;
    dwSamrResume                = 0;

    do
    {
        status = SamrEnumDomainAliases(hSamrBinding,
                                       hDomain,
                                       &dwSamrResume,
                                       dwAccountFlags,
                                       &ppwszDomainAliases,
                                       &pdwDomainRids,
                                       &dwNumDomainEntries);
        if (status != STATUS_SUCCESS &&
            status != STATUS_MORE_ENTRIES)
        {
            BAIL_ON_NT_STATUS(status);
        }

        for (i = 0; i < dwNumDomainEntries; i++)
        {
            err = LwAllocateWc16String(&ppwszAliases[dwTotalNumDomainEntries + i],
                                       ppwszDomainAliases[i]);
            BAIL_ON_WIN_ERROR(err);

            pdwRids[dwTotalNumDomainEntries + i] = pdwDomainRids[i];
        }

        dwTotalNumDomainEntries += dwNumDomainEntries;
        dwNumDomainEntries       = 0;

        if (ppwszDomainAliases)
        {
            SamrFreeMemory(ppwszDomainAliases);
            ppwszDomainAliases = NULL;
        }

        if (pdwDomainRids)
        {
            SamrFreeMemory(pdwDomainRids);
            pdwDomainRids = NULL;
        }

    }
    while (status == STATUS_MORE_ENTRIES);

    dwSamrResume = 0;

    do
    {
        status = SamrEnumDomainAliases(hSamrBinding,
                                       hBtinDomain,
                                       &dwSamrResume,
                                       dwAccountFlags,
                                       &ppwszBtinDomainAliases,
                                       &pdwBtinDomainRids,
                                       &dwNumBtinDomainEntries);
        if (status != STATUS_SUCCESS &&
            status != STATUS_MORE_ENTRIES)
        {
            BAIL_ON_NT_STATUS(status);
        }

        for (i = 0; i < dwNumBtinDomainEntries; i++)
        {
            err = LwAllocateWc16String(&ppwszAliases[dwTotalNumDomainEntries +
                                                     dwTotalNumBtinDomainEntries + i],
                                       ppwszBtinDomainAliases[i]);
            BAIL_ON_WIN_ERROR(err);

            pdwRids[dwTotalNumDomainEntries +
                    dwTotalNumBtinDomainEntries + i] = pdwBtinDomainRids[i];
        }

        dwTotalNumBtinDomainEntries += dwNumBtinDomainEntries;
        dwNumBtinDomainEntries       = 0;

        if (ppwszBtinDomainAliases)
        {
            SamrFreeMemory(ppwszBtinDomainAliases);
            ppwszBtinDomainAliases = NULL;
        }

        if (pdwBtinDomainRids)
        {
            SamrFreeMemory(pdwBtinDomainRids);
            pdwBtinDomainRids = NULL;
        }
    }
    while (status == STATUS_MORE_ENTRIES);

    for (i = dwResume; i < dwTotalNumEntries; i++)
    {
        if (dwLevel == 0)
        {
            pSourceBuffer = ppwszAliases[i];
        }
        else
        {
            DOMAIN_HANDLE hDom = NULL;
            DWORD dwRid = 0;

            hDom  = (i < dwTotalNumDomainEntries) ? hDomain : hBtinDomain;
            dwRid = pdwRids[i];

            status = SamrOpenAlias(hSamrBinding,
                                   hDom,
                                   dwAliasAccessFlags,
                                   dwRid,
                                   &hAlias);
            BAIL_ON_NT_STATUS(status);

            status = SamrQueryAliasInfo(hSamrBinding,
                                        hAlias,
                                        wInfoLevel,
                                        &pSamrAliasInfo);
            BAIL_ON_NT_STATUS(status);

            ppAliasInfo[i - dwResume]  = &pSamrAliasInfo->all;
            pSourceBuffer              = &pSamrAliasInfo->all;

            status = SamrClose(hSamrBinding, hAlias);
            BAIL_ON_NT_STATUS(status);
        }

        dwSize = 0;
        err = NetAllocateLocalGroupInfo(NULL,
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
        if (dwLevel == 0)
        {
            pSourceBuffer = ppwszAliases[dwResume + i];
        }
        else
        {
            pSourceBuffer = ppAliasInfo[i];
        }

        pBufferCursor = pBuffer + (i * dwInfoLevelSize);

        err = NetAllocateLocalGroupInfo(pBufferCursor,
                                        &dwSpaceAvailable,
                                        dwLevel,
                                        pSourceBuffer,
                                        &dwSize,
                                        eValidation);
        BAIL_ON_WIN_ERROR(err);

    }

    if (dwResume + dwNumEntries < dwTotalNumEntries)
    {
        err = ERROR_MORE_DATA;
    }

    *ppBuffer           = pBuffer;
    *pdwNumEntries      = dwNumEntries;
    *pdwTotalNumEntries = dwTotalNumEntries; 
    *pdwResume          = dwResume + dwNumEntries;

cleanup:
    NetDisconnectSamr(&pConn);

    if (pdwRids)
    {
        NetFreeMemory(pdwRids);
    }

    if (ppwszAliases)
    {
        for (i = 0; i < dwTotalNumEntries; i++)
        {
            LW_SAFE_FREE_MEMORY(ppwszAliases[i]);
        }

        NetFreeMemory(ppwszAliases);
    }

    if (ppAliasInfo)
    {
        for (i = 0; i < dwNumEntries; i++)
        {
            if (ppAliasInfo[i])
            {
                SamrFreeMemory(ppAliasInfo[i]);
            }
        }

        NetFreeMemory(ppAliasInfo);
    }

    if (ppwszDomainAliases)
    {
        SamrFreeMemory(ppwszDomainAliases);
    }

    if (pdwDomainRids)
    {
        SamrFreeMemory(pdwDomainRids);
    }

    if (ppwszBtinDomainAliases)
    {
        SamrFreeMemory(ppwszBtinDomainAliases);
    }

    if (pdwBtinDomainRids)
    {
        SamrFreeMemory(pdwBtinDomainRids);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (pBuffer)
    {
        NetFreeMemory(pBuffer);
    }

    *ppBuffer           = NULL;
    *pdwNumEntries      = 0;
    *pdwTotalNumEntries = 0; 
    *pdwResume          = 0;

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
