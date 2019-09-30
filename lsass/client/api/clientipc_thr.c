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
 *        clientipc.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include "client.h"

typedef struct _LSA_IPC_CLIENT
{
    LWMsgContext* pContext;
    LWMsgProtocol* pProtocol;
    LWMsgPeer* pPeer;
    LWMsgSession* pSession;
} LSA_IPC_CLIENT, *PLSA_IPC_CLIENT;

PLSA_IPC_CLIENT gpIpcClient = NULL;

static
VOID
LsaIpcClientDestroy(
    PLSA_IPC_CLIENT pClient
    )
{
    if (pClient)
    {
        if (pClient->pSession)
        {
            lwmsg_peer_disconnect(pClient->pPeer);
        }

        if (pClient->pPeer)
        {
            lwmsg_peer_delete(pClient->pPeer);
        }

        if (pClient->pProtocol)
        {
            lwmsg_protocol_delete(pClient->pProtocol);
        }

        if (pClient->pContext)
        {
            lwmsg_context_delete(pClient->pContext);
        }

        LwFreeMemory(pClient);
    }
}

static
DWORD
LsaIpcClientCreate(
    PLSA_IPC_CLIENT* ppClient
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CLIENT pClient = NULL;
    static LWMsgTime connectTimeout = {10, 0};

    dwError = LwAllocateMemory(sizeof(*pClient), OUT_PPVOID(&pClient));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pClient->pContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(pClient->pContext, &pClient->pProtocol));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(
        pClient->pProtocol,
        LsaIPCGetProtocolSpec()));

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_new(
        pClient->pContext,
        pClient->pProtocol,
        &pClient->pPeer));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_connect_endpoint(
        pClient->pPeer,
        LWMSG_ENDPOINT_DIRECT,
        "lsass"));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_connect_endpoint(
        pClient->pPeer,
        LWMSG_ENDPOINT_LOCAL,
        CACHEDIR "/" LSA_SERVER_FILENAME));
    BAIL_ON_LSA_ERROR(dwError);

    if (getenv("LW_DISABLE_CONNECT_TIMEOUT") == NULL)
    {
        /* Give up connecting within 2 seconds */
        dwError = MAP_LWMSG_ERROR(lwmsg_peer_set_timeout(
                                      pClient->pPeer,
                                      LWMSG_TIMEOUT_ESTABLISH,
                                      &connectTimeout));
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_connect(pClient->pPeer, &pClient->pSession));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    *ppClient = pClient;

    return dwError;

error:

    LsaIpcClientDestroy(pClient);
    pClient = NULL;

    goto cleanup;
}

static
DWORD
LsaIpcClientInit(
    VOID
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CLIENT pClient = NULL;

    if (gpIpcClient == NULL)
    {
        dwError = LsaIpcClientCreate(&pClient);
        BAIL_ON_LSA_ERROR(dwError);

        if (LwInterlockedCompareExchangePointer(OUT_PPVOID(&gpIpcClient), pClient, NULL) == NULL)
        {
            pClient = NULL;
        }
    }

error:

    if (pClient)
    {
        LsaIpcClientDestroy(pClient);
    }

    return dwError;
}

static
VOID
__attribute__((destructor))
LsaIpcClientShutdown(
    VOID
    )
{
    if (gpIpcClient)
    {
        LsaIpcClientDestroy(gpIpcClient);
        gpIpcClient = NULL;
    }
}

DWORD
LsaOpenServerThreaded(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext = NULL;

    BAIL_ON_INVALID_POINTER(phConnection);

    dwError = LsaIpcClientInit();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(LSA_CLIENT_CONNECTION_CONTEXT), OUT_PPVOID(&pContext));
    BAIL_ON_LSA_ERROR(dwError);

    pContext->pSession = gpIpcClient->pSession;

    *phConnection = pContext;

error:

    return dwError;
}
