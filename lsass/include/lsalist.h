/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

/**
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * @file
 *
 *     lsalist.h
 *
 * @brief
 *
 *     Embedded Linked Lists API
 *
 * @details
 *
 *     This API lets you manipulate embedded linked lists.  These
 *     at linked lists where the links are stored inside the object,
 *     thus allowing efficient (and simple) manipulation of lists
 *     containing the object.  (For example, you can remove an object
 *     from a list only by using a reference to the object since the
 *     object conains the list pointers and w/o doing any searching.
 *     Another example is that you never need to allocate extra
 *     memory when adding an element to a list.)
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#ifndef __LSALIST_H__
#define __LSALIST_H__

///
/// Doubly-linked list structure.
///
typedef struct _LSA_LIST_LINKS {
    struct _LSA_LIST_LINKS* Next;
    struct _LSA_LIST_LINKS* Prev;
} LSA_LIST_LINKS, *PLSA_LIST_LINKS;

VOID
LsaListInit(
    OUT LSA_LIST_LINKS* Head
    );

BOOLEAN
LsaListIsEmpty(
    IN LSA_LIST_LINKS* Head
    );

VOID
LsaListInsertAfter(
    IN LSA_LIST_LINKS* Head,
    IN LSA_LIST_LINKS* Element
    );

VOID
LsaListInsertBefore(
    IN LSA_LIST_LINKS* Head,
    IN LSA_LIST_LINKS* Element
    );

VOID
LsaListRemove(
    IN LSA_LIST_LINKS* Element
    );

LSA_LIST_LINKS*
LsaListRemoveAfter(
    IN LSA_LIST_LINKS* Head
    );

LSA_LIST_LINKS*
LsaListRemoveBefore(
    IN LSA_LIST_LINKS* Head
    );

#define LsaListInsertHead LsaListInsertAfter
#define LsaListInsertTail LsaListInsertBefore

#define LsaListRemoveHead LsaListRemoveAfter
#define LsaListRemoveTail LsaListRemoveBefore

#endif
