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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-acct.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module
 *
 *        Account Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

/*
 * This is where we check if the password expired.
 * If the password is correct, but has expired, we return
 * PAM_NEW_AUTHTOK_REQD instead of PAM_SUCCESS
 */
int
pam_sm_acct_mgmt(
	pam_handle_t* pamh,
	int           flags,
	int           argc,
	const char**  argv
	)
{
    DWORD dwError = 0;
    PPAMCONTEXT pPamContext = NULL;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 2;
    PSTR  pszLoginId = NULL;
    PLSA_PAM_CONFIG pConfig = NULL;
    int iPamError = 0;
    PSTR pszExpireDone;

    LSA_LOG_PAM_DEBUG("pam_sm_acct_mgmt::begin");

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);

    dwError = LsaPamGetContext(
                    pamh,
                    flags,
                    argc,
                    argv,
                    &pPamContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPamGetLoginId(
                    pamh,
                    pPamContext,
                    &pszLoginId,
                    TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaShouldIgnoreUser(pszLoginId))
    {
        LSA_LOG_PAM_DEBUG("By passing lsassd for local account");
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckUserInList(
                    hLsaConnection,
                    pszLoginId,
                    NULL);
    if (dwError)
    {
        LSA_LOG_PAM_ERROR("User %s is denied access because they are not in the 'require membership of' list",
                          LSA_SAFE_LOG_STRING(pszLoginId));
        if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszAccessDeniedMessage))
        {
            LsaPamConverse(pamh,
                           pConfig->pszAccessDeniedMessage,
                           PAM_TEXT_INFO,
                           NULL);
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaValidateUser(
                    hLsaConnection,
                    pszLoginId,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    if (pPamContext->bPasswordExpired)
    {
        // If during pam_sm_authenticate,
        // we detected that the password expired,
        // we handle it here
        pPamContext->bPasswordExpired = FALSE;
        dwError = LW_ERROR_PASSWORD_EXPIRED;
        LsaPamConverse(
            pamh,
            "Your password has expired",
            PAM_ERROR_MSG,
            NULL);
        BAIL_ON_LSA_ERROR(dwError);
    }

    iPamError = pam_get_data(
        pamh,
        PAM_LSASS_EXPIRE_WARNING_DONE,
        (PAM_GET_DATA_TYPE)&pszExpireDone);
    if (iPamError == PAM_NO_MODULE_DATA)
    {
        dwError = LsaFindUserByName(
                        hLsaConnection,
                        pszLoginId,
                        dwUserInfoLevel,
                        (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);

        if (pUserInfo->bPromptPasswordChange == TRUE &&
            pUserInfo->bPasswordExpired == FALSE && 
            pUserInfo->bPasswordNeverExpires == FALSE) {

            CHAR szMessage[512];

            switch (pUserInfo->dwDaysToPasswordExpiry)
            {
                case 0:
                    sprintf(szMessage, "Your password will expire today\n");
                    break;
                case 1:
                    sprintf(szMessage, "Your password will expire in 1 day\n");
                    break;
                default:
                    sprintf(szMessage, "Your password will expire in %u days\n",
                           pUserInfo->dwDaysToPasswordExpiry);
                    break;
            }
            LsaPamConverse(pamh, szMessage, PAM_TEXT_INFO, NULL);
        }

        dwError = LsaPamSetDataString(
            pamh,
            PAM_LSASS_EXPIRE_WARNING_DONE,
            "TRUE");
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    LW_SAFE_FREE_STRING(pszLoginId);

    LSA_LOG_PAM_DEBUG("pam_sm_acct_mgmt::end");

    return LsaPamOpenPamFilterAcctMgmt(
                                LsaPamMapErrorCode(dwError, pPamContext));

error:

    if (dwError == LW_ERROR_NO_SUCH_USER || dwError == LW_ERROR_NOT_HANDLED)
    {
        LSA_LOG_PAM_WARNING("pam_sm_acct_mgmt failed [login:%s][error code:%u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("pam_sm_acct_mgmt failed [login:%s][error code:%u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
        if (pszLoginId && !strcmp(pszLoginId, "root"))
        {
            dwError = LW_ERROR_NO_SUCH_USER;
            LSA_LOG_PAM_ERROR("Converting error to %u for root", dwError);
        }
    }

    goto cleanup;
}
