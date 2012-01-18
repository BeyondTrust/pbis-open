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
 *        lsacache.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Generic Cache
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

static inline
DWORD
LsaCacheKick(
    PLSA_CACHE pCache,
    PVOID pEntry
    )
{
    DWORD dwError = 0;
    PLSA_CACHE_ENTRY pEntryHeader = (PLSA_CACHE_ENTRY) pEntry;

    if (--pEntryHeader->dwRefCount == 0)
    {
        pCache->dwNumKicks++;
        dwError = pCache->pfKick(pEntry, pCache->pData);
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}

static inline
PVOID*
LsaCacheGetBucket(
    PLSA_CACHE pCache,
    DWORD dwHash,
    DWORD dwIndex
    )
{
    return pCache->ppEntries + (dwIndex * pCache->dwNumBuckets) + (dwHash % pCache->dwNumBuckets);
}

DWORD
LsaCacheNew(
    DWORD dwNumKeys,
    DWORD dwNumBuckets,
    PFLSA_CACHE_HASH pfHash,
    PFLSA_CACHE_EQUAL pfEqual,
    PFLSA_CACHE_GETKEY pfGetKey,
    PFLSA_CACHE_MISS pfMiss,
    PFLSA_CACHE_KICK pfKick,
    PVOID pData,
    PLSA_CACHE* ppCache
    )
{
    DWORD dwError = 0;
    PLSA_CACHE pCache = NULL;

    dwError = LwAllocateMemory(sizeof(*pCache), (void**) (void*) &pCache);
    BAIL_ON_LSA_ERROR(dwError);

    pCache->dwNumKeys = dwNumKeys;
    pCache->dwNumBuckets = dwNumBuckets;
    pCache->pfHash = pfHash;
    pCache->pfEqual = pfEqual;
    pCache->pfGetKey = pfGetKey;
    pCache->pfMiss = pfMiss;
    pCache->pfKick = pfKick;
    pCache->pData = pData;

    dwError = LwAllocateMemory(sizeof(PVOID) * dwNumKeys * dwNumBuckets, (void**) (void*) &pCache->ppEntries);
    BAIL_ON_LSA_ERROR(dwError);

    *ppCache = pCache;

cleanup:

    return dwError;

error:

    if (pCache)
    {
        LW_SAFE_FREE_MEMORY(pCache->ppEntries);
        LW_SAFE_FREE_MEMORY(pCache);
    }

    goto cleanup;    
}

static inline
DWORD
LsaCacheInsertKey(
    PLSA_CACHE pCache,
    PVOID pEntry,
    DWORD dwHash,
    DWORD dwIndex
    )
{
    DWORD dwError = 0;
    PVOID* ppBucket = NULL;

    ppBucket = LsaCacheGetBucket(pCache, dwHash, dwIndex);

    if (*ppBucket && *ppBucket != pEntry)
    {
        pCache->dwNumCollisions++;
        dwError = LsaCacheKick(pCache, *ppBucket);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!*ppBucket)
    {
        pCache->dwNumUsedBuckets++;
    }
    
    *ppBucket = pEntry;
    ((PLSA_CACHE_ENTRY) pEntry)->dwRefCount++;

error:

    return dwError;
}

DWORD
LsaCacheInsert(
    PLSA_CACHE pCache,
    PVOID pEntry
    )
{
    DWORD dwError = 0;
    DWORD dwHash = 0;
    DWORD dwIndex = 0;
    PVOID pKey = NULL;

    for (dwIndex = 0; dwIndex < pCache->dwNumKeys; dwIndex++)
    {
        pKey = pCache->pfGetKey(pEntry, dwIndex, pCache->pData);
        if (pKey)
        {
            dwHash = pCache->pfHash(pKey, dwIndex, pCache->pData);
            
            dwError = LsaCacheInsertKey(pCache, pEntry, dwHash, dwIndex);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

error:

    return dwError;
}
    
DWORD
LsaCacheRemove(
    PLSA_CACHE pCache,
    PVOID pEntry
    )
{
    DWORD dwError = 0;
    DWORD dwHash = 0;
    DWORD dwIndex = 0;
    PVOID pKey = NULL;
    PVOID* ppBucket = NULL;


    for (dwIndex = 0; dwIndex < pCache->dwNumKeys; dwIndex++)
    {
        pKey = pCache->pfGetKey(pEntry, dwIndex, pCache->pData);
        if (pKey)
        {
            dwHash = pCache->pfHash(pKey, dwIndex, pCache->pData);
            ppBucket = LsaCacheGetBucket(pCache, dwHash, dwIndex);
            if (*ppBucket == pEntry)
            {
                *ppBucket = NULL;
                pCache->dwNumUsedBuckets--;
            }
        }
    }

    ((PLSA_CACHE_ENTRY) pEntry)->dwRefCount = 0;

    return dwError;
}

DWORD
LsaCacheLookup(
    PLSA_CACHE pCache,
    PVOID pKey,
    DWORD dwIndex,
    PVOID* ppEntry
    )
{
    DWORD dwError = 0;
    DWORD dwHash = 0;
    PVOID* ppBucket = NULL;
    PVOID pEntry = NULL;
    PVOID pEntryKey = NULL;

    *ppEntry = NULL;

    dwHash = pCache->pfHash(pKey, dwIndex, pCache->pData);
    ppBucket = LsaCacheGetBucket(pCache, dwHash, dwIndex);
    
    if (*ppBucket)
    {
        pEntry = *ppBucket;
        pEntryKey = pCache->pfGetKey(pEntry, dwIndex, pCache->pData);
        if (pCache->pfEqual(pEntryKey, pKey, dwIndex, pCache->pData))
        {
            *ppEntry = pEntry;
        }
    }
    
    if (!*ppEntry)
    {
        pCache->dwNumHashMisses++;

        for (ppBucket = pCache->ppEntries;
             ppBucket < pCache->ppEntries + pCache->dwNumKeys * pCache->dwNumBuckets;
             ppBucket++)
        {
            pEntry = *ppBucket;

            if (pEntry)
            {
                pEntryKey = pCache->pfGetKey(pEntry, dwIndex, pCache->pData);
                if (pCache->pfEqual(pEntryKey, pKey, dwIndex, pCache->pData))
                {
                    dwError = LsaCacheInsert(pCache, pEntry);
                    BAIL_ON_LSA_ERROR(dwError);
                    *ppEntry = pEntry;                
                    break;
                }
            }
        }
    }

    if (!*ppEntry && pCache->pfMiss)
    {
        pCache->dwNumFullMisses++;

        dwError = pCache->pfMiss(pKey, dwIndex, pCache->pData, ppEntry);
        BAIL_ON_LSA_ERROR(dwError);

        if (*ppEntry)
        {
            dwError = LsaCacheInsert(pCache, *ppEntry);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

error:

    return dwError;
}

DWORD
LsaCacheFlush(
    PLSA_CACHE pCache
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PVOID pEntry = NULL;

    for (dwIndex = 0;
         dwIndex < pCache->dwNumBuckets * pCache->dwNumKeys;
         dwIndex++)
    {
        pEntry = pCache->ppEntries[dwIndex];

        LsaCacheRemove(pCache, pEntry);

        dwError = pCache->pfKick(pEntry, pCache->pData);
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}

VOID
LsaCacheDelete(
    PLSA_CACHE pCache
    )
{
    /* Flush the cache, ignoring any failures */
    LsaCacheFlush(pCache);

    LW_SAFE_FREE_MEMORY(pCache->ppEntries);
    LW_SAFE_FREE_MEMORY(pCache);
}
