/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Client Authentication API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaAuthenticateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword,
    PSTR*  ppszMessage
    )
{
    LSA_AUTH_USER_PAM_PARAMS params = { 0 };
    PLSA_AUTH_USER_PAM_INFO pInfo = NULL;
    DWORD dwError = 0;

    params.dwFlags = LSA_AUTH_USER_PAM_FLAG_RETURN_MESSAGE;
    params.pszLoginName = pszLoginName;
    params.pszPassword = pszPassword;

    dwError = LsaAuthenticateUserPam(
            hLsaConnection,
            &params,
            &pInfo);

    if (ppszMessage)
    {
        if (pInfo)
        {
            *ppszMessage = pInfo->pszMessage;
            pInfo->pszMessage = NULL;
        }
        else
        {
            *ppszMessage = NULL;
        }
    }
    if (pInfo)
    {
        LsaFreeAuthUserPamInfo(pInfo);
    }

    return dwError;
}

LSASS_API
DWORD
LsaAuthenticateUserPam(
    HANDLE hLsaConnection,
    PLSA_AUTH_USER_PAM_PARAMS pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pParams->pszLoginName);

    dwError = LsaTransactAuthenticateUserPam(
                hLsaConnection,
                pParams,
                ppPamAuthInfo);
error:

    return dwError;
}

LSASS_API
DWORD
LsaValidateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszLoginName);

    dwError = LsaTransactValidateUser(
                hLsaConnection,
                pszLoginName,
                pszPassword);
error:

    return dwError;
}

LSASS_API
DWORD
LsaCheckUserInList(
    HANDLE   hLsaConnection,
    PCSTR    pszLoginName,
    PCSTR    pszListName
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;
    LSA_IPC_CHECK_USER_IN_LIST_REQ checkUserinListReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    checkUserinListReq.pszLoginName = pszLoginName;
    checkUserinListReq.pszListName = pszListName;

    request.tag = LSA_Q_CHECK_USER_IN_LIST;
    request.object = &checkUserinListReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_CHECK_USER_IN_LIST_SUCCESS:
            // response.object == NULL;
            break;
        case LSA_R_CHECK_USER_IN_LIST_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaChangePassword(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszOldPassword,
    PCSTR  pszNewPassword
    )
{
    return LsaTransactChangePassword(
            hLsaConnection,
            pszLoginName,
            pszOldPassword,
            pszNewPassword);
}

LSASS_API
DWORD
LsaSetPassword(
   HANDLE hLsaConnection,
   PCSTR  pszLoginName,
   PCSTR  pszNewPassword
   )
{
    return LsaTransactSetPassword(
            hLsaConnection,
            pszLoginName,
            pszNewPassword);
}

LSASS_API
DWORD
LsaAuthenticateUserEx(
	IN HANDLE hLsaConnection,
        IN PCSTR pszTargetProvider,
	IN LSA_AUTH_USER_PARAMS* pParams,
	OUT PLSA_AUTH_USER_INFO* ppUserInfo
	)
{
    return LsaTransactAuthenticateUserEx(
            hLsaConnection,
            pszTargetProvider,
            pParams,
            ppUserInfo);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
