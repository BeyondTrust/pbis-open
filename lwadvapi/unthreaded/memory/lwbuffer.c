/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwbuffer.c
 *
 * Abstract:
 *
 *        Memory buffer allocation functions.
 *
 *        Functions preparing allocation of a structure in flat
 *        memory buffer (returned from rpc client functions).
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


#ifndef BAIL_ON_NT_STATUS
#define BAIL_ON_NT_STATUS(status)                  \
    if ((status) != 0)                             \
    {                                              \
        goto error;                                \
    }
#endif


#define LWBUF_ALIGN_SIZE(size)                                      \
    (((size) % sizeof(PVOID)) ?                                     \
        ((size) + sizeof(PVOID) - ((size) % sizeof(PVOID))) :       \
        (size))


#define LWBUF_ALIGN_TYPE(offset, size, left, type)                      \
    {                                                                   \
        DWORD dwAlign = (offset) % sizeof(type);                        \
                                                                        \
        dwAlign   = (dwAlign) ? (sizeof(type) - dwAlign) : 0;           \
        (size)   += dwAlign;                                            \
        (offset) += dwAlign;                                            \
        (left)   -= dwAlign;                                            \
    }


#define LWBUF_ALIGN(offset, size, left)                                 \
    LWBUF_ALIGN_TYPE(offset, size, left, PVOID)


#define LWBUF_ALIGN_PTR(offset, size, left)                             \
    LWBUF_ALIGN_TYPE(offset, size, left, PVOID)


#define BAIL_IF_NOT_ENOUGH_SPACE(size, space, err)                      \
    if ((size) > (space))                                               \
    {                                                                   \
        err = ERROR_INSUFFICIENT_BUFFER;                                \
        BAIL_ON_LW_ERROR((err));                                        \
    }

#define BAIL_IF_PTR_OVERLAP(type, target_ptr, err)                      \
    if ((pCursor + sizeof(type)) > target_ptr)                          \
    {                                                                   \
        err = ERROR_INSUFFICIENT_BUFFER;                                \
        BAIL_ON_LW_ERROR(err);                                          \
    }

#define LWBUF_TARGET_PTR(buffer_ptr, target_size, space)                \
    ((pCursor = (buffer_ptr) + dwOffset),                               \
     ((pCursor + (space)) - LWBUF_ALIGN_SIZE(target_size)))



DWORD
LwBufferAllocByte(
    OUT PVOID      pBuffer,
    IN OUT PDWORD  pdwOffset,
    IN OUT PDWORD  pdwSpaceLeft,
    IN BYTE        ubSource,
    IN OUT PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PBYTE pubDest = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pBuffer)
    {
        pCursor = pBuffer + dwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwOffset += sizeof(ubSource);
    dwSize    = sizeof(ubSource);

    if (pCursor && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwSize, dwSpaceLeft, err);

        pubDest  = (PBYTE)pCursor;
        *pubDest = ubSource;

        dwSpaceLeft  -= dwSize;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
LwBufferAllocWord(
    OUT PVOID   pBuffer,
    IN OUT PDWORD  pdwOffset,
    IN OUT PDWORD  pdwSpaceLeft,
    IN WORD   swSource,
    IN OUT PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWORD pswDest = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pBuffer)
    {
        pCursor = pBuffer + dwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwOffset += sizeof(swSource);
    dwSize    = sizeof(swSource);

    if (pCursor && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwSize, dwSpaceLeft, err);

        pswDest  = (PWORD)pCursor;
        *pswDest = swSource;

        dwSpaceLeft  -= dwSize;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
LwBufferAllocDword(
    OUT PVOID   pBuffer,
    IN OUT PDWORD  pdwOffset,
    IN OUT PDWORD  pdwSpaceLeft,
    IN DWORD   dwSource,
    IN OUT PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PDWORD pdwDest = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pBuffer)
    {
        pCursor = pBuffer + dwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwOffset += sizeof(dwSource);
    dwSize    = sizeof(dwSource);

    if (pCursor && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwSize, dwSpaceLeft, err);

        pdwDest  = (PDWORD)pCursor;
        *pdwDest = dwSource;

        dwSpaceLeft  -= dwSize;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
LwBufferAllocUlong64(
    OUT PVOID   pBuffer,
    IN OUT PDWORD  pdwOffset,
    IN OUT PDWORD  pdwSpaceLeft,
    IN ULONG64   ullSource,
    IN OUT PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PULONG64 pullDest = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pBuffer)
    {
        pCursor = pBuffer + dwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwOffset += sizeof(ullSource);
    dwSize    = sizeof(ullSource);

    if (pCursor && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwSize, dwSpaceLeft, err);

        pullDest  = (PULONG64)pCursor;
        *pullDest = ullSource;

        dwSpaceLeft  -= dwSize;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
LwBufferAllocWC16String(
    OUT PVOID        pBuffer,
    IN OUT PDWORD    pdwOffset,
    IN OUT PDWORD    pdwSpaceLeft,
    IN PCWSTR        pwszSource,
    IN OUT PDWORD    pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    size_t sStrLen = 0;
    DWORD dwStrSize = 0;
    PWSTR *ppwszDest = NULL;
    PVOID pStr = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pBuffer)
    {
        pCursor = pBuffer + dwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    LWBUF_ALIGN_PTR(dwOffset, dwSize, dwSpaceLeft);

    if (pwszSource)
    {
        dwError = LwWc16sLen(pwszSource, &sStrLen);
        BAIL_ON_LW_ERROR(dwError);

        /* it's a 2-byte unicode string */
        dwStrSize += sStrLen * 2;

        /* string termination */
        dwStrSize += sizeof(WCHAR);
    }

    if (pCursor && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwStrSize, dwSpaceLeft, dwError);

        if (pwszSource)
        {
            pStr = LWBUF_TARGET_PTR(pBuffer, dwStrSize, dwSpaceLeft);

            /* sanity check - the string and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(PWSTR, pStr, dwError);

            dwError = LwWc16snCpy((PWSTR)pStr,
                                  pwszSource,
                                  sStrLen);
            BAIL_ON_LW_ERROR(dwError);
        }

        /* recalculate space after copying the string */
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = (PWSTR)pStr;
        dwSpaceLeft  -= (pStr) ? LWBUF_ALIGN_SIZE(dwStrSize) : 0;

        /* recalculate space after setting the string pointer */
        dwSpaceLeft  -= sizeof(PWSTR);

        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwOffset += sizeof(PWSTR);
    dwSize   += (LWBUF_ALIGN_SIZE(dwStrSize) + sizeof(PWSTR));

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LwBufferAllocUnicodeString(
    OUT PVOID           pBuffer,
    IN OUT PDWORD       pdwOffset,
    IN OUT PDWORD       pdwSpaceLeft,
    IN PUNICODE_STRING  pSource,
    IN OUT PDWORD       pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    DWORD dwStrSize = 0;
    PVOID pStr = NULL;
    PWSTR *ppwszDest = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pSource)
    {
        dwStrSize = (pSource->Buffer) ? (pSource->Length + sizeof(WCHAR)) : 0;
    }

    LWBUF_ALIGN(dwOffset, dwSize, dwSpaceLeft);

    /* string length field */
    dwError = LwBufferAllocWord(pBuffer,
                                &dwOffset,
                                &dwSpaceLeft,
                                (WORD)pSource->Length,
                                &dwSize);
    BAIL_ON_LW_ERROR(dwError);

    /* string size field */
    dwError = LwBufferAllocWord(pBuffer,
                                &dwOffset,
                                &dwSpaceLeft,
                                (WORD)pSource->MaximumLength,
                                &dwSize);
    BAIL_ON_LW_ERROR(dwError);

    LWBUF_ALIGN_PTR(dwOffset, dwSize, dwSpaceLeft);

    if (pBuffer && pdwSpaceLeft)
    {
        pCursor = pBuffer + dwOffset;

        BAIL_IF_NOT_ENOUGH_SPACE(dwStrSize, dwSpaceLeft, dwError);

        if (pSource &&
            pSource->MaximumLength && pSource->Buffer)
        {
            pStr = LWBUF_TARGET_PTR(pBuffer, dwStrSize, dwSpaceLeft);

            /* sanity check - the string and current buffer cursor
               must not overlap */
            BAIL_IF_PTR_OVERLAP(PWSTR, pStr, dwError);

            dwError = LwWc16snCpy((PWSTR)pStr,
                                  pSource->Buffer,
                                  pSource->Length / 2);
            BAIL_ON_LW_ERROR(dwError);
        }

        /* recalculate space after copying the string */
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = (PWSTR)pStr;
        dwSpaceLeft  -= (pStr) ? LWBUF_ALIGN_SIZE(dwStrSize) : 0;
        dwSize       += (pStr) ? LWBUF_ALIGN_SIZE(dwStrSize) : 0;

        /* recalculate space after setting the string pointer */
        dwSpaceLeft  -= sizeof(PWSTR);

        *pdwSpaceLeft = dwSpaceLeft;
    }
    else
    {
        dwSize += LWBUF_ALIGN_SIZE(dwStrSize);
    }

    /* include size of the pointer */
    dwOffset += sizeof(PWSTR);
    dwSize   += sizeof(PWSTR);

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LwBufferAllocAnsiString(
    OUT PVOID           pBuffer,
    IN OUT PDWORD       pdwOffset,
    IN OUT PDWORD       pdwSpaceLeft,
    IN PANSI_STRING     pSource,
    IN OUT PDWORD       pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    DWORD dwStrSize = 0;
    PVOID pStr = NULL;
    PSTR *ppszDest = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pSource)
    {
        dwStrSize = pSource->Length + sizeof(CHAR);
    }

    if (pBuffer && pdwSpaceLeft && pSource)
    {
        LWBUF_ALIGN(dwOffset, dwSize, dwSpaceLeft);

        /* string length field */
        dwError = LwBufferAllocWord(pBuffer,
                                    &dwOffset,
                                    &dwSpaceLeft,
                                    (WORD)pSource->Length,
                                    &dwSize);
        BAIL_ON_LW_ERROR(dwError);

        /* string size field */
        dwError = LwBufferAllocWord(pBuffer,
                                    &dwOffset,
                                    &dwSpaceLeft,
                                    (WORD)pSource->MaximumLength,
                                    &dwSize);
        BAIL_ON_LW_ERROR(dwError);

        LWBUF_ALIGN_PTR(dwOffset, dwSize, dwSpaceLeft);
        pCursor = pBuffer + dwOffset;

        BAIL_IF_NOT_ENOUGH_SPACE(dwSize, dwSpaceLeft, dwError);

        pStr = LWBUF_TARGET_PTR(pBuffer, dwStrSize, dwSpaceLeft);

        /* sanity check - the string and current buffer cursor
           must not overlap */
        BAIL_IF_PTR_OVERLAP(PWSTR, pStr, dwError);

        memcpy(pStr, pSource->Buffer, pSource->Length);

        /* recalculate space after copying the string */
        ppszDest     = (PSTR*)pCursor;
        *ppszDest    = (PSTR)pStr;
        dwSpaceLeft -= LWBUF_ALIGN_SIZE(dwStrSize);
        dwSize      += LWBUF_ALIGN_SIZE(dwStrSize);

        /* recalculate space after setting the string pointer */
        dwSpaceLeft  -= sizeof(PSTR);

        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor && pdwSpaceLeft)
    {
        /* string length field */
        dwError = LwBufferAllocWord(pBuffer,
                                    &dwOffset,
                                    &dwSpaceLeft,
                                    0,
                                    &dwSize);
        BAIL_ON_LW_ERROR(dwError);

        /* string size field */
        dwError = LwBufferAllocWord(pBuffer,
                                    &dwOffset,
                                    &dwSpaceLeft,
                                    0,
                                    &dwSize);
        BAIL_ON_LW_ERROR(dwError);

        LWBUF_ALIGN(dwOffset, dwSize, dwSpaceLeft);
        pCursor = pBuffer + dwOffset;

        /* recalculate space after copying the string */
        ppszDest  = (PSTR*)pCursor;
        *ppszDest = NULL;

        /* recalculate space after setting the string pointer */
        dwSpaceLeft  -= sizeof(PSTR);

        *pdwSpaceLeft = dwSpaceLeft;
    }
    else
    {
        /* size of length and size fields */
        dwSize   += 2 * sizeof(USHORT);
        dwOffset += 2 * sizeof(USHORT);

        LWBUF_ALIGN(dwOffset, dwSize, dwSpaceLeft);
        dwSize += LWBUF_ALIGN_SIZE(dwStrSize);
    }

    /* include size of the pointer */
    dwOffset += sizeof(PSTR);
    dwSize   += sizeof(PSTR);

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LwBufferAllocWC16StringFromUnicodeString(
    OUT PVOID            pBuffer,
    IN OUT PDWORD        pdwOffset,
    IN OUT PDWORD        pdwSpaceLeft,
    IN PUNICODE_STRING   pSource,
    IN OUT PDWORD        pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWSTR *ppwszDest = NULL;
    PVOID pStr = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pBuffer)
    {
        pCursor = pBuffer + dwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    LWBUF_ALIGN(dwOffset, dwSize, dwSpaceLeft);

    if (pSource)
    {
        /* length is in bytes so just add 2 bytes of null termination */
        dwSize += pSource->Length + sizeof(WCHAR);
    }

    if (pCursor && pSource && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwSize, dwSpaceLeft, dwError);

        pStr = LWBUF_TARGET_PTR(pBuffer, dwSize, dwSpaceLeft);

        /* sanity check - the string and current buffer cursor
           must not overlap */
        BAIL_IF_PTR_OVERLAP(PWSTR, pStr, dwError);

        dwError = LwWc16snCpy((PWSTR)pStr,
                              pSource->Buffer,
                              pSource->Length / 2);
        BAIL_ON_LW_ERROR(dwError);

        /* recalculate space after copying the string */
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = (PWSTR)pStr;
        dwSpaceLeft  -= LWBUF_ALIGN_SIZE(dwSize);

        /* recalculate space after setting the string pointer */
        dwSpaceLeft  -= sizeof(PWSTR);

        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor && pdwSpaceLeft)
    {
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = NULL;

        dwSpaceLeft  -= sizeof(PWSTR);

        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwOffset += sizeof(PWSTR);
    dwSize   += sizeof(PWSTR);

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += LWBUF_ALIGN_SIZE(dwSize);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LwBufferAllocUnicodeStringFromWC16String(
    OUT PVOID       pBuffer,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN PCWSTR       pwszSource,
    IN OUT PDWORD   pdwSize
    )
{
    const WCHAR wszNullStr[] = {'\0'};
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwOffset = 0;
    size_t sStrLen = 0;
    DWORD dwStrSize = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    LWBUF_ALIGN(dwOffset, dwSize, dwSpaceLeft);

    if (!pwszSource)
    {
        pwszSource = &wszNullStr[0];
    }

    dwError = LwWc16sLen(pwszSource, &sStrLen);
    BAIL_ON_LW_ERROR(dwError);

    /* it's a 2-byte unicode string */
    dwStrSize = sStrLen * sizeof(WCHAR);

    /* string length field */
    dwError = LwBufferAllocWord(pBuffer,
                                &dwOffset,
                                &dwSpaceLeft,
                                (WORD)dwStrSize,
                                &dwSize);
    BAIL_ON_LW_ERROR(dwError);

    /* string size field */
    dwError = LwBufferAllocWord(pBuffer,
                                &dwOffset,
                                &dwSpaceLeft,
                                (WORD)dwStrSize,
                                &dwSize);
    BAIL_ON_LW_ERROR(dwError);

    /* the string itself */
    dwError = LwBufferAllocWC16String(pBuffer,
                                      &dwOffset,
                                      &dwSpaceLeft,
                                      pwszSource,
                                      &dwSize);
    BAIL_ON_LW_ERROR(dwError);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LwBufferAllocUnicodeStringExFromWC16String(
    OUT PVOID       pBuffer,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN PCWSTR       pwszSource,
    IN OUT PDWORD   pdwSize
    )
{
    const WCHAR wszNullStr[] = {'\0'};
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwOffset = 0;
    size_t sStrLen = 0;
    DWORD dwStrSize = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (!pwszSource)
    {
        pwszSource = &wszNullStr[0];
    }

    dwError = LwWc16sLen(pwszSource, &sStrLen);
    BAIL_ON_LW_ERROR(dwError);

    /* it's a 2-byte unicode string */
    dwStrSize = sStrLen * 2;

    /* string termination */
    dwStrSize += sizeof(WCHAR);

    /* string length field */
    dwError = LwBufferAllocWord(pBuffer,
                                &dwOffset,
                                &dwSpaceLeft,
                                (WORD)(dwStrSize - sizeof(WCHAR)),
                                &dwSize);
    BAIL_ON_LW_ERROR(dwError);

    /* string size field */
    dwError = LwBufferAllocWord(pBuffer,
                                &dwOffset,
                                &dwSpaceLeft,
                                (WORD)dwStrSize,
                                &dwSize);
    BAIL_ON_LW_ERROR(dwError);

    /* the string itself */
    dwError = LwBufferAllocWC16String(pBuffer,
                                      &dwOffset,
                                      &dwSpaceLeft,
                                      pwszSource,
                                      &dwSize);
    BAIL_ON_LW_ERROR(dwError);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LwBufferAllocSid(
    OUT PVOID       pBuffer,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN PSID         pSourceSid,
    IN DWORD        dwSourceSidLength,
    IN OUT PDWORD   pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    DWORD dwSidSize = 0;
    PVOID pSid = NULL;
    PSID *ppDest = NULL;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pBuffer)
    {
        pCursor =  pBuffer + dwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    LWBUF_ALIGN(dwOffset, dwSize, dwSpaceLeft);

    if (pSourceSid)
    {
        dwSidSize = RtlLengthRequiredSid(pSourceSid->SubAuthorityCount);
    }
    else if (dwSourceSidLength)
    {
        dwSidSize = dwSourceSidLength;
    }
    else
    {
        dwSidSize = 0;
    }

    if (pCursor && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwSidSize, dwSpaceLeft, dwError);

        pSid = LWBUF_TARGET_PTR(pBuffer, dwSidSize, dwSpaceLeft);

        /* sanity check - the string and current buffer cursor
           must not overlap */
        BAIL_IF_PTR_OVERLAP(PSID, pSid, dwError);

        if (pSourceSid)
        {
            ntStatus = RtlCopySid(dwSidSize,
                                  (PSID)pSid,
                                  pSourceSid);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /* recalculate size and space after copying the SID */
        ppDest        = (PSID*)pCursor;
        *ppDest       = pSourceSid ? (PSID)pSid : NULL;
        dwSpaceLeft  -= LWBUF_ALIGN_SIZE(dwSidSize);

        /* recalculate size and space after setting the SID pointer */
        dwSpaceLeft  -= sizeof(PSID);

        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwOffset += sizeof(PSID);
    dwSize   += (LWBUF_ALIGN_SIZE(dwSidSize) + sizeof(PSID));

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
LwBufferAllocFixedBlob(
    OUT PVOID       pBuffer,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN PVOID        pBlob,
    IN DWORD        dwBlobSize,
    IN OUT PDWORD   pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    if (pdwOffset)
    {
        dwOffset = *pdwOffset;
    }

    if (pBuffer)
    {
        pCursor = pBuffer + dwOffset;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwOffset += dwBlobSize;
    dwSize    = dwBlobSize;

    if (pCursor && pdwSpaceLeft)
    {
        BAIL_IF_NOT_ENOUGH_SPACE(dwSize, dwSpaceLeft, dwError);

        if (pBlob)
        {
            memcpy(pCursor, pBlob, dwBlobSize);
        }

        /* recalculate size and space after copying the blob */
        dwSpaceLeft  -= dwSize;

        (*pdwSpaceLeft) = dwSpaceLeft;
    }

    if (pdwOffset)
    {
        *pdwOffset = dwOffset;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
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
