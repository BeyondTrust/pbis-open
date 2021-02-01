/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        test-zct.c
 *
 * Abstract:
 *
 *        Test ZCT library
 *
 *        Test the ZCT library
 *
 * Authors: Danilo Almeida <dalmeida@likewise.com>
 *
 */

#include <moonunit/moonunit.h>
#include <lw/base.h>
#include <lwio/lwzct.h>

#define MU_ASSERT_STATUS_SUCCESS(status) \
    MU_ASSERT(STATUS_SUCCESS == (status))

MU_TEST(ZCT, 0000_Create)
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_ZCT_VECTOR pZct = NULL;

    status = LwZctCreate(&pZct, 0);
    MU_ASSERT(STATUS_INVALID_PARAMETER == status);
    LwZctDestroy(&pZct);

    status = LwZctCreate(&pZct, LW_ZCT_IO_TYPE_READ_SOCKET);
    MU_ASSERT_STATUS_SUCCESS(status);
    LwZctDestroy(&pZct);

    status = LwZctCreate(&pZct, LW_ZCT_IO_TYPE_READ_SOCKET);
    MU_ASSERT_STATUS_SUCCESS(status);
    LwZctDestroy(&pZct);

    status = LwZctCreate(&pZct, 17);
    MU_ASSERT(STATUS_INVALID_PARAMETER == status);
    LwZctDestroy(&pZct);
}

MU_TEST(ZCT, 0001_MemoryAddPrepare)
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_ZCT_VECTOR pZct = NULL;
    LW_ZCT_ENTRY entry = { 0 };
    ULONG size = 0;
    char buffer1[] = "One";
    char buffer2[] = "Two";
    char buffer3[] = "Three";
    char buffer4[] = "Four";
    char buffer5[] = "Five";

    entry.Type = LW_ZCT_ENTRY_TYPE_MEMORY;

    status = LwZctCreate(&pZct, LW_ZCT_IO_TYPE_READ_SOCKET);
    MU_ASSERT_STATUS_SUCCESS(status);

    entry.Length = sizeof(buffer1);
    entry.Data.Memory.Buffer = buffer1;

    status = LwZctAppend(pZct, &entry, 1);
    MU_ASSERT_STATUS_SUCCESS(status);

    entry.Length = sizeof(buffer2);
    entry.Data.Memory.Buffer = buffer2;

    status = LwZctPrepend(pZct, &entry, 1);
    MU_ASSERT_STATUS_SUCCESS(status);

    entry.Length = sizeof(buffer3);
    entry.Data.Memory.Buffer = buffer3;

    status = LwZctPrepend(pZct, &entry, 1);
    MU_ASSERT_STATUS_SUCCESS(status);

    entry.Length = sizeof(buffer4);
    entry.Data.Memory.Buffer = buffer4;

    status = LwZctAppend(pZct, &entry, 1);
    MU_ASSERT_STATUS_SUCCESS(status);

    entry.Length = sizeof(buffer5);
    entry.Data.Memory.Buffer = buffer5;

    status = LwZctAppend(pZct, &entry, 1);
    MU_ASSERT_STATUS_SUCCESS(status);

    // 3 2 1 4 5

    size = LwZctGetLength(pZct);
    MU_ASSERT(size == 24);

    status = LwZctPrepareIo(pZct);
    MU_ASSERT_STATUS_SUCCESS(status);

    // MU_FAILURE("check internal state with debugger");

    LwZctDestroy(&pZct);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
