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
 *        net_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetAPI memory allocation functions.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
NetInitMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    return status;
}


NTSTATUS
NetDestroyMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    return status;
}


NTSTATUS
NetAllocateMemory(
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
NetFreeMemory(
    IN PVOID pPtr
    )
{
    free(pPtr);
}


DWORD
NetAllocBufferByte(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    BYTE    ubSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PBYTE pubDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwSize = sizeof(ubSource);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pubDest  = (PBYTE)pCursor;
        *pubDest = ubSource;

        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
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
NetAllocBufferWord(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    WORD    wSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWORD pwDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwSize = sizeof(wSource);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pwDest    = (PWORD)pCursor;
        *pwDest   = wSource;

        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
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
NetAllocBufferDword(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    DWORD   dwSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PDWORD pdwDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwSize = sizeof(dwSource);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pdwDest   = (PDWORD)pCursor;
        *pdwDest  = dwSource;

        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
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
NetAllocBufferUlong64(
    PVOID   *ppCursor,
    PDWORD   pdwSpaceLeft,
    ULONG64  ullSource,
    PDWORD   pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PULONG64 pullDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwSize = sizeof(ullSource);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pullDest      = (PULONG64)pCursor;
        *pullDest     = ullSource;

        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
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
NetAllocBufferWinTimeFromNtTime(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    LONG64  ntTime,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD dwTime = LwNtTimeToWinTime((ULONG64)ntTime);

    err = NetAllocBufferDword(ppCursor,
                              pdwSpaceLeft,
                              dwTime,
                              pdwSize);
    return err;
}


DWORD
NetAllocBufferNtTimeFromWinTime(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    DWORD   dwTime,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    ULONG64 ullTime = LwWinTimeToNtTime(dwTime);

    err = NetAllocBufferUlong64(ppCursor,
                                pdwSpaceLeft,
                                ullTime,
                                pdwSize);
    return err;
}


#define SET_USER_FLAG(acb_flags, uf_flags)           \
    if (dwAcbFlags & (acb_flags))                    \
    {                                                \
        dwUserFlags |= (uf_flags);                   \
    }


DWORD
NetAllocBufferUserFlagsFromAcbFlags(
    PVOID *ppCursor,
    PDWORD pdwSpaceLeft,
    DWORD  dwAcbFlags,
    PDWORD pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD dwUserFlags = 0;

    /* ACB flags not covered:
        - ACB_NO_AUTH_DATA_REQD
        - ACB_MNS
        - ACB_AUTOLOCK

       UF flags not covered:
        - UF_SCRIPT
        - UF_LOCKOUT
        - UF_PASSWD_CANT_CHANGE,
        - UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION
    */

    SET_USER_FLAG(ACB_DISABLED, UF_ACCOUNTDISABLE);
    SET_USER_FLAG(ACB_HOMDIRREQ, UF_HOMEDIR_REQUIRED);
    SET_USER_FLAG(ACB_PWNOTREQ, UF_PASSWD_NOTREQD);
    SET_USER_FLAG(ACB_TEMPDUP, UF_TEMP_DUPLICATE_ACCOUNT);
    SET_USER_FLAG(ACB_NORMAL, UF_NORMAL_ACCOUNT);
    SET_USER_FLAG(ACB_DOMTRUST, UF_INTERDOMAIN_TRUST_ACCOUNT);
    SET_USER_FLAG(ACB_WSTRUST, UF_WORKSTATION_TRUST_ACCOUNT);
    SET_USER_FLAG(ACB_SVRTRUST, UF_SERVER_TRUST_ACCOUNT);
    SET_USER_FLAG(ACB_PWNOEXP, UF_DONT_EXPIRE_PASSWD);
    SET_USER_FLAG(ACB_ENC_TXT_PWD_ALLOWED, UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED);
    SET_USER_FLAG(ACB_SMARTCARD_REQUIRED, UF_SMARTCARD_REQUIRED);
    SET_USER_FLAG(ACB_TRUSTED_FOR_DELEGATION, UF_TRUSTED_FOR_DELEGATION);
    SET_USER_FLAG(ACB_NOT_DELEGATED, UF_NOT_DELEGATED);
    SET_USER_FLAG(ACB_USE_DES_KEY_ONLY, UF_USE_DES_KEY_ONLY);
    SET_USER_FLAG(ACB_DONT_REQUIRE_PREAUTH, UF_DONT_REQUIRE_PREAUTH);
    SET_USER_FLAG(ACB_PW_EXPIRED, UF_PASSWORD_EXPIRED);

    err = NetAllocBufferDword(ppCursor,
                              pdwSpaceLeft,
                              dwUserFlags,
                              pdwSize);
    return err;
}


#define SET_ACB_FLAG(uf_flags, acb_flags)            \
    if (dwUserFlags & (uf_flags))                    \
    {                                                \
        dwAcbFlags |= (acb_flags);                   \
    }


DWORD
NetAllocBufferAcbFlagsFromUserFlags(
    PVOID *ppCursor,
    PDWORD pdwSpaceLeft,
    DWORD  dwUserFlags,
    PDWORD pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD dwAcbFlags = 0;

    /* ACB flags not covered:
        - ACB_NO_AUTH_DATA_REQD
        - ACB_MNS
        - ACB_AUTOLOCK

       UF flags not covered:
        - UF_SCRIPT
        - UF_LOCKOUT
        - UF_PASSWD_CANT_CHANGE,
        - UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION
    */

    SET_ACB_FLAG(UF_ACCOUNTDISABLE, ACB_DISABLED);
    SET_ACB_FLAG(UF_HOMEDIR_REQUIRED, ACB_HOMDIRREQ);
    SET_ACB_FLAG(UF_PASSWD_NOTREQD, ACB_PWNOTREQ);
    SET_ACB_FLAG(UF_TEMP_DUPLICATE_ACCOUNT, ACB_TEMPDUP);
    SET_ACB_FLAG(UF_NORMAL_ACCOUNT, ACB_NORMAL);
    SET_ACB_FLAG(UF_INTERDOMAIN_TRUST_ACCOUNT, ACB_DOMTRUST);
    SET_ACB_FLAG(UF_WORKSTATION_TRUST_ACCOUNT, ACB_WSTRUST);
    SET_ACB_FLAG(UF_SERVER_TRUST_ACCOUNT, ACB_SVRTRUST);
    SET_ACB_FLAG(UF_DONT_EXPIRE_PASSWD, ACB_PWNOEXP);
    SET_ACB_FLAG(UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED, ACB_ENC_TXT_PWD_ALLOWED);
    SET_ACB_FLAG(UF_SMARTCARD_REQUIRED, ACB_SMARTCARD_REQUIRED);
    SET_ACB_FLAG(UF_TRUSTED_FOR_DELEGATION, ACB_TRUSTED_FOR_DELEGATION);
    SET_ACB_FLAG(UF_NOT_DELEGATED, ACB_NOT_DELEGATED);
    SET_ACB_FLAG(UF_USE_DES_KEY_ONLY, ACB_USE_DES_KEY_ONLY);
    SET_ACB_FLAG(UF_DONT_REQUIRE_PREAUTH, ACB_DONT_REQUIRE_PREAUTH);
    SET_ACB_FLAG(UF_PASSWORD_EXPIRED, ACB_PW_EXPIRED);

    err = NetAllocBufferDword(ppCursor,
                              pdwSpaceLeft,
                              dwAcbFlags,
                              pdwSize);
    return err;
}


DWORD
NetAllocBufferWC16String(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PCWSTR                pwszSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    size_t dwSize = 0;
    PWSTR *ppwszDest = NULL;
    PVOID pStr = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pwszSource)
    {
        err = LwWc16sLen(pwszSource, &dwSize);
        BAIL_ON_WIN_ERROR(err);

        /* it's a 2-byte unicode string */
        dwSize *= 2;

        /* string termination */
        dwSize += sizeof(WCHAR);
    }

    if (pCursor && pwszSource)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pStr = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > pStr)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        err = LwWc16snCpy((PWSTR)pStr, pwszSource, (dwSize / 2) - 1);
        BAIL_ON_WIN_ERROR(err);

        /* recalculate size and space after copying the string */
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = (PWSTR)pStr;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the string pointer */
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PWSTR);

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
NetAllocBufferWC16StringFromUnicodeString(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    UNICODE_STRING       *pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWSTR *ppwszDest = NULL;
    PVOID pStr = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pSource)
    {
        dwSize += pSource->Length + sizeof(WCHAR);
    }

    if (pCursor && pSource)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pStr = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > pStr)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        if (pSource &&
            pSource->MaximumLength && pSource->Buffer)
        {
            err = LwWc16snCpy((PWSTR)pStr,
                              pSource->Buffer,
                              pSource->Length / 2);
            BAIL_ON_WIN_ERROR(err);
        }
        else
        {
            pStr = NULL;
        }

        /* recalculate size and space after copying the string */
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = (PWSTR)pStr;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the string pointer */
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize  += sizeof(PWSTR);

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
NetAllocBufferUnicodeStringFromWC16String(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PCWSTR                pwszSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    const WCHAR wszNullStr[] = {'\0'};
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwStrSize = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (!pwszSource)
    {
        pwszSource = &wszNullStr[0];
    }

    err = LwWc16sLen(pwszSource, (size_t*)&dwStrSize);
    BAIL_ON_WIN_ERROR(err);

    /* it's a 2-byte unicode string */
    dwStrSize *= 2;

    /* string termination */
    dwStrSize += sizeof(WCHAR);

    if (pCursor)
    {
        /* string length field */
        err = NetAllocBufferWord(&pCursor,
                                 &dwSpaceLeft,
                                 (WORD)(dwStrSize - sizeof(WCHAR)),
                                 &dwSize);
        BAIL_ON_WIN_ERROR(err);

        /* string size field */
        err = NetAllocBufferWord(&pCursor,
                                 &dwSpaceLeft,
                                 (WORD)dwStrSize,
                                 &dwSize);
        BAIL_ON_WIN_ERROR(err);

        ALIGN_PTR_IN_BUFFER(UNICODE_STRING, MaximumLength,
                            pCursor, dwSize, dwSpaceLeft);

        /* the string itself */
        err = NetAllocBufferWC16String(&pCursor,
                                       &dwSpaceLeft,
                                       pwszSource,
                                       &dwSize,
                                       eValidation);
        BAIL_ON_WIN_ERROR(err);
                                        
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else
    {
        /* size of length and size fields */
        dwSize += 2 * sizeof(USHORT);

        /* size of the string */
        dwSize += dwStrSize;

        ALIGN_PTR_IN_BUFFER(UNICODE_STRING, MaximumLength,
                            pCursor, dwSize, dwSpaceLeft);

        /* size of the string pointer */
        dwSize += sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
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
NetAllocBufferLogonHours(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    LogonHours           *pHours,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PBYTE *ppbDest = NULL;
    PBYTE pbBytes = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    /*
     * The actual value of pHours is ignored at the moment
     */

    /* Logon hours is a 21-byte bit string */
    dwSize += sizeof(UINT8) * 21;

    /* Align the size */
    dwSize += sizeof(PVOID) - dwSize % sizeof(PVOID);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pbBytes = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > (PVOID)pbBytes)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        /* Allow all logon hours */
        memset(pbBytes, 1, dwSize);

        /* recalculate size and space after copying the string */
        ppbDest       = (PBYTE*)pCursor;
        *ppbDest      = (PBYTE)pbBytes;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the string pointer */
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PBYTE);

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


/*
 * See the definition of LogonHours in include/lwrpc/samrdefs.h
 */
DWORD
NetAllocBufferSamrLogonHoursFromNetLogonHours(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PDWORD                pdwHours,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    WORD wUnitsPerWeek = 0;
    PBYTE pbUnits = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    /*
     * The actual value of pHours is ignored at the moment
     */

    if (pCursor)
    {
        /* units_per_week */
        err = NetAllocBufferWord(&pCursor,
                                 &dwSpaceLeft,
                                 wUnitsPerWeek,
                                 &dwSize);
        BAIL_ON_WIN_ERROR(err);

        err = NetAllocBufferByteBlob(&pCursor,
                                     &dwSpaceLeft,
                                     pbUnits,
                                     (DWORD)wUnitsPerWeek,
                                     &dwSize,
                                     eValidation);
        BAIL_ON_WIN_ERROR(err);
                                      
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else
    {
        /* size of units_per_week */
        dwSize += sizeof(WORD);

        /* size of the blob */
        dwSize += wUnitsPerWeek / 8;

        /* size of the blob pointer */
        dwSize += sizeof(PBYTE);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
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
NetAllocBufferSid(
    PVOID               *ppCursor,
    PDWORD               pdwSpaceLeft,
    PSID                 pSourceSid,
    DWORD                dwSourceSidLength,
    PDWORD               pdwSize,
    NET_VALIDATION_LEVEL eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PVOID pSid = NULL;
    PSID *ppDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pSourceSid)
    {
        dwSize = RtlLengthRequiredSid(pSourceSid->SubAuthorityCount);

    }
    else if (dwSourceSidLength)
    {
        dwSize = dwSourceSidLength;
    }
    else
    {
        /* reserve max space if there's no clue about the size */
        dwSize = RtlLengthRequiredSid(SID_MAX_SUB_AUTHORITIES);
    }

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pSid = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PSID)) > pSid)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        if (pSourceSid)
        {
            status = RtlCopySid(dwSize,
                                (PSID)pSid,
                                pSourceSid);
            BAIL_ON_NT_STATUS(status);
        }

        /* recalculate size and space after copying the SID */
        ppDest        = (PSID*)pCursor;
        *ppDest       = (PSID)pSid;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the SID pointer */
        pCursor      += sizeof(PSID);
        dwSpaceLeft  -= sizeof(PSID);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PSID);

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferByteBlob(
    PVOID               *ppCursor,
    PDWORD               pdwSpaceLeft,
    PBYTE                pbBlob,
    DWORD                dwBlobSize,
    PDWORD               pdwSize,
    NET_VALIDATION_LEVEL eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PBYTE *ppbDest = NULL;
    PBYTE pbBytes = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    /*
     * The actual value of pHours is ignored at the moment
     */

    dwSize += dwBlobSize;

    if (pCursor && pbBlob)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pbBytes = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > (PVOID)pbBytes)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        /* copy the blob */
        memcpy(pbBytes, pbBlob, dwSize);

        /* recalculate size and space after copying the blob */
        ppbDest       = (PBYTE*)pCursor;
        *ppbDest      = (PBYTE)pbBytes;
        dwSpaceLeft  -= dwSize;

        /* recalculate cursor and space after setting the blob pointer */
        pCursor      += sizeof(PBYTE);
        dwSpaceLeft  -= sizeof(PBYTE);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += sizeof(PBYTE);
        dwSpaceLeft  -= sizeof(PBYTE);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PBYTE);

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
NetAllocBufferFixedBlob(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PBYTE                 pbBlob,
    DWORD                 dwBlobSize,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    /*
     * The actual value of pHours is ignored at the moment
     */

    dwSize += dwBlobSize;

    if (pCursor && pbBlob)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        /* copy the blob */
        memcpy(pCursor, pbBlob, dwSize);

        /* recalculate size and space after copying the blob */
        dwSpaceLeft  -= dwSize;

        /* recalculate cursor and space after setting the blob pointer */
        pCursor      += dwSize;

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
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
NetAllocBufferNT4Name(
    PVOID   *ppCursor,
    PDWORD   pdwSpaceLeft,
    PWSTR    pwszDomainName,
    PWSTR    pwszAccountName,    
    PDWORD   pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    size_t sDomainNameLen = 0;
    size_t sAccountNameLen = 0;
    PWSTR *ppwszDest = NULL;
    PVOID pNT4Name = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    err = LwWc16sLen(pwszDomainName, &sDomainNameLen);
    if (err != ERROR_SUCCESS &&
        err != ERROR_INVALID_PARAMETER)
    {
        BAIL_ON_WIN_ERROR(err);
    }

    err = LwWc16sLen(pwszAccountName, &sAccountNameLen);
    if (err != ERROR_SUCCESS &&
        err != ERROR_INVALID_PARAMETER)
    {
        BAIL_ON_WIN_ERROR(err);
    }

    /* +2 because extra space is needed for the separator ('\')
       and NULL termination */
    dwSize = sizeof(WCHAR) * (sDomainNameLen + sAccountNameLen + 2);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        pNT4Name = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > pNT4Name)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WIN_ERROR(err);
        }

        if (pwszDomainName && pwszAccountName)
        {
            if (sw16printfw((PWSTR)pNT4Name,
                            dwSize / sizeof(WCHAR),
                            L"%ws\\%ws",
                            pwszDomainName,
                            pwszAccountName) < 0)
            {
                err = LwErrnoToWin32Error(errno);
                BAIL_ON_WIN_ERROR(err);
            }
        }
        else if (pwszDomainName)
        {
            if (sw16printfw((PWSTR)pNT4Name,
                            dwSize / sizeof(WCHAR),
                            L"%ws\\",
                            pwszDomainName) < 0)
            {
                err = LwErrnoToWin32Error(errno);
                BAIL_ON_WIN_ERROR(err);
            }
        }
        else if (pwszAccountName)
        {
            if (sw16printfw((PWSTR)pNT4Name,
                            dwSize / sizeof(WCHAR),
                            L"\\%ws",
                            pwszAccountName) < 0)
            {
                err = LwErrnoToWin32Error(errno);
                BAIL_ON_WIN_ERROR(err);
            }
        }
        else
        {
            ((PWSTR)pNT4Name)[0] = '\\';
        }

        /* recalculate size and space after copying the SID */
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = (PWSTR)pNT4Name;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the SID pointer */
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PWSTR);

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

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
