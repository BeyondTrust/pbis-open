/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
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

static
BOOLEAN
LWIIsLocalHost(
    const char * hostname
    )
{
    DWORD            dwError = 0;
    BOOLEAN          bResult = FALSE;
    char             localHost[256];
    struct addrinfo* localInfo = NULL;
    struct addrinfo* remoteInfo = NULL;
    PCSTR            pcszLocalHost = NULL;
    PCSTR            pcszRemoteHost = NULL;
    CHAR             canonNameLocal[NI_MAXHOST] = "";
    CHAR             canonNameRemote[NI_MAXHOST] = "";

    memset(localHost, 0, sizeof(localHost));

    if ( !strcasecmp(hostname, "localhost") ||
         !strcmp(hostname, "127.0.0.1") )
    {
        bResult = TRUE;
        goto cleanup;
    }

    dwError = gethostname(localHost, sizeof(localHost) - 1);
    if ( !LW_IS_NULL_OR_EMPTY_STR(localHost) )
    {
        dwError = getaddrinfo(localHost, NULL, NULL, &localInfo);
        if ( dwError )
        {
            pcszLocalHost = localHost;
        }
        else
        {
            dwError = getnameinfo(localInfo->ai_addr, localInfo->ai_addrlen, canonNameLocal, NI_MAXHOST, NULL, 0, 0);

            if(dwError || !canonNameLocal[0]) {
                pcszLocalHost = localHost;
            }
            else {
                pcszLocalHost = canonNameLocal;
            }
        }

        dwError = getaddrinfo(hostname, NULL, NULL, &remoteInfo);
        if ( dwError )
        {
            pcszRemoteHost = hostname;
        }
        else
        {
            dwError = getnameinfo(remoteInfo->ai_addr, remoteInfo->ai_addrlen, canonNameRemote, NI_MAXHOST, NULL, 0, 0);

            if(dwError || !canonNameRemote[0]) {
                pcszRemoteHost = hostname;
            }
            else {
                pcszRemoteHost = canonNameRemote;
            }
        }

        if ( !strcasecmp(pcszLocalHost, pcszRemoteHost) )
        {
            bResult = TRUE;
        }
    }

cleanup:

    if (localInfo)
    {
        freeaddrinfo(localInfo);
    }
    if (remoteInfo)
    {
        freeaddrinfo(remoteInfo);
    }

    return bResult;
}

DWORD
LWICreateEventLogRpcBinding(
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

    if (hostname == NULL || LWIIsLocalHost(hostname))
    {
        /* If no host is specified, connect to the local host over ncalrpc */
        bLocalHost = TRUE;
        protocol = "ncalrpc";
        endpoint = CACHEDIR "/.eventlog";
    }
    else
    {
        /* Connect using tcp */
        protocol = "ncacn_ip_tcp";
        endpoint = NULL;
    }

    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() hostname=%s, *event_binding=%.16X\n",
                    hostname, *event_binding);

    RPC_STRING_BINDING_COMPOSE((char*) protocol, (char*) hostname, (char*) endpoint, &pszBindingString, &winerror);
    BAIL_ON_DCE_ERROR(dwError, winerror);

    if (pszBindingString == NULL || *pszBindingString == '\0') {
        BAIL_ON_DCE_ERROR(dwError, RPC_S_INVALID_STRING_BINDING);
    }

    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() pszBindingString=%s, running rbfsb\n",
                    pszBindingString);

    RPC_BINDING_FROM_STRING_BINDING(pszBindingString, &eventBinding_local, &winerror);
    BAIL_ON_DCE_ERROR(dwError, winerror);

    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() eventBinding_local=%.16X, finished rbfsb\n",
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
        
        EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() using host principal [%s]\n",
                        hostPrincipal);
        
        winerror = RpcBindingSetAuthInfo(eventBinding_local,
                                  (unsigned char*)hostPrincipal,
                                  rpc_c_protect_level_pkt_privacy,
                                  rpc_c_authn_gss_negotiate,
                                  NULL,
                                  rpc_c_authz_name);
        BAIL_ON_DCE_ERROR(dwError, winerror);
        
        EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() eventBinding_local=%.16X, auth info set"
                        "winerror=0x%08x\n", eventBinding_local, winerror);
        
    }

    *event_binding = eventBinding_local;

    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() finished successfully\n");

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
    EVT_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() label error: winerror=%d\n",
                    winerror);

    goto cleanup;
}


DWORD
LWIFreeEventLogRpcBinding(
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
