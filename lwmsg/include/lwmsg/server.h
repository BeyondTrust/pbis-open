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
 *        server.h
 *
 * Abstract:
 *
 *        Multi-threaded server API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_SERVER_H__
#define __LWMSG_SERVER_H__

#include <lwmsg/peer.h>

/**
 * @file server.h
 * @brief Server API
 */

#define LWMSG_SERVER_MODE_LOCAL LWMSG_ENDPOINT_LOCAL

typedef struct LWMsgPeer LWMsgServer;
typedef enum LWMsgEndpointType LWMsgServerMode;
typedef LWMsgPeerCallFunction LWMsgServerCallFunction;
typedef LWMsgPeerExceptionFunction LWMsgServerExceptionFunction;

static inline
LWMsgStatus
lwmsg_server_new(
    LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgServer** server
    )
{
    return lwmsg_peer_new(context, protocol, server);
}

static inline
void
lwmsg_server_delete(
    LWMsgServer* server
    )
{
    lwmsg_peer_delete(server);
}

static inline
LWMsgStatus
lwmsg_server_set_timeout(
    LWMsgServer* server,
    LWMsgTimeout type,
    LWMsgTime* value
    )
{
    return lwmsg_peer_set_timeout(server, type, value);
}

static inline
LWMsgStatus
lwmsg_server_set_max_clients(
    LWMsgServer* server,
    unsigned int max_clients
    )
{
    return lwmsg_peer_set_max_listen_clients(server, max_clients);
}

static inline
LWMsgStatus
lwmsg_server_set_max_dispatch(
    LWMsgServer* server,
    unsigned int max_dispatch
    )
{
    return LWMSG_STATUS_SUCCESS;
}

static inline
LWMsgStatus
lwmsg_server_set_max_io(
    LWMsgServer* server,
    unsigned int max_io
    )
{
    return LWMSG_STATUS_SUCCESS;
}

static inline
LWMsgStatus
lwmsg_server_set_max_backlog(
    LWMsgServer* server,
    int max_backlog)
{
    return lwmsg_peer_set_max_listen_backlog(server, max_backlog);
}

static inline
LWMsgStatus
lwmsg_server_add_dispatch_spec(
    LWMsgServer* server,
    LWMsgDispatchSpec* spec
    )
{
    return lwmsg_peer_add_dispatch_spec(server, spec);
}

static inline
LWMsgStatus
lwmsg_server_set_fd(
    LWMsgServer* server,
    LWMsgEndpointType type,
    int fd
    )
{
    return lwmsg_peer_add_listen_fd(server, type, fd);
}

static inline
LWMsgStatus
lwmsg_server_set_endpoint(
    LWMsgServer* server,
    LWMsgEndpointType type,
    const char* endpoint,
    mode_t permissions
    )
{
    return lwmsg_peer_add_listen_endpoint(server, type, endpoint, permissions);
}

static inline
LWMsgStatus
lwmsg_server_set_session_functions(
    LWMsgServer* server,
    LWMsgSessionConstructFunction construct,
    LWMsgSessionDestructFunction destruct,
    void* data
    )
{
    return lwmsg_peer_set_listen_session_functions(server, construct, destruct, data);
}

static inline
void
lwmsg_server_set_dispatch_data(
    LWMsgServer* server,
    void* data
    )
{
    lwmsg_peer_set_dispatch_data(server, data);
}

static inline
void*
lwmsg_server_get_dispatch_data(
    LWMsgServer* server
    )
{
    return lwmsg_peer_get_dispatch_data(server);
}

static inline
LWMsgStatus
lwmsg_server_start(
    LWMsgServer* server
    )
{
    return lwmsg_peer_start_listen(server);
}

static inline
LWMsgStatus
lwmsg_server_stop(
    LWMsgServer* server
    )
{
    return lwmsg_peer_stop_listen(server);
}

static inline
LWMsgStatus
lwmsg_server_set_exception_function(
    LWMsgServer* server,
    LWMsgServerExceptionFunction except,
    void* except_data
    )
{
    return lwmsg_peer_set_exception_function(server, except, except_data);
}

#endif
