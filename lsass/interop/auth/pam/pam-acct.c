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
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-acct.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
        LSA_LOG_PAM_WARNING("Bypassing lsass for ignore user %s", pszLoginId);
        dwError = LW_ERROR_IGNORE_THIS_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);


    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckUserInList(
                    hLsaConnection,
                    pszLoginId,
                    NULL);
    if (dwError && dwError != LW_ERROR_NO_SUCH_USER)
    {
        if (dwError == LW_ERROR_NO_SUCH_USER)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
        LSA_LOG_PAM_ERROR("User %s is denied access because they are not in the 'require membership of' list",
                          LSA_SAFE_LOG_STRING(pszLoginId));
        if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszAccessDeniedMessage))
        {
            LsaPamConverse(pamh,
                           pConfig->pszAccessDeniedMessage,
                           PAM_TEXT_INFO,
                           NULL);
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaValidateUser(
                    hLsaConnection,
                    pszLoginId,
                    NULL);
    if (dwError == LW_ERROR_PASSWORD_EXPIRED)
    {
        dwError = 0;
        pPamContext->bPasswordExpired = TRUE;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pPamContext->bPasswordExpired)
    {
        // If during pam_sm_authenticate,
        // we detected that the password expired,
        // we handle it here
        if (!pPamContext->bPasswordMessageShown)
        {
            LsaPamConverse(
                pamh,
                "Your password has expired",
                PAM_ERROR_MSG,
                NULL);
            pPamContext->bPasswordMessageShown = TRUE;
        }
        dwError = LW_ERROR_PASSWORD_EXPIRED;
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

    if (dwError == LW_ERROR_NO_SUCH_USER || dwError == LW_ERROR_NOT_HANDLED ||
        dwError == LW_ERROR_IGNORE_THIS_USER )
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
