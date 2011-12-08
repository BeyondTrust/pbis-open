/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
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
    PSTR pszSmartCardPromptGecos = NULL;
    DWORD dwSmartCardServicesSize = 0;
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
            "SmartCardPromptGecos",
            TRUE,
            LwRegTypeMultiString,
            0,
            0,
            NULL,
            &pszSmartCardPromptGecos,
            &dwSmartCardPromptGecosSize
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
