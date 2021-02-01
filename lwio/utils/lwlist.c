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

/**
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * @file
 *
 *     lwlist.c
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
#include <lwlist.h>

VOID
LwListInit(
    OUT LW_LIST_LINKS* Head
    )
{
    Head->Next = Head->Prev = Head;
}

BOOLEAN
LwListIsEmpty(
    IN LW_LIST_LINKS* Head
    )
{
    return (Head->Next == Head);
}

static
BOOLEAN
LwListContains(
    IN LW_LIST_LINKS* Head,
    IN LW_LIST_LINKS* Element
)
{
    LW_LIST_LINKS* next;
    
    if (Head == NULL) return FALSE;
    if (Element == NULL) return FALSE;
    
    for (next = Head->Next; (next != NULL) && (next != Head); next = next->Next) {
        if (next == Element) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
LwListInsertAfter(
    IN LW_LIST_LINKS* Head,
    IN LW_LIST_LINKS* Element
    )
{
    // Ensure we don't already have this entry in the list as it will corrupt the list pointers
    if (LwListContains(Head, Element) == FALSE) {
        Element->Next = Head->Next;
        Element->Prev = Head;
        Head->Next->Prev = Element;
        Head->Next = Element;
    }
}

VOID
LwListInsertBefore(
    IN LW_LIST_LINKS* Head,
    IN LW_LIST_LINKS* Element
    )
{
    // Ensure we don't already have this entry in the list as it will corrupt the list pointers
    if (LwListContains(Head, Element) == FALSE) {
        Element->Next = Head;
        Element->Prev = Head->Prev;
        Head->Prev->Next = Element;
        Head->Prev = Element;
    }
}

VOID
LwListRemove(
    IN LW_LIST_LINKS* Element
    )
{
    Element->Prev->Next = Element->Next;
    Element->Next->Prev = Element->Prev;
    Element->Next = Element->Prev = Element;
}

LW_LIST_LINKS*
LwListRemoveAfter(
    IN LW_LIST_LINKS* Head
    )
{
    LW_LIST_LINKS* element = Head->Next;
    if (element != Head)
    {
        LwListRemove(element);
    }
    else
    {
        element = NULL;
    }

    return element;
}

LW_LIST_LINKS*
LwListRemoveBefore(
    IN LW_LIST_LINKS* Head
    )
{
    LW_LIST_LINKS* element = Head->Prev;
    if (element != Head)
    {
        LwListRemove(element);
    }
    else
    {
        element = NULL;
    }

    return element;
}

/**
 * Travserse a list. 
 * Return NULL when the end of the list has been reached 
 * or is empty
 **/

PLW_LIST_LINKS
LwListTraverse(
    IN PLW_LIST_LINKS Head,
    IN PLW_LIST_LINKS Cursor
    )
{
    if (LwListIsEmpty(Head)) {
        return NULL;
    }
    
    if (Cursor == NULL) {
        return Head->Next;
    }
    
    if (Cursor->Next == Head) {
        return NULL;
    }

    return Cursor->Next;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
