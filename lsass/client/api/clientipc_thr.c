/*
 * Copyright Likewise Software
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
 *        clientipc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
