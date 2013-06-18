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
 * lwt_lsa_validate_user <Config File>
 *
 * Verify that information returned by AD matches the CSV file.
 *
 *
 */


#include "includes.h"

/*
 *
 * Get user info from csv 
 * and check for user validation
 *
 */
DWORD
Lwt_LsaValidateUser(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/* 
 * 
 * Test with invalid parameters
 *
 */
static    
DWORD
TestAPIForInvalidParams(
    HANDLE hLsaConnection,
    PLWTFAILDATA pInvalidData
    );

/*
 *
 *  Validation for the account expired user
 *
 */
static
DWORD
ValidateUserAccountExpired(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 *  Validation for password expired user
 *
 */
static
DWORD
ValidateUserPasswordExpired(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 * Validation for invalid user
 *
 */
static
DWORD
ValidateUserInvalid(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 *
 * Validation using invalid data
 *
 */
static
DWORD
ValidateUserNULLData(
    HANDLE hLsaConnection
    );

static
DWORD
CheckForInvalidParams(
        HANDLE hLsaConnection,
        PLWTFAILDATA pInvalidData,
        PSTR pszMessage
        );

int 
validate_user_main(
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

    dwError = Lwt_LsaValidateUser( 
                            hLsaConnection, 
                            pTestData);
    
    if ( dwError )
    {
        goto error;
    }

cleanup:

    Lwt_LsaTestTeardown( 
                &hLsaConnection, 
                &pTestData);
    
    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;

}

/*
 *
 * Check for user validation
 *
 */
DWORD
Lwt_LsaValidateUser(
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
            !pUser->dwAccountExpired && 
            !pUser->dwUserLockedOut &&
            !pUser->dwPasswordExpired )   /* valid user */
        {
            dwLocalError = ValidateUserValid( 
                                    hLsaConnection, 
                                    pUser->pszNTName, 
                                    pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
            
            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = ValidateUserValid( 
                                        hLsaConnection, 
                                        pUser->pszUserPrincipalName, 
                                        pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }
        else if ( !pUser->dwEnabled &&
                  !pUser->dwAccountExpired && 
                  !pUser->dwUserLockedOut )  /* disabled user */
        {
            dwLocalError = ValidateUserDisabled( 
                                        hLsaConnection, 
                                        pUser->pszNTName, 
                                        pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
                
            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = ValidateUserDisabled( 
                                            hLsaConnection, 
                                            pUser->pszUserPrincipalName, 
                                            pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }
        else if ( pUser->dwAccountExpired && !pUser->dwUserLockedOut )    /* user account expired */
        {
            dwLocalError = ValidateUserAccountExpired( 
                                            hLsaConnection, 
                                            pUser->pszNTName, 
                                            pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
            
            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = ValidateUserAccountExpired(
                                                hLsaConnection, 
                                                pUser->pszUserPrincipalName, 
                                                pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }
        else if ( pUser->dwUserLockedOut )    /* user locked out */
        {
            dwLocalError = ValidateUserLockedOut( 
                                        hLsaConnection, 
                                        pUser->pszNTName, 
                                        pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
            
            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = ValidateUserLockedOut( 
                                            hLsaConnection, 
                                            pUser->pszUserPrincipalName, 
                                            pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }
        else if ( pUser->dwPasswordExpired )    /* user with expired password */
        {
            dwLocalError = ValidateUserPasswordExpired( 
                                            hLsaConnection, 
                                            pUser->pszNTName, 
                                            pUser->pszPassword);
            BAIL_ON_TEST_BROKE(dwLocalError);
            
            if ( pUser->pszUserPrincipalName ) 
            {
                dwLocalError = ValidateUserPasswordExpired( 
                                                hLsaConnection, 
                                                pUser->pszUserPrincipalName, 
                                                pUser->pszPassword);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }
        FreeUser(&pUser);
    }
    
    /* invalid user */
    dwLocalError = ValidateUserInvalid( hLsaConnection, pTestData );
    BAIL_ON_TEST_BROKE(dwLocalError);

    /* NULL arguements */
    dwLocalError = ValidateUserNULLData( hLsaConnection ); 
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:
    return dwError;

error:
    FreeUser(&pUser);
    goto cleanup;
}

/*
 *
 * Validation for valid user
 *
 */
DWORD
ValidateUserValid(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = 
        "Validation returns LW_ERROR_SUCCSS for user account in good standing with valid password.";
    PCSTR pszTestAPIs = 
        "LsaValidateUser";

    char szTestMsg[128] = { 0 };
    
    dwLocalError = LsaValidateUser( 
                        hLsaConnection, 
                        pszUser, 
                        pszPassword);
    if ( dwLocalError != LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Validation returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 *
 * Validation for disabled user
 *
 */
DWORD
ValidateUserDisabled(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = 
        "Validation returns LW_ERROR_ACCOUNT_DISABLED for disabled user accounts.";
    PCSTR pszTestAPIs = 
        "LsaValidateUser";

    char szTestMsg[128] = { 0 };
    
    dwLocalError = LsaValidateUser( 
                        hLsaConnection, 
                        pszUser, 
                        pszPassword);
    if ( dwLocalError != LW_ERROR_ACCOUNT_DISABLED )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Validation returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 *
 *  Validation for the account expired user
 *
 */
static
DWORD
ValidateUserAccountExpired(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = 
        "Validation returns LW_ERROR_ACCOUNT_EXPIRED for expired user accounts.";
    PCSTR pszTestAPIs = 
        "LsaValidateUser";

    char szTestMsg[128] = { 0 };

    dwLocalError = LsaValidateUser( 
                        hLsaConnection, 
                        pszUser, 
                        pszPassword);
    if ( dwLocalError != LW_ERROR_ACCOUNT_EXPIRED )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Validation returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }
    
    return dwError;
}

/*
 *
 *  Validation for locked out user
 *
 */
DWORD
ValidateUserLockedOut(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = 
        "Validation returns LW_ERROR_ACCOUNT_LOCKED for locked out user.";
    PCSTR pszTestAPIs = 
        "LsaValidateUser";

    char szTestMsg[128] = { 0 };

    dwLocalError = LsaValidateUser( 
                        hLsaConnection, 
                        pszUser, 
                        pszPassword);

    if ( dwLocalError != LW_ERROR_ACCOUNT_LOCKED )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Validation returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 *
 *  Validation for password expired user
 *
 */
static
DWORD
ValidateUserPasswordExpired(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = 
        "Validation returns LW_ERROR_PASSWORD_EXPIRED for expired password.";
    PCSTR pszTestAPIs = 
        "LsaValidateUser";

    char szTestMsg[128] = { 0 };

    dwLocalError = LsaValidateUser( 
                        hLsaConnection, 
                        pszUser, 
                        pszPassword);

    if ( dwLocalError != LW_ERROR_PASSWORD_EXPIRED )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Validation returned unexpected value(%lu) for user %s.",
            (unsigned long)dwLocalError,
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 *
 * Validation for invalid user
 *
 */
static
DWORD
ValidateUserInvalid(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwInvalidTests = 0;
    PLWTFAILDATA pInvalidData = NULL;

    for ( dwInvalidTests = 0; dwInvalidTests < pTestData->dwNumInvalidDataSet;
            dwInvalidTests++)
    {
        dwError = GetInvalidDataRecord( 
                                pTestData,
                                dwInvalidTests,
                                &pInvalidData);
        BAIL_ON_LWT_ERROR(dwError);
        
        dwError = TestAPIForInvalidParams(hLsaConnection, pInvalidData);
        
        FreeInvalidDataRecord(pInvalidData);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
   
}

static    
DWORD
TestAPIForInvalidParams(
    HANDLE hLsaConnection,
    PLWTFAILDATA pInvalidData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = 0;
    PSTR pszMessage = NULL;

    if ( pInvalidData->Field == LWTUSER_INVALID)
    {
        pszMessage = "user name";
        dwLocalError = CheckForInvalidParams(
                                    hLsaConnection,
                                    pInvalidData,
                                    pszMessage
                                    );
        BAIL_ON_TEST_BROKE(dwLocalError);
    }
    
    if ( pInvalidData->Field == LWTPASSWORD_INVALID)
    {
        pszMessage = "password";
        dwLocalError = CheckForInvalidParams(
                                    hLsaConnection,
                                    pInvalidData,
                                    pszMessage
                                    );
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/*
 *
 * Validation using NULL data
 *
 */
static
DWORD
ValidateUserNULLData(
    HANDLE hLsaConnection
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = 
        "Validation rejects NULL parameters.";
    PCSTR pszTestAPIs = 
        "LsaValidateUser";
    char szTestMsg[128] = { 0 };
    
    dwLocalError = LsaValidateUser( 
                        hLsaConnection, 
                        NULL, 
                        NULL);
    if ( dwLocalError != LW_ERROR_INVALID_PARAMETER )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf( 
            szTestMsg,
            sizeof(szTestMsg), 
            "Validation with NULL parameters succeeded unexpectedly with return code (%lu)",
            (unsigned long)dwLocalError);
        LWT_LOG_TEST(szTestMsg);
    }
    
    return dwError;
}

static
DWORD
CheckForInvalidParams(
        HANDLE hLsaConnection,
        PLWTFAILDATA pInvalidData,
        PSTR pszMessage
        )
{
    DWORD dwError = LW_ERROR_SUCCESS;
//    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PCSTR pszTestAPIs = "LsaValidateUser";
    PCSTR pszTestDescription = "Validation rejects bad parameters.";
    char szTestMsg[256] = { 0 };

    dwError = LsaValidateUser( 
                        hLsaConnection, 
                        pInvalidData->pszUserName, 
                        pInvalidData->pszPassword);
    
    if ( dwError != pInvalidData->dwErrorCode )
    {
        snprintf( szTestMsg, 
                  sizeof(szTestMsg), 
                  "Validation returned invalid error code %lu for invalid %s. Username: %s", 
                  (unsigned long)dwError,
                  pszMessage,
                  pInvalidData->pszUserName); 
        
        LWT_LOG_TEST(szTestMsg);
        dwError = LW_ERROR_TEST_FAILED;
    }

    return dwError;
}
