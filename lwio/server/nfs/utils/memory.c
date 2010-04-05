/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        memory.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - NFS
 *
 *        Utilities
 *
 *        Memory management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
NfsAllocateMemory(
    IN  size_t size,
    OUT PVOID* ppMemory
    )
{
    return RTL_ALLOCATE(ppMemory, VOID, size);
}

NTSTATUS
NfsReallocMemory(
    IN  PVOID  pMemory,
    IN  size_t size,
    OUT PVOID* ppNewMemory
    )
{
    PVOID pNewMemory = LwRtlMemoryRealloc(pMemory, size);

    if (!pNewMemory)
    {
        *ppNewMemory = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        *ppNewMemory = pNewMemory;
        return STATUS_SUCCESS;
    }
}

VOID
NfsFreeMemory(
    IN PVOID pMemory
    )
{
    LwRtlMemoryFree(pMemory);
}
