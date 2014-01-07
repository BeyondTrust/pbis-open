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
 *        lwdlinkedlist.h
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) Doubly Linked List Functions
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *          
 */
#ifndef __LWDLINKED_LIST_H__
#define __LWDLINKED_LIST_H__

#include <lw/types.h>
#include <lw/attrs.h>

LW_BEGIN_EXTERN_C

typedef struct __LW_DLINKED_LIST
{
    PVOID pItem;

    struct __LW_DLINKED_LIST * pNext;

    struct __LW_DLINKED_LIST * pPrev;

} LW_DLINKED_LIST, *PLW_DLINKED_LIST;

typedef VOID (*PFN_LW_DLINKED_LIST_FUNC)(PVOID pData, PVOID pUserData);

DWORD
LwDLinkedListPrepend(
    PLW_DLINKED_LIST* ppList,
    PVOID        pItem
    );

DWORD
LwDLinkedListAppend(
    PLW_DLINKED_LIST* ppList,
    PVOID        pItem
    );

BOOLEAN
LwDLinkedListDelete(
    PLW_DLINKED_LIST* ppList,
    PVOID        pItem
    );

VOID
LwDLinkedListForEach(
    PLW_DLINKED_LIST          pList,
    PFN_LW_DLINKED_LIST_FUNC pFunc,
    PVOID                pUserData
    );

DWORD
LwDLinkedListLength(
    PLW_DLINKED_LIST pList
    );

VOID
LwDLinkedListFree(
    PLW_DLINKED_LIST pList
    );

LW_END_EXTERN_C

#endif /* __LWDLINKED_LIST_H__ */
