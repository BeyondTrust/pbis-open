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
