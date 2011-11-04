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
 *        Likewise IO (LWIO)
 *
 *        Utilities
 *
 *        Hashtable
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "includes.h"

NTSTATUS
SMBHashCreate(
        size_t sTableSize,
        SMB_HASH_KEY_COMPARE fnComparator,
        SMB_HASH_KEY fnHash,
        SMB_HASH_FREE_ENTRY fnFree, //optional
        SMB_HASH_TABLE** ppResult)
{
    SMB_HASH_TABLE *pResult = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LwIoAllocateMemory(
                    sizeof(*pResult),
                    (PVOID*)&pResult);
    BAIL_ON_NT_STATUS(ntStatus);

    pResult->sTableSize = sTableSize;
    pResult->sCount = 0;
    pResult->fnComparator = fnComparator;
    pResult->fnHash = fnHash;
    pResult->fnFree = fnFree;

    ntStatus = LwIoAllocateMemory(
                    sizeof(*pResult->ppEntries) * sTableSize,
                    (PVOID*)&pResult->ppEntries);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppResult = pResult;

cleanup:
    return ntStatus;

error:
    SMBHashSafeFree(&pResult);
    goto cleanup;
}

//Don't call this
void
SMBHashFree(
        SMB_HASH_TABLE* pResult)
{
    SMBHashSafeFree(&pResult);
}

VOID
SMBHashSafeFree(
        SMB_HASH_TABLE** ppResult)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_HASH_ITERATOR iterator;
    SMB_HASH_ENTRY *pEntry = NULL;

    if(*ppResult == NULL)
    {
        goto cleanup;
    }

    ntStatus = SMBHashGetIterator(*ppResult, &iterator);
    BAIL_ON_NT_STATUS(ntStatus);

    while ((pEntry = SMBHashNext(&iterator)) != NULL)
    {
        if ((*ppResult)->fnFree != NULL)
        {
            (*ppResult)->fnFree(pEntry);
        }
        LWIO_SAFE_FREE_MEMORY(pEntry);
    }

    LWIO_SAFE_FREE_MEMORY((*ppResult)->ppEntries);
    LwIoFreeMemory(*ppResult);

    *ppResult = NULL;

cleanup:
error:
    ;
}

NTSTATUS
SMBHashSetValue(
        SMB_HASH_TABLE *pTable,
        PVOID  pKey,
        PVOID  pValue)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    size_t sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
    SMB_HASH_ENTRY **ppExamine = &pTable->ppEntries[sBucket];
    SMB_HASH_ENTRY *pNewEntry = NULL;

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
    ntStatus = LwIoAllocateMemory(
                    sizeof(*pNewEntry),
                    (PVOID*)&pNewEntry);
    BAIL_ON_NT_STATUS(ntStatus);
    pNewEntry->pKey = pKey;
    pNewEntry->pValue = pValue;

    *ppExamine = pNewEntry;
    pTable->sCount++;

cleanup:
    return ntStatus;

error:
    LWIO_SAFE_FREE_MEMORY(pNewEntry);
    goto cleanup;
}

//Returns STATUS_NOT_FOUND if pKey is not in the table
NTSTATUS
SMBHashGetValue(
        SMB_HASH_TABLE *pTable,
        PCVOID  pKey,
        PVOID* ppValue)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    size_t sBucket = 0;
    SMB_HASH_ENTRY *pExamine = NULL;

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

    ntStatus = STATUS_NOT_FOUND;

cleanup:

    return ntStatus;
}

BOOLEAN
SMBHashExists(
    PSMB_HASH_TABLE pTable,
    PCVOID pKey
    )
{
    NTSTATUS ntStatus = SMBHashGetValue(pTable, pKey, NULL);
    return (STATUS_SUCCESS == ntStatus) ? TRUE : FALSE;
}

//Invalidates all iterators
NTSTATUS
SMBHashResize(
        SMB_HASH_TABLE *pTable,
        size_t sTableSize)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_HASH_ENTRY **ppEntries;
    SMB_HASH_ITERATOR iterator;
    SMB_HASH_ENTRY *pEntry = NULL;
    size_t sBucket;

    ntStatus = LwIoAllocateMemory(
                    sizeof(*ppEntries) * sTableSize,
                    (PVOID*)&ppEntries);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashGetIterator(pTable, &iterator);
    BAIL_ON_NT_STATUS(ntStatus);

    while ((pEntry = SMBHashNext(&iterator)) != NULL)
    {
        sBucket = pTable->fnHash(pEntry->pKey) % sTableSize;
        pEntry->pNext = ppEntries[sBucket];
        ppEntries[sBucket] = pEntry;
    }

    LWIO_SAFE_FREE_MEMORY(pTable->ppEntries);
    pTable->ppEntries = ppEntries;
    pTable->sTableSize = sTableSize;

cleanup:
    return ntStatus;

error:
    LWIO_SAFE_FREE_MEMORY(ppEntries);

    goto cleanup;
}

NTSTATUS
SMBHashGetIterator(
        SMB_HASH_TABLE *pTable,
        SMB_HASH_ITERATOR *pIterator)
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

    return STATUS_SUCCESS;
}

// returns NULL after passing the last entry
SMB_HASH_ENTRY *
SMBHashNext(
        SMB_HASH_ITERATOR *pIterator
        )
{
    SMB_HASH_ENTRY *pRet;

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
SMBHashRemoveKey(
        SMB_HASH_TABLE *pTable,
        PCVOID  pKey)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    size_t sBucket = pTable->fnHash(pKey) % pTable->sTableSize;
    SMB_HASH_ENTRY **ppExamine = &pTable->ppEntries[sBucket];
    SMB_HASH_ENTRY *pDelete;

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
            LWIO_SAFE_FREE_MEMORY(pDelete);
            goto cleanup;
        }

        ppExamine = &(*ppExamine)->pNext;
    }

    //The key isn't in the table yet.
    ntStatus = STATUS_NOT_FOUND;

cleanup:
    return ntStatus;
}

int
SMBHashCaselessStringCompare(
        PCVOID str1,
        PCVOID str2)
{
    return strcasecmp((PCSTR)str1, (PCSTR)str2);
}

int
SMBHashCaselessWc16StringCompare(
        PCVOID str1,
        PCVOID str2)
{
    return !LwRtlWC16StringIsEqual(str1, str2, FALSE);
}

int
SMBHashCompareUINT32(
    PCVOID key1,
    PCVOID key2
    )
{
    uint32_t* pKey1 = (uint32_t*)key1;
    uint32_t* pKey2 = (uint32_t*)key2;

    if (*pKey1 > *pKey2)
    {
        return 1;
    }
    else if (*pKey1 < *pKey2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

size_t
SMBHashCaselessString(
        PCVOID str)
{
    size_t result = 0;
    PCSTR pos = (PCSTR)str;
    int lowerChar;

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
SMBHashCaselessWc16String(
        PCVOID str)
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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
