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
 *        dssetup_srv.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        DsSetup rpc server configuration
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
DsrSrvInitialiseConfig(
    PDSSETUP_SRV_CONFIG pConfig
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
                &pConfig->pszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    pConfig->bRegisterTcpIp = FALSE;


cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pConfig->pszLpcSocketPath);
    LW_SAFE_FREE_STRING(pConfig->pszLsaLpcSocketPath);

    goto cleanup;
}


DWORD
DsrSrvReadRegistry(
    PDSSETUP_SRV_CONFIG pConfig
    )
{
    
    DWORD dwError = 0;

    PLSA_CONFIG_REG pReg = NULL;

    dwError = LsaOpenConfig(
                "Services\\lsass\\Parameters\\RPCServers\\dssetup",
                "Policy\\Services\\lsass\\Parameters\\RPCServers\\dssetup",
                &pReg);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pReg)
    {
        goto error;
    }

    dwError = LsaReadConfigString(
                pReg,
                "LpcSocketPath",
                FALSE,
                &(pConfig->pszLpcSocketPath),
                NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaReadConfigBoolean(
                pReg,
                "RegisterTcpIp",
                TRUE,
                &pConfig->bRegisterTcpIp);
    BAIL_ON_LSA_ERROR(dwError);

    LsaCloseConfig(pReg);
    pReg = NULL;


    dwError = LsaOpenConfig(
                "Services\\lsass\\Parameters\\RPCServers\\lsarpc",
                "Policy\\Services\\lsass\\Parameters\\RPCServers\\lsarpc",
                &pReg);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pReg)
    {
        goto error;
    }

    dwError = LsaReadConfigString(
                pReg,
                "LpcSocketPath",
                FALSE,
                &(pConfig->pszLsaLpcSocketPath),
                NULL);
    BAIL_ON_LSA_ERROR(dwError);

    LsaCloseConfig(pReg);
    pReg = NULL;

cleanup:
    return dwError;

error:
    LsaCloseConfig(pReg);
    pReg = NULL;

    goto cleanup;
}

DWORD
DsrSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gDsrSrvConfig.pszLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gDsrSrvConfig.pszLpcSocketPath,
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
DsrSrvConfigGetLsaLpcSocketPath(
    PSTR *ppszLsaLpcSocketPath
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gDsrSrvConfig.pszLsaLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gDsrSrvConfig.pszLsaLpcSocketPath,
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
DsrSrvConfigShouldRegisterTcpIp(
    BOOLEAN* pbResult
    )
{
    DWORD dwError = 0;
    BOOL bLocked = 0;
 
    GLOBAL_DATA_LOCK(bLocked);

    *pbResult = gDsrSrvConfig.bRegisterTcpIp;

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
