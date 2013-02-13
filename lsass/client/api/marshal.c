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
 *        marshal.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Client API Info Level Marshalling
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *          Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "client.h"

DWORD
LsaConvertTimeNt2Unix(
    UINT64 ntTime,
    PUINT64 pUnixTime
        )
{
    UINT64 unixTime = 0;

    unixTime = ntTime/10000000LL - 11644473600LL;

    *pUnixTime = unixTime;

    return 0;
}

DWORD
LsaConvertTimeUnix2Nt(
    UINT64 unixTime,
    PUINT64 pNtTime
        )
{
    UINT64 ntTime = 0;

    ntTime = (unixTime+11644473600LL)*10000000LL;

    *pNtTime = ntTime;

    return 0;
}

DWORD
LsaMarshalUserInfo(
    PLSA_SECURITY_OBJECT pUser,
    DWORD       dwUserInfoLevel,
    PVOID*      ppUserInfo
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    /* These variables represent pUserInfo casted to different types. Do not
     * free these values directly, free pUserInfo instead.
     */
    PLSA_USER_INFO_0 pUserInfo0 = NULL;
    PLSA_USER_INFO_1 pUserInfo1 = NULL;
    PLSA_USER_INFO_2 pUserInfo2 = NULL;

    *ppUserInfo = NULL;

    BAIL_ON_INVALID_POINTER(pUser);

    if (pUser->type != LSA_OBJECT_TYPE_USER)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pUser->enabled)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwUserInfoLevel)
    {
        case 0:
            dwError = LwAllocateMemory(
                            sizeof(LSA_USER_INFO_0),
                            (PVOID*)&pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
            pUserInfo0 = (PLSA_USER_INFO_0) pUserInfo;
            break;
        case 1:
            dwError = LwAllocateMemory(
                            sizeof(LSA_USER_INFO_1),
                            (PVOID*)&pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
            pUserInfo0 = (PLSA_USER_INFO_0) pUserInfo;
            pUserInfo1 = (PLSA_USER_INFO_1) pUserInfo;
            break;
        case 2:
            dwError = LwAllocateMemory(
                            sizeof(LSA_USER_INFO_2),
                            (PVOID*)&pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
            pUserInfo0 = (PLSA_USER_INFO_0) pUserInfo;
            pUserInfo1 = (PLSA_USER_INFO_1) pUserInfo;
            pUserInfo2 = (PLSA_USER_INFO_2) pUserInfo;
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    if (pUserInfo0 != NULL)
    {
        pUserInfo0->uid = pUser->userInfo.uid;
        pUserInfo0->gid = pUser->userInfo.gid;

        dwError = LwAllocateString(
            pUser->userInfo.pszUnixName,
            &pUserInfo0->pszName);
        BAIL_ON_LSA_ERROR(dwError);

        // Optional values use LwStrDupOrNull. Required values use
        // LwAllocateString.
        dwError = LwStrDupOrNull(
                    pUser->userInfo.pszPasswd,
                    &pUserInfo0->pszPasswd);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwStrDupOrNull(
                    pUser->userInfo.pszGecos,
                    &pUserInfo0->pszGecos);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateString(
                    pUser->userInfo.pszShell,
                    &pUserInfo0->pszShell);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateString(
                    pUser->userInfo.pszHomedir,
                    &pUserInfo0->pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateString(
                    pUser->pszObjectSid,
                    &pUserInfo0->pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo1 != NULL)
    {
        dwError = LwStrDupOrNull(
                      pUser->pszDN,
                      &pUserInfo1->pszDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwStrDupOrNull(
                      pUser->userInfo.pszUPN,
                      &pUserInfo1->pszUPN);
        BAIL_ON_LSA_ERROR(dwError);

        pUserInfo1->bIsGeneratedUPN = pUser->userInfo.bIsGeneratedUPN;
        pUserInfo1->bIsLocalUser = pUser->bIsLocal;
        pUserInfo1->pLMHash = NULL;
        pUserInfo1->dwLMHashLen = 0;
        pUserInfo1->pNTHash = NULL;
        pUserInfo1->dwNTHashLen = 0;
    }

    if (pUserInfo2 != NULL)
    {
        struct timeval current_tv;
        UINT64 u64current_NTtime = 0;

        if (pUser->userInfo.bIsAccountInfoKnown)
        {
            if (gettimeofday(&current_tv, NULL) < 0)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
            LsaConvertTimeUnix2Nt(current_tv.tv_sec,
                                 &u64current_NTtime);

            if (pUser->userInfo.bPasswordNeverExpires ||
                pUser->userInfo.qwPwdExpires == 0)
            {
                //password never expires
                pUserInfo2->dwDaysToPasswordExpiry = 0LL;
            }
            else if (pUser->userInfo.bPasswordExpired ||
                     u64current_NTtime >= pUser->userInfo.qwPwdExpires)
            {
                //password is expired already
                pUserInfo2->dwDaysToPasswordExpiry = 0LL;
            }
            else
            {
                pUserInfo2->dwDaysToPasswordExpiry =
                    (pUser->userInfo.qwPwdExpires - u64current_NTtime) /
                    (10000000LL * 24*60*60);
            }

            pUserInfo2->bPasswordNeverExpires = pUser->userInfo.bPasswordNeverExpires;

            pUserInfo2->bPasswordExpired = pUser->userInfo.bPasswordExpired;
            pUserInfo2->bPromptPasswordChange = pUser->userInfo.bPromptPasswordChange;
            pUserInfo2->bUserCanChangePassword = pUser->userInfo.bUserCanChangePassword;
            pUserInfo2->bAccountDisabled = pUser->userInfo.bAccountDisabled;
            pUserInfo2->bAccountExpired = pUser->userInfo.bAccountExpired;
            pUserInfo2->bAccountLocked = pUser->userInfo.bAccountLocked;
        }
        else
        {
            pUserInfo2->dwDaysToPasswordExpiry = 0LL;
            pUserInfo2->bPasswordExpired = FALSE;
            pUserInfo2->bPasswordNeverExpires = TRUE;
            pUserInfo2->bPromptPasswordChange = FALSE;
            pUserInfo2->bUserCanChangePassword = FALSE;
            pUserInfo2->bAccountDisabled = FALSE;
            pUserInfo2->bAccountExpired = FALSE;
            pUserInfo2->bAccountLocked = FALSE;
        }
    }

    *ppUserInfo = pUserInfo;

cleanup:

    return dwError;


error:
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
        pUserInfo = NULL;
    }

    *ppUserInfo = NULL;

    goto cleanup;
}

DWORD
LsaMarshalUserInfoList(
    DWORD dwObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects,
    DWORD dwUserInfoLevel,
    DWORD dwUserInfoLength,
    PVOID* ppUserInfo,
    PDWORD pdwObjectUsedCount,
    PDWORD pdwUserInfoCount
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    DWORD dwUserInfoCount = 0;

    for (dwIndex = 0; dwIndex < dwObjectCount && dwUserInfoCount < dwUserInfoLength; dwIndex++)
    {
        dwError = LsaMarshalUserInfo(ppObjects[dwIndex], dwUserInfoLevel, &ppUserInfo[dwUserInfoCount]);
        switch (dwError)
        {
        case LW_ERROR_SUCCESS:
            dwUserInfoCount++;
            break;
        case LW_ERROR_NO_SUCH_USER:
            dwError = 0;
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pdwObjectUsedCount = dwIndex;
    *pdwUserInfoCount = dwUserInfoCount;

cleanup:

    return dwError;

error:

    *pdwUserInfoCount = 0;

    goto cleanup;
}

DWORD
LsaMarshalGroupInfo(
    HANDLE hLsa,
    LSA_FIND_FLAGS FindFlags,
    PLSA_SECURITY_OBJECT     pGroup,
    DWORD                   dwGroupInfoLevel,
    PVOID*                  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    /* The variable represents pGroupInfo casted to different types. Do not
     * free these values directly, free pGroupInfo instead.
     */   
    size_t sIndex = 0;
    size_t sEnabled = 0;
    DWORD dwMemberCount = 0;
    PLSA_SECURITY_OBJECT* ppMembers = NULL;

    *ppGroupInfo = NULL;

    BAIL_ON_INVALID_POINTER(pGroup);

    if (pGroup->type != LSA_OBJECT_TYPE_GROUP)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pGroup->enabled)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwGroupInfoLevel)
    {
        case 0:
        {
            PLSA_GROUP_INFO_0 pGroupInfo0 = NULL;

            dwError = LwAllocateMemory(
                            sizeof(LSA_GROUP_INFO_0),
                            (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            pGroupInfo0 = (PLSA_GROUP_INFO_0) pGroupInfo;
            
            pGroupInfo0->gid = pGroup->groupInfo.gid;
            
            dwError = LwAllocateString(
                                pGroup->groupInfo.pszUnixName,
                                &pGroupInfo0->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateString(
                                pGroup->pszObjectSid,
                                &pGroupInfo0->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
        }
        case 1:
        {
            PLSA_GROUP_INFO_1 pGroupInfo1 = NULL; 

            dwError = LsaQueryExpandedGroupMembers(
                hLsa,
                NULL,
                FindFlags,
                LSA_OBJECT_TYPE_USER,
                pGroup->pszObjectSid,
                &dwMemberCount,
                &ppMembers);
            if (dwError == LW_ERROR_NO_SUCH_OBJECT)
            {
                dwError = LW_ERROR_NO_SUCH_GROUP;
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateMemory(
                            sizeof(LSA_GROUP_INFO_1),
                            (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            pGroupInfo1 = (PLSA_GROUP_INFO_1) pGroupInfo;
     
            pGroupInfo1->gid = pGroup->groupInfo.gid;

            dwError = LwAllocateString(
                                pGroup->groupInfo.pszUnixName,
                                &pGroupInfo1->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            // Optional values use LwStrDupOrNull. Required values use
            // LwAllocateString.
            dwError = LwStrDupOrNull(
                        pGroup->groupInfo.pszPasswd,
                        &pGroupInfo1->pszPasswd);
            BAIL_ON_LSA_ERROR(dwError);        

            dwError = LwAllocateString(
                                pGroup->pszObjectSid,
                                &pGroupInfo1->pszSid);
            BAIL_ON_LSA_ERROR(dwError);

            if (pGroup->pszDN)
            {
                dwError = LwAllocateString(
                                    pGroup->pszDN,
                                    &pGroupInfo1->pszDN);
                BAIL_ON_LSA_ERROR(dwError);
            }

            for (sIndex = 0; sIndex < dwMemberCount; sIndex++)
            {
                if (ppMembers[sIndex])
                {
                    if (ppMembers[sIndex]->enabled)
                    {
                        sEnabled++;
                    }
                    
                    if (ppMembers[sIndex]->type != LSA_OBJECT_TYPE_USER)
                    {
                        dwError = LW_ERROR_INVALID_PARAMETER;
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                }
            }

            dwError = LwAllocateMemory(
                            //Leave room for terminating null pointer
                            sizeof(PSTR) * (sEnabled+1),
                            (PVOID*)&pGroupInfo1->ppszMembers);
            BAIL_ON_LSA_ERROR(dwError);

            sEnabled = 0;

            for (sIndex = 0; sIndex < dwMemberCount; sIndex++)               
            {
                if (ppMembers[sIndex])
                {
                    if (ppMembers[sIndex]->enabled)
                    {
                        dwError = LwAllocateString(
                            ppMembers[sIndex]->userInfo.pszUnixName,
                            &pGroupInfo1->ppszMembers[sEnabled++]);
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                }
            }
            
            break;
        }

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:
    
    if (ppMembers)
    {
        LsaUtilFreeSecurityObjectList(dwMemberCount, ppMembers);
    }
    
    return dwError;
    
error:
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
        pGroupInfo = NULL;
    }
    
    *ppGroupInfo = NULL;
    
    goto cleanup;
}

DWORD
LsaMarshalGroupInfo1(
    HANDLE hLsa,
    LSA_FIND_FLAGS FindFlags,
    PLSA_SECURITY_OBJECT     pGroup,
    DWORD dwMemberCount,
    PLSA_SECURITY_OBJECT* ppMembers,
    DWORD                   dwGroupInfoLevel,
    PVOID*                  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    PLSA_GROUP_INFO_1 pGroupInfo1 = NULL;
    /* The variable represents pGroupInfo casted to different types. Do not
     * free these values directly, free pGroupInfo instead.
     */   
    size_t sIndex = 0;
    size_t sEnabled = 0;

    *ppGroupInfo = NULL;

    BAIL_ON_INVALID_POINTER(pGroup);

    if (pGroup->type != LSA_OBJECT_TYPE_GROUP)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pGroup->enabled)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        sizeof(LSA_GROUP_INFO_1),
        (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroupInfo1 = (PLSA_GROUP_INFO_1) pGroupInfo;
     
    pGroupInfo1->gid = pGroup->groupInfo.gid;
    
    dwError = LwAllocateString(
        pGroup->groupInfo.pszUnixName,
        &pGroupInfo1->pszName);
    BAIL_ON_LSA_ERROR(dwError);
    
    // Optional values use LwStrDupOrNull. Required values use
    // LwAllocateString.
    dwError = LwStrDupOrNull(
        pGroup->groupInfo.pszPasswd,
        &pGroupInfo1->pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);        
    
    dwError = LwAllocateString(
        pGroup->pszObjectSid,
        &pGroupInfo1->pszSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pGroup->pszDN)
    {
        dwError = LwAllocateString(
            pGroup->pszDN,
            &pGroupInfo1->pszDN);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    for (sIndex = 0; sIndex < dwMemberCount; sIndex++)
    {
        if (ppMembers[sIndex])
        {
            if (ppMembers[sIndex]->enabled)
            {
                sEnabled++;
            }
            
            if (ppMembers[sIndex]->type != LSA_OBJECT_TYPE_USER)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }
    
    dwError = LwAllocateMemory(
        //Leave room for terminating null pointer
        sizeof(PSTR) * (sEnabled+1),
        (PVOID*)&pGroupInfo1->ppszMembers);
    BAIL_ON_LSA_ERROR(dwError);
    
    sEnabled = 0;
    
    for (sIndex = 0; sIndex < dwMemberCount; sIndex++)               
    {
        if (ppMembers[sIndex])
        {
            if (ppMembers[sIndex]->enabled)
            {
                dwError = LwAllocateString(
                    ppMembers[sIndex]->userInfo.pszUnixName,
                    &pGroupInfo1->ppszMembers[sEnabled++]);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:
    
    return dwError;
    
error:
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
        pGroupInfo = NULL;
    }
    
    *ppGroupInfo = NULL;
    
    goto cleanup;
}

DWORD
LsaMarshalGroupInfoList(
    HANDLE hLsa,
    LSA_FIND_FLAGS FindFlags,
    DWORD dwObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects,
    DWORD dwGroupInfoLevel,
    DWORD dwGroupInfoLength,
    PVOID* ppGroupInfo,
    PDWORD pdwObjectUsedCount,
    PDWORD pdwGroupInfoCount
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    DWORD dwGroupInfoCount = 0;

    for (dwIndex = 0; dwIndex < dwObjectCount && dwGroupInfoCount < dwGroupInfoLength; dwIndex++)
    {
        if (ppObjects[dwIndex])
        {
            dwError = LsaMarshalGroupInfo(
                hLsa,
                FindFlags,
                ppObjects[dwIndex],
                dwGroupInfoLevel,
                &ppGroupInfo[dwGroupInfoCount]);
            switch (dwError)
            {
            case LW_ERROR_SUCCESS:
                dwGroupInfoCount++;
                break;
            case LW_ERROR_NO_SUCH_GROUP:
                dwError = 0;
                continue;
            default:
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    *pdwObjectUsedCount = dwIndex;
    *pdwGroupInfoCount = dwGroupInfoCount;

cleanup:

    return dwError;

error:

    *pdwGroupInfoCount = 0;
    *pdwObjectUsedCount = 0;

    goto cleanup;
}

DWORD
LsaMarshalUserModInfoToUserModInfo2(
    HANDLE hLsa,
    PLSA_USER_MOD_INFO pModInfo1,
    PLSA_USER_MOD_INFO_2* ppModInfo2
    )
{
    DWORD dwError = 0;
    PLSA_USER_MOD_INFO_2 pModInfo2 = NULL;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwUid = (DWORD) pModInfo1->uid;
    DWORD dwGid = (DWORD) pModInfo1->gid;

    dwError = LwAllocateMemory(sizeof(*pModInfo2), OUT_PPVOID(&pModInfo2));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(&pModInfo2->actions, &pModInfo1->actions, sizeof(pModInfo2->actions));

    QueryList.pdwIds = &dwUid;

    dwError = LsaFindObjects(
        hLsa,
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

    dwError = LwStrDupOrNull(ppObjects[0]->pszObjectSid, &pModInfo2->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaUtilFreeSecurityObjectList(1, ppObjects);
    BAIL_ON_LSA_ERROR(dwError);
    ppObjects = NULL;

    if (pModInfo1->actions.bSetPrimaryGroup)
    {
        QueryList.pdwIds = &dwGid;
        
        dwError = LsaFindObjects(
            hLsa,
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

        dwError = LwStrDupOrNull(ppObjects[0]->pszObjectSid, &pModInfo2->pszPrimaryGroupSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwStrDupOrNull(pModInfo1->pszAddToGroups, &pModInfo2->pszAddToGroups);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pModInfo1->pszRemoveFromGroups, &pModInfo2->pszRemoveFromGroups);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pModInfo1->pszExpiryDate, &pModInfo2->pszExpiryDate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pModInfo1->pszHomedir, &pModInfo2->pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pModInfo1->pszShell, &pModInfo2->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pModInfo1->pszGecos, &pModInfo2->pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pModInfo1->pszPassword, &pModInfo2->pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    if (pModInfo1->pNtPasswordHash)
    {
        dwError = LsaDataBlobCopy(&pModInfo2->pNtPasswordHash, pModInfo1->pNtPasswordHash);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pModInfo1->pLmPasswordHash)
    {
        dwError = LsaDataBlobCopy(&pModInfo2->pLmPasswordHash, pModInfo1->pLmPasswordHash);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppModInfo2 = pModInfo2;

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    return dwError;

error:

    *ppModInfo2 = NULL;

    if (pModInfo2)
    {
        LsaFreeUserModInfo2(pModInfo2);
    }

    goto cleanup;
}

DWORD
LsaMarshalGroupModInfoToGroupModInfo2(
    HANDLE hLsa,
    PLSA_GROUP_MOD_INFO pModInfo1,
    PLSA_GROUP_MOD_INFO_2* ppModInfo2
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_MOD_INFO_2 pModInfo2 = NULL;
    DWORD dwIndex = 0;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwGid = (DWORD) pModInfo1->gid;
    
    dwError = LwAllocateMemory(sizeof(*pModInfo2), OUT_PPVOID(&pModInfo2));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(&pModInfo2->actions, &pModInfo1->actions, sizeof(pModInfo2->actions));
    pModInfo2->dwAddMembersNum = pModInfo1->dwAddMembersNum;
    pModInfo2->dwRemoveMembersNum = pModInfo1->dwRemoveMembersNum;
    
    QueryList.pdwIds = &dwGid;
    
    dwError = LsaFindObjects(
        hLsa,
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

    dwError = LwStrDupOrNull(ppObjects[0]->pszObjectSid, &pModInfo2->pszSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pModInfo1->pAddMembers)
    {
        dwError = LwAllocateMemory(
            sizeof(*pModInfo2->ppszAddMembers) * pModInfo2->dwAddMembersNum,
            OUT_PPVOID(&pModInfo2->ppszAddMembers));
        BAIL_ON_LSA_ERROR(dwError);
        
        for (dwIndex = 0; dwIndex < pModInfo2->dwAddMembersNum; dwIndex++)
        {
            dwError = LwAllocateString(
                pModInfo1->pAddMembers[dwIndex].pszSid,
                &pModInfo2->ppszAddMembers[dwIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pModInfo1->pRemoveMembers)
    {
        dwError = LwAllocateMemory(
            sizeof(*pModInfo2->ppszRemoveMembers) * pModInfo2->dwRemoveMembersNum,
            OUT_PPVOID(&pModInfo2->ppszRemoveMembers));
        BAIL_ON_LSA_ERROR(dwError);

        for (dwIndex = 0; dwIndex < pModInfo2->dwRemoveMembersNum; dwIndex++)
        {
            dwError = LwAllocateString(
                pModInfo1->pRemoveMembers[dwIndex].pszSid,
                &pModInfo2->ppszRemoveMembers[dwIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppModInfo2 = pModInfo2;

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    return dwError;

error:

    *ppModInfo2 = NULL;

    if (pModInfo2)
    {
        LsaFreeGroupModInfo2(pModInfo2);
    }

    goto cleanup;
}

DWORD
LsaMarshalGroupInfo0ToGroupAddInfo(
    HANDLE hLsa,
    PLSA_GROUP_INFO_0 pGroupInfo,
    PLSA_GROUP_ADD_INFO* ppAddInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_ADD_INFO pAddInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pAddInfo), OUT_PPVOID(&pAddInfo));
    BAIL_ON_LSA_ERROR(dwError);

    pAddInfo->gid = pGroupInfo->gid;
    
    dwError = LwStrDupOrNull(pGroupInfo->pszName, &pAddInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppAddInfo = pAddInfo;

cleanup:

    return dwError;
    
error:

    *ppAddInfo = NULL;

    if (pAddInfo)
    {
        LsaFreeGroupAddInfo(pAddInfo);
    }

    goto cleanup;
}

DWORD
LsaMarshalGroupInfo1ToGroupAddInfo(
    HANDLE hLsa,
    PLSA_GROUP_INFO_1 pGroupInfo,
    PLSA_GROUP_ADD_INFO* ppAddInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_ADD_INFO pAddInfo = NULL;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwCount = 0;
    DWORD dwIndex = 0;

    dwError = LsaMarshalGroupInfo0ToGroupAddInfo(
        hLsa,
        &pGroupInfo->info0,
        &pAddInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGroupInfo->ppszMembers)
    {
        for (dwCount = 0; pGroupInfo->ppszMembers[dwCount]; dwCount++);

        pAddInfo->dwMemberCount = dwCount;

        QueryList.ppszStrings = (PCSTR*) pGroupInfo->ppszMembers;

        dwError = LsaFindObjects(
            hLsa,
            NULL,
            0,
            LSA_OBJECT_TYPE_UNDEFINED,
            LSA_QUERY_TYPE_BY_NAME,
            dwCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateMemory(
            dwCount * sizeof(*pAddInfo->ppszMemberSids),
            OUT_PPVOID(&pAddInfo->ppszMemberSids));
        BAIL_ON_LSA_ERROR(dwError);

        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            if (!ppObjects[dwIndex])
            {
                dwError = LW_ERROR_NO_SUCH_OBJECT;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LwAllocateString(
                ppObjects[dwIndex]->pszObjectSid,
                &pAddInfo->ppszMemberSids[dwIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppAddInfo = pAddInfo;

cleanup:

    LsaUtilFreeSecurityObjectList(dwCount, ppObjects);

    return dwError;
    
error:

    *ppAddInfo = NULL;

    if (pAddInfo)
    {
        LsaFreeGroupAddInfo(pAddInfo);
    }

    goto cleanup;
}

DWORD
LsaMarshalUserInfo0ToUserAddInfo(
    HANDLE hLsa,
    PLSA_USER_INFO_0 pUserInfo,
    PLSA_USER_ADD_INFO* ppAddInfo
    )
{
    DWORD dwError = 0;
    PLSA_USER_ADD_INFO pAddInfo = NULL;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwGid = (DWORD) pUserInfo->gid;

    dwError = LwAllocateMemory(sizeof(*pAddInfo), OUT_PPVOID(&pAddInfo));
    BAIL_ON_LSA_ERROR(dwError);

    pAddInfo->uid = pUserInfo->uid;

    dwError = LwStrDupOrNull(pUserInfo->pszName, &pAddInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pUserInfo->pszPasswd, &pAddInfo->pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pUserInfo->pszGecos, &pAddInfo->pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pUserInfo->pszShell, &pAddInfo->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pUserInfo->pszHomedir, &pAddInfo->pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserInfo->gid > 0)
    {
        QueryList.pdwIds = &dwGid;

        dwError = LsaFindObjects(
            hLsa,
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

        dwError = LwAllocateString(ppObjects[0]->pszObjectSid, &pAddInfo->pszPrimaryGroupSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppAddInfo = pAddInfo;

cleanup:

    return dwError;
    
error:

    *ppAddInfo = NULL;

    if (pAddInfo)
    {
        LsaFreeUserAddInfo(pAddInfo);
    }

    goto cleanup;
}
