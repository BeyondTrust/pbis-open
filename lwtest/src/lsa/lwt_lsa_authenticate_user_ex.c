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
Lwt_LsaAuthenticateUserEx(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/* 
 * Check User authentication
 *
 */

static
DWORD 
AuthenticateUserEx(
    HANDLE hLsaConnection,
    PLWTUSER pUSer
    );
/*
 *
 * Initialize auth params
 *
 */

static
DWORD
InitAuthParams(
    PLWTUSER pUser,
    LSA_AUTH_USER_PARAMS** ppAuthParams
    );

/*
 *
 * verify user info returned 
 * from LsaAuthenticateUserEx
 *
 */
static
DWORD
VerifyUserInfo(
    PLWTUSER pUser,
    PLSA_AUTH_USER_INFO pUserInfo
    );

int 
authenticate_user_ex_main(
    int argc, 
    char *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    HANDLE hLsaConnection = NULL;
    PTESTDATA pTestData = NULL;

    dwError = Lwt_LsaTestSetup(argc, argv, &hLsaConnection, &pTestData);
    if ( dwError )
        goto error;

    dwError = Lwt_LsaAuthenticateUserEx(hLsaConnection, pTestData);
    
    if ( dwError )
        goto error;

cleanup:

    Lwt_LsaTestTeardown(&hLsaConnection, &pTestData);
    
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
Lwt_LsaAuthenticateUserEx(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    size_t nCurrentUser;
    PLWTUSER pUser = NULL;

     for ( nCurrentUser = 0; 
          nCurrentUser < pTestData->dwNumUsers; 
          nCurrentUser++)
    {
        dwLocalError = GetUser(pTestData, nCurrentUser, &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);

        /* TODO: check for disabled user */

        dwLocalError = AuthenticateUserEx(hLsaConnection, pUser);
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
 * Check User authentication
 *
 */
static
DWORD 
AuthenticateUserEx(
    HANDLE hLsaConnection,
    PLWTUSER pUser
    )
{
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;

    LSA_AUTH_USER_PARAMS* pParams;
    PLSA_AUTH_USER_INFO pUserInfo;
    
    PCSTR pszTestDescription = 
        "Check User Authentication";
    PCSTR pszTestAPIs = 
        "LsaAuthenticateUser";

    char szTestMsg[128] = { 0 };

    snprintf(szTestMsg, sizeof(szTestMsg), "Looking for %s:%s", pParams->pszAccountName, pParams->pass.clear.pszPassword);
        
        
    dwLocalError = InitAuthParams(pUser, &pParams);   
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = LsaAuthenticateUserEx(hLsaConnection, NULL, pParams, &pUserInfo);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = VerifyUserInfo(pUser, pUserInfo);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:

    LsaFreeAuthUserInfo(&pUserInfo);
    LsaFreeAuthUserParams(&pParams);
    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:
    goto cleanup;
}

/*
 *
 * Initialize auth params
 *
 */

static
DWORD
InitAuthParams(
    PLWTUSER pUser,
    LSA_AUTH_USER_PARAMS** ppAuthParams
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LSA_AUTH_USER_PARAMS* pParams = NULL;

    dwError = LwAllocateMemory(sizeof(LSA_AUTH_USER_PARAMS), (LW_PVOID*)&pParams);
    BAIL_ON_LWT_ERROR(dwError);

    pParams->AuthType = LSA_AUTH_PLAINTEXT;
    pParams->pszAccountName = (LW_PSTR) pUser->pszNTName;
    pParams->pszDomain = (LW_PSTR) pUser->pszNetBiosName;
    pParams->pszWorkstation = "vm-ubuntu-test.mydc.likewise.com"; /*TODO: find out how to get workstaion name */
    pParams->pass.clear.pszPassword = (LW_PSTR) pUser->pszPassword;

cleanup:
    *ppAuthParams = pParams;
    return dwError;
error:
    goto cleanup;
}


/*
 *
 * verify user info returned 
 * from LsaAuthenticateUserEx
 *
 */
static
DWORD
VerifyUserInfo(
    PLWTUSER pUser,
    PLSA_AUTH_USER_INFO pUserInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    
    PCSTR pszTestDescription = 
        "Check LSA_AUTH_USER_INFO structure returned by"
        "LsaAuthenticateUserEx contains the same value as in CSV file";
    PCSTR pszTestAPIs = 
        "LsaauthenticateEx,"
        "LsaFreeAuthUserInfo,"
        "LsaFreeAuthUserParams";
   
    char szTestMsg[128] = { 0 };


    if ( !IsNullOrEmpty(pUserInfo->pszAccount) && 
        strcmp(pUserInfo->pszAccount, pUser->pszNTName) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field pszAccount:%s \
            info in csv:%s", pUserInfo->pszAccount, pUser->pszNTName);
        goto error;

    }

    if ( !IsNullOrEmpty(pUser->pszUserPrincipalName) )
    {
        if ( !IsNullOrEmpty(pUserInfo->pszUserPrincipalName) && 
            strcmp(pUserInfo->pszUserPrincipalName, pUser->pszUserPrincipalName) )
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
                "got inconsistant info for the field pszUserPrincipalName:%s \
                info in csv:%s", pUserInfo->pszUserPrincipalName, 
                pUser->pszUserPrincipalName);
            goto error;
        }

    }

    if ( !IsNullOrEmpty(pUserInfo->pszFullName) &&
        strcmp(pUserInfo->pszFullName, pUser->pszNTName) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field pszFullName:%s \
            info in csv:%s", pUserInfo->pszFullName, pUser->pszNTName);
        goto error;
    }

    if ( !IsNullOrEmpty(pUserInfo->pszDomain) &&
        strcmp(pUserInfo->pszDomain, pUser->pszNetBiosName) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field pszDomain:%s \
            info in csv:%s", pUserInfo->pszDomain, 
            pUser->pszNetBiosName);
        goto error;
    }
#if 0
    if ( pUserInfo->LogonCount != pUser->dwLogonCount )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field LogonCount:%d \
            info in csv:%d", pUserInfo->LogonCount, pUser->dwLogonCount);
        goto error;
    }
#endif
    if ( pUserInfo->BadPasswordCount != pUser->dwBadPasswordCount )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field BadPasswordCount:%d \
            info in csv:%d", pUserInfo->BadPasswordCount, 
            pUser->dwBadPasswordCount);
        goto error;
    }

    if ( pUserInfo->LogonTime != pUser->dwLogonTime )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field LogonTime:%d \
            info in csv:%d", pUserInfo->LogonTime, 
            pUser->dwLogonTime);
        goto error;
    }

    if ( pUserInfo->LogoffTime != pUser->dwLogoffTime )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field LogoffTime:%d \
            info in csv:%d", pUserInfo->LogoffTime, 
            pUser->dwLogoffTime);
        goto error;
    }

    if ( pUserInfo->KickoffTime != pUser->dwKickoffTime )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field KickoffTime:%d \
            info in csv:%d", pUserInfo->KickoffTime, 
            pUser->dwKickoffTime);
        goto error;
    }

    if ( pUserInfo->LastPasswordChange != pUser->dwLastPasswordChange )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field LastPasswordChange:%d \
            info in csv:%d", pUserInfo->LastPasswordChange, 
            pUser->dwLastPasswordChange);
        goto error;
    }

    if ( pUserInfo->CanChangePassword != pUser->dwCanChangePassword )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field CanChangePassword:%d \
            info in csv:%d", pUserInfo->CanChangePassword, 
            pUser->dwCanChangePassword);
        goto error;
    }

    if ( pUserInfo->MustChangePassword != pUser->dwMustChangePassword )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for the field MustChangePassword:%d \
            info in csv:%d", pUserInfo->MustChangePassword, 
            pUser->dwMustChangePassword);
        goto error;
    }
/*
    LW_DWORD dwUserFlags;
    LW_PSTR pszDnsDomain;

    LW_DWORD dwAcctFlags;
    PLW_LSA_DATA_BLOB pSessionKey;
    PLW_LSA_DATA_BLOB pLmSessionKey;
 
    LW_PSTR pszLogonServer;
    LW_PSTR pszLogonScript;
    LW_PSTR pszProfilePath;
    LW_PSTR pszHomeDirectory;
    LW_PSTR pszHomeDrive;

    LW_PSTR pszDomainSid;
    LW_DWORD dwUserRid;
    LW_DWORD dwPrimaryGroupRid;

    LW_DWORD dwNumRids;
    PLSA_RID_ATTRIB pRidAttribList;

    LW_DWORD dwNumSids;
    PLSA_SID_ATTRIB pSidAttribList;
*/
cleanup:
    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:
    goto cleanup;
}

