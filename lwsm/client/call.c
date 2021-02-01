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
                                   (LWMsgEndpointType)LWMSG_CONNECTION_MODE_LOCAL,
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
        gpClient = NULL;
    }
    
    if (gpProtocol)
    {
        lwmsg_protocol_delete(gpProtocol);
        gpProtocol = NULL;
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
