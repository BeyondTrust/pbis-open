/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2010
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
 *        lsa_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Lsa rpc memory management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
LsaAllocateDomainInfo(
    OUT PVOID          pBuffer,
    IN OUT PDWORD      pdwOffset,
    IN OUT PDWORD      pdwSpaceLeft,
    IN LsaDomainInfo  *pIn,
    IN OUT PDWORD      pdwSize
    );


static
NTSTATUS
LsaAllocateAuditEventsInfo(
    OUT PVOID            pBuffer,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN AuditEventsInfo  *pIn,
    IN OUT PDWORD        pdwSize
    );


NTSTATUS
LsaRpcAllocateMemory(
    OUT PVOID *ppOut,
    IN  size_t sSize
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
LsaRpcFreeMemory(
    IN PVOID pPtr
    )
{
    free(pPtr);
}


NTSTATUS
LsaAllocateTranslatedSids(
    OUT TranslatedSid       *pOut,
    IN OUT PDWORD            pdwOffset,
    IN OUT PDWORD            pdwSpaceLeft,
    IN  TranslatedSidArray  *pIn,
    IN OUT PDWORD            pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    DWORD iTransSid = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iTransSid = 0; iTransSid < pIn->count; iTransSid++)
    {
        LWBUF_ALLOC_WORD(pBuffer, pIn->sids[iTransSid].type);
        LWBUF_ALIGN_TYPE(pdwOffset, pdwSize, pdwSpaceLeft, DWORD);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->sids[iTransSid].rid);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->sids[iTransSid].index);
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
LsaAllocateTranslatedSids2(
    OUT TranslatedSid2       *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  TranslatedSidArray2  *pIn,
    IN OUT PDWORD             pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    DWORD iTransSid = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iTransSid = 0; iTransSid < pIn->count; iTransSid++)
    {
        LWBUF_ALLOC_WORD(pBuffer, pIn->sids[iTransSid].type);
        LWBUF_ALIGN_TYPE(pdwOffset, pdwSize, pdwSpaceLeft, DWORD);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->sids[iTransSid].rid);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->sids[iTransSid].index);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->sids[iTransSid].unknown1);
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
LsaAllocateTranslatedSids3(
    OUT TranslatedSid3       *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  TranslatedSidArray3  *pIn,
    IN OUT PDWORD             pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    DWORD iTransSid = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iTransSid = 0; iTransSid < pIn->count; iTransSid++)
    {
        LWBUF_ALLOC_WORD(pBuffer, pIn->sids[iTransSid].type);
        LWBUF_ALIGN_PTR(pdwOffset, pdwSize, pdwSpaceLeft);

        if (pIn->sids[iTransSid].sid)
        {
            LWBUF_ALLOC_PSID(pBuffer, pIn->sids[iTransSid].sid);
        }
        else if (pIn->sids[iTransSid].type == SID_TYPE_DOMAIN ||
                 pIn->sids[iTransSid].type == SID_TYPE_INVALID ||
                 pIn->sids[iTransSid].type == SID_TYPE_UNKNOWN)
        {
            LWBUF_ALLOC_PSID(pBuffer, NULL);
        }
        else 
        {
            ntStatus = STATUS_INVALID_SID;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        LWBUF_ALLOC_DWORD(pBuffer, pIn->sids[iTransSid].index);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->sids[iTransSid].unknown1);
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
LsaAllocateRefDomainList(
    OUT RefDomainList *pOut,
    IN OUT PDWORD      pdwOffset,
    IN OUT PDWORD      pdwSpaceLeft,
    IN  RefDomainList *pIn,
    IN OUT PDWORD      pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    PVOID pDomains = NULL;
    PVOID *ppDomains = NULL;
    DWORD iDom = 0;
    DWORD dwDomainsSize = 0;
    DWORD dwDomainsSpaceLeft = 0;
    DWORD dwDomainsOffset = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->count);
    LWBUF_ALIGN_PTR(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pIn->count > 0)
    {
        for (iDom = 0; iDom < pIn->count; iDom++)
        {
            ntStatus = LsaAllocateDomainInfo(NULL,
                                             &dwDomainsOffset,
                                             NULL,
                                             &(pIn->domains[iDom]),
                                             &dwDomainsSize);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwDomainsSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->count)
        {
            pDomains = LWBUF_TARGET_PTR(pBuffer, dwDomainsSize, pdwSpaceLeft);

            /* sanity check - the data pointer and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(PBYTE, pDomains, dwError);

            dwDomainsSpaceLeft = dwDomainsSize;
            dwDomainsOffset    = 0;

            /* Allocate the entries */
            for (iDom = 0; iDom < pIn->count; iDom++)
            {
                PVOID pDomCursor = pDomains + (iDom * sizeof(pIn->domains[0]));

                ntStatus = LsaAllocateDomainInfo(pDomCursor,
                                                 &dwDomainsOffset,
                                                 &dwDomainsSpaceLeft,
                                                 &(pIn->domains[iDom]),
                                                 pdwSize);
                BAIL_ON_NT_STATUS(ntStatus);

                dwDomainsOffset = 0;
            }
        }

        ppDomains        = (PVOID*)pCursor;
        *ppDomains       = (PVOID)pDomains;
        (*pdwSpaceLeft) -= (pDomains) ? LWBUF_ALIGN_SIZE(dwDomainsSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft) -= sizeof(PVOID);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwDomainsSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(PVOID);
    (*pdwSize)   += sizeof(PVOID);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->max_size);

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
LsaAllocateDomainInfo(
    OUT PVOID          pBuffer,
    IN OUT PDWORD      pdwOffset,
    IN OUT PDWORD      pdwSpaceLeft,
    IN LsaDomainInfo  *pIn,
    IN OUT PDWORD      pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;

    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->name);
    LWBUF_ALLOC_PSID(pBuffer, pIn->sid);

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
LsaAllocateTranslatedNames(
    OUT TranslatedName       *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  TranslatedNameArray  *pIn,
    IN OUT PDWORD             pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    DWORD iTransName = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iTransName = 0; iTransName < pIn->count; iTransName++)
    {
        LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);
        LWBUF_ALLOC_WORD(pBuffer, pIn->names[iTransName].type);
        LWBUF_ALIGN_PTR(pdwOffset, pdwSize, pdwSpaceLeft);
        LWBUF_ALLOC_UNICODE_STRING(
                             pBuffer,
                             (PUNICODE_STRING)&pIn->names[iTransName].name);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->names[iTransName].sid_index);
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
LsaAllocatePolicyInformation(
    OUT LsaPolicyInformation *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  WORD                  swLevel,
    IN  LsaPolicyInformation *pIn,
    IN OUT PDWORD             pdwSize
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
    case LSA_POLICY_INFO_AUDIT_LOG:
        LWBUF_ALLOC_DWORD(pBuffer, pIn->audit_log.percent_full);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->audit_log.log_size);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->audit_log.retention_time);
        LWBUF_ALLOC_BYTE(pBuffer, pIn->audit_log.shutdown_in_progress);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->audit_log.time_to_shutdown);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->audit_log.next_audit_record);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->audit_log.unknown);
        break;

    case LSA_POLICY_INFO_AUDIT_EVENTS:
        ntStatus = LsaAllocateAuditEventsInfo(pBuffer,
                                              pdwOffset,
                                              pdwSpaceLeft,
                                              &pIn->audit_events,
                                              pdwSize);
        break;

    case LSA_POLICY_INFO_DOMAIN:
        ntStatus = LsaAllocateDomainInfo(pBuffer,
                                         pdwOffset,
                                         pdwSpaceLeft,
                                         &pIn->domain,
                                         pdwSize);
        break;

    case LSA_POLICY_INFO_PD:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->pd.name);
        break;

    case LSA_POLICY_INFO_ACCOUNT_DOMAIN:
        ntStatus = LsaAllocateDomainInfo(pBuffer,
                                         pdwOffset,
                                         pdwSpaceLeft,
                                         &pIn->account_domain,
                                         pdwSize);
        break;

    case LSA_POLICY_INFO_ROLE:
        LWBUF_ALLOC_WORD(pBuffer, pIn->role.unknown);
        LWBUF_ALLOC_WORD(pBuffer, pIn->role.role);
        break;

    case LSA_POLICY_INFO_REPLICA:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->replica.source);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->replica.account);
        break;

    case LSA_POLICY_INFO_QUOTA:
        LWBUF_ALLOC_DWORD(pBuffer, pIn->quota.paged_pool);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->quota.non_paged_pool);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->quota.min_wss);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->quota.max_wss);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->quota.pagefile);
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->quota.unknown);
        break;

    case LSA_POLICY_INFO_DB:
        LWBUF_ALLOC_ULONG64(pBuffer, pIn->db.modified_id);
        LWBUF_ALLOC_NTTIME(pBuffer, pIn->db.db_create_time);
        break;

    case LSA_POLICY_INFO_AUDIT_FULL_SET:
        LWBUF_ALLOC_BYTE(pBuffer, pIn->audit_set.shutdown_on_full);
        break;

    case LSA_POLICY_INFO_AUDIT_FULL_QUERY:
        LWBUF_ALLOC_WORD(pBuffer, pIn->audit_query.unknown);
        LWBUF_ALLOC_BYTE(pBuffer, pIn->audit_query.shutdown_on_full);
        LWBUF_ALLOC_BYTE(pBuffer, pIn->audit_query.log_is_full);
        break;

    case LSA_POLICY_INFO_DNS:
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->dns.name);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->dns.dns_domain);
        LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                                   (PUNICODE_STRING)&pIn->dns.dns_forest);
        LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);
        LWBUF_ALLOC_BLOB(pBuffer,
                         sizeof(pIn->dns.domain_guid),
                         (PBYTE)&(pIn->dns.domain_guid));
        LWBUF_ALLOC_PSID(pBuffer, pIn->dns.sid);
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
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
LsaAllocateAuditEventsInfo(
    OUT PVOID            pOut,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN AuditEventsInfo  *pIn,
    IN OUT PDWORD        pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    PVOID pBlob = NULL;
    PVOID *ppBlob = NULL;
    DWORD dwBlobSize = 0;
    DWORD dwBlobSpaceLeft = 0;

    LWBUF_ALLOC_DWORD(pBuffer, pIn->auditing_mode);
    LWBUF_ALIGN_PTR(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pIn->count)
    {
        dwBlobSize = sizeof(pIn->settings[0]) * pIn->count;
    }

    /* Set pointer to the entries array */
    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwBlobSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->count)
        {
            pBlob = LWBUF_TARGET_PTR(pBuffer, dwBlobSize, pdwSpaceLeft);

            /* sanity check - the blob and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(AuditEventsInfo*, pBlob, dwError);

            dwBlobSpaceLeft = dwBlobSize;
            dwBlobSize      = 0;

            dwError = LwBufferAllocFixedBlob(
                                       pBlob,
                                       &dwBlobSize,
                                       &dwBlobSpaceLeft,
                                       (PBYTE)pIn->settings,
                                       sizeof(pIn->settings[0]) * pIn->count,
                                       &dwBlobSize);
            BAIL_ON_WIN_ERROR(dwError);
        }

        ppBlob           = (PVOID*)pCursor;
        *ppBlob          = (PVOID)pBlob;
        (*pdwSpaceLeft) -= (pBlob) ? LWBUF_ALIGN_SIZE(dwBlobSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft) -= sizeof(PVOID);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwBlobSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(PVOID);
    (*pdwSize)   += sizeof(PVOID);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->count);

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
LsaAllocateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR_RELATIVE   *ppOut,
    IN  PLSA_SECURITY_DESCRIPTOR_BUFFER  pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = LsaRpcAllocateMemory(OUT_PPVOID(&pSecDesc),
                                    pIn->BufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pSecDesc, pIn->pBuffer, pIn->BufferLen);

    *ppOut = pSecDesc;

cleanup:
    return ntStatus;

error:
    if (pSecDesc)
    {
        LsaRpcFreeMemory(pSecDesc);
    }

    *ppOut = NULL;

    goto cleanup;
}


NTSTATUS
LsaAllocateSids(
    OUT PSID                    *pOut,
    IN OUT PDWORD                pdwOffset,
    IN OUT PDWORD                pdwSpaceLeft,
    IN PLSA_ACCOUNT_ENUM_BUFFER  pIn,
    IN OUT PDWORD                pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    DWORD iSid = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iSid = 0; iSid < pIn->NumAccounts; iSid++)
    {
        LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);
        LWBUF_ALLOC_PSID(pBuffer, pIn->pAccount[iSid].pSid);
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
LsaAllocateAccountRightNames(
    OUT PWSTR               *pOut,
    IN OUT PDWORD            pdwOffset,
    IN OUT PDWORD            pdwSpaceLeft,
    IN PLSA_ACCOUNT_RIGHTS   pIn,
    IN OUT PDWORD            pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    DWORD iName = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iName = 0; iName < pIn->NumAccountRights; iName++)
    {
        LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);
        LWBUF_ALLOC_WC16STR_FROM_UNICODE_STRING(
                                   pBuffer,
                                   &pIn->pAccountRight[iName]);
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
LsaAllocatePrivilegeNames(
    OUT PWSTR                     *pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN PLSA_PRIVILEGE_ENUM_BUFFER  pIn,
    IN OUT PDWORD                  pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    DWORD iName = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iName = 0; iName < pIn->NumPrivileges; iName++)
    {
        LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);
        LWBUF_ALLOC_WC16STR_FROM_UNICODE_STRING(
                                   pBuffer,
                                   &pIn->pPrivilege[iName].Name);
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
LsaAllocatePrivilegeValues(
    OUT PWSTR                     *pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN PLSA_PRIVILEGE_ENUM_BUFFER  pIn,
    IN OUT PDWORD                  pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    DWORD iName = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (iName = 0; iName < pIn->NumPrivileges; iName++)
    {
        LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);
        LWBUF_ALLOC_WC16STR_FROM_UNICODE_STRING(
                                   pBuffer,
                                   &pIn->pPrivilege[iName].Name);
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
