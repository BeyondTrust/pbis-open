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
    UNICODE_STRING *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = LsaSrvAllocateMemory((void**)&pwszStr,
                                  (pIn->MaximumLength + 1) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszStr, pIn->Buffer, (pIn->Length / sizeof(WCHAR)));
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
    UNICODE_STRING *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = LsaSrvAllocateMemory((void**)&pwszStr,
                                  (pIn->MaximumLength) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszStr, pIn->Buffer, (pIn->Length / sizeof(WCHAR)));
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
    PUNICODE_STRING pOut,
    PCWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sLen = 0;
    size_t sSize = 0;

    BAIL_ON_INVALID_POINTER(pOut);

    if (pwszIn)
    {
        dwError = LwWc16sLen(pwszIn, &sLen);
        BAIL_ON_LSA_ERROR(dwError);

        sSize = sLen * sizeof(WCHAR);

        ntStatus = LsaSrvAllocateMemory(OUT_PPVOID(&pOut->Buffer),
                                        sSize);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        memcpy(pOut->Buffer, pwszIn, sLen * sizeof(WCHAR));
    }
    else
    {
        pOut->Buffer = NULL;
    }

    pOut->MaximumLength = sSize;
    pOut->Length        = sLen * sizeof(WCHAR);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pOut->Buffer)
    {
        LsaSrvFreeMemory(pOut->Buffer);
        pOut->Buffer = NULL;
    }

    pOut->MaximumLength = 0;
    pOut->Length        = 0;

    goto cleanup;
}


NTSTATUS
LsaSrvInitUnicodeStringEx(
    PUNICODE_STRING pOut,
    PCWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sLen = 0;
    size_t sSize = 0;

    BAIL_ON_INVALID_POINTER(pOut);

    if (pwszIn)
    {
        dwError = LwWc16sLen(pwszIn, &sLen);
        BAIL_ON_LSA_ERROR(dwError);

        sSize = (sLen + 1) * sizeof(WCHAR);

        ntStatus = LsaSrvAllocateMemory(OUT_PPVOID(&pOut->Buffer),
                                        sSize);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        memcpy(pOut->Buffer, pwszIn, sLen * sizeof(WCHAR));
    }
    else
    {
        pOut->Buffer = NULL;
    }

    pOut->MaximumLength = sSize;
    pOut->Length        = sLen * sizeof(WCHAR);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pOut->Buffer)
    {
        LsaSrvFreeMemory(pOut->Buffer);
        pOut->Buffer = NULL;
    }

    pOut->MaximumLength = 0;
    pOut->Length        = 0;

    goto cleanup;
}


NTSTATUS
LsaSrvDuplicateUnicodeString(
    UNICODE_STRING *pOut,
    UNICODE_STRING *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = pIn->Length;
    dwSize = pIn->MaximumLength;

    ntStatus = LsaSrvAllocateMemory((void**)&(pOut->Buffer),
                                    dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->Buffer, pIn->Buffer, dwLen);
    pOut->MaximumLength = dwSize;
    pOut->Length        = dwLen;

cleanup:
    return ntStatus;

error:
    if (pOut->Buffer) {
        LsaSrvFreeMemory(pOut->Buffer);
    }

    pOut->MaximumLength = 0;
    pOut->Length        = 0;
    goto cleanup;
}


NTSTATUS
LsaSrvDuplicateUnicodeStringEx(
    UNICODE_STRING *pOut,
    UNICODE_STRING *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = pIn->Length;
    dwSize = pIn->MaximumLength;

    ntStatus = LsaSrvAllocateMemory((void**)&(pOut->Buffer),
                                    dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->Buffer, pIn->Buffer, dwLen);
    pOut->MaximumLength = dwSize;
    pOut->Length        = dwLen;

cleanup:
    return ntStatus;

error:
    if (pOut->Buffer) {
        LsaSrvFreeMemory(pOut->Buffer);
    }

    pOut->MaximumLength = 0;
    pOut->Length        = 0;
    goto cleanup;
}


VOID
LsaSrvFreeUnicodeString(
    PUNICODE_STRING pString
    )
{
    if (pString->Buffer)
    {
        rpc_ss_free(pString->Buffer);
    }

    pString->Length        = 0;
    pString->MaximumLength = 0;
    pString->Buffer        = NULL;
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
