/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        ipc-protocol.c
 *
 * Abstract:
 *
 *        IPC call support
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static LWMsgProtocol* gpProtocol = NULL;
static LWMsgPeer* gpClient = NULL;
static LWMsgSession* gpSession = NULL;
static pthread_once_t gOnce = ONCE_INIT;
static DWORD gOnceError = 0;

static
void
__LwSmIpcCallInit(
    void
    )
{
    DWORD dwError = 0;
    
    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_new(NULL, &gpProtocol));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_add_protocol_spec(
                                   gpProtocol,
                                   LwSmIpcGetProtocolSpec()));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_new(NULL, gpProtocol, &gpClient));
    BAIL_ON_ERROR(dwError);
    
    dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_connect_endpoint(
                                   gpClient, 
                                   LWMSG_CONNECTION_MODE_LOCAL,
                                   SM_ENDPOINT));
    BAIL_ON_ERROR(dwError);
    
    dwError = MAP_LWMSG_STATUS(lwmsg_peer_connect(
                                   gpClient,
                                   &gpSession));
    BAIL_ON_ERROR(dwError);

cleanup:
    
    gOnceError = dwError;

    return;
    
error:
    
    if (gpClient)
    {
        lwmsg_peer_delete(gpClient);
    }
    
    if (gpProtocol)
    {
        lwmsg_protocol_delete(gpProtocol);
    }

    goto cleanup;
}

static
VOID
__attribute__((destructor))
__LwSmIpcCallShutdown(
    VOID
    )
{
    if (gpSession)
    {
        lwmsg_peer_disconnect(gpClient);
        gpSession = NULL;
    }

    if (gpClient)
    {
        lwmsg_peer_delete(gpClient);
        gpClient = NULL;
    }

    if (gpProtocol)
    {
        lwmsg_protocol_delete(gpProtocol);
        gpProtocol = NULL;
    }
}

static
DWORD
LwSmIpcCallInit(
    VOID
    )
{
    pthread_once(&gOnce, __LwSmIpcCallInit);

    return gOnceError;
}

DWORD
LwSmIpcAcquireCall(
    LWMsgCall** ppCall
    )
{
    DWORD dwError = 0;

    dwError = LwSmIpcCallInit();
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_acquire_call(gpClient, ppCall));
    BAIL_ON_ERROR(dwError);

error:

    return dwError;
}
