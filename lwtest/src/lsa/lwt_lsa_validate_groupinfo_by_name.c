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
 * Module Name: lwt_lsa_validate_groupinfo_byname.c
 *
 * Verifies the group information returned from AD with the CSV file.
 *
 */

#include "lwt_lsa_validate_groupinfo_common.h"

/*
 * ValidateGroupInfoByUserName
 * 
 * Function gets and validates the group information for users in userlist structure 
 * 
 */
static
DWORD
ValidateGroupInfoByUserName(
    HANDLE hLsaConnection,
    PTESTDATA pTestData    
    );

/*
 * GetUserName 
 * 
 * Function gets user name from the CSV file 
 * 
 */
static
DWORD
GetUserName( 
    PLWTUSER pUserInfoFromAD,
    DWORD dwUser,
    PSTR *ppszUserName
    );

/*
 * ValidateUserGroupInfo
 * 
 * Function gets and validates the group information for a user 
 * 
 */
static
DWORD 
ValidateUserGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    PCSTR pszUserName
    );

/*
 * ProcessUserGroupInfo 
 * 
 * Function processes the group information for a user 
 * 
 */
static
DWORD
ProcessUserGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    PLWTUSER pUserInfoFromAD
    );

/*
 * ValidateGroupInfoByUser
 * 
 * Function processes group information of level0 and level 1 
 * 
 */
static
DWORD
ValidateGroupInfoByUser( 
    PTESTDATA pTestData,
    DWORD dwGroupInfoLevel,
    DWORD dwNumGroups,
    PVOID *ppGroupInfoList,           
    PSTR pszLogInfo
    );

/*
 * ValidateForNullParams
 *
 * Function validates the API for NULL function parameters.
 *
 */
static
DWORD
ValidateForNullParams(
    HANDLE hLsaConnection
    );

/*
 * ValidateNullParams
 *
 * Function validates the API for NULL function parameters.
 *
 */
static
DWORD
ValidateNullParams(
    HANDLE hLsaConnection,
    PCSTR pszUserName,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PDWORD pdwGroupsFound,
    PVOID* ppGroupInfoList,
    DWORD  dwExpectedErrorCode,
    PSTR   pszNullParam
    );

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

/*
 * ValidateInvalidParams
 *
 * Function validates the API for NULL function parameters.
 *
 */
static
DWORD
ValidateInvalidParams(
    HANDLE hLsaConnection,
    PCSTR pszUserName,
    LSA_FIND_FLAGS FindFlags,
    DWORD dwGroupInfoLevel,
    PDWORD pdwGroupsFound,
    PVOID* ppGroupInfoList,
    DWORD dwExpectedErrorCode,
    PSTR pszInvalidParam
    );

/*
 * validate_groupinfo_by_name_main() 
 *
 * Verifies the group information returned from AD with the CSV file.
 *
 */
int 
validate_groupinfo_by_name_main(
    int  argc, 
    char *argv[]
    )
{
    DWORD  dwError = LW_ERROR_SUCCESS;
    DWORD  dwLocalError = LW_ERROR_SUCCESS;
    PTESTDATA pTestData = NULL;
    HANDLE hLsaConnection = (HANDLE)NULL;

    dwLocalError = Lwt_LsaTestSetup( argc,
                                     argv,
                                     &hLsaConnection,
                                     &pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = ValidateGroupInfoByUserName( hLsaConnection, 
                                                pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = ValidateForInvalidParams( hLsaConnection,
                                             pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);
    
    dwLocalError = ValidateForNullParams(hLsaConnection);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:
    Lwt_LsaTestTeardown( &hLsaConnection, 
                         &pTestData);

    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;
}

/*
 * ValidateGroupInfoByUserName
 * 
 * Function gets and validates the group information for users in 
 * userlist structure 
 * 
 */
static
DWORD
ValidateGroupInfoByUserName(
    HANDLE hLsaConnection,
    PTESTDATA pTestData    
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwUserCnt = 0;
    PLWTUSER pUserInfoFromAD = NULL;

    if ( !pTestData || !pTestData->pUserIface || !pTestData->pGroupIface )
    {
        dwLocalError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    for ( dwUserCnt = 0; dwUserCnt < pTestData->dwNumUsers; dwUserCnt++ )
    {
        dwLocalError = GetUser( pTestData,
                                dwUserCnt,
                                &pUserInfoFromAD);
        BAIL_ON_TEST_BROKE(dwLocalError);
    
        if ( pUserInfoFromAD )
        {
            dwLocalError = ProcessUserGroupInfo( hLsaConnection, 
                                                 pTestData, 
                                                 pUserInfoFromAD);
            BAIL_ON_TEST_BROKE(dwLocalError); 
        }

        FreeUser(&pUserInfoFromAD);
    }

cleanup:
    return dwError;

error:
    FreeUser(&pUserInfoFromAD);
    goto cleanup;
}

/*
 * ProcessUserGroupInfo 
 * 
 * Function processes the group information for a user 
 * 
 */
static
DWORD
ProcessUserGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    PLWTUSER pUserInfoFromAD
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwUser = 0;
    DWORD dwMaxUser = 4; //SamAccountName, UserPrincipalName, name, Alias
    PSTR  pszUserName = NULL;

    for ( dwUser = 0; dwUser < dwMaxUser; dwUser++ )
    {
        dwLocalError = GetUserName( pUserInfoFromAD,
                                    dwUser,
                                    &pszUserName);

        if ( LW_ERROR_SUCCESS == dwLocalError )
        {
            dwLocalError = ValidateUserGroupInfo( hLsaConnection, 
                                                  pTestData, 
                                                  pszUserName);
            BAIL_ON_TEST_BROKE(dwLocalError); 
        }
    }

error:
    return dwError;
}
    

/*
 * ValidateUserGroupInfo
 * 
 * Function gets and validates the group information for a user 
 * 
 */
static
DWORD 
ValidateUserGroupInfo(
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
    CHAR  pszLogInfo[32] = {0};
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName";
    PLWTGROUP pGroupList = NULL;
    PLSA_GROUP_INFO_0 *ppListGrpInfoLevel0 = NULL;
    PLSA_GROUP_INFO_1 *ppListGrpInfoLevel1 = NULL;

    sprintf(pszLogInfo, "User '%s'", pszUserName);

    for ( ; dwGroupInfoLevel <= MAX_GROUP_INFOLEVEL; dwGroupInfoLevel++ )
    {
        dwLocalError = GetGroupsForUserByName( hLsaConnection,
                                               pszUserName,
                                               dwGroupInfoLevel,
                                               &ppGroupInfoList,
                                               &dwNumGroups);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = ValidateGroupInfoByUser( pTestData,
                                                dwGroupInfoLevel,
                                                dwNumGroups,
                                                ppGroupInfoList,
                                                pszLogInfo);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( !dwGroupInfoLevel )
        {
            ppListGrpInfoLevel0 = (PLSA_GROUP_INFO_0*)ppGroupInfoList;
            dwGrpCountInLevel0  = dwNumGroups;
        }
        else 
        {
            ppListGrpInfoLevel1 = (PLSA_GROUP_INFO_1*)ppGroupInfoList;
            dwGrpCountInLevel1  = dwNumGroups;
        }
    }

    if ( ppListGrpInfoLevel0 && ppListGrpInfoLevel1 )
    {
        dwLocalError = CompareGroupInfoLevels( ppListGrpInfoLevel0, 
                                               dwGrpCountInLevel0, 
                                               ppListGrpInfoLevel1, 
                                               dwGrpCountInLevel1,
                                               pszLogInfo,
                                               pszTestAPIs);
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

cleanup:
    if ( ppListGrpInfoLevel0 )
    {
        LsaFreeGroupInfoList( 0, 
                              (PVOID*)ppListGrpInfoLevel0, 
                              dwGrpCountInLevel0);
        ppListGrpInfoLevel0 = NULL;
    }

    if ( ppListGrpInfoLevel1 )
    {
        LsaFreeGroupInfoList( 1, 
                              (PVOID*)ppListGrpInfoLevel1, 
                              dwGrpCountInLevel1);            
        ppListGrpInfoLevel1 = NULL;
    }

    return dwError;

error:
    FreeGroup(&pGroupList);
    goto cleanup;
}

/*
 * ValidateForNullParams
 *
 * Function validates the API for NULL function parameters.
 *
 */
static
DWORD 
ValidateForNullParams(
    HANDLE hLsaConnection
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwNumGroups = 0;
    DWORD dwGroupInfoLevel = 0;
    PSTR  pszUserName = NULL;
    PVOID *ppGroupInfoList = NULL;
    LSA_FIND_FLAGS nFindFlags = 0;

    do
    {
        /* Testing NULL connection handler parameter*/
        dwLocalError = ValidateNullParams( NULL,
                                           pszUserName, 
                                           nFindFlags, 
                                           dwGroupInfoLevel, 
                                           &dwNumGroups, 
                                           ppGroupInfoList,
                                           LW_ERROR_INVALID_LSA_CONNECTION,
                                           "Connection");
        BAIL_ON_TEST_BROKE(dwLocalError);

        /* Testing NULL username parameter - Function doesn't 
         * return error for NULL username. */
        dwLocalError = ValidateNullParams( hLsaConnection, 
                                           NULL, 
                                           nFindFlags, 
                                           dwGroupInfoLevel, 
                                           &dwNumGroups, 
                                           ppGroupInfoList,
                                           LW_ERROR_INVALID_PARAMETER,
                                           "Username");
        BAIL_ON_TEST_BROKE(dwLocalError);

        /* Testing NULL pointer, representing number of groups, parameter */
        dwLocalError = ValidateNullParams( hLsaConnection,
                                           pszUserName, 
                                           nFindFlags, 
                                           dwGroupInfoLevel, 
                                           (PDWORD)NULL, 
                                           ppGroupInfoList,
                                           LW_ERROR_INVALID_PARAMETER,
                                           "NumGroups");
        BAIL_ON_TEST_BROKE(dwLocalError);

        /* Testing NULL GroupInfoList pointer */
        dwLocalError = ValidateNullParams( hLsaConnection, 
                                           pszUserName, 
                                           nFindFlags, 
                                           dwGroupInfoLevel, 
                                           &dwNumGroups,
                                           (PVOID*)NULL,
                                           LW_ERROR_INVALID_PARAMETER,
                                           "ppGroupInfoList");
        BAIL_ON_TEST_BROKE(dwLocalError);

    } while ( ++dwGroupInfoLevel <= MAX_GROUP_INFOLEVEL );

error:
    return dwError;
}

/*
 * ValidateNullParams
 *
 * Function validates the API for NULL function parameters.
 *
 */
static
DWORD
ValidateNullParams(
    HANDLE hLsaConnection,
    PCSTR pszUserName,
    LSA_FIND_FLAGS FindFlags,
    DWORD dwGroupInfoLevel,
    PDWORD pdwGroupsFound,
    PVOID* ppGroupInfoList,
    DWORD dwExpectedErrorCode,
    PSTR pszNullParam
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName";
    PCSTR pszTestDescription = 
            "API returns invalid parameter error for NULL function parameters";

    dwLocalError = LsaGetGroupsForUserByName( hLsaConnection, 
                                              pszUserName, 
                                              FindFlags, 
                                              dwGroupInfoLevel, 
                                              pdwGroupsFound,
                                              &ppGroupInfoList);

    if ( dwLocalError != dwExpectedErrorCode ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned with error code (%lu) for NULL %s parameter \
                  and level (%lu)",
                  (unsigned long)dwLocalError,
                  pszNullParam,
                  (unsigned long)dwGroupInfoLevel);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( ppGroupInfoList ) 
    { 
        LsaFreeGroupInfoList( dwGroupInfoLevel, 
                              ppGroupInfoList, 
                              *pdwGroupsFound);
        ppGroupInfoList = NULL; 
    }

    return dwError;
}

/*
 * ValidateForInvalidParams
 * 
 * Function validates the API for NULL function parameters 
 * 
 */
static
DWORD 
ValidateForInvalidParams(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwNumGroups = 0;
    DWORD dwTestIndex = 0;
    PVOID *ppGroupInfoList = NULL;
    PLWTFAILDATA   pInvalidData = NULL;
    LSA_FIND_FLAGS nFindFlags = 0;

    for ( ; dwTestIndex < pTestData->dwNumInvalidDataSet; dwTestIndex++ )
    {
        dwLocalError = GetInvalidDataRecord( pTestData, 
                                             dwTestIndex, 
                                             &pInvalidData);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( LWTUSER_INVALID == pInvalidData->Field )
        {
            dwLocalError = ValidateInvalidParams( hLsaConnection, 
                                                  pInvalidData->pszUserName,
                                                  nFindFlags, 
                                                  pInvalidData->dwLevel,
                                                  &dwNumGroups,
                                                  ppGroupInfoList,
                                                  pInvalidData->dwErrorCode,
                                                  "username");
            BAIL_ON_TEST_BROKE(dwLocalError);
        }
        else if ( LWTGROUPINFOLEVEL_INVALID == pInvalidData->Field )
        {
            dwLocalError = ValidateInvalidParams( hLsaConnection,
                                                  pInvalidData->pszUserName,
                                                  nFindFlags,
                                                  pInvalidData->dwLevel,
                                                  &dwNumGroups,
                                                  ppGroupInfoList,
                                                  pInvalidData->dwErrorCode,
                                                  "group info level");
            BAIL_ON_TEST_BROKE(dwLocalError);
        }
    }

error:
    return dwError;
}

/*
 * ValidateInvalidParams
 *
 * Function validates the API for NULL function parameters.
 *
 */
static
DWORD
ValidateInvalidParams(
    HANDLE hLsaConnection,
    PCSTR  pszUserName,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PDWORD pdwGroupsFound,
    PVOID* ppGroupInfoList,
    DWORD  dwExpectedErrorCode,
    PSTR   pszInvalidParam
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName";
    PCSTR pszTestDescription = 
        "API returns invalid parameter error for invalid function parameters";

    dwLocalError = LsaGetGroupsForUserByName( hLsaConnection, 
                                              pszUserName, 
                                              FindFlags, 
                                              dwGroupInfoLevel, 
                                              pdwGroupsFound,
                                              &ppGroupInfoList);

    if ( dwLocalError != dwExpectedErrorCode ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned with error code (%lu) for invalid %s",
                  (unsigned long)dwLocalError,
                  pszInvalidParam);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( ppGroupInfoList ) 
    { 
        LsaFreeGroupInfoList( dwGroupInfoLevel, 
                              ppGroupInfoList, 
                              *pdwGroupsFound);
        ppGroupInfoList = NULL; 
    }

    return dwError;
}
/*
 * GetUserName 
 * 
 * Function gets user name from the CSV file 
 * 
 */
static
DWORD
GetUserName( 
    PLWTUSER pUserInfo,
    DWORD dwUser,
    PSTR  *ppszUserName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    switch ( dwUser )
    {
        case 0:
            *ppszUserName = pUserInfo->pszNTName;
            break;

        case 1:
            *ppszUserName = pUserInfo->pszAlias;
            break;

        case 2:
            *ppszUserName = pUserInfo->pszUserPrincipalName;
            break;

        case 3:
            *ppszUserName = pUserInfo->pszSamAccountName;
            break;
    }

    if ( IsNullOrEmpty(*ppszUserName) )
    {
        dwError = LW_ERROR_TEST_FAILED ;
    }

    return dwError;
}

/*
 * ValidateGroupInfoByUser
 * 
 * Function processes group information of level0 and level 1 
 * 
 */
static
DWORD
ValidateGroupInfoByUser( 
    PTESTDATA pTestData,
    DWORD dwGroupInfoLevel,
    DWORD dwNumGroups,
    PVOID *ppGrpInfoList,           
    PSTR  pszLogInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwGroup = 0;
    PSTR  pszUser = NULL;
    PCSTR pszTestAPIs = "LsaGetGroupsForUserByName";
    PLWTGROUP pGroupList = NULL;
    PLSA_GROUP_INFO_0 pListGrpInfoLevel0 = NULL;
    PLSA_GROUP_INFO_1 pListGrpInfoLevel1 = NULL;

    for ( dwGroup = 0; dwGroup < dwNumGroups; dwGroup++ )
    {
        if ( ppGrpInfoList[dwGroup] )
        {
            pszUser = ((PLSA_GROUP_INFO_0)ppGrpInfoList[dwGroup])->pszName;

            dwLocalError = ResolveGroupListByName( pszUser, 
                                                   pTestData, 
                                                   &pGroupList);
            BAIL_ON_TEST_BROKE(dwLocalError);

            if ( !dwGroupInfoLevel )
            {
                pListGrpInfoLevel0 = (PLSA_GROUP_INFO_0)ppGrpInfoList[dwGroup];

                dwLocalError = ValidateGroupInfoLevel0( pListGrpInfoLevel0, 
                                                        pGroupList,
                                                        pszLogInfo,
                                                        pszTestAPIs);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
            else 
            {
                pListGrpInfoLevel1 = (PLSA_GROUP_INFO_1)ppGrpInfoList[dwGroup];

                dwLocalError = ValidateGroupInfoLevel1( pListGrpInfoLevel1, 
                                                        pGroupList, 
                                                        pTestData,
                                                        pszLogInfo,
                                                        pszTestAPIs);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }

            FreeGroup(&pGroupList);
        }
    }

error:
    return dwError;
}

