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
 * Module Name: lwt_lsa_validate_groupinfo_common.c
 *
 * Verifies the group information returned from AD with the CSV file.
 *
 */

#include "lwt_lsa_validate_groupinfo_common.h"

/*
 * ValidateMemberWithUsers
 * 
 * Function searches the group member in user list and validates the user 
 * information and group member information and make sure that both are same
 * 
 */
static
DWORD
ValidateMemberWithUsers(
    PSTR pszMember,
    PLWTGROUP pGroupList,
    PTESTDATA pTestData,
    PCSTR pszTestAPI
    );

/*
 * LogInvalidGroupMembers
 * 
 * Function gets inconsistent group members and logs it to file
 * 
 */
static
DWORD
LogInvalidGroupMembers( 
    PLWTGROUP pGroupList,
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PSTR pszUserInfo,
    PCSTR pszTestAPI
    );

/*
 * SearchAndValidateGroupMember
 * 
 * Function validates the group member for Sid, Gid 
 * 
 */
static
DWORD
SearchAndValidateGroupMember( 
    PLSA_GROUP_INFO_1 *ppListGrpInfoLevel1,
    PLSA_GROUP_INFO_0 pGrpInfoLevel0,
    DWORD dwLevel1GrpCount,
    PSTR pszUserInfo,
    PCSTR pszTestApi
    );

/*
 * ProcessGroupMembers
 * 
 * Function process the group member for valid and invalid group members
 * 
 */
static
DWORD
ProcessGroupMembers( 
    PTESTDATA pTestData,
    PLWTGROUP pGroupList,
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PSTR pszUserInfo,
    PCSTR pszTestAPI
    );

/*
 * ProcessGroupListMembers
 * 
 * Function process the list of group member for Sid, Gid, DN and 
 * password values
 * 
 */
static
DWORD
ProcessGroupListMembers( 
    PLWTGROUP pGroupList,
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PSTR pszUserInfo,
    PCSTR pszTestAPI
    );

/*
 * ProcessGroupListMembers
 * 
 * Function searches for group member in group list
 * 
 */
static
DWORD
SearchGroupMember( 
    PSTR* ppszGrpMemberList,
    PSTR  pszGrpMember
    );

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
    PTESTDATA pTestData,
    PSTR pszUserInfo,
    PCSTR pszTestAPI
    );

/*
 * GetGroupsForUserByName
 *   
 * Function retrives the group information for user from AD
 * 
 */
DWORD
GetGroupsForUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszUserName,
    DWORD  dwGroupInfoLevel,
    PVOID  **pppGroupInfoList,
    PDWORD pdwGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[256] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName()";
    PCSTR pszTestDescription = 
        "LsaGetGroupsForUserByName() retrieved group information for users";
    LSA_FIND_FLAGS nFindFlags = 0;

    snprintf( szTestMsg,
              sizeof(szTestMsg),
              "Retrieving '%lu' level Group information for User:'%s' ",
              (unsigned long)dwGroupInfoLevel,
              pszUserName);

    dwLocalError = LsaGetGroupsForUserByName( hLsaConnection,
                                              pszUserName,
                                              nFindFlags,
                                              dwGroupInfoLevel,
                                              pdwGroups,
                                              pppGroupInfoList);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:
    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:
    if ( *pppGroupInfoList )
    {
        LsaFreeGroupInfoList( dwGroupInfoLevel,
                              *pppGroupInfoList,
                              *pdwGroups);

        *pppGroupInfoList = NULL;
    }
    goto cleanup;
}

/*
 * GetGroupsForUserById
 *
 * Function retrives the group information for user from AD
 *
 */
DWORD 
GetGroupsForUserById(
    HANDLE hLsaConnection,
    uid_t  nUnixUid,
    DWORD  dwGroupInfoLevel,
    PVOID  **pppGroupInfoList,
    PDWORD pdwNumGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[256] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserById()";
    PCSTR pszTestDescription = 
        "API retrieves group information for users by using user id as key";
    LSA_FIND_FLAGS nFindFlags = 0;

    dwLocalError = LsaGetGroupsForUserById(
                                hLsaConnection,
                                nUnixUid,
                                nFindFlags,
                                dwGroupInfoLevel,
                                pdwNumGroups,
                                pppGroupInfoList
                                );

    if ( dwLocalError != LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned unexpected error code (%lu) for \
                  user id (%lu).",
                  (unsigned long)dwLocalError,
                  (unsigned long)nUnixUid);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 * ResolveGroupListByName
 * 
 * Function gets the csv grouplist for a group
 * 
 */
DWORD
ResolveGroupListByName(
    PSTR       pszGroupName,
    PTESTDATA  pTestData,
    PLWTGROUP* ppGroupList
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwGroupIndex = 0;
    PLWTGROUP pGroupList = NULL;

    for ( dwGroupIndex = 0; 
          dwGroupIndex < pTestData->dwNumGroups; 
          dwGroupIndex++ )
    {
        dwLocalError = GetGroup( pTestData, 
                                 dwGroupIndex, 
                                 &pGroupList);
        BAIL_ON_TEST_BROKE(dwLocalError);
 
        if ( pGroupList )
        {
            if ( StringsNoCaseAreEqual(pszGroupName, pGroupList->pszName) )
            {
                break;
            }
            FreeGroup(&pGroupList);
        }
    }

cleanup: 
    *ppGroupList = pGroupList;
    return dwError;

error:
    FreeGroup(&pGroupList);
    pGroupList = NULL;
    goto cleanup;
}

/*
 * ValidateGroupInfoLevel0
 * 
 * Function validates group level 0 information from AD with group 
 * information in csv file
 * 
 */
DWORD 
ValidateGroupInfoLevel0(
    PLSA_GROUP_INFO_0 pADGroupInfoList,
    PLWTGROUP pGroupList,
    PSTR  pszLogInfo,
    PCSTR pszTestAPI
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = pszTestAPI;
    PCSTR pszTestDescription = 
        "Validating level 0 group information returned by API with \
        AD group information";

    if ( !pGroupList || !pADGroupInfoList )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group information is missing for %s\n", 
                  pszLogInfo);
        LWT_LOG_TEST(szTestMsg);

        goto error;
    }

    if ( pGroupList->nGid != pADGroupInfoList->gid )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group gid mismatch for %s. API returned gid (%lu) but \
                  AD shows gid (%lu)\n",
                  pszLogInfo,
                  (unsigned long) pADGroupInfoList->gid, 
                  (unsigned long) pGroupList->nGid);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( !StringsAreEqual(pGroupList->pszSid, pADGroupInfoList->pszSid) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group sid mismatch for %s. API returned sid '%s' but \
                  AD shows sid '%s'\n", 
                  pszLogInfo,
                  pADGroupInfoList->pszSid,
                  pGroupList->pszSid);
        LWT_LOG_TEST(szTestMsg);
    }    

error:
    return dwError;
}

/*
 * ValidateGroupInfoLevel1
 * 
 * Function validates group level 1 information from AD with group 
 * information in csv file
 * 
 */
DWORD
ValidateGroupInfoLevel1(
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PLWTGROUP pGroupList,
    PTESTDATA pTestData,
    PSTR  pszUserInfo,
    PCSTR pszTestAPI
    )    
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = pszTestAPI;
    PCSTR pszTestDescription = 
        "Validating level 1 group information returned by API with AD \
        group information";

    if ( !pGroupList || !pADGroupInfoList )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group information is missing for %s\n", 
                  pszUserInfo);
        LWT_LOG_TEST(szTestMsg);

        goto error;
    }

    dwLocalError = ProcessGroupMembers( pTestData,
                                        pGroupList,
                                        pADGroupInfoList,
                                        pszUserInfo,
                                        pszTestAPIs);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = ProcessGroupListMembers( pGroupList,
                                            pADGroupInfoList,
                                            pszUserInfo,
                                            pszTestAPIs);
    BAIL_ON_TEST_BROKE(dwLocalError);
    


error:
    return dwError;
}

/*
 * CompareGroupInfoLevels
 * 
 * Function compares group information in level 0 and level1 
 * 
 */
DWORD
CompareGroupInfoLevels(
    PLSA_GROUP_INFO_0 *ppListGrpInfoLev0,
    DWORD dwLevel0GrpCount,
    PLSA_GROUP_INFO_1 *ppListGrpInfoLevel1,
    DWORD dwLevel1GrpCount,
    PSTR  pszUserInfo,
    PCSTR pszTestApi
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwGroup = 0;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs =  pszTestApi;
    PCSTR pszTestDescription = 
        "API returns consistent group information in level 0 and level 1";

    if ( !ppListGrpInfoLev0 || !ppListGrpInfoLevel1 )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Level0 and Level1 group informations missing for %s\n",
                  pszUserInfo);
        LWT_LOG_TEST(szTestMsg);

        goto error;
    }

    while ( dwGroup < dwLevel0GrpCount && ppListGrpInfoLev0[dwGroup] )
    {
        dwLocalError = SearchAndValidateGroupMember( ppListGrpInfoLevel1,
                                                     ppListGrpInfoLev0[dwGroup],
                                                     dwLevel1GrpCount,
                                                     pszUserInfo,
                                                     pszTestAPIs);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwGroup++;
    }
   
error:
    return dwError;
}

/*
 * CompareMemberList 
 * 
 * Function checks for existance of members in one list with the other list  
 * 
 */
DWORD
CompareMemberList(
    PSTR* ppszGrpMemberList_1,
    PSTR* ppszGrpMemberList_2,
    PBOOLEAN pbEqual 
    )
{    
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwMember = 0;
    BOOLEAN bGroupMemberFound = TRUE;

    while ( !IsNullOrEmpty(ppszGrpMemberList_1[dwMember]) )
    {
        dwError = SearchGroupMember( ppszGrpMemberList_2,
                                     ppszGrpMemberList_1[dwMember]);

        if ( LW_ERROR_TEST_FAILED == dwError )
        {
            bGroupMemberFound = FALSE;
            break;
        }

        dwMember++;
    }

    *pbEqual = bGroupMemberFound;
    return dwError;
}

/*
 * ProcessGroupListMembers
 * 
 * Function process the list of group member for Sid, Gid, DN and 
 * password values
 * 
 */
static
DWORD
ProcessGroupListMembers( 
    PLWTGROUP pGroupList,
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PSTR  pszUserInfo,
    PCSTR pszTestAPI
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = pszTestAPI;
    PCSTR pszTestDescription = 
        "Validating level 1 group information returned by API with AD \
        group information";

    if ( pGroupList->nGid != pADGroupInfoList->gid )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group gid mismatch for %s in group '%s'. API returned \
                  gid (%lu) but AD shows (%lu)\n",
                  pszUserInfo,
                  pADGroupInfoList->pszName,
                  (unsigned long) pADGroupInfoList->gid, 
                  (unsigned long) pGroupList->nGid);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( !StringsAreEqual(pGroupList->pszSid, pADGroupInfoList->pszSid) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group sid mismatch for %s in group '%s'. API returned \
                  sid '%s'  but AD shows '%s'\n",
                  pszUserInfo,
                  pADGroupInfoList->pszName,
                  pADGroupInfoList->pszSid,
                  pGroupList->pszSid); 
        LWT_LOG_TEST(szTestMsg);
    }

    if ( !StringsAreEqual(pGroupList->pszDN, pADGroupInfoList->pszDN) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group DN mismatch for %s in group '%s'. API returned \
                  DN '%s'  but AD shows '%s'\n",
                  pszUserInfo,
                  pADGroupInfoList->pszName,
                  pADGroupInfoList->pszDN,
                  pGroupList->pszDN);
        LWT_LOG_TEST(szTestMsg);
    }
    
    if ( !StringsAreEqual( pGroupList->pszPassword, 
                           pADGroupInfoList->pszPasswd) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group password mismatch for %s in group '%s'. API returned \
                  password '%s' but AD shows '%s'\n",
                  pszUserInfo,
                  pADGroupInfoList->pszName,
                  pADGroupInfoList->pszPasswd,
                  pGroupList->pszPassword);
        LWT_LOG_TEST(szTestMsg);
    } 

    return dwError;
}

/*
 * LogInvalidGroupMembers
 * 
 * Function gets inconsistent group members and logs it to file
 * 
 */
static
DWORD
LogInvalidGroupMembers( 
    PLWTGROUP pGroupList,
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PSTR  pszUserInfo,
    PCSTR pszTestAPI
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwMember = 0;
    CHAR  szMembers[256] = { 0 };
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = pszTestAPI;
    PCSTR pszTestDescription = 
        "Validating level 1 group information returned by API with AD \
        group information";

    if ( pGroupList->ppszMembers )
    {
        snprintf( szMembers, 
                  sizeof(szMembers), 
                  "API returned zero group members where AD shows members:");

        while ( pGroupList->ppszMembers[dwMember] )
        {
            strcat(szMembers, pGroupList->ppszMembers[dwMember]);
            strcat(szMembers, ", ");
            dwMember++;
        }
    }
    else if ( pADGroupInfoList->ppszMembers )
    {
        snprintf( szMembers, 
                  sizeof(szMembers), 
                  "AD shows empty group members where API returns \
                  group members:");

        while ( pADGroupInfoList->ppszMembers[dwMember] )
        {
            strcat(szMembers, pADGroupInfoList->ppszMembers[dwMember]);
            strcat(szMembers, ", ");
            dwMember++;
        }
    }

    dwError = LW_ERROR_TEST_FAILED;
    snprintf( szTestMsg, 
              sizeof(szTestMsg), 
              "Group members are mismatched for %s in group:'%s'. %s",
              pszUserInfo,                    
              pADGroupInfoList->pszName,
              szMembers);
    LWT_LOG_TEST(szTestMsg);

    return dwError;
}

/*
 * ProcessGroupMembers
 * 
 * Function process the group memberfor valid and invalid group members
 * 
 */
static
DWORD
ProcessGroupMembers( 
    PTESTDATA pTestData,
    PLWTGROUP pGroupList,
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PSTR  pszUserInfo,
    PCSTR pszTestAPI
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    if ( ( !pGroupList->ppszMembers && pADGroupInfoList->ppszMembers ) || 
         ( pGroupList->ppszMembers && !pADGroupInfoList->ppszMembers ) )
    {
        dwLocalError = LogInvalidGroupMembers( pGroupList,
                                               pADGroupInfoList, 
                                               pszUserInfo,
                                               pszTestAPI);
        BAIL_ON_TEST_BROKE(dwLocalError);
    }
    else if ( pGroupList->ppszMembers && pADGroupInfoList->ppszMembers )
    {
        dwLocalError = ValidateGroupMembers( pADGroupInfoList->ppszMembers, 
                                             pGroupList, 
                                             pTestData,
                                             pszUserInfo,
                                             pszTestAPI);
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

error:
    return dwError;
}

/*
 * SearchAndValidateGroupMember
 * 
 * Function validates the group member for Sid, Gid 
 * 
 */
static
DWORD
SearchAndValidateGroupMember( 
    PLSA_GROUP_INFO_1 *ppListGrpInfoLev1,
    PLSA_GROUP_INFO_0 pGrpInfoLevel0,
    DWORD dwLevel1GrpCount,
    PSTR  pszUserInfo,
    PCSTR pszTestApi
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwGroupCnt = 0;
    BOOL  bUserFound = FALSE;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = pszTestApi;
    PCSTR pszTestDescription = 
            "API returns consistent group information in level 0 and level 1";

    while ( dwGroupCnt < dwLevel1GrpCount && ppListGrpInfoLev1[dwGroupCnt] )
    {
        if ( StringsAreEqual( pGrpInfoLevel0->pszName, 
                              ppListGrpInfoLev1[dwGroupCnt]->pszName) )
        {
            if ( pGrpInfoLevel0->gid != ppListGrpInfoLev1[dwGroupCnt]->gid )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf( szTestMsg, 
                          sizeof(szTestMsg), 
                          "Group gid mismatch for %s. Gid in level0 is (%lu) \
                          where in level1 (%lu)\n", 
                          pszUserInfo,
                          (unsigned long) pGrpInfoLevel0->gid,
                          (unsigned long) ppListGrpInfoLev1[dwGroupCnt]->gid);
                LWT_LOG_TEST(szTestMsg);
            }

            if ( !StringsAreEqual( pGrpInfoLevel0->pszSid, 
                                   ppListGrpInfoLev1[dwGroupCnt]->pszSid) )
            {
                dwError = LW_ERROR_TEST_FAILED;

                snprintf( szTestMsg, 
                          sizeof(szTestMsg), 
                          "Group sid mismatch for %s. Sid in level0 is '%s' \
                          where in level1 '%s'\n", 
                          pszUserInfo,
                          pGrpInfoLevel0->pszSid, 
                          ppListGrpInfoLev1[dwGroupCnt]->pszSid);
                LWT_LOG_TEST(szTestMsg);
            }

            bUserFound = TRUE;

            break;
        }

        dwGroupCnt++;            
    }

    if ( !bUserFound )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group '%s' doesn't exists in Group_Info_level 1 for %s\n", 
                  pGrpInfoLevel0->pszName,
                  pszUserInfo);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
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
    PTESTDATA pTestData,
    PSTR  pszUserInfo,
    PCSTR pszTestAPI
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwMember = 0;
    CHAR  szADMembers[256] = { 0 };
    CHAR  szMembers[256] = { 0 };
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = pszTestAPI;
    PCSTR pszTestDescription = 
        "Validating group members returned by API with AD group information";
    BOOLEAN bEqual = FALSE;

    dwLocalError = CompareMemberList( pGroupList->ppszMembers, 
                                      ppszMembers,
                                      &bEqual);
    if ( bEqual )
    {
        dwLocalError = CompareMemberList( ppszMembers, 
                                          pGroupList->ppszMembers,
                                          &bEqual);
    }

    if ( !bEqual )
    {
        while ( pGroupList->ppszMembers[dwMember] )
        {
            strcat(szADMembers, pGroupList->ppszMembers[dwMember]);
            strcat(szADMembers, ", ");
            dwMember++;
        }

        dwMember = 0;
        while ( !IsNullOrEmpty(ppszMembers[dwMember]) )
        {
            strcat(szMembers, ppszMembers[dwMember]);
            strcat(szMembers, ", ");
            dwMember++;
        }

        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group members are mismatched for '%s'. API returned group \
                  member list:'%s', but AD shows list:'%s'\n",
                  pszUserInfo,
                  szMembers,
                  szADMembers);
        LWT_LOG_TEST(szTestMsg);
    }

    while ( !IsNullOrEmpty(ppszMembers[dwMember]) )
    {
        dwLocalError = ValidateMemberWithUsers( ppszMembers[dwMember], 
                                                pGroupList, 
                                                pTestData,
                                                pszTestAPIs);
        BAIL_ON_TEST_BROKE(dwLocalError);
        dwMember++;
    }

error:
    return dwError;
}

/*
 * ProcessGroupListMembers
 * 
 * Function searches for group member in group list
 * 
 */
static
DWORD
SearchGroupMember( 
    PSTR* ppszGrpMemberList,
    PSTR  pszGrpMember
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwMember = 0;
    BOOLEAN bGroupMemberFound = FALSE;

    while ( !IsNullOrEmpty(ppszGrpMemberList[dwMember]) )
    {
        if ( StringsAreEqual( pszGrpMember, 
                              ppszGrpMemberList[dwMember]) )
        {
            bGroupMemberFound = TRUE;
            break;
        }
        dwMember++;            
    }

    if ( !bGroupMemberFound )
    {
        dwError = LW_ERROR_TEST_FAILED;
    }

    return dwError;
}

/*
 * ValidateMemberWithUsers
 * 
 * Function searches the group member in user list and validates the user \
 * information and group member information and make sure that both are same
 * 
 */
static
DWORD
ValidateMemberWithUsers(
    PSTR pszMember,
    PLWTGROUP pGroupList,
    PTESTDATA pTestData,
    PCSTR pszTestAPI
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwIndex = 0;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = pszTestAPI;
    PCSTR pszTestDescription = 
        "Validating gid returned by API with AD group information for level1";
    PLWTUSER pUser = NULL;
    
    for ( dwIndex = 0; dwIndex < pTestData->dwNumUsers; dwIndex++ )
    {
        dwLocalError = GetUser( pTestData, 
                                dwIndex, 
                                &pUser); 
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( pUser && StringsAreEqual(pszMember, pUser->pszNTName) )  
        {
            if ( pUser->nUnixGid != pGroupList->nGid )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf( szTestMsg, 
                          sizeof(szTestMsg), 
                          "Group GID mismatch for group member '%s' in group \
                          '%s'. User gid is (%lu), but gid of group is (%lu)", 
                          pszMember, 
                          pGroupList->pszName,
                          (unsigned long) pUser->nUnixGid, 
                          (unsigned long) pGroupList->nGid);
                LWT_LOG_TEST(szTestMsg);
            }
            break;
        }
        FreeUser(&pUser);
    }

    if ( dwIndex == pTestData->dwNumUsers )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group member:'%s' in group:'%s' is not found in userlist \
                  structure\n",
                  pszMember, 
                  pGroupList->pszName);
        LWT_LOG_TEST(szTestMsg);
    }

error:
    FreeUser(&pUser);
    return dwError;
}
