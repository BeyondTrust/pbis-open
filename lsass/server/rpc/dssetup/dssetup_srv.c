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
 *        dssetup_srv.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        DsSetup server management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
LsaInitializeRpcSrv(
    PSTR* ppszRpcSrvName,
    PLSA_RPCSRV_FUNCTION_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;

    pthread_mutex_init(&gDsrSrvDataMutex, NULL);

    dwError = RpcSvcRegisterRpcInterface(dssetup_v0_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszRpcSrvName = (PSTR)gpszDsrRpcSrvName;
    *ppFnTable      = &gDsrRpcFuncTable;

    dwError = DsrSrvInitialiseConfig(&gDsrSrvConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DsrSrvReadRegistry(&gDsrSrvConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DsrSrvInitServerSecurityDescriptor(&gpDsrSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    bDsrSrvInitialised = TRUE;

error:
    return dwError;
}


DWORD
LsaShutdownRpcSrv(
    PCSTR pszProviderName,
    PLSA_RPCSRV_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;

    dwError = RpcSvcUnregisterRpcInterface(dssetup_v0_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DsrSrvDestroyServerSecurityDescriptor(&gpDsrSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_destroy(&gDsrSrvDataMutex);

    bDsrSrvInitialised = FALSE;

error:
    return dwError;
}


DWORD
DsrRpcStartServer(
    void
    )
{
    PCSTR pszDescription = "Directory Services Setup";
    
    ENDPOINT EndPoints[] = {
        { NULL,            NULL },
        { NULL,            NULL },
    };
    DWORD dwError = 0;
    BOOLEAN bRegisterTcpIp = FALSE;

    dwError = DsrSrvConfigShouldRegisterTcpIp(&bRegisterTcpIp);
    BAIL_ON_LSA_ERROR(dwError);
    if (bRegisterTcpIp)
    {
        EndPoints[0].pszProtocol = "ncacn_ip_tcp";
    }

    dwError = RpcSvcBindRpcInterface(&gpDsrSrvBinding,
                                     dssetup_v0_0_s_ifspec,
                                     EndPoints,
                                     pszDescription);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}


DWORD
DsrRpcStopServer(
    void
    )
{
    DWORD dwError = 0;

    dwError = RpcSvcUnbindRpcInterface(&gpDsrSrvBinding,
                                       dssetup_v0_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
