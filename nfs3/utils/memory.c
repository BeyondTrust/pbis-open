/*
 * Copyright Likewise Software    2004-2010
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
 *        NFS3
 *
 *        Memory routines.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
Nfs3pAllocateMemory(
    size_t  size,
    PVOID*  ppMemory,
    BOOLEAN bClear
    );

NTSTATUS
Nfs3AllocateMemory(
    size_t size,
    PVOID* ppMemory
    )
{
    return Nfs3pAllocateMemory(size, ppMemory, FALSE);
}

NTSTATUS
Nfs3AllocateMemoryClear(
    size_t size,
    PVOID* ppMemory
    )
{
    return Nfs3pAllocateMemory(size, ppMemory, TRUE);
}

VOID
Nfs3FreeMemory(
    PVOID* ppMemory
    )
{
    if (*ppMemory)
    {
        LwRtlMemoryFree(*ppMemory);
        *ppMemory = NULL;
    }
}

static
NTSTATUS
Nfs3pAllocateMemory(
    size_t  size,
    PVOID*  ppMemory,
    BOOLEAN bClear
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pMemory = NULL;

    pMemory = LwRtlMemoryAllocate(size, bClear);
    if (pMemory == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    *ppMemory = pMemory;

    return ntStatus;

error:

    pMemory = NULL;

    goto cleanup;
}
