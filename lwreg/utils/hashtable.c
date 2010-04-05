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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        hashtable.c
 *
 * Abstract:
 *
 *        Likewise Registry Hashtable
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "includes.h"

NTSTATUS
RegHashCreate(
    size_t sTableSize,
    REG_HASH_KEY_COMPARE fnComparator,
    REG_HASH_KEY fnHash,
    REG_HASH_FREE_ENTRY fnFree, //optional
    REG_HASH_COPY_ENTRY fnCopy, //optional
    REG_HASH_TABLE** ppResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    REG_HASH_TABLE *pResult = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pResult, REG_HASH_TABLE, sizeof(*pResult));
    BAIL_ON_NT_STATUS(status);

    pResult->sTableSize = sTableSize;
    pResult->sCount = 0;
    pResult->fnComparator = fnComparator;
    pResult->fnHash = fnHash;
    pResult->fnFree = fnFree;
    pResult->fnCopy = fnCopy;

    status = LW_RTL_ALLOCATE((PVOID*)&pResult->ppEntries, REG_HASH_ENTRY,
    		                  sizeof(*pResult->ppEntries) * sTableSize);
    BAIL_ON_NT_STATUS(status);

    *ppResult = pResult;

cleanup:
    return status;

error:
    RegHashSafeFree(&pResult);
    goto cleanup;
}

//Don't call this
void
RegHashFree(
    REG_HASH_TABLE* pResult
    )
{
    RegHashSafeFree(&pResult);
}

void
RegHashRemoveAll(
    REG_HASH_TABLE* pResult
    )
{
    size_t sBucket = 0;
    REG_HASH_ENTRY *pEntry = NULL;

    for (sBucket = 0; pResult->sCount; sBucket++)
    {
        LW_REG_ASSERT(sBucket < pResult->sTableSize);
        while ( (pEntry = pResult->ppEntries[sBucket]) != NULL)
        {
            if (pResult->fnFree != NULL)
            {
                pResult->fnFree(pEntry);
            }
            pResult->ppEntries[sBucket] = pEntry->pNext;
            pResult->sCount--;
            LWREG_SAFE_FREE_MEMORY(pEntry);
        }
    }
}

void
RegHashSafeFree(
    REG_HASH_TABLE** ppResult
    )
{
    if (*ppResult != NULL)
    {
        RegHashRemoveAll(*ppResult);
        LWREG_SAFE_FREE_MEMORY((*ppResult)->ppEntries);
        LWREG_SAFE_FREE_MEMORY(*ppResult);
    }
}

NTSTATUS
RegHashSetValue(
    REG_HASH_TABLE *pTable,
    PVOID  pKey,
    PVOID  pValue
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
    REG_HASH_ENTRY **ppExamine = &pTable->ppEntries[sBucket];
    REG_HASH_ENTRY *pNewEntry = NULL;

    while (*ppExamine != NULL)
    {
        //There's a hash collision
        if (!pTable->fnComparator((*ppExamine)->pKey, pKey))
        {
            //The key already exists; replace it
            if (pTable->fnFree != NULL)
            {
                pTable->fnFree(*ppExamine);
            }

            (*ppExamine)->pKey = pKey;
            (*ppExamine)->pValue = pValue;
            goto cleanup;
        }

        ppExamine = &(*ppExamine)->pNext;
    }

    //The key isn't in the table yet.
    status = LW_RTL_ALLOCATE(&pNewEntry, REG_HASH_ENTRY, sizeof(*pNewEntry));
    BAIL_ON_NT_STATUS(status);

    pNewEntry->pKey = pKey;
    pNewEntry->pValue = pValue;

    *ppExamine = pNewEntry;
    pTable->sCount++;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pNewEntry);
    goto cleanup;
}

//Returns STATUS_OBJECT_NAME_NOT_FOUND if pKey is not in the table
NTSTATUS
RegHashGetValue(
    REG_HASH_TABLE *pTable,
    PCVOID  pKey,
    PVOID* ppValue //Optional
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sBucket = 0;
    REG_HASH_ENTRY *pExamine = NULL;

    if (pTable->sTableSize > 0)
    {
        sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
        pExamine = pTable->ppEntries[sBucket];
    }

    while (pExamine != NULL)
    {
        if (!pTable->fnComparator(pExamine->pKey, pKey))
        {
            //Found it!
            if (ppValue != NULL)
            {
                *ppValue = pExamine->pValue;
            }
            goto cleanup;
        }

        pExamine = pExamine->pNext;
    }

    status = STATUS_OBJECT_NAME_NOT_FOUND;

cleanup:

    return status;
}

BOOLEAN
RegHashExists(
    IN PREG_HASH_TABLE pTable,
    IN PCVOID pKey
    )
{
	NTSTATUS status = RegHashGetValue(pTable, pKey, NULL);
    return (STATUS_SUCCESS == status) ? TRUE : FALSE;
}

NTSTATUS
RegHashCopy(
    IN  REG_HASH_TABLE *pTable,
    OUT REG_HASH_TABLE **ppResult
    )
{
	NTSTATUS          status = STATUS_SUCCESS;
    REG_HASH_ITERATOR iterator;
    REG_HASH_ENTRY    EntryCopy;
    REG_HASH_ENTRY    *pEntry = NULL;
    REG_HASH_TABLE    *pResult = NULL;

    memset(&EntryCopy, 0, sizeof(EntryCopy));

    status = RegHashCreate(
                  pTable->sTableSize,
                  pTable->fnComparator,
                  pTable->fnHash,
                  pTable->fnCopy ? pTable->fnFree : NULL,
                  pTable->fnCopy,
                  &pResult);
    BAIL_ON_NT_STATUS(status);

    RegHashGetIterator(pTable, &iterator);

    while ((pEntry = RegHashNext(&iterator)) != NULL)
    {
        if ( pTable->fnCopy )
        {
            status = pTable->fnCopy(pEntry, &EntryCopy);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            EntryCopy.pKey = pEntry->pKey;
            EntryCopy.pValue = pEntry->pValue;
        }

        status = RegHashSetValue(
                      pResult,
                      EntryCopy.pKey,
                      EntryCopy.pValue);
        BAIL_ON_NT_STATUS(status);

        memset(&EntryCopy, 0, sizeof(EntryCopy));
    }

    *ppResult = pResult;

cleanup:

    return status;

error:

    if ( pTable->fnCopy && pTable->fnFree )
    {
        pTable->fnFree(&EntryCopy);
    }

    RegHashSafeFree(&pResult);

    goto cleanup;
}

//Invalidates all iterators
NTSTATUS
RegHashResize(
    REG_HASH_TABLE *pTable,
    size_t sTableSize
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    REG_HASH_ENTRY **ppEntries;
    REG_HASH_ITERATOR iterator;
    REG_HASH_ENTRY *pEntry = NULL;
    size_t sBucket;

    status = LW_RTL_ALLOCATE((PVOID*)&ppEntries, REG_HASH_ENTRY,
    		                  sizeof(*ppEntries) * sTableSize);
    BAIL_ON_NT_STATUS(status);

    RegHashGetIterator(pTable, &iterator);

    while ((pEntry = RegHashNext(&iterator)) != NULL)
    {
        sBucket = pTable->fnHash(pEntry->pKey) % sTableSize;
        pEntry->pNext = ppEntries[sBucket];
        ppEntries[sBucket] = pEntry;
    }

    LWREG_SAFE_FREE_MEMORY(pTable->ppEntries);
    pTable->ppEntries = ppEntries;
    pTable->sTableSize = sTableSize;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(ppEntries);

    goto cleanup;
}

VOID
RegHashGetIterator(
    REG_HASH_TABLE *pTable,
    REG_HASH_ITERATOR *pIterator
    )
{
    pIterator->pTable = pTable;
    pIterator->sEntryIndex = 0;
    if(pTable->sTableSize > 0)
    {
        pIterator->pEntryPos = pTable->ppEntries[0];
    }
    else
    {
        pIterator->pEntryPos = NULL;
    }

    return;
}

// returns NULL after passing the last entry
REG_HASH_ENTRY *
RegHashNext(
    REG_HASH_ITERATOR *pIterator
    )
{
    REG_HASH_ENTRY *pRet;

    // If there are any entries left, return a non-null entry
    while (pIterator->pEntryPos == NULL &&
            pIterator->sEntryIndex < pIterator->pTable->sTableSize)
    {
        pIterator->sEntryIndex++;
        if (pIterator->sEntryIndex < pIterator->pTable->sTableSize)
        {
            pIterator->pEntryPos = pIterator->pTable->ppEntries[
                pIterator->sEntryIndex];
        }
    }

    pRet = pIterator->pEntryPos;
    if (pRet != NULL)
    {
        //Advance the iterator
        pIterator->pEntryPos = pRet->pNext;
    }

    return pRet;
}

NTSTATUS
RegHashRemoveKey(
    REG_HASH_TABLE *pTable,
    PVOID  pKey
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
    REG_HASH_ENTRY **ppExamine = &pTable->ppEntries[sBucket];
    REG_HASH_ENTRY *pDelete;

    while (*ppExamine != NULL)
    {
        if (!pTable->fnComparator((*ppExamine)->pKey, pKey))
        {
            //Found it!
            pDelete = *ppExamine;
            if (pTable->fnFree != NULL)
            {
                pTable->fnFree(pDelete);
            }

            //Remove it from the list
            pTable->sCount--;
            *ppExamine = pDelete->pNext;
            LWREG_SAFE_FREE_MEMORY(pDelete);
            goto cleanup;
        }

        ppExamine = &(*ppExamine)->pNext;
    }

    //The key isn't in the table yet.
    status = STATUS_OBJECT_NAME_NOT_FOUND;

cleanup:
    return status;
}

int
RegHashCaselessWC16StringCompare(
    PCVOID str1,
    PCVOID str2
    )
{
	return !LwRtlWC16StringIsEqual(str1, str2, FALSE);
}

size_t
RegHashCaselessStringHash(
    PCVOID str
    )
{
    size_t result = 0;
    PCSTR pos = (PCSTR)str;
    int lowerChar;

    if (!str)
    {
        return 0;
    }

    while (*pos != '\0')
    {
        // rotate result to the left 3 bits with wrap around
        result = (result << 3) | (result >> (sizeof(size_t)*8 - 3));

        lowerChar = tolower((int)*pos);
        result += lowerChar;
        pos++;
    }

    return result;
}

size_t
RegHashCaselessWc16String(
    PCVOID str
    )
{
    size_t result = 0;
    PCWSTR pos = (PCWSTR)str;
    int lowerChar;

    while (*pos != '\0')
    {
        // rotate result to the left 3 bits with wrap around
        result = (result << 3) | (result >> (sizeof(size_t)*8 - 3));

        lowerChar = *pos <= 255 ? tolower(*pos) : *pos;
        result += lowerChar;
        pos++;
    }

    return result;
}

VOID
RegHashFreeWc16StringKey(
    IN OUT const REG_HASH_ENTRY *pEntry
    )
{
    if (pEntry->pKey)
    {
    	RegFreeMemory(pEntry->pKey);
    }
}

int
RegHashPVoidCompare(
    IN PCVOID pvData1,
    IN PCVOID pvData2
    )
{
    if (pvData1 > pvData2)
    {
        return 1;
    }
    else if (pvData1 == pvData2)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

size_t
RegHashPVoidHash(
    IN PCVOID pvData
    )
{
    return (size_t)pvData;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
