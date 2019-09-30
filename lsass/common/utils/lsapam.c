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

#include "includes.h"

#define LSA_PAM_LOGON_RIGHTS_DENIED_MESSAGE "Access denied"
#define LSA_PAM_ACTIVE_DIRECTORY_PASSWORD_PROMPT "Active Directory Password: "
#define LSA_PAM_LOCAL_PASSWORD_PROMPT "Unix Password: "
#define LSA_PAM_OTHER_PASSWORD_PROMPT "Other Password: "

DWORD
LsaUtilAllocatePamConfig(
    OUT PLSA_PAM_CONFIG *ppConfig
    )
{
    DWORD dwError = 0;
    PLSA_PAM_CONFIG pConfig = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LSA_PAM_CONFIG),
                    OUT_PPVOID(&pConfig));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUtilInitializePamConfig(pConfig);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppConfig = pConfig;

    return dwError;

error:
    if (pConfig)
    {
        LsaUtilFreePamConfig(pConfig);
        pConfig = NULL;
    }

    goto cleanup;
}

DWORD
LsaUtilInitializePamConfig(
    OUT PLSA_PAM_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(LSA_PAM_CONFIG));

    pConfig->bLsaPamDisplayMOTD = FALSE;
    pConfig->dwLogLevel = LSA_PAM_LOG_LEVEL_ERROR;

    dwError = LwAllocateString(
                    LSA_PAM_LOGON_RIGHTS_DENIED_MESSAGE,
                    &pConfig->pszAccessDeniedMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwAllocateString(
                    LSA_PAM_ACTIVE_DIRECTORY_PASSWORD_PROMPT,
                    &pConfig->pszActiveDirectoryPasswordPrompt);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwAllocateString(
                    LSA_PAM_LOCAL_PASSWORD_PROMPT,
                    &pConfig->pszLocalPasswordPrompt);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwAllocateString(
                    LSA_PAM_OTHER_PASSWORD_PROMPT,
                    &pConfig->pszOtherPasswordPrompt);

    pConfig->bNssApplyAccessControl = FALSE;

cleanup:

    return dwError;

error:

    LsaUtilFreePamConfigContents(pConfig);

    goto cleanup;
}

VOID
LsaUtilFreePamConfig(
    IN PLSA_PAM_CONFIG pConfig
    )
{
    LsaUtilFreePamConfigContents(pConfig);
    LW_SAFE_FREE_MEMORY(pConfig);
}

VOID
LsaUtilFreePamConfigContents(
    IN PLSA_PAM_CONFIG pConfig
    )
{
    if (pConfig)
    {
        DWORD i;

        LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
        LW_SAFE_FREE_STRING(pConfig->pszActiveDirectoryPasswordPrompt);
        LW_SAFE_FREE_STRING(pConfig->pszLocalPasswordPrompt);
        LW_SAFE_FREE_STRING(pConfig->pszOtherPasswordPrompt);

        for (i = 0; i < pConfig->dwNumSmartCardServices; ++i)
        {
            LW_SAFE_FREE_STRING(pConfig->ppszSmartCardServices[i]);
        }
        LW_SAFE_FREE_MEMORY(pConfig->ppszSmartCardServices);

        for (i = 0; i < pConfig->dwNumSmartCardRemoteServices; ++i)
        {
            LW_SAFE_FREE_STRING(pConfig->ppszSmartCardRemoteServices[i]);
        }
        LW_SAFE_FREE_MEMORY(pConfig->ppszSmartCardRemoteServices);

        for (i = 0; i < pConfig->dwNumSmartCardPromptGecos; ++i)
        {
            LW_SAFE_FREE_STRING(pConfig->ppszSmartCardPromptGecos[i]);
        }
        LW_SAFE_FREE_MEMORY(pConfig->ppszSmartCardPromptGecos);

        memset(pConfig, 0, sizeof(LSA_PAM_CONFIG));
    }
}

