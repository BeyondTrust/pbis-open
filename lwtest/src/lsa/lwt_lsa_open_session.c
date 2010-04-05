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


DWORD
Lwt_LsaOpenSession(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );


static
DWORD
CheckLsaOpenSession(
    HANDLE hLsaConnection,
    PCSTR pszLoginId,
    PLWTUSER pUser
    );


static
DWORD
VerifyNullHandling(
    HANDLE hLsaConnection
    );

int 
open_session_main(
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

    dwError = Lwt_LsaOpenSession(hLsaConnection, pTestData);
    if ( dwError )
        goto error;

cleanup:

    Lwt_LsaTestTeardown(&hLsaConnection, &pTestData);

    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;
}


/*
 * Lwt_LsaOpenSession
 *
 * Check that LsaEnumUsers, LsaBeginEnumUsers, LsaEndEnumUsers behave
 * as expected, mostly by matching information against pUsersCsv.
 */
DWORD
Lwt_LsaOpenSession(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    size_t nCurrentUser;
    PLWTUSER pUser = NULL;

    if ( ! pTestData)
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        goto error;
    }

    /* For each user (line), verify the information is correct. */
    for ( nCurrentUser = 0; 
          nCurrentUser < pTestData->dwNumUsers; 
          nCurrentUser++)
    {
        dwLocalError = GetUser(pTestData, nCurrentUser, &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( !IsNullOrEmpty(pUser->pszNTName) )
        {
            dwLocalError = CheckLsaOpenSession(
                            hLsaConnection,
                            pUser->pszNTName,
                            pUser);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        if ( !IsNullOrEmpty(pUser->pszUserPrincipalName) )
        {
            dwLocalError = CheckLsaOpenSession(
                           hLsaConnection,
                            pUser->pszUserPrincipalName,
                            pUser);
            BAIL_ON_TEST_BROKE(dwLocalError);
        }

        FreeUser(&pUser);
    }

    dwLocalError = VerifyNullHandling(hLsaConnection);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:

    return dwError;

error:
    goto cleanup;
}


/*
 * CheckLsaOpenSession
 * 
 */
static
DWORD 
CheckLsaOpenSession(
    HANDLE hLsaConnection,
    PCSTR pszLoginId,
    PLWTUSER pUser
    )
{
    PCSTR pszTestDescription = 
        "Home directory exists after call to LsaOpenSession for valid user.";
    PCSTR pszTestAPIs = 
        "LsaOpenSession,"
        "LsaCloseSession,"
        "LsaCheckUserInList,"
        "LsaAuthenticateUser";
    char szTestMsg[128] = { 0 };

    DWORD dwError = LW_ERROR_SUCCESS;

    int bSessionIsOpen = 0;

    snprintf(szTestMsg, sizeof(szTestMsg), "Session for %s", pszLoginId);

    dwError = LsaOpenSession(hLsaConnection, pszLoginId);
    if ( dwError )
        goto error;
    bSessionIsOpen = 1;

    if ( !IsNullOrEmpty(pUser->pszUnixHomeDirectory) )
    {
        struct stat statbuf;
        if ( stat(pUser->pszUnixHomeDirectory, &statbuf) < 0 )
        {
            char buf[64];
            snprintf(
                    buf,
                    sizeof(buf), 
                    ",could not stat %s", 
                    pUser->pszUnixHomeDirectory);
            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
            goto error;
        }

        if ( !S_ISDIR(statbuf.st_mode) )
        {
            Lwt_strcat(
                    szTestMsg, 
                    sizeof(szTestMsg), 
                    ",home is not a directory.");
            dwError = LW_ERROR_TEST_FAILED;
        }

        if ( !IsNullOrEmpty(pUser->pszUnixUid) )
        {
            if ( statbuf.st_uid != pUser->nUnixUid )
            {
                Lwt_strcat(
                        szTestMsg, 
                        sizeof(szTestMsg), 
                        ",uid doesn't match expected");
                dwError = LW_ERROR_TEST_FAILED;
            }
        }
    }


cleanup:

    if ( bSessionIsOpen )
    {
        dwError = LsaCloseSession(hLsaConnection, pszLoginId);
        bSessionIsOpen = 0;
    }

    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:

    goto cleanup;

}

/*
 * VerifyNullHandling
 * 
 */
static
DWORD 
VerifyNullHandling(
    HANDLE hLsaConnection
    )
{
    PCSTR pszTestDescription = 
        "LsaOpenSession does not accept NULL login id.";
    PCSTR pszTestAPIs = 
        "LsaOpenSession";
    char szTestMsg[128] = { 0 };

    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;

    int bSessionIsOpen = 0;

    dwLocalError = LsaOpenSession(hLsaConnection, NULL);
    if ( dwLocalError == LW_ERROR_SUCCESS )
    {
        bSessionIsOpen = 1;

        snprintf(
            szTestMsg, 
            sizeof(szTestMsg), 
            "LsaOpenSession did not return error for a NULL login id.");

        dwError = LW_ERROR_TEST_FAILED;
        goto error;
    }

cleanup:

    if ( bSessionIsOpen )
    {
        dwError = LsaCloseSession(hLsaConnection, NULL);
        bSessionIsOpen = 0;
    }

    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:

    goto cleanup;

}
