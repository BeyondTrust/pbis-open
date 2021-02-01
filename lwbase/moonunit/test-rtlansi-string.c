/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 * Authors: David Leimbach <dleimbach@likewise.com>
 */

#include <moonunit/moonunit.h>
#include "lw/rtlstring.h"

MU_TEST(RtlAnsi, ansi_simple)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    ANSI_STRING string = {0};
    ANSI_STRING expected = {0};

    status = RtlAnsiStringAllocateFromCString(&expected, "42");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocatePrintf(&string, "%d", 42);
    MU_ASSERT(status == 0);

    cmpResult = RtlAnsiStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlAnsiStringFree(&string);
    RtlAnsiStringFree(&expected);
}

MU_TEST(RtlAnsi, ansi_null)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    ANSI_STRING string = {0};
    ANSI_STRING expected = {0};

    status = RtlAnsiStringAllocateFromCString(&expected, "(null)");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocatePrintf(&string, "%s", (char *)0);
    MU_ASSERT(status == 0);

    cmpResult = RtlAnsiStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlAnsiStringFree(&string);
    RtlAnsiStringFree(&expected);
}

MU_TEST(RtlAnsi, ansi_nil)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    ANSI_STRING string = {0};
    ANSI_STRING expected = {0};

    status = RtlAnsiStringAllocateFromCString(&expected, sizeof(size_t) == 4 ? "00000000" : "0000000000000000");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocatePrintf(&string, "%p", (void *)0);
    MU_ASSERT(status == 0);

    cmpResult = RtlAnsiStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    LwRtlAnsiStringFree(&string);
    RtlAnsiStringFree(&expected);
}

MU_TEST(RtlAnsi, ansi_ansi)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    ANSI_STRING string = {0};
    ANSI_STRING expected = {0};

    status = RtlAnsiStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocatePrintf(&string, "%Z", &expected);
    MU_ASSERT(status == 0);

    cmpResult = RtlAnsiStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlAnsiStringFree(&string);
    RtlAnsiStringFree(&expected);
}

MU_TEST(RtlAnsi, ansi_append)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    ANSI_STRING string = {0};
    ANSI_STRING expected = {0};

    status = RtlAnsiStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocatePrintf(&string, "%s", "Hello");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocateAppendPrintf(&string, " %s", "World");
    MU_ASSERT(status == 0);

    cmpResult = RtlAnsiStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlAnsiStringFree(&string);
    RtlAnsiStringFree(&expected);
}

MU_TEST(RtlAnsi, ansi_unicode)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    ANSI_STRING string = {0};
    UNICODE_STRING message = {0};
    ANSI_STRING expected = {0};

    status = RtlAnsiStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocateFromCString(&message, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocatePrintf(&string, "%wZ", &message);
    MU_ASSERT(status == 0);

    cmpResult = RtlAnsiStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlAnsiStringFree(&string);
    RtlAnsiStringFree(&expected);
    RtlUnicodeStringFree(&message);
}

MU_TEST(RtlAnsi, ansi_append_unicode)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    ANSI_STRING string = {0};
    UNICODE_STRING string2 = {0};
    ANSI_STRING expected = {0};

    status = RtlAnsiStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlUnicodeStringAllocateFromCString(&string2, "World");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocatePrintf(&string, "%s", "Hello");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocateAppendPrintf(&string, " %wZ", &string2);
    MU_ASSERT(status == 0);

    cmpResult = RtlAnsiStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlAnsiStringFree(&string);
    RtlUnicodeStringFree(&string2);
    RtlAnsiStringFree(&expected);
}

MU_TEST(RtlAnsi, ansi_append_to_null)
{
    NTSTATUS status = 0;
    LW_BOOLEAN cmpResult;
    ANSI_STRING string = {0};
    ANSI_STRING expected = {0};

    status = RtlAnsiStringAllocateFromCString(&expected, "Hello World");
    MU_ASSERT(status == 0);

    status = RtlAnsiStringAllocateAppendPrintf(&string, "%s%s", "Hello"," World");
    MU_ASSERT(status == 0);

    cmpResult = RtlAnsiStringIsEqual(&expected, &string, 1);
    MU_ASSERT(cmpResult);

    RtlAnsiStringFree(&string);
    RtlAnsiStringFree(&expected);
}
