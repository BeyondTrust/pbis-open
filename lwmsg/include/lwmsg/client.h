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
 *        client.h
 *
 * Abstract:
 *
 *        Multi-threaded client API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_CLIENT_H__
#define __LWMSG_CLIENT_H__

/**
 * @file client.h
 * @brief Multithreaded Client API
 */

#include <lwmsg/peer.h>

typedef struct LWMsgPeer LWMsgClient;

static inline
LWMsgStatus
lwmsg_client_new(
    LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgClient** client)
{
    return lwmsg_peer_new(context, protocol, client);
}

static inline
LWMsgStatus
lwmsg_client_set_endpoint(
    LWMsgClient* client,
    LWMsgEndpointType type,
    const char* endpoint
    )
{
    return lwmsg_peer_add_connect_endpoint(client, type, endpoint);
}

static inline
LWMsgStatus
lwmsg_client_acquire_call(
    LWMsgClient* client,
    LWMsgCall** call
    )
{
    return lwmsg_peer_acquire_call(client, call);
}

static inline
LWMsgStatus
lwmsg_client_set_max_concurrent(
    LWMsgClient* client,
    unsigned int max
    )
{
    return LWMSG_STATUS_SUCCESS;
}

static inline
LWMsgStatus
lwmsg_client_shutdown(
    LWMsgClient* client
    )
{
    return lwmsg_peer_disconnect(client);
}

static inline
void
lwmsg_client_delete(
    LWMsgClient* client
    )
{
    return lwmsg_peer_delete(client);
}

#endif
