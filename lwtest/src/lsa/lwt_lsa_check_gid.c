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
 * Module Name: lwt_lsa_check_gid.c
 *
 * Verify that GID information returned by AD matches the CSV file.
 *
 */

#include "includes.h"

/*
 * ValidateCheckGid 
 * 
 * Function validates GID of all users with csv file GID value
 * 
 */
static
DWORD
ValidateCheckGid(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 * ValidateUserGid
 * 
 * Function compares the user Gid value from API with csv file GID value
 * 
 */
static
DWORD 
ValidateUserGid(
    gid_t  nUserGid,
    PSTR   pszUserName,
    gid_t* pnGidList,
    DWORD  dwGroupCount
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
 * GetUserGIDByName
 * 
 * Function retrives the user GID from AD 
 *  
 */
static
DWORD
GetUserGIDByName(
    HANDLE  hLsaConnection,
    PSTR    pszUserName,
    gid_t** ppUserGid,
    PDWORD  pdwGroupNums
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
 * FreeGidList 
 * 
 * Function frees Gid list memory 
 * 
 */
static
VOID
FreeGidList(
    gid_t **ppGid
    );

/*
 * ValidateCheckGidByUser 
 * 
 * Function validates the gid of the user
 *  
 */
static
DWORD
ValidateCheckGidByUser(
    HANDLE hLsaConnection,
    PLWTUSER pUserInfoFromAD
    );

/*
 * check_gid_main() 
 *
 * Verify that GID information returned by AD matches the CSV file values.
 *
 */
int 
check_gid_main(
    int   argc, 
    char  *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PTESTDATA pTestData = NULL;
    HANDLE hLsaConnection = NULL;

    dwLocalError = Lwt_LsaTestSetup( argc,
                                     argv,
                                     &hLsaConnection,
                                     &pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = ValidateCheckGid( hLsaConnection, 
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
 * ValidateCheckGid 
 * 
 * Function validates GID of all users with csv file GID value
 * 
 */
static
DWORD
ValidateCheckGid(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwUserCount = 0;
    PLWTUSER pUserInfoFromAD = NULL;

    if ( !pTestData || !pTestData->pUserIface )
    {
        dwLocalError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    for ( dwUserCount = 0; dwUserCount < pTestData->dwNumUsers; dwUserCount++ )
    {   
        dwLocalError = GetUser( pTestData, 
                                dwUserCount, 
                                &pUserInfoFromAD); 
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = ValidateCheckGidByUser( hLsaConnection,
                                               pUserInfoFromAD);
        BAIL_ON_TEST_BROKE(dwLocalError);

        FreeUser(&pUserInfoFromAD);
    }

cleanup:
    return dwError;

error:
    FreeUser(&pUserInfoFromAD);    

    goto cleanup;
}

/*
 * ValidateCheckGidByUser 
 * 
 * Function validates the gid of the user
 *  
 */
static
DWORD
ValidateCheckGidByUser(
    HANDLE hLsaConnection,
    PLWTUSER pUserInfoFromAD
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwUser = 0;
    DWORD dwMaxUser = 2; /* SamAccountName, UserPrincipalName, name, Alias */
    DWORD dwGroupsCount = 0;
    PSTR  pszUserName = NULL;
    gid_t *pUserGidList = NULL;

    if ( !pUserInfoFromAD )
    {
        goto error;
    }

    for ( dwUser = 0; dwUser < dwMaxUser; dwUser++ )
    {
        dwLocalError = GetUserName( pUserInfoFromAD,
                                    dwUser,
                                    &pszUserName);

        if ( LW_ERROR_SUCCESS != dwLocalError )
        {
            continue;
        }

        dwLocalError = GetUserGIDByName( hLsaConnection,    
                                         pszUserName, 
                                         &pUserGidList,
                                         &dwGroupsCount);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = ValidateUserGid( pUserInfoFromAD->nUnixGid,
                                        pszUserName,
                                        pUserGidList,
                                        dwGroupsCount);
        BAIL_ON_TEST_BROKE(dwLocalError);

        FreeGidList(&pUserGidList);
    }

cleanup:
    return dwError;

error:
    FreeGidList(&pUserGidList);
    goto cleanup;
}

/*
 * GetUserGIDByName
 * 
 * Function retrives the user GID from AD 
 *  
 */
static
DWORD 
GetUserGIDByName(
    HANDLE  hLsaConnection,
    PSTR    pszUserName,
    gid_t** ppUserGid,
    PDWORD  pdwGroupNums 
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szLog[256] = { 0 };
    PCSTR pszTestAPIs = "LsaGetGidsForUserByName";
    PCSTR pszTestDescription = "API retrieves GID list for valid user";

    dwLocalError = LsaGetGidsForUserByName( hLsaConnection,
                                            pszUserName,
                                            pdwGroupNums,
                                            ppUserGid);
    
    if ( dwLocalError != LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szLog, 
                  sizeof(szLog), 
                  "API returned unexpected error code (%lu) for user '%s'.",
                  (unsigned long)dwLocalError,
                  pszUserName);
        LWT_LOG_TEST(szLog);
    }
   
    return dwError;
}

/*
 * ValidateUserGid
 * 
 * Function compares the user Gid value from API with csv file GID value
 * 
 */
static
DWORD 
ValidateUserGid(
    gid_t  nUserGid,
    PSTR   pszUserName,
    gid_t* pGidList,
    DWORD  dwGroupCount
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwGroup = 0;
    CHAR  szLog[256] = { 0 };
    PCSTR pszTestAPIs =  "LsaGetGidsForUserByName";
    PCSTR pszTestDescription = 
        "Users GID list retrieved from API should match with users GID in AD";

    if ( !pGidList || !dwGroupCount )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( szLog, 
                  sizeof(szLog), 
                  "User '%s' don't have gid!!!", 
                  pszUserName);
        LWT_LOG_TEST(szLog);

        goto error;
    }

    for ( dwGroup = 0; dwGroup < dwGroupCount; dwGroup++ ) 
    {
        if ( nUserGid != pGidList[dwGroup] )
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf( szLog, 
                      sizeof(szLog), 
                      "User Gid mismatch for User '%s'. Api gives gid (%lu), \
                      where AD shows (%lu)", 
                      pszUserName, 
                      (unsigned long) pGidList[dwGroup],
                      (unsigned long) nUserGid);
            LWT_LOG_TEST(szLog);
        }
    }

error:
    return dwError;
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
    gid_t *pGid = NULL;
    CHAR  szTestMsg[256] = { 0 };
    PSTR  pszUserName = NULL;
    PCSTR pszTestAPIs = "LsaGetGidsForUserByName";
    PCSTR pszTestDescription = 
        "API returns Invalid Parameter error for NULL input function \
        parameters";

    /* Testing NULL connection handler parameter */
    dwLocalError = LsaGetGidsForUserByName( NULL,
                                            pszUserName, 
                                            &dwNumGroups, 
                                            &pGid);

    if ( LW_ERROR_INVALID_PARAMETER != dwLocalError ) 
    {
        dwError = LW_ERROR_TEST_FAILED;    
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned with error code (%lu) for NULL Connection \
                  parameter",
                  (unsigned long)dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }
    
    FreeGidList(&pGid);

    /* Testing NULL user name parameter*/
    dwLocalError = LsaGetGidsForUserByName( hLsaConnection,
                                            NULL, 
                                            &dwNumGroups, 
                                            &pGid);

    if ( LW_ERROR_INVALID_PARAMETER != dwLocalError ) 
    {
        dwError = LW_ERROR_TEST_FAILED;    
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned with error code (%lu) for NULL Username \
                  parameter",
                  (unsigned long)dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }
    
    FreeGidList(&pGid);

    /* Testing NULL pointer, representing number of groups, parameter */
    dwLocalError = LsaGetGidsForUserByName( hLsaConnection, 
                                            pszUserName, 
                                            (PDWORD)NULL, 
                                            &pGid);

    if ( LW_ERROR_INVALID_PARAMETER != dwLocalError ) 
    {
        dwError = LW_ERROR_TEST_FAILED;    
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned with error code (%lu) for NULL Groups count \
                  pointer parameter",
                  (unsigned long)dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }
    
    FreeGidList(&pGid);

    /* Testing NULL gid pointer parameter */
    dwLocalError = LsaGetGidsForUserByName( hLsaConnection, 
                                            pszUserName, 
                                            &dwNumGroups, 
                                            NULL);

    if ( LW_ERROR_INVALID_PARAMETER != dwLocalError ) 
    {
        dwError = LW_ERROR_TEST_FAILED;    
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "API returned with error code (%lu) for NULL Gid \
                  pointer parameter",
                  (unsigned long)dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }
    
    FreeGidList(&pGid);

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
    DWORD dwNumGroups = 0;
    DWORD dwTest = 0;
    gid_t *pGid = NULL;
    CHAR  szTestMsg[256] = { 0 };
    PLWTFAILDATA pInvalidData = NULL;
    PCSTR pszTestAPIs = "LsaGetGidsForUserByName";
    PCSTR pszTestDescription = 
        "API returns invalid parameter error for invalid function parameters";

    if ( !pTestData || !pTestData->pInvalidDataIface )
    {
        dwLocalError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    for ( dwTest = 0; dwTest < pTestData->dwNumInvalidDataSet; dwTest++ )
    {
        dwLocalError = GetInvalidDataRecord( pTestData, 
                                             dwTest, 
                                             &pInvalidData);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( LWTUSER_INVALID == pInvalidData->Field )
        {
            dwLocalError = LsaGetGidsForUserByName( hLsaConnection, 
                                                    pInvalidData->pszUserName,
                                                    &dwNumGroups, 
                                                    &pGid);

            if ( dwLocalError != pInvalidData->dwErrorCode )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf( szTestMsg, 
                          sizeof(szTestMsg), 
                          "API returned with error code (%lu) for invalid \
                          username parameter",
                          (unsigned long)dwLocalError);
                LWT_LOG_TEST(szTestMsg);
            }

            FreeGidList(&pGid);
        }
    }

error:
    return dwError;
}

/*
 * FreeGidList 
 * 
 * Function frees Gid list memory 
 * 
 */
static
VOID
FreeGidList(
    gid_t **ppGid
    )
{
    gid_t *pGid = *ppGid;
    
    if ( pGid )
    {
        LwFreeMemory(pGid);
        pGid = NULL;
    }

    *ppGid = pGid;
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
    PLWTUSER pUserInfoFromAD,
    DWORD dwUser,
    PSTR *ppszUserName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    switch ( dwUser )
    {
        case 0:
            *ppszUserName = pUserInfoFromAD->pszNTName; 
            break;

        case 1:
            *ppszUserName = pUserInfoFromAD->pszAlias;
            break;

        case 2:
            *ppszUserName = pUserInfoFromAD->pszUserPrincipalName;
            break;
    }
    
    if ( IsNullOrEmpty(*ppszUserName) )
    {
        dwError = LW_ERROR_TEST_FAILED ;
    }

    return dwError;
}
