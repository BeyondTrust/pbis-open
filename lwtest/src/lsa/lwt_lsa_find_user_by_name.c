/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2009
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
 * lwt-lsafinduserbyname <Config CSV File>
 *
 * Verify that information returned by LsaFindUserByName matches the CSV file.
 *
 */

#include "includes.h"


/*
 * Lwt_LsaFindUserByName
 *
 * Calls tests for LsaFindUserByName.
 */
DWORD
Lwt_LsaFindUserByName(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 * FindUserByName0
 *
 * Check that LsaFindUserByName gets LSA_USER_INFO_0 for given user.
 */
static
DWORD
FindUserByName0(
    HANDLE hLsaConnection,
    PLWTUSER pUser,
    PCSTR pszLookedUpBy,
    PLSA_USER_INFO_0 *ppUserInfo0
    );

/*
 * FindUserByName1
 *
 * Check that LsaFindUserByName gets LSA_USER_INFO_1 for given user.
 */
static
DWORD
FindUserByName1(
    HANDLE hLsaConnection,
    PLWTUSER pUser,
    PCSTR pszLookedUpBy,
    PLSA_USER_INFO_1 *ppUserInfo1
    );

/*
 * MatchUserInfo0
 *
 * Check LSA_USER_INFO_0 matches information in CSV.
 */
static
DWORD
MatchUserInfo0(
    PLWTUSER pUser,
    PCSTR pszLookedupBy,
    PLSA_USER_INFO_0 pUserInfo
    );

/*
 * VerifyNullHandling
 *
 * Make sure NULL does not crash server.
 */
static
DWORD
VerifyNullHandling(
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

static
VOID
FreeUserInfo(
    DWORD dwLevel,
    PVOID pUserInfo
    );

int
find_user_by_name_main(
    int argc, 
    char *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = NULL;
    PTESTDATA pTestData = NULL;

    dwError = Lwt_LsaTestSetup( argc,
                                argv,
                                &hLsaConnection,
                                &pTestData);
    if ( dwError )
    {
        goto error;
    }

    dwError = Lwt_LsaFindUserByName( hLsaConnection, 
                                     pTestData);
    BAIL_ON_TEST_BROKE(dwError);

cleanup:

    Lwt_LsaTestTeardown( &hLsaConnection, 
                         &pTestData);

    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;
}

DWORD
Lwt_LsaFindUserByName(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    size_t nCurrentUser;
    PLWTUSER pUser = NULL;
    PLSA_USER_INFO_0 pUserInfo0 = NULL;
    PLSA_USER_INFO_1 pUserInfo1 = NULL;

    if ( ! pTestData )
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        goto error;
    }

    /* For each user (line), verify the information is correct. */
    for ( nCurrentUser = 0; 
          nCurrentUser < pTestData->dwNumUsers; 
          nCurrentUser++ )
    {
        dwLocalError = GetUser( pTestData, 
                                nCurrentUser, 
                                &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( pUser->pszNTName )
        {
            dwLocalError = FindUserByName0( hLsaConnection,
                                            pUser,
                                            pUser->pszNTName,
                                            &pUserInfo0);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        if ( pUserInfo0 )
        {
            dwLocalError = MatchUserInfo0( pUser,
                                           pUser->pszNTName,
                                           pUserInfo0);
            BAIL_ON_TEST_BROKE(dwLocalError);

            LsaFreeUserInfo(0, pUserInfo0);
            pUserInfo0 = NULL;
        }


        if ( pUser->pszUserPrincipalName )
        {
            dwLocalError = FindUserByName0( hLsaConnection,
                                            pUser,
                                            pUser->pszUserPrincipalName,
                                            &pUserInfo0);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        if ( pUserInfo0 )
        {
            dwLocalError = MatchUserInfo0( pUser,
                                           pUser->pszUserPrincipalName,
                                           pUserInfo0);
            BAIL_ON_TEST_BROKE(dwLocalError);

            LsaFreeUserInfo(0, pUserInfo0);
            pUserInfo0 = NULL;
        }


        if ( pUser->pszNTName )
        {
            dwLocalError = FindUserByName1( hLsaConnection,
                                            pUser,
                                            pUser->pszNTName,
                                            &pUserInfo1);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        if ( pUserInfo1 )
        {
            LsaFreeUserInfo(1, pUserInfo1);
            pUserInfo1 = NULL;
        }

        FreeUser(&pUser);
    }

    dwLocalError = VerifyNullHandling(hLsaConnection);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = ValidateForInvalidParams(hLsaConnection, pTestData);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:

    if ( pUserInfo0 )
    {
        LsaFreeUserInfo(0, pUserInfo0);
        pUserInfo0 = NULL;
    }

    if ( pUserInfo1 )
    {
        LsaFreeUserInfo(1, pUserInfo1);
        pUserInfo1 = NULL;
    }

    if ( pUser )
    {
        FreeUser(&pUser);
    }

    return dwError;

error:
    goto cleanup;

}


/*
 * FindUserByName0
 * 
 * Check that LsaFindUserByName gets LSA_USER_INFO_0 for given user.
 */
static
DWORD
FindUserByName0(
    HANDLE hLsaConnection,
    PLWTUSER pUser,
    PCSTR pszLookedUpBy,
    PLSA_USER_INFO_0 *ppUserInfo0
    )
{
    PCSTR pszTestDescription =
        "LsaFindUserByName retrieved LSA_USER_INFO_0 for given user.";
    PCSTR pszTestAPIs = 
        "LsaFindUserByName";
    char szTestMsg[128] = { 0 };

    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pUserInfo0 = NULL;

    snprintf( szTestMsg, 
              sizeof(szTestMsg), 
              "\n\tAccount %s.\n", 
              pszLookedUpBy);

    dwLocalError = LsaFindUserByName( hLsaConnection,
                                      pszLookedUpBy,
                                      0, 
                                      (PVOID*)&pUserInfo0);
    if ( dwLocalError )
    {
        char buf[128];
        char szErrorMsg[128];

        LwGetErrorString( dwLocalError, 
                          szErrorMsg, 
                          sizeof(szErrorMsg));

        snprintf( buf,
                  sizeof(buf),
                  "\tLsaFindUserByName reports %lu (%s)\n",
                  (unsigned long)dwLocalError,
                  szErrorMsg);

        Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
        dwError = LW_ERROR_TEST_FAILED;
        goto error;
    }

cleanup:

    *ppUserInfo0 = pUserInfo0;

    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:

    if ( pUserInfo0 )
    {
        LsaFreeUserInfo(0, pUserInfo0);
        pUserInfo0 = NULL;
    }

    goto cleanup;
}

/*
 * FindUserByName1
 * 
 * Check that LsaFindUserByName gets LSA_USER_INFO_1 for given user.
 */
static
DWORD
FindUserByName1(
    HANDLE hLsaConnection,
    PLWTUSER pUser,
    PCSTR pszLookedUpBy,
    PLSA_USER_INFO_1 *ppUserInfo1
    )
{
    PCSTR pszTestDescription =
        "LsaFindUserByName retrieved LSA_USER_INFO_1 for given user.";
    PCSTR pszTestAPIs = 
        "LsaFindUserByName";
    char szTestMsg[128] = { 0 };
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PLSA_USER_INFO_1 pUserInfo1 = NULL;

    snprintf( szTestMsg, 
              sizeof(szTestMsg), 
              "\n\tAccount %s.\n", 
              pszLookedUpBy);

    dwLocalError = LsaFindUserByName( hLsaConnection,
                                      pszLookedUpBy,
                                      1, 
                                      (PVOID*)&pUserInfo1);
    if ( dwLocalError )
    {
        char buf[128];
        char szErrorMsg[128];

        LwGetErrorString( dwLocalError, 
                          szErrorMsg, 
                          sizeof(szErrorMsg));

        snprintf( buf,
                  sizeof(buf),
                  "\tLsaFindUserByName reports %lu (%s)\n",
                  (unsigned long)dwLocalError,
                  szErrorMsg);

        Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
        dwError = LW_ERROR_TEST_FAILED;
        goto error;
    }

cleanup:

    *ppUserInfo1 = pUserInfo1;

    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:

    if ( pUserInfo1 )
    {
        LsaFreeUserInfo(1, pUserInfo1);
        pUserInfo1 = NULL;
    }

    goto cleanup;
}

/*
 * MatchUserInfo0
 *
 * Check LSA_USER_INFO_0 matches information in CSV.
 */
static
DWORD
MatchUserInfo0(
    PLWTUSER pUser,
    PCSTR pszLookedUpBy,
    PLSA_USER_INFO_0 pUserInfo
    )
{
    PCSTR pszTestDescription =
        "LsaFindUserByName retrieved LSA_USER_INFO_0 that matches expected values.";
    PCSTR pszTestAPIs = 
        "LsaFindUserByName";
    char szTestMsg[128] = { 0 };
    DWORD dwError = LW_ERROR_SUCCESS;

    snprintf( szTestMsg, 
              sizeof(szTestMsg), 
              "\n\tAccount %s.\n", 
              pszLookedUpBy);

    if ( pUser->pszAlias )
    {
        if ( !pUserInfo->pszName || 
             strcasecmp(pUser->pszAlias, pUserInfo->pszName) )
        {
            char buf[128];

            snprintf( buf,
                      sizeof(buf),
                      "\tAlias: test[%s] != lsassd[%s]\n",
                      pUser->pszAlias,
                      pUserInfo->pszName);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }
    else if ( pUser->pszNTName )
    {
        if ( !pUserInfo->pszName || 
             strcasecmp(pUser->pszNTName, pUserInfo->pszName) )
        {
            char buf[128];

            snprintf( buf,
                      sizeof(buf),
                      "\tNT Name: test[%s] != lsassd[%s]\n",
                      pUser->pszNTName,
                      pUserInfo->pszName);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( !IsNullOrEmpty(pUser->pszSid) )
    {
        if ( ! pUserInfo->pszSid || 
             strcmp(pUser->pszSid, pUserInfo->pszSid) )
        {
            char buf[128];

            snprintf( buf,
                      sizeof(buf),
                      "\tsid: test[%s] != lsassd[%s]\n",
                      pUser->pszSid,
                      pUserInfo->pszSid);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }


    if ( pUser->pszUnixUid )
    {
        if ( pUser->nUnixUid != pUserInfo->uid )
        {
            char buf[128];

            snprintf( buf,
                      sizeof(buf),
                      "\tuid: test[%lu (%s)] != lsassd[%lu]\n",
                      (unsigned long) pUser->nUnixUid,
                      pUser->pszUnixUid,
                      (unsigned long) pUserInfo->uid);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( pUser->pszUnixGid )
    {
        if ( pUser->nUnixGid != pUserInfo->gid )
        {
            char buf[128];

            snprintf( buf,
                      sizeof(buf),
                      "\tgid: test[%lu (%s)] != lsassd[%lu]\n",
                      (unsigned long)pUser->nUnixGid,
                      pUser->pszUnixGid,
                      (unsigned long)pUserInfo->gid);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( !IsNullOrEmpty(pUser->pszUnixGecos) )
    {
        if ( ! pUserInfo->pszGecos ||
             strcmp(pUser->pszUnixGecos, pUserInfo->pszGecos) )
        {
            char buf[128];

            snprintf( buf,
                      sizeof(buf),
                      "\tgecos: test[%s] != lsassd[%s]\n",
                      pUser->pszUnixGecos,
                      pUserInfo->pszGecos ? pUserInfo->pszGecos : "<null>");

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( !IsNullOrEmpty(pUser->pszUnixLoginShell) )
    {
        if ( ! pUserInfo->pszShell || 
             strcmp(pUser->pszUnixLoginShell, pUserInfo->pszShell) )
        {
            char buf[128];

            snprintf( buf,
                      sizeof(buf),
                      "\tshell: test[%s] != lsassd[%s]\n",
                      pUser->pszUnixLoginShell,
                      pUserInfo->pszShell);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( !IsNullOrEmpty(pUser->pszUnixHomeDirectory) )
    {
        if ( ! pUserInfo->pszHomedir || 
             strcmp(pUser->pszUnixHomeDirectory, pUserInfo->pszHomedir) )
        {
            char buf[128];

            snprintf( buf,
                      sizeof(buf),
                      "\thome directory: test[%s] != lsassd[%s]\n",
                      pUser->pszUnixHomeDirectory,
                      pUserInfo->pszHomedir);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    LWT_LOG_TEST(szTestMsg);
    return dwError;
}

    
/*
 * VerifyNullHandling
 *
 * Make sure NULL does not crash server.
 */
static
DWORD
VerifyNullHandling(
    HANDLE hLsaConnection
    )
{
    PCSTR pszTestDescription = 
        "LsaFindUserByName returns error given NULL user name.";
    PCSTR pszTestAPIs = 
        "LsaFindUserByName";
    char szTestMsg[128] = { 0 };
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    LSA_USER_INFO_0 *pUserInfo = NULL;
    size_t i;

    for ( i = 0; i < 3; i++ )
    {
        dwLocalError = LsaFindUserByName( hLsaConnection,
                                          NULL,
                                          i, 
                                          (void**) &pUserInfo);
        
        if ( pUserInfo )
        {
            LsaFreeUserInfo(i, pUserInfo);
        }

        if ( dwLocalError != LW_ERROR_INVALID_PARAMETER )
        {
            dwError = LW_ERROR_TEST_FAILED;
            goto error;
        }
    }

cleanup:
    LWT_LOG_TEST(szTestMsg);
    return dwError;

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
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwTest = 0;
    CHAR  szTestMsg[256] = { 0 };
    PLWTFAILDATA pInvalidData = NULL;
    PVOID pUserInfo = NULL;
    PCSTR pszTestAPIs = "LsaFindUserByName";
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
            dwLocalError = LsaFindUserByName(
                                hLsaConnection,
                                pInvalidData->pszUserName,
                                pInvalidData->dwLevel,
                                &pUserInfo
                                );

            if ( dwLocalError != pInvalidData->dwErrorCode )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf( szTestMsg, 
                          sizeof(szTestMsg), 
                          "API returned with error code (%lu) for invalid user name parameter",
                          (unsigned long)dwLocalError);
                LWT_LOG_TEST(szTestMsg);
            }

            FreeUserInfo(pInvalidData->dwLevel, pUserInfo);
        }

        if ( LWTUSERINFOLEVEL_INVALID == pInvalidData->Field )
        {
            dwLocalError = LsaFindUserByName(
                                hLsaConnection,
                                pInvalidData->pszUserName,
                                pInvalidData->dwLevel,
                                &pUserInfo
                                );

            if ( dwLocalError != pInvalidData->dwErrorCode )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf( szTestMsg, 
                          sizeof(szTestMsg), 
                          "API returned with error code (%lu) for invalid user name parameter",
                          (unsigned long)dwLocalError);
                LWT_LOG_TEST(szTestMsg);
            }

            FreeUserInfo(pInvalidData->dwLevel, pUserInfo);
        }

        FreeInvalidDataRecord(pInvalidData);
    }

error:
    return dwError;
}

static
VOID
FreeUserInfo(
    DWORD dwLevel,
    PVOID pUserInfo
    )
{
    if ( pUserInfo )
    {
        LsaFreeUserInfo(
                    dwLevel, 
                    pUserInfo);
        pUserInfo = NULL;
    }
    return;
}
