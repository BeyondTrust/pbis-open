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
 *
 *
 * This programs verifies the user information across different level are consistent
 * It validates the information from the AD with a CSV fie for different user properties
 */

#include "includes.h"

static
DWORD 
Lwt_LsaCheckUserInfo(
    HANDLE hLSAConnection,
    DWORD dwLevel,
    PTESTDATA pTestData
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
    HANDLE hLSAConnection,
    PTESTDATA pTestData
    );

int 
check_user_info_main(
    int argc, 
    char *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    HANDLE hLSAConnection = NULL;
    PTESTDATA pTestData;
    
    dwError = Lwt_LsaTestSetup( argc,
                                argv,
                                &hLSAConnection,
                                &pTestData);
    BAIL_ON_LWT_ERROR(dwError);

    /*Start collect the validating the info for  level 0*/
    dwLocalError = Lwt_LsaCheckUserInfo(hLSAConnection, 
                                        0,
                                        pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);
    
    dwLocalError = ValidateForInvalidParams(
                                        hLSAConnection, 
                                        pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);
    

    /* Compare the group information based on different levels*/
    /*if (pUserInfo0 && pUserInfo1)
    {
        Lwt_Lsa_ValidateInfosBasedOnLevel(
                                        pUserInfo0,
                                        pUserInfo1,
                                        0,
                                        1);
    }
    
    
    if (pUserInfo0 && pUserInfo1)
    {
        Lwt_Lsa_ValidateInfosBasedOnLevel(
                                        pUserInfo0,
                                        pUserInfo1,
                                        0,
                                        2);
    }
    
    
    if(pUserInfo1 && pUserInfo2)
    {
        Lwt_Lsa_ValidateInfosBasedOnLevel(
                                        pUserInfo0,
                                        pUserInfo1,
                                        1,
                                        2);
    }*/
    
cleanup:
    Lwt_LsaTestTeardown(
                    &hLSAConnection,
                    &pTestData
                    );

    return LwtMapErrorToProgramStatus(dwError);
                        
error:
    goto cleanup;
}

/*
 * Higher level api to query the user info by ID
 *
 */
static
DWORD 
Lwt_LsaCheckUserInfo(
    HANDLE hLSAConnection,
    DWORD dwLevel,
    PTESTDATA pTestData)
{
    DWORD dwUserCount=0;
    DWORD dwError = LW_ERROR_SUCCESS;
    PLWTUSER pUser = NULL;
    PCSTR pszTestDescription = "API test for LsaCheckUserInList";
    PCSTR pszTestAPIs = "LsaCheckUserInList";
    char pszTestMsg[256] = {0};
    
    for (dwUserCount = 0; dwUserCount !=  pTestData->dwNumUsers;dwUserCount++)
    {
        dwError = GetUser(
                        pTestData,
                        dwUserCount,
                        &pUser
                        );
        BAIL_ON_LWT_ERROR(dwError);

        if (pUser)
        {
            memset(pszTestMsg, 0, 256);
            snprintf(pszTestMsg, sizeof(pszTestMsg), "Checking for User:%s", pUser->pszNTName); 
            dwError = LsaCheckUserInList(
                        hLSAConnection,
                        pUser->pszNTName,
                        NULL
                        );
            BAIL_ON_TEST_BROKE(dwError);
        }
        FreeUser(&pUser);
    }
cleanup :
    FreeUser(&pUser);
    LWT_LOG_TEST(pszTestMsg);
    return dwError;

error :
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
    CHAR  szTestMsg[256] = { 0 };
    PLWTFAILDATA pInvalidData = NULL;
    PCSTR pszTestAPIs = "LsaCheckUserInList";
    PCSTR pszTestDescription = 
            "API returns invalid parameter error for invalid function parameters";

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

        if ( LWTUSER_INVALID == pInvalidData->Field )
        {
            dwLocalError = LsaCheckUserInList( 
                                    hLSAConnection,
                                    pInvalidData->pszUserName,
                                    NULL);

            if ( pInvalidData->dwErrorCode != dwLocalError )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf( 
                    szTestMsg, 
                    sizeof(szTestMsg), 
                    "API returned with error code (%lu) for invalid user name parameter",
                    (unsigned long)dwLocalError);
                LWT_LOG_TEST(szTestMsg);
            }
        }
        FreeInvalidDataRecord(pInvalidData);
    }

error:
    return dwError;
}

