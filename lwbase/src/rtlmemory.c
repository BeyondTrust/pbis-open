/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rtlmemory.c
 *
 * Abstract:
 *
 *        Likewise Memory Utilities
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"
#include <lw/rtlgoto.h>

PVOID
LwRtlMemoryAllocate(
    IN size_t Size,
    IN BOOLEAN Clear
    )
{
    PVOID pMemory = NULL;

    // Currently, we fail zero-sized allocations.
    // If we want to relax that in the future, we can do so.
    if (Size <= 0)
    {
        GOTO_CLEANUP();
    }

    // Note -- If this allocator changes, need to change iostring routines.
    if (Clear)
    {
        pMemory = calloc(1, Size);
    }
    else
    {
        pMemory = malloc(Size);
    }

cleanup:

    return pMemory;
}

PVOID
LwRtlMemoryRealloc(
    LW_IN LW_PVOID pMemory,
    LW_IN size_t Size
    )
{
    assert(Size > 0);

    return realloc(pMemory, Size);
}

VOID
LwRtlMemoryFree(
    IN OUT LW_PVOID pMemory
    )
{
    assert(pMemory);
    free(pMemory);
}





/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
