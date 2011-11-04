/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
