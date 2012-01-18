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
 *        connection.c
 *
 * Abstract:
 *
 *        Connection API
 *        Primary entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include "assoc-private.h"
#include "connection-private.h"
#include "util-private.h"
#include "convert-private.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

static LWMsgStatus
lwmsg_connection_construct(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->sendbuffer));
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->recvbuffer));

    priv->packet_size = 8192;
    priv->fd = -1;
    priv->mode = LWMSG_CONNECTION_MODE_NONE;
    
    memset(&priv->timeout.establish, 0xFF, sizeof(priv->timeout.establish));
    memset(&priv->timeout.message, 0xFF, sizeof(priv->timeout.message));

error:

    return status;
}

static void
lwmsg_connection_destruct(
    LWMsgAssoc* assoc
    )
{
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    lwmsg_connection_buffer_destruct(&priv->sendbuffer);
    lwmsg_connection_buffer_destruct(&priv->recvbuffer);

    if (priv->fd != -1)
    {
        close(priv->fd);
        priv->fd = -1;
    }

    if (priv->endpoint)
    {
        free(priv->endpoint);
        priv->endpoint = NULL;
    }

    if (priv->active_session)
    {
        lwmsg_session_release(priv->active_session);
    }

    if (priv->default_session)
    {
        lwmsg_session_release(priv->default_session);
    }

    if (priv->marshal_context)
    {
        lwmsg_data_context_delete(priv->marshal_context);
    }
}


static LWMsgStatus
lwmsg_connection_close(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_EVENT_CLOSE));

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_reset(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_EVENT_RESET));

error:
    
    return status;
}

static LWMsgStatus
lwmsg_connection_send_msg(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    
    priv->params.message = message;
    
    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_EVENT_SEND));

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_recv_msg(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    
    priv->params.message = message;

    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_EVENT_RECV));

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_finish(
    LWMsgAssoc* assoc,
    LWMsgMessage** message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    
    if (message)
    {
        priv->params.message = NULL;
    }
    
    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_EVENT_FINISH));

    if (message)
    {
        *message = priv->params.message;
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_set_nonblock(
    LWMsgAssoc* assoc,
    LWMsgBool nonblock
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    switch (lwmsg_assoc_get_state(assoc))
    {
    case LWMSG_ASSOC_STATE_NOT_ESTABLISHED:
    case LWMSG_ASSOC_STATE_IDLE:
    case LWMSG_ASSOC_STATE_CLOSED:
        priv->is_nonblock = nonblock;
        break;
    default:
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE,
                          "Cannot set blocking while connection is busy");
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_get_session(
    LWMsgAssoc* assoc,
    LWMsgSession** session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    
    if (!priv->active_session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }
    
    *session = priv->active_session;

error:

    return status;
}
    
static LWMsgAssocState
lwmsg_connection_get_state(
    LWMsgAssoc* assoc
    )
{
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    LWMsgBool need_send = LWMSG_FALSE;
    LWMsgBool need_recv = LWMSG_FALSE;

    switch (priv->state)
    {
    case CONNECTION_STATE_NONE:
        return LWMSG_ASSOC_STATE_NONE;
    case CONNECTION_STATE_START:
        return LWMSG_ASSOC_STATE_NOT_ESTABLISHED;
    case CONNECTION_STATE_ESTABLISHED:
        need_send = (priv->sendbuffer.current != NULL || !lwmsg_ring_is_empty(&priv->sendbuffer.fragments));
        need_recv = (priv->recvbuffer.current != NULL);
        if (need_send && need_recv)
        {
            return LWMSG_ASSOC_STATE_BLOCKED_SEND_RECV;
        }
        else if (need_send)
        {
            return LWMSG_ASSOC_STATE_BLOCKED_SEND;
        }
        else if (need_recv)
        {
            return LWMSG_ASSOC_STATE_BLOCKED_RECV;
        }
        else
        {
            return LWMSG_ASSOC_STATE_IDLE;
        }
    case CONNECTION_STATE_FINISH_CONNECT_SOCKET:
        return LWMSG_ASSOC_STATE_BLOCKED_SEND;
    case CONNECTION_STATE_FINISH_SEND_CONNECT:
    case CONNECTION_STATE_FINISH_SEND_ACCEPT:
        return LWMSG_ASSOC_STATE_BLOCKED_SEND_RECV;
    case CONNECTION_STATE_FINISH_RECV_CONNECT:
    case CONNECTION_STATE_FINISH_RECV_ACCEPT:
        return LWMSG_ASSOC_STATE_BLOCKED_RECV;
    case CONNECTION_STATE_CLOSED:
        return LWMSG_ASSOC_STATE_CLOSED;
    case CONNECTION_STATE_ERROR:
        return LWMSG_ASSOC_STATE_ERROR;
    default:
        return LWMSG_ASSOC_STATE_BUSY;
    }
}

static
LWMsgStatus
lwmsg_connection_set_timeout(
    LWMsgAssoc* assoc,
    LWMsgTimeout type,
    LWMsgTime* value
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    LWMsgTime* target = NULL;

    if (value != NULL &&
        (value->seconds < 0 || value->microseconds < 0))
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER,
                          "Invalid (negative) timeout value");
    }

    switch (type)
    {
    case LWMSG_TIMEOUT_MESSAGE:
        target = &priv->timeout.message;
        break;
    case LWMSG_TIMEOUT_ESTABLISH:
        target = &priv->timeout.establish;
        break;
    default:
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_UNSUPPORTED,
                          "Unsupported timeout type");
    }

    if (value)
    {
        *target = *value;
    }
    else
    {
        memset(target, 0xFF, sizeof(*target));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_connect(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    if (!session)
    {
        if (!priv->default_session)
        {
            BAIL_ON_ERROR(status = lwmsg_assoc_session_new(assoc, &priv->default_session));
        }

        session = priv->default_session;
    }

    priv->params.establish.session = session;

    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_EVENT_CONNECT));

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_accept(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    if (!session)
    {
        if (!priv->default_session)
        {
            BAIL_ON_ERROR(status = lwmsg_assoc_session_new(assoc, &priv->default_session));
        }

        session = priv->default_session;
    }

    priv->params.establish.session = session;
    
    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_EVENT_ACCEPT));

error:

    return status;
}

static LWMsgAssocClass connection_class =
{
    .construct = lwmsg_connection_construct,
    .destruct = lwmsg_connection_destruct,
    .send_msg = lwmsg_connection_send_msg,
    .recv_msg = lwmsg_connection_recv_msg,
    .close = lwmsg_connection_close,
    .reset = lwmsg_connection_reset,
    .get_session = lwmsg_connection_get_session,
    .get_state = lwmsg_connection_get_state,
    .set_timeout = lwmsg_connection_set_timeout,
    .connect_peer = lwmsg_connection_connect,
    .accept_peer = lwmsg_connection_accept,
    .set_nonblock = lwmsg_connection_set_nonblock,
    .finish = lwmsg_connection_finish
};

LWMsgStatus
lwmsg_connection_new(
    const LWMsgContext* context,
    LWMsgProtocol* prot,
    LWMsgAssoc** out_assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;
    ConnectionPrivate* priv = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_new(context, prot, &connection_class, sizeof(*priv), &assoc));

    priv = CONNECTION_PRIVATE(assoc);
    priv->state = CONNECTION_STATE_START;

    *out_assoc = assoc;

done:

    return status;

error:

    goto done;
}

LWMsgStatus
lwmsg_connection_set_packet_size(
    LWMsgAssoc* assoc,
    size_t size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    if (priv->active_session)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE, "Cannot change packet size of established connection");
    }

    priv->packet_size = size;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_set_fd(
    LWMsgAssoc* assoc,
    LWMsgConnectionMode mode,
    int fd
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    if (fd < 0)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid file descriptor");
    }

    if (priv->fd != -1 || priv->endpoint != NULL)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE, "Connection parameters already set");
    }

    priv->fd = fd;
    priv->mode = mode;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_set_endpoint(
    LWMsgAssoc* assoc,
    LWMsgConnectionMode mode,
    const char* endpoint
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    if (priv->fd != -1 || priv->endpoint != NULL)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE, "Connection parameters already set");
    }

    priv->endpoint = strdup(endpoint);
    
    if (!priv->endpoint)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    priv->mode = mode;

error:

    return status;
}
