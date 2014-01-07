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
 * license@likewisesoftware.comb
 */

#include "config.h"
#include "ctbase.h"
#include "cthash.h"

DWORD
CtHashCreate(
        size_t sTableSize,
        CT_HASH_KEY_COMPARE fnComparator,
        CT_HASH_KEY fnHash,
        CT_HASH_FREE_ENTRY fnFree, //optional
        CT_HASH_COPY_ENTRY fnCopy, //optional
        CT_HASH_TABLE** ppResult)
{
    CT_HASH_TABLE *pResult = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;

    dwError = CTAllocateMemory(
                    sizeof(*pResult),
                    (PVOID*)&pResult);
    BAIL_ON_CENTERIS_ERROR(dwError);

    pResult->sTableSize = sTableSize;
    pResult->sCount = 0;
    pResult->fnComparator = fnComparator;
    pResult->fnHash = fnHash;
    pResult->fnFree = fnFree;
    pResult->fnCopy = fnCopy;

    dwError = CTAllocateMemory(
                    sizeof(*pResult->ppEntries) * sTableSize,
                    (PVOID*)&pResult->ppEntries);
    BAIL_ON_CENTERIS_ERROR(dwError);

    *ppResult = pResult;

cleanup:
    return dwError;

error:
    CtHashSafeFree(&pResult);
    goto cleanup;
}

size_t
CtHashGetKeyCount(
    PCT_HASH_TABLE pTable
    )
{
    return pTable->sCount;
}

//Don't call this
void
CtHashFree(
        CT_HASH_TABLE* pResult)
{
    CtHashSafeFree(&pResult);
}

void
CtHashRemoveAll(
        CT_HASH_TABLE* pResult)
{
    size_t sBucket = 0;
    CT_HASH_ENTRY *pEntry = NULL;

    for (sBucket = 0; pResult->sCount; sBucket++)
    {
        while ( (pEntry = pResult->ppEntries[sBucket]) != NULL)
        {
            if (pResult->fnFree != NULL)
            {
                pResult->fnFree(pEntry);
            }
            pResult->ppEntries[sBucket] = pEntry->pNext;
            pResult->sCount--;
            CT_SAFE_FREE_MEMORY(pEntry);
        }
    }
}

void
CtHashSafeFree(
        CT_HASH_TABLE** ppResult)
{
    if (*ppResult != NULL)
    {
        CtHashRemoveAll(*ppResult);
        CT_SAFE_FREE_MEMORY((*ppResult)->ppEntries);
        CT_SAFE_FREE_MEMORY(*ppResult);
    }
}

DWORD
CtHashSetValue(
        CT_HASH_TABLE *pTable,
        PVOID  pKey,
        PVOID  pValue)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
    CT_HASH_ENTRY **ppExamine = &pTable->ppEntries[sBucket];
    CT_HASH_ENTRY *pNewEntry = NULL;

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
    dwError = CTAllocateMemory(
                    sizeof(*pNewEntry),
                    (PVOID*)&pNewEntry);
    BAIL_ON_CENTERIS_ERROR(dwError);
    pNewEntry->pKey = pKey;
    pNewEntry->pValue = pValue;

    *ppExamine = pNewEntry;
    pTable->sCount++;

cleanup:
    return dwError;

error:
    CT_SAFE_FREE_MEMORY(pNewEntry);
    goto cleanup;
}

//Returns ERROR_NOT_FOUND if pKey is not in the table
DWORD
CtHashGetValue(
        CT_HASH_TABLE *pTable,
        PCVOID  pKey,
        PVOID* ppValue)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sBucket = 0;
    CT_HASH_ENTRY *pExamine = NULL;
    
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

    dwError = ERROR_NOT_FOUND;

cleanup:

    return dwError;
}

BOOLEAN
CtHashExists(
    IN PCT_HASH_TABLE pTable,
    IN PCVOID pKey
    )
{
    DWORD dwError = CtHashGetValue(pTable, pKey, NULL);
    return (LW_ERROR_SUCCESS == dwError) ? TRUE : FALSE;
}

DWORD
CtHashCopy(
    IN  CT_HASH_TABLE *pTable,
    OUT CT_HASH_TABLE **ppResult
    )
{
    DWORD             dwError = LW_ERROR_SUCCESS;
    CT_HASH_ITERATOR iterator;
    CT_HASH_ENTRY    EntryCopy;
    CT_HASH_ENTRY    *pEntry = NULL;
    CT_HASH_TABLE    *pResult = NULL;

    memset(&EntryCopy, 0, sizeof(EntryCopy));

    dwError = CtHashCreate(
                  pTable->sTableSize,
                  pTable->fnComparator,
                  pTable->fnHash,
                  pTable->fnCopy ? pTable->fnFree : NULL,
                  pTable->fnCopy,
                  &pResult);
    BAIL_ON_CENTERIS_ERROR(dwError);

    dwError = CtHashGetIterator(pTable, &iterator);
    BAIL_ON_CENTERIS_ERROR(dwError);

    while ((pEntry = CtHashNext(&iterator)) != NULL)
    {
        if ( pTable->fnCopy )
        {
            dwError = pTable->fnCopy(pEntry, &EntryCopy);
            BAIL_ON_CENTERIS_ERROR(dwError);
        }
        else
        {
            EntryCopy.pKey = pEntry->pKey;
            EntryCopy.pValue = pEntry->pValue;
        }

        dwError = CtHashSetValue(
                      pResult,
                      EntryCopy.pKey,
                      EntryCopy.pValue);
        BAIL_ON_CENTERIS_ERROR(dwError);

        memset(&EntryCopy, 0, sizeof(EntryCopy));
    }

    *ppResult = pResult;

cleanup:

    return dwError;

error:

    if ( pTable->fnCopy && pTable->fnFree )
    {
        pTable->fnFree(&EntryCopy);
    }

    CtHashSafeFree(&pResult);

    goto cleanup;
}

//Invalidates all iterators
DWORD
CtHashResize(
        CT_HASH_TABLE *pTable,
        size_t sTableSize)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    CT_HASH_ENTRY **ppEntries;
    CT_HASH_ITERATOR iterator;
    CT_HASH_ENTRY *pEntry = NULL;
    size_t sBucket;

    dwError = CTAllocateMemory(
                    sizeof(*ppEntries) * sTableSize,
                    (PVOID*)&ppEntries);
    BAIL_ON_CENTERIS_ERROR(dwError);

    dwError = CtHashGetIterator(pTable, &iterator);
    BAIL_ON_CENTERIS_ERROR(dwError);

    while ((pEntry = CtHashNext(&iterator)) != NULL)
    {
        sBucket = pTable->fnHash(pEntry->pKey) % sTableSize;
        pEntry->pNext = ppEntries[sBucket];
        ppEntries[sBucket] = pEntry;
    }

    CT_SAFE_FREE_MEMORY(pTable->ppEntries);
    pTable->ppEntries = ppEntries;
    pTable->sTableSize = sTableSize;

cleanup:
    return dwError;

error:
    CT_SAFE_FREE_MEMORY(ppEntries);

    goto cleanup;
}

DWORD
CtHashGetIterator(
        CT_HASH_TABLE *pTable,
        CT_HASH_ITERATOR *pIterator)
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
    
    return LW_ERROR_SUCCESS;
}

// returns NULL after passing the last entry
CT_HASH_ENTRY *
CtHashNext(
        CT_HASH_ITERATOR *pIterator
        )
{
    CT_HASH_ENTRY *pRet;

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

DWORD
CtHashRemoveKey(
        CT_HASH_TABLE *pTable,
        PVOID  pKey)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
    CT_HASH_ENTRY **ppExamine = &pTable->ppEntries[sBucket];
    CT_HASH_ENTRY *pDelete;

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
            CT_SAFE_FREE_MEMORY(pDelete);
            goto cleanup;
        }

        ppExamine = &(*ppExamine)->pNext;
    }

    //The key isn't in the table yet.
    dwError = ERROR_NOT_FOUND;

cleanup:
    return dwError;
}

int
CtHashStringCompare(
        PCVOID str1,
        PCVOID str2)
{
    if (str1 == NULL)
    {
        if (str2 == NULL)
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    if (str2 == NULL)
    {
        return 1;
    }
    return strcmp((PCSTR)str1, (PCSTR)str2);
}

size_t
CtHashStringHash(
        PCVOID str)
{
    size_t result = 0;
    PCSTR pos = (PCSTR)str;

    if (!str)
    {
        return 0;
    }

    while (*pos != '\0')
    {
        // rotate result to the left 3 bits with wrap around
        result = (result << 3) | (result >> (sizeof(size_t)*8 - 3));

        result += *pos;
        pos++;
    }

    return result;
}

VOID
CtHashFreeStringKey(
    IN OUT const CT_HASH_ENTRY *pEntry
    )
{
    if (pEntry->pKey)
    {
        CTFreeMemory(pEntry->pKey);
    }
}

int
CtHashPVoidCompare(
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
CtHashPVoidHash(
    IN PCVOID pvData
    )
{
    return (size_t)pvData;
}
