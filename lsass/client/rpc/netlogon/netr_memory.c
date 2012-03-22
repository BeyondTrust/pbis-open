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
 *        netr_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Netlogon rpc memory management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


static
NTSTATUS
NetrAllocateRidWithAttributeArray(
    OUT PVOID                     *pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN  PRID_WITH_ATTRIBUTE_ARRAY  pIn,
    IN OUT PDWORD                  pdwSize
    );


static
NTSTATUS
NetrAllocateRidWithAttribute(
    OUT PRID_WITH_ATTRIBUTE   pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  PRID_WITH_ATTRIBUTE   pRids,
    IN OUT PDWORD             pdwSize
    );


static
NTSTATUS
NetrAllocateChallengeResponse(
    OUT PVOID         pOut,
    IN OUT PDWORD     pdwOffset,
    IN OUT PDWORD     pdwSpaceLeft,
    IN  PBYTE         pResponse,
    IN  DWORD         dwResponseLen,
    IN OUT PDWORD     pdwSize
    );


static
NTSTATUS
NetrAllocateSamInfo2(
    OUT NetrSamInfo2    *pOut,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  NetrSamInfo2    *pIn,
    IN OUT PDWORD        pdwSize
    );


static
NTSTATUS
NetrInitSamBaseInfo(
    OUT NetrSamBaseInfo *pOut,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  NetrSamBaseInfo *pIn,
    IN OUT PDWORD        pdwSize
    );


static
NTSTATUS
NetrAllocateSamInfo3(
    OUT NetrSamInfo3    *pOut,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  NetrSamInfo3    *pIn,
    IN OUT PDWORD        pdwSize
    );


static
NTSTATUS
NetrAllocateSidAttr(
    OUT NetrSidAttr  *pOut,
    IN OUT PDWORD     pdwOffset,
    IN OUT PDWORD     pdwSpaceLeft,
    IN  NetrSidAttr  *pIn,
    IN OUT PDWORD     pdwSize
    );


static
NTSTATUS
NetrAllocatePacInfo(
    OUT NetrPacInfo  *pOut,
    IN OUT PDWORD     pdwOffset,
    IN OUT PDWORD     pdwSpaceLeft,
    IN  NetrPacInfo  *pIn,
    IN OUT PDWORD     pdwSize
    );


static
NTSTATUS
NetrAllocateSamInfo6(
    OUT NetrSamInfo6  *pOut,
    IN OUT PDWORD      pdwOffset,
    IN OUT PDWORD      pdwSpaceLeft,
    IN  NetrSamInfo6  *pIn,
    IN OUT PDWORD      pdwSize
    );


static
NTSTATUS
NetrAllocateDomainInfo1(
    OUT NetrDomainInfo1  *pOut,
    IN OUT PDWORD         pdwOffset,
    IN OUT PDWORD         pdwSpaceLeft,
    IN  NetrDomainInfo1  *pIn,
    IN OUT PDWORD         pdwSize
    );


static
NTSTATUS
NetrAllocateDomainTrustInfo(
    OUT NetrDomainTrustInfo *pOut,
    IN OUT PDWORD            pdwOffset,
    IN OUT PDWORD            pdwSpaceLeft,
    IN  NetrDomainTrustInfo *pIn,
    IN OUT PDWORD            pdwSize
    );


NTSTATUS
NetrAllocateMemory(
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
NetrFreeMemory(
    IN PVOID pPtr
    )
{
    free(pPtr);
}


/*
 * Type specific functions
 */

NTSTATUS
NetrAllocateDomainTrusts(
    OUT NetrDomainTrust      *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  NetrDomainTrustList  *pIn,
    IN OUT PDWORD             pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    UINT32 i = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    for (i = 0; i < pIn->count; i++)
    {
        LWBUF_ALLOC_PWSTR(pBuffer, pIn->array[i].netbios_name);
        LWBUF_ALLOC_PWSTR(pBuffer, pIn->array[i].dns_name);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->array[i].trust_flags);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->array[i].parent_index);
        LWBUF_ALLOC_WORD(pBuffer, pIn->array[i].trust_type);
        LWBUF_ALIGN_TYPE(pdwOffset, pdwSize, pdwSpaceLeft, DWORD);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->array[i].trust_attrs);
        LWBUF_ALLOC_PSID(pBuffer, pIn->array[i].sid);
        LWBUF_ALLOC_BLOB(pBuffer,
                         sizeof(pIn->array[i].guid),
                         &(pIn->array[i].guid));
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
NetrInitIdentityInfo(
    OUT PVOID            *pIdentity,
    IN OUT PDWORD         pdwOffset,
    IN OUT PDWORD         pdwSpaceLeft,
    IN  PCWSTR            pwszDomain,
    IN  PCWSTR            pwszWorkstation,
    IN  PCWSTR            pwszAccount,
    IN  UINT32            ParamControl,
    IN  UINT32            LogonIdLow,
    IN  UINT32            LogonIdHigh,
    IN OUT PDWORD         pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pIdentity;
    PWSTR pwszNbtWorkstation = NULL;
    size_t sNbtWorkstationLen = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pwszWorkstation, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccount, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    /*
     * Create "\\WORKSTATION" name
     */
    dwError = LwWc16sLen(pwszWorkstation, &sNbtWorkstationLen);
    BAIL_ON_WIN_ERROR(dwError);

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pwszNbtWorkstation),
                                  sizeof(WCHAR) * (sNbtWorkstationLen  + 3));
    BAIL_ON_NT_STATUS(ntStatus);

    if (sw16printfw(
            pwszNbtWorkstation,
            sNbtWorkstationLen + 3,
            L"\\\\%ws",
            pwszWorkstation) < 0)
    {
        ntStatus = ErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWBUF_ALLOC_UNICODE_STRING_FROM_WC16STR(pBuffer, pwszDomain);
    LWBUF_ALLOC_DWORD(pBuffer, ParamControl);
    LWBUF_ALLOC_DWORD(pBuffer, LogonIdLow);
    LWBUF_ALLOC_DWORD(pBuffer, LogonIdHigh);
    LWBUF_ALLOC_UNICODE_STRING_FROM_WC16STR(pBuffer, pwszAccount);
    LWBUF_ALLOC_UNICODE_STRING_FROM_WC16STR(pBuffer, pwszNbtWorkstation);

cleanup:
    if (pwszNbtWorkstation)
    {
        NetrFreeMemory(pwszNbtWorkstation);
    }

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
NetrAllocateLogonPasswordInfo(
    OUT NetrPasswordInfo  *pOut,
    IN OUT PDWORD          pdwOffset,
    IN OUT PDWORD          pdwSpaceLeft,
    IN  PCWSTR             pwszDomain,
    IN  PCWSTR             pwszWorkstation,
    IN  PCWSTR             pwszAccount,
    IN  PCWSTR             pwszPassword,
    IN  NetrCredentials   *pCreds,
    IN OUT PDWORD          pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    BYTE LmHash[16] = {0};
    BYTE NtHash[16] = {0};
    RC4_KEY RC4Key = {0};

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    /* pwszDomain can be NULL */
    BAIL_ON_INVALID_PTR(pwszAccount, ntStatus);
    BAIL_ON_INVALID_PTR(pwszWorkstation, ntStatus);
    BAIL_ON_INVALID_PTR(pwszPassword, ntStatus);
    BAIL_ON_INVALID_PTR(pCreds, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    /* Create password hashes (NT and LM) */
    NetrGetLmHash(LmHash, pwszPassword);
    NetrGetNtHash(NtHash, pwszPassword);

    RC4_set_key(&RC4Key,
                sizeof(pCreds->session_key),
                pCreds->session_key);
    RC4(&RC4Key, sizeof(NtHash), NtHash, NtHash);
    RC4(&RC4Key, sizeof(LmHash), LmHash, LmHash);

    ntStatus = NetrInitIdentityInfo(pBuffer,
                                    pdwOffset,
                                    pdwSpaceLeft,
                                    pwszDomain,
                                    pwszWorkstation,
                                    pwszAccount,
                                    0,
                                    0,
                                    0,
                                    pdwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    LWBUF_ALLOC_HASH_PASSWORD(pBuffer, LmHash);
    LWBUF_ALLOC_HASH_PASSWORD(pBuffer, NtHash);

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
NetrAllocateLogonNetworkInfo(
    OUT NetrNetworkInfo  *pOut,
    IN OUT PDWORD         pdwOffset,
    IN OUT PDWORD         pdwSpaceLeft,
    IN  PCWSTR            pwszDomain,
    IN  PCWSTR            pwszWorkstation,
    IN  PCWSTR            pwszAccount,
    IN  PBYTE             pChallenge,
    IN  PBYTE             pLmResp,
    IN  UINT32            LmRespLen,
    IN  PBYTE             pNtResp,
    IN  UINT32            NtRespLen,
    IN OUT PDWORD         pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    /* pwszDomain can be NULL */
    BAIL_ON_INVALID_PTR(pwszAccount, ntStatus);
    BAIL_ON_INVALID_PTR(pwszWorkstation, ntStatus);
    BAIL_ON_INVALID_PTR(pChallenge, ntStatus);
    /* LanMan Response can be NULL */
    BAIL_ON_INVALID_PTR(pNtResp, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    ntStatus = NetrInitIdentityInfo(pBuffer,
                                    pdwOffset,
                                    pdwSpaceLeft,
                                    pwszDomain,
                                    pwszWorkstation,
                                    pwszAccount,
                                    MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT | MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT,
                                    0,
                                    0,
                                    pdwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    LWBUF_ALLOC_BLOB(pBuffer,
                     sizeof(pOut->challenge),
                     pChallenge);

    /* Allocate NT Response */
    ntStatus = NetrAllocateChallengeResponse(pBuffer,
                                             pdwOffset,
                                             pdwSpaceLeft,
                                             pNtResp,
                                             NtRespLen,
                                             pdwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Allocate optional LM Response */
    ntStatus = NetrAllocateChallengeResponse(pBuffer,
                                             pdwOffset,
                                             pdwSpaceLeft,
                                             pLmResp,
                                             LmRespLen,
                                             pdwSize);
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
NetrAllocateChallengeResponse(
    OUT PVOID         pOut,
    IN OUT PDWORD     pdwOffset,
    IN OUT PDWORD     pdwSpaceLeft,
    IN  PBYTE         pResponse,
    IN  DWORD         dwResponseLen,
    IN OUT PDWORD     pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    PVOID pRespData = NULL;
    PBYTE *ppRespData = NULL;
    DWORD dwResponseSize = dwResponseLen;
    DWORD dwResponseSpaceLeft = 0;
    DWORD dwResponseOffset = 0;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    LWBUF_ALLOC_WORD(pBuffer, dwResponseLen);
    LWBUF_ALLOC_WORD(pBuffer, dwResponseLen);
    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwResponseSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pResponse)
        {
            pRespData = LWBUF_TARGET_PTR(pBuffer, dwResponseSize, pdwSpaceLeft);

            /* sanity check - the data pointer and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(PBYTE, pRespData, dwError);

            dwResponseSpaceLeft = dwResponseSize;
            dwResponseOffset    = 0;

            dwError = LwBufferAllocFixedBlob(pRespData,
                                             &dwResponseOffset,
                                             &dwResponseSpaceLeft,
                                             pResponse,
                                             dwResponseSize,
                                             pdwSize);
            BAIL_ON_WIN_ERROR(dwError);
        }

        ppRespData       = (PBYTE*)pCursor;
        *ppRespData      = pRespData;
        (*pdwSpaceLeft) -= (pRespData) ? LWBUF_ALIGN_SIZE(dwResponseSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft)  -= sizeof(PBYTE);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwResponseSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(PBYTE);
    (*pdwSize)   += sizeof(PBYTE);

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
NetrAllocateSamInfo2(
    OUT NetrSamInfo2    *pOut,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  NetrSamInfo2    *pIn,
    IN OUT PDWORD        pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    ntStatus = NetrInitSamBaseInfo(pBuffer,
                                   pdwOffset,
                                   pdwSpaceLeft,
                                   &pIn->base,
                                   pdwSize);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
NetrInitSamBaseInfo(
    OUT NetrSamBaseInfo *pOut,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  NetrSamBaseInfo *pIn,
    IN OUT PDWORD        pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    LWBUF_ALLOC_WINNTTIME(pBuffer, pIn->last_logon);
    LWBUF_ALLOC_WINNTTIME(pBuffer, pIn->last_logoff);
    LWBUF_ALLOC_WINNTTIME(pBuffer, pIn->acct_expiry);
    LWBUF_ALLOC_WINNTTIME(pBuffer, pIn->last_password_change);
    LWBUF_ALLOC_WINNTTIME(pBuffer, pIn->allow_password_change);
    LWBUF_ALLOC_WINNTTIME(pBuffer, pIn->force_password_change);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->account_name);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->full_name);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->logon_script);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->profile_path);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->home_directory);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->home_drive);
    LWBUF_ALLOC_WORD(pBuffer, pIn->logon_count);
    LWBUF_ALLOC_WORD(pBuffer, pIn->bad_password_count);
    LWBUF_ALLOC_DWORD(pBuffer, pIn->rid);
    LWBUF_ALLOC_DWORD(pBuffer, pIn->primary_gid);

    ntStatus = NetrAllocateRidWithAttributeArray(pBuffer,
                                                 pdwOffset,
                                                 pdwSpaceLeft,
                                                 &pIn->groups,
                                                 pdwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->user_flags);
    LWBUF_ALLOC_SESSION_KEY(pBuffer, &pIn->key);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->logon_server);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->domain);
    LWBUF_ALLOC_PSID(pBuffer, pIn->domain_sid);
    LWBUF_ALLOC_SESSION_KEY(pBuffer, &pIn->lmkey);
    LWBUF_ALLOC_DWORD(pBuffer, pIn->acct_flags);
    LWBUF_ALLOC_BLOB(pBuffer, sizeof(pIn->unknown), pIn->unknown);

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
NetrAllocateRidWithAttributeArray(
    OUT PVOID                      *pOut,
    IN OUT PDWORD                   pdwOffset,
    IN OUT PDWORD                   pdwSpaceLeft,
    IN  PRID_WITH_ATTRIBUTE_ARRAY   pIn,
    IN OUT PDWORD                   pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    DWORD dwRidsSize = 0;
    DWORD dwRidsSpaceLeft = 0;
    DWORD dwRidsOffset = 0;
    DWORD iRid = 0;
    PVOID pRids = NULL;
    PRID_WITH_ATTRIBUTE *ppRids = NULL;
    
    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->dwCount);
    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pIn->dwCount)
    {
        dwRidsSize = sizeof(pIn->pRids[0]) * pIn->dwCount;
    }

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwRidsSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->pRids)
        {
            pRids = LWBUF_TARGET_PTR(pBuffer, dwRidsSize, pdwSpaceLeft);

            /* sanity check - the rids pointer and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(PRID_WITH_ATTRIBUTE, pRids, dwError);

            dwRidsSpaceLeft = dwRidsSize;
            dwRidsOffset    = 0;

            /* Allocate the rid entries */
            for (iRid = 0; iRid < pIn->dwCount; iRid++)
            {
                PVOID pRidCursor = pRids + (iRid * sizeof(pIn->pRids[0]));

                ntStatus = NetrAllocateRidWithAttribute(pRidCursor,
                                                        &dwRidsOffset,
                                                        &dwRidsSpaceLeft,
                                                        &(pIn->pRids[iRid]),
                                                        pdwSize);
                BAIL_ON_NT_STATUS(ntStatus);

                dwRidsOffset = 0;
            }
        }

        ppRids           = (PRID_WITH_ATTRIBUTE*)pCursor;
        *ppRids          = (PRID_WITH_ATTRIBUTE)pRids;
        (*pdwSpaceLeft) -= (pRids) ? LWBUF_ALIGN_SIZE(dwRidsSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft)  -= sizeof(PRID_WITH_ATTRIBUTE);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwRidsSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(PRID_WITH_ATTRIBUTE);
    (*pdwSize)   += sizeof(PRID_WITH_ATTRIBUTE);

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
NetrAllocateRidWithAttribute(
    OUT PRID_WITH_ATTRIBUTE  pOut,
    IN OUT PDWORD            pdwOffset,
    IN OUT PDWORD            pdwSpaceLeft,
    IN  PRID_WITH_ATTRIBUTE  pRids,
    IN OUT PDWORD            pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pRids, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALLOC_DWORD(pBuffer, pRids->dwRid);
    LWBUF_ALLOC_DWORD(pBuffer, pRids->dwAttributes);

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
NetrAllocateSamInfo3(
    OUT NetrSamInfo3    *pOut,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  NetrSamInfo3    *pIn,
    IN OUT PDWORD        pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    UINT32 iSid = 0;
    DWORD dwSidsOffset = 0;
    DWORD dwSidsSize = 0;
    DWORD dwSidsSpaceLeft = 0;
    PVOID pSids = NULL;
    NetrSidAttr **ppSids = NULL;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    ntStatus = NetrInitSamBaseInfo((NetrSamBaseInfo*)pBuffer,
                                   pdwOffset,
                                   pdwSpaceLeft,
                                   &pIn->base,
                                   pdwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->sidcount);
    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pIn->sidcount)
    {
        for (iSid = 0; iSid < pIn->sidcount; iSid++)
        {
            ntStatus = NetrAllocateSidAttr(NULL,
                                           &dwSidsOffset,
                                           NULL,
                                           &(pIn->sids[iSid]),
                                           &dwSidsSize);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwSidsSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->sids)
        {
            pSids = LWBUF_TARGET_PTR(pBuffer, dwSidsSize, pdwSpaceLeft);

            /* sanity check - the sids pointer and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(NetrSidAttr*, pSids, dwError);

            dwSidsSpaceLeft = dwSidsSize;
            dwSidsOffset    = 0;

            /* Allocate the sid entries */
            for (iSid = 0; iSid < pIn->sidcount; iSid++)
            {
                PVOID pSidCursor = pSids + (iSid * sizeof(pIn->sids[0]));

                ntStatus = NetrAllocateSidAttr(pSidCursor,
                                               &dwSidsOffset,
                                               &dwSidsSpaceLeft,
                                               &(pIn->sids[iSid]),
                                               pdwSize);
                BAIL_ON_NT_STATUS(ntStatus);

                dwSidsOffset = 0;
            }
        }

        ppSids           = (NetrSidAttr**)pCursor;
        *ppSids          = (NetrSidAttr*)pSids;
        (*pdwSpaceLeft) -= (pSids) ? LWBUF_ALIGN_SIZE(dwSidsSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft)  -= sizeof(NetrSidAttr*);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwSidsSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(NetrSidAttr*);
    (*pdwSize)   += sizeof(NetrSidAttr*);

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
NetrAllocateSidAttr(
    OUT NetrSidAttr  *pOut,
    IN OUT PDWORD     pdwOffset,
    IN OUT PDWORD     pdwSpaceLeft,
    IN  NetrSidAttr  *pIn,
    IN OUT PDWORD     pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);
    
    LWBUF_ALLOC_PSID(pBuffer, pIn->sid);
    LWBUF_ALLOC_DWORD(pBuffer, pIn->attribute);
    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

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
NetrAllocatePacInfo(
    OUT NetrPacInfo  *pOut,
    IN OUT PDWORD     pdwOffset,
    IN OUT PDWORD     pdwSpaceLeft,
    IN  NetrPacInfo  *pIn,
    IN OUT PDWORD     pdwSize
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


static
NTSTATUS
NetrAllocateSamInfo6(
    OUT NetrSamInfo6  *pOut,
    IN OUT PDWORD      pdwOffset,
    IN OUT PDWORD      pdwSpaceLeft,
    IN  NetrSamInfo6  *pIn,
    IN OUT PDWORD      pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    ntStatus = NetrAllocateSamInfo3((NetrSamInfo3*)pBuffer,
                                    pdwOffset,
                                    pdwSpaceLeft,
                                    (NetrSamInfo3*)pIn,
                                    pdwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->forest);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer, (PUNICODE_STRING)&pIn->principal);
    LWBUF_ALLOC_BLOB(pBuffer, sizeof(pIn->unknown), pIn->unknown);

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
NetrAllocateValidationInfo(
    OUT NetrValidationInfo  *pOut,
    IN OUT PDWORD            pdwOffset,
    IN OUT PDWORD            pdwSpaceLeft,
    IN  WORD                 swLevel,
    IN  NetrValidationInfo  *pIn,
    IN OUT PDWORD            pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    DWORD dwInfoOffset = 0;
    DWORD dwInfoSize = 0;
    DWORD dwInfoSpaceLeft = 0;
    PVOID pInfo = NULL;
    PVOID *ppInfo = NULL;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    /*
     * pIn->sam2 is essentially the same pointer as other legs of the union
     * so the following condition checks if anything has been passed at all
     */
    if (pIn->sam2)
    {
        switch (swLevel)
        {
        case 2:
            ntStatus = NetrAllocateSamInfo2(NULL,
                                            &dwInfoOffset,
                                            NULL,
                                            pIn->sam2,
                                            &dwInfoSize);
            break;

        case 3:
            ntStatus = NetrAllocateSamInfo3(NULL,
                                            &dwInfoOffset,
                                            NULL,
                                            pIn->sam3,
                                            &dwInfoSize);
            break;

        case 4:
            ntStatus = NetrAllocatePacInfo(NULL,
                                           &dwInfoOffset,
                                           NULL,
                                           pIn->pac4,
                                           &dwInfoSize);
            break;

        case 5:
            ntStatus = NetrAllocatePacInfo(NULL,
                                           &dwInfoOffset,
                                           NULL,
                                           pIn->pac5,
                                           &dwInfoSize);
            break;

        case 6:
            ntStatus = NetrAllocateSamInfo6(NULL,
                                            &dwInfoOffset,
                                            NULL,
                                            pIn->sam6,
                                            &dwInfoSize);
            break;

        default:
            ntStatus = STATUS_INVALID_LEVEL;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwInfoSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->sam2)
        {
            pInfo = LWBUF_TARGET_PTR(pBuffer, dwInfoSize, pdwSpaceLeft);

            /* sanity check - the info pointer and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(PVOID, pInfo, dwError);

            dwInfoSpaceLeft = dwInfoSize;
            dwInfoOffset    = 0;

            /* Allocate the info */
            switch (swLevel)
            {
            case 2:
                ntStatus = NetrAllocateSamInfo2(pInfo,
                                                &dwInfoOffset,
                                                &dwInfoSpaceLeft,
                                                pIn->sam2,
                                                pdwSize);
                break;

            case 3:
                ntStatus = NetrAllocateSamInfo3(pInfo,
                                                &dwInfoOffset,
                                                &dwInfoSpaceLeft,
                                                pIn->sam3,
                                                pdwSize);
                break;

            case 4:
                ntStatus = NetrAllocatePacInfo(pInfo,
                                               &dwInfoOffset,
                                               &dwInfoSpaceLeft,
                                               pIn->pac4,
                                               pdwSize);
                break;

            case 5:
                ntStatus = NetrAllocatePacInfo(pInfo,
                                               &dwInfoOffset,
                                               &dwInfoSpaceLeft,
                                               pIn->pac5,
                                               pdwSize);
                break;

            case 6:
                ntStatus = NetrAllocateSamInfo6(pInfo,
                                                &dwInfoOffset,
                                                &dwInfoSpaceLeft,
                                                pIn->sam6,
                                                pdwSize);
                break;

            default:
                ntStatus = STATUS_INVALID_LEVEL;
            }
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ppInfo           = pCursor;
        *ppInfo          = pInfo;
        (*pdwSpaceLeft) -= (pInfo) ? LWBUF_ALIGN_SIZE(dwInfoSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft)  -= sizeof(PVOID);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwInfoSize);
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
NetrAllocateDomainInfo(
    OUT NetrDomainInfo  *pOut,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN  WORD             swLevel,
    IN  NetrDomainInfo  *pIn,
    IN OUT PDWORD        pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    DWORD dwInfoSize = 0;
    DWORD dwInfoOffset = 0;
    DWORD dwInfoSpaceLeft = 0;
    PVOID pInfo = NULL;
    NetrDomainInfo1 **ppInfo = NULL;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pIn->info1)
    {
        switch (swLevel)
        {
        case 1:
            ntStatus = NetrAllocateDomainInfo1(NULL,
                                               &dwInfoOffset,
                                               NULL,
                                               pIn->info1,
                                               &dwInfoSize);
            break;

        case 2:
            ntStatus = NetrAllocateDomainInfo1(NULL,
                                               &dwInfoOffset,
                                               NULL,
                                               pIn->info2,
                                               &dwInfoSize);
            break;

        default:
            ntStatus = STATUS_INVALID_LEVEL;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwInfoSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->info1)
        {
            pInfo = LWBUF_TARGET_PTR(pBuffer, dwInfoSize, pdwSpaceLeft);

            /* sanity check - the rids pointer and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(NetrDomainInfo1*, pInfo, dwError);

            dwInfoSpaceLeft = dwInfoSize;
            dwInfoOffset    = 0;

            switch (swLevel)
            {
            case 1:
                ntStatus = NetrAllocateDomainInfo1(pBuffer,
                                                   &dwInfoOffset,
                                                   &dwInfoSpaceLeft,
                                                   pIn->info1,
                                                   pdwSize);
                break;

            case 2:
                ntStatus = NetrAllocateDomainInfo1(pBuffer,
                                                   &dwInfoOffset,
                                                   &dwInfoSpaceLeft,
                                                   pIn->info2,
                                                   pdwSize);
                break;

            default:
                ntStatus = STATUS_INVALID_LEVEL;
            }
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ppInfo           = (NetrDomainInfo1**)pCursor;
        *ppInfo          = (NetrDomainInfo1*)pInfo;
        (*pdwSpaceLeft) -= (pInfo) ? LWBUF_ALIGN_SIZE(dwInfoSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft)  -= sizeof(NetrDomainInfo1*);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwInfoSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(NetrDomainInfo1*);
    (*pdwSize)   += sizeof(NetrDomainInfo1*);

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
NetrAllocateDomainInfo1(
    OUT NetrDomainInfo1  *pOut,
    IN OUT PDWORD         pdwOffset,
    IN OUT PDWORD         pdwSpaceLeft,
    IN  NetrDomainInfo1  *pIn,
    IN OUT PDWORD         pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;
    PVOID pCursor = NULL;
    DWORD iTrust = 0;
    DWORD dwTrustsSize = 0;
    DWORD dwTrustsOffset = 0;
    DWORD dwTrustsSpaceLeft = 0;
    PVOID pTrusts = NULL;
    NetrDomainTrustInfo **ppTrusts = NULL;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    ntStatus = NetrAllocateDomainTrustInfo(pBuffer,
                                           pdwOffset,
                                           pdwSpaceLeft,
                                           &pIn->domain_info,
                                           pdwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    LWBUF_ALLOC_DWORD(pBuffer, pIn->num_trusts);
    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    if (pIn->num_trusts)
    {
        for (iTrust = 0; iTrust < pIn->num_trusts; iTrust++)
        {
            ntStatus = NetrAllocateDomainTrustInfo(NULL,
                                                   &dwTrustsOffset,
                                                   NULL,
                                                   &(pIn->trusts[iTrust]),
                                                   &dwTrustsSize);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (pBuffer && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwTrustsSize, pdwSpaceLeft, dwError);
        pCursor = pBuffer + (*pdwOffset);

        if (pIn->trusts)
        {
            pTrusts = LWBUF_TARGET_PTR(pBuffer, dwTrustsSize, pdwSpaceLeft);

            /* sanity check - the rids pointer and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(NetrDomainTrustInfo*, pTrusts, dwError);

            dwTrustsSpaceLeft = dwTrustsSize;
            dwTrustsOffset    = 0;

            /* Allocate the trust entries */
            for (iTrust = 0; iTrust < pIn->num_trusts; iTrust++)
            {
                PVOID pTrustCursor = pTrusts +
                                     (iTrust * sizeof(NetrDomainTrustInfo));

                ntStatus = NetrAllocateDomainTrustInfo(pTrustCursor,
                                                       &dwTrustsOffset,
                                                       &dwTrustsSpaceLeft,
                                                       &(pIn->trusts[iTrust]),
                                                       pdwSize);
                BAIL_ON_NT_STATUS(ntStatus);

                dwTrustsOffset = 0;
            }
        }

        ppTrusts         = (NetrDomainTrustInfo**)pCursor;
        *ppTrusts        = (NetrDomainTrustInfo*)pTrusts;
        (*pdwSpaceLeft) -= (pTrusts) ? LWBUF_ALIGN_SIZE(dwTrustsSize) : 0;

        /* recalculate space after setting the pointer */
        (*pdwSpaceLeft)  -= sizeof(NetrDomainTrustInfo*);
    }
    else
    {
        (*pdwSize) += LWBUF_ALIGN_SIZE(dwTrustsSize);
    }

    /* include size of the pointer */
    (*pdwOffset) += sizeof(NetrDomainTrustInfo*);
    (*pdwSize)   += sizeof(NetrDomainTrustInfo*);

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
NetrAllocateDomainTrustInfo(
    OUT NetrDomainTrustInfo *pOut,
    IN OUT PDWORD            pdwOffset,
    IN OUT PDWORD            pdwSpaceLeft,
    IN  NetrDomainTrustInfo *pIn,
    IN OUT PDWORD            pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    NetrDomainTrustInfo *pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALIGN(pdwOffset, pdwSize, pdwSpaceLeft);

    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->domain_name);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->full_domain_name);
    LWBUF_ALLOC_UNICODE_STRING(pBuffer,
                               (PUNICODE_STRING)&pIn->forest);
    LWBUF_ALLOC_BLOB(pBuffer, sizeof(pIn->guid), &pIn->guid);
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
NetrAllocateDcNameInfo(
    OUT DsrDcNameInfo  *pOut,
    IN OUT PDWORD       pdwOffset,
    IN OUT PDWORD       pdwSpaceLeft,
    IN  DsrDcNameInfo  *pIn,
    IN OUT PDWORD       pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DsrDcNameInfo *pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALLOC_PWSTR(pBuffer, pIn->dc_name);
    LWBUF_ALLOC_PWSTR(pBuffer, pIn->dc_address);
    LWBUF_ALLOC_WORD(pBuffer, pIn->address_type);
    LWBUF_ALLOC_BLOB(pBuffer, sizeof(pIn->domain_guid), &pIn->domain_guid);
    LWBUF_ALLOC_PWSTR(pBuffer, pIn->domain_name);
    LWBUF_ALLOC_PWSTR(pBuffer, pIn->forest_name);
    LWBUF_ALLOC_DWORD(pBuffer, pIn->flags);
    LWBUF_ALLOC_PWSTR(pBuffer, pIn->dc_site_name);
    LWBUF_ALLOC_PWSTR(pBuffer, pIn->cli_site_name);

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
