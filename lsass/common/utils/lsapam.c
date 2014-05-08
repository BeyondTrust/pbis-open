/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
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

        for (i = 0; i < pConfig->dwNumSmartCardPromptGecos; ++i)
        {
            LW_SAFE_FREE_STRING(pConfig->ppszSmartCardPromptGecos[i]);
        }
        LW_SAFE_FREE_MEMORY(pConfig->ppszSmartCardPromptGecos);

        memset(pConfig, 0, sizeof(LSA_PAM_CONFIG));
    }
}

