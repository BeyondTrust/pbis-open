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
 * lwt-lsaenumusers <Config CSV File>
 *
 * Verify that information returned by LsaEnumUsers matches the CSV file.
 */

#include "includes.h"


/*
 * Lwt_LsaEnumUsers
 *
 * Check that LsaEnumUsers, LsaBeginEnumUsers, LsaEndEnumUsers behave
 * as expected, mostly by matching information against pUsersCsv.
 */
DWORD
Lwt_LsaEnumUsers(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );



/*
* Handle invalid parameter for APIS Enumeration
*
*/
static
DWORD
ValidateEnumUsersForInvalidData(
    HANDLE hLsaConnecton,
    PTESTDATA pTestData
    );


/*
* API for checking invalid error
*
*/
static
DWORD
CheckAPIForInvalidData(
    HANDLE hLsaConnection,
    PLWTFAILDATA pLwtFailData
    );
/*
 * VerifyErrorConditions
 * 
 * Check for proper operation with deliberate errors.
 */
static
DWORD
VerifyErrorConditions(
    HANDLE hLsaConnection
    );


static
DWORD
CheckForUserInUserInfoList(
    DWORD dwUserInfoLevel,
    PVOID *ppUserInfoList,
    DWORD dwNumUsers,
    PCSTR pszUser);



int 
enum_users_main(
    int argc, 
    char *argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = NULL;
    PTESTDATA pTestData = NULL;
   
    dwError = Lwt_LsaTestSetup(
                   argc,
                   argv,
                   &hLsaConnection,
                   &pTestData);
    if ( dwError )
        goto error;

    dwError = Lwt_LsaEnumUsers(hLsaConnection, pTestData);
    if ( dwError )
        goto error;

    dwError = ValidateEnumUsersForInvalidData(
        hLsaConnection,
        pTestData);

    if (dwError)
        goto error;

cleanup:

    Lwt_LsaTestTeardown(&hLsaConnection, &pTestData);
    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;
}


/*
 * Lwt_LsaEnumUsers
 *
 * Check that LsaEnumUsers, LsaBeginEnumUsers, LsaEndEnumUsers behave
 * as expected, mostly by matching information against pUsersCsv.
 */
DWORD
Lwt_LsaEnumUsers(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    size_t nCurrentUser;
    PLWTUSER pUser = NULL;

    
    if (!pTestData)
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        goto error;
    }

    /* For each user (line), verify the information is correct. */
    for ( nCurrentUser = 0; 
          nCurrentUser < pTestData->dwNumUsers; 
          nCurrentUser++)
    {
        DWORD dwUserInfoLevel;

        dwLocalError = GetUser(pTestData, nCurrentUser, &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);

        for ( dwUserInfoLevel = 0; dwUserInfoLevel < 3; dwUserInfoLevel++)
        {
            PCSTR pszName = pUser->pszNTName;
            if ( pUser->pszAlias )
                pszName = pUser->pszAlias;

            if ( pszName )
            {
                dwLocalError = CheckLsaEnumUsers(
                                hLsaConnection,
                                pszName,
                                dwUserInfoLevel,
                                1);
                BAIL_ON_TEST_BROKE(dwLocalError);


                dwLocalError = CheckLsaEnumUsers(
                                hLsaConnection,
                                pszName,
                                dwUserInfoLevel,
                                100);
                BAIL_ON_TEST_BROKE(dwLocalError);
 
                dwLocalError = CheckLsaEnumUsers(
                                hLsaConnection,
                                pszName,
                                dwUserInfoLevel,
                                500);
                BAIL_ON_TEST_BROKE(dwLocalError);
            }
        }

        FreeUser(&pUser);
    }

    VerifyErrorConditions(hLsaConnection);

cleanup:

    if ( pUser )
    {
        FreeUser(&pUser);
    }
    return dwError;

error:
    goto cleanup;
}

/*
 * CheckLsaEnumUsers
 * 
 * Check LSA_USER_INFO_* list from LsaEnumUsers has expected user.
 * 
 */
DWORD
CheckLsaEnumUsers(
    HANDLE hLsaConnection,
    PCSTR pszUser,
    DWORD dwUserInfoLevel,
    DWORD dwMaxNumUsers
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwNumUsers = 0;
    HANDLE hResume = NULL;
    PVOID  *ppUserInfoList = NULL;
    /* Set to true if we ever return more users than we should.
     * Used to avoid repeating messages uselessly.
     */
    BOOL bViolated_dwMaxNumUsers = 0;

    char szTestMsg[128] = { 0 };
    PCSTR pszTestDescription = 
        "LsaEnumUsers retrieved LSA_USER_INFO_* list containing expected user.";
    PCSTR pszTestAPIs = 
        "LsaBeginEnumUsers,"
        "LsaEnumUsers,"
        "LsaFreeUserInfoList,"
        "LsaEndEnumUsers";

    snprintf(
        szTestMsg,
        sizeof(szTestMsg),
        "Looking for %s, lists of max length %lu, dwUserInfoLevel = %lu.",
        pszUser,
        (unsigned long)dwMaxNumUsers,
        (unsigned long)dwUserInfoLevel);


    /* Only one flag right now: LSA_FIND_FLAGS_NSS */
    dwLocalError = LsaBeginEnumUsers(
                        hLsaConnection,
                        dwUserInfoLevel,
                        dwMaxNumUsers,
                        0, /* Flags */
                        &hResume);
    BAIL_ON_TEST_BROKE(dwLocalError);

    do
    {
        dwNumUsers = 0;
        dwLocalError = LsaEnumUsers(
                        hLsaConnection,
                        hResume,
                        &dwNumUsers,
                        (PVOID**) &ppUserInfoList);
        BAIL_ON_TEST_BROKE(dwLocalError);

        /* Avoid testing/reporting problem more than once. */
        if ( ! bViolated_dwMaxNumUsers )
        {
            if ( dwNumUsers > dwMaxNumUsers )
            {
                char buf[64];

                bViolated_dwMaxNumUsers = 1;

                snprintf(
                    buf,
                    sizeof(buf),
                    "Violation: returned %lu users.",
                    (unsigned long)dwNumUsers);

                Lwt_strcat(
                    szTestMsg,
                    sizeof(szTestMsg),
                    buf);

                dwError = LW_ERROR_TEST_FAILED;
            }
        }

        if (  CheckForUserInUserInfoList(
                    dwUserInfoLevel,
                    ppUserInfoList, 
                    dwNumUsers,
                    pszUser) == LW_ERROR_SUCCESS )
        {
             /* Found user, good, time to leave. */
            goto cleanup;
        }

        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsers);
        ppUserInfoList = NULL;

    } while ( dwNumUsers > 0 );

    /* If we are here, a user was missing. */
    dwError = LW_ERROR_TEST_FAILED;

cleanup:
    if ( ppUserInfoList )
    { 
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsers);
        ppUserInfoList = NULL;
        dwNumUsers = 0;
    }

    if ( hResume != (HANDLE)NULL) 
    {
        LsaEndEnumUsers(hLsaConnection, hResume);
        hResume = NULL;
    }
    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:
    goto cleanup;
}


static
DWORD
CheckForUserInUserInfoList(
    DWORD dwUserInfoLevel,
    PVOID *ppUserInfoList,
    DWORD dwNumUsers,
    PCSTR pszUser
    )
{
    DWORD i;

    if ( dwUserInfoLevel == 0 )
    {
        PLSA_USER_INFO_0 *ppUserInfoList_0 = (PLSA_USER_INFO_0*) ppUserInfoList;
        for ( i = 0 ; i < dwNumUsers; i++ )
        {
            if ( !strcasecmp(ppUserInfoList_0[i]->pszName, pszUser) )
            {
                return LW_ERROR_SUCCESS;
            }
        }
    }
    else if ( dwUserInfoLevel == 1 )
    {
        PLSA_USER_INFO_1 *ppUserInfoList_1 = (PLSA_USER_INFO_1*) ppUserInfoList;
        for ( i = 0 ; i < dwNumUsers; i++ )
        {
            if ( !strcasecmp(ppUserInfoList_1[i]->pszName, pszUser) )
            {
                return LW_ERROR_SUCCESS;
            }
        }
    }
    else if ( dwUserInfoLevel == 2 )
    {
        PLSA_USER_INFO_2 *ppUserInfoList_2 = (PLSA_USER_INFO_2*) ppUserInfoList;
        for ( i = 0 ; i < dwNumUsers; i++ )
        {
            if ( !strcasecmp(ppUserInfoList_2[i]->pszName, pszUser) )
            {
                return LW_ERROR_SUCCESS;
            }
        }
    }
    return LW_ERROR_NO_SUCH_USER;
}


/*
 * VerifyErrorConditions
 * 
 * Check for proper operation with deliberate errors.
 */
static
DWORD
VerifyErrorConditions(
    HANDLE hLsaConnection
    )
{
    PCSTR pszTestDescription = 
        "LsaEnumUsers returns error given invalid parameters.";
    PCSTR pszTestAPIs = 
        "LsaBeginEnumUsers,"
        "LsaEnumUsers,"
        "LsaFreeUserInfoList,"
        "LsaEndEnumUsers";
    char szTestMsg[128] = { 0 };


    HANDLE hResume = NULL;

    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;


    /* Case : LsaBeginEnumUsers: Call LsaBeginEnumUsers and then LsaEndEnumUsers. */
    dwLocalError = LsaBeginEnumUsers(
                        hLsaConnection,
                        0,  /* UserInfoLevel. */
                        10, /* Max users to return. */
                        0,  /* Flags. */
                        &hResume);
    BAIL_ON_TEST_BROKE(dwLocalError);
    if ( hResume != (HANDLE)NULL) 
    {
        LsaEndEnumUsers(hLsaConnection, hResume);
        hResume = NULL;
    }

cleanup:
    if ( hResume != (HANDLE)NULL) 
    {
        LsaEndEnumUsers(hLsaConnection, hResume);
        hResume = NULL;
    }
    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:
    goto cleanup;
}





static
DWORD
ValidateEnumUsersForInvalidData(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwFailDataIndex = 0;
    PLWTFAILDATA pLwtFailData = NULL;

    if (!pTestData || !pTestData->pInvalidDataIface)
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwError);
    }

    for (dwFailDataIndex = 0; dwFailDataIndex < pTestData->dwNumInvalidDataSet;
         dwFailDataIndex++)
    {
        dwError = GetInvalidDataRecord(
            pTestData,
            dwFailDataIndex,
            &pLwtFailData);
        BAIL_ON_LWT_ERROR(dwError);

        dwError = CheckAPIForInvalidData(hLsaConnection, pLwtFailData);
    }

cleanup:
    return dwError;

error:
    if (pLwtFailData)
    {
        FreeInvalidDataRecord(pLwtFailData);
    }
    goto cleanup;
}


/*
* API for checking invalid error
*
*/
static
DWORD
CheckAPIForInvalidData(
    HANDLE hLsaConnection,
    PLWTFAILDATA pLwtFailData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hResume = (HANDLE)NULL;
    DWORD dwBatchLimit = 10;
    char szTestMsg[256] = { 0 };
    
    PCSTR pszTestDescription = 
        "Verify LsaEnumUsers parametre for invalid Level and Max Entries";
    PCSTR pszTestAPIs = 
        "LsaBeginEnumUsers";
    

    if (pLwtFailData->Field == LWTMAXUSER_INVALID)
    {
        dwError = LsaBeginEnumUsers(
            hLsaConnection,
            pLwtFailData->dwLevel,
            pLwtFailData->dwMaxEntries,     
            0,
            &hResume);

        if (dwError != pLwtFailData->dwErrorCode)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg,
                sizeof(szTestMsg),
                "Invalid Error Code %lu returned for invalid  max userentry %lu for user name %s",
                (unsigned long)dwError,
                (unsigned long)pLwtFailData->dwMaxEntries,
                pLwtFailData->pszUserName);

            LWT_LOG_TEST(szTestMsg);

        }
    }
    if (pLwtFailData->Field == LWTUSERINFOLEVEL_INVALID)
    {
        dwError = LsaBeginEnumUsers(
            hLsaConnection,
            pLwtFailData->dwLevel,
            dwBatchLimit,
            0,
            &hResume);

        if (dwError != pLwtFailData->dwErrorCode)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg,
                sizeof(szTestMsg),
                "Invalid Error Code %lu returned for invalid group info level %lu",
                (unsigned long)dwError,
                (unsigned long)pLwtFailData->dwLevel);
            
            LWT_LOG_TEST(szTestMsg);
        }
    }
    if (hResume)
    {
        LsaEndEnumUsers(hLsaConnection, hResume);
    }
    return dwError;
}
