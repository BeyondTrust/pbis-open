/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        groups.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Group Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
            Wei Fu (wfu@likewisesoftware.com)
 */
#include "client.h"

typedef struct __LSA_CLIENT_ENUM_GROUPS_HANDLE
{
    LSA_FIND_FLAGS FindFlags;
    DWORD  dwGroupInfoLevel;
    DWORD  dwMaxNumGroups;
    DWORD dwObjectCount;
    DWORD dwObjectIndex;
    PLSA_SECURITY_OBJECT* ppObjects;
    HANDLE hEnum;
} LSA_CLIENT_ENUM_GROUPS_HANDLE, *PLSA_CLIENT_ENUM_GROUPS_HANDLE;

LSASS_API
DWORD
LsaAddGroup(
    HANDLE hLsaConnection,
    PVOID  pGroupInfo,
    DWORD  dwGroupInfoLevel
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_ADD_INFO pAddInfo = NULL;

    switch (dwGroupInfoLevel)
    {
    case 0:
        dwError = LsaMarshalGroupInfo0ToGroupAddInfo(
            hLsaConnection,
            pGroupInfo,
            &pAddInfo);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case 1:
        dwError = LsaMarshalGroupInfo1ToGroupAddInfo(
            hLsaConnection,
            pGroupInfo,
            &pAddInfo);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaTransactAddGroup2(
        hLsaConnection,
        NULL,
        pAddInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (pAddInfo)
    {
        LsaFreeGroupAddInfo(pAddInfo);
    }

    return dwError;
}

LSASS_API
DWORD
LsaModifyGroup(
    HANDLE hLsaConnection,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo2 = NULL;

    dwError = LsaMarshalGroupModInfoToGroupModInfo2(
        hLsaConnection,
        pGroupModInfo,
        &pGroupModInfo2);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaTransactModifyGroup2(
            hLsaConnection,
            NULL,
            pGroupModInfo2);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (pGroupModInfo2)
    {
        LsaFreeGroupModInfo2(pGroupModInfo2);
    }

    return dwError;
}

LSASS_API
DWORD
LsaFindGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_ITEM QueryItem;
    DWORD dwObjectCount = 1;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PLSA_SECURITY_OBJECT pGroup = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszGroupName);

    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(ppGroupInfo);

    switch (dwGroupInfoLevel)
    {
    case 1:
        /* Fast path */
        QueryItem.pszString = pszGroupName;
        
        dwError = LsaFindGroupAndExpandedMembers(
            hLsaConnection,
            NULL,
            FindFlags,
            LSA_QUERY_TYPE_BY_NAME,
            QueryItem,
            &pGroup,
            &dwObjectCount,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaMarshalGroupInfo1(
            hLsaConnection,
            FindFlags,
            pGroup,
            dwObjectCount,
            ppObjects,
            dwGroupInfoLevel,
            &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        QueryList.ppszStrings = &pszGroupName;
        
        dwError = LsaFindObjects(
            hLsaConnection,
            NULL,
            0,
            LSA_OBJECT_TYPE_GROUP,
            LSA_QUERY_TYPE_BY_NAME,
            1,
            QueryList,
        &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (ppObjects[0] == NULL)
        {
            dwError = LW_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        dwError = LsaMarshalGroupInfo(
            hLsaConnection,
            FindFlags,
            ppObjects[0],
            dwGroupInfoLevel,
            &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    if (ppGroupInfo)
    {
        *ppGroupInfo = pGroupInfo;
    }

    if (pGroup)
    {
        LsaFreeSecurityObject(pGroup);
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(dwObjectCount, ppObjects);
    }

    return dwError;
}


LSASS_API
DWORD
LsaFindGroupById(
    HANDLE hLsaConnection,
    gid_t  gid,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_ITEM QueryItem;
    DWORD dwObjectCount = 1;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PLSA_SECURITY_OBJECT pGroup = NULL;
    DWORD dwGid = (DWORD) gid;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);

    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(ppGroupInfo);

    switch (dwGroupInfoLevel)
    {
    case 1:
        /* Fast path */
        QueryItem.dwId = dwGid;
        
        dwError = LsaFindGroupAndExpandedMembers(
            hLsaConnection,
            NULL,
            FindFlags,
            LSA_QUERY_TYPE_BY_UNIX_ID,
            QueryItem,
            &pGroup,
            &dwObjectCount,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaMarshalGroupInfo1(
            hLsaConnection,
            FindFlags,
            pGroup,
            dwObjectCount,
            ppObjects,
            dwGroupInfoLevel,
            &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        QueryList.pdwIds = &dwGid;
        
        dwError = LsaFindObjects(
            hLsaConnection,
            NULL,
            FindFlags,
            LSA_OBJECT_TYPE_GROUP,
            LSA_QUERY_TYPE_BY_UNIX_ID,
            1,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (ppObjects[0] == NULL)
        {
            dwError = LW_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        dwError = LsaMarshalGroupInfo(
            hLsaConnection,
            FindFlags,
            ppObjects[0],
            dwGroupInfoLevel,
            &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    if (ppGroupInfo)
    {
        *ppGroupInfo = pGroupInfo;
    }
    
    if (pGroup)
    {
        LsaFreeSecurityObject(pGroup);
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(dwObjectCount, ppObjects);
    }

    return dwError;
}

LSASS_API
DWORD
LsaBeginEnumGroups(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_ENUM_GROUPS_HANDLE pEnum = NULL;

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);

    pEnum->dwGroupInfoLevel = dwGroupInfoLevel;
    pEnum->dwMaxNumGroups = dwMaxNumGroups;
    pEnum->FindFlags = FindFlags;
    
    dwError = LsaOpenEnumObjects(
        hLsaConnection,
        NULL,
        &pEnum->hEnum,
        FindFlags,
        LSA_OBJECT_TYPE_GROUP,
        NULL);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = pEnum;

cleanup:

    return dwError;

error:

    *phResume = NULL;

    if (pEnum)
    {
        LsaEndEnumGroups(hLsaConnection, pEnum);
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaBeginEnumGroupsWithCheckOnlineOption(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    return LsaBeginEnumGroups(
        hLsaConnection,
        dwGroupInfoLevel,
        dwMaxNumGroups,
        FindFlags | (bCheckGroupMembersOnline ? LSA_FIND_FLAGS_NSS : 0),
        phResume);
}

LSASS_API
DWORD
LsaEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_ENUM_GROUPS_HANDLE pEnum = hResume;
    DWORD dwTotalInfoCount = 0;
    DWORD dwInfoCount = 0;
    DWORD dwObjectsUsed = 0;
    PVOID* ppGroupInfo = NULL;

    if (!pEnum->hEnum)
    {
        dwError = LW_ERROR_NO_MORE_GROUPS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*ppGroupInfo) * pEnum->dwMaxNumGroups, OUT_PPVOID(&ppGroupInfo));
    BAIL_ON_LSA_ERROR(dwError);

    while (dwTotalInfoCount < pEnum->dwMaxNumGroups)
    {
        if (!pEnum->ppObjects)
        {
            dwError = LsaEnumObjects(
                hLsaConnection,
                pEnum->hEnum,
                pEnum->dwMaxNumGroups - dwTotalInfoCount,
                &pEnum->dwObjectCount,
                &pEnum->ppObjects);
            if (dwError == ERROR_NO_MORE_ITEMS)
            {
                dwError = 0;
                break;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }

        while (pEnum->dwObjectIndex < pEnum->dwObjectCount)
        {
            dwError = LsaMarshalGroupInfoList(
                hLsaConnection,
                pEnum->FindFlags,
                pEnum->dwObjectCount - pEnum->dwObjectIndex,
                pEnum->ppObjects + pEnum->dwObjectIndex,
                pEnum->dwGroupInfoLevel,
                pEnum->dwMaxNumGroups - dwTotalInfoCount,
                ppGroupInfo + dwTotalInfoCount,
                &dwObjectsUsed,
                &dwInfoCount);
            BAIL_ON_LSA_ERROR(dwError);

            pEnum->dwObjectIndex += dwObjectsUsed;
            dwTotalInfoCount += dwInfoCount;
        }

        LsaUtilFreeSecurityObjectList(pEnum->dwObjectCount, pEnum->ppObjects);
        pEnum->ppObjects = NULL;
        pEnum->dwObjectIndex = 0;
    }

    if (dwTotalInfoCount == 0)
    {
        dwError = LsaCloseEnum(hLsaConnection, pEnum->hEnum);
        pEnum->hEnum = NULL;
        BAIL_ON_LSA_ERROR(dwError);

        *pdwNumGroupsFound = 0;
        *pppGroupInfoList = NULL;
        
        LW_SAFE_FREE_MEMORY(ppGroupInfo);
    }
    else
    {
        *pdwNumGroupsFound = dwTotalInfoCount;
        *pppGroupInfoList = ppGroupInfo;
    }

cleanup:

    return dwError;

error:

    *pdwNumGroupsFound = 0;
    *pppGroupInfoList = NULL;

    if (ppGroupInfo)
    {
        LsaFreeGroupInfoList(pEnum->dwGroupInfoLevel, ppGroupInfo, dwTotalInfoCount);
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaEndEnumGroups(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_ENUM_GROUPS_HANDLE pEnum = hResume;

    if (pEnum)
    {
        if (pEnum->hEnum)
        {
            dwError = LsaCloseEnum(hLsaConnection, pEnum->hEnum);
        }

        if (pEnum->ppObjects)
        {
            LsaUtilFreeSecurityObjectList(pEnum->dwObjectCount, pEnum->ppObjects);
        }

        LwFreeMemory(pEnum);
    }

    return dwError;
}

LSASS_API
DWORD
LsaDeleteGroupById(
    HANDLE hLsaConnection,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwGid = (DWORD) gid;

    QueryList.pdwIds = &dwGid;
    
    dwError = LsaFindObjects(
        hLsaConnection,
        NULL,
        0,
        LSA_OBJECT_TYPE_GROUP,
        LSA_QUERY_TYPE_BY_UNIX_ID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaTransactDeleteObject(
        hLsaConnection,
        NULL,
        ppObjects[0]->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaDeleteGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszName
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;

    if (geteuid() != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszName);

    dwError = LsaValidateGroupName(pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszName,
                    0,
                    dwGroupInfoLevel,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDeleteGroupById(
                    hLsaConnection,
                    ((PLSA_GROUP_INFO_0)pGroupInfo)->gid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaGetGidsForUserByName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,
    PDWORD  pdwGroupFound,
    gid_t** ppGidResults
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    static const DWORD dwGroupInfoLevel = 0;
    DWORD dwGroupFound = 0;
    gid_t* pGidResult = NULL;
    PVOID*  ppGroupInfoList = NULL;
    DWORD iGroup = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszUserName);
    BAIL_ON_INVALID_POINTER(ppGidResults);

    dwError = LsaValidateUserName(pszUserName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                  hLsaConnection,
                  pszUserName,
                  dwUserInfoLevel,
                  &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetGroupsForUserById(
                hLsaConnection,
                ((PLSA_USER_INFO_0)pUserInfo)->uid,
                LSA_FIND_FLAGS_NSS,
                dwGroupInfoLevel,
                &dwGroupFound,
                &ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(gid_t) * dwGroupFound,
                    (PVOID*)&pGidResult);
    BAIL_ON_LSA_ERROR(dwError);

    for (iGroup = 0; iGroup < dwGroupFound; iGroup++)
    {
        *(pGidResult+iGroup) = ((PLSA_GROUP_INFO_0)(*(ppGroupInfoList+iGroup)))->gid;
    }

    *ppGidResults = pGidResult;
    *pdwGroupFound = dwGroupFound;

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwGroupFound);
    }

    return dwError;

error:

    *ppGidResults = NULL;
    *pdwGroupFound = 0;

    goto cleanup;
}

static
DWORD
LsaGetGroupsForUserBySid(
    IN HANDLE hLsaConnection,
    IN PCSTR pszSid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PDWORD pdwGroupsFound,
    OUT PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSids = 0;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsUsed = 0;
    DWORD dwInfoCount = 0;
    PVOID* ppGroupInfo = NULL;

    dwError = LsaQueryMemberOf(
        hLsaConnection,
        NULL,
        FindFlags,
        1,
        (PSTR*) &pszSid,
        &dwGroupSidCount,
        &ppszGroupSids);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwGroupSidCount == 0)
    {
        *pdwGroupsFound = 0;
        *pppGroupInfoList = NULL;
    }
    else
    {
        QueryList.ppszStrings = (PCSTR*) ppszGroupSids;
        
        dwError = LsaFindObjects(
            hLsaConnection,
            NULL,
            FindFlags,
            LSA_OBJECT_TYPE_GROUP,
            LSA_QUERY_TYPE_BY_SID,
            dwGroupSidCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateMemory(sizeof(*ppGroupInfo) * dwGroupSidCount, OUT_PPVOID(&ppGroupInfo));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaMarshalGroupInfoList(
            hLsaConnection,
            FindFlags,
            dwGroupSidCount,
            ppObjects,
            dwGroupInfoLevel,
            dwGroupSidCount,
            ppGroupInfo,
            &dwObjectsUsed,
            &dwInfoCount);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwInfoCount == 0)
        {
             *pdwGroupsFound = 0;
             *pppGroupInfoList = NULL;

             LW_SAFE_FREE_MEMORY(ppObjects);
        }
        else
        {
            *pdwGroupsFound = dwInfoCount;
            *pppGroupInfoList = ppGroupInfo;
        }
    }

cleanup:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwGroupSidCount, ppObjects);
    }

    if (ppszGroupSids)
    {
        LwFreeStringArray(ppszGroupSids, dwGroupSidCount);
    }

    return dwError;

error:

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    if (ppGroupInfo)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfo, dwInfoCount);
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaGetGroupsForUserByName(
    IN HANDLE hLsaConnection,
    IN PCSTR pszUserName,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PDWORD pdwGroupsFound,
    OUT PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;

    QueryList.ppszStrings = &pszUserName;
    
    dwError = LsaFindObjects(
        hLsaConnection,
        NULL,
        FindFlags,
        LSA_OBJECT_TYPE_USER,
        LSA_QUERY_TYPE_BY_NAME,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }     
    
    dwError = LsaGetGroupsForUserBySid(
        hLsaConnection,
        ppObjects[0]->pszObjectSid,
        FindFlags,
        dwGroupInfoLevel,
        pdwGroupsFound,
        pppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(1, ppObjects);
    }

    return dwError;

error:

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaGetGroupsForUserById(
    HANDLE  hLsaConnection,
    uid_t   uid,
    LSA_FIND_FLAGS FindFlags,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    DWORD dwUid = (DWORD) uid;

    QueryList.pdwIds = &dwUid;
    
    dwError = LsaFindObjects(
        hLsaConnection,
        NULL,
        FindFlags,
        LSA_OBJECT_TYPE_USER,
        LSA_QUERY_TYPE_BY_UNIX_ID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }     
    
    dwError = LsaGetGroupsForUserBySid(
        hLsaConnection,
        ppObjects[0]->pszObjectSid,
        FindFlags,
        dwGroupInfoLevel,
        pdwGroupsFound,
        pppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(1, ppObjects);
    }

    return dwError;

error:

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}

VOID
LsaFreeEnumObjectsInfo(
    PLSA_ENUM_OBJECTS_INFO pInfo
    )
{
    LW_SAFE_FREE_STRING(pInfo->pszGUID);
    LwFreeMemory(pInfo);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
