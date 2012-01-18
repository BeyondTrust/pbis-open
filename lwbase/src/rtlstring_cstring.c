/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rtlstring_cstring.c
 *
 * Abstract:
 *
 *        Base C-style String Functions
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *          David Leimbach (dleimbach@likewise.com)
 */

#include "includes.h"
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <wc16str.h>
#include <lwprintf.h>
#include <stdio.h>

size_t
LwRtlCStringNumChars(
    IN PCSTR pszString
    )
{
    return strlen(pszString);
}

NTSTATUS
LwRtlCStringAllocateFromWC16String(
    OUT PSTR* ppszNewString,
    IN PCWSTR pszOriginalString
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;

    if (pszOriginalString)
    {
        pszNewString = awc16stombs(pszOriginalString);
        if (!pszNewString)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            GOTO_CLEANUP();
        }
    }

cleanup:
    if (status)
    {
        LwRtlCStringFree(&pszNewString);
    }
    *ppszNewString = pszNewString;
    return status;
}

NTSTATUS
LwRtlCStringAllocateFromUnicodeString(
    OUT PSTR* ppszNewString,
    IN PUNICODE_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    UNICODE_STRING terminatedOriginalString = { 0 };
    PWSTR pwszTerminatedString = NULL;
    PSTR pszNewString = NULL;

    if (LW_RTL_STRING_IS_NULL_TERMINATED(pOriginalString))
    {
        pwszTerminatedString = pOriginalString->Buffer;
    }
    else
    {
        // Since duplicate always does NULL-termination, we can
        // safely use the Buffer field as a WC16String.

        status = LwRtlUnicodeStringDuplicate(&terminatedOriginalString, pOriginalString);
        GOTO_CLEANUP_ON_STATUS(status);

        pwszTerminatedString = terminatedOriginalString.Buffer;
    }

    status = LwRtlCStringAllocateFromWC16String(&pszNewString, pwszTerminatedString);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszNewString);
    }
    LwRtlUnicodeStringFree(&terminatedOriginalString);

    *ppszNewString = pszNewString;

    return status;
}

NTSTATUS
LwRtlCStringDuplicate(
    OUT PSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    size_t size = 0;
    PSTR pszNewString = NULL;

    if (!pszOriginalString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    size = (strlen(pszOriginalString) + 1) * sizeof(pszOriginalString[0]);

    status = LW_RTL_ALLOCATE_NOCLEAR(&pszNewString, CHAR, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    memcpy(pszNewString, pszOriginalString, size);

cleanup:
    if (status)
    {
        LwRtlCStringFree(&pszNewString);
    }

    *ppszNewString = pszNewString;

    return status;
}

VOID
LwRtlCStringFree(
    IN OUT PSTR* ppszString
    )
{
    RTL_FREE(ppszString);
}

LW_LONG
LwRtlCStringCompare(
    LW_IN LW_PCSTR pString1,
    LW_IN LW_PCSTR pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    )
{
    LW_LONG result = 0;
    int innerResult = 0;

    if (bIsCaseSensitive)
    {
        innerResult = strcmp(pString1, pString2);
    }
    else
    {
        innerResult = strcasecmp(pString1, pString2);
    }

    if (innerResult > 0)
    {
        result = 1;
    }
    else if (innerResult < 0)
    {
        result = -1;
    }

    return result;
}

LW_BOOLEAN
LwRtlCStringIsEqual(
    LW_IN LW_PCSTR pString1,
    LW_IN LW_PCSTR pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    )
{
    return LwRtlCStringCompare(pString1, pString2, bIsCaseSensitive) == 0;
}

NTSTATUS
LwRtlCStringAllocatePrintfV(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN va_list Args
    )
{
    size_t charsWritten = 0;

    return LwErrnoToNtStatus(
        LwPrintfAllocateStringV(
        ppszString,
        &charsWritten,
        pszFormat,
        Args));
}


/* Implement both strstr() and strcasestr() portably */
BOOLEAN
LwRtlCStringFindSubstring(
    LW_IN LW_PCSTR pHaystack,
    LW_IN LW_PCSTR pNeedle,
    LW_IN LW_BOOLEAN isCaseSensitive,
    LW_OUT LW_OPTIONAL LW_PCSTR *ppSubString
    )
{
    LW_PCSTR pRetStr = NULL;
    LW_PCSTR h = pHaystack;
    LW_PCSTR hret = h;
    LW_PCSTR n = pNeedle;

    /* strcasestr() functionality */
    while (*h && *n)
    {
        if ((*h == *n) || 
            (!isCaseSensitive && toupper((int) *h) == toupper((int) *n)))
        {
            h++;
            n++;
        }
        else
        {
            h++;
            hret = h;
            n = pNeedle;
        }
    }
    pRetStr = *n ? NULL : hret;

    if (ppSubString)
    {
        *ppSubString = pRetStr;
    }
    return pRetStr ? TRUE : FALSE;
}


NTSTATUS
LwRtlCStringAllocatePrintf(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, pszFormat);
    status = LwRtlCStringAllocatePrintfV(ppszString, pszFormat, args);
    va_end(args);

    return status;
}

static
NTSTATUS
LwRtlCStringAllocateAppendPrintfV(
    IN OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN va_list Args
    )
{
    NTSTATUS status = 0;
    PSTR pszAddString = NULL;
    PSTR pszNewString = NULL;

    status = LwRtlCStringAllocatePrintfV(
                &pszAddString,
                pszFormat,
                Args);
    GOTO_CLEANUP_ON_STATUS(status);

    if (*ppszString)
    {
        status = LwRtlCStringAllocatePrintf(&pszNewString,
                                            "%s%s",
                                            *ppszString,
                                            pszAddString);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        pszNewString = pszAddString;
        pszAddString = NULL;
    }

cleanup:
    if (status)
    {
        LwRtlCStringFree(&pszNewString);
    }
    else
    {
        LwRtlCStringFree(ppszString);
        *ppszString = pszNewString;
    }

    LwRtlCStringFree(&pszAddString);

    return status;
}

NTSTATUS
LwRtlCStringAllocateAppendPrintf(
    IN OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, pszFormat);
    status = LwRtlCStringAllocateAppendPrintfV(ppszString, pszFormat, args);
    va_end(args);

    return status;
}

