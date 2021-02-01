/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
    int EE ATTRIBUTE_UNUSED = 0;
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

/**
 * @brief Return current rounded up to a multiple of multiple
 *
 * n.b. this does NOT check for overflow
 *
 * @param current the current size
 * @param multiple
 *
 * @return current rounded to a mutiple of multiple,
 *  when mulitple is 0 or 1, current is returned
 */
static
size_t
round_to_multiple(
    size_t current,
    size_t multiple
    )
{
    if (multiple == 0) {
        return current;
    }

    size_t remainder = current % multiple;

    return (remainder == 0)
        ? current
        : current + multiple - remainder;
}

/**
 * @brief Grow the buffer if needed to accommodate the required number of characters
 * (not including terminating null) and return the new size
 *
 * This reallocates the buffer; Failing to reallocate the buffer will leave buffer
 * untouched and 0 will be returned.
 *
 * @param buffer
 * @param current the current size
 * @param multiple the buffer will be grown to a size which is a multiple of this value
 * @param required the number of additional characters the buffer has to include
 *  (not including the terminating null
 *
 * @return the size of the buffer, or 0 if the buffer could not be reallocated
 */
static
size_t
grow_buffer(
    char **buffer,
    size_t current_size,
    size_t multiple,
    size_t required
    )
{
    const size_t used = (*buffer) ? strlen(*buffer) : 0;
    const size_t remaining = current_size - used;

    size_t new_size = 0;
    char * new = NULL;

    if (remaining >= (required + 1)) {
        return current_size;
    }

    new_size = round_to_multiple(used + required + 1, multiple);
    new = LwRtlMemoryRealloc((void *)*buffer, new_size);

    if (!new) {
        new_size = 0;
    } else {
        *buffer = new;
    }

    return new_size;
}

PSTR
LwRtlCStringStrcatGrow(
    IN OUT PSTR* buffer,
    IN OUT size_t *current_size,
    IN size_t multiple,
    IN PCSTR src
    )
{
    size_t new_buffer_size = grow_buffer(buffer, *current_size, multiple, strlen(src));

    if (!new_buffer_size) {
        /* needed to but failed to grow the buffer */
        return NULL;
    }

    /* new buffers won't be zero filled so must strcpy */
    (*current_size == 0)
        ? strcpy(*buffer, src)
        : strcat(*buffer, src);

    *current_size = new_buffer_size;
    return *buffer;
}

