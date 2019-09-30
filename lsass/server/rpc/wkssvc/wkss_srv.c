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
 *        wkssvc_srv.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        WksSvc server management functions
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
    DWORD dwError = ERROR_SUCCESS;

    pthread_mutex_init(&gWkssSrvDataMutex, NULL);

    dwError = RpcSvcRegisterRpcInterface(wkssvc_v1_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszRpcSrvName = (PSTR)gpszWkssRpcSrvName;
    *ppFnTable      = &gWkssRpcFuncTable;

    dwError = WkssSrvInitialiseConfig(&gWkssSrvConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = WkssSrvReadRegistry(&gWkssSrvConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = WkssSrvInitServerSecurityDescriptor(&gpWkssSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    bWkssSrvInitialised = TRUE;

error:
    return dwError;
}


DWORD
LsaShutdownRpcSrv(
    PCSTR pszProviderName,
    PLSA_RPCSRV_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = RpcSvcUnregisterRpcInterface(wkssvc_v1_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = WkssSrvDestroyServerSecurityDescriptor(&gpWkssSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_destroy(&gWkssSrvDataMutex);

    bWkssSrvInitialised = FALSE;

error:
    return dwError;
}


DWORD
WkssRpcStartServer(
    void
    )
{
    PCSTR pszDescription = "Workstation Service";
    
    ENDPOINT EndPoints[] = {
        { "ncalrpc",       NULL },  /* endpoint is fetched from config parameter */
        { NULL,  NULL },
        { NULL,            NULL }
    };
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszLpcSocketPath = NULL;
    BOOLEAN bRegisterTcpIp = FALSE;

    dwError = WkssSrvConfigGetLpcSocketPath(&pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    while (EndPoints[i].pszProtocol)
    {
        if (strcmp(EndPoints[i].pszProtocol, "ncalrpc") == 0 &&
            pszLpcSocketPath)
        {
            EndPoints[i].pszEndpoint = pszLpcSocketPath;
        }

        i++;
    }

    dwError = WkssSrvConfigShouldRegisterTcpIp(&bRegisterTcpIp);
    BAIL_ON_LSA_ERROR(dwError);
    if (bRegisterTcpIp)
    {
        EndPoints[i++].pszProtocol = "ncacn_ip_tcp";
    }

    dwError = RpcSvcBindRpcInterface(&gpWkssSrvBinding,
                                     wkssvc_v1_0_s_ifspec,
                                     EndPoints,
                                     pszDescription);
    BAIL_ON_LSA_ERROR(dwError);

error:
    LW_SAFE_FREE_STRING(pszLpcSocketPath);

    return dwError;
}


DWORD
WkssRpcStopServer(
    void
    )
{
    DWORD dwError = 0;

    dwError = RpcSvcUnbindRpcInterface(&gpWkssSrvBinding,
                                       wkssvc_v1_0_s_ifspec);
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
