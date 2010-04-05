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
 *        wkssvc_memeory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        WksSvc memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
WkssSrvAllocateMemory(
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


VOID
WkssSrvFreeMemory(
    PVOID pPtr
    )
{
    rpc_ss_free(pPtr);
}


NTSTATUS
WkssSrvAllocateSidFromWC16String(
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
    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&pSidCopy),
                                     ulSidSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(ulSidSize, pSidCopy, pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSid = pSidCopy;

cleanup:
    RTL_FREE(&pSid);

    return ntStatus;

error:
    if (pSidCopy)
    {
        WkssSrvFreeMemory(pSidCopy);
    }

    *ppSid = NULL;
    goto cleanup;
}


NTSTATUS
WkssSrvDuplicateSid(
    PSID *ppSidOut,
    PSID pSidIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;

    ulSidSize = RtlLengthSid(pSidIn);
    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&pSid),
                                     ulSidSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(ulSidSize, pSid, pSidIn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSidOut = pSid;

cleanup:
    return ntStatus;

error:
    if (pSid)
    {
        WkssSrvFreeMemory(pSid);
    }

    *ppSidOut = NULL;
    goto cleanup;
}


NTSTATUS
WkssSrvDuplicateWC16String(
    PWSTR *ppwszOut,
    PWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszStr = NULL;
    size_t sLen = 0;

    dwError = LwWc16sLen(pwszIn, &sLen);
    BAIL_ON_LW_ERROR(dwError);

    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&pwszStr),
                                     sizeof(pwszStr[0]) * (sLen + 1));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16snCpy(pwszStr, pwszIn, sLen);
    BAIL_ON_LW_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pwszStr)
    {
        WkssSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
WkssSrvGetFromUnicodeString(
    PWSTR *ppwszOut,
    UnicodeString *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&pwszStr),
                                     (pIn->size + 1) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16snCpy(pwszStr, pIn->string, (pIn->len / sizeof(WCHAR)));
    BAIL_ON_LW_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pwszStr)
    {
        WkssSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
WkssSrvGetFromUnicodeStringEx(
    PWSTR *ppwszOut,
    UnicodeStringEx *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&pwszStr),
                                     (pIn->size) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16snCpy(pwszStr, pIn->string, (pIn->len / sizeof(WCHAR)));
    BAIL_ON_LW_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pwszStr)
    {
        WkssSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
WkssSrvInitUnicodeString(
    UnicodeString *pOut,
    PCWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sLen = 0;
    size_t sSize = 0;

    if (pwszIn)
    {
        dwError = LwWc16sLen(pwszIn, &sLen);
        BAIL_ON_LW_ERROR(dwError);
    }

    sSize = sLen * sizeof(WCHAR);

    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&(pOut->string)),
                                     sSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->string, pwszIn, sSize);
    pOut->size = sSize;
    pOut->len  = sSize;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pOut->string)
    {
        WkssSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
    goto cleanup;
}


NTSTATUS
WkssSrvInitUnicodeStringEx(
    UnicodeStringEx *pOut,
    PCWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sLen = 0;
    size_t sSize = 0;

    if (pwszIn)
    {
        dwError = LwWc16sLen(pwszIn, &sLen);
        BAIL_ON_LW_ERROR(dwError);
    }

    sSize = (sLen + 1) * sizeof(WCHAR);

    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&(pOut->string)),
                                     sSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->string, pwszIn, sSize - sizeof(WCHAR));
    pOut->size = sSize;
    pOut->len  = sSize - sizeof(WCHAR);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pOut->string)
    {
        WkssSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
    goto cleanup;
}


NTSTATUS
WkssSrvDuplicateUnicodeString(
    UnicodeString *pOut,
    UnicodeString *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = pIn->len;
    dwSize = pIn->size;

    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&(pOut->string)),
                                     dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->string, pIn->string, dwLen);
    pOut->size = dwSize;
    pOut->len  = dwLen;

cleanup:
    return ntStatus;

error:
    if (pOut->string)
    {
        WkssSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
    goto cleanup;
}


NTSTATUS
WkssSrvSidAppendRid(
    PSID *ppOutSid,
    PSID pInSid,
    DWORD dwRid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwSidLen = 0;
    PSID pSid = NULL;

    dwSidLen = RtlLengthRequiredSid(pInSid->SubAuthorityCount + 1);
    ntStatus = WkssSrvAllocateMemory(OUT_PPVOID(&pSid), dwSidLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(dwSidLen, pSid, pInSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlAppendRidSid(dwSidLen, pSid, dwRid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppOutSid = pSid;

cleanup:
    return ntStatus;

error:
    if (pSid)
    {
        WkssSrvFreeMemory(pSid);
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
