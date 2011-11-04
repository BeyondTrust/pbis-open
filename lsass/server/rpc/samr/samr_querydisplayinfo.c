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
 *        samr_querydisplayinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQueryDisplayInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrSrvFillDisplayInfoFull(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    );


static
NTSTATUS
SamrSrvFillDisplayInfoGeneral(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    );


static
NTSTATUS
SamrSrvFillDisplayInfoGeneralGroups(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    );


static
NTSTATUS
SamrSrvFillDisplayInfoAscii(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfoAscii *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    );


NTSTATUS
SamrSrvQueryDisplayInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT16 level,
    /* [in] */ UINT32 start_idx,
    /* [in] */ UINT32 max_entries,
    /* [in] */ UINT32 buf_size,
    /* [out] */ UINT32 *total_size,
    /* [out] */ UINT32 *returned_size,
    /* [out] */ SamrDisplayInfo *info
    )
{
    const wchar_t wszFilterFmt[] = L"%ws=%u AND %ws>%u ORDER BY %ws";

    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;

    PWSTR wszAttributesLevel1[] = {
        wszAttrRecordId,
        wszAttrObjectSid,
        wszAttrSamAccountName,
        wszAttrDescription,
        wszAttrFullName,
        wszAttrAccountFlags,
        NULL
    };

    PWSTR wszAttributesLevel2[] = {
        wszAttrRecordId,
        wszAttrObjectSid,
        wszAttrSamAccountName,
        wszAttrDescription,
        wszAttrAccountFlags,
        NULL
    };

    PWSTR wszAttributesLevel3[] = {
        wszAttrRecordId,
        wszAttrObjectSid,
        wszAttrSamAccountName,
        wszAttrDescription,
        NULL
    };

    PWSTR wszAttributesLevel4[] = {
        wszAttrRecordId,
        wszAttrSamAccountName,
        NULL
    };

    PWSTR wszAttributesLevel5[] = {
        wszAttrRecordId,
        wszAttrSamAccountName,
        NULL
    };

    PWSTR *pwszAttributes[] = {
        wszAttributesLevel1,
        wszAttributesLevel2,
        wszAttributesLevel3,
        wszAttributesLevel4,
        wszAttributesLevel5
    };

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    DWORD dwObjectClass = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    SamrDisplayInfo Info;
    DWORD dwTotalSize = 0;
    DWORD dwSize = 0;
    DWORD dwCount = 0;
    DWORD i = 0;

    memset(&Info, 0, sizeof(Info));

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pDomCtx->dwAccessGranted & DOMAIN_ACCESS_OPEN_ACCOUNT))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pConnCtx = pDomCtx->pConnCtx;
    pwszBase = pDomCtx->pwszDn;

    switch (level) {
    case 1:
    case 2:
    case 4:
    case 5:
        dwObjectClass = DS_OBJECT_CLASS_USER;
        break;

    case 3:
        dwObjectClass = DIR_OBJECT_CLASS_DOMAIN_GROUP;
        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
        break;
    }

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrRecordId)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrSamAccountName)/sizeof(WCHAR)) - 1) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    ntStatus = SamrSrvAllocateMemory(
                                OUT_PPVOID(&pwszFilter),
                                dwFilterLen * sizeof(*pwszFilter));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrObjectClass,
                    dwObjectClass,
                    wszAttrRecordId,
                    start_idx,
                    wszAttrSamAccountName) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              pwszAttributes[level - 1],
                              FALSE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    dwTotalSize += sizeof(UINT32);    /* "count" field in info structure */

    for (i = 0; i < dwEntriesNum; i++)
    {
        pEntry = &(pEntries[i]);

        switch (level) {
        case 1:
            ntStatus = SamrSrvFillDisplayInfoFull(pDomCtx,
                                                  pEntry,
                                                  NULL,
                                                  i,
                                                  dwCount,
                                                  &dwTotalSize);
            break;

        case 2:
            ntStatus = SamrSrvFillDisplayInfoGeneral(pDomCtx,
                                                     pEntry,
                                                     NULL,
                                                     i,
                                                     dwCount,
                                                     &dwTotalSize);
            break;

        case 3:
            ntStatus = SamrSrvFillDisplayInfoGeneralGroups(pDomCtx,
                                                           pEntry,
                                                           NULL,
                                                           i,
                                                           dwCount,
                                                           &dwTotalSize);
            break;

        case 4:
        case 5:
            ntStatus = SamrSrvFillDisplayInfoAscii(pDomCtx,
                                                   pEntry,
                                                   NULL,
                                                   i,
                                                   dwCount,
                                                   &dwTotalSize);
            break;
        }

        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        if (dwTotalSize < buf_size && i < max_entries) {
            dwCount = i + 1;
        }
    }

    /* At least one account entry is returned regardless of declared
       max response size */
    dwCount  = (!dwCount) ? 1 : dwCount;

    dwSize += sizeof(UINT32);    /* "count" field in info structure */
    i       = start_idx;

    if (dwEntriesNum == 0)
    {
        i = 0;
        ntStatus = STATUS_NO_MORE_ENTRIES;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    for (i = 0; i < dwCount && i < dwEntriesNum; i++)
    {
        pEntry = &(pEntries[i]);

        switch (level)
        {
        case 1:
            ntStatus = SamrSrvFillDisplayInfoFull(pDomCtx,
                                                  pEntry,
                                                  &Info,
                                                  i,
                                                  dwCount,
                                                  &dwSize);
            break;

        case 2:
            ntStatus = SamrSrvFillDisplayInfoGeneral(pDomCtx,
                                                     pEntry,
                                                     &Info,
                                                     i,
                                                     dwCount,
                                                     &dwSize);
            break;

        case 3:
            ntStatus = SamrSrvFillDisplayInfoGeneralGroups(pDomCtx,
                                                           pEntry,
                                                           &Info,
                                                           i,
                                                           dwCount,
                                                           &dwSize);
            break;

        case 4:
            ntStatus = SamrSrvFillDisplayInfoAscii(pDomCtx,
                                                   pEntry,
                                                   &Info.info4,
                                                   i,
                                                   dwCount,
                                                   &dwSize);
            break;

        case 5:
            ntStatus = SamrSrvFillDisplayInfoAscii(pDomCtx,
                                                   pEntry,
                                                   &Info.info5,
                                                   i,
                                                   dwCount,
                                                   &dwSize);
            break;
        }

        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (dwCount < dwEntriesNum)
    {
        ntStatus = STATUS_MORE_ENTRIES;
    }

    *total_size    = dwTotalSize;
    *returned_size = dwSize;
    *info          = Info;

cleanup:
    if (pwszFilter)
    {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    /* Regardless of info level the pointer is at the same position */
    memset(&Info, 0, sizeof(Info));

    *total_size    = dwTotalSize;
    *returned_size = 0;
    *info          = Info;

    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDisplayInfoFull(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    PWSTR pwszSid = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszDescription = NULL;
    PWSTR pwszFullName = NULL;
    LONG64 llRecId = 0;
    PSID pSid = NULL;
    DWORD dwRid = 0;
    DWORD dwAccountFlags = 0;
    SamrDisplayInfoFull *pInfo1 = NULL;
    SamrDisplayEntryFull *pDisplayEntry = NULL;
    DWORD dwSize = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRecordId,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llRecId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);    
    
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);    
    
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrAccountFlags,
                                               DIRECTORY_ATTR_TYPE_INTEGER,
                                               &dwAccountFlags);
    BAIL_ON_LSA_ERROR(dwError);    
    
    dwSize  = (*pdwSize);
    dwSize += sizeof(pInfo->info1.entries[0]);
    dwSize += wc16slen(pwszUsername) * sizeof(WCHAR);
    dwSize += wc16slen(pwszFullName) * sizeof(WCHAR);

    /* If NULL pointer is passed we're just calculating the size */
    if (pInfo == NULL) goto done;
    pInfo1 = &pInfo->info1;

    if (!pInfo1->entries) {
        ntStatus = SamrSrvAllocateMemory(
                            (void**)&pInfo1->entries,
                            sizeof(pInfo1->entries[0]) * dwCount);
        pInfo1->count = dwCount;
    }

    pDisplayEntry = &(pInfo1->entries[i]);

    pDisplayEntry->idx           = (UINT32)llRecId;

    ntStatus = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];

    pDisplayEntry->rid           = (UINT32)dwRid;
    pDisplayEntry->account_flags = (UINT32)dwAccountFlags;

    ntStatus = SamrSrvInitUnicodeString(&pDisplayEntry->account_name,
                                        pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvInitUnicodeString(&pDisplayEntry->full_name,
                                        pwszFullName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvInitUnicodeString(&pDisplayEntry->description,
                                        pwszDescription);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

done:
    *pdwSize  = dwSize;

cleanup:
    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    return ntStatus;

error:
    if (pInfo1 && pInfo1->entries) {
        SamrSrvFreeMemory(pInfo1->entries);
        pInfo1->entries = NULL;
    }

    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDisplayInfoGeneral(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    PWSTR pwszSid = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszDescription = NULL;
    LONG64 llRecId = 0;
    PSID pSid = NULL;
    ULONG dwRid = 0;
    DWORD dwAccountFlags = 0;
    SamrDisplayInfoGeneral *pInfo2 = NULL;
    SamrDisplayEntryGeneral *pDisplayEntry = NULL;
    DWORD dwSize = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRecordId,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llRecId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);    

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrAccountFlags,
                                               DIRECTORY_ATTR_TYPE_INTEGER,
                                               &dwAccountFlags);
    BAIL_ON_LSA_ERROR(dwError);    
    
    dwSize  = (*pdwSize);
    dwSize += sizeof(pInfo->info2.entries[0]);
    dwSize += wc16slen(pwszUsername) * sizeof(WCHAR);

    /* If NULL pointer is passed we're just calculating the size */
    if (pInfo == NULL) goto done;
    pInfo2 = &pInfo->info2;

    if (!pInfo2->entries) {
        ntStatus = SamrSrvAllocateMemory(
                            (void**)&pInfo2->entries,
                            sizeof(pInfo2->entries[0]) * dwCount);
        pInfo2->count = dwCount;
    }

    pDisplayEntry = &(pInfo2->entries[i]);

    pDisplayEntry->idx           = (UINT32)llRecId;

    ntStatus = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];

    pDisplayEntry->rid           = (UINT32)dwRid;
    pDisplayEntry->account_flags = (UINT32)dwAccountFlags;

    ntStatus = SamrSrvInitUnicodeString(&pDisplayEntry->account_name,
                                        pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvInitUnicodeString(&pDisplayEntry->description,
                                        NULL);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

done:
    *pdwSize  = dwSize;

cleanup:
    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    return ntStatus;

error:
    if (pInfo2 && pInfo2->entries) {
        SamrSrvFreeMemory(pInfo2->entries);
        pInfo2->entries = NULL;
    }

    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDisplayInfoGeneralGroups(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    PWSTR pwszSid = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszDescription = NULL;
    LONG64 llRecId = 0;
    PSID pSid = NULL;
    ULONG dwRid = 0;
    DWORD dwGroupAttributes = 0;
    SamrDisplayInfoGeneralGroups *pInfo3 = NULL;
    SamrDisplayEntryGeneralGroup *pDisplayEntry = NULL;
    DWORD dwSize = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRecordId,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llRecId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);    

    dwSize  = (*pdwSize);
    dwSize += sizeof(pInfo->info3.entries[0]);
    dwSize += wc16slen(pwszUsername) * sizeof(WCHAR);

    /* If NULL pointer is passed we're just calculating the size */
    if (pInfo == NULL) goto done;
    pInfo3 = &pInfo->info3;

    if (!pInfo3->entries) {
        ntStatus = SamrSrvAllocateMemory(
                           (void**)&pInfo3->entries,
                           sizeof(pInfo3->entries[0]) * dwCount);
        pInfo3->count = dwCount;
    }

    pDisplayEntry = &(pInfo3->entries[i]);

    pDisplayEntry->idx           = (UINT32)llRecId;

    ntStatus = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];

    pDisplayEntry->rid           = (UINT32)dwRid;

    /* TODO: Add support for group attributes when it's time
       for domain groups */
    pDisplayEntry->account_flags = (UINT32)dwGroupAttributes;

    ntStatus = SamrSrvInitUnicodeString(&pDisplayEntry->account_name,
                                        pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvInitUnicodeString(&pDisplayEntry->description,
                                        NULL);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

done:
    *pdwSize  = dwSize;

cleanup:
    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    return ntStatus;

error:
    if (pInfo3 && pInfo3->entries) {
        SamrSrvFreeMemory(pInfo3->entries);
        pInfo3->entries = NULL;
    }

    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDisplayInfoAscii(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfoAscii *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    PWSTR pwszUsername = NULL;
    LONG64 llRecId = 0;
    SamrDisplayEntryAscii *pDisplayEntry = NULL;
    DWORD dwSize = 0;
    DWORD dwUsernameLen = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRecordId,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llRecId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwUsernameLen = wc16slen(pwszUsername);

    dwSize  = (*pdwSize);
    dwSize += sizeof(pInfo->entries[0]);
    dwSize += (dwUsernameLen + 1) * sizeof(CHAR);

    /* If NULL pointer is passed we're just calculating the size */
    if (pInfo == NULL) goto done;

    if (!pInfo->entries) {
        ntStatus = SamrSrvAllocateMemory(
                             (void**)&pInfo->entries,
                             sizeof(pInfo->entries[0]) * dwCount);
        pInfo->count = dwCount;
    }

    pDisplayEntry = &(pInfo->entries[i]);

    pDisplayEntry->idx = (UINT32)llRecId;

    pDisplayEntry->account_name.Length        = dwUsernameLen;
    pDisplayEntry->account_name.MaximumLength = dwUsernameLen;
    
    ntStatus = SamrSrvAllocateMemory(
                       (void**)&pDisplayEntry->account_name.Buffer,
                       (dwUsernameLen + 1) * sizeof(CHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16stombs(pDisplayEntry->account_name.Buffer,
               pwszUsername,
               dwUsernameLen + 1);

done:
    *pdwSize  = dwSize;

cleanup:
    return ntStatus;

error:
    if (pInfo && pInfo->entries) {
        SamrSrvFreeMemory(pInfo->entries);
        pInfo->entries = NULL;
    }

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
