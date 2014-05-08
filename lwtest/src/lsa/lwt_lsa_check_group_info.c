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
 * Module Name: lwt_lsa_check_group_info.c
 *
 * Verifies the group information returned from AD with the CSV file.
 *
 */

#include "includes.h"

#define MAX_GROUP_INFOLEVEL 1 /* 0 and 1*/

/*
 * ValidateForInvalidParams
 * 
 * Function validates the API for Invalid function parameters.
 * 
 */
static
DWORD 
ValidateForInvalidParams(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

static
DWORD
CheckForInvalidParams(
        HANDLE hLsaConnection,
        PLWTFAILDATA pInvalidData,
        DWORD dwGroupInfoLevel,
        PSTR pszMessage
        );

/*
 * ValidateGroupInfoLevel0
 * 
 * Function validates group level 0 information from AD with group information in csv file
 * 
 */
static 
DWORD 
ValidateGroupInfoLevel0(
    PLSA_GROUP_INFO_0 pADGroupInfoList,
    PLWTGROUP pGroupList )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = "Validating level 0 group information";
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName";
    CHAR  szTestMsg[256] = { 0 };

    snprintf(szTestMsg, sizeof(szTestMsg), "Validating group info level 0 for group:%s\n", 
                                pADGroupInfoList->pszName);
    if (!pGroupList)
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
                    "Grouplist structure don't have group information for group:%s\n", 
                    pADGroupInfoList->pszName);
        LWT_LOG_TEST(szTestMsg);
        goto error;
    }

    if (pGroupList->nGid != pADGroupInfoList->gid)
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg),
            "GID value mismatch for group:'%s', grouplist csv gives:%lu and API returns:%lu\n", 
            pADGroupInfoList->pszName,
            (unsigned long)pGroupList->nGid,
            (unsigned long)pADGroupInfoList->gid);
        goto error;
    }

    if (!StringsAreEqual(pGroupList->pszSid, pADGroupInfoList->pszSid))
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
                "SID value mismatch for group:'%s', \
                grouplist csv file gives:%s and API returns:%s\n", 
                pADGroupInfoList->pszName, 
                pGroupList->pszSid, 
                pADGroupInfoList->pszSid);
        goto error;
    }    

cleanup:
    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:
    goto cleanup;
}


/*
 * ValidateGroupMember
 * 
 * Function validates a group member for group information 
 * between user list and group list csv files.    
 * 
 */
static
DWORD
ValidateGroupMember(
    PSTR pszMember,
    PLWTGROUP pGroupList,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwIndex = 0;
    PCSTR pszTestDescription = "Validating Group member with userlist structure";
    PCSTR pszTestAPIs =  "LsaGetGroupsForUserByName";
    CHAR  szTestMsg[256] = { 0 };
    
    for (dwIndex = 0; dwIndex < pTestData->dwNumUsers; dwIndex++)
    {
        if (StringsAreEqual(pszMember, pTestData->ppUserList[dwIndex]->pszNetBiosName))  
        {
            if (pTestData->ppUserList[dwIndex]->nUserGid != pGroupList->nGid)
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                        "Group member:%s have different GID:'%lu' than its \
                        associated group:'%s's gid:'%lu'\n",
                        pszMember,
                        (unsigned long)pTestData->ppUserList[dwIndex]->nUserGid,
                        (unsigned long)pGroupList->pszName, pGroupList->nGid);
                goto error;
            }
        }
    }

    if (dwIndex == pTestData->dwNumUsers)
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, 
                sizeof(szTestMsg), 
                "Group member:%s is not found in userlist structure\n",
                pszMember);
        goto error;
    }

cleanup:
    return dwError;

error:
    LWT_LOG_TEST(szTestMsg);
    goto cleanup;
}


/*
 * SearchAndValidateGroupMember 
 * 
 * Function checks for existance of members in one list with the other list  
 * 
 */
static 
DWORD
SearchAndValidateGroupMember(
    PSTR* ppszGrpMemberList_1,
    PSTR* ppszGrpMemberList_2
    )
{    
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwFirstMemberList = 0;
    PCSTR pszTestDescription = 
                "Validating Group members in AD group and csv group structure";
    PCSTR pszTestAPIs =  "LsaGetGroupsForUserByName";
    CHAR  szTestMsg[256] = { 0 };

    while (!IsNullOrEmpty(ppszGrpMemberList_1[dwFirstMemberList]))
    {
        BOOL bFound = FALSE;
        DWORD dwSecondMemberList = 0;

        while (!IsNullOrEmpty(ppszGrpMemberList_2[dwSecondMemberList]))
        {
            if (StringsAreEqual(ppszGrpMemberList_1[dwFirstMemberList], 
                ppszGrpMemberList_2[dwSecondMemberList]))
            {
                bFound = TRUE;
                break;
            }
            dwSecondMemberList++;            
        }
    
        if (FALSE == bFound)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, 
                    sizeof(szTestMsg), 
                    "Group member:'%s' is missing either in \
                    AD group list or group list structure\n",  
                    ppszGrpMemberList_1[dwFirstMemberList]);
            goto error;
        }
        dwFirstMemberList++;
    }

cleanup:
    return dwError;

error:
    LWT_LOG_TEST(szTestMsg);
    goto cleanup;
}
    

/*
 * ValidateGroupMembers
 * 
 * Function validates group informations of group members.    
 * 
 */
static 
DWORD
ValidateGroupMembers(
    PSTR* ppszMembers,
    PLWTGROUP pGroupList,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwMember = 0;

    dwLocalError = SearchAndValidateGroupMember(pGroupList->ppszMembers, 
                                                ppszMembers);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = SearchAndValidateGroupMember(ppszMembers, 
                                                pGroupList->ppszMembers);
    BAIL_ON_TEST_BROKE(dwLocalError);

    while (!IsNullOrEmpty(ppszMembers[dwMember]))
    {
        dwLocalError = ValidateGroupMember(ppszMembers[dwMember], 
                                            pGroupList, 
                                            pTestData);
        BAIL_ON_TEST_BROKE(dwLocalError);
        dwMember++;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


/*
 * ValidateGroupInfoLeve1
 * 
 * Function validates group level 1 information 
 * from AD with group information in csv file
 * 
 */
static
DWORD
ValidateGroupInfoLevel1(
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PLWTGROUP pGroupList,
    PTESTDATA pTestData
    )    
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = "Comparing Level 1 Group information of a user";
    PCSTR pszTestAPIs =  "LsaGetGroupsForUserByName";
    CHAR  szTestMsg[256] = { 0 };

    if (!pGroupList)
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, 
                    sizeof(szTestMsg), 
                    "CSV file don't have group information for group:%s\n", 
                    pADGroupInfoList->pszName);
        goto error;
    }

    if ((!pGroupList->ppszMembers && pADGroupInfoList->ppszMembers) || 
            (pGroupList->ppszMembers && !pADGroupInfoList->ppszMembers))
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, 
                sizeof(szTestMsg), 
                "Mismatch in Group member list in AD and \
                group structure for group:'%s'\n", 
                pADGroupInfoList->pszName);
        goto error;
    }
    else if (pGroupList->ppszMembers && pADGroupInfoList->ppszMembers)
    {
        dwLocalError = ValidateGroupMembers(pADGroupInfoList->ppszMembers, 
                                            pGroupList, 
                                            pTestData);
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    if (pGroupList->nGid != pADGroupInfoList->gid)
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, 
                    sizeof(szTestMsg), 
                    "GID value mismatch for group:'%s', \
                    csv file gives:%lu and API returns:%lu\n", 
                    pADGroupInfoList->pszName,
                    (unsigned long)pGroupList->nGid,
                    (unsigned long)pADGroupInfoList->gid );
        goto error;
    }

    if (!StringsAreEqual(pGroupList->pszSid, pADGroupInfoList->pszSid))
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, 
                    sizeof(szTestMsg), 
                    "SID value mismatch for group:'%s', \
                    csv file gives:'%s' and API returns:'%s'\n", 
                    pADGroupInfoList->pszName, 
                    pGroupList->pszSid, 
                    pADGroupInfoList->pszSid);
        goto error;
    }

       if (!StringsAreEqual(pGroupList->pszDN, pADGroupInfoList->pszDN))
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, 
                    sizeof(szTestMsg), 
                    "DN mismatch for group:'%s', \
                    csv file gives:'%s' and API returns:'%s'\n", 
                    pADGroupInfoList->pszName, 
                    pGroupList->pszDN, 
                    pADGroupInfoList->pszDN);
        goto error;
    }
    
    if (!StringsAreEqual(pGroupList->pszPassword, pADGroupInfoList->pszPasswd))
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, 
                    sizeof(szTestMsg), 
                    "Password mismatch for group:'%s', \
                    csv file gives:'%s' and API returns:'%s'\n", 
                    pADGroupInfoList->pszName, 
                    pGroupList->pszPassword, 
                    pADGroupInfoList->pszPasswd);
        goto error;
    } 

cleanup:
    return dwError;

error:
    LWT_LOG_TEST(szTestMsg);
    goto cleanup;
}


/*
 * CompareGroupInfoLevels
 * 
 * Function compares group information in level 0 and level1 
 * 
 */
static
DWORD
CompareGroupInfoLevels(
    PLSA_GROUP_INFO_0 *ppListGrpInfoLevel0,
    DWORD dwGrpCountInLevel0,
    PLSA_GROUP_INFO_1 *ppListGrpInfoLevel1,
    DWORD dwGrpCountInLevel1
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwArrIndex1 = 0;
    DWORD dwArrIndex2 = 0;
    BOOL  bUserFound = FALSE;
    PCSTR pszTestDescription = 
                    "Comparing Group information between level 0 and level 1";
    PCSTR pszTestAPIs =  "LsaGetGroupsForUserByName";
    CHAR  szTestMsg[256] = { 0 };

    for (dwArrIndex1 = 0;dwArrIndex1 < dwGrpCountInLevel0; dwArrIndex1++)
    {
        bUserFound = FALSE;
        for (dwArrIndex2 = 0; dwArrIndex2 < dwGrpCountInLevel1; dwArrIndex2++)
        {
            if (ppListGrpInfoLevel0[dwArrIndex1] && 
                    ppListGrpInfoLevel1[dwArrIndex2])
            {
                if (StringsAreEqual(ppListGrpInfoLevel0[dwArrIndex1]->pszName, 
                        ppListGrpInfoLevel1[dwArrIndex2]->pszName))
                {
                    bUserFound = TRUE;
                    if (ppListGrpInfoLevel0[dwArrIndex1]->gid != ppListGrpInfoLevel1[dwArrIndex2]->gid)
                    {
                        dwError = LW_ERROR_TEST_FAILED;
                        snprintf(szTestMsg, sizeof(szTestMsg), "GID mismatch for group:'%s', level 0 value:'%lu' and Level 1 value:'%lu'\n",
                            ppListGrpInfoLevel0[dwArrIndex1]->pszName,
                            (unsigned long)ppListGrpInfoLevel0[dwArrIndex1]->gid, 
                            (unsigned long)ppListGrpInfoLevel1[dwArrIndex2]->gid);
                        LWT_LOG_TEST(szTestMsg);
                        goto error;
                    }
                    if (!StringsAreEqual(ppListGrpInfoLevel0[dwArrIndex1]->pszSid, ppListGrpInfoLevel1[dwArrIndex2]->pszSid))
                    {
                        dwError = LW_ERROR_TEST_FAILED;
                        snprintf(szTestMsg, sizeof(szTestMsg), "SID value mismatch for group:'%s', level 0 value:'%s' and Level 1 value:'%s'\n",
                            ppListGrpInfoLevel0[dwArrIndex1]->pszName, ppListGrpInfoLevel0[dwArrIndex1]->pszSid, ppListGrpInfoLevel1[dwArrIndex2]->pszSid);
                        LWT_LOG_TEST(szTestMsg);
                        goto error;
                    }
                }
            }
        }

        if (FALSE == bUserFound)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), "User:'%s' doesn't exists in Group_Info_level 1\n", ppListGrpInfoLevel0[dwArrIndex1]->pszName);
               LWT_LOG_TEST(szTestMsg);
            goto error;
        }
    }
        
cleanup:
    return dwError;

error:
    goto cleanup;

}


/*
 * GetGroupListData
 * 
 * Function gets the csv grouplist for a group
 * 
 */
static
DWORD
GetGroupListData(
    PSTR      pszGroupName,
    PTESTDATA pTestData,
    PLWTGROUP*   ppGroupList
    )
{
    DWORD  dwError = LW_ERROR_SUCCESS;
    DWORD  dwGroupIndex = 0;
    PLWTGROUP pGroupList = NULL;

    for (dwGroupIndex = 0; dwGroupIndex < pTestData->dwNumGroups; dwGroupIndex++)
    {
        if (StringsNoCaseAreEqual(pszGroupName, 
            pTestData->ppGroupList[dwGroupIndex]->pszName))
        {
            pGroupList = pTestData->ppGroupList[dwGroupIndex];
            break;
        }
    }

/*cleanup: */
    *ppGroupList = pGroupList;
    return dwError;
/*
error:
    goto cleanup;*/
}


/*
 * VerifyUserGroupInfo
 * 
 * Function gets and validates the group information for a user 
 * 
 */
static
DWORD 
VerifyUserGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    PCSTR pszUserName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwNumGroups = 0;    
    DWORD dwGroupInfoLevel = 0;
    DWORD dwGrpCountInLevel0 = 0;
    DWORD dwGrpCountInLevel1 = 0;
    PVOID *ppGroupInfoList = NULL;
    LSA_FIND_FLAGS nFindFlags = 0;
    PLSA_GROUP_INFO_0 *ppListGrpInfoLevel0 = NULL;
    PLSA_GROUP_INFO_1 *ppListGrpInfoLevel1 = NULL;

    do 
    {
        DWORD  nIndex = 0;
        PLWTGROUP pGroupList = NULL;
        dwLocalError = LsaGetGroupsForUserByName(
                                    hLsaConnection,
                                    pszUserName,
                                    nFindFlags,
                                    dwGroupInfoLevel,
                                    &dwNumGroups,
                                    &ppGroupInfoList);
        BAIL_ON_TEST_BROKE(dwLocalError);

        switch (dwGroupInfoLevel)
        {    
            case 0:    
                ppListGrpInfoLevel0 = (PLSA_GROUP_INFO_0*)ppGroupInfoList;
                dwGrpCountInLevel0  = dwNumGroups;
                break;
                
            case 1:
                ppListGrpInfoLevel1 = (PLSA_GROUP_INFO_1*)ppGroupInfoList;
                dwGrpCountInLevel1  = dwNumGroups;
                break;

            default:
                break;
        }


        for (nIndex = 0; nIndex < dwNumGroups; nIndex++)
        {
            switch (dwGroupInfoLevel)
            {
               case 0:
                if (ppListGrpInfoLevel0[nIndex])
                {
                    GetGroupListData(ppListGrpInfoLevel0[nIndex]->pszName, 
                                        pTestData, 
                                        &pGroupList);
                    dwLocalError = ValidateGroupInfoLevel0(ppListGrpInfoLevel0[nIndex], 
                                                            pGroupList);
                    BAIL_ON_TEST_BROKE(dwLocalError);
                }
                break;

            case 1:
                if (ppListGrpInfoLevel1[nIndex])
                {
                    GetGroupListData(ppListGrpInfoLevel1[nIndex]->pszName, 
                                      pTestData, 
                                      &pGroupList);
                    dwLocalError = ValidateGroupInfoLevel1(ppListGrpInfoLevel1[nIndex], 
                                                            pGroupList, pTestData);
                    BAIL_ON_TEST_BROKE(dwLocalError);
                }
                break;

            default:
                dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;
                BAIL_ON_TEST_BROKE(dwError);
                break;
            }
        }
        dwGroupInfoLevel++;
    } while (dwGroupInfoLevel <= MAX_GROUP_INFOLEVEL);

    if (ppListGrpInfoLevel0 && ppListGrpInfoLevel1)
    {
        dwLocalError = CompareGroupInfoLevels(ppListGrpInfoLevel0, 
                                                dwGrpCountInLevel0, 
                                                ppListGrpInfoLevel1, 
                                                dwGrpCountInLevel1);
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

cleanup:
    if (ppListGrpInfoLevel0)
    {
        LsaFreeGroupInfoList(0, 
                            (PVOID*)ppListGrpInfoLevel0, 
                            dwGrpCountInLevel0); 
        ppListGrpInfoLevel0 = NULL;
    }

    if (ppListGrpInfoLevel1)
    {
        LsaFreeGroupInfoList(1, (PVOID*)ppListGrpInfoLevel1, dwGrpCountInLevel1);            
        ppListGrpInfoLevel1 = NULL;
    }

    return dwError;

error:
    goto cleanup;
}


/*
 * Lwt_LsaValidateUsersGroupInfo
 * 
 * Function gets and validates the group information for users in userlist structure 
 * 
 */
DWORD
Lwt_LsaValidateUsersGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData    
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD nUserIndex = 0;

    if ( !pTestData || !pTestData->ppUserList || !pTestData->ppGroupList )
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        goto error;
    }
    
    for (nUserIndex = 0 ; nUserIndex < pTestData->dwNumUsers; nUserIndex++)
    {
        PSTR pszName = NULL;
        
        if (!IsNullOrEmpty(pTestData->ppUserList[nUserIndex]->pszUserPrincipalName))
        {
            pszName  = pTestData->ppUserList[nUserIndex]->pszUserPrincipalName;
        }
        else if (!IsNullOrEmpty(pTestData->ppUserList[nUserIndex]->pszSamAccountName))
        {
            pszName = pTestData->ppUserList[nUserIndex]->pszSamAccountName;
        }
        else
        {
            continue;
        }
            
        dwLocalError = VerifyUserGroupInfo(hLsaConnection, pTestData, pszName);
        BAIL_ON_TEST_BROKE(dwLocalError); 
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

/*
 * Lwt_LsaNullAndInvalidParamsTest
 * 
 * Function validates the API for NULL and invalid parameters. 
 * 
 */
DWORD 
Lwt_LsaNullAndInvalidParamsTest(
    HANDLE hLsaConnection,
    PSTR  pszUserName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwNumGroups = 0;
    DWORD dwGroupInfoLevel = 0;
    PVOID *ppGroupInfoList = NULL;
    PCSTR pszTestDescription = "Testing the API behavior for NULL and Invalid parameters";
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName";
    CHAR  szTestMsg[256] = { 0 };
    LSA_FIND_FLAGS nFindFlags = 0;

    do
    {
#if 0
        /* Crashes the system - Testing NULL connection handler parameter*/
        dwLocalError = LsaGetGroupsForUserByName( 
                                    NULL, 
                                    pszUserName, 
                                    nFindFlags, 
                                    dwGroupInfoLevel, 
                                    &dwNumGroups, 
                                    &ppGroupInfoList);
        if (!dwLocalError) 
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), "API failed for NULL Connection handler parameter in level %lu\n", (unsigned long)dwGroupInfoLevel);
            LWT_LOG_TEST(szTestMsg);
        }

        if (ppGroupInfoList) 
        { 
            LsaFreeGroupInfoList(
                        dwGroupInfoLevel, 
                        ppGroupInfoList, 
                        dwNumGroups);
            ppGroupInfoList = NULL; 
        }
#endif
        /* Testing NULL username parameter - Function doesn't return error for NULL username. */
        dwLocalError = LsaGetGroupsForUserByName(
                                    hLsaConnection, 
                                    NULL, 
                                    nFindFlags, 
                                    dwGroupInfoLevel, 
                                    &dwNumGroups, 
                                    &ppGroupInfoList);
        if (!dwLocalError) 
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg,
                sizeof(szTestMsg),
                "API failed for NULL User name parameter in level %lu\n",
                (unsigned long)dwGroupInfoLevel);
            LWT_LOG_TEST(szTestMsg);
        }

        if (ppGroupInfoList) 
        { 
            LsaFreeGroupInfoList(
                        dwGroupInfoLevel, 
                        ppGroupInfoList, 
                        dwNumGroups);
            ppGroupInfoList = NULL; 
        }
#if 0
        /* Crashes the system - Testing NULL pointer, representing number of groups, parameter */
        dwLocalError = LsaGetGroupsForUserByName(
                                    hLsaConnection, 
                                    pszUserName, 
                                    nFindFlags, 
                                    dwGroupInfoLevel, 
                                    (PDWORD)NULL, 
                                    &ppGroupInfoList);
        if (!dwLocalError) 
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), "API failed for NULL pointer for number of groups, parameter in level %lu\n",
                    (unsigned long) dwGroupInfoLevel);
            LWT_LOG_TEST(szTestMsg);
        }

        if (ppGroupInfoList) 
        { 
            LsaFreeGroupInfoList(
                        dwGroupInfoLevel, 
                        ppGroupInfoList, 
                        dwNumGroups);
            ppGroupInfoList = NULL; 
        }
        /* Crashes the system - "Testing NULL GroupInfoList pointer */
        dwLocalError = LsaGetGroupsForUserByName(
                                    hLsaConnection, 
                                    pszUserName, 
                                    nFindFlags, 
                                    dwGroupInfoLevel, 
                                    &dwNumGroups,
                                    (PVOID**)NULL);
        if (!dwLocalError) 
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), "API failed for NULL Group Info List pointer parameter in level %lu\n", (unsigned long)dwGroupInfoLevel);
            LWT_LOG_TEST(szTestMsg);
        }

        if (ppGroupInfoList) 
        { 
            LsaFreeGroupInfoList(
                        dwGroupInfoLevel, 
                        ppGroupInfoList, 
                        dwNumGroups);
            ppGroupInfoList = NULL; 
        }
#endif
        /* Testing invalid FIND_FLAG parameter */
        dwLocalError = LsaGetGroupsForUserByName( 
                                    hLsaConnection, 
                                    pszUserName,
                                    -1, 
                                    dwGroupInfoLevel, 
                                    &dwNumGroups,
                                    &ppGroupInfoList);
        if (!dwLocalError) 
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, 
                        sizeof(szTestMsg), 
                        "API failed for invalid FIND_FLAGS parameter in level %lu\n", 
                        (unsigned long)dwGroupInfoLevel);
            LWT_LOG_TEST(szTestMsg);
        }   

        if (ppGroupInfoList) 
        { 
            LsaFreeGroupInfoList(
                        dwGroupInfoLevel, 
                        ppGroupInfoList, 
                        dwNumGroups);
            ppGroupInfoList = NULL; 
        }

        dwLocalError = LsaGetGroupsForUserByName(
                                    hLsaConnection, 
                                    pszUserName,
                                    1234567, 
                                    dwGroupInfoLevel, 
                                    &dwNumGroups,
                                    &ppGroupInfoList);
        if (!dwLocalError) 
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, 
                    sizeof(szTestMsg), 
                    "API failed for invalid FIND_FLAGS parameter in level %lu\n", 
                    (unsigned long)dwGroupInfoLevel);
            LWT_LOG_TEST(szTestMsg);
        }

        if (ppGroupInfoList) 
        { 
            LsaFreeGroupInfoList(
                        dwGroupInfoLevel, 
                        ppGroupInfoList, 
                        dwNumGroups);
            ppGroupInfoList = NULL; 
        }
    
        dwGroupInfoLevel++;
    }while (dwGroupInfoLevel <= MAX_GROUP_INFOLEVEL);


cleanup:
    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(
                dwGroupInfoLevel, 
                ppGroupInfoList, 
                dwNumGroups);
        ppGroupInfoList = NULL;
    }

    return dwError;

error:
    goto cleanup;
}


int 
check_group_info_main(
    int  argc, 
    char *argv[]
    )
{
    DWORD  dwError = LW_ERROR_SUCCESS;
    DWORD  dwLocalError = LW_ERROR_SUCCESS;
    HANDLE  hLsaConnection = (HANDLE)NULL;
    PTESTDATA pTestData = NULL;
    PSTR pszName = NULL;
    DWORD dwUserCount = 0;

    dwError = Lwt_LsaTestSetup(
                   argc,
                   argv,
                   &hLsaConnection,
                   &pTestData);
    if ( dwError )
        goto error;


    dwLocalError = Lwt_LsaValidateUsersGroupInfo(hLsaConnection, pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

    /* Getting a valid user name to do NULL and INVALID parameter test for API*/
    do 
    { 
        if (!IsNullOrEmpty(pTestData->ppUserList[dwUserCount]->pszUserPrincipalName))
        {
            pszName  = pTestData->ppUserList[dwUserCount]->pszUserPrincipalName;
            break;
        }
        else if (!IsNullOrEmpty(pTestData->ppUserList[dwUserCount]->pszSamAccountName))
        {
            pszName = pTestData->ppUserList[dwUserCount]->pszSamAccountName;
            break;
        }

    } while (++dwUserCount < pTestData->dwNumUsers);

    if (dwUserCount < pTestData->dwNumUsers)
    {
        dwLocalError = Lwt_LsaNullAndInvalidParamsTest(hLsaConnection, pszName);
        BAIL_ON_TEST_BROKE(dwLocalError);
    }
    
    dwLocalError = ValidateForInvalidParams(hLsaConnection, pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:
    Lwt_LsaTestTeardown(&hLsaConnection, &pTestData);
    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;
}

/*
 * ValidateForInvalidParams
 * 
 * Function validates the API for Invalid function parameters.
 * 
 */
static
DWORD 
ValidateForInvalidParams(
    HANDLE hLSAConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwTest = 0;
    PLWTFAILDATA pInvalidData = NULL;
    PSTR pszMessage = NULL;

    if ( !pTestData || !pTestData->pInvalidDataIface )
    {
        dwLocalError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    for ( dwTest = 0; dwTest < pTestData->dwNumInvalidDataSet; dwTest++ )
    {
        dwLocalError = GetInvalidDataRecord(
                                        pTestData, 
                                        dwTest, 
                                        &pInvalidData);
        BAIL_ON_TEST_BROKE(dwLocalError);

        /* Testing invalid username parameter */
        if ( LWTUSER_INVALID == pInvalidData->Field )
        {
            pszMessage = "user name";
            dwLocalError = CheckForInvalidParams(
                                        hLSAConnection,
                                        pInvalidData,
                                        pszMessage);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        if ( LWTGROUPINFOLEVEL_INVALID == pInvalidData->Field )
        {
            /* Testing invalid GroupInfoLevel parameter*/
            pszMessage = "API returned with error code (%lu) for invalid group info level parameter"
            dwLocalError = CheckForInvalidParams(
                                        hLSAConnection,
                                        pInvalidData,
                                        pszMessage);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }
        FreeInvalidDataRecord(pInvalidData);
    }

error:
    return dwError;
}

static
DWORD
CheckForInvalidParams(
        HANDLE hLsaConnection,
        PLWTFAILDATA pInvalidData,
        DWORD dwGroupInfoLevel,
        PSTR pszMessage
        )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwNumGroups = 0;
    PVOID *ppGroupInfoList = NULL;
    LSA_FIND_FLAGS nFindFlags = 0;
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName";
    PCSTR pszTestDescription = 
            "API returns invalid parameter error for invalid function parameters";
    CHAR  szTestMsg[256] = { 0 };

    dwError = LsaGetGroupsForUserByName(
                                hLsaConnection, 
                                pInvalidData->pszUserName,
                                nFindFlags, 
                                pInvalidData->dwLevel, 
                                &dwNumGroups,
                                &ppGroupInfoList);

    if ( dwLocalError != pInvalidData->dwErrorCode )
    {
        snprintf( 
            szTestMsg, 
            sizeof(szTestMsg), 
            "API returned with error code (%lu) for invalid (%s) parameter"
            (unsigned long)dwLocalError,
            pszMessage);
        LWT_LOG_TEST(szTestMsg);
        dwError = LW_ERROR_TEST_FAILED;
    }

    if (ppGroupInfoList) 
    { 
        LsaFreeGroupInfoList(
                    pInvalidData->dwLevel, 
                    ppGroupInfoList, 
                    dwNumGroups);
        ppGroupInfoList = NULL; 
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}
