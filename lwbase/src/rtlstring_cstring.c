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
 *
 */

#include "includes.h"
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <wc16str.h>
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

/* Replacement for vasprintf */
static
char*
my_vasprintf(
    const char* fmt,
    va_list ap
    )
{
    int len;
    va_list my_ap;
    char* str, *str_new;

    /* Some versions of vsnprintf won't accept
       a null or zero-length buffer */
    str = malloc(1);

    if (!str)
    {
        return NULL;
    }
    
    va_copy(my_ap, ap);
    
    len = vsnprintf(str, 1, fmt, my_ap);
    
    /* Some versions of vsnprintf return -1 when
       the buffer was too small rather than the
       number of characters that would be written,
       so we have loop in search of a large enough
       buffer */
    if (len == -1)
    {
        int capacity = 16;
        do
        {
            capacity *= 2;
            va_copy(my_ap, ap);
            str_new = realloc(str, capacity);
            if (!str_new)
            {
                free(str);
                return NULL;
            }
            str = str_new;
        len = vsnprintf(str, capacity, fmt, my_ap);
        } while (len == -1 || capacity <= len);
        str[len] = '\0';
        
        return str;
    }
    else
    {
        va_copy(my_ap, ap);
        
        str_new = realloc(str, len+1);
        
        if (!str_new)
        {
            free(str);
            return NULL;
        }

        str = str_new;

        if (vsnprintf(str, len+1, fmt, my_ap) < len)
        {
            free(str);
            return NULL;
        }
        else
        {
            return str;
        }
    }
}

NTSTATUS
LwRtlCStringAllocatePrintfV(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN va_list Args
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;

    // TODO -- Memory model? (currenlty using free)
    // TODO -- Enhance with %Z, %wZ, etc.
    pszNewString = my_vasprintf(pszFormat, Args);

    if (pszNewString == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    *ppszString = pszNewString;

    return status;
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
    PSTR pszResultString = *ppszString;

    status = LwRtlCStringAllocatePrintfV(&pszAddString, pszFormat, Args);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pszResultString)
    {
        status = LwRtlCStringAllocatePrintf(&pszNewString, "%s%s", pszResultString, pszAddString);
        GOTO_CLEANUP_ON_STATUS(status);
        LwRtlCStringFree(&pszResultString);
    }
    else
    {
        pszNewString = pszAddString;
        pszAddString = NULL;
    }

    pszResultString = pszNewString;

cleanup:
    if (status)
    {
        LwRtlCStringFree(&pszNewString);
    }
    else
    {
        *ppszString = pszResultString;
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

