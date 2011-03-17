/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        hash.c
 *
 * Abstract:
 *
 *        Hash table and hash map APIs
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

#define GCOS(status) GOTO_CLEANUP_ON_STATUS(status)
#define RESIZE_THRESHOLD(ulSize) ((ULONG) (((ULONG64) ulSize * 80) / 100))

struct _LW_HASHTABLE
{
    ULONG ulSize;
    ULONG ulThreshold;
    ULONG ulCount;
    PLW_HASHTABLE_NODE* ppBuckets;
    LW_HASH_GET_KEY_FUNCTION pfnGetKey;
    LW_HASH_DIGEST_FUNCTION pfnDigest;
    LW_HASH_EQUAL_FUNCTION pfnEqual;
    PVOID pUserData;
};

static
LW_NTSTATUS
HashLookup(
    PCLW_HASHTABLE pTable,
    PCVOID pKey,
    ULONG ulDigest,
    PLW_HASHTABLE_NODE** pppNode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_HASHTABLE_NODE* ppNode = NULL;
    PLW_HASHTABLE_NODE pNode = NULL;

    for (ppNode = &pTable->ppBuckets[ulDigest % pTable->ulSize];
        *ppNode;
        ppNode = &pNode->pNext)
    {
        pNode = *ppNode;
        if (pNode->ulDigest == ulDigest &&
            pTable->pfnEqual(pKey, pTable->pfnGetKey(pNode, pTable->pUserData), pTable->pUserData))
        {
            goto cleanup;
        }
    }

    status = STATUS_NOT_FOUND;

cleanup:

    *pppNode = ppNode;

    return status;
}

static
LW_NTSTATUS
HashLocate(
    PCLW_HASHTABLE pTable,
    PLW_HASHTABLE_NODE pNode,
    PLW_HASHTABLE_NODE** pppNode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_HASHTABLE_NODE* ppNode = NULL;

    for (ppNode = &pTable->ppBuckets[pNode->ulDigest % pTable->ulSize];
        *ppNode;
        ppNode = &(*ppNode)->pNext)
    {
        if (*ppNode == pNode)
        {
            goto cleanup;
        }
    }

    ppNode = NULL;
    status = STATUS_NOT_FOUND;

cleanup:

    *pppNode = ppNode;

    return status;
}

LW_NTSTATUS
LwRtlCreateHashTable(
    LW_OUT PLW_HASHTABLE* ppTable,
    LW_IN LW_HASH_GET_KEY_FUNCTION pfnGetKey,
    LW_IN LW_HASH_DIGEST_FUNCTION pfnDigest,
    LW_IN LW_HASH_EQUAL_FUNCTION pfnEqual,
    LW_IN PVOID pUserData,
    LW_IN LW_ULONG ulSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_HASHTABLE pTable = NULL;

    if (!ppTable || !pfnGetKey || !pfnDigest || !pfnEqual || ulSize < 1)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE_AUTO(&pTable);
    GCOS(status);

    pTable->pfnGetKey = pfnGetKey;
    pTable->pfnDigest = pfnDigest;
    pTable->pfnEqual = pfnEqual;
    pTable->pUserData = pUserData;
    pTable->ulSize = ulSize;
    pTable->ulThreshold = RESIZE_THRESHOLD(ulSize);

    status = LW_RTL_ALLOCATE_ARRAY_AUTO(&pTable->ppBuckets, ulSize);
    GCOS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        LwRtlFreeHashTable(&pTable);
    }

    if (ppTable)
    {
        *ppTable = pTable;
    }

    return status;
}

VOID
LwRtlHashTableInsert(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_IN PLW_HASHTABLE_NODE pNode,
    LW_OUT LW_OPTIONAL PLW_HASHTABLE_NODE* ppPrevNode
    )
{
    PCVOID pKey = pTable->pfnGetKey(pNode, pTable->pUserData);
    NTSTATUS status = STATUS_SUCCESS;
    PLW_HASHTABLE_NODE* ppNode = NULL;

    pNode->ulDigest = pTable->pfnDigest(pKey, pTable->pUserData);

    status = HashLookup(pTable, pKey, pNode->ulDigest, &ppNode);
    if (status == STATUS_SUCCESS)
    {
        if (ppPrevNode)
        {
            *ppPrevNode = *ppNode;
        }

        pNode->pNext = (*ppNode)->pNext;
        *ppNode = pNode;
    }
    else
    {
        pNode->pNext = NULL;
        *ppNode = pNode;
        pTable->ulCount++;
    }
}

VOID
LwRtlHashTableResizeAndInsert(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_IN PLW_HASHTABLE_NODE pNode,
    LW_OUT LW_OPTIONAL PLW_HASHTABLE_NODE* ppPrevNode
    )
{
    if (pTable->ulCount >= pTable->ulThreshold)
    {
        LwRtlHashTableResize(pTable, pTable->ulSize * 2);
    }

    LwRtlHashTableInsert(pTable, pNode, ppPrevNode);
}

LW_NTSTATUS
LwRtlHashTableRemove(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_IN PLW_HASHTABLE_NODE pNode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_HASHTABLE_NODE* ppNode = NULL;

    if (!pTable || !pNode)
    {
        status = STATUS_INVALID_PARAMETER;
        GCOS(status);
    }

    status = HashLocate(pTable, pNode, &ppNode);
    GCOS(status);

    *ppNode = pNode->pNext;
    pTable->ulCount--;

cleanup:

    return status;
}

LW_NTSTATUS
LwRtlHashTableFindKey(
    LW_IN PCLW_HASHTABLE pTable,
    LW_OUT LW_OPTIONAL PLW_HASHTABLE_NODE* ppNode,
    LW_IN PCVOID pKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulDigest = 0;
    PLW_HASHTABLE_NODE* ppFound = NULL;
    PLW_HASHTABLE_NODE pNode = NULL;

    if (!pTable)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    ulDigest = pTable->pfnDigest(pKey, pTable->pUserData);

    status = HashLookup(pTable, pKey, ulDigest, &ppFound);
    GCOS(status);

    pNode = *ppFound;

cleanup:

    if (ppNode)
    {
        *ppNode = pNode;
    }

    return status;
}

VOID
LwRtlHashTableResetIter(
    LW_OUT PLW_HASHTABLE_ITER pIter
    )
{
    pIter->pNext = NULL;
    pIter->ulIndex = 0;
}

PLW_HASHTABLE_NODE
LwRtlHashTableIterate(
    LW_IN PCLW_HASHTABLE pTable,
    LW_IN LW_OUT PLW_HASHTABLE_ITER pIter
    )
{
    PLW_HASHTABLE_NODE pNode = NULL;

    if (pIter->pNext)
    {
        pNode = pIter->pNext;
        pIter->pNext = pNode->pNext;
    }
    else
    {
        for (; pIter->ulIndex < pTable->ulSize; pIter->ulIndex++)
        {
            if (pTable->ppBuckets[pIter->ulIndex])
            {
                pNode = pTable->ppBuckets[pIter->ulIndex++];
                pIter->pNext = pNode->pNext;
                break;
            }
        }
    }

    return pNode;
}

ULONG
LwRtlHashTableGetSize(
    LW_IN PCLW_HASHTABLE pTable
    )
{
    return pTable->ulSize;
}

ULONG
LwRtlHashTableGetCount(
    LW_IN PCLW_HASHTABLE pTable
    )
{
    return pTable->ulCount;
}


LW_NTSTATUS
LwRtlHashTableResize(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    ULONG ulSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulOldSize = 0;
    ULONG ulOldCount = 0;
    ULONG ulIndex = 0;
    PLW_HASHTABLE_NODE* ppOldBuckets = NULL;
    PLW_HASHTABLE_NODE pNode = NULL;
    PLW_HASHTABLE_NODE pNext = NULL;

    if (!pTable || ulSize < 1)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    ulOldCount = pTable->ulCount;
    ulOldSize = pTable->ulSize;
    ppOldBuckets = pTable->ppBuckets;

    pTable->ppBuckets = NULL;
    pTable->ulSize = ulSize;
    pTable->ulCount = 0;

    status = LW_RTL_ALLOCATE_ARRAY_AUTO(&pTable->ppBuckets, ulSize);
    if (!NT_SUCCESS(status))
    {
        pTable->ppBuckets = ppOldBuckets;
        pTable->ulSize = ulOldSize;
        pTable->ulCount = ulOldCount;
    }
    GCOS(status);

    for (ulIndex = 0; ulIndex < ulOldSize; ulIndex++)
    {
        for (pNode = ppOldBuckets[ulIndex]; pNode; pNode = pNext)
        {
            pNext = pNode->pNext;
            LwRtlHashTableInsert(pTable, pNode, NULL);
        }
    }

    pTable->ulThreshold = RESIZE_THRESHOLD(ulSize);

    RTL_FREE(&ppOldBuckets);

cleanup:

    return status;
}

VOID
LwRtlHashTableClear(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_IN LW_HASHNODE_FREE_FUNCTION pFree,
    LW_IN PVOID pUserData
    )
{
    LW_HASHTABLE_ITER iter = LW_HASHTABLE_ITER_INIT;
    PLW_HASHTABLE_NODE pNode = NULL;

    if (pFree)
    {
        while ((pNode = LwRtlHashTableIterate(pTable, &iter)))
        {
            pFree(pNode, pUserData);
        }
    }

    memset(pTable->ppBuckets, 0, sizeof(*pTable->ppBuckets) * pTable->ulSize);
    pTable->ulCount = 0;
}

VOID
LwRtlFreeHashTable(
    LW_IN LW_OUT PLW_HASHTABLE* ppTable
    )
{
    if (ppTable)
    {
        if (*ppTable)
        {
            RTL_FREE(&(*ppTable)->ppBuckets);
        }
        RTL_FREE(ppTable);
    }
}

typedef struct _HASHPAIR_NODE
{
    LW_HASHTABLE_NODE Node;
    LW_HASHMAP_PAIR Pair;
} HASHPAIR_NODE, *PHASHPAIR_NODE;

typedef struct _HASHMAP_CLEAR_INFO
{
    LW_HASHPAIR_FREE_FUNCTION pFree;
    PVOID pUserData;
} HASHMAP_CLEAR_INFO, *PHASHMAP_CLEAR_INFO;

static
PCVOID
HashPairNodeGetKey(
    PLW_HASHTABLE_NODE pNode,
    PVOID pUnused
    )
{
    return ((PHASHPAIR_NODE) pNode)->Pair.pKey;
}

static
VOID
HashPairFreeNode(
    PLW_HASHTABLE_NODE pNode,
    PVOID pData
    )
{
    PHASHMAP_CLEAR_INFO pInfo = (PHASHMAP_CLEAR_INFO) pData;

    pInfo->pFree(&((PHASHPAIR_NODE) pNode)->Pair, pInfo->pUserData);

    RTL_FREE(&pNode);
}

LW_NTSTATUS
LwRtlCreateHashMap(
    LW_OUT PLW_HASHMAP* ppMap,
    LW_IN LW_HASH_DIGEST_FUNCTION pfnDigest,
    LW_IN LW_HASH_EQUAL_FUNCTION pfnEqual,
    LW_IN PVOID pUserData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_HASHTABLE pTable = NULL;

    status = LwRtlCreateHashTable(
        &pTable,
        HashPairNodeGetKey,
        pfnDigest,
        pfnEqual,
        pUserData,
        17);
    GCOS(status);

cleanup:

    if (ppMap)
    {
        *ppMap = (PLW_HASHMAP) pTable;
    }

    return status;
}

LW_NTSTATUS
LwRtlHashMapInsert(
    LW_IN LW_OUT PLW_HASHMAP pMap,
    LW_IN PVOID pKey,
    LW_IN PVOID pValue,
    LW_OUT LW_OPTIONAL PLW_HASHMAP_PAIR pPrevPair
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PHASHPAIR_NODE pPairNode = NULL;
    PLW_HASHTABLE_NODE pPrevNode = NULL;

    if (!pMap)
    {
        status = STATUS_INVALID_PARAMETER;
        GCOS(status);
    }

    status = LW_RTL_ALLOCATE_AUTO(&pPairNode);
    GCOS(status);

    pPairNode->Pair.pKey = pKey;
    pPairNode->Pair.pValue = pValue;

    LwRtlHashTableResizeAndInsert((PLW_HASHTABLE) pMap, &pPairNode->Node, &pPrevNode);

cleanup:

    if (pPrevPair)
    {
        if (pPrevNode)
        {
            *pPrevPair = ((PHASHPAIR_NODE) pPrevNode)->Pair;
        }
        else
        {
            memset(pPrevPair, 0, sizeof(*pPrevPair));
        }
    }

    RTL_FREE(&pPrevNode);

    return status;
}

LW_NTSTATUS
LwRtlHashMapRemove(
    LW_IN LW_OUT PLW_HASHMAP pMap,
    LW_IN PCVOID pKey,
    LW_OUT LW_OPTIONAL PLW_HASHMAP_PAIR pPair
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_HASHTABLE_NODE pNode = NULL;

    if (!pMap)
    {
        status = STATUS_INVALID_PARAMETER;
        GCOS(status);
    }

    status = LwRtlHashTableFindKey((PLW_HASHTABLE) pMap, &pNode, pKey);
    GCOS(status);

    status = LwRtlHashTableRemove((PLW_HASHTABLE) pMap, pNode);
    GCOS(status);

cleanup:

    if (pPair)
    {
        if (pNode)
        {
            *pPair = ((PHASHPAIR_NODE) pNode)->Pair;
        }
        else
        {
            memset(pPair, 0, sizeof(*pPair));
        }
    }

    RTL_FREE(&pNode);

    return status;
}

LW_NTSTATUS
LwRtlHashMapFindKey(
    LW_IN PCLW_HASHMAP pMap,
    LW_OUT OPTIONAL PVOID* ppValue,
    LW_IN PCVOID pKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_HASHTABLE_NODE pNode = NULL;

    if (!pMap)
    {
        status = STATUS_INVALID_PARAMETER;
        GCOS(status);
    }

    status = LwRtlHashTableFindKey((PLW_HASHTABLE) pMap, &pNode, pKey);
    GCOS(status);

cleanup:

    if (ppValue)
    {
        if (pNode)
        {
            *ppValue = ((PHASHPAIR_NODE) pNode)->Pair.pValue;
        }
        else
        {
            *ppValue = NULL;
        }
    }

    return status;
}

VOID
LwRtlHashMapResetIter(
    LW_OUT PLW_HASHMAP_ITER pIter
    )
{
    LwRtlHashTableResetIter(&pIter->Inner);
}

BOOLEAN
LwRtlHashMapIterate(
    LW_IN PCLW_HASHMAP pMap,
    LW_IN LW_OUT PLW_HASHMAP_ITER pIter,
    LW_OUT PLW_HASHMAP_PAIR pPair
    )
{
    PLW_HASHTABLE_NODE pNode = LwRtlHashTableIterate((PLW_HASHTABLE) pMap, &pIter->Inner);

    if (pNode)
    {
        *pPair = ((PHASHPAIR_NODE) pNode)->Pair;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID
LwRtlHashMapClear(
    LW_IN LW_OUT PLW_HASHMAP pMap,
    LW_IN LW_HASHPAIR_FREE_FUNCTION pFree,
    LW_IN PVOID pUserData
    )
{
    HASHMAP_CLEAR_INFO info = {NULL, NULL};

    info.pFree = pFree;
    info.pUserData = pUserData;

    LwRtlHashTableClear((PLW_HASHTABLE) pMap, HashPairFreeNode, &info);
}

ULONG
LwRtlHashMapGetCount(
    LW_IN PCLW_HASHMAP pMap
    )
{
    return LwRtlHashTableGetCount((PLW_HASHTABLE) pMap);
}

VOID
LwRtlFreeHashMap(
    LW_IN LW_OUT PLW_HASHMAP* ppMap
    )
{
    PLW_HASHTABLE pTable = NULL;

    if (ppMap && *ppMap)
    {
        LwRtlHashMapClear(*ppMap, NULL, NULL);
        pTable = (PLW_HASHTABLE) *ppMap;
        LwRtlFreeHashTable(&pTable);
        *ppMap = NULL;
    }
}

LW_ULONG
LwRtlHashDigestPstr(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    )
{
    PCSTR pszStr = (PCSTR) pKey;
    ULONG ulDigest = 0;

    if (pszStr)
    {
        while (*pszStr)
        {
            ulDigest = ulDigest * 31 + *(pszStr++);
        }
    }

    return ulDigest;
}

LW_BOOLEAN
LwRtlHashEqualPstr(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    )
{
    PCSTR pszStr1 = (PCSTR) pKey1;
    PCSTR pszStr2 = (PCSTR) pKey2;

    return ((pszStr1 == NULL && pszStr2 == NULL) ||
            (pszStr1 != NULL && pszStr2 != NULL &&
             LwRtlCStringIsEqual(pszStr1, pszStr2, TRUE)));
}

LW_ULONG
LwRtlHashDigestPstrCaseless(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    )
{
    PCSTR pszStr = (PCSTR) pKey;
    ULONG ulDigest = 0;

    if (pszStr)
    {
        while (*pszStr)
        {
            ulDigest = ulDigest * 31 + toupper((int) *(pszStr++));
        }
    }

    return ulDigest;
}

LW_BOOLEAN
LwRtlHashEqualPstrCaseless(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    )
{
    PCSTR pszStr1 = (PCSTR) pKey1;
    PCSTR pszStr2 = (PCSTR) pKey2;

    return ((pszStr1 == NULL && pszStr2 == NULL) ||
            (pszStr1 != NULL && pszStr2 != NULL &&
             LwRtlCStringIsEqual(pszStr1, pszStr2, FALSE)));
}

LW_ULONG
LwRtlHashDigestPwstr(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    )
{
    PCWSTR pwszStr = (PCWSTR) pKey;
    ULONG ulDigest = 0;

    if (pwszStr)
    {
        while (*pwszStr)
        {
            ulDigest = ulDigest * 31 + *(pwszStr++);
        }
    }

    return ulDigest;
}

LW_BOOLEAN
LwRtlHashEqualPwstr(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    )
{
    PCWSTR pwszStr1 = (PCWSTR) pKey1;
    PCWSTR pwszStr2 = (PCWSTR) pKey2;

    return ((pwszStr1 == NULL && pwszStr2 == NULL) ||
            (pwszStr1 != NULL && pwszStr2 != NULL &&
             LwRtlWC16StringIsEqual(pwszStr1, pwszStr2, TRUE)));
}

static
WCHAR
ToUpper(
    WCHAR c
    )
{
    if (c >= 0x61 && c <= 0x7A)
    {
        return c - 0x20;
    }
    else
    {
        return c;
    }
}

LW_ULONG
LwRtlHashDigestPwstrCaseless(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    )
{
    PCWSTR pwszStr = (PCWSTR) pKey;
    ULONG ulDigest = 0;

    if (pwszStr)
    {
        while (*pwszStr)
        {
            ulDigest = ulDigest * 31 + ToUpper(*(pwszStr++));
        }
    }

    return ulDigest;
}

LW_BOOLEAN
LwRtlHashEqualPwstrCaseless(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    )
{
    PCWSTR pwszStr1 = (PCWSTR) pKey1;
    PCWSTR pwszStr2 = (PCWSTR) pKey2;

    return ((pwszStr1 == NULL && pwszStr2 == NULL) ||
            (pwszStr1 != NULL && pwszStr2 != NULL &&
             LwRtlWC16StringIsEqual(pwszStr1, pwszStr2, FALSE)));
}

LW_ULONG
LwRtlHashDigestPointer(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    )
{
    /*
     * Since most pointers are at least 32-bit aligned, shift
     * off the lower two bits of the address since they will
     * most likely be 0 and won't contribute entropy to the digest
     */
    return (ULONG) (((size_t) pKey) >> 2);
}

LW_BOOLEAN
LwRtlHashEqualPointer(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    )
{
    return pKey1 == pKey2;
}
