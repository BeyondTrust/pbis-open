/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2010
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
 *        lru.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Utilities
 *
 *        LRU implementation
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 */

#include "includes.h"


typedef struct __LWIO_LRU_ENTRY_P
{
    LWIO_LRU_ENTRY              entry;

    struct __LWIO_LRU_ENTRY_P*  pNext;
    struct __LWIO_LRU_ENTRY_P*  pPrev;

} LWIO_LRU_ENTRY_P, *PLWIO_LRU_ENTRY_P;


struct __LWIO_LRU
{
    PSMB_HASH_TABLE     pHashTable;
    PLWIO_LRU_ENTRY_P   pHead;
    PLWIO_LRU_ENTRY_P   pTail;
    LWIO_LRU_FN_FREE    fnFree;
    ULONG               ulMaxSize;
    ULONG               ulCount;

};


struct __LWIO_LRU_ITERATOR
{
    PLWIO_LRU_ENTRY_P   pEntry;

};


static
VOID
LwioLruAddEntryToList(
    PLWIO_LRU           pLru,
    PLWIO_LRU_ENTRY_P   pEntry
    );

static
VOID
LwioLruRemoveEntry(
    PLWIO_LRU           pLru,
    PLWIO_LRU_ENTRY_P   pEntry
    );

static
VOID
LwioLruRemoveEntryFromList(
    PLWIO_LRU           pLru,
    PLWIO_LRU_ENTRY_P   pEntry
    );

static
VOID
LwioLruTouchEntry(
    PLWIO_LRU           pLru,
    PLWIO_LRU_ENTRY_P   pEntry
    );


NTSTATUS
LwioLruCreate(
    IN          ULONG               ulSize,
    IN OPTIONAL ULONG               ulHashSize,
    IN          LWIO_LRU_FN_COMPARE fnComparator,
    IN          LWIO_LRU_FN_HASH    fnHash,
    IN          LWIO_LRU_FN_FREE    fnFree,
    OUT         PLWIO_LRU*          ppLru
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_LRU pLru = NULL;

    // Create lru
    ntStatus = LwIoAllocateMemory(sizeof(*pLru), (PVOID*)&pLru);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulHashSize == 0)
    {
        ulHashSize = ulSize;
    }

    if (ulHashSize == 1)
    {
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Create hashtable
    ntStatus = SMBHashCreate(
                    ulHashSize,
                    (SMB_HASH_KEY_COMPARE)fnComparator,
                    (SMB_HASH_KEY)fnHash,
                    NULL,
                    &pLru->pHashTable);
    BAIL_ON_NT_STATUS(ntStatus);

    pLru->fnFree = fnFree;
    pLru->ulMaxSize = ulSize;

    *ppLru = pLru;

cleanup:

    return ntStatus;

error:

    LwioLruSafeFree(&pLru);

    goto cleanup;
}

VOID
LwioLruSafeFree(
    IN OUT      PLWIO_LRU*  ppLru)
{
    PLWIO_LRU pLru = *ppLru;

    if (pLru)
    {
        PLWIO_LRU_ENTRY_P pNextEntry = NULL;
        for (; pLru->pHead; pLru->pHead = pNextEntry)
        {
            pNextEntry = pLru->pHead->pNext;

            pLru->fnFree(pLru->pHead->entry);
            LwIoFreeMemory(pLru->pHead);
        }
        
        SMBHashSafeFree(&pLru->pHashTable);
        LwIoFreeMemory(pLru);

        pLru = NULL;
        *ppLru = pLru;
    }
}

/* Becomes owner of both pKey and pValue.
 * Sets the element to be first in LRU.
 */
NTSTATUS
LwioLruSetValue(
    IN          PLWIO_LRU   pLru,
    IN          PVOID       pKey,
    IN          PVOID       pValue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_LRU_ENTRY_P pNewEntry = NULL;

    LwioLruRemove(pLru, pKey);

    // Create new entry
    ntStatus = LwIoAllocateMemory(sizeof(*pNewEntry), (PVOID*)&pNewEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    pNewEntry->entry.pKey = pKey;
    pNewEntry->entry.pValue = pValue;

    // Add to hashtable
    ntStatus = SMBHashSetValue(pLru->pHashTable, pKey, pNewEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    // Add to the head of the linked list
    LwioLruAddEntryToList(pLru, pNewEntry);

    // Check for size
    pLru->ulCount++;
    if (pLru->ulCount > pLru->ulMaxSize)
    {
        LwioLruRemoveEntry(pLru, pLru->pTail);

        LWIO_ASSERT(pLru->ulCount == pLru->ulMaxSize);
    }

cleanup:

    return ntStatus;

error:

    IO_SAFE_FREE_MEMORY(pNewEntry);

    goto cleanup;
}

/* Advances the element to the first position in LRU */
NTSTATUS
LwioLruGetValue(
    IN          PLWIO_LRU   pLru,
    IN          PCVOID      pKey,
    OUT         PVOID*      ppValue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pEntryP = NULL;

    ntStatus = SMBHashGetValue(pLru->pHashTable, pKey, &pEntryP);
    BAIL_ON_NT_STATUS(ntStatus);

    LwioLruTouchEntry(pLru, (PLWIO_LRU_ENTRY_P)pEntryP);

    *ppValue = ((PLWIO_LRU_ENTRY_P)pEntryP)->entry.pValue;

error:

    return ntStatus;
}

/* Advances the element to the first position in LRU */
BOOLEAN
LwioLruHasValue(
    IN          PLWIO_LRU   pLru,
    IN          PCVOID      pKey
    )
{
    PVOID pValue = NULL;

    return (LwioLruGetValue(pLru, pKey, &pValue) == STATUS_SUCCESS);
}

ULONG
LwioLruSize(
    IN          PLWIO_LRU   pLru
    )
{
    return pLru->ulCount;
}

BOOLEAN
LwioLruIsEmpty(
    IN          PLWIO_LRU   pLru
    )
{
    return (pLru->ulCount == 0);
}

/* The iteration order is from MRU to LRU */
NTSTATUS
LwioLruIteratorAllocate(
    IN          PLWIO_LRU           pLru,
    OUT         PLWIO_LRU_ITERATOR* ppIterator
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_LRU_ITERATOR pIterator = NULL;

    ntStatus = LwIoAllocateMemory(sizeof(*pIterator), (PVOID*)&pIterator);
    BAIL_ON_NT_STATUS(ntStatus);

    pIterator->pEntry = pLru->pHead;

    *ppIterator = pIterator;

cleanup:

    return ntStatus;

error:

    LwioLruIteratorSafeFree(&pIterator);

    goto cleanup;
}

/* Does NOT affect order of elements in LRU */
PLWIO_LRU_ENTRY
LwioLruNext(
    IN OUT      PLWIO_LRU_ITERATOR  pIterator
    )
{
    PLWIO_LRU_ENTRY pNextEntry = NULL;

    if (pIterator->pEntry)
    {
        pNextEntry = &pIterator->pEntry->entry;
        pIterator->pEntry = pIterator->pEntry->pNext;
    }

    return pNextEntry;
}

VOID
LwioLruIteratorSafeFree(
    IN OUT      PLWIO_LRU_ITERATOR* ppIterator
    )
{
    IO_SAFE_FREE_MEMORY(*ppIterator);
}

NTSTATUS
LwioLruRemove(
    IN          PLWIO_LRU   pLru,
    IN          PVOID       pKey
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pEntryP = NULL;

    ntStatus = SMBHashGetValue(pLru->pHashTable, pKey, &pEntryP);
    BAIL_ON_NT_STATUS(ntStatus);

    LwioLruRemoveEntry(pLru, (PLWIO_LRU_ENTRY_P)pEntryP);

error:

    return ntStatus;
}


static
VOID
LwioLruRemoveEntry(
    PLWIO_LRU           pLru,
    PLWIO_LRU_ENTRY_P   pEntry
    )
{
    SMBHashRemoveKey(pLru->pHashTable, pEntry->entry.pKey);
    LwioLruRemoveEntryFromList(pLru, pEntry);

    pLru->fnFree(pEntry->entry);
    LwIoFreeMemory(pEntry);

    pLru->ulCount--;
}

static
VOID
LwioLruRemoveEntryFromList(
    PLWIO_LRU           pLru,
    PLWIO_LRU_ENTRY_P   pEntry
    )
{
    if (pEntry == pLru->pHead && pEntry == pLru->pTail)
    {
        pLru->pHead = NULL;
        pLru->pTail = NULL;
    }
    else if (pEntry == pLru->pHead)
    {
        pLru->pHead = pLru->pHead->pNext;
        pLru->pHead->pPrev = NULL;
    }
    else if (pEntry == pLru->pTail)
    {
        pLru->pTail = pLru->pTail->pPrev;
        pLru->pTail->pNext = NULL;
    }
    else
    {
        pEntry->pPrev->pNext = pEntry->pNext;
        pEntry->pNext->pPrev = pEntry->pPrev;
    }

    pEntry->pNext = NULL;
    pEntry->pPrev = NULL;
}

static
VOID
LwioLruAddEntryToList(
    PLWIO_LRU           pLru,
    PLWIO_LRU_ENTRY_P   pEntry
    )
{
    LWIO_ASSERT(pEntry->pNext == NULL && pEntry->pPrev == NULL);

    if (!pLru->pHead)
    {
        pLru->pHead = pEntry;
        pLru->pTail = pEntry;
    }
    else
    {
        pEntry->pNext = pLru->pHead;
        pLru->pHead->pPrev = pEntry;
        pLru->pHead = pEntry;
    }
}

static
VOID
LwioLruTouchEntry(
    PLWIO_LRU           pLru,
    PLWIO_LRU_ENTRY_P   pEntry
    )
{
    LwioLruRemoveEntryFromList(pLru, pEntry);
    LwioLruAddEntryToList(pLru, pEntry);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
