/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        samr_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Samr rpc memory management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrAllocateLogonHours(
    OUT PVOID        pBuffer,
    IN OUT PDWORD    pdwOffset,
    IN OUT PDWORD    pdwSpaceLeft,
    IN LogonHours   *pIn,
    IN OUT PDWORD    pdwSize
    );


static
NTSTATUS
SamrAllocateUserInfo21(
    OUT UserInfo   *pOut,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN  UserInfo21 *pIn,
    IN OUT PDWORD   pdwSize
    );


static
NTSTATUS
SamrAllocateDisplayEntryFull(
    OUT PVOID                 pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  SamrDisplayEntryFull *pIn,
    IN OUT PDWORD             pdwSize
    );


static
NTSTATUS
SamrAllocateDisplayEntryGeneral(
    OUT PVOID                    pOut,
    IN OUT PDWORD                pdwOffset,
    IN OUT PDWORD                pdwSpaceLeft,
    IN  SamrDisplayEntryGeneral *pIn,
    IN OUT PDWORD                pdwSize
    );


static
NTSTATUS
SamrAllocateDisplayEntryGeneralGroup(
    OUT PVOID                         pOut,
    IN OUT PDWORD                     pdwOffset,
    IN OUT PDWORD                     pdwSpaceLeft,
    IN  SamrDisplayEntryGeneralGroup *pIn,
    IN OUT PDWORD                     pdwSize
    );


static
NTSTATUS
SamrAllocateDisplayEntryAscii(
    OUT PVOID                   pOut,
    IN OUT PDWORD               pdwOffset,
    IN OUT PDWORD               pdwSpaceLeft,
    IN  SamrDisplayEntryAscii  *pIn,
    IN OUT PDWORD               pdwSize
    );


NTSTATUS
SamrAllocateMemory(
    OUT PVOID  *ppOut,
    IN  size_t  sSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pMem = NULL;

    pMem = malloc(sSize);
    if (pMem == NULL)
    {
        ntStatus = STATUS_NO_MEMORY;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memset(pMem, 0, sSize);
    *ppOut = pMem;

cleanup:
    return ntStatus;

error:
    *ppOut = NULL;
    goto cleanup;
}


VOID
SamrFreeMemory(
    IN  PVOID pPtr
    )
{
    free(pPtr);
}


NTSTATUS
SamrAllocateNamesFromRidNameArray(
    OUT PWSTR           *ppwszNames,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  PRID_NAME_ARRAY  pIn,
    IN OUT PDWORD        pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD iName = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iName = 0; iName < pIn->dwCount; iName++)
    {
        LWBUF_ALLOC_WC16STR_FROM_UNICODE_STRING(
                               ppwszNames,
                               (PUNICODE_STRING)&(pIn->pEntries[iName].Name));
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateRidsFromRidNameArray(
    OUT UINT32          *pRids,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  PRID_NAME_ARRAY  pIn,
    IN OUT PDWORD        pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    UINT32 iRid = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iRid = 0; iRid < pIn->dwCount; iRid++)
    {
        LWBUF_ALLOC_DWORD(pRids, pIn->pEntries[iRid].dwRid);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateNames(
    OUT PWSTR        *ppwszNames,
    IN OUT PDWORD     pdwOffset,
    IN OUT PDWORD     pdwSpaceLeft,
    IN  PENTRY_ARRAY  pIn,
    IN OUT PDWORD     pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD iName = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iName = 0; iName < pIn->dwCount; iName++)
    {
        LWBUF_ALLOC_WC16STR_FROM_UNICODE_STRING(
                          ppwszNames,
                          (PUNICODE_STRING)&(pIn->pEntries[iName].Name));
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateNamesFromUnicodeStringArray(
    OUT PWSTR                *ppwszNames,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN PUNICODE_STRING_ARRAY  pIn,
    IN OUT PDWORD             pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD iName = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iName = 0; iName < pIn->dwCount; iName++)
    {
        LWBUF_ALLOC_WC16STR_FROM_UNICODE_STRING(
                          ppwszNames,
                          (PUNICODE_STRING)&(pIn->pNames[iName]));
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateIds(
    OUT UINT32     *pIds,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN  PIDS        pIn,
    IN OUT PDWORD   pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD iId = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iId = 0; iId < pIn->dwCount; iId++)
    {
        LWBUF_ALLOC_DWORD(pIds, pIn->pIds[iId]);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateSids(
    OUT PSID       *ppSids,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN  PSID_ARRAY  pIn,
    IN OUT PDWORD   pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD iSid = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iSid = 0; iSid < pIn->dwNumSids; iSid++)
    {
        LWBUF_ALLOC_PSID(ppSids, pIn->pSids[iSid].pSid);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateRidsFromRidWithAttributeArray(
    OUT UINT32                    *pRids,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN  PRID_WITH_ATTRIBUTE_ARRAY  pIn,
    IN OUT PDWORD                  pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD iRid = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iRid = 0; iRid < pIn->dwCount; iRid++)
    {
        LWBUF_ALLOC_DWORD(pRids, pIn->pRids[iRid].dwRid);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateAttributesFromRidWithAttributeArray(
    OUT UINT32                    *pAttributes,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN  PRID_WITH_ATTRIBUTE_ARRAY  pIn,
    IN OUT PDWORD                  pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD iAttr = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iAttr = 0; iAttr < pIn->dwCount; iAttr++)
    {
        LWBUF_ALLOC_DWORD(pAttributes, pIn->pRids[iAttr].dwAttributes);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrAllocateLogonHours(
    OUT PVOID        pOut,
    IN OUT PDWORD    pdwOffset,
    IN OUT PDWORD    pdwSpaceLeft,
    IN LogonHours   *pIn,
    IN OUT PDWORD    pdwSize
    )
{
    const DWORD dwLogonHoursSize = 1260;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    PVOID pLogonHours = NULL;
    PVOID *ppLogonHours = NULL;
    DWORD dwBlobSpaceLeft = 0;
    DWORD dwBlobOffset = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    /* length field */
    dwError = LwBufferAllocWord(pBuffer,
                                pdwOffset,
                                pdwSpaceLeft,
                                (WORD)pIn->units_per_week,
                                pdwSize);
    BAIL_ON_WIN_ERROR(dwError);

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwLogonHoursSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->units)
        {
            pLogonHours = LWBUF_TARGET_PTR(pBuffer, dwLogonHoursSize, pdwSpaceLeft);

            BAIL_IF_PTR_OVERLAP(PUINT8, pLogonHours, dwError);

            dwBlobSpaceLeft = dwLogonHoursSize;
            dwBlobOffset    = 0;
            
            /* logon hours blob */
            dwError  = LwBufferAllocFixedBlob(pLogonHours,
                                              &dwBlobOffset,
                                              &dwBlobSpaceLeft,
                                              pIn->units,
                                              dwLogonHoursSize,
                                              pdwSize);
            BAIL_ON_WIN_ERROR(dwError);
        }

        ppLogonHours     = (PVOID*)pCursor;
        *ppLogonHours    = pLogonHours;
        (*pdwSpaceLeft) -= (pLogonHours) ? LWBUF_ALIGN_SIZE(dwLogonHoursSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft) -= sizeof(PVOID);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwLogonHoursSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(PVOID);
    (*pdwSize)   += sizeof(PVOID);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateAliasInfo(
    OUT AliasInfo  *pOut,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN  WORD        swLevel,
    IN  AliasInfo  *pIn,
    IN OUT PDWORD   pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    switch (swLevel)
    {
    case ALIAS_INFO_ALL:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->all.name);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->all.num_members);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->all.description);
        break;

    case ALIAS_INFO_NAME:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->name);
        break;

    case ALIAS_INFO_DESCRIPTION:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->description);
        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateDomainInfo(
    OUT DomainInfo  *pOut,
    IN OUT PDWORD    pdwOffset,
    IN OUT PDWORD    pdwSpaceLeft,
    IN  WORD         swLevel,
    IN  DomainInfo  *pIn,
    IN OUT PDWORD    pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    switch (swLevel)
    {
    case 1:
        LWBUF_ALLOC_WORD(pBuffer, pIn->info1.min_pass_length);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info1.pass_history_length);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info1.pass_properties);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info1.max_pass_age);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info1.min_pass_age);
        break;

    case 2:
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info2.force_logoff_time);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info2.comment);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info2.domain_name);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info2.primary);
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->info2.sequence_num);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info2.unknown1);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info2.role);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info2.unknown2);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info2.num_users);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info2.num_groups);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info2.num_aliases);
        break;

    case 3:
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info3.force_logoff_time);
        break;

    case 4:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info4.comment);
        break;

    case 5:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.domain_name);
        break;

    case 6:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info6.primary);
        break;

    case 7:
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info7.role);
        break;

    case 8:
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->info8.sequence_number);
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->info8.domain_create_time);
        break;

    case 9:
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->info9.unknown);
        break;

    case 11:
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info11.info2.force_logoff_time);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info11.info2.comment);
        LWBUF_ALLOC_UNICODE_STRING(
                               pBuffer,
                               (PUNICODE_STRING)&pIn->info11.info2.domain_name);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info11.info2.primary);
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->info11.info2.sequence_num);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info11.info2.unknown1);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info11.info2.role);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info11.info2.unknown2);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info11.info2.num_users);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info11.info2.num_groups);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info11.info2.num_aliases);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info11.lockout_duration);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info11.lockout_window);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info11.lockout_threshold);
        break;

    case 12:
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info12.lockout_duration);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info12.lockout_window);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info12.lockout_threshold);
        break;

    case 13:
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->info13.sequence_number);
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->info13.domain_create_time);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info13.unknown1);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info13.unknown2);
        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateUserInfo(
    OUT UserInfo   *pOut,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN  WORD        swLevel,
    IN  UserInfo   *pIn,
    IN OUT PDWORD   pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    switch (swLevel)
    {
    case 1:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info1.account_name);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info1.full_name);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info1.primary_gid);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info1.description);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info1.comment);
        break;

    case 2:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info2.comment);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info2.unknown1);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info2.country_code);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info2.code_page);
        break;

    case 3:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info3.account_name);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info3.full_name);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info3.rid);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info3.primary_gid);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info3.home_directory);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info3.home_drive);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info3.logon_script);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info3.profile_path);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info3.workstations);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info3.last_logon);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info3.last_logoff);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info3.last_password_change);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info3.allow_password_change);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info3.force_password_change);
        LWBUF_ALLOC_LOGON_HOURS(pBuffer, &pIn->info3.logon_hours);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info3.bad_password_count);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info3.logon_count);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info3.account_flags);
        break;

    case 4:
        LWBUF_ALLOC_LOGON_HOURS(pBuffer, &pIn->info4.logon_hours);
        break;

    case 5:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.account_name);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.full_name);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info5.rid);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info5.primary_gid);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.home_directory);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.home_drive);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.logon_script);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.profile_path);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.description);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info5.workstations);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info5.last_logon);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info5.last_logoff);
        LWBUF_ALLOC_LOGON_HOURS(pBuffer, &pIn->info5.logon_hours);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info5.bad_password_count);
        LWBUF_ALLOC_WORD(pBuffer, pIn->info5.logon_count);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info5.last_password_change);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info5.account_expiry);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info5.account_flags);
        break;

    case 6:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info6.account_name);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info6.full_name);
        break;

    case 7:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info7.account_name);
        break;

    case 8:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info8.full_name);
        break;

    case 9:
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info9.primary_gid);
        break;

    case 10:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info10.home_directory);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info10.home_drive);
        break;

    case 11:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info11.logon_script);
        break;

    case 12:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info12.profile_path);
        break;

    case 13:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info13.description);
        break;

    case 14:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info14.workstations);
        break;

    case 16:
        LWBUF_ALLOC_DWORD(pBuffer, pIn->info16.account_flags);
        break;

    case 17:
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->info17.account_expiry);
        break;

    case 20:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->info20.parameters);
        break;

    case 21:
        ntStatus = SamrAllocateUserInfo21(pBuffer,
                                          pdwOffset,
                                          pdwSpaceLeft,
                                          &pIn->info21,
                                          pdwSize);
        break;

    case 23:
        ntStatus = SamrAllocateUserInfo21(pBuffer,
                                          pdwOffset,
                                          pdwSpaceLeft,
                                          &pIn->info23.info,
                                          pdwSize);
        LWBUF_ALLOC_CRYPT_PASSWORD(pBuffer, pIn->info23.password);
        break;

    case 24:
        LWBUF_ALLOC_CRYPT_PASSWORD(pBuffer, pIn->info24.password);
        LWBUF_ALLOC_BYTE(pBuffer, pIn->info24.password_len);
        break;

    case 25:
        ntStatus = SamrAllocateUserInfo21(pBuffer,
                                          pdwOffset,
                                          pdwSpaceLeft,
                                          &pIn->info25.info,
                                          pdwSize);
        LWBUF_ALLOC_CRYPT_PASSWORD_EX(pBuffer, pIn->info25.password);
        break;

    case 26:
        LWBUF_ALLOC_CRYPT_PASSWORD_EX(pBuffer, pIn->info26.password);
        LWBUF_ALLOC_BYTE(pBuffer, pIn->info26.password_len);
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
    }

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrAllocateUserInfo21(
    OUT UserInfo   *pOut,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN  UserInfo21 *pIn,
    IN OUT PDWORD   pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    UserInfo21 *pInfo = &pOut->info21;
    PVOID pBuffer = pInfo;
    PVOID pCursor = NULL;
    PVOID pBufBlob = NULL;
    PVOID *ppBufBlob = NULL;
    DWORD dwBufBlobSpaceLeft = 0;
    DWORD dwBufBlobOffset = 0;
    DWORD dwBufBlobSize = 0;

    LWBUF_ALLOC_NTTIME(pBuffer, pIn->last_logon);
    LWBUF_ALLOC_NTTIME(pBuffer, pIn->last_logoff);
    LWBUF_ALLOC_NTTIME(pBuffer, pIn->last_password_change);
    LWBUF_ALLOC_NTTIME(pBuffer, pIn->account_expiry);
    LWBUF_ALLOC_NTTIME(pBuffer, pIn->allow_password_change);
    LWBUF_ALLOC_NTTIME(pBuffer, pIn->force_password_change);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->account_name);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->full_name);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->home_directory);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->home_drive);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->logon_script);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->profile_path);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->description);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->workstations);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->comment);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->parameters);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->unknown1);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->unknown2);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->unknown3);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->buf_count);
    dwBufBlobSize = pIn->buf_count;

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwBufBlobSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->buffer)
        {
            pBufBlob = LWBUF_TARGET_PTR(pBuffer, dwBufBlobSize, pdwSpaceLeft);

            /* sanity check - the data pointer and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(PUINT8, pBufBlob, dwError);

            dwBufBlobSpaceLeft = dwBufBlobSize;
            dwBufBlobOffset    = 0;

            dwError = LwBufferAllocFixedBlob(pBufBlob,
                                             &dwBufBlobOffset,
                                             &dwBufBlobSpaceLeft,
                                             pIn->buffer,
                                             pIn->buf_count,
                                             pdwSize);
            BAIL_ON_WIN_ERROR(dwError);
        }

        ppBufBlob        = (PVOID*)pCursor;
        *ppBufBlob       = pBufBlob;
        (*pdwSpaceLeft) -= (pBufBlob) ? LWBUF_ALIGN_SIZE(dwBufBlobSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft) -= sizeof(PVOID);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwBufBlobSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(PVOID);
    (*pdwSize)   += sizeof(PVOID);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->rid);
    LWBUF_ALLOC_DWORD(pBuffer, pIn->primary_gid);
    LWBUF_ALLOC_DWORD(pBuffer, pIn->account_flags);
    LWBUF_ALLOC_DWORD(pBuffer, pIn->fields_present);
    LWBUF_ALLOC_LOGON_HOURS(pBuffer, &pIn->logon_hours);
    LWBUF_ALLOC_WORD(pBuffer, pIn->bad_password_count);
    LWBUF_ALLOC_WORD(pBuffer, pIn->logon_count);
    LWBUF_ALLOC_WORD(pBuffer, pIn->country_code);
    LWBUF_ALLOC_WORD(pBuffer, pIn->code_page);
    LWBUF_ALLOC_BYTE(pBuffer, pIn->nt_password_set);
    LWBUF_ALLOC_BYTE(pBuffer, pIn->lm_password_set);
    LWBUF_ALLOC_BYTE(pBuffer, pIn->password_expired);
    LWBUF_ALLOC_BYTE(pBuffer, pIn->unknown4);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateDisplayInfo(
    OUT PVOID              pOut,
    IN OUT PDWORD          pdwOffset,
    IN OUT PDWORD          pdwSpaceLeft,
    IN WORD                swLevel,
    IN  SamrDisplayInfo   *pIn,
    IN OUT PDWORD          pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    PVOID *ppEntry = NULL;
    DWORD i = 0;
    DWORD dwCount = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    /*
     * All info levels start the same way so it is safe
     * to use level 1 to get the counter field
     */
    dwCount = pIn->info1.count;

    LWBUF_ALLOC_DWORD(pBuffer, dwCount);
    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    /* Set pointer to the entries array */
    if (pBuffer)
    {
        pCursor  = pBuffer + (*pdwOffset);
        ppEntry  = (PVOID*)pCursor;
    }

    (*pdwOffset) += sizeof(PVOID);
    (*pdwSize) += sizeof(PVOID);

    if (pdwSpaceLeft)
    {
        (*pdwSpaceLeft) -= sizeof(PVOID);
    }

    if (pBuffer)
    {
        *ppEntry = (PVOID)(pBuffer + (*pdwOffset));
    }

    /* Allocate the entries */
    for (i = 0; i < dwCount; i++)
    {
        switch (swLevel)
        {
        case 1:
            ntStatus = SamrAllocateDisplayEntryFull(pBuffer,
                                                    pdwOffset,
                                                    pdwSpaceLeft,
                                                    &(pIn->info1.entries[i]),
                                                    pdwSize);
            break;

        case 2:
            ntStatus = SamrAllocateDisplayEntryGeneral(pBuffer,
                                                       pdwOffset,
                                                       pdwSpaceLeft,
                                                       &(pIn->info2.entries[i]),
                                                       pdwSize);
            break;

        case 3:
            ntStatus = SamrAllocateDisplayEntryGeneralGroup(
                                                    pBuffer,
                                                    pdwOffset,
                                                    pdwSpaceLeft,
                                                    &(pIn->info3.entries[i]),
                                                    pdwSize);
            break;

        case 4:
            ntStatus = SamrAllocateDisplayEntryAscii(pBuffer,
                                                     pdwOffset,
                                                     pdwSpaceLeft,
                                                     &(pIn->info4.entries[i]),
                                                     pdwSize);
            break;

        case 5:
            ntStatus = SamrAllocateDisplayEntryAscii(pBuffer,
                                                     pdwOffset,
                                                     pdwSpaceLeft,
                                                     &(pIn->info5.entries[i]),
                                                     pdwSize);
            break;

        default:
            ntStatus = STATUS_INVALID_INFO_CLASS;
            break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrAllocateDisplayEntryFull(
    OUT PVOID                 pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  SamrDisplayEntryFull *pIn,
    IN OUT PDWORD             pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;

    LWBUF_ALLOC_DWORD(pOut, pIn->idx);
    LWBUF_ALLOC_DWORD(pOut, pIn->rid);
    LWBUF_ALLOC_DWORD(pOut, pIn->account_flags);
    LWBUF_ALLOC_UNICODE_STRING(pOut, (PUNICODE_STRING)&pIn->account_name);
    LWBUF_ALLOC_UNICODE_STRING(pOut, (PUNICODE_STRING)&pIn->description);
    LWBUF_ALLOC_UNICODE_STRING(pOut, (PUNICODE_STRING)&pIn->full_name);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrAllocateDisplayEntryGeneral(
    OUT PVOID                    pOut,
    IN OUT PDWORD                pdwOffset,
    IN OUT PDWORD                pdwSpaceLeft,
    IN  SamrDisplayEntryGeneral *pIn,
    IN OUT PDWORD                pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;

    LWBUF_ALLOC_DWORD(pOut, pIn->idx);
    LWBUF_ALLOC_DWORD(pOut, pIn->rid);
    LWBUF_ALLOC_DWORD(pOut, pIn->account_flags);
    LWBUF_ALLOC_UNICODE_STRING(pOut, (PUNICODE_STRING)&pIn->account_name);
    LWBUF_ALLOC_UNICODE_STRING(pOut, (PUNICODE_STRING)&pIn->description);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrAllocateDisplayEntryGeneralGroup(
    OUT PVOID                         pOut,
    IN OUT PDWORD                     pdwOffset,
    IN OUT PDWORD                     pdwSpaceLeft,
    IN  SamrDisplayEntryGeneralGroup *pIn,
    IN OUT PDWORD                     pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;

    LWBUF_ALLOC_DWORD(pOut, pIn->idx);
    LWBUF_ALLOC_DWORD(pOut, pIn->rid);
    LWBUF_ALLOC_DWORD(pOut, pIn->account_flags);
    LWBUF_ALLOC_UNICODE_STRING(pOut, (PUNICODE_STRING)&pIn->account_name);
    LWBUF_ALLOC_UNICODE_STRING(pOut, (PUNICODE_STRING)&pIn->description);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrAllocateDisplayEntryAscii(
    OUT PVOID                   pOut,
    IN OUT PDWORD               pdwOffset,
    IN OUT PDWORD               pdwSpaceLeft,
    IN  SamrDisplayEntryAscii  *pIn,
    IN OUT PDWORD               pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;

    LWBUF_ALLOC_DWORD(pOut, pIn->idx);
    LWBUF_ALLOC_ANSI_STRING(pOut, &pIn->account_name);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR_RELATIVE    *ppOut,
    IN  PSAMR_SECURITY_DESCRIPTOR_BUFFER  pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrAllocateMemory((PVOID*)&pSecDesc,
                                  pIn->ulBufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pSecDesc, pIn->pBuffer, pIn->ulBufferLen);

    *ppOut = pSecDesc;

cleanup:
    return ntStatus;

error:
    if (pSecDesc)
    {
        SamrFreeMemory(pSecDesc);
    }

    *ppOut = NULL;

    goto cleanup;
}
