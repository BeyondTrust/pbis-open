/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */
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
 *        rtlstring_unicode.c
 *
 * Abstract:
 *
 *        Base UNICODE_STRING Functions
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *          David Leimbach (dleimbach@likewise.com)
 */

#include "includes.h"
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <wc16str.h>

VOID
LwRtlUnicodeStringInit(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
    )
{
    size_t length = 0;

    if (SourceString)
    {
        length = wc16slen(SourceString);
        length = LW_MIN(length, LW_UNICODE_STRING_MAX_CHARS);
        length *= sizeof(SourceString[0]);
    }

    DestinationString->Buffer = (PWSTR) SourceString;
    DestinationString->Length = (USHORT) length;
    DestinationString->MaximumLength = DestinationString->Length + sizeof(SourceString[0]);
}

NTSTATUS
LwRtlUnicodeStringInitEx(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t length = 0;

    if (SourceString)
    {
        length = wc16slen(SourceString);
        if (length > LW_UNICODE_STRING_MAX_CHARS)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }
        length *= sizeof(SourceString[0]);
    }

    DestinationString->Buffer = (PWSTR) SourceString;
    DestinationString->Length = (USHORT) length;
    DestinationString->MaximumLength = DestinationString->Length + sizeof(SourceString[0]);

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RtlZeroMemory(DestinationString, sizeof(*DestinationString));
    }
    return status;
}

NTSTATUS
LwRtlUnicodeStringAllocateFromAnsiString(
    OUT PUNICODE_STRING pNewString,
    IN PANSI_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    ANSI_STRING terminatedOriginalString = { 0 };
    PSTR pszTerminatedString = NULL;
    UNICODE_STRING newString = { 0 };

    if (LW_RTL_STRING_IS_NULL_TERMINATED(pOriginalString))
    {
        pszTerminatedString = pOriginalString->Buffer;
    }
    else
    {
        // Since duplicate always does NULL-termination, we can
        // safely use the Buffer field as a WC16String.

        status = LwRtlAnsiStringDuplicate(&terminatedOriginalString, pOriginalString);
        GOTO_CLEANUP_ON_STATUS(status);

        pszTerminatedString = terminatedOriginalString.Buffer;
    }

    status = LwRtlUnicodeStringAllocateFromCString(&newString, pszTerminatedString);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LwRtlUnicodeStringFree(&newString);
    }
    LwRtlAnsiStringFree(&terminatedOriginalString);

    *pNewString = newString;

    return status;
}

NTSTATUS
LwRtlUnicodeStringAllocateFromWC16String(
    OUT PUNICODE_STRING pString,
    IN PCWSTR pszString
    )
{
    NTSTATUS status = 0;
    PWSTR pszNewString = NULL;
    UNICODE_STRING newString = { 0 };

    status = RtlWC16StringDuplicate(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = NULL;
    newString.Length = wc16slen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        RTL_FREE(&pszNewString);
        RtlUnicodeStringFree(&newString);
    }

    *pString = newString;

    return status;
}

NTSTATUS
LwRtlUnicodeStringAllocateFromCString(
    OUT PUNICODE_STRING pString,
    IN PCSTR pszString
    )
{
    NTSTATUS status = 0;
    PWSTR pszNewString = NULL;
    UNICODE_STRING newString = { 0 };

    status = RtlWC16StringAllocateFromCString(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = NULL;
    newString.Length = wc16slen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        RTL_FREE(&pszNewString);
        RtlUnicodeStringFree(&newString);
    }

    *pString = newString;

    return status;
}

NTSTATUS
LwRtlUnicodeStringDuplicate(
    OUT PUNICODE_STRING pNewString,
    IN PUNICODE_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    UNICODE_STRING newString = { 0 };

    if (!pOriginalString || !pNewString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (pOriginalString->Buffer && pOriginalString->Length > 0)
    {
        // Add a NULL anyhow.

        newString.Length = pOriginalString->Length;
        newString.MaximumLength = pOriginalString->Length + sizeof(pOriginalString->Buffer[0]);

        status = RTL_ALLOCATE(&newString.Buffer, WCHAR, newString.MaximumLength);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        memcpy(newString.Buffer, pOriginalString->Buffer, pOriginalString->Length);
        newString.Buffer[newString.Length/sizeof(newString.Buffer[0])] = 0;
    }

cleanup:
    if (status)
    {
        RtlUnicodeStringFree(&newString);
    }

    if (pNewString)
    {
        *pNewString = newString;
    }

    return status;
}

VOID
LwRtlUnicodeStringFree(
    IN OUT PUNICODE_STRING pString
    )
{
    RTL_FREE(&pString->Buffer);
    pString->Length = pString->MaximumLength = 0;
}

BOOLEAN
LwRtlUnicodeStringIsEqual(
    IN PUNICODE_STRING pString1,
    IN PUNICODE_STRING pString2,
    IN BOOLEAN bIsCaseSensitive
    )
{
    BOOLEAN bIsEqual = FALSE;

    // TODO--comparison -- need fix in libunistr...

    if (pString1->Length != pString2->Length)
    {
        GOTO_CLEANUP();
    }
    else if (bIsCaseSensitive)
    {
        ULONG i;
        for (i = 0; i < pString1->Length / sizeof(pString1->Buffer[0]); i++)
        {
            if (pString1->Buffer[i] != pString2->Buffer[i])
            {
                GOTO_CLEANUP();
            }
        }
    }
    else
    {
        ULONG i;
        for (i = 0; i < pString1->Length / sizeof(pString1->Buffer[0]); i++)
        {
            wchar16_t c1[] = { pString1->Buffer[i], 0 };
            wchar16_t c2[] = { pString2->Buffer[i], 0 };
            wc16supper(c1);
            wc16supper(c2);
            if (c1[0] != c2[0])
            {
                GOTO_CLEANUP();
            }
        }
    }

    bIsEqual = TRUE;

cleanup:
    return bIsEqual;
}

BOOLEAN
LwRtlUnicodeStringIsPrefix(
    IN PUNICODE_STRING pPrefix,
    IN PUNICODE_STRING pString,
    IN BOOLEAN bIsCaseSensitive
    )
{
    BOOLEAN bIsPrefix = FALSE;
    UNICODE_STRING truncatedString = { 0 };

    if (pPrefix->Length > pString->Length)
    {
        GOTO_CLEANUP();
    }

    truncatedString.Buffer = pString->Buffer;
    truncatedString.Length = truncatedString.MaximumLength = pPrefix->Length;

    bIsPrefix = LwRtlUnicodeStringIsEqual(pPrefix, &truncatedString, bIsCaseSensitive);

cleanup:
    return bIsPrefix;
}

NTSTATUS
LwRtlUnicodeStringParseULONG(
    OUT PULONG pResult,
    IN PUNICODE_STRING pString,
    OUT PUNICODE_STRING pRemainingString
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG64 value = 0;
    ULONG numChars = 0;
    ULONG index = 0;
    UNICODE_STRING remaining = { 0 };

    if (!pString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    numChars = LW_RTL_STRING_NUM_CHARS(pString);
    for (index = 0;
         ((index < numChars) &&
          LwRtlIsDecimalDigit(pString->Buffer[index]));
         index++)
    {
        value = value * 10 + LwRtlDecimalDigitValue(pString->Buffer[index]);
        if (value > MAXULONG)
        {
            status = STATUS_INTEGER_OVERFLOW;
            GOTO_CLEANUP();
        }
    }

    if (0 == index)
    {
        status = STATUS_NOT_FOUND;
        GOTO_CLEANUP();
    }

    remaining.Buffer = &pString->Buffer[index];
    remaining.Length = pString->Length - LW_PTR_OFFSET(pString->Buffer, remaining.Buffer);
    remaining.MaximumLength = remaining.Length;

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (pString)
        {
            remaining = *pString;
        }
    }

    *pResult = (ULONG) value;
    *pRemainingString = remaining;

    return status;
}

LW_NTSTATUS
LwRtlUnicodeStringAllocatePrintfWV(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN const wchar_t* pszFormat,
    LW_IN va_list Args
    )
{
    NTSTATUS status = 0;
    PWSTR pszOutputString = NULL;
    UNICODE_STRING newString = { 0 };

    status = LwRtlWC16StringAllocatePrintfWV(
                    &pszOutputString,
                    pszFormat,
                    Args);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlUnicodeStringInitEx(&newString, pszOutputString);
    GOTO_CLEANUP_ON_STATUS(status);

    pszOutputString = NULL;

cleanup:
    if (status)
    {
        RTL_UNICODE_STRING_FREE(&newString);
    }

    RTL_FREE(&pszOutputString);

    *pString = newString;

    return status;
}

LW_NTSTATUS
LwRtlUnicodeStringAllocatePrintfW(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN const wchar_t* pszFormat,
    LW_IN ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, pszFormat);
    status = LwRtlUnicodeStringAllocatePrintfWV(pString, pszFormat, args);
    va_end(args);

    return status;
}

LW_NTSTATUS
LwRtlUnicodeStringAllocatePrintf(
    LW_OUT LW_PUNICODE_STRING pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, Format);
    status = LwRtlUnicodeStringAllocatePrintfV(pNewString, Format, args);
    va_end(args);

    return status;
}

LW_NTSTATUS
LwRtlUnicodeStringAllocatePrintfV(
    LW_OUT LW_PUNICODE_STRING pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN va_list Args
    )
{
    NTSTATUS status = 0;
    PWSTR pOutputString = NULL;
    UNICODE_STRING newString = { 0 };

    status = LwRtlWC16StringAllocatePrintfV(
                    &pOutputString,
                    Format,
                    Args);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlUnicodeStringInitEx(&newString, pOutputString);
    GOTO_CLEANUP_ON_STATUS(status);

    pOutputString = NULL;

cleanup:
    if (status)
    {
        RTL_UNICODE_STRING_FREE(&newString);
    }

    RTL_FREE(&pOutputString);

    *pNewString = newString;

    return status;
}

static
NTSTATUS
LwRtlUnicodeStringAllocateAppendPrintfV(
    IN OUT PUNICODE_STRING pString,
    IN PCSTR Format,
    IN va_list Args
    )
{
    NTSTATUS status = 0;
    UNICODE_STRING addString = { 0 };
    UNICODE_STRING newString = { 0 };

    status = LwRtlUnicodeStringAllocatePrintfV(&addString, Format, Args);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pString->Buffer)
    {
        status = LwRtlUnicodeStringAllocatePrintf(&newString,
                                                 "%wZ%wZ",
                                                  pString,
                                                  &addString);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        newString = addString;
        LwRtlZeroMemory(&addString, sizeof(addString));
    }

cleanup:
    if (status)
    {
        LW_RTL_UNICODE_STRING_FREE(&newString);
    }
    else
    {
        LW_RTL_UNICODE_STRING_FREE(pString);
        *pString = newString;
    }

    LW_RTL_UNICODE_STRING_FREE(&addString);

    return status;
}


LW_NTSTATUS
LwRtlUnicodeStringAllocateAppendPrintf(
    LW_IN LW_OUT LW_PUNICODE_STRING pString,
    LW_IN LW_PCSTR Format,
    ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, Format);
    status = LwRtlUnicodeStringAllocateAppendPrintfV(pString, Format, args);
    va_end(args);

    return status;
}
