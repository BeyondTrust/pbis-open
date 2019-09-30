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
 *        rtlmemory.c
 *
 * Abstract:
 *
 *        BeyondTrust Memory Utilities
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
