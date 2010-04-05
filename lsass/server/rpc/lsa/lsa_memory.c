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
 *        lsa_memeory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    void *pOut = NULL;

    pOut = rpc_ss_allocate(dwSize);
    BAIL_ON_NO_MEMORY(pOut);

    memset(pOut, 0, dwSize);

    *ppOut = pOut;

cleanup:
    return ntStatus;

error:
    *ppOut = NULL;
    goto cleanup;
}


void
LsaSrvFreeMemory(
    void *pPtr
    )
{
    rpc_ss_free(pPtr);
}


NTSTATUS
LsaSrvAllocateSidFromWC16String(
    PSID *ppSid,
    PCWSTR pwszSidStr
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;
    PSID pSidCopy = NULL;

    ntStatus = RtlAllocateSidFromWC16String(&pSid,
                                          pwszSidStr);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ulSidSize = RtlLengthSid(pSid);
    ntStatus = LsaSrvAllocateMemory((void**)&pSidCopy,
                                  ulSidSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(ulSidSize, pSidCopy, pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSid = pSidCopy;

cleanup:
    if (pSid) {
        RTL_FREE(&pSid);
    }

    return ntStatus;

error:
    if (pSidCopy) {
        LsaSrvFreeMemory(pSidCopy);
    }

    *ppSid = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvDuplicateSid(
    PSID *ppSidOut,
    PSID pSidIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;

    ulSidSize = RtlLengthSid(pSidIn);
    ntStatus = LsaSrvAllocateMemory((void**)&pSid,
                                  ulSidSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(ulSidSize, pSid, pSidIn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSidOut = pSid;

cleanup:
    return ntStatus;

error:
    if (pSid) {
        LsaSrvFreeMemory(pSid);
    }

    *ppSidOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvDuplicateWC16String(
    PWSTR *ppwszOut,
    PWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;
    DWORD dwLen = 0;

    dwLen = wc16slen(pwszIn);
    ntStatus = LsaSrvAllocateMemory((void**)&pwszStr,
                                  (dwLen + 1) * sizeof(*pwszStr));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszStr, pwszIn, dwLen);

    *ppwszOut = pwszStr;

cleanup:
    return ntStatus;

error:
    if (pwszStr) {
        LsaSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvGetFromUnicodeString(
    PWSTR *ppwszOut,
    UnicodeString *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = LsaSrvAllocateMemory((void**)&pwszStr,
                                  (pIn->size + 1) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszStr, pIn->string, (pIn->len / sizeof(WCHAR)));
    *ppwszOut = pwszStr;

cleanup:
    return ntStatus;

error:
    if (pwszStr) {
        LsaSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvGetFromUnicodeStringEx(
    PWSTR *ppwszOut,
    UnicodeStringEx *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = LsaSrvAllocateMemory((void**)&pwszStr,
                                  (pIn->size) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszStr, pIn->string, (pIn->len / sizeof(WCHAR)));
    *ppwszOut = pwszStr;

cleanup:
    return ntStatus;

error:
    if (pwszStr) {
        LsaSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvInitUnicodeString(
    UnicodeString *pOut,
    PCWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = (pwszIn) ? wc16slen(pwszIn) : 0;
    dwSize = dwLen * sizeof(WCHAR);

    ntStatus = LsaSrvAllocateMemory((void**)&(pOut->string),
                                  dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->string, pwszIn, dwSize);
    pOut->size = dwSize;
    pOut->len  = dwSize;

cleanup:
    return ntStatus;

error:
    if (pOut->string) {
        LsaSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
    goto cleanup;
}


NTSTATUS
LsaSrvInitUnicodeStringEx(
    UnicodeStringEx *pOut,
    PCWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = (pwszIn) ? wc16slen(pwszIn) : 0;
    dwSize = (dwLen + 1) * sizeof(WCHAR);

    ntStatus = LsaSrvAllocateMemory((void**)&(pOut->string),
                                  dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->string, pwszIn, dwSize - sizeof(WCHAR));
    pOut->size = dwSize;
    pOut->len  = dwSize - sizeof(WCHAR);

cleanup:
    return ntStatus;

error:
    if (pOut->string) {
        LsaSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
    goto cleanup;
}


NTSTATUS
LsaSrvDuplicateUnicodeString(
    UnicodeString *pOut,
    UnicodeString *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = pIn->len;
    dwSize = pIn->size;

    ntStatus = LsaSrvAllocateMemory((void**)&(pOut->string),
                                  dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->string, pIn->string, dwLen);
    pOut->size = dwSize;
    pOut->len  = dwLen;

cleanup:
    return ntStatus;

error:
    if (pOut->string) {
        LsaSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
    goto cleanup;
}


NTSTATUS
LsaSrvDuplicateUnicodeStringEx(
    UnicodeStringEx *pOut,
    UnicodeStringEx *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = pIn->len;
    dwSize = pIn->size;

    ntStatus = LsaSrvAllocateMemory((void**)&(pOut->string),
                                  dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->string, pIn->string, dwLen);
    pOut->size = dwSize;
    pOut->len  = dwLen;

cleanup:
    return ntStatus;

error:
    if (pOut->string) {
        LsaSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
    goto cleanup;
}


NTSTATUS
LsaSrvSidAppendRid(
    PSID *ppOutSid,
    PSID pInSid,
    DWORD dwRid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwSidLen = 0;
    PSID pSid = NULL;

    dwSidLen = RtlLengthRequiredSid(pInSid->SubAuthorityCount + 1);
    ntStatus = LsaSrvAllocateMemory((void**)&pSid, dwSidLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(dwSidLen, pSid, pInSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlAppendRidSid(dwSidLen, pSid, dwRid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppOutSid = pSid;

cleanup:
    return ntStatus;

error:
    if (pSid) {
        LsaSrvFreeMemory(pSid);
    }

    *ppOutSid = NULL;
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
