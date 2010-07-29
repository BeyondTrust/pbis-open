/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "includes.h"
#include "lam-auth.h"
#include "lam-user.h"

int
LsaNssNormalizeUsername(
    PSTR pszInput,
    PSTR pszOutput
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pInfo = NULL;
    const DWORD dwInfoLevel = 0;
    uint64_t qwConvert = 0;
    int iDigit = 0;
    PSTR pszPos = NULL;

    if (strlen(pszInput) < S_NAMELEN)
    {
        strcpy(pszOutput, pszInput);
        goto cleanup;
    }

    dwError = LsaNssCommonEnsureConnected(&lsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                lsaConnection.hLsaConnection,
                pszInput,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    qwConvert = pInfo->uid;

    pszPos = pszOutput + S_NAMELEN - 1;
    *pszPos-- = 0;

    if (qwConvert < 10000000)
    {
        // Mangle the username with the old rules
        while(pszPos > pszOutput)
        {
            iDigit = qwConvert % 10;
            *pszPos = iDigit + '0';

            qwConvert /= 10;
            pszPos--;
        }
    }
    else
    {
        // Mangle the username with the new rules. The mangled user name will
        // start with _ and the second character will be a letter. The uid
        // number (with padding) will be in base 32.
        qwConvert += 10737418240ull;

        while(pszPos > pszOutput)
        {
            iDigit = qwConvert % 32;
            if (iDigit < 10)
            {
                *pszPos = iDigit + '0';
            }
            else
            {
                *pszPos = iDigit + 'a' - 10;
            }

            qwConvert /= 32;
            pszPos--;
        }
    }
    *pszPos = '_';

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                (PVOID)pInfo);
    }
    if (dwError != LW_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &errno);
        return 0;
    }
    return strlen(pszOutput);

error:

    *pszOutput = 0;

    LsaNssCommonCloseConnection(&lsaConnection);

    goto cleanup;
}

int
LsaNssAuthenticate(
    PSTR pszUser,
    PSTR pszResponse,
    int* pReenter,
    PSTR* ppszOutputMessage
    )
{
    int iError = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pInfo = NULL;
    const DWORD dwInfoLevel = 0;
    PSTR pszMessage = NULL;
    PLSA_USER_INFO_2 pInfo2 = NULL;

    LSA_LOG_PAM_DEBUG("Lsass LAM authenticating user [%s]",
            pszUser? pszUser: "(null)");

    dwError = LsaNssCommonEnsureConnected(&lsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssFindUserByAixName(
                lsaConnection.hLsaConnection,
                pszUser,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAuthenticateUser(
                lsaConnection.hLsaConnection,
                pInfo->pszName,
                pszResponse,
                &pszMessage);
    if (dwError == LW_ERROR_PASSWORD_EXPIRED)
    {
        // Double check that the user's password is marked as expired
        dwError = LsaFindUserByName(
                    lsaConnection.hLsaConnection,
                    pInfo->pszName,
                    2,
                    (PVOID*)&pInfo2);
        BAIL_ON_LSA_ERROR(dwError);

        if (!pInfo2->bPasswordExpired)
        {
            // Something went wrong in lsassd -- don't let the user login
            dwError = LW_ERROR_PASSWORD_EXPIRED;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckUserInList(
                lsaConnection.hLsaConnection,
                pInfo->pszName,
                NULL);
    BAIL_ON_LSA_ERROR(dwError);

    // Need to ensure that home directories are created.
    dwError = LsaOpenSession(
                lsaConnection.hLsaConnection,
                pInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (ppszOutputMessage)
    {
        *ppszOutputMessage = pszMessage;
    }
    else
    {
        LW_SAFE_FREE_STRING(pszMessage);
    }
    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                pInfo);
    }
    if (pInfo2 != NULL)
    {
        LsaFreeUserInfo(
                2,
                pInfo2);
    }

    switch(dwError)
    {
        case LW_ERROR_SUCCESS:
            iError = AUTH_SUCCESS;
            break;
        case LW_ERROR_NOT_HANDLED:
        case LW_ERROR_NO_SUCH_USER:
            iError = AUTH_NOTFOUND;
            break;
        case LW_ERROR_ACCOUNT_EXPIRED:
        case LW_ERROR_ACCOUNT_DISABLED:
        case LW_ERROR_ACCOUNT_LOCKED:
            iError = AUTH_FAILURE;
            break;
        default:
            iError = AUTH_UNAVAIL;
            break;
    }
    LSA_LOG_PAM_DEBUG("Lsass LAM authenticate finishing with likewise code %u and LAM code %d",
            dwError,
            iError);
    return iError;

error:
    LsaNssCommonCloseConnection(&lsaConnection);

    goto cleanup;
}

int
LsaNssIsPasswordExpired(
        PSTR pszUser,
        PSTR* ppszMessage
        )
{
    PLSA_USER_INFO_2 pInfo = NULL;
    const DWORD dwInfoLevel = 2;
    DWORD dwError = LW_ERROR_SUCCESS;
    int iError = 0;

    LSA_LOG_PAM_DEBUG("Lsass LAM checking expired password for user [%s]",
            pszUser? pszUser: "(null)");
    *ppszMessage = NULL;

    dwError = LsaNssCommonEnsureConnected(&lsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssFindUserByAixName(
                lsaConnection.hLsaConnection,
                pszUser,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pInfo->bPasswordExpired)
    {
        dwError = LwAllocateStringPrintf(
                ppszMessage,
                "%s's password is expired",
                pszUser);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pInfo->bPromptPasswordChange &&
        pInfo->dwDaysToPasswordExpiry)
    {
        dwError = LwAllocateStringPrintf(
                ppszMessage,
                "Your password will expire in %u days\n",
                pInfo->dwDaysToPasswordExpiry);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                (PVOID)pInfo);
    }
    switch(dwError)
    {
        case LW_ERROR_SUCCESS:
            iError = 0;
            break;
        case LW_ERROR_PASSWORD_EXPIRED:
            iError = 1;
            break;
        case LW_ERROR_NO_SUCH_USER:
            iError = -1;
            errno = ENOENT;
            break;
        default:
            // password is expired and cannot login
            LsaNssMapErrorCode(dwError, &errno);
            iError = 2;
            break;
    }
    LSA_LOG_PAM_DEBUG("Lsass LAM expired password check finishing with likewise code %u and LAM code %d",
            dwError,
            iError);
    return iError;

error:
    LsaNssCommonCloseConnection(&lsaConnection);

    goto cleanup;
}

int
LsaNssChangePassword(
        PSTR pszUser,
        PSTR pszOldPass,
        PSTR pszNewPass,
        PSTR* ppszError)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pInfo = NULL;
    const DWORD dwInfoLevel = 0;

    *ppszError = NULL;

    dwError = LsaNssCommonEnsureConnected(&lsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssFindUserByAixName(
                lsaConnection.hLsaConnection,
                pszUser,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangePassword(
                lsaConnection.hLsaConnection,
                pInfo->pszName,
                pszNewPass,
                pszOldPass);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                (PVOID)pInfo);
    }
    if(dwError != LW_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &errno);
        return -1;
    }
    return 0;

error:
    LsaNssCommonCloseConnection(&lsaConnection);

    goto cleanup;
}
