/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwmem.h
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWMEM_H__
#define __LWMEM_H__

#include <lw/types.h>
#include <lw/attrs.h>

#define LW_SAFE_FREE_MEMORY(mem) \
    do { \
        if (mem) \
        { \
            LwFreeMemory(mem); \
            (mem) = NULL; \
        } \
    } while (0)

#define LW_SECURE_FREE_MEMORY(mem, len) \
    do { \
        if (mem) \
        { \
            memset(mem, 0, len); \
            LwFreeMemory(mem); \
            (mem) = NULL; \
        } \
    } while (0)


LW_BEGIN_EXTERN_C

LW_DWORD
LwAllocateMemory(
    LW_IN LW_DWORD dwSize,
    LW_OUT LW_PVOID* ppMemory
    );

LW_DWORD
LwReallocMemory(
    LW_IN LW_PVOID pMemory,
    LW_OUT LW_PVOID* ppNewMemory,
    LW_IN LW_DWORD dwSize
    );

LW_VOID
LwFreeMemory(
    LW_IN LW_OUT LW_PVOID pMemory
    );

LW_END_EXTERN_C


#endif /* __LWMEM_H__ */
