/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        offline-helper.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */
#include "adprovider.h"

DWORD
AD_OfflineGetGroupMembers(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszGroupSid,
    OUT size_t* psMemberObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppMemberObjects
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sGroupMembershipsCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppGroupMemberships = NULL;
    size_t sMemberSidsCount = 0;
    // Only free top level array, do not free string pointers as they
    // track elements inside ppMemberships.
    PSTR* ppszMemberSids = NULL;
    size_t sObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    size_t sIndex = 0;

    dwError = ADCacheGetGroupMembers(
        pState->hCacheConnection,
        pszGroupSid,
        AD_GetTrimUserMembershipEnabled(pState),
        &sGroupMembershipsCount,
        &ppGroupMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
        sizeof(*ppszMemberSids) * sGroupMembershipsCount,
        (PVOID*)&ppszMemberSids);
    BAIL_ON_LSA_ERROR(dwError);

    sMemberSidsCount = 0;
    for (sIndex = 0; sIndex < sGroupMembershipsCount; sIndex++)
    {
        if (ppGroupMemberships[sIndex]->pszChildSid)
        {
            ppszMemberSids[sMemberSidsCount++] = ppGroupMemberships[sIndex]->pszChildSid;
        }
    }

    dwError = AD_OfflineFindObjectsBySidList(
        pState,
        sMemberSidsCount,
        ppszMemberSids,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    sObjectsCount = sMemberSidsCount;
    AD_FilterNullEntries(ppObjects, &sObjectsCount);

    *psMemberObjectsCount = sObjectsCount;
    *pppMemberObjects = ppObjects;

    ppObjects = NULL;
    sObjectsCount = 0;

cleanup:
    ADCacheSafeFreeObjectList(sObjectsCount, &ppObjects);
    LW_SAFE_FREE_MEMORY(ppszMemberSids);
    ADCacheSafeFreeGroupMembershipList(sGroupMembershipsCount,
                                          &ppGroupMemberships);

    return dwError;

error:
    *psMemberObjectsCount = 0;
    *pppMemberObjects = NULL;

    goto cleanup;
}

DWORD
AD_OfflineFindObjectsBySidList(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT *ppObjects = NULL;

    /* 
     * Lookup users and groups from the cache.
     */

    dwError = ADCacheFindObjectsBySidList(
                    pState->hCacheConnection,
                    sCount,
                    ppszSidList,
                    &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    *pppObjects = ppObjects;
    ppObjects = NULL;

cleanup:
    ADCacheSafeFreeObjectList(sCount, &ppObjects);
    return dwError;

error:
    *pppObjects = NULL;
    goto cleanup;
}

DWORD
AD_GatherSidsFromGroupMemberships(
    IN BOOLEAN bGatherParentSids,
    IN OPTIONAL PFN_LSA_GATHER_SIDS_FROM_GROUP_MEMBERSHIP_CALLBACK pfnIncludeCallback,
    IN size_t sMemberhipsCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMemberships,
    OUT size_t* psSidsCount,
    OUT PSTR** pppszSids
    )
{
    // NOTE: The result points to the sids inside the memberships.
    // Do not deallocate the memberhips while using the result.
    // Call LW_SAFE_FREE_MEMORY() on result when done so as to
    // not free up the sids (which reside in the memberships).
    DWORD dwError = LW_ERROR_SUCCESS;
    // Do not free actual strings, just the array.
    PSTR* ppszSids = NULL;
    size_t sSidsCount = 0;
    size_t sIndex = 0;
    size_t sOldSidsCount = 0;

    for (;;)
    {
        sSidsCount = 0;
        for (sIndex = 0; sIndex < sMemberhipsCount; sIndex++)
        {
            PLSA_GROUP_MEMBERSHIP pMembership = ppMemberships[sIndex];
            PSTR pszSid = NULL;

            if (!pMembership)
            {
                continue;
            }

            if (pfnIncludeCallback)
            {
                if (!pfnIncludeCallback(pMembership))
                {
                    continue;
                }
            }

            if (bGatherParentSids)
            {
                pszSid = pMembership->pszParentSid;
            }
            else
            {
                pszSid = pMembership->pszChildSid;
            }

            if (pszSid)
            {
                if (ppszSids)
                {
                    ppszSids[sSidsCount] = pszSid;
                }
                sSidsCount++;
            }
        }

        if (ppszSids)
        {
            // Done.
            assert(sOldSidsCount == sSidsCount);
            break;
        }

        if (sSidsCount < 1)
        {
            // Nothing to do.
            break;
        }

        // Allocate memory so we can gather up stuff.
        dwError = LwAllocateMemory(sizeof(*ppszSids) * sMemberhipsCount,
                                    (PVOID*)&ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        // Remember so that we can ASSERT.
        sOldSidsCount = sSidsCount;
    }


cleanup:
    *pppszSids = ppszSids;
    *psSidsCount = sSidsCount;
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(ppszSids);
    sSidsCount = 0;
    goto cleanup;
}

struct _LSA_AD_GROUP_EXPANSION_DATA {
    PLSA_HASH_TABLE pGroupsToExpand;
    PLSA_HASH_TABLE pExpandedGroups;
    PLSA_HASH_TABLE pUsers;
    LSA_HASH_ITERATOR GroupsToExpandIterator;
    BOOLEAN bIsIteratorInitialized;
    BOOLEAN bDiscardedDueToDepth;
    DWORD dwMaxDepth;
    DWORD dwLastError;
};

DWORD
AD_GroupExpansionDataCreate(
    OUT PLSA_AD_GROUP_EXPANSION_DATA* ppExpansionData,
    IN DWORD dwMaxDepth
    )
{
    DWORD dwError = 0;
    PLSA_AD_GROUP_EXPANSION_DATA pExpansionData = NULL;
    const size_t sNumberOfBuckets = 20;

    dwError = LwAllocateMemory(
                sizeof(*pExpansionData),
                (PVOID*) &pExpansionData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                sNumberOfBuckets,
                AD_CompareObjectSids,
                AD_HashObjectSid,
                NULL,
                NULL,
                &pExpansionData->pGroupsToExpand);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                sNumberOfBuckets,
                AD_CompareObjectSids,
                AD_HashObjectSid,
                NULL,
                NULL,
                &pExpansionData->pExpandedGroups);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                sNumberOfBuckets,
                AD_CompareObjectSids,
                AD_HashObjectSid,
                NULL,
                NULL,
                &pExpansionData->pUsers);
    BAIL_ON_LSA_ERROR(dwError);

    pExpansionData->dwMaxDepth = dwMaxDepth;

    *ppExpansionData = pExpansionData;

cleanup:
    return dwError;

error:
    *ppExpansionData = NULL;
    AD_GroupExpansionDataDestroy(pExpansionData);
    goto cleanup;
}

DWORD
AD_GroupExpansionDataAddExpansionResults(
    IN PLSA_AD_GROUP_EXPANSION_DATA pExpansionData,
    IN DWORD dwExpandedGroupDepth,
    IN OUT size_t* psMembersCount,
    IN OUT PLSA_SECURITY_OBJECT** pppMembers
    )
{
    DWORD dwError = 0;
    size_t sMembersCount = *psMembersCount;
    PLSA_SECURITY_OBJECT* ppMembers = *pppMembers;

    dwError = pExpansionData->dwLastError;
    BAIL_ON_LSA_ERROR(dwError);

    if (dwExpandedGroupDepth > pExpansionData->dwMaxDepth)
    {
        // This should never happen
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((sMembersCount + pExpansionData->pUsers->sCount) * 2 >
            pExpansionData->pUsers->sTableSize)
    {
        dwError = LsaHashResize(
                        pExpansionData->pUsers,
                        (sMembersCount +
                             pExpansionData->pUsers->sCount + 10) * 3);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((sMembersCount + pExpansionData->pGroupsToExpand->sCount) * 2 >
            pExpansionData->pGroupsToExpand->sTableSize)
    {
        dwError = LsaHashResize(
                        pExpansionData->pGroupsToExpand,
                        (sMembersCount +
                             pExpansionData->pGroupsToExpand->sCount + 10) * 3);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (; sMembersCount > 0; sMembersCount--)
    {
        PLSA_SECURITY_OBJECT pCurrentMember = ppMembers[sMembersCount-1];

        if (!pCurrentMember)
        {
            continue;
        }

        if (pCurrentMember->type == LSA_OBJECT_TYPE_USER)
        {
            if (!LsaHashExists(pExpansionData->pUsers,
                               ppMembers[sMembersCount-1]))
            {
                dwError = LsaHashSetValue(
                    pExpansionData->pUsers,
                    ppMembers[sMembersCount-1],
                    (PVOID)(size_t)dwExpandedGroupDepth);
                BAIL_ON_LSA_ERROR(dwError);
                ppMembers[sMembersCount-1] = NULL;
            }
            else
            {
                ADCacheSafeFreeObject(&ppMembers[sMembersCount-1]);
            }
        }
        else if (pCurrentMember->type == LSA_OBJECT_TYPE_GROUP)
        {
            if (dwExpandedGroupDepth >= pExpansionData->dwMaxDepth)
            {
                pExpansionData->bDiscardedDueToDepth = TRUE;
                ADCacheSafeFreeObject(&ppMembers[sMembersCount-1]);
            }
            else if (LsaHashExists(pExpansionData->pExpandedGroups,
                                   pCurrentMember) ||
                     LsaHashExists(pExpansionData->pGroupsToExpand,
                                   pCurrentMember))
            {
                ADCacheSafeFreeObject(&ppMembers[sMembersCount-1]);
            }
            else
            {
                dwError = LsaHashSetValue(
                            pExpansionData->pGroupsToExpand,
                            ppMembers[sMembersCount-1],
                            (PVOID)(size_t)dwExpandedGroupDepth);
                BAIL_ON_LSA_ERROR(dwError);
                ppMembers[sMembersCount-1] = NULL;
            }
        }
        else
        {
            // some other kind of object -- should not happen
            ADCacheSafeFreeObject(&ppMembers[sMembersCount-1]);
        }
    }

cleanup:
    if (ppMembers && (sMembersCount == 0))
    {
        ADCacheSafeFreeObjectList(sMembersCount, &ppMembers);
    }
    *psMembersCount = sMembersCount;
    *pppMembers = ppMembers;
    return dwError;

error:
    ADCacheSafeFreeObjectList(sMembersCount, &ppMembers);
    if (dwError && !pExpansionData->dwLastError)
    {
        pExpansionData->dwLastError = dwError;
    }
    goto cleanup;
}

DWORD
AD_GroupExpansionDataGetNextGroupToExpand(
    IN PLSA_AD_GROUP_EXPANSION_DATA pExpansionData,
    OUT PLSA_SECURITY_OBJECT* ppGroupToExpand,
    OUT PDWORD pdwGroupToExpandDepth
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pGroupToExpand = NULL;
    DWORD dwGroupToExpandDepth = 0;
    const LSA_HASH_ENTRY* pHashEntry = NULL;

    dwError = pExpansionData->dwLastError;
    BAIL_ON_LSA_ERROR(dwError);

    if (pExpansionData->pGroupsToExpand->sCount < 1)
    {
        // Nothing to return
        goto cleanup;
    }

    if (pExpansionData->bIsIteratorInitialized)
    {
        pHashEntry = LsaHashNext(&pExpansionData->GroupsToExpandIterator);
    }

    if (!pHashEntry)
    {
        // Either the iterator is not initialized or we
        // reached the end of the hash table and need to start over.
        dwError = LsaHashGetIterator(
                    pExpansionData->pGroupsToExpand,
                    &pExpansionData->GroupsToExpandIterator);
        BAIL_ON_LSA_ERROR(dwError);

        pExpansionData->bIsIteratorInitialized = TRUE;

        pHashEntry = LsaHashNext(&pExpansionData->GroupsToExpandIterator);
        if (!pHashEntry)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    pGroupToExpand = (PLSA_SECURITY_OBJECT) pHashEntry->pKey;
    dwGroupToExpandDepth = (size_t) pHashEntry->pValue;
    dwGroupToExpandDepth++;

    // Move the object to the expanded list.  Note that the object is
    // not necessarily expanded yet, but we must remove it from
    // the "to expand" list.  It does not hurt to track it in the
    // "expanded" list.

    dwError = LsaHashSetValue(pExpansionData->pExpandedGroups,
                              pGroupToExpand,
                              (PVOID)(size_t)dwGroupToExpandDepth);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashRemoveKey(pExpansionData->pGroupsToExpand, pGroupToExpand);
    if (dwError)
    {
        LSA_LOG_DEBUG("ASSERT: cannot fail");
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppGroupToExpand = pGroupToExpand;
    *pdwGroupToExpandDepth = dwGroupToExpandDepth;

    return dwError;

error:
    ADCacheSafeFreeObject(&pGroupToExpand);
    dwGroupToExpandDepth = 0;

    if (dwError && !pExpansionData->dwLastError)
    {
        pExpansionData->dwLastError = dwError;
    }
    goto cleanup;
}

DWORD
AD_GroupExpansionDataGetResults(
    IN PLSA_AD_GROUP_EXPANSION_DATA pExpansionData,
    OUT OPTIONAL PBOOLEAN pbIsFullyExpanded,
    OUT size_t* psUserMembersCount,
    OUT PLSA_SECURITY_OBJECT** pppUserMembers
    )
{
    DWORD dwError = 0;
    LSA_HASH_ITERATOR hashIterator;
    LSA_HASH_ENTRY* pHashEntry = NULL;
    size_t sHashCount = 0;
    PLSA_SECURITY_OBJECT* ppUserMembers = NULL;
    size_t sUserMembersCount = 0;
    BOOLEAN bIsFullyExpanded = FALSE;

    dwError = pExpansionData->dwLastError;
    BAIL_ON_LSA_ERROR(dwError);

    // Fill in the final list of users and return it.
    sHashCount = pExpansionData->pUsers->sCount;
    dwError = LwAllocateMemory(
                sizeof(*ppUserMembers) * sHashCount,
                (PVOID*)&ppUserMembers);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashGetIterator(pExpansionData->pUsers, &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    for (sUserMembersCount = 0;
         (pHashEntry = LsaHashNext(&hashIterator)) != NULL;
         sUserMembersCount++)
    {
        PLSA_SECURITY_OBJECT pUser = (PLSA_SECURITY_OBJECT) pHashEntry->pKey;

        dwError = LsaHashRemoveKey(pExpansionData->pUsers, pUser);
        BAIL_ON_LSA_ERROR(dwError);

        ppUserMembers[sUserMembersCount] = pUser;
    }

    if (sUserMembersCount != sHashCount)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pExpansionData->bDiscardedDueToDepth &&
        (pExpansionData->pGroupsToExpand->sCount == 0))
    {
        bIsFullyExpanded = TRUE;
    }

cleanup:
    if (pbIsFullyExpanded)
    {
        *pbIsFullyExpanded = bIsFullyExpanded;
    }

    *psUserMembersCount = sUserMembersCount;
    *pppUserMembers = ppUserMembers;

    return dwError;

error:
    ADCacheSafeFreeObjectList(sUserMembersCount, &ppUserMembers);
    sUserMembersCount = 0;

    if (dwError && !pExpansionData->dwLastError)
    {
        pExpansionData->dwLastError = dwError;
    }
    goto cleanup;
}

VOID
AD_GroupExpansionDataDestroy(
    IN OUT PLSA_AD_GROUP_EXPANSION_DATA pExpansionData
    )
{
    if (pExpansionData)
    {
        if (pExpansionData->pGroupsToExpand)
        {
            pExpansionData->pGroupsToExpand->fnFree = AD_FreeHashObject;
        }
        if (pExpansionData->pExpandedGroups)
        {
            pExpansionData->pExpandedGroups->fnFree = AD_FreeHashObject;
        }
        if (pExpansionData->pUsers)
        {
            pExpansionData->pUsers->fnFree = AD_FreeHashObject;
        }
        LsaHashSafeFree(&pExpansionData->pGroupsToExpand);
        LsaHashSafeFree(&pExpansionData->pExpandedGroups);
        LsaHashSafeFree(&pExpansionData->pUsers);
        LwFreeMemory(pExpansionData);
    }
}

