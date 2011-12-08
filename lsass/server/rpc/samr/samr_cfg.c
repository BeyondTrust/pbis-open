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
 *        samr_cfg.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr rpc server configuration
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


DWORD
SamrSrvInitialiseConfig(
    PSAMR_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(*pConfig));

    dwError = LwAllocateString(
                SAMR_RPC_CFG_DEFAULT_LPC_SOCKET_PATH,
                &pConfig->pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                SAMR_RPC_CFG_DEFAULT_LOGIN_SHELL,
                &pConfig->pszDefaultLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                SAMR_RPC_CFG_DEFAULT_HOMEDIR_PREFIX,
                &pConfig->pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                SAMR_RPC_CFG_DEFAULT_HOMEDIR_TEMPLATE,
                &pConfig->pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    pConfig->bRegisterTcpIp = FALSE;

cleanup:    
    return dwError;

error:
    SamrSrvFreeConfigContents(pConfig);
    goto cleanup;
}

VOID
SamrSrvFreeConfigContents(
    PSAMR_SRV_CONFIG pConfig
    )
{
    if (pConfig)
    {
        LW_SAFE_FREE_STRING(pConfig->pszLpcSocketPath);
        LW_SAFE_FREE_STRING(pConfig->pszDefaultLoginShell);
        LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
        LW_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);
    }
}

DWORD
SamrSrvReadRegistry(
    PSAMR_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    LWREG_CONFIG_ITEM Config[] =
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
           "LoginShellTemplate",
           TRUE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pConfig->pszDefaultLoginShell,
           NULL
        },
        {
           "HomeDirPrefix",
           TRUE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pConfig->pszHomedirPrefix,
           NULL
        },
        {
           "HomeDirTemplate",
           TRUE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pConfig->pszHomedirTemplate,
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

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\RPCServers\\samr",
                "Policy\\Services\\lsass\\Parameters\\RPCServers\\samr",
                Config,
                sizeof(Config)/sizeof(Config[0]));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:    
    goto cleanup;
}

DWORD
SamrSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gSamrSrvConfig.pszLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gSamrSrvConfig.pszLpcSocketPath,
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
SamrSrvConfigGetDefaultLoginShell(
    PSTR *ppszDefaultLoginShell
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszDefaultLoginShell = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gSamrSrvConfig.pszDefaultLoginShell)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gSamrSrvConfig.pszDefaultLoginShell,
                               &pszDefaultLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszDefaultLoginShell = pszDefaultLoginShell;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    goto cleanup;
}

DWORD
SamrSrvConfigGetHomedirPrefix(
    PSTR *ppszHomedirPrefix
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszHomedirPrefix = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gSamrSrvConfig.pszHomedirPrefix)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gSamrSrvConfig.pszHomedirPrefix,
                               &pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszHomedirPrefix = pszHomedirPrefix;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    goto cleanup;
}


DWORD
SamrSrvConfigGetHomedirTemplate(
    PSTR *ppszHomedirTemplate
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszHomedirTemplate = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gSamrSrvConfig.pszHomedirTemplate)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gSamrSrvConfig.pszHomedirTemplate,
                                &pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszHomedirTemplate = pszHomedirTemplate;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    goto cleanup;
}

DWORD
SamrSrvConfigShouldRegisterTcpIp(
    BOOLEAN* pbResult
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;

    GLOBAL_DATA_LOCK(bLocked);

    *pbResult = gSamrSrvConfig.bRegisterTcpIp;

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
