/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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

