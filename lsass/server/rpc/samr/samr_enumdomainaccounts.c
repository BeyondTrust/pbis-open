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
 *        samr_enumdomainaccounts.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrEnumDomainAccounts function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvEnumDomainAccounts(
    IN  handle_t          hBinding,
    IN  DOMAIN_HANDLE     hDomain,
    IN OUT PDWORD         pdwResume,
    IN  DWORD             dwObjectClass,
    IN  DWORD             dwFlagsFilter,
    IN  DWORD             dwMaxSize,
    OUT RID_NAME_ARRAY  **ppNames,
    OUT UINT32           *pdwNumEntries
    )
{
    wchar_t wszFilterFmt[] = L"%ws=%u AND %ws='%ws'";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntEnumStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDomainName[] = DS_ATTR_DOMAIN;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    size_t sDomainNameLen = 0;
    PWSTR pwszDomainName = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwTotalSize = 0;
    DWORD dwSize = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    DWORD dwNumEntriesReturned = 0;
    DWORD dwResume = 0;
    DWORD dwAccountFlags = 0;
    size_t sNameLen = 0;
    PWSTR pwszName = NULL;
    PWSTR pwszSid = NULL;
    PSID pSid = NULL;
    DWORD dwRid = 0;
    RID_NAME_ARRAY *pNames = NULL;
    RID_NAME *pName = NULL;
    DWORD dwNewResumeIdx = 0;

    PWSTR wszAttributes[] = {
        wszAttrSamAccountName,
        wszAttrObjectSid,
        wszAttrAccountFlags,
        NULL
    };

    BAIL_ON_INVALID_PTR(hDomain);
    BAIL_ON_INVALID_PTR(pdwResume);
    BAIL_ON_INVALID_PTR(ppNames);
    BAIL_ON_INVALID_PTR(pdwNumEntries);

    pDomCtx  = (PDOMAIN_CONTEXT)hDomain;
    dwResume = *pdwResume;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pDomCtx->dwAccessGranted & DOMAIN_ACCESS_ENUM_ACCOUNTS))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pConnCtx       = pDomCtx->pConnCtx;
    pwszBase       = pDomCtx->pwszDn;
    pwszDomainName = pDomCtx->pwszDomainName;

    dwError = LwWc16sLen(pwszDomainName, &sDomainNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrDomainName)/sizeof(WCHAR)) - 1) +
                  (sDomainNameLen + 1) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(dwFilterLen * sizeof(pwszFilter[0]),
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrObjectClass,
                    dwObjectClass,
                    wszAttrDomainName,
                    pwszDomainName) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvAllocateMemory(OUT_PPVOID(&pNames),
                                     sizeof(*pNames));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);


    if (dwResume >= dwNumEntries)
    {
        ntEnumStatus = STATUS_NO_MORE_ENTRIES;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwTotalSize += sizeof(pNames->dwCount);

    for (i = 0; i + dwResume < dwNumEntries; i++)
    {
        pEntry = &(pEntries[i + dwResume]);

        dwError = DirectoryGetEntryAttrValueByName(
                                     pEntry,
                                     wszAttrSamAccountName,
                                     DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                     &pwszName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                                     pEntry,
                                     wszAttrAccountFlags,
                                     DIRECTORY_ATTR_TYPE_INTEGER,
                                     &dwAccountFlags);
        BAIL_ON_LSA_ERROR(dwError);

        dwSize = 0;

        if (dwFlagsFilter &&
            !(dwAccountFlags & dwFlagsFilter))
        {
            continue;
        }

        dwError = LwWc16sLen(pwszName, &sNameLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwSize += sizeof(UINT32);
        dwSize += sNameLen * sizeof(pwszName[0]);
        dwSize += 2 * sizeof(UINT16);

        dwTotalSize += dwSize;
        dwCount++;

        if (dwTotalSize > dwMaxSize)
        {
            dwTotalSize -= dwSize;
            dwCount--;

            ntEnumStatus = STATUS_MORE_ENTRIES;
            break;
        }
    }

    /*
     * At least one entry is returned regardless of declared
     * max response size
     */
    dwNumEntriesReturned = (dwSize > 0 && dwCount == 0) ? 1 : dwCount;

    pNames->dwCount = dwNumEntriesReturned;
    ntStatus = SamrSrvAllocateMemory(
                           OUT_PPVOID(&pNames->pEntries),
                           sizeof(pNames->pEntries[0]) * pNames->dwCount);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0, dwCount = 0;
         dwCount < dwNumEntriesReturned && i + dwResume < dwNumEntries;
         i++)
    {
        pEntry = &(pEntries[i + dwResume]);

        dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                                   wszAttrObjectSid,
                                                   DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                   &pwszSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                                   wszAttrSamAccountName,
                                                   DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                   &pwszName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                                   wszAttrAccountFlags,
                                                   DIRECTORY_ATTR_TYPE_INTEGER,
                                                   &dwAccountFlags);
        BAIL_ON_LSA_ERROR(dwError);

        dwNewResumeIdx = i + dwResume + 1;

        if (dwFlagsFilter &&
            !(dwAccountFlags & dwFlagsFilter))
        {
            continue;
        }

        pName = &(pNames->pEntries[dwCount++]);

        ntStatus = RtlAllocateSidFromWC16String(&pSid, pwszSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwRid        = pSid->SubAuthority[pSid->SubAuthorityCount - 1];
        pName->dwRid = (UINT32)dwRid;
        
        ntStatus = SamrSrvInitUnicodeString(&pName->Name,
                                            pwszName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        RTL_FREE(&pSid);
    }

    *pdwResume      = dwNewResumeIdx;
    *pdwNumEntries  = dwNumEntriesReturned;
    *ppNames        = pNames;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszFilter);
    RTL_FREE(&pSid);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    if (ntStatus == STATUS_SUCCESS &&
        ntEnumStatus != STATUS_SUCCESS)
    {
        ntStatus = ntEnumStatus;
    }

    return ntStatus;

error:
    if (pNames)
    {
        for (i = 0; i < pNames->dwCount; i++)
        {
            SamrSrvFreeUnicodeString(&(pNames->pEntries[i].Name));
        }
        SamrSrvFreeMemory(pNames->pEntries);
        SamrSrvFreeMemory(pNames);
    }

    *pdwResume      = 0;
    *pdwNumEntries  = 0;
    *ppNames        = NULL;
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
