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
 *        rtlstring_ansi.c
 *
 * Abstract:
 *
 *        Base ANSI_STRING Functions
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
LwRtlAnsiStringInit(
    OUT PANSI_STRING DestinationString,
    IN PCSTR SourceString
    )
{
    size_t length = 0;

    if (SourceString)
    {
        length = strlen(SourceString);
        length = LW_MIN(length, LW_ANSI_STRING_MAX_CHARS);
        length *= sizeof(SourceString[0]);
    }

    DestinationString->Buffer = (PSTR) SourceString;
    DestinationString->Length = (USHORT) length;
    DestinationString->MaximumLength = DestinationString->Length + sizeof(SourceString[0]);
}

NTSTATUS
LwRtlAnsiStringInitEx(
    OUT PANSI_STRING DestinationString,
    IN PCSTR SourceString
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t length = 0;

    if (SourceString)
    {
        length = strlen(SourceString);
        if (length > LW_ANSI_STRING_MAX_CHARS)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }
        length *= sizeof(SourceString[0]);
    }

    DestinationString->Buffer = (PSTR) SourceString;
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
LwRtlAnsiStringAllocateFromCString(
    OUT PANSI_STRING pNewString,
    IN PCSTR pszString
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;
    ANSI_STRING newString = { 0 };

    status = RtlCStringDuplicate(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlAnsiStringInitEx(&newString, pszNewString);
    GOTO_CLEANUP_ON_STATUS(status);

    pszNewString = 0;

cleanup:
    if (status)
    {
        RtlCStringFree(&pszNewString);
        RtlAnsiStringFree(&newString);
    }

    *pNewString = newString;

    return status;
}

#if 0
NTSTATUS
LwRtlAnsiStringAllocateFromWC16String(
    OUT PANSI_STRING pNewString,
    IN PCWSTR pszString
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;
    ANSI_STRING newString = { 0 };

    status = RtlCStringDuplicate(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = 0;
    newString.Length = wc16slen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        RtlCStringFree(&pszNewString);
        RtlAnsiStringFree(&newString);
    }

    *pString = newString;

    return status;
}

NTSTATUS
LwRtlAnsiStringAllocateFromUnicodeString(
    OUT PANSI_STRING pNewString,
    IN PUNICODE_STRING pString
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;
    ANSI_STRING newString = { 0 };

    status = RtlCStringDuplicate(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = 0;
    newString.Length = wc16slen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        RtlCStringFree(&pszNewString);
        RtlAnsiStringFree(&newString);
    }

    *pString = newString;

    return status;
}
#endif

NTSTATUS
LwRtlAnsiStringDuplicate(
    OUT PANSI_STRING pNewString,
    IN PANSI_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    ANSI_STRING newString = { 0 };

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

        status = RTL_ALLOCATE(&newString.Buffer, CHAR, newString.MaximumLength);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        memcpy(newString.Buffer, pOriginalString->Buffer, pOriginalString->Length);
        newString.Buffer[newString.Length/sizeof(newString.Buffer[0])] = 0;
    }

cleanup:
    if (status)
    {
        RtlAnsiStringFree(&newString);
    }

    if (pNewString)
    {
        *pNewString = newString;
    }

    return status;
}

VOID
LwRtlAnsiStringFree(
    IN OUT PANSI_STRING pString
    )
{
    RTL_FREE(&pString->Buffer);
    pString->Length = pString->MaximumLength = 0;
}

BOOLEAN
LwRtlAnsiStringIsEqual(
    IN PANSI_STRING pString1,
    IN PANSI_STRING pString2,
    IN BOOLEAN bIsCaseSensitive
    )
{
    BOOLEAN bIsEqual = FALSE;

    if (pString1->Length != pString2->Length)
    {
        bIsEqual = FALSE;
        GOTO_CLEANUP();
    }
    else if (bIsCaseSensitive)
    {
        bIsEqual = !strncmp(pString1->Buffer, pString2->Buffer, LW_RTL_STRING_NUM_CHARS(pString1));
    }
    else
    {
        bIsEqual = !strncasecmp(pString1->Buffer, pString2->Buffer, LW_RTL_STRING_NUM_CHARS(pString1));
    }

cleanup:
    return bIsEqual;
}

BOOLEAN
LwRtlAnsiStringIsPrefix(
    IN PANSI_STRING pPrefix,
    IN PANSI_STRING pString,
    IN BOOLEAN bIsCaseSensitive
    )
{
    BOOLEAN bIsPrefix = FALSE;
    ANSI_STRING truncatedString = { 0 };

    if (pPrefix->Length > pString->Length)
    {
        bIsPrefix = FALSE;
        GOTO_CLEANUP();
    }

    truncatedString.Buffer = pString->Buffer;
    truncatedString.Length = truncatedString.MaximumLength = pPrefix->Length;

    bIsPrefix = LwRtlAnsiStringIsEqual(pPrefix, &truncatedString, bIsCaseSensitive);

cleanup:
    return bIsPrefix;
}

NTSTATUS
LwRtlAnsiStringParseULONG(
    OUT PULONG pResult,
    IN PANSI_STRING pString,
    OUT PANSI_STRING pRemainingString
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG64 value = 0;
    ULONG numChars = 0;
    ULONG index = 0;
    ANSI_STRING remaining = { 0 };

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
LwRtlAnsiStringAllocatePrintf(
    LW_OUT LW_PANSI_STRING pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, Format);
    status = LwRtlAnsiStringAllocatePrintfV(pNewString, Format, args);
    va_end(args);

    return status;
}

LW_NTSTATUS
LwRtlAnsiStringAllocatePrintfV(
    LW_OUT LW_PANSI_STRING pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN va_list Args
    )
{
    NTSTATUS status = 0;
    PSTR pOutputString = NULL;
    ANSI_STRING newString = { 0 };

    status = LwRtlCStringAllocatePrintfV(
                    &pOutputString,
                    Format,
                    Args);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlAnsiStringInitEx(&newString, pOutputString);
    GOTO_CLEANUP_ON_STATUS(status);

    pOutputString = NULL;

cleanup:
    if (status)
    {
        RTL_ANSI_STRING_FREE(&newString);
    }

    RTL_FREE(&pOutputString);

    *pNewString = newString;

    return status;
}

static
NTSTATUS
LwRtlAnsiStringAllocateAppendPrintfV(
    IN OUT PANSI_STRING pString,
    IN PCSTR Format,
    IN va_list Args
    )
{
    NTSTATUS status = 0;
    ANSI_STRING addString = { 0 };
    ANSI_STRING newString = { 0 };

    status = LwRtlAnsiStringAllocatePrintfV(&addString, Format, Args);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pString->Buffer)
    {
        status = LwRtlAnsiStringAllocatePrintf(&newString,
                                                "%Z%Z",
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
        LW_RTL_ANSI_STRING_FREE(&newString);
    }
    else
    {
        LW_RTL_ANSI_STRING_FREE(pString);
        *pString = newString;
    }

    LW_RTL_ANSI_STRING_FREE(&addString);

    return status;
}


LW_NTSTATUS
LwRtlAnsiStringAllocateAppendPrintf(
    LW_IN LW_OUT LW_PANSI_STRING pString,
    LW_IN LW_PCSTR Format,
    ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, Format);
    status = LwRtlAnsiStringAllocateAppendPrintfV(pString, Format, args);
    va_end(args);

    return status;
}

