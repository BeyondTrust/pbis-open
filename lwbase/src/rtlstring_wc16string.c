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
 *        rtlstring_ansi.c
 *
 * Abstract:
 *
 *        Base C-Style WCHAR String Functions
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "includes.h"
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <wc16str.h>
#include <wc16printf.h>

size_t
LwRtlWC16StringNumChars(
    IN PCWSTR pszString
    )
{
    return wc16slen(pszString);
}

NTSTATUS
LwRtlWC16StringAllocateFromCString(
    OUT PWSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
    NTSTATUS status = 0;
    PWSTR pszNewString = NULL;

    if (pszOriginalString)
    {
        pszNewString = ambstowc16s(pszOriginalString);
        if (!pszNewString)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            GOTO_CLEANUP();
        }
    }

cleanup:
    if (status)
    {
        free(pszNewString);
        pszNewString = NULL;
    }
    *ppszNewString = pszNewString;
    return status;
}

LW_NTSTATUS
LwRtlWC16StringAllocateFromUnicodeString(
    LW_OUT LW_PWSTR* ppszNewString,
    LW_IN LW_PUNICODE_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    UNICODE_STRING terminatedOriginalString = { 0 };
    PWSTR pszNewString = NULL;

    // Since duplicate always does NULL-termination, we can
    // safely use the Buffer field as a WC16String.

    status = LwRtlUnicodeStringDuplicate(&terminatedOriginalString, pOriginalString);
    GOTO_CLEANUP_ON_STATUS(status);

    pszNewString = terminatedOriginalString.Buffer;
    terminatedOriginalString.Buffer = NULL;
    terminatedOriginalString.Length = 0;
    terminatedOriginalString.MaximumLength = 0;

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
LwRtlWC16StringDuplicate(
    OUT PWSTR* ppszNewString,
    IN PCWSTR pszOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    size_t size = 0;
    PWSTR pszNewString = NULL;

    if (!pszOriginalString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    size = (LwRtlWC16StringNumChars(pszOriginalString) + 1) * sizeof(pszOriginalString[0]);

    status = RTL_ALLOCATE(&pszNewString, wchar16_t, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    memcpy(pszNewString, pszOriginalString, size);

cleanup:
    if (status)
    {
        RTL_FREE(&pszNewString);
    }

    *ppszNewString = pszNewString;

    return status;
}

VOID
LwRtlWC16StringFree(
    OUT PWSTR* ppszString
    )
{
    RTL_FREE(ppszString);
}

LW_BOOLEAN
LwRtlWC16StringIsEqual(
    LW_IN LW_PCWSTR pString1,
    LW_IN LW_PCWSTR pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    )
{
    BOOLEAN bIsEqual = FALSE;
    PCWSTR pCurrent1 = pString1;
    PCWSTR pCurrent2 = pString2;

    // TODO--comparison -- need fix in libunistr...

    if (bIsCaseSensitive)
    {
        while (pCurrent1[0] && pCurrent2[0])
        {
            if (pCurrent1[0] != pCurrent2[0])
            {
                GOTO_CLEANUP();
            }
            pCurrent1++;
            pCurrent2++;
        }
        if (pCurrent1[0] || pCurrent2[0])
        {
            GOTO_CLEANUP();
        }
    }
    else
    {
        while (pCurrent1[0] && pCurrent2[0])
        {
            wchar16_t c1[] = { pCurrent1[0], 0 };
            wchar16_t c2[] = { pCurrent2[0], 0 };
            wc16supper(c1);
            wc16supper(c2);
            if (c1[0] != c2[0])
            {
                GOTO_CLEANUP();
            }
            pCurrent1++;
            pCurrent2++;
        }
        if (pCurrent1[0] || pCurrent2[0])
        {
            GOTO_CLEANUP();
        }
    }

    bIsEqual = TRUE;

cleanup:
    return bIsEqual;
}

LW_NTSTATUS
LwRtlWC16StringAllocatePrintfWV(
    LW_OUT LW_PWSTR* ppszString,
    LW_IN const wchar_t* pszFormat,
    LW_IN va_list Args
    )
{
    NTSTATUS status = 0;
    PWSTR pszNewString = NULL;

    pszNewString = asw16printfwv(pszFormat, Args);

    if (pszNewString == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    *ppszString = pszNewString;

    return status;
}

LW_NTSTATUS
LwRtlWC16StringAllocatePrintfW(
    LW_OUT LW_PWSTR* ppszString,
    LW_IN const wchar_t* pszFormat,
    LW_IN ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, pszFormat);
    status = LwRtlWC16StringAllocatePrintfWV(ppszString, pszFormat, args);
    va_end(args);

    return status;
}
