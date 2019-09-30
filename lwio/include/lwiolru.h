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
 *        lwiolru.h
 *
 * Abstract:
 *
 *        BeyondTrust IO (LWIO)
 * 
 *        LRU implementation, based on hashtable and linked list data structures
 *        NOT thread safe in any way - synchronize on the upper layer
 *        Must me locked all the time while accessing the values,
 *        because they may be deleted by LRU itself.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 */
#ifndef __LWIO_LRU_H__
#define __LWIO_LRU_H__


typedef struct __LWIO_LRU_ENTRY
{
    PVOID   pKey;
    PVOID   pValue;

} LWIO_LRU_ENTRY, *PLWIO_LRU_ENTRY;

typedef struct __LWIO_LRU           LWIO_LRU, *PLWIO_LRU;
typedef struct __LWIO_LRU_ITERATOR  LWIO_LRU_ITERATOR, *PLWIO_LRU_ITERATOR;

typedef LONG    (*LWIO_LRU_FN_COMPARE)  (PCVOID, PCVOID);
typedef ULONG   (*LWIO_LRU_FN_HASH)     (PCVOID);
typedef VOID    (*LWIO_LRU_FN_FREE)     (LWIO_LRU_ENTRY);


NTSTATUS
LwioLruCreate(
    IN          ULONG               ulSize,
    IN OPTIONAL ULONG               ulHashSize,
    IN          LWIO_LRU_FN_COMPARE fnComparator,
    IN          LWIO_LRU_FN_HASH    fnHash,
    IN          LWIO_LRU_FN_FREE    fnFree,
    OUT         PLWIO_LRU*          ppLru
    );

VOID
LwioLruSafeFree(
    IN OUT      PLWIO_LRU*  ppLru
    );

/* Becomes owner of both pKey and pValue.
 * Sets the element to be first in LRU.
 */
NTSTATUS
LwioLruSetValue(
    IN          PLWIO_LRU   pLru,
    IN          PVOID       pKey,
    IN          PVOID       pValue
    );

/* Advances the element to the first position in LRU */
NTSTATUS
LwioLruGetValue(
    IN          PLWIO_LRU   pLru,
    IN          PCVOID      pKey,
    OUT         PVOID*      ppValue
    );

/* Advances the element to the first position in LRU */
BOOLEAN
LwioLruHasValue(
    IN          PLWIO_LRU   pLru,
    IN          PCVOID      pKey
    );

ULONG
LwioLruSize(
    IN          PLWIO_LRU   pLru
    );

BOOLEAN
LwioLruIsEmpty(
    IN          PLWIO_LRU   pLru
    );

/* The iteration order is from MRU to LRU */
NTSTATUS
LwioLruIteratorAllocate(
    IN          PLWIO_LRU           pLru,
    OUT         PLWIO_LRU_ITERATOR* ppIterator
    );

/* Does NOT affect order of elements in LRU */
PLWIO_LRU_ENTRY
LwioLruNext(
    IN OUT      PLWIO_LRU_ITERATOR  pIterator
    );

VOID
LwioLruIteratorSafeFree(
    IN OUT      PLWIO_LRU_ITERATOR* ppIterator
    );

NTSTATUS
LwioLruRemove(
    IN          PLWIO_LRU   pLru,
    IN          PVOID       pKey
    );

#endif /* __LWIO_LRU_H__ */
