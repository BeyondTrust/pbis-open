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
 *        users.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        User Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#define LSA_ENABLE_DEPRECATED

#include "client.h"

typedef struct __LSA_CLIENT_ENUM_USERS_HANDLE
{
    DWORD  dwUserInfoLevel;
    DWORD  dwMaxNumUsers;
    DWORD dwObjectCount;
    DWORD dwObjectIndex;
    PLSA_SECURITY_OBJECT* ppObjects;
    HANDLE hEnum;
} LSA_CLIENT_ENUM_USERS_HANDLE, *PLSA_CLIENT_ENUM_USERS_HANDLE;

LSASS_API
DWORD
LsaAddUser(
    HANDLE hLsaConnection,
    PVOID  pUserInfo,
    DWORD  dwUserInfoLevel
    )
{
    DWORD dwError = 0;
    PLSA_USER_ADD_INFO pAddInfo = NULL;

    switch (dwUserInfoLevel)
    {
    case 0:
        dwError = LsaMarshalUserInfo0ToUserAddInfo(
            hLsaConnection,
            pUserInfo,
            &pAddInfo);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case 1:
    case 2:
        dwError = LW_ERROR_NOT_SUPPORTED;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaTransactAddUser2(
        hLsaConnection,
        NULL,
        pAddInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (pAddInfo)
    {
        LsaFreeUserAddInfo(pAddInfo);
    }

    return dwError;
}

LSASS_API
DWORD
LsaModifyUser(
    HANDLE hLsaConnection,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
    PLSA_USER_MOD_INFO_2 pUserModInfo2 = NULL;

    dwError = LsaMarshalUserModInfoToUserModInfo2(
        hLsaConnection,
        pUserModInfo,
        &pUserModInfo2);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaTransactModifyUser2(
            hLsaConnection,
            NULL,
            pUserModInfo2);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (pUserModInfo2)
    {
        LsaFreeUserModInfo2(pUserModInfo2);
    }

    return dwError;
}

LSASS_API
DWORD
LsaFindUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszName);

    dwError = LsaValidateUserInfoLevel(dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(ppUserInfo);

    QueryList.ppszStrings = &pszName;
    
    dwError = LsaFindObjects(
        hLsaConnection,
        NULL,
        0,
        LSA_OBJECT_TYPE_USER,
        LSA_QUERY_TYPE_BY_NAME,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMarshalUserInfo(
        ppObjects[0],
        dwUserInfoLevel,
        &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (ppUserInfo)
    {
        *ppUserInfo = pUserInfo;
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
    }

    return dwError;
}

LSASS_API
DWORD
LsaFindUserById(
    HANDLE hLsaConnection,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwUid = (DWORD) uid;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);

    dwError = LsaValidateUserInfoLevel(dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(ppUserInfo);

    QueryList.pdwIds = &dwUid;
    
    dwError = LsaFindObjects(
        hLsaConnection,
        NULL,
        0,
        LSA_OBJECT_TYPE_USER,
        LSA_QUERY_TYPE_BY_UNIX_ID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMarshalUserInfo(
        ppObjects[0],
        dwUserInfoLevel,
        &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (ppUserInfo)
    {
        *ppUserInfo = pUserInfo;
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
    }

    return dwError;
}

LSASS_API
DWORD
LsaBeginEnumUsers(
    HANDLE  hLsaConnection,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_ENUM_USERS_HANDLE pEnum = NULL;

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);

    pEnum->dwUserInfoLevel = dwUserInfoLevel;
    pEnum->dwMaxNumUsers = dwMaxNumUsers;
    
    dwError = LsaOpenEnumObjects(
        hLsaConnection,
        NULL,
        &pEnum->hEnum,
        FindFlags,
        LSA_OBJECT_TYPE_USER,
        NULL);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = pEnum;

cleanup:

    return dwError;

error:

    *phResume = NULL;

    if (pEnum)
    {
        LsaEndEnumUsers(hLsaConnection, pEnum);
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaEnumUsers(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_ENUM_USERS_HANDLE pEnum = hResume;
    DWORD dwTotalInfoCount = 0;
    DWORD dwInfoCount = 0;
    DWORD dwObjectsUsed = 0;
    PVOID* ppUserInfo = NULL;

    if (!pEnum->hEnum)
    {
        dwError = LW_ERROR_NO_MORE_USERS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*ppUserInfo) * pEnum->dwMaxNumUsers, OUT_PPVOID(&ppUserInfo));
    BAIL_ON_LSA_ERROR(dwError);

    while (dwTotalInfoCount < pEnum->dwMaxNumUsers)
    {
        if (!pEnum->ppObjects)
        {
            dwError = LsaEnumObjects(
                hLsaConnection,
                pEnum->hEnum,
                pEnum->dwMaxNumUsers - dwTotalInfoCount,
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
            dwError = LsaMarshalUserInfoList(
                pEnum->dwObjectCount - pEnum->dwObjectIndex,
                pEnum->ppObjects + pEnum->dwObjectIndex,
                pEnum->dwUserInfoLevel,
                pEnum->dwMaxNumUsers - dwTotalInfoCount,
                ppUserInfo + dwTotalInfoCount,
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

        *pdwNumUsersFound = 0;
        *pppUserInfoList = NULL;
        
        LW_SAFE_FREE_MEMORY(ppUserInfo);
    }
    else
    {
        *pdwNumUsersFound = dwTotalInfoCount;
        *pppUserInfoList = ppUserInfo;
    }

cleanup:

    return dwError;

error:

    *pdwNumUsersFound = 0;
    *pppUserInfoList = NULL;

    if (ppUserInfo)
    {
        LsaFreeUserInfoList(pEnum->dwUserInfoLevel, ppUserInfo, dwTotalInfoCount);
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaEndEnumUsers(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_ENUM_USERS_HANDLE pEnum = hResume;

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
LsaDeleteUserById(
    HANDLE hLsaConnection,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwUid = (DWORD) uid;

    QueryList.pdwIds = &dwUid;
    
    dwError = LsaFindObjects(
        hLsaConnection,
        NULL,
        0,
        LSA_OBJECT_TYPE_USER,
        LSA_QUERY_TYPE_BY_UNIX_ID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
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
LsaDeleteUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    if (geteuid() != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszName);

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszName,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDeleteUserById(
                    hLsaConnection,
                    ((PLSA_USER_INFO_0)pUserInfo)->uid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaGetNamesBySidList(
    IN HANDLE hLsaConnection,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SID_INFO* ppSIDInfoList,
    OUT OPTIONAL CHAR *pchDomainSeparator
    )
{
    DWORD dwError = 0;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PLSA_SID_INFO pSidInfo = NULL;
    DWORD dwIndex = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);

    QueryList.ppszStrings = (PCSTR*) ppszSidList;

    dwError = LsaFindObjects(
        hLsaConnection,
        NULL,
        0,
        LSA_OBJECT_TYPE_UNDEFINED,
        LSA_QUERY_TYPE_BY_SID,
        (DWORD) sCount,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pSidInfo) * sCount, OUT_PPVOID(&pSidInfo));
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < sCount; dwIndex++)
    {
        if (ppObjects[dwIndex] == NULL)
        {
            pSidInfo[dwIndex].accountType = AccountType_NotFound;
            continue;
        }
        else
        {
            pSidInfo[dwIndex].accountType = ppObjects[dwIndex]->type;
        }

        if (pSidInfo[dwIndex].accountType != AccountType_NotFound)
        {
            dwError = LwAllocateString(
                ppObjects[dwIndex]->pszSamAccountName,
                &pSidInfo[dwIndex].pszSamAccountName);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = LwAllocateString(
                ppObjects[dwIndex]->pszNetbiosDomainName,
                &pSidInfo[dwIndex].pszDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppSIDInfoList = pSidInfo;

    if (pchDomainSeparator)
    {
        *pchDomainSeparator = '\\';
    }

cleanup:

    return dwError;

error:

    *ppSIDInfoList = NULL;

    if (pSidInfo)
    {
        LsaFreeSIDInfoList(pSidInfo, sCount);
    }

    goto cleanup;

}

LSASS_API
DWORD
LsaGetSmartCardUserObject(
    IN HANDLE hLsaConnection,
    OUT PLSA_SECURITY_OBJECT* ppObject,
    OUT PSTR* ppszSmartCardReader
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(ppObject);
    BAIL_ON_INVALID_POINTER(ppszSmartCardReader);

    dwError = LsaTransactGetSmartCardUserObject(
        hLsaConnection,
        ppObject,
        ppszSmartCardReader);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}
