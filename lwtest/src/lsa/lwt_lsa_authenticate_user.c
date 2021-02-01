/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * lwt_lsa_authenticate_user <Config File>
 *
 * Verify that information returned by AD matches the CSV file.
 *
 *
 */


#include "includes.h"

/*
 *
 * Get user info from csv 
 * and check for user authentication
 *
 */
DWORD
Lwt_LsaAuthenticateUser(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 *
 *  Authenticate for the account expired user
 *
 */
static
DWORD
AuthenticateUserAccountExpired(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );


/*
 *
 *  Authenticate for password expired user
 *
 */
static
DWORD
AuthenticateUserPasswordExpired(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 * Authenticate for invalid user
 *
 */
static
DWORD
AuthenticateUserInvalid(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
* API Error code check
*/
static    
DWORD
CheckAPIInvalidErrors(
    HANDLE hLsaConnection,
    PLWTFAILDATA pLwtFailData,
    PSTR pszInvalidData
    );


/*
 *
 * Authenticate using invalid data
 *
 */
static
DWORD
AuthenticateUserNULLData(
    HANDLE hLsaConnection
    );

int 
authenticate_user_main(
    int argc, 
    char *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    HANDLE hLsaConnection = NULL;
    PTESTDATA pTestData = NULL;

    dwError = Lwt_LsaTestSetup( 
                            argc, 
                            argv, 
                            &hLsaConnection, 
                            &pTestData);
    if ( dwError )
    {
        goto error;
    }

    dwError = Lwt_LsaAuthenticateUser( 
                                hLsaConnection, 
                                pTestData);
    if ( dwError )
    {
        goto error;
    }

cleanup:

    Lwt_LsaTestTeardown( 
                hLsaConnection, 
                &pTestData);

    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;
}

/*
 *
 * Check for user authentication
 *
 */
DWORD
Lwt_LsaAuthenticateUser(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    size_t nCurrentUser;
    PLWTUSER pUser = NULL;


    if ( !pTestData )
    {   
        dwError = LW_ERROR_TEST_SKIPPED;
        goto error;
    }

    for ( nCurrentUser = 0; 
          nCurrentUser < pTestData->dwNumUsers; 
          nCurrentUser++ )
    {
        dwLocalError = GetUser( 
                        pTestData, 
                        nCurrentUser, 
                        &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( pUser->dwEnabled && 
            !pUser->dwPasswordExpired && 
            !pUser->dwAccountExpired && 
            !pUser->dwUserLockedOut )   /* valid user */
        {
            dwLocalError = AuthenticateUserValid( 
                                        hLsaConnection, 
                                        pUser->pszNTName,
                                        pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
            
            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = AuthenticateUserValid( 
                                            hLsaConnection,
                                            pUser->pszUserPrincipalName,
                                            pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
           }
        }
        else if ( !pUser->dwEnabled &&
                  !pUser->dwAccountExpired &&
                  !pUser->dwUserLockedOut )   /* disabled user */
        {
            dwLocalError = AuthenticateUserDisabled( 
                                        hLsaConnection,
                                        pUser->pszNTName,
                                        pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
                

            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = AuthenticateUserDisabled( 
                                            hLsaConnection,
                                            pUser->pszUserPrincipalName,
                                            pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }
        else if ( pUser->dwAccountExpired && 
                    !pUser->dwUserLockedOut )    /* user account expired */
        {
            dwLocalError = AuthenticateUserAccountExpired( 
                                                hLsaConnection,
                                                pUser->pszNTName,
                                                pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);

            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = AuthenticateUserAccountExpired( 
                                                    hLsaConnection,
                                                    pUser->pszUserPrincipalName,
                                                    pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }
        else if ( pUser->dwUserLockedOut )    /* user locked out */
        {
            dwLocalError = AuthenticateUserLockedOut( 
                                            hLsaConnection,
                                            pUser->pszNTName,
                                            pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = AuthenticateUserLockedOut( 
                                                hLsaConnection,
                                                pUser->pszUserPrincipalName,
                                                pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
                
            }
        }
        else if ( pUser->dwPasswordExpired )    /* user with expired password */
        {
            dwLocalError = AuthenticateUserPasswordExpired( 
                                                hLsaConnection,
                                                pUser->pszNTName,
                                                pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = AuthenticateUserPasswordExpired( 
                                                    hLsaConnection,
                                                    pUser->pszUserPrincipalName,
                                                    pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }
        FreeUser(&pUser);
    }
    
    /* invalid user */
    dwLocalError = AuthenticateUserInvalid( 
                                hLsaConnection,
                                pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

    /* NULL arguements */
    dwLocalError = AuthenticateUserNULLData( hLsaConnection );
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:
    return dwError;

error:
    FreeUser(&pUser);
    goto cleanup;
}

/*
 *
 * Authenticate for valid user
 *
 */
DWORD
AuthenticateUserValid(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestAPIs = "LsaAuthenticateUser";
    PCSTR pszTestDescription = 
        "LsaAuthenticateUser() successful for valid username and password";

    char szTestMsg[128] = { 0 };
    
    dwLocalError = LsaAuthenticateUser( 
                            hLsaConnection, 
                            pszUser, 
                            pszPassword,
                            NULL);

    if ( dwLocalError != LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg, 
            sizeof(szTestMsg), 
            "Authentication returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 *
 * Authenticate for disabled user
 *
 */
DWORD
AuthenticateUserDisabled(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestAPIs = "LsaAuthenticateUser";
    PCSTR pszTestDescription = 
        "Authentication returns LW_ERROR_ACCOUNT_DISABLED for disabled user accounts.";

    char szTestMsg[128] = { 0 };
    
    dwLocalError = LsaAuthenticateUser(hLsaConnection, pszUser, pszPassword, NULL);

    if ( dwLocalError != LW_ERROR_ACCOUNT_DISABLED )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Authentication returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 *
 *  Authenticate for the account expired user
 *
 */
static
DWORD
AuthenticateUserAccountExpired(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestAPIs = "LsaAuthenticateUser";
    PCSTR pszTestDescription = 
        "Authentication returns LW_ERROR_ACCOUNT_EXPIRED for expired user account.";

    char szTestMsg[128] = { 0 };

    dwLocalError = LsaAuthenticateUser(hLsaConnection, pszUser, pszPassword, NULL);

    if ( dwLocalError != LW_ERROR_ACCOUNT_EXPIRED )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Authentication returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }
    
    return dwError;
}

/*
 *
 *  Authenticate for the locked out user
 *
 */
DWORD
AuthenticateUserLockedOut(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestAPIs = "LsaAuthenticateUser";
    PCSTR pszTestDescription = 
        "Authentication returns LW_ERROR_ACCOUNT_LOCKED for locked out user.";

    char szTestMsg[128] = { 0 };

    dwLocalError = LsaAuthenticateUser(hLsaConnection, pszUser, pszPassword, NULL);

    if ( dwLocalError != LW_ERROR_ACCOUNT_LOCKED )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Authentication returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }
    
    return dwError;
}

/*
 *
 *  Authenticate for password expired user
 *
 */
static
DWORD
AuthenticateUserPasswordExpired(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestAPIs = "LsaAuthenticateUser";
    PCSTR pszTestDescription = 
        "Authentication returns LW_ERROR_PASSWORD_EXPIRED for expired password.";

    char szTestMsg[128] = { 0 };

    dwLocalError = LsaAuthenticateUser(hLsaConnection, pszUser, pszPassword, NULL);
    if ( dwLocalError != LW_ERROR_PASSWORD_EXPIRED )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Authentication returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 *
 * Authenticate for invalid user
 *
 */
static
DWORD
AuthenticateUserInvalid(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PLWTFAILDATA pLwtFailData = NULL;
    DWORD dwInvalidTests = 0;
    PSTR pszInvalidData = NULL;
    
    if (!pTestData || !pTestData->pInvalidDataIface)
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwError);
    }


    for ( dwInvalidTests = 0; dwInvalidTests < pTestData->dwNumInvalidDataSet;
          dwInvalidTests++)
    {
        dwError = GetInvalidDataRecord( 
                                pTestData,
                                dwInvalidTests,
                                &pLwtFailData);
        BAIL_ON_LWT_ERROR(dwError);

        if ( pLwtFailData->Field == LWTUSER_INVALID)
        {
            pszInvalidData = "user name";
            dwLocalError = CheckAPIInvalidErrors(
                                        hLsaConnection,
                                        pLwtFailData,
                                        pszInvalidData
                                        );
            BAIL_ON_TEST_BROKE(dwLocalError);
        }
       
        if ( pLwtFailData->Field == LWTPASSWORD_INVALID)
        {
            pszInvalidData = "password";
            dwLocalError = CheckAPIInvalidErrors(
                                        hLsaConnection,
                                        pLwtFailData,
                                        pszInvalidData
                                        );
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        FreeInvalidDataRecord(pLwtFailData);
    }
cleanup:
    return dwError;

error:
    goto cleanup;
}


/*
 *
 * Authentication using invalid data
 *
 */
static
DWORD
AuthenticateUserNULLData(
    HANDLE hLsaConnection
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestAPIs = "LsaAuthenticateUser";
    PCSTR pszTestDescription = "Authentication rejects NULL parameters.";
    char szTestMsg[128] = { 0 };
    
    dwLocalError = LsaAuthenticateUser(hLsaConnection, NULL, NULL, NULL);
    if ( dwLocalError == LW_ERROR_SUCCESS  )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Authentication with NULL parameters succeeded unexpectedly with return code (%lu).",
            (unsigned long) dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }
    
    return dwError;
}
    
static    
DWORD
CheckAPIInvalidErrors(
    HANDLE hLsaConnection,
    PLWTFAILDATA pLwtFailData,
    PSTR pszInvalidData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = 0;
    
    char szTestMsg[256] = { 0 };
    PCSTR pszTestAPIs = "LsaAuthenticateUser";
    PCSTR pszTestDescription = "Authentication rejects bad parameters.";

    dwLocalError = LsaAuthenticateUser(hLsaConnection, pLwtFailData->pszUserName, pLwtFailData->pszPassword, NULL);
    if ( dwLocalError == LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg, 
            sizeof(szTestMsg), 
            "Authentication returned invalid error code %lu for invalid (%s). User: %s", 
            (unsigned long)dwLocalError,
            pszInvalidData,
            pLwtFailData->pszUserName); 
        
        LWT_LOG_TEST(szTestMsg);
    }
    
    return dwError;
}
