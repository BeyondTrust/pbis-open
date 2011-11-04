/*
 * Copyright Likewise Software
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
 * Module Name:
 *
 *        api2.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Client API (version 2)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include "client.h"

DWORD
LsaFindObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    return LsaTransactFindObjects(
        hLsa,
        pszTargetProvider,
        FindFlags,
        ObjectType,
        QueryType,
        dwCount,
        QueryList,
        pppObjects);
}

DWORD
LsaOpenEnumObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    return LsaTransactOpenEnumObjects(
        hLsa,
        pszTargetProvider,
        phEnum,
        FindFlags,
        ObjectType,
        pszDomainName);
}

DWORD
LsaEnumObjects(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    return LsaTransactEnumObjects(
        hLsa,
        hEnum,
        dwMaxObjectsCount,
        pdwObjectsCount,
        pppObjects);
}

DWORD
LsaOpenEnumMembers(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
    return LsaTransactOpenEnumMembers(
        hLsa,
        pszTargetProvider,
        phEnum,
        FindFlags,
        pszSid);
}

DWORD
LsaEnumMembers(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PSTR** pppszMember
    )
{
    return LsaTransactEnumMembers(
        hLsa,
        hEnum,
        dwMaxObjectsCount,
        pdwObjectsCount,
        pppszMember);
}

DWORD
LsaQueryMemberOf(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    return LsaTransactQueryMemberOf(
        hLsa,
        pszTargetProvider,
        FindFlags,
        dwSidCount,
        ppszSids,
        pdwGroupSidCount,
        pppszGroupSids);
}

DWORD
LsaCloseEnum(
    IN HANDLE hLsa,
    IN OUT HANDLE hEnum
    )
{
    return LsaTransactCloseEnum(
        hLsa,
        hEnum);
}

VOID
LsaFreeSidList(
    IN DWORD dwSidCount,
    IN OUT PSTR* ppszSids
    )
{
    LwFreeStringArray(ppszSids, dwSidCount);
}

VOID
LsaFreeSecurityObjectList(
    IN DWORD dwObjectCount,
    IN OUT PLSA_SECURITY_OBJECT* ppObjects
    )
{
    LsaUtilFreeSecurityObjectList(dwObjectCount, ppObjects);
}

VOID
LsaFreeSecurityObject(
    IN OUT PLSA_SECURITY_OBJECT pObject
    )
{
    LsaUtilFreeSecurityObject(pObject);
}

static
VOID
LsaFreeMemberHashEntry(
    const LW_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        LsaFreeSecurityObject(pEntry->pValue);
    }
}

static
DWORD
LsaQueryExpandedGroupMembersInternal(
    IN HANDLE hLsa,
    PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN PCSTR pszSid,
    IN OUT PLW_HASH_TABLE pHash
    )
{
    DWORD dwError = 0;
    HANDLE hEnum = NULL;
    static const DWORD dwMaxEnumCount = 512;
    DWORD dwEnumCount = 0;
    PSTR* ppszSids = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    DWORD dwIndex = 0;

    dwError = LsaOpenEnumMembers(
        hLsa,
        pszTargetProvider,
        &hEnum,
        FindFlags,
        pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    for (;;)
    {
        dwError = LsaEnumMembers(
            hLsa,
            hEnum,
            dwMaxEnumCount,
            &dwEnumCount,
            &ppszSids);
        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            dwError = 0;
            break;
        }
        BAIL_ON_LSA_ERROR(dwError);


        if ((pHash->sCount + dwEnumCount) * 2 > pHash->sTableSize)
        {
            dwError = LwHashResize(
                pHash,
                (pHash->sCount + dwEnumCount + 10) * 4);
            BAIL_ON_LSA_ERROR(dwError);
        }

        QueryList.ppszStrings = (PCSTR*) ppszSids;

        dwError = LsaFindObjects(
            hLsa,
            pszTargetProvider,
            FindFlags,
            LSA_OBJECT_TYPE_UNDEFINED,
            LSA_QUERY_TYPE_BY_SID,
            dwEnumCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        for (dwIndex = 0; dwIndex < dwEnumCount; dwIndex++)
        {
            if (ppObjects[dwIndex] && !LwHashExists(pHash, ppObjects[dwIndex]->pszObjectSid))
            {
                dwError = LwHashSetValue(
                    pHash,
                    ppObjects[dwIndex]->pszObjectSid,
                    ppObjects[dwIndex]);
                BAIL_ON_LSA_ERROR(dwError);

                if (ppObjects[dwIndex]->type == LSA_OBJECT_TYPE_GROUP)
                {
                    dwError = LsaQueryExpandedGroupMembersInternal(
                        hLsa,
                        pszTargetProvider,
                        FindFlags,
                        ObjectType,
                        ppObjects[dwIndex]->pszObjectSid,
                        pHash);
                    ppObjects[dwIndex] = NULL;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                else
                {
                    ppObjects[dwIndex] = NULL;
                }
            }
        }

        if (ppszSids)
        {
            LwFreeStringArray(ppszSids, dwEnumCount);
        }

        if (ppObjects)
        {
            LsaFreeSecurityObjectList(dwEnumCount, ppObjects);
            ppObjects = NULL;
        }
    }

cleanup:

    if (ppszSids)
    {
        LwFreeStringArray(ppszSids, dwEnumCount);
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(dwEnumCount, ppObjects);
        ppObjects = NULL;
    }

    if (hEnum)
    {
        LsaCloseEnum(hLsa, hEnum);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaQueryExpandedGroupMembers(
    IN HANDLE hLsa,
    PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN PCSTR pszSid,
    OUT PDWORD pdwMemberCount,
    OUT PLSA_SECURITY_OBJECT** pppMembers
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PLW_HASH_TABLE pHash = NULL;
    LW_HASH_ITERATOR hashIterator = {0};
    LW_HASH_ENTRY* pHashEntry = NULL;
    DWORD dwMemberCount = 0;
    DWORD dwFilteredMemberCount = 0;
    PLSA_SECURITY_OBJECT* ppMembers = NULL;
    PLSA_SECURITY_OBJECT pMember = NULL;

    dwError = LwHashCreate(
        29,
        LwHashCaselessStringCompare,
        LwHashCaselessStringHash,
        LsaFreeMemberHashEntry,
        NULL,
        &pHash);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaQueryExpandedGroupMembersInternal(
        hLsa,
        pszTargetProvider,
        FindFlags,
        ObjectType,
        pszSid,
        pHash);

    dwMemberCount = (DWORD) LwHashGetKeyCount(pHash);
    
    if (dwMemberCount)
    {
        dwError = LwAllocateMemory(
            sizeof(*ppMembers) * dwMemberCount,
            OUT_PPVOID(&ppMembers));
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = LwHashGetIterator(pHash, &hashIterator);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (dwIndex = 0; (pHashEntry = LwHashNext(&hashIterator)) != NULL; dwIndex++)
        {
            pMember = pHashEntry->pValue;

            if (ObjectType == LSA_OBJECT_TYPE_UNDEFINED ||
                pMember->type == ObjectType)
            {
                ppMembers[dwFilteredMemberCount++] = pMember;
                pHashEntry->pValue = NULL;
            }
        }
    }

    *pppMembers = ppMembers;
    *pdwMemberCount = dwFilteredMemberCount;

cleanup:

    LwHashSafeFree(&pHash);

    return dwError;

error:

    *pppMembers = NULL;
    *pdwMemberCount = 0;

    if (ppMembers)
    {
        LsaFreeSecurityObjectList(dwMemberCount, ppMembers);
    }

    goto cleanup;
}

LW_DWORD
LsaFindGroupAndExpandedMembers(
    LW_IN LW_HANDLE hLsa,
    LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LSA_QUERY_TYPE QueryType,
    LW_IN LSA_QUERY_ITEM QueryItem,
    LW_OUT PLSA_SECURITY_OBJECT* ppGroupObject,
    LW_OUT LW_PDWORD pdwMemberObjectCount,
    LW_OUT PLSA_SECURITY_OBJECT** pppMemberObjects
    )
{
    return LsaTransactFindGroupAndExpandedMembers(
        hLsa,
        pszTargetProvider,
        FindFlags,
        QueryType,
        QueryItem,
        ppGroupObject,
        pdwMemberObjectCount,
        pppMemberObjects);
}

LW_DWORD
LsaDeleteObject(
    LW_HANDLE hLsaConnection,
    PCSTR pszTargetProvider,
    LW_PCSTR pszSid
    )
{
    return LsaTransactDeleteObject(
        hLsaConnection,
        pszTargetProvider,
        pszSid);
}

LSASS_API
DWORD
LsaModifyUser2(
    HANDLE hLsaConnection,
    PCSTR pszTargetProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    )
{
    return LsaTransactModifyUser2(
        hLsaConnection,
        pszTargetProvider,
        pUserModInfo);
}

LSASS_API
DWORD
LsaAddUser2(
    HANDLE hLsaConnection,
    PCSTR pszTargetProvider,
    PLSA_USER_ADD_INFO pUserAddInfo
    )
{
    return LsaTransactAddUser2(
            hLsaConnection,
            pszTargetProvider,
            pUserAddInfo);
}
LSASS_API
DWORD
LsaAddGroup2(
    HANDLE hLsaConnection,
    PCSTR pszTargetProvider,
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    )
{
    return LsaTransactAddGroup2(
            hLsaConnection,
            pszTargetProvider,
            pGroupAddInfo);
}

LSASS_API
DWORD
LsaModifyGroup2(
    HANDLE hLsaConnection,
    PCSTR pszTargetProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    )
{
    return LsaTransactModifyGroup2(
        hLsaConnection,
        pszTargetProvider,
        pGroupModInfo);
}
