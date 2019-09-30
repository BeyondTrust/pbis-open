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
 *        session.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Login Session API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaOpenSession(
    HANDLE hLsaConnection,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszLoginId);

    request.tag = LSA_Q_OPEN_SESSION;
    request.object = (PVOID)pszLoginId;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_OPEN_SESSION_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_OPEN_SESSION_FAILURE:
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
LsaCloseSession(
    HANDLE hLsaConnection,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    request.tag = LSA_Q_CLOSE_SESSION;
    request.object = (PVOID)pszLoginId;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_CLOSE_SESSION_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_CLOSE_SESSION_FAILURE:
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
