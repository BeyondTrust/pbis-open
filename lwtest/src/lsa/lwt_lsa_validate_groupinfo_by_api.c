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
 * w
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name: lwt_lsa_validate_groupinfo_byapi.c
 *
 * compares the group information between lsa api's
 *
 */

#include "lwt_lsa_validate_groupinfo_common.h"

/*
 * ValidateGroupInfoByAPIResults 
 *  
 * Function retrieves group information using lsa api's and compares the result 
 * 
 */
static
DWORD
ValidateGroupInfoByAPIResults(
    HANDLE hLsaConnection,
    PTESTDATA pTestData    
    );

/*
 * CompareGroupInfoByUser 
 * 
 * Function compares the group information retrived from AD using user name and user id as parameters
 * 
 */
static 
DWORD
CompareGroupInfoByUser(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 * CompareGroupInfoByGroup 
 * 
 * Function compares the group information retrived from AD using group name and gid as parameters
 * 
 */
static 
DWORD
CompareGroupInfoByGroup(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 * CompareGroupInformations 
 * 
 * Function compares the group information for the input parameters
 * 
 */
static 
DWORD
CompareGroupInformations(
    PVOID* ppGroupInfoListByName,
    PVOID* ppGroupInfoListById,
    DWORD  dwGroupsByName,
    DWORD  dwGroupsById,
    DWORD  dwGroupInfoLevel,
    PCSTR  pszUserName
    );

/*
 * CompareGroupInfoLevel0 
 * 
 * Function compares the group information in level 0 
 * 
 */
static 
DWORD
CompareGroupInfoLevel0(
    PLSA_GROUP_INFO_0 pGroupInfoList_1,
    PLSA_GROUP_INFO_0 pGroupInfoList_2
    );

/*
 * CompareGroupInfoLevel1 
 * 
 * Function compares the group information in level 1
 * 
 */
static 
DWORD
CompareGroupInfoLevel1(
    PLSA_GROUP_INFO_1 pGroupInfoList_1,
    PLSA_GROUP_INFO_1 pGroupInfoList_2
    );

/*
 * ValidateEnumGroupInfoByUser 
 * 
 * Function compares the group information of user with the enumerated group information  
 * 
 */
static
DWORD
ValidateEnumGroupInfoByUser(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    DWORD dwGroupInfoLevel,
    PVOID* ppEnumGroupInfoList,
    DWORD dwGroups
    );

/*
 * ValidateEnumGroupInfoByGroup 
 * 
 * Function compares the group information of group with the enumerated group information  
 * 
 */
static
DWORD
ValidateEnumGroupInfoByGroup(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    DWORD dwGroupInfoLevel,
    PVOID* ppEnumGroupInfoList,
    DWORD dwGroups
    );

/*
 * ValidateUserAndGroupWithEnumGroupInfo 
 * 
 * Function enumerates the group information and compares the user and group's group 
 * information with enumerated group information 
 * 
 */
static
DWORD
ValidateUserAndGroupWithEnumGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 * EnumerateGroups 
 * 
 * Function enumerates and stores the group information
 * 
 */
static
DWORD
EnumerateGroups(
    HANDLE hLsaConnection,
    DWORD dwLevel,
    BOOL bOnline,
    PVOID* ppGroupInfo
    );

/*
 * EnumerateGroupCount 
 * 
 * Function enumerates the groups and get group count
 * 
 */
static
DWORD
EnumerateGroupCount(
    HANDLE hLsaConnection,
    DWORD dwLevel,
    BOOL bOnline,
    PDWORD pdwMaxGroups
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
    PLSA_GROUP_INFO_1 pGroupInfoListByName,
    PLSA_GROUP_INFO_1 pGroupInfoListById
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
    PSTR* ppszMembersByName,
    PSTR* ppszMembersById
    );

/*
 * CompareUserGroupWithEnumGroup 
 * 
 * Function compares group information agaist group information \
 * of groups in list.    
 * 
 */
static
DWORD
CompareUserGroupWithEnumGroup(
    HANDLE hLsaConnection,
    PLWTUSER pUserInfo,
    DWORD  dwGroupInfoLevel,
    PVOID* ppEnumGroupInfoList,
    DWORD  dwEnumGroups
    );

/*
 * CompareUserGroupInfo
 * 
 * Function compares user's group informations .    
 * 
 */
static 
DWORD
CompareUserGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    PLWTUSER pUserInfo
    );

/*
 * EnumeratorGroupList
 * 
 * Function enumerates group list     
 * 
 */
static
DWORD
EnumeratorGroupList( 
    HANDLE hLsaConnection,
    DWORD  dwGroupInfoLevel,
    PDWORD pdwGroups,
    PVOID* ppEnumGroupInfoList
    );

/*
 * ValidateGroupInfoWithList 
 * 
 * Function validates the group information with groups in the list  
 * 
 */
static
DWORD
ValidateGroupInfoWithList( 
    PVOID pGroupInfoListByName,
    PVOID *ppEnumGroupInfoList,
    DWORD dwEnumGroups,
    DWORD dwGroupInfoLevel,
    PSTR  pszUserName
    );

/*
 * CompareGroupWithEnumGroup 
 * 
 * Function compares group information agaist the enumerated group's 
 * information  
 * 
 */
static
DWORD
CompareGroupWithEnumGroup( 
    HANDLE hLsaConnection,
    PLWTGROUP pGroup,
    DWORD dwGroupInfoLevel,
    PVOID *ppEnumGroupInfoList,
    DWORD dwEnumGroups
    );

/*
 * ValidateGroupWithGroupList
 * 
 * Function validates group information agaist the group informations in the
 * list
 * 
 */
static
DWORD
ValidateGroupWithGroupList(
    HANDLE hLsaConnection,
    PLWTGROUP pGroup
    );

/*
 * ValidateGroupInfoLists 
 * 
 * Function compares group informations reulted from APIs 
 * 
 */
static
DWORD
ValidateGroupInfoLists( 
    PVOID pGroupInfoListByName,
    PVOID pGroupInfoListById,
    DWORD dwGroupInfoLevel
    );

/*
 * GetGroupInfoByGrpName
 * 
 * Function gets the group information 
 * 
 */
static
DWORD
GetGroupInfoByGrpName( 
    HANDLE hLsaConnection,
    PSTR   pszGroupName,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfoListByName
    );

/*
 * GetGroupInfoByGrpId
 * 
 * Function gets the group information 
 * 
 */
static
DWORD
GetGroupInfoByGrpId( 
    HANDLE hLsaConnection,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfoListById
    );

/*
 * validate_groupinfo_by_api_main()
 *
 * Verifies the group information returned from AD with the CSV file.
 *
 */
int 
validate_groupinfo_by_api_main(
    int  argc, 
    char *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = NULL;
    PTESTDATA pTestData = NULL;

    dwLocalError = Lwt_LsaTestSetup( argc,
                                     argv,
                                     &hLsaConnection,
                                     &pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = ValidateGroupInfoByAPIResults( hLsaConnection, 
                                                  pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:

    Lwt_LsaTestTeardown( &hLsaConnection, 
                         &pTestData);
    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;
}

/*
 * ValidateGroupInfoByAPIResults 
 *  
 * Function retrieves group information using lsa api's and compares the result 
 * 
 */
static
DWORD
ValidateGroupInfoByAPIResults(
    HANDLE hLsaConnection,
    PTESTDATA pTestData    
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    if ( !pTestData || !pTestData->pUserIface || !pTestData->pGroupIface )
    {
        dwLocalError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    dwLocalError = CompareGroupInfoByUser( hLsaConnection, 
                                           pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = ValidateUserAndGroupWithEnumGroupInfo( hLsaConnection, 
                                                          pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = CompareGroupInfoByGroup( hLsaConnection, 
                                            pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

/*
 * ValidateUserAndGroupWithEnumGroupInfo 
 * 
 * Function enumerates the group information and compares the user and group's group 
 * information with enumerated group information 
 * 
 */
static
DWORD
ValidateUserAndGroupWithEnumGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD   dwError = LW_ERROR_SUCCESS;
    DWORD   dwLocalError = LW_ERROR_SUCCESS;
    DWORD   dwGroupInfoLevel = 0;
    DWORD   dwGroups = 0;
    PVOID*  ppEnumGroupInfoList = NULL;

    for ( ; dwGroupInfoLevel <= MAX_GROUP_INFOLEVEL; dwGroupInfoLevel++ )
    {
        dwLocalError = EnumeratorGroupList( hLsaConnection,
                                            dwGroupInfoLevel,
                                            &dwGroups,
                                            ppEnumGroupInfoList);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = ValidateEnumGroupInfoByUser( hLsaConnection, 
                                                    pTestData,
                                                    dwGroupInfoLevel,
                                                    ppEnumGroupInfoList,
                                                    dwGroups);
        BAIL_ON_TEST_BROKE(dwLocalError);
        
        dwLocalError = ValidateEnumGroupInfoByGroup( hLsaConnection, 
                                                     pTestData,
                                                     dwGroupInfoLevel,
                                                     ppEnumGroupInfoList,
                                                     dwGroups);
        BAIL_ON_TEST_BROKE(dwLocalError);
   
        if ( ppEnumGroupInfoList )
        {
            LsaFreeGroupInfoList( dwGroupInfoLevel, 
                                  ppEnumGroupInfoList, 
                                  dwGroups);
            ppEnumGroupInfoList = NULL;
        }
    } 

    return dwError;

error:
    if ( ppEnumGroupInfoList )
    { 
        LsaFreeGroupInfoList( dwGroupInfoLevel, 
                              ppEnumGroupInfoList, 
                              dwGroups);
        ppEnumGroupInfoList = NULL;
        
    }
    return dwError;
}

/*
 * ValidateEnumGroupInfoByUser 
 * 
 * Function compares the group information of user with the enumerated group information  
 * 
 */
static
DWORD
ValidateEnumGroupInfoByUser(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    DWORD dwGroupInfoLevel,
    PVOID* ppEnumGroupInfoList,
    DWORD dwEnumGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwUser = 0;
    PLWTUSER pUserInfo = NULL;

    for ( dwUser = 0 ; dwUser < pTestData->dwNumUsers; dwUser++ )
    {
        dwLocalError = GetUser( pTestData,
                                dwUser,
                                &pUserInfo);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( pUserInfo && !IsNullOrEmpty(pUserInfo->pszNTName) )
        {
            dwLocalError = CompareUserGroupWithEnumGroup( hLsaConnection,
                                                          pUserInfo,
                                                          dwGroupInfoLevel,
                                                          ppEnumGroupInfoList,
                                                          dwEnumGroups);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        FreeUser(&pUserInfo);
    }
    
    return dwError;

error:
    FreeUser(&pUserInfo);
    return dwError;
}

/*
 * CompareUserGroupWithEnumGroup 
 * 
 * Function compares group information agaist group information \
 * of groups in list.    
 * 
 */
static
DWORD
CompareUserGroupWithEnumGroup(
    HANDLE hLsaConnection,
    PLWTUSER pUserInfo,
    DWORD  dwGroupInfoLevel,
    PVOID* ppEnumGroupInfoList,
    DWORD  dwEnumGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwGroup = 0;
    DWORD dwNumGroupsByName = 0;
    PVOID *ppGroupInfoListByName = NULL;

    dwLocalError = GetGroupsForUserByName( hLsaConnection,
                                           pUserInfo->pszNTName,
                                           dwGroupInfoLevel,
                                           &ppGroupInfoListByName,
                                           &dwNumGroupsByName);
    BAIL_ON_TEST_BROKE(dwLocalError);

    for ( dwGroup = 0; dwGroup < dwNumGroupsByName; dwGroup++ )
    {
        if ( ppGroupInfoListByName[dwGroup] )
        {
            dwLocalError = ValidateGroupInfoWithList( ppGroupInfoListByName[dwGroup],
                                                      ppEnumGroupInfoList,
                                                      dwEnumGroups,
                                                      dwGroupInfoLevel,
                                                      pUserInfo->pszNTName);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }
    }

error:
    if ( ppGroupInfoListByName )
    {
        LsaFreeGroupInfoList( dwGroupInfoLevel,
                              ppGroupInfoListByName,
                              dwNumGroupsByName);
        ppGroupInfoListByName = NULL;
    }

    return dwError;
}

/*
 * ValidateEnumGroupInfoByGroup 
 * 
 * Function compares the group information of group with the enumerated group information  
 * 
 */
static
DWORD
ValidateEnumGroupInfoByGroup(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    DWORD dwGroupInfoLevel,
    PVOID *ppEnumGroupInfoList,
    DWORD dwEnumGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwGroup = 0;
    PLWTGROUP pGroup = NULL;

    for ( dwGroup = 0; dwGroup < pTestData->dwNumGroups; dwGroup++ )
    {
        dwLocalError = GetGroup( pTestData,
                                 dwGroup,
                                 &pGroup);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( pGroup && !IsNullOrEmpty(pGroup->pszName) )
        {
            CompareGroupWithEnumGroup( hLsaConnection,
                                       pGroup,
                                       dwGroupInfoLevel,
                                       ppEnumGroupInfoList,
                                       dwEnumGroups);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        FreeGroup(&pGroup);
    }

    return dwError;

error:
    FreeGroup(&pGroup);
    return dwError;
}

/*
 * CompareGroupInfoByUser 
 * 
 * Function compares the group information retrived from AD 
 * using user name and user id as parameters
 * 
 */
static 
DWORD
CompareGroupInfoByUser(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwUser = 0;
    PLWTUSER pUserInfo = NULL;

    for ( dwUser = 0 ; dwUser < pTestData->dwNumUsers; dwUser++ )
    {
        dwLocalError = GetUser( pTestData,
                                dwUser,
                                &pUserInfo);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( pUserInfo && !IsNullOrEmpty(pUserInfo->pszNTName) )
        {
            dwLocalError = CompareUserGroupInfo( hLsaConnection,
                                                 pTestData,
                                                 pUserInfo);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        FreeUser(&pUserInfo);
    }

    return dwError;

error:
    FreeUser(&pUserInfo);
    return dwError;
}

/*
 * ValidateGroupMembers
 * 
 * Function compares user's group informations .    
 * 
 */
static 
DWORD
CompareUserGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    PLWTUSER pUserInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwNumGroupsById = 0;
    DWORD dwGroupInfoLevel = 0;
    DWORD dwNumGroupsByName = 0;
    PVOID *ppGroupInfoListById = NULL;
    PVOID *ppGroupInfoListByName = NULL;

    for ( ; dwGroupInfoLevel <= MAX_GROUP_INFOLEVEL; dwGroupInfoLevel++ )
    {
        dwLocalError = GetGroupsForUserByName( hLsaConnection,
                                               pUserInfo->pszNTName,
                                               dwGroupInfoLevel,
                                               &ppGroupInfoListByName,
                                               &dwNumGroupsByName);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = GetGroupsForUserById( hLsaConnection,
                                             pUserInfo->nUnixUid,
                                             dwGroupInfoLevel,
                                             &ppGroupInfoListById,
                                             &dwNumGroupsById);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = CompareGroupInformations( ppGroupInfoListByName, 
                                                 ppGroupInfoListById, 
                                                 dwNumGroupsByName, 
                                                 dwNumGroupsById, 
                                                 dwGroupInfoLevel,
                                                 pUserInfo->pszNTName);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( ppGroupInfoListByName )
        {
            LsaFreeGroupInfoList( dwGroupInfoLevel,
                                  ppGroupInfoListByName,
                                  dwNumGroupsByName);
            ppGroupInfoListByName = NULL;
        }

        if ( ppGroupInfoListById )
        {
            LsaFreeGroupInfoList( dwGroupInfoLevel,
                                  ppGroupInfoListById,
                                  dwNumGroupsById);
            ppGroupInfoListById = NULL;
        }
    }

    return dwError;

error:
    if ( ppGroupInfoListByName )
    {
        LsaFreeGroupInfoList( dwGroupInfoLevel,
                              ppGroupInfoListByName,
                              dwNumGroupsByName);
        ppGroupInfoListByName = NULL;
    }

    if ( ppGroupInfoListById )
    {
        LsaFreeGroupInfoList( dwGroupInfoLevel,
                              ppGroupInfoListById,
                              dwNumGroupsById);
        ppGroupInfoListById = NULL;
    }

    return dwError;
}

/*
 * CompareGroupInformations 
 * 
 * Function compares the group information for the input parameters
 * 
 */
static 
DWORD
CompareGroupInformations(
    PVOID *ppGroupInfoListByName,
    PVOID *ppGroupInfoListById,
    DWORD dwGroupsByName,
    DWORD dwGroupsById,
    DWORD dwGroupInfoLevel,
    PCSTR pszUserName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwGroup = 0;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName, LsaGetGroupsForUserById";
    PCSTR pszTestDescription = 
        "Comparing group information resulted from LsaGetGroupsForUserByName()\
        and LsaGetGroupsForUserById() APIs";

    if ( dwGroupsByName != dwGroupsById )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg),
                  "Group count mismatching for user '%s'. \
                  'LsaGetGroupsForUserByName()' returns (%d) where \
                  'LsaGetGroupsForUserById()' returns (%d)", 
                  pszUserName,
                  dwGroupsByName,
                  dwGroupsById);
        LWT_LOG_TEST(szTestMsg);
        goto error;
    }

    while ( dwGroup < dwGroupsByName && ppGroupInfoListByName[dwGroup] )
    {
        dwLocalError = ValidateGroupInfoWithList( ppGroupInfoListById,
                                                  ppGroupInfoListByName[dwGroup],
                                                  dwGroupsById,
                                                  dwGroupInfoLevel,
                                                  (PSTR)pszUserName);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwGroup++;
    }

error:
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
    PSTR* ppszMembersByName,
    PSTR* ppszMembersById
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwMember = 0;
    CHAR  szMembersById[256] = { 0 };
    CHAR  szMembersByName[256] = { 0 };
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName, LsaGetGroupsForUserById";
    PCSTR pszTestDescription = 
        "Validating group info level 1 group members resulted by APIs";
    BOOLEAN bEqual = FALSE;

    dwLocalError = CompareMemberList( ppszMembersByName, 
                                      ppszMembersById,
                                      &bEqual);
    if ( bEqual )
    {
        dwLocalError = CompareMemberList( ppszMembersById, 
                                          ppszMembersByName,
                                          &bEqual);
    }

    if ( !bEqual )
    {
        while ( ppszMembersById[dwMember] )
        {
            strcat(szMembersById, ppszMembersById[dwMember]);
            strcat(szMembersById, ", ");
            dwMember++;
        }

        dwMember = 0;
        while ( !IsNullOrEmpty(ppszMembersByName[dwMember]) )
        {
            strcat(szMembersByName, ppszMembersByName[dwMember]);
            strcat(szMembersByName, ", ");
            dwMember++;
        }

        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group members are mismatched. LsaGetGroupsForUserByName() \
                  returned:'%s' and LsaGetGroupsForUserById returns:'%s'\n",
                  szMembersByName,
                  szMembersById);
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
    PLSA_GROUP_INFO_1 pGroupInfoListByName,
    PLSA_GROUP_INFO_1 pGroupInfoListById
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwMember = 0;
    CHAR  szMembers[256] = { 0 };
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName, LsaGetGroupsForUserById";
    PCSTR pszTestDescription = 
        "Validating level 1 group information resulted by APIs";

    if ( pGroupInfoListByName->ppszMembers )
    {
        snprintf( szMembers, 
                  sizeof(szMembers), 
                  "LsaGetGroupsForUserById() returned zero group members \
                  where LsaGetGroupsForUserByName() returned members:");

        while ( pGroupInfoListByName->ppszMembers[dwMember] )
        {
            strcat(szMembers, pGroupInfoListByName->ppszMembers[dwMember]);
            strcat(szMembers, ", ");
            dwMember++;
        }
    }
    else if ( pGroupInfoListById->ppszMembers )
    {
        snprintf( szMembers, 
                  sizeof(szMembers), 
                  "LsaGetGroupsForUserByName() returned zero group members \
                  where LsaGetGroupsForUserById() returned members:");

        while ( pGroupInfoListById->ppszMembers[dwMember] )
        {
            strcat(szMembers, pGroupInfoListById->ppszMembers[dwMember]);
            strcat(szMembers, ", ");
            dwMember++;
        }
    }

    dwError = LW_ERROR_TEST_FAILED;
    snprintf( szTestMsg, 
              sizeof(szTestMsg), 
              "Group members resulted by the APIs are mismatched for \
              group:'%s'. %s",
              pGroupInfoListByName->pszName,
              szMembers);
    LWT_LOG_TEST(szTestMsg);

    return dwError;
}

/*
 * CompareGroupInfoByGroup 
 * 
 * Function compares the group information retrived from AD using group name and gid as parameters
 * 
 */
static 
DWORD
CompareGroupInfoByGroup(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwGroup = 0;
    PLWTGROUP pGroup = NULL;

    for ( dwGroup = 0; dwGroup < pTestData->dwNumGroups; dwGroup++ )
    {
        dwLocalError = GetGroup( pTestData, 
                                 dwGroup, 
                                 &pGroup);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( pGroup && !IsNullOrEmpty(pGroup->pszName) )
        {
            dwLocalError = ValidateGroupWithGroupList( hLsaConnection,
                                                       pGroup);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        FreeGroup(&pGroup);
    }

error:
    FreeGroup(&pGroup);
    return dwError;
}

/*
 * EnumerateGroups 
 * 
 * Function enumerates and stores the group information
 * 
 */
static
DWORD
EnumerateGroups(
    HANDLE  hLsaConnection,
    DWORD   dwLevel,
    BOOL    bOnline,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PVOID *ppTempInfoList = NULL;
    DWORD dwMaxGrpCount = 0;
    DWORD dwBatchLimit = 10;
    DWORD dwGroup = 0;
    DWORD dwMaxGroups = 0;
    CHAR  szTestMsg[256] = { 0 };
    PCSTR pszTestDescription = "Enumerating groups for group information";
    PCSTR pszTestAPIs =  "LsaBeginEnumGroupsWithCheckOnlineOption(), \
                          LsaEnumGroups(), LsaEndEnumGroups()";
    HANDLE hResume = NULL;
    LSA_FIND_FLAGS FindFlags = 0;

    dwLocalError = LsaBeginEnumGroupsWithCheckOnlineOption( hLsaConnection,
                                                            dwLevel,
                                                            dwBatchLimit,
                                                            bOnline,
                                                            FindFlags,
                                                            &hResume);
    if ( LW_ERROR_SUCCESS != dwLocalError )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "LsaBeginEnumGroupsWithCheckOnlineOption() API returned \
                  unexpected error code (%lu)",
                  (unsigned long)dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }

    BAIL_ON_TEST_BROKE(dwLocalError);

    do
    {
        if ( ppTempInfoList )
        {
            LwFreeMemory(ppTempInfoList);
            ppTempInfoList = NULL;
        }

        dwLocalError = LsaEnumGroups( hLsaConnection,
                                      hResume,
                                      &dwMaxGrpCount,
                                      &ppTempInfoList);

        if ( LW_ERROR_SUCCESS != dwLocalError )
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf( szTestMsg, 
                      sizeof(szTestMsg), 
                      "LsaEnumGroups() returned unexpected error code (%lu)",
                      (unsigned long)dwLocalError);
            LWT_LOG_TEST(szTestMsg);
        }
        BAIL_ON_TEST_BROKE(dwLocalError);
    
        if ( !dwMaxGrpCount )
        {
            break;
        }

        for ( dwGroup = 0; dwGroup < dwMaxGrpCount; dwGroup++ )
        {
            ppGroupInfo[dwMaxGroups + dwGroup]= ppTempInfoList[dwGroup];
        }
        dwMaxGroups += dwMaxGrpCount;

    } while ( dwMaxGrpCount );

cleanup:
    if ( ppTempInfoList )
    {
        LwFreeMemory(ppTempInfoList);
    }

    if ( hLsaConnection && hResume )
    {
        LsaEndEnumGroups( hLsaConnection, 
                          hResume);
        hResume = (HANDLE)NULL;
    }

    LWT_LOG_TEST(szTestMsg);
    return (dwError);

error:
    goto cleanup;
}

/*
 * EnumerateGroupCount 
 * 
 * Function enumerates the groups and get group count
 * 
 */
static
DWORD
EnumerateGroupCount(
    HANDLE  hLsaConnection,
    DWORD   dwLevel,
    BOOL    bOnline,
    PDWORD  pdwMaxGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwMaxGrpCount = 0;
    DWORD dwBatchLimit = 10;
    DWORD dwGroupCount = 0;
    PVOID *ppTempInfoList = NULL;
    HANDLE hResume = (HANDLE)NULL;
    LSA_FIND_FLAGS FindFlags = 0;
    CHAR  szTestMsg[256] = { 0 };
    PCSTR pszTestDescription = "Enumerating groups for group information";
    PCSTR pszTestAPIs = "LsaBeginEnumGroupsWithCheckOnlineOption(), \
                         LsaEnumGroups(), LsaEndEnumGroups()";

    dwLocalError = LsaBeginEnumGroupsWithCheckOnlineOption( hLsaConnection,
                                                            dwLevel,
                                                            dwBatchLimit,
                                                            bOnline,
                                                            FindFlags,
                                                            &hResume);
    if ( LW_ERROR_SUCCESS != dwLocalError )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "LsaBeginEnumGroupsWithCheckOnlineOption() API returned \
                  unexpected error code (%lu)",
                  (unsigned long)dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }

    BAIL_ON_TEST_BROKE(dwLocalError);

    do
    {
        if ( ppTempInfoList ) 
        {
           LsaFreeGroupInfoList( dwLevel, 
                                 ppTempInfoList, 
                                 dwMaxGrpCount);
           ppTempInfoList = NULL;
        } 

        dwLocalError = LsaEnumGroups( hLsaConnection,
                                      hResume,
                                      &dwMaxGrpCount,
                                      &ppTempInfoList);

        if ( LW_ERROR_SUCCESS != dwLocalError )
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf( szTestMsg, 
                      sizeof(szTestMsg), 
                      "LsaEnumGroups() returned unexpected error code (%lu)",
                      (unsigned long)dwLocalError);
            LWT_LOG_TEST(szTestMsg);
        }
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwGroupCount += dwMaxGrpCount;

    } while ( dwMaxGrpCount );

cleanup:
    if ( ppTempInfoList ) 
    {
        LsaFreeGroupInfoList( dwLevel, 
                              ppTempInfoList, 
                              dwMaxGrpCount);
    }
 
    if ( hResume != NULL && hLsaConnection != NULL ) 
    {
        LsaEndEnumGroups( hLsaConnection, 
                          hResume);
        hResume = (HANDLE)NULL;
    }

    *pdwMaxGroups = dwGroupCount;
    LWT_LOG_TEST(szTestMsg);
    return (dwError);

error:
    dwGroupCount = 0;
    goto cleanup;
}

/*
 * EnumeratorGroupList
 * 
 * Function enumerates group list     
 * 
 */
static
DWORD
EnumeratorGroupList( 
    HANDLE hLsaConnection,
    DWORD  dwGroupInfoLevel,
    PDWORD pdwGroups,
    PVOID* ppEnumGroupInfoList
    )
{
    DWORD  dwError = LW_ERROR_SUCCESS;
    DWORD  dwLocalError = LW_ERROR_SUCCESS;
    DWORD  dwGroups = 0;
    BOOL   bOnline = TRUE;

    dwLocalError = EnumerateGroupCount( hLsaConnection,
                                        dwGroupInfoLevel,
                                        bOnline,
                                        &dwGroups);
    BAIL_ON_TEST_BROKE(dwLocalError);

    if ( dwGroups > 0 )
    {
        dwLocalError = LwAllocateMemory( dwGroups * sizeof(PVOID),
                                         (PVOID)&ppEnumGroupInfoList);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = EnumerateGroups( hLsaConnection,
                                        dwGroupInfoLevel,
                                        bOnline,
                                        ppEnumGroupInfoList);
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

cleanup:
    *pdwGroups = dwGroups;
    return dwError;

error:
    dwGroups = 0;
    goto cleanup;
}

/*
 * ValidateGroupInfoWithList 
 * 
 * Function validates the group information with groups in the list  
 * 
 */
static
DWORD
ValidateGroupInfoWithList( 
    PVOID pGroupInfoListByName,
    PVOID *ppEnumGroupInfoList,
    DWORD dwEnumGroups,
    DWORD dwGroupInfoLevel,
    PSTR  pszUserName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwEnumGroup = 0;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName()";
    PCSTR pszTestDescription = 
        "Validating user group information with group information resulted \
        from enumeration";

    for ( dwEnumGroup = 0; dwEnumGroup < dwEnumGroups; dwEnumGroup++ )
    {
        if ( StringsNoCaseAreEqual( ((PLSA_GROUP_INFO_0)pGroupInfoListByName)->pszName, 
                                    ((PLSA_GROUP_INFO_0)ppEnumGroupInfoList[dwEnumGroup])->pszName) )
        {
            dwLocalError = CompareGroupInfoLevel0( (PLSA_GROUP_INFO_0)pGroupInfoListByName,
                                                   (PLSA_GROUP_INFO_0)ppEnumGroupInfoList[dwEnumGroup]);
            BAIL_ON_TEST_BROKE(dwLocalError);

            if ( dwGroupInfoLevel )
            {
                dwLocalError = CompareGroupInfoLevel1( (PLSA_GROUP_INFO_1)pGroupInfoListByName,
                                                       (PLSA_GROUP_INFO_1)ppEnumGroupInfoList[dwEnumGroup]);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
            break;
        }
    }

    if ( dwEnumGroup == dwEnumGroups )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Group '%s' is missing in group list of user:'%s'\n", 
                  ((PLSA_GROUP_INFO_0)pGroupInfoListByName)->pszName,
                  pszUserName);
        LWT_LOG_TEST(szTestMsg);
    }

error:
    return dwError;
}

/*
 * CompareGroupWithEnumGroup 
 * 
 * Function compares group information agaist the enumerated group's 
 * information  
 * 
 */
static
DWORD
CompareGroupWithEnumGroup( 
    HANDLE hLsaConnection,
    PLWTGROUP pGroup,
    DWORD dwGroupInfoLevel,
    PVOID *ppEnumGroupInfoList,
    DWORD dwEnumGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaFindGroupByName";
    PVOID pGroupInfoListByName = NULL;
    PCSTR pszTestDescription = "Validating group information with group \
    information resulted from enumeration";
    LSA_FIND_FLAGS FindFlags = 0;

    dwLocalError = LsaFindGroupByName( hLsaConnection,
                                       pGroup->pszName,
                                       FindFlags,
                                       dwGroupInfoLevel,
                                       &pGroupInfoListByName);

    if ( LW_ERROR_SUCCESS != dwLocalError )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned unexpected error code (%lu)",
                  (unsigned long)dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = ValidateGroupInfoWithList( pGroupInfoListByName,
                                              ppEnumGroupInfoList,
                                              dwEnumGroups,
                                              dwGroupInfoLevel,
                                              pGroup->pszName);
    BAIL_ON_TEST_BROKE(dwLocalError);

error:
    if ( pGroupInfoListByName )
    {
        LsaFreeGroupInfo( dwGroupInfoLevel, 
                          pGroupInfoListByName);
        pGroupInfoListByName = NULL;
    }

    return dwError;
}

/*
 * ValidateGroupWithGroupList
 * 
 * Function validates group information agaist the group informations in the
 * list
 * 
 */
static
DWORD
ValidateGroupWithGroupList(
    HANDLE hLsaConnection,
    PLWTGROUP pGroup
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PVOID pGroupInfoListByName = NULL;
    PVOID pGroupInfoListById = NULL;
    DWORD dwGroupInfoLevel = 0;

    for ( ; dwGroupInfoLevel <= MAX_GROUP_INFOLEVEL; dwGroupInfoLevel++ )
    {
        dwLocalError = GetGroupInfoByGrpName( hLsaConnection,
                                              pGroup->pszName,
                                              dwGroupInfoLevel,
                                              &pGroupInfoListByName);
        BAIL_ON_TEST_BROKE(dwLocalError);
                                              
        dwLocalError = GetGroupInfoByGrpId( hLsaConnection,
                                            pGroup->nGid,
                                            dwGroupInfoLevel,
                                            &pGroupInfoListById);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = ValidateGroupInfoLists( pGroupInfoListByName,
                                               pGroupInfoListById,
                                               dwGroupInfoLevel);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( pGroupInfoListByName )
        {
            LsaFreeGroupInfo( dwGroupInfoLevel, 
                              pGroupInfoListByName);
            pGroupInfoListByName = NULL;
        }

        if ( pGroupInfoListById )
        {
            LsaFreeGroupInfo( dwGroupInfoLevel, 
                              pGroupInfoListById);
            pGroupInfoListById = NULL;
        }
    }

    return dwError;

error:
    if ( pGroupInfoListByName )
    {
        LsaFreeGroupInfo( dwGroupInfoLevel, 
                          pGroupInfoListByName);
        pGroupInfoListByName = NULL;
    }

    if ( pGroupInfoListById )
    {
        LsaFreeGroupInfo( dwGroupInfoLevel, 
                          pGroupInfoListById);
        pGroupInfoListById = NULL;
    }

    return dwError;
}

/*
 * ValidateGroupInfoLists 
 * 
 * Function compares group informations reulted from APIs 
 * 
 */
static
DWORD
ValidateGroupInfoLists( 
    PVOID pGroupInfoListByName,
    PVOID pGroupInfoListById,
    DWORD dwGroupInfoLevel
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    if ( pGroupInfoListByName && pGroupInfoListById )
    {
        dwLocalError = CompareGroupInfoLevel0( pGroupInfoListByName, 
                                               pGroupInfoListById);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( dwGroupInfoLevel )
        {
            dwLocalError = CompareGroupInfoLevel1( pGroupInfoListByName, 
                                                   pGroupInfoListById);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }
    }

error:
    return dwError;
}

/*
 * CompareGroupInfoLevel0 
 * 
 * Function compares the group information in level 0 
 * 
 */
static 
DWORD
CompareGroupInfoLevel0(
    PLSA_GROUP_INFO_0 pGroupInfoList_1,
    PLSA_GROUP_INFO_0 pGroupInfoList_2
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName, LsaGetGroupsForUserById";
    PCSTR pszTestDescription = 
        "Comparing level0 group information resulted from \
        LsaGetGroupsForUserByName and LsaGetGroupsForUserById APIs";

    if ( !StringsNoCaseAreEqual( pGroupInfoList_1->pszName, 
                                 pGroupInfoList_2->pszName) ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg,
                  sizeof(szTestMsg), 
                  "Group name mismatch. Group name from api \
                  'LsaGetGroupsForUserByName' is '%s' where \
                  'LsaGetGroupsForUserById' returns '%s'\n", 
                  pGroupInfoList_1->pszName,
                  pGroupInfoList_2->pszName);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( pGroupInfoList_1->gid != pGroupInfoList_2->gid )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg,
                  sizeof(szTestMsg), 
                  "Group gid mismatch for group '%s'. Api \
                  'LsaGetGroupsForUserByName' gives (%lu) where \
                  'LsaGetGroupsForUserById' returns (%lu)\n", 
                  pGroupInfoList_1->pszName,
                  (unsigned long) pGroupInfoList_1->gid,
                  (unsigned long) pGroupInfoList_2->gid);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( !StringsAreEqual( pGroupInfoList_1->pszSid, 
                           pGroupInfoList_2->pszSid) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg,
                  sizeof(szTestMsg), 
                  "Group Sid mismatch for group '%s'. Api \
                  'LsaGetGroupsForUserByName' gives (%s) where \
                  'LsaGetGroupsForUserById' returns (%s)\n", 
                  pGroupInfoList_1->pszName,
                  pGroupInfoList_1->pszSid,
                  pGroupInfoList_2->pszSid);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 * CompareGroupInfoLevel1 
 * 
 * Function compares the group information in level 1
 * 
 */
static 
DWORD
CompareGroupInfoLevel1(
    PLSA_GROUP_INFO_1 pGroupInfoList_1,
    PLSA_GROUP_INFO_1 pGroupInfoList_2
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName, LsaGetGroupsForUserById";
    PCSTR pszTestDescription = 
        "Comparing level1 group information resulted from \
        LsaGetGroupsForUserByName and LsaGetGroupsForUserById APIs";

    if ( (!pGroupInfoList_1->ppszMembers && pGroupInfoList_2->ppszMembers) || 
         (pGroupInfoList_1->ppszMembers && !pGroupInfoList_2->ppszMembers) )
    {
        dwLocalError = LogInvalidGroupMembers( pGroupInfoList_1,
                                               pGroupInfoList_2); 
        BAIL_ON_TEST_BROKE(dwLocalError);
    }
    else if ( pGroupInfoList_1->ppszMembers && pGroupInfoList_2->ppszMembers )
    {
        dwLocalError = ValidateGroupMembers( pGroupInfoList_1->ppszMembers,
                                             pGroupInfoList_2->ppszMembers);  
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    if ( !StringsAreEqual( pGroupInfoList_1->pszDN, 
                           pGroupInfoList_2->pszDN) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg,
                  sizeof(szTestMsg), 
                  "Group DN mismatch. Group DN from api \
                  'LsaGetGroupsForUserByName' is '%s' where \
                  'LsaGetGroupsForUserById' returns '%s'\n", 
                  pGroupInfoList_1->pszDN,
                  pGroupInfoList_2->pszDN);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( !StringsAreEqual( pGroupInfoList_1->pszPasswd, 
                           pGroupInfoList_2->pszPasswd) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg,
                  sizeof(szTestMsg), 
                  "Group password mismatch. Group password from api \
                  'LsaGetGroupsForUserByName' is '%s' where \
                  'LsaGetGroupsForUserById' returns '%s'\n", 
                  pGroupInfoList_1->pszPasswd,
                  pGroupInfoList_2->pszPasswd);
        LWT_LOG_TEST(szTestMsg);
    }

error:
    return dwError;
}

/*
 * GetGroupInfoByGrpName
 * 
 * Function gets the group information 
 * 
 */
static
DWORD
GetGroupInfoByGrpName( 
    HANDLE hLsaConnection,
    PSTR   pszGroupName,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfoListByName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    LSA_FIND_FLAGS FindFlags = 0;
    CHAR  szTestMsg[256] = { 0 };
    PCSTR pszTestAPIs = "LsaFindGroupByName(), LsaFindGroupById()";
    PCSTR pszTestDescription = 
        "Comparing the group information from API's LsaFindGroupByName(), \
        LsaFindGroupById()";

    dwLocalError = LsaFindGroupByName( hLsaConnection,
                                       pszGroupName,
                                       FindFlags,
                                       dwGroupInfoLevel,
                                       ppGroupInfoListByName);

    if ( LW_ERROR_SUCCESS != dwLocalError )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned unexpected error code (%lu) for group '%s'\n",
                  (unsigned long)dwLocalError,
                  pszGroupName);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 * GetGroupInfoByGrpId
 * 
 * Function gets the group information 
 * 
 */
static
DWORD
GetGroupInfoByGrpId( 
    HANDLE hLsaConnection,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfoListById
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    LSA_FIND_FLAGS FindFlags = 0;
    CHAR  szTestMsg[256] = { 0 };
    PCSTR pszTestAPIs = "LsaFindGroupByName(), LsaFindGroupById()";
    PCSTR pszTestDescription = 
        "Comparing the group information from API's LsaFindGroupByName(), \
        LsaFindGroupById()";

    dwLocalError = LsaFindGroupById( hLsaConnection,
                                     gid,
                                     FindFlags,
                                     dwGroupInfoLevel,
                                     ppGroupInfoListById);

    if ( LW_ERROR_SUCCESS != dwLocalError )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned unexpected error code (%lu) for Gid (%lu)",
                  (unsigned long)dwLocalError,
                  (unsigned long)gid);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}
