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
 *        wkssvc_cfg.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        WksSvc rpc server configuration
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


DWORD
WkssSrvInitialiseConfig(
    PWKSS_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(*pConfig));

    dwError = LwAllocateString(
            WKSS_RPC_CFG_DEFAULT_LPC_SOCKET_PATH,
            &pConfig->pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
            WKSS_RPC_CFG_DEFAULT_LPC_SOCKET_PATH,
            &pConfig->pszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    pConfig->bRegisterTcpIp = FALSE;

cleanup:
    return dwError;

error:
    WkssSrvFreeConfigContents(pConfig);
    goto cleanup;
}

VOID
WkssSrvFreeConfigContents(
    PWKSS_SRV_CONFIG pConfig
    )
{
    if (pConfig)
    {
        LW_SAFE_FREE_STRING(pConfig->pszLpcSocketPath);
    }
}


DWORD
WkssSrvReadRegistry(
    PWKSS_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    LWREG_CONFIG_ITEM WkssConfig[] =
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
    LWREG_CONFIG_ITEM LsaConfig[] =
    {
        {
           "LpcSocketPath",
           FALSE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pConfig->pszLsaLpcSocketPath,
           NULL
        },
    };

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\RPCServers\\wkssvc",
                "Policy\\Services\\lsass\\Parameters\\RPCServers\\wkssvc",
                WkssConfig,
                sizeof(WkssConfig)/sizeof(WkssConfig[0]));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\RPCServers\\lsarpc",
                "Policy\\Services\\lsass\\Parameters\\RPCServers\\lsarpc",
                LsaConfig,
                sizeof(LsaConfig)/sizeof(LsaConfig[0]));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
WkssSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gWkssSrvConfig.pszLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gWkssSrvConfig.pszLpcSocketPath,
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
WkssSrvConfigGetLsaLpcSocketPath(
    PSTR *ppszLsaLpcSocketPath
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gWkssSrvConfig.pszLsaLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gWkssSrvConfig.pszLsaLpcSocketPath,
                               &pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszLsaLpcSocketPath = pszLpcSocketPath;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    goto cleanup;
}

DWORD
WkssSrvConfigShouldRegisterTcpIp(
    BOOLEAN* pbResult
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
 
    GLOBAL_DATA_LOCK(bLocked);

    *pbResult = gWkssSrvConfig.bRegisterTcpIp;

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
