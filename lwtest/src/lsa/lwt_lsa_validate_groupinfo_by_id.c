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
 * Module Name: lwt_lsa_validate_groupinfo_byid.c
 *
 * Verifies the group information returned from AD with the CSV file.
 *
 */

#include "lwt_lsa_validate_groupinfo_common.h"

/*
 * ValidateGroupsForUserById 
 * 
 * Function gets and validates the group information for users in userlist 
 * structure 
 * 
 */
static
DWORD
ValidateGroupsForUserById(
    HANDLE hLsaConnection,
    PTESTDATA pTestData    
    );

/*
 * ValidateUserGroupInfo
 * 
 * Function retrieves and validates the group information for a user using UID
 * 
 */
static
DWORD 
ValidateUserGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    uid_t nUnixUid
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
    uid_t  nUid,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PDWORD pdwGroupsFound,
    PVOID* ppGroupInfoList,
    DWORD  dwExpectedErrorCode,
    PSTR   pszInvalidParam
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
    uid_t  nUid,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PDWORD pdwGroupsFound,
    PVOID  *ppGroupInfoList,
    DWORD  dwExpectedErrorCode,
    PSTR   pszNullParam
    );

/*
 * validate_groupinfo_by_id_main()
 *
 * Verifies the group information returned from AD with the CSV file.
 *
 */
int 
validate_groupinfo_by_id_main(
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

    dwLocalError = ValidateGroupsForUserById( hLsaConnection, 
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
 * ValidateGroupsForUserById 
 * 
 * Function gets and validates the group information for users in userlist 
 * structure 
 * 
 */
static
DWORD
ValidateGroupsForUserById(
    HANDLE hLsaConnection,
    PTESTDATA pTestData    
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwUser = 0;
    PLWTUSER pUser = NULL;

    if ( !pTestData || !pTestData->pUserIface || !pTestData->pGroupIface )
    {
        dwLocalError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    for ( dwUser = 0 ; dwUser < pTestData->dwNumUsers; dwUser++ )
    {
        dwLocalError = GetUser( pTestData,
                                dwUser,
                                &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = ValidateUserGroupInfo( hLsaConnection, 
                                              pTestData, 
                                              pUser->nUnixUid);
        BAIL_ON_TEST_BROKE(dwLocalError);

        FreeUser(&pUser);
    }

cleanup:
    return dwError;

error:
    FreeUser(&pUser);
    goto cleanup;
}

/*
 * ValidateUserGroupInfo
 * 
 * Function retrieves and validates the group information for a user using UID
 * 
 */
static
DWORD 
ValidateUserGroupInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData,
    uid_t nUnixUid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwNumGroups = 0;    
    DWORD dwGroupInfoLevel = 0;
    DWORD dwGrpCountInLevel0 = 0;
    DWORD dwGrpCountInLevel1 = 0;
    CHAR  pszLogInfo[32] = {0};
    PVOID *ppGroupInfoList = NULL;
    PCSTR pszTestAPIs = "LsaGetGroupsForUserById";
    PLSA_GROUP_INFO_0 *ppListGrpInfoLevel0 = NULL;
    PLSA_GROUP_INFO_1 *ppListGrpInfoLevel1 = NULL;

    sprintf(pszLogInfo, "User with uid '%lu'", (unsigned long) nUnixUid);

    for ( ; dwGroupInfoLevel <= MAX_GROUP_INFOLEVEL; dwGroupInfoLevel++ )
    {
        dwLocalError = GetGroupsForUserById( hLsaConnection,
                                             nUnixUid,
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
    uid_t nUserUId = 0;
    PVOID *ppGroupInfoList = NULL;
    LSA_FIND_FLAGS nFindFlags = 0;

    do
    {
        /* Testing NULL connection handler parameter*/
        dwLocalError = ValidateNullParams( NULL,   
                                           nUserUId, 
                                           nFindFlags,
                                           dwGroupInfoLevel, 
                                           &dwNumGroups, 
                                           ppGroupInfoList,
                                           LW_ERROR_INVALID_LSA_CONNECTION,
                                           "Connection");
        BAIL_ON_TEST_BROKE(dwLocalError);

        /* Testing NULL pointer, representing number of groups, parameter */
        dwLocalError = ValidateNullParams( hLsaConnection, 
                                           nUserUId, 
                                           nFindFlags, 
                                           dwGroupInfoLevel, 
                                           (PDWORD)NULL, 
                                           ppGroupInfoList,
                                           LW_ERROR_INVALID_PARAMETER,
                                           "pointer to number of groups");
        BAIL_ON_TEST_BROKE(dwLocalError);

        /* Testing NULL GroupInfoList pointer */
        dwLocalError = ValidateNullParams( hLsaConnection, 
                                           nUserUId, 
                                           nFindFlags, 
                                           dwGroupInfoLevel, 
                                           &dwNumGroups,
                                           (PVOID*)NULL,
                                           LW_ERROR_INVALID_PARAMETER,
                                           "pointer to group info list");
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
    uid_t  nUid,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PDWORD pdwGroupsFound,
    PVOID* ppGroupInfoList,
    DWORD  dwExpectedErrorCode,
    PSTR   pszNullParam
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[512] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGroupsForUserById";
    PCSTR pszTestDescription = 
            "API returns invalid parameter error for NULL function parameters";

    dwLocalError = LsaGetGroupsForUserById( hLsaConnection, 
                                            nUid, 
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
 * Function validates the API for Invalid function parameters. 
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
    DWORD dwTest = 0;
    DWORD dwNumGroups = 0;
    PVOID *ppGroupInfoList = NULL;
    PLWTFAILDATA   pInvalidData = NULL;
    LSA_FIND_FLAGS nFindFlags = 0;

    for ( dwTest = 0; dwTest < pTestData->dwNumInvalidDataSet; dwTest++ )
    {
        dwLocalError = GetInvalidDataRecord( pTestData, 
                                             dwTest, 
                                             &pInvalidData);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( LWTUID_INVALID == pInvalidData->Field )
        {
            dwLocalError = ValidateInvalidParams( hLsaConnection, 
                                                  pInvalidData->nUid,
                                                  nFindFlags, 
                                                  pInvalidData->dwLevel, 
                                                  &dwNumGroups,
                                                  ppGroupInfoList,
                                                  pInvalidData->dwErrorCode,
                                                  "uid");
            BAIL_ON_TEST_BROKE(dwLocalError);
        }
        else if ( LWTGROUPINFOLEVEL_INVALID == pInvalidData->Field )
        {
            dwLocalError = ValidateInvalidParams( hLsaConnection,
                                                  pInvalidData->nUid,
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
    uid_t  nUid,
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
    PCSTR pszTestAPIs = "LsaGetGroupsForUserById";
    PCSTR pszTestDescription = 
        "API returns invalid parameter error for invalid function parameters";

    dwLocalError = LsaGetGroupsForUserById( hLsaConnection, 
                                            nUid, 
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
    PCSTR pszTestAPIs = "LsaGetGroupsForUserById";
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
        }
    }
error:
    return dwError;
}

