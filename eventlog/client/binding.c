/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) BeyondTrust Corporation 2004-2007
 * Copyright (C) BeyondTrust Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 * Eventlog Client RPC Binding
 *
 */
#include "includes.h"

DWORD
LwEvtCreateEventlogRpcBinding(
    const char * hostname,
    handle_t *   event_binding
    )
{
    DWORD winerror = 0;
    DWORD dwError = 0;
    const char * protocol;
    const char * endpoint;
    char * pszBindingString = NULL;
    char *hostPrincipal = NULL;
    size_t hostPrincipalSize = 0;
    int ret = 0;
    handle_t eventBinding_local = 0;
    BOOLEAN bLocalHost = FALSE;

    /* Connect using tcp */
    protocol = "ncacn_ip_tcp";
    endpoint = NULL;

    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventlogRpcBinding() hostname=%s, *event_binding=%.16X\n",
                    hostname, *event_binding);

    RPC_STRING_BINDING_COMPOSE((char*) protocol, (char*) hostname, (char*) endpoint, &pszBindingString, &winerror);
    BAIL_ON_DCE_ERROR(dwError, winerror);

    if (pszBindingString == NULL || *pszBindingString == '\0') {
        BAIL_ON_DCE_ERROR(dwError, RPC_S_INVALID_STRING_BINDING);
    }

    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventlogRpcBinding() pszBindingString=%s, running rbfsb\n",
                    pszBindingString);

    RPC_BINDING_FROM_STRING_BINDING(pszBindingString, &eventBinding_local, &winerror);
    BAIL_ON_DCE_ERROR(dwError, winerror);

    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventlogRpcBinding() eventBinding_local=%.16X, finished rbfsb\n",
                    eventBinding_local);


    if (hostname != NULL && !bLocalHost)
    {
        /* Set up authentication if we are connecting to a remote host */
        hostPrincipalSize = strlen(hostname) + 6;
        
        dwError = LwAllocateMemory(hostPrincipalSize, (PVOID*)&hostPrincipal);
        BAIL_ON_EVT_ERROR(dwError);
        
        ret = snprintf(hostPrincipal, hostPrincipalSize, "host/%s", hostname);
        if (ret < 0 || ret >= hostPrincipalSize) {
            BAIL_ON_EVT_ERROR(ERROR_INSUFFICIENT_BUFFER);
        }
        
        EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventlogRpcBinding() using host principal [%s]\n",
                        hostPrincipal);
        
        winerror = RpcBindingSetAuthInfo(eventBinding_local,
                                  (unsigned char*)hostPrincipal,
                                  rpc_c_protect_level_pkt_privacy,
                                  rpc_c_authn_gss_negotiate,
                                  NULL,
                                  rpc_c_authz_name);
        BAIL_ON_DCE_ERROR(dwError, winerror);
        
        EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventlogRpcBinding() eventBinding_local=%.16X, auth info set"
                        "winerror=0x%08x\n", eventBinding_local, winerror);
        
    }

    *event_binding = eventBinding_local;

    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventlogRpcBinding() finished successfully\n");

cleanup:
    if (hostPrincipal)
    {
        LwFreeMemory(hostPrincipal);
    }

    if (pszBindingString)
    {
        DWORD tempstatus = 0;
        RPC_STRING_FREE(&pszBindingString, &tempstatus);
    }

    return dwError;

error:
    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventlogRpcBinding() label error: winerror=%d\n",
                    winerror);

    goto cleanup;
}


DWORD
LwEvtFreeEventlogRpcBinding(
    handle_t event_binding
    )
{
    DWORD rpcstatus = 0;

    /* Free the binding */
    if (event_binding != NULL) {
        RPC_BINDING_FREE(&event_binding, &rpcstatus);
    }

    return rpcstatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
