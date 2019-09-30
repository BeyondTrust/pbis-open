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
 *        lsa_cfg.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa rpc server configuration
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


DWORD
LsaSrvInitialiseConfig(
    PLSA_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(*pConfig));

    dwError = LwAllocateString(
            LSA_DEFAULT_LPC_SOCKET_PATH,
            &pConfig->pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                LSA_DEFAULT_LPC_SOCKET_PATH,
                &pConfig->pszSamrLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    pConfig->bRegisterTcpIp = FALSE;

cleanup:
    return dwError;

error:
    LsaSrvFreeConfigContents(pConfig);
    goto cleanup;
}

VOID
LsaSrvFreeConfigContents(
    PLSA_SRV_CONFIG pConfig
    )
{
    if ( pConfig )
    {
        LW_SAFE_FREE_STRING(pConfig->pszLpcSocketPath);
        LW_SAFE_FREE_STRING(pConfig->pszSamrLpcSocketPath);
    }
}


DWORD
LsaSrvReadRegistry(
    PLSA_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    LWREG_CONFIG_ITEM LsaConfig[] =
    {
        {
           "LpcSocketPath",
           FALSE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pConfig->pszLpcSocketPath,
           NULL
        },
        {
           "RegisterTcpIp",
           TRUE,
           LwRegTypeBoolean,
           0,
           MAXDWORD,
           NULL,
           &pConfig->bRegisterTcpIp,
           NULL
        },
    };
    LWREG_CONFIG_ITEM SamrConfig[] =
    {
        {
           "LpcSocketPath",
           FALSE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pConfig->pszSamrLpcSocketPath,
           NULL
        },
    };

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\RPCServers\\lsarpc",
                "Policy\\Services\\lsass\\Parameters\\RPCServers\\lsarpc",
                LsaConfig,
                sizeof(LsaConfig)/sizeof(LsaConfig[0]));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\RPCServers\\samr",
                "Policy\\Services\\lsass\\Parameters\\RPCServers\\samr",
                SamrConfig,
                sizeof(SamrConfig)/sizeof(SamrConfig[0]));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gLsaSrvConfig.pszLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gLsaSrvConfig.pszLpcSocketPath,
                                &pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszLpcSocketPath = pszLpcSocketPath;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    goto cleanup;
}


DWORD
LsaSrvConfigGetSamrLpcSocketPath(
    PSTR *ppszSamrLpcSocketPath
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gLsaSrvConfig.pszSamrLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gLsaSrvConfig.pszSamrLpcSocketPath,
                                &pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszSamrLpcSocketPath = pszLpcSocketPath;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaSrvConfigShouldRegisterTcpIp(
    BOOLEAN* pbResult
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
 
    GLOBAL_DATA_LOCK(bLocked);

    *pbResult = gLsaSrvConfig.bRegisterTcpIp;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    *pbResult = FALSE;
    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
