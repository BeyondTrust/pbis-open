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
 *        peer.c
 *
 * Abstract:
 *
 *        Multi-threaded peer API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <config.h>
#include "peer-private.h"
#include "util-private.h"
#include "connection-private.h"
#include "protocol-private.h"
#include "session-private.h"
#include "assoc-private.h"

#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

void
lwmsg_peer_lock(
    LWMsgPeer* peer
    )
{
    pthread_mutex_lock(&peer->lock);
}

void
lwmsg_peer_unlock(
    LWMsgPeer* peer
    )
{
    pthread_mutex_unlock(&peer->lock);
}

LWMsgStatus
lwmsg_peer_new(
    const LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgPeer** out_peer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgPeer* peer = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&peer));

    lwmsg_ring_init(&peer->connect_endpoints);
    lwmsg_ring_init(&peer->listen_endpoints);

    memset(&peer->timeout, 0xFF, sizeof(peer->timeout));

    peer->context = context;

    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutex_init(&peer->lock, NULL)));
    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_cond_init(&peer->event, NULL)));

    BAIL_ON_ERROR(status = lwmsg_task_acquire_manager(&peer->task_manager));
    BAIL_ON_ERROR(status = lwmsg_task_group_new(peer->task_manager, &peer->listen_tasks));
    BAIL_ON_ERROR(status = lwmsg_task_group_new(peer->task_manager, &peer->connect_tasks));

    peer->max_clients = 100;
    peer->max_backlog = 8;
    peer->protocol = protocol;

    *out_peer = peer;

error:

    return status;
}

static
void
lwmsg_peer_destroy_endpoint_list(
    LWMsgRing* list
    )
{
    LWMsgRing* ring = NULL;
    LWMsgRing* next = NULL;
    PeerEndpoint* endpoint = NULL;

    for (ring = list->next; ring != list; ring = next)
    {
        next = ring->next;
        endpoint = LWMSG_OBJECT_FROM_MEMBER(ring, PeerEndpoint, ring);

        lwmsg_ring_remove(&endpoint->ring);

        if (endpoint->endpoint)
        {
            free(endpoint->endpoint);
        }

        free(endpoint);
    }
}

void
lwmsg_peer_delete(
    LWMsgPeer* peer
    )
{
    lwmsg_peer_stop_listen(peer);
    lwmsg_peer_disconnect(peer);

    lwmsg_error_clear(&peer->error);

    if (peer->listen_tasks)
    {
        lwmsg_task_group_cancel(peer->listen_tasks);
        lwmsg_task_group_wait(peer->listen_tasks);
        lwmsg_task_group_delete(peer->listen_tasks);
    }

    if (peer->connect_tasks)
    {
        lwmsg_task_group_cancel(peer->connect_tasks);
        lwmsg_task_group_wait(peer->connect_tasks);
        lwmsg_task_group_delete(peer->connect_tasks);
    }

    if (peer->task_manager)
    {
        lwmsg_task_release_manager(peer->task_manager);
    }

    if (peer->connect_session)
    {
        lwmsg_session_release(peer->connect_session);
    }

    if (peer->session_manager)
    {
        lwmsg_session_manager_delete(peer->session_manager);
    }
    
    pthread_mutex_destroy(&peer->lock);
    pthread_cond_destroy(&peer->event);

    if (peer->dispatch.vector)
    {
        free(peer->dispatch.vector);
    }

    lwmsg_peer_destroy_endpoint_list(&peer->connect_endpoints);
    lwmsg_peer_destroy_endpoint_list(&peer->listen_endpoints);

    free(peer);
}

LWMsgStatus
lwmsg_peer_set_timeout(
    LWMsgPeer* peer,
    LWMsgTimeout type,
    LWMsgTime* value
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTime* target = NULL;

    lwmsg_peer_lock(peer);

    if (peer->state != PEER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (value != NULL &&
        (value->seconds < 0 || value->microseconds < 0))
    {
        PEER_RAISE_ERROR(peer, status = LWMSG_STATUS_INVALID_PARAMETER,
                           "Invalid (negative) timeout value");
    }
    
    switch (type)
    {
    case LWMSG_TIMEOUT_MESSAGE:
        target = &peer->timeout.message;
        break;
    case LWMSG_TIMEOUT_ESTABLISH:
        target = &peer->timeout.establish;
        break;
    case LWMSG_TIMEOUT_IDLE:
        target = &peer->timeout.idle;
        break;
    default:
        PEER_RAISE_ERROR(peer, status = LWMSG_STATUS_UNSUPPORTED,
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

    lwmsg_peer_unlock(peer);

    return status;
}

LWMsgStatus
lwmsg_peer_set_max_listen_clients(
    LWMsgPeer* peer,
    unsigned int max_clients
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_peer_lock(peer);

    if (peer->state != PEER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    peer->max_clients = max_clients;

error:

    lwmsg_peer_unlock(peer);

    return status;
}

LWMsgStatus
lwmsg_peer_set_max_listen_backlog(
    LWMsgPeer* peer,
    unsigned int max_backlog
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_peer_lock(peer);

    if (peer->state != PEER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    peer->max_backlog = max_backlog;

error:

    lwmsg_peer_unlock(peer);

    return status;
}

LWMsgStatus
lwmsg_peer_add_dispatch_spec(
    LWMsgPeer* peer,
    LWMsgDispatchSpec* table
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t max_message_tag = 0;
    LWMsgDispatchSpec** new_vector = NULL;
    size_t i;

    lwmsg_peer_lock(peer);

    for (i = 0; table[i].type != LWMSG_DISPATCH_TYPE_END; i++)
    {
        if (table[i].tag > max_message_tag)
        {
            max_message_tag = table[i].tag;
        }
    }
    
    if (peer->dispatch.vector_length < max_message_tag + 1)
    {
        new_vector = realloc(peer->dispatch.vector, sizeof(*new_vector) * (max_message_tag + 1));

        if (!new_vector)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }
        
        /* Zero out new elements of vector */
        memset(new_vector + peer->dispatch.vector_length, 0, 
               (max_message_tag + 1 - peer->dispatch.vector_length) * sizeof(*new_vector));

        peer->dispatch.vector_length = max_message_tag + 1;
        peer->dispatch.vector = new_vector;
    }

    for (i = 0; table[i].type != LWMSG_DISPATCH_TYPE_END; i++)
    {
        peer->dispatch.vector[table[i].tag] = &table[i];
    }

error:

    lwmsg_peer_unlock(peer);

    return status;
}

LWMsgStatus
lwmsg_peer_add_listen_fd(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    int fd
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;
    PeerEndpoint* peer_endpoint = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&peer_endpoint));
    lwmsg_ring_init(&peer_endpoint->ring);
    
    peer_endpoint->type = type;
    peer_endpoint->fd = fd;

    PEER_LOCK(peer, locked);

    lwmsg_ring_enqueue(&peer->listen_endpoints, &peer_endpoint->ring);

done:

    PEER_UNLOCK(peer, locked);

    return status;

error:

    if (peer_endpoint)
    {
        if (peer_endpoint->endpoint)
        {
            free(peer_endpoint->endpoint);
        }

        free(peer_endpoint);
    }

    goto done;
}

LWMsgStatus
lwmsg_peer_add_listen_endpoint(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    const char* endpoint,
    mode_t      permissions
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;
    PeerEndpoint* peer_endpoint = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&peer_endpoint));
    lwmsg_ring_init(&peer_endpoint->ring);
    
    peer_endpoint->type = type;
    peer_endpoint->endpoint = strdup(endpoint);
    peer_endpoint->permissions = permissions;
    peer_endpoint->fd = -1;

    if (!peer_endpoint->endpoint)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    PEER_LOCK(peer, locked);

    lwmsg_ring_enqueue(&peer->listen_endpoints, &peer_endpoint->ring);

done:

    PEER_UNLOCK(peer, locked);

    return status;

error:

    if (peer_endpoint)
    {
        if (peer_endpoint->endpoint)
        {
            free(peer_endpoint->endpoint);
        }

        free(peer_endpoint);
    }

    goto done;
}

static
LWMsgStatus
lwmsg_peer_startup(
    LWMsgPeer* peer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgRing* ring = NULL;
    PeerEndpoint* endpoint = NULL;
    PeerListenTask* task = NULL;
    char* message = NULL;

    if (!peer->session_manager)
    {
        BAIL_ON_ERROR(status = lwmsg_shared_session_manager_new(
                          peer->session_construct,
                          peer->session_destruct,
                          peer->session_construct_data,
                          &peer->session_manager));
    }

    for (ring = peer->listen_endpoints.next; ring != &peer->listen_endpoints; ring = ring->next)
    {
        endpoint = LWMSG_OBJECT_FROM_MEMBER(ring, PeerEndpoint, ring);

        BAIL_ON_ERROR(status = lwmsg_peer_listen_task_new(
                          peer,
                          endpoint->type,
                          endpoint->endpoint,
                          endpoint->permissions,
                          endpoint->fd,
                          &task));
        task = NULL;
    }

    /* Run all listen tasks */
    lwmsg_task_group_wake(peer->listen_tasks);

    LWMSG_LOG_INFO(peer->context, "Listener started");

    if (lwmsg_context_would_log(peer->context, LWMSG_LOGLEVEL_TRACE))
    {
        BAIL_ON_ERROR(status = lwmsg_protocol_print_alloc(peer->protocol, &message));
        LWMSG_LOG_TRACE(peer->context, "Listen protocol:\n%s", message);
    }

done:

    if (message)
    {
        lwmsg_context_free(peer->protocol->context, message);
    }

    return status;

error:

    lwmsg_task_group_cancel(peer->listen_tasks);

    goto done;
}

LWMsgStatus
lwmsg_peer_start_listen(
    LWMsgPeer* peer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;

    PEER_LOCK(peer, locked);

    if (peer->state == PEER_STATE_STOPPED)
    {
        peer->state = PEER_STATE_STARTING;
        PEER_UNLOCK(peer, locked);
        
        BAIL_ON_ERROR(status = lwmsg_peer_startup(peer));

        PEER_LOCK(peer, locked);
        peer->state = PEER_STATE_STARTED;
        pthread_cond_broadcast(&peer->event);
    }
    else if (peer->state == PEER_STATE_STARTING)
    {
        /* Wait for someone else to finish starting peer */
        while (peer->state == PEER_STATE_STARTING)
        {
            pthread_cond_wait(&peer->event, &peer->lock);
        }
    }
    
    if (peer->state == PEER_STATE_ERROR)
    {
        BAIL_ON_ERROR(status = peer->status);
    }

done:

    PEER_UNLOCK(peer, locked);

    return status;

error:

    PEER_LOCK(peer, locked);
    peer->state = PEER_STATE_ERROR;
    peer->status = status;
    pthread_cond_broadcast(&peer->event);

    goto done;
}

static
LWMsgStatus
lwmsg_peer_shutdown(
    LWMsgPeer* peer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    LWMSG_LOG_INFO(peer->context, "Shutting down listener");

    lwmsg_task_group_cancel(peer->listen_tasks);
    lwmsg_task_group_wait(peer->listen_tasks);

    LWMSG_LOG_INFO(peer->context, "Listener shut down");

    return status;
}

LWMsgStatus
lwmsg_peer_stop_listen(
    LWMsgPeer* peer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;

    PEER_LOCK(peer, locked);

    if (peer->state == PEER_STATE_STARTED)
    {
        peer->state = PEER_STATE_STOPPING;
        PEER_UNLOCK(peer, locked);

        BAIL_ON_ERROR(status = lwmsg_peer_shutdown(peer));

        PEER_LOCK(peer, locked);
        peer->state = PEER_STATE_STOPPED;
        pthread_cond_broadcast(&peer->event);
    }
    else if (peer->state == PEER_STATE_STOPPING)
    {
        /* Wait for someone else to finish stopping peer */
        while (peer->state == PEER_STATE_STOPPING)
        {
            pthread_cond_wait(&peer->event, &peer->lock);
        }
    }

    if (peer->state == PEER_STATE_ERROR)
    {
        BAIL_ON_ERROR(status = peer->status);
    }

done:

    PEER_UNLOCK(peer, locked);

    return status;

error:

    PEER_LOCK(peer, locked);
    peer->state = PEER_STATE_ERROR;
    peer->status = status;
    pthread_cond_broadcast(&peer->event);

    goto done;
}

LWMsgStatus
lwmsg_peer_set_listen_session_functions(
    LWMsgPeer* peer,
    LWMsgSessionConstructFunction construct,
    LWMsgSessionDestructFunction destruct,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_peer_lock(peer);

    if (peer->state != PEER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    peer->session_construct = construct;
    peer->session_destruct = destruct;
    peer->session_construct_data = data;

error:

    lwmsg_peer_unlock(peer);

    return status;
}

LWMsgStatus
lwmsg_peer_set_dispatch_data(
    LWMsgPeer* peer,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_peer_lock(peer);

    if (peer->state != PEER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    peer->dispatch_data = data;

error:

    lwmsg_peer_unlock(peer);

    return status;
}

void*
lwmsg_peer_get_dispatch_data(
    LWMsgPeer* peer
    )
{
    return peer->dispatch_data;
}

LWMsgBool
lwmsg_peer_acquire_client_slot(
    LWMsgPeer* peer
    )
{
    LWMsgBool result = LWMSG_FALSE;
    LWMsgBool wake = LWMSG_FALSE;

    lwmsg_peer_lock(peer);
    if (peer->num_clients < peer->max_clients)
    {
        if (++peer->num_clients == peer->max_clients)
        {
            wake = LWMSG_TRUE;
        }
        result = LWMSG_TRUE;
    }

    lwmsg_peer_unlock(peer);

    if (wake)
    {
        lwmsg_task_group_wake(peer->listen_tasks);
    }

    return result;
}

void
lwmsg_peer_release_client_slot(
    LWMsgPeer* peer
    )
{
    LWMsgBool wake = LWMSG_FALSE;

    lwmsg_peer_lock(peer);
    if (peer->num_clients-- == peer->max_clients)
    {
        wake = LWMSG_TRUE;
    }
    lwmsg_peer_unlock(peer);

    if (wake)
    {
        lwmsg_task_group_wake(peer->listen_tasks);
    }
}

size_t
lwmsg_peer_get_num_clients(
    LWMsgPeer* peer
    )
{
    size_t result = 0;

    lwmsg_peer_lock(peer);
    result = peer->num_clients;
    lwmsg_peer_unlock(peer);

    return result;
}

LWMsgStatus
lwmsg_peer_set_exception_function(
    LWMsgPeer* peer,
    LWMsgPeerExceptionFunction except,
    void* except_data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    
    lwmsg_peer_lock(peer);

    if (peer->state != PEER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }
    
    peer->except = except;
    peer->except_data = except_data;

error:

    lwmsg_peer_unlock(peer);

    return status;
}

LWMsgStatus
lwmsg_peer_add_connect_endpoint(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    const char* endpoint
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;
    PeerEndpoint* peer_endpoint = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&peer_endpoint));
    lwmsg_ring_init(&peer_endpoint->ring);
    
    peer_endpoint->type = type;
    peer_endpoint->endpoint = strdup(endpoint);

    if (!peer_endpoint->endpoint)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    PEER_LOCK(peer, locked);

    lwmsg_ring_enqueue(&peer->connect_endpoints, &peer_endpoint->ring);

done:

    PEER_UNLOCK(peer, locked);

    return status;

error:

    if (peer_endpoint)
    {
        if (peer_endpoint->endpoint)
        {
            free(peer_endpoint->endpoint);
        }

        free(peer_endpoint);
    }

    goto done;
}

static
LWMsgStatus
lwmsg_peer_connect_endpoint(
    LWMsgPeer* peer,
    PeerEndpoint* endpoint
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;
    LWMsgBool locked = LWMSG_FALSE;

    switch (endpoint->type)
    {
    case LWMSG_ENDPOINT_LOCAL:
        /* Create association to endpoint */
        BAIL_ON_ERROR(status = lwmsg_connection_new(peer->context, peer->protocol, &assoc));
        BAIL_ON_ERROR(status = lwmsg_connection_set_endpoint(assoc, endpoint->type, endpoint->endpoint));
        BAIL_ON_ERROR(status = lwmsg_assoc_set_nonblock(assoc, LWMSG_TRUE));
        /* Create task to manage it */
        BAIL_ON_ERROR(status = lwmsg_peer_assoc_task_new_connect(peer, assoc, peer->connect_session, &peer->connect_task));
        assoc = NULL;

        LWMSG_LOCK(locked, &peer->connect_task->call_lock);
        peer->connect_task->status = LWMSG_STATUS_PENDING;

        /* Wake task up */
        lwmsg_task_wake(peer->connect_task->event_task);

        /* Wait for task to tell us it connected */
        while (peer->connect_task->status == LWMSG_STATUS_PENDING)
        {
            pthread_cond_wait(&peer->connect_task->call_event, &peer->connect_task->call_lock);
        }
        BAIL_ON_ERROR(status = peer->connect_task->status);

        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNSUPPORTED);
    }

done:

    LWMSG_UNLOCK(locked, &peer->connect_task->call_lock);

    return status;

error:

    LWMSG_UNLOCK(locked, &peer->connect_task->call_lock);

    if (assoc)
    {
        lwmsg_assoc_delete(assoc);
    }

    if (peer->connect_task)
    {
        lwmsg_peer_task_cancel_and_unref(peer->connect_task);
        peer->connect_task = NULL;
    }

    goto done;
}

static
LWMsgStatus
lwmsg_peer_try_connect(
    LWMsgPeer* peer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgRing* ring = NULL;
    PeerEndpoint* endpoint = NULL;

    if (!peer->session_manager)
    {
        BAIL_ON_ERROR(status = lwmsg_shared_session_manager_new(
                          peer->session_construct,
                          peer->session_destruct,
                          peer->session_construct_data,
                          &peer->session_manager));
    }
    
    if (!peer->connect_session)
    {
        BAIL_ON_ERROR(status = lwmsg_session_create(
                          peer->session_manager,
                          &peer->connect_session));
    }

    if (!peer->connect_task)
    {
        for (ring = peer->connect_endpoints.next; ring != &peer->connect_endpoints; ring = ring->next)
        {
            endpoint = LWMSG_OBJECT_FROM_MEMBER(ring, PeerEndpoint, ring);
            status = lwmsg_peer_connect_endpoint(peer, endpoint);
            switch (status)
            {
            case LWMSG_STATUS_SUCCESS:
                goto done;
            case LWMSG_STATUS_CONNECTION_REFUSED:
            case LWMSG_STATUS_TIMEOUT:
            case LWMSG_STATUS_FILE_NOT_FOUND:
                break;
            default:
                BAIL_ON_ERROR(status);
            }
        }
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_peer_connect_in_lock(
    LWMsgPeer* peer,
    LWMsgBool* locked
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    PEER_LOCK(peer, *locked);

    switch (peer->connect_state)
    {
    case PEER_STATE_STOPPED:
        peer->connect_status = LWMSG_STATUS_SUCCESS;
        peer->connect_state = PEER_STATE_STARTING;
        PEER_UNLOCK(peer, *locked);
        status = lwmsg_peer_try_connect(peer);
        PEER_LOCK(peer, *locked);
        if (status == LWMSG_STATUS_SUCCESS)
        {
            peer->connect_state = PEER_STATE_STARTED;
            pthread_cond_broadcast(&peer->event);
        }
        else
        {
            peer->connect_status = status;
            peer->connect_state = PEER_STATE_STOPPED;
        }
        break;
    case PEER_STATE_STARTING:
        while (peer->connect_state == PEER_STATE_STARTING)
        {
            BAIL_ON_ERROR(status = peer->connect_status);
            pthread_cond_wait(&peer->event, &peer->lock);
        }
        break;
    case PEER_STATE_STARTED:
        break;
    case PEER_STATE_STOPPING:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    case PEER_STATE_ERROR:
        BAIL_ON_ERROR(status = peer->connect_state);
        break;
    }

error:

    return status;
}


LWMsgStatus
lwmsg_peer_connect(
    LWMsgPeer* peer,
    LWMsgSession** session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;

    BAIL_ON_ERROR(status = lwmsg_peer_connect_in_lock(peer, &locked));
    
    if (session)
    {
        *session = peer->connect_session;
    }

error:

    PEER_UNLOCK(peer, locked);

    return status;
}

LWMsgStatus
lwmsg_peer_disconnect(
    LWMsgPeer* peer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;

    PEER_LOCK(peer, locked);
    switch (peer->connect_state)
    {
    case PEER_STATE_STARTED:
        peer->connect_state = PEER_STATE_STOPPING;
        peer->connect_status = LWMSG_STATUS_SUCCESS;

        PEER_UNLOCK(peer, locked);

        if (peer->connect_task)
        {
            lwmsg_peer_task_cancel_and_unref(peer->connect_task);
            peer->connect_task = NULL;
        }

        PEER_LOCK(peer, locked);

        peer->connect_state = PEER_STATE_STOPPED;
        pthread_cond_broadcast(&peer->event);
        break;
    case PEER_STATE_STOPPING:
        while (peer->connect_state == PEER_STATE_STOPPING)
        {
            BAIL_ON_ERROR(status = peer->connect_status);
            pthread_cond_wait(&peer->event, &peer->lock);
        }
        break;
    case PEER_STATE_STOPPED:
        break;
    case PEER_STATE_STARTING:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
        break;
    case PEER_STATE_ERROR:
        BAIL_ON_ERROR(status = peer->connect_status);
        break;
    }

error:

    PEER_UNLOCK(peer, locked);

    return status;
}

LWMsgStatus
lwmsg_peer_acquire_call(
    LWMsgPeer* peer,
    LWMsgCall** call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgStatus orig_status = LWMSG_STATUS_SUCCESS;
    PeerCall* pcall = NULL;
    LWMsgBool locked = LWMSG_FALSE;
    PeerAssocTask* task = NULL;

    do
    {
        /* Ensure we are in a connected state, and acquire a reference
           to the connect task */
        PEER_LOCK(peer, locked);
        BAIL_ON_ERROR(status = lwmsg_peer_connect_in_lock(peer, &locked));
        task = peer->connect_task;
        task->refcount++;
        PEER_UNLOCK(peer, locked);
        
        /* Check that the connect task is in a good state */
        pthread_mutex_lock(&task->call_lock);
        status = task->status;
        pthread_mutex_unlock(&task->call_lock);
        
        switch (status)
        {
        case LWMSG_STATUS_PEER_CLOSE:
        case LWMSG_STATUS_PEER_RESET:
            /* Drop the reference */
            lwmsg_peer_task_unref(task);
            task = NULL;
            /* Disconnect and try again */
            lwmsg_peer_disconnect(peer);
            orig_status = status;
            status = LWMSG_STATUS_AGAIN;
            break;
        default:
            break;
            BAIL_ON_ERROR(status);
        }
    } while (status == LWMSG_STATUS_AGAIN);
            
    BAIL_ON_ERROR(status = lwmsg_peer_call_new(task, &pcall));
    task = NULL;

    *call = LWMSG_CALL(pcall);

done:

    PEER_UNLOCK(peer, locked);

    return status;

error:

    PEER_UNLOCK(peer, locked);

    if (pcall)
    {
        lwmsg_peer_call_delete(pcall);
    }

    if (task)
    {
        lwmsg_peer_task_unref(task);
    }

    switch (status)
    {
    case LWMSG_STATUS_CONNECTION_REFUSED:
    case LWMSG_STATUS_FILE_NOT_FOUND:
        if (orig_status)
        {
            status = orig_status;
        }
        break;
    default:
        break;
    }

    goto done;
}
