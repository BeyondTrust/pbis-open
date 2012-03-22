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
 *        rtlmemory.h
 *
 * Abstract:
 *
 *        Likewise RTL Memory API
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __RTL_MEMORY_H__
#define __RTL_MEMORY_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

LW_BEGIN_EXTERN_C

// Need string.h to get memset, memcpy, memmove, and memcmp
// to be used by Rtl{Zero,Fill,Copy,Move,Equal}Memory below.
#include <string.h>

#define LwRtlZeroMemory(Destination, Length) \
    memset(Destination, 0, Length)

#define LwRtlFillMemory(Destination, Length, Fill) \
    memset(Destination, Fill, Length)

#define LwRtlCopyMemory(Destination, Source, Length) \
    memcpy(Destination, Source, Length)

#define LwRtlMoveMemory(Destination, Source, Length) \
    memmove(Destination, Source, Length)

#define LwRtlEqualMemory(Source1, Source2, Length) \
    (!memcmp(Source1, Source2, Length))

// TODO-Rename below from LwRtlMemory* to LwRtl*Memory

// TODO-Prefix name with underscore.
LW_PVOID
LwRtlMemoryAllocate(
    LW_IN size_t Size,
    LW_IN BOOLEAN Clear
    );

LW_PVOID
LwRtlMemoryRealloc(
    LW_IN LW_PVOID pMemory,
    LW_IN size_t Size
    );

LW_VOID
LwRtlMemoryFree(
    LW_IN LW_OUT LW_PVOID pMemory
    );

// TODO-Change param order so Type is up-front such that pointer and size
// are next to each other.
// TODO-Make Type a pointer type -- for opaque types...
#define LW_RTL_ALLOCATE(ppMemory, Type, Size) \
	( (*(ppMemory)) = (Type*) LwRtlMemoryAllocate(Size, TRUE), (*(ppMemory)) ? LW_STATUS_SUCCESS : LW_STATUS_INSUFFICIENT_RESOURCES )
#define LW_RTL_ALLOCATE_NOCLEAR(ppMemory, Type, Size) \
	( (*(ppMemory)) = (Type*) LwRtlMemoryAllocate(Size, FALSE), (*(ppMemory)) ? LW_STATUS_SUCCESS : LW_STATUS_INSUFFICIENT_RESOURCES )

#define LW_RTL_ALLOCATE_AUTO(ppMemory) \
	( (*(ppMemory)) = LwRtlMemoryAllocate(sizeof(**(ppMemory)), TRUE), (*(ppMemory)) ? LW_STATUS_SUCCESS : LW_STATUS_INSUFFICIENT_RESOURCES )

#define LW_RTL_ALLOCATE_ARRAY_AUTO(ppMemory, ulCount) \
	( (*(ppMemory)) = LwRtlMemoryAllocate(sizeof(**(ppMemory)) * (ulCount), TRUE), (*(ppMemory)) ? LW_STATUS_SUCCESS : LW_STATUS_INSUFFICIENT_RESOURCES )


#define LW_RTL_FREE(ppMemory) \
    do { \
        if (*(ppMemory)) \
        { \
            LwRtlMemoryFree(*(ppMemory)); \
            (*(ppMemory)) = NULL; \
        } \
    } while (0)

#ifndef LW_STRICT_NAMESPACE

#define RtlZeroMemory(Destination, Length)          LwRtlZeroMemory(Destination, Length) 
#define RtlFillMemory(Destination, Length, Fill)    LwRtlFillMemory(Destination, Length, Fill) 
#define RtlCopyMemory(Destination, Source, Length)  LwRtlCopyMemory(Destination, Source, Length) 
#define RtlMoveMemory(Destination, Source, Length)  LwRtlMoveMemory(Destination, Source, Length) 
#define RtlEqualMemory(Source1, Source2, Length)    LwRtlEqualMemory(Source1, Source2, Length) 

#define RtlMemoryAllocate(Size, Clear)      LwRtlMemoryAllocate(Size, Clear)
#define RtlMemoryRealloc(Pointer, Size)     LwRtlMemoryRealloc(Pointer, Size)
#define RtlMemoryFree(Pointer)              LwRtlMemoryFree(Pointer)

#define RTL_ALLOCATE(ppMemory, Type, Size) \
    LW_RTL_ALLOCATE(ppMemory, Type, Size)

#define RTL_FREE(ppMemory) \
    LW_RTL_FREE(ppMemory)

#endif /* LW_STRICT_NAMESPACE */

LW_END_EXTERN_C

#endif /* __RTL_MEMORY_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
