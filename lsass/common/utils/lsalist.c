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
 *     lsalist.c
 *
 * @brief
 *
 *     Embedded Linked Lists API Implementation
 *
 * @details
 *
 *     This API lets you manipulate embedded linked lists.  These
 *     at linked lists where the links are stored inside the object,
 *     thus allowing efficient (and simple) manipulation of lists
 *     containing the object.  (For example, you can remove an object
 *     from a list only by using a reference to the object since the
 *     object conains the list pointers.  Another example is that
 *     you never need to allocate extra memory blocks when adding
 *     elements to a list.)
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

// ISSUE-2008/07/30-dalmeida -- Add documentation.

#include "includes.h"
#include <lsalist.h>

VOID
LsaListInit(
    OUT LSA_LIST_LINKS* Head
    )
{
    Head->Next = Head->Prev = Head;
}

BOOLEAN
LsaListIsEmpty(
    IN LSA_LIST_LINKS* Head
    )
{
    return (Head->Next == Head);
}

VOID
LsaListInsertAfter(
    IN LSA_LIST_LINKS* Head,
    IN LSA_LIST_LINKS* Element
    )
{
    Element->Next = Head->Next;
    Element->Prev = Head;
    Head->Next->Prev = Element;
    Head->Next = Element;
}

VOID
LsaListInsertBefore(
    IN LSA_LIST_LINKS* Head,
    IN LSA_LIST_LINKS* Element
    )
{
    Element->Next = Head;
    Element->Prev = Head->Prev;
    Head->Prev->Next = Element;
    Head->Prev = Element;
}

VOID
LsaListRemove(
    IN LSA_LIST_LINKS* Element
    )
{
    Element->Prev->Next = Element->Next;
    Element->Next->Prev = Element->Prev;
    LsaListInit(Element);
}

LSA_LIST_LINKS*
LsaListRemoveAfter(
    IN LSA_LIST_LINKS* Head
    )
{
    LSA_LIST_LINKS* element = Head->Next;
    LsaListRemove(element);
    return element;
}

LSA_LIST_LINKS*
LsaListRemoveBefore(
    IN LSA_LIST_LINKS* Head
    )
{
    LSA_LIST_LINKS* element = Head->Prev;
    LsaListRemove(element);
    return element;
}
