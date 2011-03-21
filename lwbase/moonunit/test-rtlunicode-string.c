/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */
/*
 * Copyright Likewise Software
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
 * Authors: David Leimbach <dleimbach@likewise.com>
 */

#include <moonunit/moonunit.h>
#include "lw/rtlstring.h"

MU_TEST(RtlUnicode, unicode_simple)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    UNICODE_STRING string = {0};
    UNICODE_STRING expected = {0};

    status = RtlUnicodeStringAllocateFromCString(&expected, "42");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocatePrintf(&string, "%d", 42);
    MU_ASSERT(status == 0);

    cmpResult = RtlUnicodeStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlUnicodeStringFree(&string);
    RtlUnicodeStringFree(&expected);
}

MU_TEST(RtlUnicode, unicode_null)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    UNICODE_STRING string = {0};
    UNICODE_STRING expected = {0};

    status = RtlUnicodeStringAllocateFromCString(&expected, "(null)");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocatePrintf(&string, "%s", (char *)0);
    MU_ASSERT(status == 0);

    cmpResult = RtlUnicodeStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlUnicodeStringFree(&string);
    RtlUnicodeStringFree(&expected);
}

MU_TEST(RtlUnicode, unicode_nil)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    UNICODE_STRING string = {0};
    UNICODE_STRING expected = {0};

    status = RtlUnicodeStringAllocateFromCString(&expected, sizeof(size_t) == 4 ? "00000000" : "0000000000000000");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocatePrintf(&string, "%p", (void *)0);
    MU_ASSERT(status == 0);

    cmpResult = RtlUnicodeStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlUnicodeStringFree(&string);
    RtlUnicodeStringFree(&expected);
}

MU_TEST(RtlUnicode, unicode_unicode)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    UNICODE_STRING string = {0};
    UNICODE_STRING expected = {0};

    status = RtlUnicodeStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocatePrintf(&string, "%wZ", &expected);
    MU_ASSERT(status == 0);

    cmpResult = RtlUnicodeStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlUnicodeStringFree(&string);
    RtlUnicodeStringFree(&expected);
}

MU_TEST(RtlUnicode, unicode_ansi)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    UNICODE_STRING string = {0};
    ANSI_STRING message = {0};
    UNICODE_STRING expected = {0};

    status = RtlUnicodeStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocateFromCString(&message, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocatePrintf(&string, "%Z", &message);
    MU_ASSERT(status == 0);

    cmpResult = RtlUnicodeStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlUnicodeStringFree(&string);
    RtlAnsiStringFree(&message);
    RtlUnicodeStringFree(&expected);
}

MU_TEST(RtlUnicode, unicode_append)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    UNICODE_STRING string = {0};
    UNICODE_STRING expected = {0};

    status = RtlUnicodeStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocatePrintf(&string, "%s", "Hello");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocateAppendPrintf(&string, " %s", "World");
    MU_ASSERT(status == 0);

    cmpResult = RtlUnicodeStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlUnicodeStringFree(&string);
    RtlUnicodeStringFree(&expected);
}

MU_TEST(RtlUnicode, unicode_append_ansi)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    UNICODE_STRING string = {0};
    ANSI_STRING string2 = {0};
    UNICODE_STRING expected = {0};

    status = RtlUnicodeStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocateFromCString(&string2, "World");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocatePrintf(&string, "%s", "Hello");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocateAppendPrintf(&string, " %Z", &string2);
    MU_ASSERT(status == 0);

    cmpResult = RtlUnicodeStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlUnicodeStringFree(&string);
    RtlAnsiStringFree(&string2);
    RtlUnicodeStringFree(&expected);
}

MU_TEST(RtlUnicode, unicode_append_to_null)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    UNICODE_STRING string = {0};
    UNICODE_STRING expected = {0};

    status = RtlUnicodeStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocateAppendPrintf(&string, "%s %s", "Hello", "World");
    MU_ASSERT(status == 0);

    cmpResult = RtlUnicodeStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlUnicodeStringFree(&string);
    RtlUnicodeStringFree(&expected);
}

