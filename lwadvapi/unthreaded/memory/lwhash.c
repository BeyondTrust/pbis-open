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
 *        lwhash.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Hashtable
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
LwHashCreate(
        size_t sTableSize,
        LW_HASH_KEY_COMPARE fnComparator,
        LW_HASH_KEY fnHash,
        LW_HASH_FREE_ENTRY fnFree, //optional
        LW_HASH_COPY_ENTRY fnCopy, //optional
        LW_HASH_TABLE** ppResult)
{
    LW_HASH_TABLE *pResult = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;

    dwError = LwAllocateMemory(
                    sizeof(*pResult),
                    OUT_PPVOID(&pResult));
    BAIL_ON_LW_ERROR(dwError);

    pResult->sTableSize = sTableSize;
    pResult->sCount = 0;
    pResult->fnComparator = fnComparator;
    pResult->fnHash = fnHash;
    pResult->fnFree = fnFree;
    pResult->fnCopy = fnCopy;

    dwError = LwAllocateMemory(
                    sizeof(*pResult->ppEntries) * sTableSize,
                    OUT_PPVOID(&pResult->ppEntries));
    BAIL_ON_LW_ERROR(dwError);

    *ppResult = pResult;

cleanup:
    return dwError;

error:
    LwHashSafeFree(&pResult);
    goto cleanup;
}

size_t
LwHashGetKeyCount(
    PLW_HASH_TABLE pTable
    )
{
    return pTable->sCount;
}

//Don't call this
void
LwHashFree(
        LW_HASH_TABLE* pResult)
{
    LwHashSafeFree(&pResult);
}

void
LwHashRemoveAll(
        LW_HASH_TABLE* pResult)
{
    size_t sBucket = 0;
    LW_HASH_ENTRY *pEntry = NULL;

    for (sBucket = 0; pResult->sCount; sBucket++)
    {
        LW_ASSERT(sBucket < pResult->sTableSize);
        while ( (pEntry = pResult->ppEntries[sBucket]) != NULL)
        {
            if (pResult->fnFree != NULL)
            {
                pResult->fnFree(pEntry);
            }
            pResult->ppEntries[sBucket] = pEntry->pNext;
            pResult->sCount--;
            LW_SAFE_FREE_MEMORY(pEntry);
        }
    }
}

void
LwHashSafeFree(
        LW_HASH_TABLE** ppResult)
{
    if (*ppResult != NULL)
    {
        LwHashRemoveAll(*ppResult);
        LW_SAFE_FREE_MEMORY((*ppResult)->ppEntries);
        LW_SAFE_FREE_MEMORY(*ppResult);
    }
}

DWORD
LwHashSetValue(
        LW_HASH_TABLE *pTable,
        PVOID  pKey,
        PVOID  pValue)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
    LW_HASH_ENTRY **ppExamine = &pTable->ppEntries[sBucket];
    LW_HASH_ENTRY *pNewEntry = NULL;

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
    dwError = LwAllocateMemory(
                    sizeof(*pNewEntry),
                    OUT_PPVOID(&pNewEntry));
    BAIL_ON_LW_ERROR(dwError);
    pNewEntry->pKey = pKey;
    pNewEntry->pValue = pValue;

    *ppExamine = pNewEntry;
    pTable->sCount++;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pNewEntry);
    goto cleanup;
}

//Returns ERROR_NOT_FOUND if pKey is not in the table
DWORD
LwHashGetValue(
        LW_HASH_TABLE *pTable,
        PCVOID  pKey,
        PVOID* ppValue)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sBucket = 0;
    LW_HASH_ENTRY *pExamine = NULL;
    
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
LwHashExists(
    IN PLW_HASH_TABLE pTable,
    IN PCVOID pKey
    )
{
    DWORD dwError = LwHashGetValue(pTable, pKey, NULL);
    return (LW_ERROR_SUCCESS == dwError) ? TRUE : FALSE;
}

DWORD
LwHashCopy(
    IN  LW_HASH_TABLE *pTable,
    OUT LW_HASH_TABLE **ppResult
    )
{
    DWORD             dwError = LW_ERROR_SUCCESS;
    LW_HASH_ITERATOR iterator;
    LW_HASH_ENTRY    EntryCopy;
    LW_HASH_ENTRY    *pEntry = NULL;
    LW_HASH_TABLE    *pResult = NULL;

    memset(&EntryCopy, 0, sizeof(EntryCopy));

    dwError = LwHashCreate(
                  pTable->sTableSize,
                  pTable->fnComparator,
                  pTable->fnHash,
                  pTable->fnCopy ? pTable->fnFree : NULL,
                  pTable->fnCopy,
                  &pResult);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwHashGetIterator(pTable, &iterator);
    BAIL_ON_LW_ERROR(dwError);

    while ((pEntry = LwHashNext(&iterator)) != NULL)
    {
        if ( pTable->fnCopy )
        {
            dwError = pTable->fnCopy(pEntry, &EntryCopy);
            BAIL_ON_LW_ERROR(dwError);
        }
        else
        {
            EntryCopy.pKey = pEntry->pKey;
            EntryCopy.pValue = pEntry->pValue;
        }

        dwError = LwHashSetValue(
                      pResult,
                      EntryCopy.pKey,
                      EntryCopy.pValue);
        BAIL_ON_LW_ERROR(dwError);

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

    LwHashSafeFree(&pResult);

    goto cleanup;
}

//Invalidates all iterators
DWORD
LwHashResize(
        LW_HASH_TABLE *pTable,
        size_t sTableSize)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LW_HASH_ENTRY **ppEntries;
    LW_HASH_ITERATOR iterator;
    LW_HASH_ENTRY *pEntry = NULL;
    size_t sBucket;

    dwError = LwAllocateMemory(
                    sizeof(*ppEntries) * sTableSize,
                    OUT_PPVOID(&ppEntries));
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwHashGetIterator(pTable, &iterator);
    BAIL_ON_LW_ERROR(dwError);

    while ((pEntry = LwHashNext(&iterator)) != NULL)
    {
        sBucket = pTable->fnHash(pEntry->pKey) % sTableSize;
        pEntry->pNext = ppEntries[sBucket];
        ppEntries[sBucket] = pEntry;
    }

    LW_SAFE_FREE_MEMORY(pTable->ppEntries);
    pTable->ppEntries = ppEntries;
    pTable->sTableSize = sTableSize;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(ppEntries);

    goto cleanup;
}

DWORD
LwHashGetIterator(
        LW_HASH_TABLE *pTable,
        LW_HASH_ITERATOR *pIterator)
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
LW_HASH_ENTRY *
LwHashNext(
        LW_HASH_ITERATOR *pIterator
        )
{
    LW_HASH_ENTRY *pRet;

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
LwHashRemoveKey(
        LW_HASH_TABLE *pTable,
        PVOID  pKey)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
    LW_HASH_ENTRY **ppExamine = &pTable->ppEntries[sBucket];
    LW_HASH_ENTRY *pDelete;

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
            LW_SAFE_FREE_MEMORY(pDelete);
            goto cleanup;
        }

        ppExamine = &(*ppExamine)->pNext;
    }

    //The key isn't in the table yet.
    dwError = ERROR_NOT_FOUND;

cleanup:
    return dwError;
}

static size_t
LwHashChar(
        size_t hash,
        int ch)
{
    // rotate result to the left 3 bits with wrap around
    return ((hash << 3) | (hash >> (sizeof(size_t)*8 - 3))) + ch;
}

int
LwHashStringCompare(
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
LwHashStringHash(
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
        result = LwHashChar(result, *pos++);
    }

    return result;
}

int
LwHashCaselessStringCompare(
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
    return strcasecmp((PCSTR)str1, (PCSTR)str2);
}

size_t
LwHashCaselessStringHash(
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
        result = LwHashChar(result, tolower((int)*pos++));
    }

    return result;
}

VOID
LwHashFreeStringKey(
    IN OUT const LW_HASH_ENTRY *pEntry
    )
{
    if (pEntry->pKey)
    {
        LwFreeString(pEntry->pKey);
    }
}

int
LwHashPVoidCompare(
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
LwHashPVoidHash(
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
