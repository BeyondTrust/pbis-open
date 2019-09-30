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

#include "api.h"

DWORD
LsaSrvGetPamConfig(
    IN HANDLE hServer,
    OUT PLSA_PAM_CONFIG *ppPamConfig
    )
{
    DWORD dwError = 0;
    LSA_PAM_CONFIG PamConfig = { 0 };
    PLSA_PAM_CONFIG pPamConfig = NULL;
    PSTR pszSmartCardServices = NULL;
    PSTR pszSmartCardRemoteServices = NULL;
    PSTR pszSmartCardPromptGecos = NULL;
    DWORD dwSmartCardServicesSize = 0;
    DWORD dwSmartCardRemoteServicesSize = 0;
    DWORD dwSmartCardPromptGecosSize = 0;
    DWORD dwCount;

    const PCSTR LogLevels[] = 
    {
        "disabled",
        "always",
        "error",
        "warning",
        "info",
        "verbose",
        "debug"
    };

    LWREG_CONFIG_ITEM ConfigDescription[] =
    {
        {
            "LogLevel",
            TRUE,
            LwRegTypeEnum,
            LSA_PAM_LOG_LEVEL_DISABLED,
            LSA_PAM_LOG_LEVEL_DEBUG,
            LogLevels,
            &PamConfig.dwLogLevel,
            NULL
        },
        {
            "DisplayMOTD",
            TRUE,
            LwRegTypeBoolean,
            0,
            0,
            NULL,
            &PamConfig.bLsaPamDisplayMOTD,
            NULL
        },
        {
            "UserNotAllowedError",
            TRUE,
            LwRegTypeString,
            0,
            0,
            NULL,
            &PamConfig.pszAccessDeniedMessage,
            NULL
        },
        {
            "SmartCardServices",
            TRUE,
            LwRegTypeMultiString,
            0,
            0,
            NULL,
            &pszSmartCardServices,
            &dwSmartCardServicesSize
        },
        {
            "SmartCardRemoteServices",
            TRUE,
            LwRegTypeMultiString,
            0,
            0,
            NULL,
            &pszSmartCardRemoteServices,
            &dwSmartCardRemoteServicesSize
        },
        {
            "SmartCardPromptGecos",
            TRUE,
            LwRegTypeMultiString,
            0,
            0,
            NULL,
            &pszSmartCardPromptGecos,
            &dwSmartCardPromptGecosSize
        },
        {
            "ActiveDirectoryPasswordPrompt",
            TRUE,
            LwRegTypeString,
            0,
            0,
            NULL,
            &PamConfig.pszActiveDirectoryPasswordPrompt,
            NULL
        },
        {
            
            "LocalPasswordPrompt",
            TRUE,
            LwRegTypeString,
            0,
            0,
            NULL,
            &PamConfig.pszLocalPasswordPrompt,
            NULL                                                            
        },
        {
            
            "OtherPasswordPrompt",
            TRUE,
            LwRegTypeString,
            0,
            0,
            NULL,
            &PamConfig.pszOtherPasswordPrompt,
            NULL                                                            
        },
        {
            "NssApplyAccessControl",
            TRUE,
            LwRegTypeBoolean,
            0,
            0,
            NULL,
            &PamConfig.bNssApplyAccessControl,
            NULL
        },
        
    };

    dwError = LwAllocateMemory(
                sizeof(LSA_PAM_CONFIG),
                OUT_PPVOID(&pPamConfig));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUtilInitializePamConfig(&PamConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\PAM",
                "Policy\\Services\\lsass\\Parameters\\PAM",
                ConfigDescription,
                sizeof(ConfigDescription)/sizeof(ConfigDescription[0]));
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaSrvSmartCardEnabled()) {
        dwError = RegByteArrayToMultiStrsA(
                (PBYTE) pszSmartCardServices,
                dwSmartCardServicesSize,
                &PamConfig.ppszSmartCardServices);
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = 0;
        while (PamConfig.ppszSmartCardServices[dwCount] != NULL)
        {
             ++dwCount;
        }
        PamConfig.dwNumSmartCardServices = dwCount;
    }

    if (LsaSrvRemoteSmartCardEnabled()) {
        dwError = RegByteArrayToMultiStrsA(
                (PBYTE) pszSmartCardRemoteServices,
                dwSmartCardRemoteServicesSize,
                &PamConfig.ppszSmartCardRemoteServices);
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = 0;
        while (PamConfig.ppszSmartCardRemoteServices[dwCount] != NULL)
        {
             ++dwCount;
        }
        PamConfig.dwNumSmartCardRemoteServices = dwCount;
    }

    dwError = RegByteArrayToMultiStrsA(
            (PBYTE) pszSmartCardPromptGecos,
            dwSmartCardPromptGecosSize,
            &PamConfig.ppszSmartCardPromptGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = 0;
    while (PamConfig.ppszSmartCardPromptGecos[dwCount] != NULL)
    {
         ++dwCount;
    }
    PamConfig.dwNumSmartCardPromptGecos = dwCount;

    *pPamConfig = PamConfig;
    memset(&PamConfig, 0, sizeof(LSA_PAM_CONFIG));

cleanup:
    LW_SAFE_FREE_STRING(pszSmartCardServices);
    LW_SAFE_FREE_STRING(pszSmartCardRemoteServices);
    LW_SAFE_FREE_STRING(pszSmartCardPromptGecos);

    *ppPamConfig = pPamConfig;

    return dwError;

error:
    if (pPamConfig)
    {
        LsaUtilFreePamConfigContents(pPamConfig);
        LW_SAFE_FREE_MEMORY(pPamConfig);
    }
    LsaUtilFreePamConfigContents(&PamConfig);

    goto cleanup;
}
