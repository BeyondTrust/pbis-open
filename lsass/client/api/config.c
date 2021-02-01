/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        config.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Configuration API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaSetMachineSid(
    HANDLE hLsaConnection,
    PCSTR pszSid
    )
{
    DWORD dwError = 0;
    size_t sSidLen = 0;

    BAIL_ON_INVALID_POINTER(pszSid);

    sSidLen = strlen(pszSid);
    if (sSidLen == 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderIoControl(hLsaConnection,
                                   LSA_PROVIDER_TAG_LOCAL,
                                   LSA_LOCAL_IO_SETDOMAINSID,
                                   (DWORD)(sSidLen + 1),
                                   (PVOID)pszSid,
                                   NULL,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

LSASS_API
DWORD
LsaSetMachineName(
    HANDLE hLsaConnection,
    PCSTR pszMachineName
    )
{
    DWORD dwError = 0;
    size_t sNameLen = 0;

    BAIL_ON_INVALID_POINTER(pszMachineName);

    sNameLen = strlen(pszMachineName);
    if (sNameLen == 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderIoControl(hLsaConnection,
                                   LSA_PROVIDER_TAG_LOCAL,
                                   LSA_LOCAL_IO_SETDOMAINNAME,
                                   (DWORD)(sNameLen + 1),
                                   (PVOID)pszMachineName,
                                   NULL,
                                   NULL);

error:
    return dwError;
}


LSASS_API
DWORD
LsaGetPamConfig(
    IN HANDLE hLsaConnection,
    OUT PLSA_PAM_CONFIG *ppPamConfig
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    request.tag = LSA_Q_GET_PAM_CONFIG;
    request.object = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_GET_PAM_CONFIG_SUCCESS:
            *ppPamConfig = (PLSA_PAM_CONFIG) response.object;
            break;
        case LSA_R_GET_PAM_CONFIG_FAILURE:
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
VOID
LsaFreePamConfig(
    IN PLSA_PAM_CONFIG pConfig
    )
{
    if (pConfig)
    {
        LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
        LW_SAFE_FREE_STRING(pConfig->pszActiveDirectoryPasswordPrompt);
        LW_SAFE_FREE_STRING(pConfig->pszLocalPasswordPrompt);
        LW_SAFE_FREE_STRING(pConfig->pszOtherPasswordPrompt);
        LW_SAFE_FREE_MEMORY(pConfig);
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
