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
#include "assoc-private.h"
#include "session-private.h"
#include "connection-private.h"
#include "buffer-private.h"
#include "security-private.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>

static
void
lwmsg_peer_task_run(
    LWMsgTask* _task,
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTaskTime* next_timeout
    );

static
void
lwmsg_peer_task_run_listen(
    LWMsgTask* _task,
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTaskTime* next_timeout
    );

static
void
lwmsg_peer_task_update_blocked(
    PeerAssocTask* task
    )
{
    switch (lwmsg_assoc_get_state(task->assoc))
    {
    case LWMSG_ASSOC_STATE_BLOCKED_SEND:
        task->send_blocked = LWMSG_TRUE;
        task->recv_blocked = LWMSG_FALSE;
        break;
    case LWMSG_ASSOC_STATE_BLOCKED_RECV:
        task->recv_blocked = LWMSG_TRUE;
        task->send_blocked = LWMSG_FALSE;
        break;
    case LWMSG_ASSOC_STATE_BLOCKED_SEND_RECV:
        task->send_blocked = task->recv_blocked = LWMSG_TRUE;
        break;
    default:
        task->send_blocked = task->recv_blocked = LWMSG_FALSE;
        break;
    }
}

static
void
lwmsg_peer_task_delete(
    PeerAssocTask* task
    )
{
    LWMsgHashIter iter = {0};
    PeerCall* call = NULL;
    LWMsgRing* ring = NULL;
    LWMsgRing* next = NULL;

    lwmsg_hash_iter_begin(&task->incoming_calls, &iter);
    while ((call = lwmsg_hash_iter_next(&task->incoming_calls, &iter)))
    {
        lwmsg_hash_remove_entry(&task->incoming_calls, call);
        lwmsg_peer_call_delete(call);
    }
    lwmsg_hash_iter_end(&task->incoming_calls, &iter);

    lwmsg_hash_destroy(&task->incoming_calls);

    lwmsg_hash_iter_begin(&task->outgoing_calls, &iter);
    while ((call = lwmsg_hash_iter_next(&task->outgoing_calls, &iter)))
    {
        lwmsg_hash_remove_entry(&task->outgoing_calls, call);
        lwmsg_peer_call_delete(call);
    }
    lwmsg_hash_iter_end(&task->outgoing_calls, &iter);

    lwmsg_hash_destroy(&task->outgoing_calls);

    for (ring = task->active_incoming_calls.next;
         ring != &task->active_incoming_calls;
         ring = next)
    {
        next = ring->next;
        call = LWMSG_OBJECT_FROM_MEMBER(ring, PeerCall, queue_ring);
        lwmsg_peer_call_delete(call);
    }

    for (ring = task->active_outgoing_calls.next;
         ring != &task->active_outgoing_calls;
         ring = next)
    {
        next = ring->next;
        call = LWMSG_OBJECT_FROM_MEMBER(ring, PeerCall, queue_ring);
        lwmsg_peer_call_delete(call);
    }

    if (task->assoc)
    {
        lwmsg_assoc_destroy_message(task->assoc, &task->incoming_message);
        if (task->destroy_outgoing)
        {
            lwmsg_assoc_destroy_message(task->assoc, &task->outgoing_message);
        }
        lwmsg_assoc_delete(task->assoc);
    }

    if (task->event_task)
    {
        lwmsg_task_release(task->event_task);
    }

    if (task->session)
    {
        if (!task->session->is_outgoing)
        {
            lwmsg_peer_release_client_slot(task->session->peer);
        }

        lwmsg_peer_session_release(task->session);
    }

    free(task);
}

void
lwmsg_peer_task_release(
    PeerAssocTask* task
    )
{
    int32_t refs = 0;

    pthread_mutex_lock(&task->session->lock);
    refs = --task->refs;
    pthread_mutex_unlock(&task->session->lock);

    if (refs == 0)
    {
        lwmsg_peer_task_delete(task);
    }
}

static
void*
lwmsg_peer_call_get_key(
    const void* entry
    )
{
    return &((PeerCall*) entry)->cookie;
}

static
size_t
lwmsg_peer_call_digest(
    const void* key
    )
{
    return (size_t) *(LWMsgCookie*) key;
}

static
LWMsgBool
lwmsg_peer_call_equal(
    const void* key1,
    const void* key2
    )
{
    return *((const LWMsgCookie*) key1) == *((const LWMsgCookie*) key2);
}

static LWMsgStatus
lwmsg_peer_assoc_task_new(
    PeerSession* session,
    PeerAssocTaskType type,
    PeerAssocTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerAssocTask* my_task = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_task));

    my_task->session = session;
    my_task->type = type;
    my_task->refs = 1;

    lwmsg_ring_init(&my_task->active_incoming_calls);
    lwmsg_ring_init(&my_task->active_outgoing_calls);

    BAIL_ON_ERROR(status = lwmsg_hash_init(
                      &my_task->incoming_calls,
                      31,
                      lwmsg_peer_call_get_key,
                      lwmsg_peer_call_digest,
                      lwmsg_peer_call_equal,
                      offsetof(PeerCall, hash_ring)));

    BAIL_ON_ERROR(status = lwmsg_hash_init(
                      &my_task->outgoing_calls,
                      31,
                      lwmsg_peer_call_get_key,
                      lwmsg_peer_call_digest,
                      lwmsg_peer_call_equal,
                      offsetof(PeerCall, hash_ring)));

    lwmsg_message_init(&my_task->incoming_message);
    lwmsg_message_init(&my_task->outgoing_message);

    *task = my_task;

error:

    if (status && my_task)
    {
        my_task->session = NULL;
        lwmsg_peer_task_delete(my_task);
    }

    return status;
}


static LWMsgStatus
lwmsg_peer_task_create_local_socket(
    PeerListenTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int sock = -1;
    struct sockaddr_un sockaddr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock == -1)
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }

    BAIL_ON_ERROR(status = lwmsg_set_close_on_exec(sock));

    sockaddr.sun_family = AF_UNIX;

    if (strlen(task->endpoint) > sizeof(sockaddr.sun_path))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    strcpy(sockaddr.sun_path, task->endpoint);
    unlink(sockaddr.sun_path);

    if (bind(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) == -1)
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }

    // ignore errors
    chmod(sockaddr.sun_path, task->perms);

    task->fd = sock;
    sock = -1;

done:

    return status;

error:

    if (sock != -1)
    {
        close(sock);
    }

    goto done;
}

static
LWMsgStatus
lwmsg_peer_task_setup_listen(
    PeerListenTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    long opts = 0;

    /* Create and bind socket if needed */
    if (task->fd == -1)
    {
        BAIL_ON_ERROR(status = lwmsg_peer_task_create_local_socket(task));
    }
    
    /* Get socket flags */
    if ((opts = fcntl(task->fd, F_GETFL, 0)) < 0)
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }
    
    /* Set non-blocking flag */
    opts |= O_NONBLOCK;
    
    /* Set socket flags */
    if (fcntl(task->fd, F_SETFL, opts) < 0)
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }
    
    if (listen(task->fd, task->peer->max_backlog))
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }
    
    if (task->endpoint)
    {
        LWMSG_LOG_INFO(task->peer->context, "Listening on endpoint %s", task->endpoint);
    }
    else
    {
        LWMSG_LOG_INFO(task->peer->context, "Listening on fd %i", task->fd);  
    }

error:

    return status;
}

LWMsgStatus
lwmsg_peer_listen_task_new(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    const char* endpoint,
    mode_t perms,
    int fd,
    PeerListenTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerListenTask* my_task = NULL;
    
    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_task));
    
    BAIL_ON_ERROR(status = lwmsg_task_new(
                      peer->task_manager,
                      peer->listen_tasks,
                      lwmsg_peer_task_run_listen,
                      my_task,
                      &my_task->event_task));

    my_task->peer = peer;

    if (endpoint)
    {
        my_task->endpoint = strdup(endpoint);
    
        if (!my_task->endpoint)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }
    }
    
    my_task->fd = fd;
    my_task->type = type;
    my_task->perms = perms;

    BAIL_ON_ERROR(status = lwmsg_peer_task_setup_listen(my_task));

    *task = my_task;

error:

    if (status && my_task)
    {
        lwmsg_peer_listen_task_delete(my_task);
    }

    return status;
}

LWMsgStatus
lwmsg_peer_assoc_task_new_accept(
    PeerSession* session,
    LWMsgAssoc* assoc,
    PeerAssocTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerAssocTask* my_task = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_peer_assoc_task_new(session, PEER_TASK_BEGIN_ACCEPT, &my_task));

    my_task->assoc = assoc;

    status = lwmsg_task_new(
        session->peer->task_manager,
        session->peer->listen_tasks,
        lwmsg_peer_task_run,
        my_task,
        &my_task->event_task);
    if (status != LWMSG_STATUS_SUCCESS)
    {
        my_task->assoc = NULL;
    }
    BAIL_ON_ERROR(status);

    *task = my_task;

done:

    return status;

error:

    if (my_task)
    {
        my_task->session = NULL;
        lwmsg_peer_task_delete(my_task);
    }

    goto done;
}

LWMsgStatus
lwmsg_peer_assoc_task_new_connect(
    PeerSession* session,
    LWMsgAssoc* assoc,
    PeerAssocTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerAssocTask* my_task = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_peer_assoc_task_new(session, PEER_TASK_BEGIN_CONNECT, &my_task));

    my_task->assoc = assoc;
    my_task->refs++;

    BAIL_ON_ERROR(status = lwmsg_task_new(
        session->peer->task_manager,
        session->peer->connect_tasks,
        lwmsg_peer_task_run,
        my_task,
        &my_task->event_task));
    *task = my_task;

done:

    return status;

error:

    if (my_task)
    {
        my_task->session = NULL;
        lwmsg_peer_task_delete(my_task);
    }

    goto done;
}

static
void
lwmsg_peer_listen_task_delete_self(
    PeerListenTask* task
    )
{
    if (task->event_task)
    {
        lwmsg_task_release(task->event_task);
    }

    if (task->endpoint)
    {
        unlink(task->endpoint);
        free(task->endpoint);
    }

    if (task->fd >= 0)
    {
        close(task->fd);
    }

    free(task);
}

void
lwmsg_peer_listen_task_delete(
    PeerListenTask* task
    )
{
    if (task->event_task)
    {
        /* Just cancel the task, it will take care of
           cleaning up after itself */
        lwmsg_task_cancel(task->event_task);
    }
    else
    {
        lwmsg_peer_listen_task_delete_self(task);
    }
}

static
LWMsgBool
lwmsg_peer_task_subject_to_timeout(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTaskTrigger trigger
    )
{
    size_t handle_count = 0;
    size_t num_clients = 0;

    if (task->type == PEER_TASK_DISPATCH)
    {
        handle_count = lwmsg_session_get_handle_count((LWMsgSession*) task->session);
        num_clients = lwmsg_peer_get_num_clients(peer);
        
        return handle_count == 0 && num_clients == peer->max_clients && 
            lwmsg_hash_get_count(&task->incoming_calls) == 0;
    }
    else
    {
        return LWMSG_TRUE;
    }

    return LWMSG_TRUE;
}

static
void
lwmsg_peer_task_set_timeout(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTime* timeout,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    *next_timeout = *timeout;

    if (lwmsg_time_is_positive(timeout) &&
        lwmsg_peer_task_subject_to_timeout(peer, task, trigger))
    {
        *next_trigger |= LWMSG_TASK_TRIGGER_TIME;
    }
}

static
LWMsgStatus
lwmsg_peer_task_handle_assoc_error(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgStatus status
    )
{
    LWMsgSessionString sessid = {0};

    lwmsg_peer_session_string_for_session((LWMsgSession*) task->session, sessid);

    if (status)
    {
        pthread_mutex_lock(&task->session->lock);
        task->status = status;
        pthread_mutex_unlock(&task->session->lock);
    }

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        break;
    case LWMSG_STATUS_CONNECTION_REFUSED:
    case LWMSG_STATUS_PEER_CLOSE:
    case LWMSG_STATUS_PEER_RESET:
    case LWMSG_STATUS_PEER_ABORT:
        LWMSG_LOG_VERBOSE(peer->context, "(session:%s) Dropping: %s",
                          sessid,
                          lwmsg_status_name(status));
        task->type = PEER_TASK_DROP;
        status = LWMSG_STATUS_SUCCESS;
        break;
    case LWMSG_STATUS_TIMEOUT:
        LWMSG_LOG_VERBOSE(peer->context, "(session:%s) Resetting: %s",
                          sessid,
                          lwmsg_status_name(status));
        task->type = PEER_TASK_BEGIN_RESET;
        status = LWMSG_STATUS_SUCCESS;
    default:
        LWMSG_LOG_VERBOSE(peer->context, "(session:%s) Dropping: %s",
                          sessid,
                          lwmsg_status_name(status));
        task->type = PEER_TASK_BEGIN_CLOSE;
        status = LWMSG_STATUS_SUCCESS;
        break;
    }

    return status;
}

static
LWMsgStatus
lwmsg_peer_task_handle_call_error(
    PeerAssocTask* task,
    PeerCall* call,
    LWMsgStatus status
    )
{
    call->status = status;
    call->state = PEER_CALL_DISPATCHED | PEER_CALL_COMPLETED;
    call->params.incoming.out.tag = LWMSG_TAG_INVALID;
    call->params.incoming.out.data = NULL;

    return LWMSG_STATUS_SUCCESS;
}

static
void
lwmsg_peer_task_run_listen(
    LWMsgTask* _task,
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTaskTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerListenTask* task = data;
    int client_fd = -1;
    LWMsgBool slot = LWMSG_FALSE;
    int err = 0;
    
    if (trigger & LWMSG_TASK_TRIGGER_INIT)
    {
        BAIL_ON_ERROR(status = lwmsg_task_set_trigger_fd(task->event_task, task->fd));
    }

    while (status == LWMSG_STATUS_SUCCESS)
    {
        slot = LWMSG_FALSE;
        client_fd = -1;
        
        if (trigger & LWMSG_TASK_TRIGGER_CANCEL)
        {
            lwmsg_task_unset_trigger_fd(task->event_task, task->fd);
            lwmsg_peer_listen_task_delete_self(task);
            *next_trigger = 0;
            goto done;
        }
        
        if ((slot = lwmsg_peer_acquire_client_slot(task->peer)))
        {
            do
            {
                client_fd = accept(task->fd, NULL, NULL);
                
                if (client_fd < 0)
                {
                    switch (errno)
                    {
                    case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
                    case EWOULDBLOCK:
#endif
                        /* We are blocked, wake up when fd is readable */
                        *next_trigger = LWMSG_TASK_TRIGGER_FD_READABLE;
                        goto done;
                    case EINTR:
                    case ECONNABORTED:
                        /* Trivially retriable errors.  Ignore them and loop again */
                        continue;
                    default:
                        err = errno;
                        LWMSG_LOG_ERROR(task->peer->context, "System error on accept(): %i", err);
                        BAIL_ON_ERROR(status = lwmsg_status_map_errno(err));
                    }
                }
            } while (client_fd < 0);
            
            BAIL_ON_ERROR(status = lwmsg_peer_accept_fd_internal(
                task->peer,
                LWMSG_ENDPOINT_LOCAL,
                client_fd,
                task->endpoint));
            client_fd = -1;
            slot = LWMSG_FALSE;
        }
        else
        {
            /* We've run out of client slots, so stop accepting clients
               until we are explicitly woken up */
            *next_trigger = LWMSG_TASK_TRIGGER_EXPLICIT;
            goto done;
        }
    }

done:

    if (slot)
    {
        lwmsg_peer_release_client_slot(task->peer);
    }

    return;

error:
    
    if (client_fd >= 0)
    {
        close(client_fd);
    }
    
    /* If the listen task aborts, the server will be left in a state
       where it is running but cannot be contacted.  Invoke the
       exception function set on the server to give the application
       a chance to bail out if it wishes.  Also, attempt to keep running
       if the error appears to be recoverable */
    if (task->peer->except)
    {
        task->peer->except(task->peer, status, task->peer->except_data);
    }

    switch (status)
    {
    case LWMSG_STATUS_BUSY:
    case LWMSG_STATUS_MEMORY:
    case LWMSG_STATUS_RESOURCE_LIMIT:
        *next_trigger = LWMSG_TASK_TRIGGER_YIELD;
        break;
    default:
        *next_trigger = 0;
        break;
    }
    
    goto done;
}

static
LWMsgStatus
lwmsg_peer_task_run_accept(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    /* We already have the fd when accepting a connection, so set it up for events now */
    BAIL_ON_ERROR(status = lwmsg_task_set_trigger_fd(task->event_task, CONNECTION_PRIVATE(task->assoc)->fd));

    status = lwmsg_assoc_accept(task->assoc, (LWMsgSession*) task->session);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        task->type = PEER_TASK_DISPATCH;
        BAIL_ON_ERROR(status = lwmsg_peer_log_accept(task));
        break;
    case LWMSG_STATUS_PENDING:
        task->type = PEER_TASK_FINISH_ACCEPT;
        lwmsg_peer_task_set_timeout(peer, task, &peer->timeout.establish, trigger, next_trigger, next_timeout);
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_peer_task_handle_assoc_error(
                          peer,
                          task,
                          status));
        break;
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_peer_task_run_connect(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_assoc_connect(task->assoc, (LWMsgSession*) task->session);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        task->type = PEER_TASK_DISPATCH;
        BAIL_ON_ERROR(status = lwmsg_peer_log_connect(task));
        /* Set up the fd the assoc just created for events */
        BAIL_ON_ERROR(status = lwmsg_task_set_trigger_fd(task->event_task, CONNECTION_PRIVATE(task->assoc)->fd));
        break;
    case LWMSG_STATUS_PENDING:
        task->type = PEER_TASK_FINISH_CONNECT;
        /* Even if the connect pended, the fd is now available.  Set it up for events */
        BAIL_ON_ERROR(status = lwmsg_task_set_trigger_fd(task->event_task, CONNECTION_PRIVATE(task->assoc)->fd));
        /* Restore status code */
        status = LWMSG_STATUS_PENDING;
        lwmsg_peer_task_set_timeout(peer, task, &peer->timeout.establish, trigger, next_trigger, next_timeout);
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_peer_task_handle_assoc_error(
                          peer,
                          task,
                          status));
        break;
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_peer_task_rundown(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgHashIter iter = {0};
    PeerCall* call = NULL;
    LWMsgMessage cancel = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;    

    pthread_mutex_lock(&task->session->lock);

    lwmsg_hash_iter_begin(&task->incoming_calls, &iter);
    while ((call = lwmsg_hash_iter_next(&task->incoming_calls, &iter)))
    {
        if (call->state & PEER_CALL_COMPLETED)
        {
            /* Destroy in call parameters */
            message.tag = call->params.incoming.in.tag;
            message.data = call->params.incoming.in.data;
            lwmsg_assoc_destroy_message(task->assoc, &message);

            /* Destroy out call parameters */
            message.tag = call->params.incoming.out.tag;
            message.data = call->params.incoming.out.data;
            lwmsg_assoc_destroy_message(task->assoc, &message);

            lwmsg_hash_remove_entry(&task->incoming_calls, call);
            lwmsg_peer_call_delete(call);
        }
        else if (!(call->state & PEER_CALL_CANCELLED))
        {
            lwmsg_peer_call_cancel_incoming(call);
        }
    }
    lwmsg_hash_iter_end(&task->incoming_calls, &iter);

    lwmsg_hash_iter_begin(&task->outgoing_calls, &iter);
    while ((call = lwmsg_hash_iter_next(&task->outgoing_calls, &iter)))
    {
        if (task->status)
        {
            cancel.status = task->status;
        }
        else
        {
            cancel.status = LWMSG_STATUS_CANCELLED;
        }
        lwmsg_peer_call_complete_outgoing(call, &cancel);
    }
    lwmsg_hash_iter_end(&task->outgoing_calls, &iter);

    if (lwmsg_hash_get_count(&task->incoming_calls) != 0 ||
        lwmsg_hash_get_count(&task->outgoing_calls) != 0)
    {
        *next_trigger = LWMSG_TASK_TRIGGER_EXPLICIT;
        BAIL_ON_ERROR(status = LWMSG_STATUS_PENDING);
    }

error:

    pthread_mutex_unlock(&task->session->lock);

    return status;
}

static
LWMsgStatus
lwmsg_peer_task_run_shutdown(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int fd = CONNECTION_PRIVATE(task->assoc)->fd;

    /* Before we close the assoc, we need to free any data it allocated */

    /* First, make sure any calls are canceled, completed, and freed */
    BAIL_ON_ERROR(status = lwmsg_peer_task_rundown(peer, task, trigger, next_trigger, next_timeout));

    /* Destroy any incoming message we never got around to dispatching */
    lwmsg_assoc_destroy_message(task->assoc, &task->incoming_message);

    /* Destroy any outgoing message we never got around to sending */
    if (task->destroy_outgoing)
    {
        lwmsg_assoc_destroy_message(task->assoc, &task->outgoing_message);
    }

    /* We are going to close the fd, so unset it now */
    if (fd >= 0)
    {
        lwmsg_task_unset_trigger_fd(task->event_task, fd);
    }

    switch (task->type)
    {
    case PEER_TASK_BEGIN_CLOSE:
        status = lwmsg_assoc_close(task->assoc);
        break;
    case PEER_TASK_BEGIN_RESET:
        status = lwmsg_assoc_reset(task->assoc);
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INTERNAL);
    }
    
    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        task->type = PEER_TASK_DROP;
        break;
    case LWMSG_STATUS_PENDING:
        /* Oops, we still need to wait for fd events, so set it on the task again */
        if (fd >= 0)
        {
            BAIL_ON_ERROR(status = lwmsg_task_set_trigger_fd(task->event_task, fd));
        }
        task->type = PEER_TASK_FINISH_CLOSE;
        lwmsg_peer_task_set_timeout(peer, task, &peer->timeout.establish, trigger, next_trigger, next_timeout);
        break;
    default:
        BAIL_ON_ERROR(status);
        break;
    }

done:

    return status;

error:

    goto done;
}

static
void
lwmsg_peer_task_cancel_call(
    PeerAssocTask* task,
    LWMsgCookie cookie
    )
{
    PeerCall* call = NULL;

    call = lwmsg_hash_find_key(&task->incoming_calls, &cookie);

    if (call)
    {
        lwmsg_peer_log_message(task, &task->incoming_message, LWMSG_FALSE);
        lwmsg_peer_call_cancel_incoming(call);
    }
}

static
void
lwmsg_peer_task_complete_call(
    PeerAssocTask* task,
    LWMsgCookie cookie
    )
{
    PeerCall* call = NULL;

    call = lwmsg_hash_find_key(&task->outgoing_calls, &cookie);

    if (call)
    {
        lwmsg_peer_log_message(task, &task->incoming_message, LWMSG_TRUE);
        lwmsg_peer_call_complete_outgoing(call, &task->incoming_message);
    }
}

static
LWMsgStatus
lwmsg_peer_task_dispatch_incoming_message(
    PeerAssocTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDispatchSpec* spec = NULL;
    LWMsgTag tag = task->incoming_message.tag;
    LWMsgPeer* peer = task->session->peer;
    PeerCall* call = NULL;

    /* If the incoming message is a reply to one of our calls,
       find it and complete it */
    if (task->incoming_message.flags & LWMSG_MESSAGE_FLAG_REPLY)
    {
        lwmsg_peer_task_complete_call(task, task->incoming_message.cookie);
        goto error;
    }
    /* If the incoming message is a control message... */
    else if (task->incoming_message.flags & LWMSG_MESSAGE_FLAG_CONTROL)
    {
        switch (task->incoming_message.status)
        {
        case LWMSG_STATUS_CANCELLED:
            /* The caller cancelled a previous call, so find it and cancel it here as well */
            lwmsg_peer_task_cancel_call(task, task->incoming_message.cookie);
            lwmsg_assoc_destroy_message(task->assoc, &task->incoming_message);
            goto error;
        default:
            /* Silently drop unknown control messages */
            lwmsg_assoc_destroy_message(task->assoc, &task->incoming_message);
            goto error;
        }
    }

    lwmsg_peer_log_message(task, &task->incoming_message, LWMSG_FALSE);

    /* Check if this cookie is already in use */
    if (lwmsg_hash_find_key(&task->incoming_calls, &task->incoming_message.cookie))
    {
        /* This association is toast */
        status = lwmsg_peer_task_handle_assoc_error(
            peer,
            task,
            LWMSG_STATUS_MALFORMED);
        lwmsg_assoc_destroy_message(task->assoc, &task->incoming_message);
        goto error;
    }

    /* Create a call structure to track call */
    BAIL_ON_ERROR(status = lwmsg_peer_call_new(task, &call));
    call->base.is_outgoing = LWMSG_FALSE;
    call->params.incoming.in.tag = LWMSG_TAG_INVALID;
    call->params.incoming.out.tag = LWMSG_TAG_INVALID;

    /* The call structure holds a reference to the session */
    task->refs++;

    /* Set cookie and insert into incoming call table */
    call->cookie = task->incoming_message.cookie;
    lwmsg_hash_insert_entry(&task->incoming_calls, call);

    /* If the incoming message is synthetic for some bizarre reason,
       just echo back the status code to the caller */
    if (task->incoming_message.flags & LWMSG_MESSAGE_FLAG_SYNTHETIC)
    {
        status = lwmsg_peer_task_handle_call_error(
            task,
            call,
            task->incoming_message.status);
        lwmsg_assoc_destroy_message(task->assoc, &task->incoming_message);
        goto error;
    }

    /* Make sure the tag is within the bounds of the dispatch vector */
    if (tag < 0 || tag >= peer->dispatch.vector_length)
    {
        status = lwmsg_peer_task_handle_call_error(
            task,
            call,
            LWMSG_STATUS_MALFORMED);
        lwmsg_assoc_destroy_message(task->assoc, &task->incoming_message);
        goto error;
    }
    
    /* Make sure the call is supported */
    spec = peer->dispatch.vector[tag];
    if (spec == NULL)
    {
        status = lwmsg_peer_task_handle_call_error(
            task,
            call,
            LWMSG_STATUS_UNSUPPORTED);
        lwmsg_assoc_destroy_message(task->assoc, &task->incoming_message);
        goto error;
    }

    /* Dispatch the call */
    status = lwmsg_peer_call_dispatch_incoming(
        call,
        spec,
        peer->dispatch_data,
        &task->incoming_message);
    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        break;
    case LWMSG_STATUS_PENDING:
        status = LWMSG_STATUS_SUCCESS;
        break;
    default:
        status = lwmsg_peer_task_handle_call_error(task, call, status);
        BAIL_ON_ERROR(status);
        break;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_peer_task_run_finish(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int fd = CONNECTION_PRIVATE(task->assoc)->fd;

#if 0
    /* When the peer runs out of available client slots, it will
       wake all of its tasks so that it can begin enforcing timeouts
       where it previously did not bother.  Tell the task manager to
       begin waking us up on timeout events if this is the case */
    if ((trigger & LWMSG_TASK_TRIGGER_EXPLICIT) &&
        lwmsg_peer_task_subject_to_timeout(peer, task, trigger))
    {
        *next_trigger |= LWMSG_TASK_TRIGGER_TIME;
    }
#endif

    /* Did we hit a timeout? */
    if (trigger & LWMSG_TASK_TRIGGER_TIME)
    {
        BAIL_ON_ERROR(status = lwmsg_peer_task_handle_assoc_error(
                          peer,
                          task,
                          LWMSG_STATUS_TIMEOUT));
    }
    /* Are we shutting down? */
    else if (trigger & LWMSG_TASK_TRIGGER_CANCEL)
    {
        /* Drop the task unceremoniously in the interest
           of shutting down quickly */
        task->type = PEER_TASK_DROP;
    }
    /* Is the task unblocked? */
    else if (!task->recv_blocked || !task->send_blocked)
    {
        switch (task->type)
        {
        case PEER_TASK_FINISH_CLOSE:
        case PEER_TASK_FINISH_RESET:
            /* We are going to close the fd, so unset it on task */
            if (fd >= 0)
            {
                lwmsg_task_unset_trigger_fd(task->event_task, fd);
            }
            break;
        default:
            break;
        }

        status = lwmsg_assoc_finish(task->assoc, NULL);
        
        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            switch (task->type)
            {
            case PEER_TASK_FINISH_ACCEPT:
                BAIL_ON_ERROR(status = lwmsg_peer_log_accept(task));
                task->type = PEER_TASK_DISPATCH;
                break;
            case PEER_TASK_FINISH_CONNECT:
                BAIL_ON_ERROR(status = lwmsg_peer_log_connect(task));
                task->type = PEER_TASK_DISPATCH;
                break;
            case PEER_TASK_FINISH_CLOSE:
            case PEER_TASK_FINISH_RESET:
                task->type = PEER_TASK_DROP;
                break;
            default:
                break;
            }
            break;
        case LWMSG_STATUS_PENDING:
            switch (task->type)
            {
            case PEER_TASK_FINISH_CLOSE:
            case PEER_TASK_FINISH_RESET:
                /* Nevermind, set the fd on the task again */
                if (fd >= 0)
                {
                    BAIL_ON_ERROR(status = lwmsg_task_set_trigger_fd(task->event_task, fd));
                }
                break;
            default:
                break;
            }
            break;
        default:
            BAIL_ON_ERROR(status = lwmsg_peer_task_handle_assoc_error(
                              peer,
                              task,
                              status));
            break;
        }
    }
    /* Nothing to do */
    else
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_PENDING);
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_peer_task_finish_transceive(
    LWMsgPeer* peer,
    PeerAssocTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_PENDING;
    LWMsgMessage* message = NULL;

    while ((!task->recv_blocked && task->incoming) ||
           (!task->send_blocked && task->outgoing))
    {
        status = lwmsg_assoc_finish(task->assoc, &message);
        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            if (message == &task->incoming_message)
            {
                task->incoming = LWMSG_FALSE;
                task->recv_partial = LWMSG_FALSE;
                BAIL_ON_ERROR(status = lwmsg_peer_task_dispatch_incoming_message(task));
            }
            else if (message == &task->outgoing_message)
            {
                lwmsg_peer_log_message(task, &task->outgoing_message, LWMSG_FALSE);
                task->outgoing = LWMSG_FALSE;
                if (task->destroy_outgoing)
                {
                    lwmsg_assoc_destroy_message(task->assoc, &task->outgoing_message);
                    task->destroy_outgoing = LWMSG_FALSE;
                }
            }
            break;
        case LWMSG_STATUS_PENDING:
            lwmsg_peer_task_update_blocked(task);
            break;
        default:
            BAIL_ON_ERROR(status);
        }
    }
    
error:

    return status;
}

static
LWMsgStatus
lwmsg_peer_task_dispatch_calls(
    PeerAssocTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_PENDING;
    PeerCall* call = NULL;
    LWMsgCookie cookie = 0;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;
    LWMsgRing* ring = NULL;
    LWMsgRing* next = NULL;
    LWMsgSessionString sessid = {0};

    for (ring = task->active_outgoing_calls.next;
         ring != &task->active_outgoing_calls;
         ring = next)
    {
        next = ring->next;
        lwmsg_ring_remove(ring);

        call = LWMSG_OBJECT_FROM_MEMBER(ring, PeerCall, queue_ring);

        /* Undispatched outgoing call -- send request*/
        if (!(call->state & PEER_CALL_DISPATCHED))
        {
            call->state |= PEER_CALL_DISPATCHED;
            
            lwmsg_message_init(&task->outgoing_message);
            task->outgoing_message.tag = call->params.outgoing.in->tag;
            task->outgoing_message.data = call->params.outgoing.in->data;
            task->outgoing_message.cookie = call->cookie;
            task->outgoing_message.status = call->status;
            
            lwmsg_peer_log_message(task, &task->outgoing_message, LWMSG_TRUE);
            status = lwmsg_assoc_send_message(task->assoc, &task->outgoing_message);
            switch (status)
            {
            case LWMSG_STATUS_SUCCESS:
                break;
            case LWMSG_STATUS_PENDING:
                task->outgoing = LWMSG_TRUE;
                BAIL_ON_ERROR(status);
                break;
            case LWMSG_STATUS_MALFORMED:
            case LWMSG_STATUS_INVALID_HANDLE:
                /* The association should still be usable, so just fail the call */
                lwmsg_message_init(&message);
                message.status = status;
                message.flags = LWMSG_MESSAGE_FLAG_REPLY | LWMSG_MESSAGE_FLAG_SYNTHETIC;
                message.cookie = call->cookie;
                lwmsg_peer_call_complete_outgoing(call, &message);
                status = LWMSG_STATUS_SUCCESS;
                break;
            default:
                BAIL_ON_ERROR(status);
            }
        }
        /* Cancelled outgoing call -- send cancel request */
        else if ((call->state & PEER_CALL_DISPATCHED) &&
                 (call->state & PEER_CALL_CANCELLED) &&
                 call->status != LWMSG_STATUS_CANCELLED)
        {
            call->status = LWMSG_STATUS_CANCELLED;
            
            lwmsg_message_init(&task->outgoing_message);
            task->outgoing_message.flags = LWMSG_MESSAGE_FLAG_CONTROL;
            task->outgoing_message.status = LWMSG_STATUS_CANCELLED;
            task->outgoing_message.cookie = call->cookie;
            
            lwmsg_peer_log_message(task, &task->outgoing_message, LWMSG_TRUE);
            status = lwmsg_assoc_send_message(task->assoc, &task->outgoing_message);
            switch (status)
            {
            case LWMSG_STATUS_SUCCESS:
                break;
            case LWMSG_STATUS_PENDING:
                task->outgoing = LWMSG_TRUE;
                BAIL_ON_ERROR(status);
                break;
            default:
                BAIL_ON_ERROR(status);
            }
        }
    }

    for (ring = task->active_incoming_calls.next;
         ring != &task->active_incoming_calls;
         ring = next)
    {
        next = ring->next;
        lwmsg_ring_remove(ring);

        call = LWMSG_OBJECT_FROM_MEMBER(ring, PeerCall, queue_ring);

        /* Completed incoming call -- send reply */
        if ((call->state & PEER_CALL_COMPLETED) &&
            (call->state & PEER_CALL_DISPATCHED))
        {
            /* Trace call completion */
            if (call->task->session->peer->trace_end)
            {
                call->task->session->peer->trace_end(
                    LWMSG_CALL(call),
                    &call->params.incoming.out,
                    call->status,
                    call->task->session->peer->trace_data);
            }

            lwmsg_message_init(&task->outgoing_message);
            task->outgoing_message.flags = LWMSG_MESSAGE_FLAG_REPLY;
            task->outgoing_message.tag = call->params.incoming.out.tag;
            task->outgoing_message.data = call->params.incoming.out.data;
            task->outgoing_message.cookie = call->cookie;
            task->outgoing_message.status = call->status;
            
            cookie = call->cookie;
            
            /* It's now safe to destroy the incoming call parameters */
            message.tag = call->params.incoming.in.tag;
            message.data = call->params.incoming.in.data;
            lwmsg_assoc_destroy_message(task->assoc, &message);
            
            /* Delete the call structure */
            lwmsg_hash_remove_entry(&task->incoming_calls, call);
            lwmsg_peer_call_delete(call);
            
            do
            {
                status = lwmsg_assoc_send_message(task->assoc, &task->outgoing_message);
                switch (status)
                {
                case LWMSG_STATUS_SUCCESS:
                    lwmsg_peer_log_message(task, &task->outgoing_message, LWMSG_FALSE);
                    lwmsg_assoc_destroy_message(task->assoc, &task->outgoing_message);
                    break;
                case LWMSG_STATUS_PENDING:
                    task->outgoing = LWMSG_TRUE;
                    task->destroy_outgoing = LWMSG_TRUE;
                    BAIL_ON_ERROR(status);
                    break;
                case LWMSG_STATUS_MALFORMED:
                case LWMSG_STATUS_INVALID_HANDLE:
                case LWMSG_STATUS_OVERFLOW:
                case LWMSG_STATUS_UNDERFLOW:
                    /* Our reply could not be sent because the dispatch function gave us
                       bogus response parameters.  Send a synthetic reply instead so the
                       caller at least knows the call is complete */
                    lwmsg_peer_session_string_for_session((LWMsgSession*) task->session, sessid);
                    LWMSG_LOG_WARNING(task->session->peer->context,
                                      "(session:%s >> %u) Response payload could not be sent: %s",
                                      sessid,
                                      cookie,
                                      lwmsg_status_name(status));
                    lwmsg_assoc_destroy_message(task->assoc, &task->outgoing_message);
                    lwmsg_message_init(&task->outgoing_message);
                    task->outgoing_message.flags = LWMSG_MESSAGE_FLAG_REPLY | LWMSG_MESSAGE_FLAG_SYNTHETIC;
                    task->outgoing_message.status = status;
                    task->outgoing_message.cookie = cookie;
                    status = LWMSG_STATUS_AGAIN;
                    break;
                default:
                    BAIL_ON_ERROR(status);
                }
            } while (status == LWMSG_STATUS_AGAIN);
        }
    }

error:

    return status;
}   

static
LWMsgStatus
lwmsg_peer_task_run_dispatch(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTaskTrigger* trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgStatus finish_status = LWMSG_STATUS_PENDING;
    LWMsgStatus recv_status = LWMSG_STATUS_PENDING;
    LWMsgStatus send_status = LWMSG_STATUS_PENDING;
    LWMsgTime* timeout = NULL;

    pthread_mutex_lock(&task->session->lock);

    /* Did we hit a timeout? */
    if (*trigger & LWMSG_TASK_TRIGGER_TIME)
    {
        BAIL_ON_ERROR(status = lwmsg_peer_task_handle_assoc_error(
                          peer,
                          task,
                          LWMSG_STATUS_TIMEOUT));
    }
    /* Are we shutting down? */
    else if (*trigger & LWMSG_TASK_TRIGGER_CANCEL)
    {
        /* Drop the task unceremoniously in the interest
           of shutting down quickly */
        task->type = PEER_TASK_DROP;
    }
    else
    {
        /* Try to finish any outstanding sends/receives */
        finish_status = lwmsg_peer_task_finish_transceive(peer, task);
        switch (finish_status)
        {
        case LWMSG_STATUS_SUCCESS:
        case LWMSG_STATUS_PENDING:
            break;
        default:
            BAIL_ON_ERROR(status = finish_status);
        }

        /* Try to receive and dispatch the next message */
        if (!task->incoming && !task->recv_blocked)
        {
            recv_status = lwmsg_assoc_recv_message(task->assoc, &task->incoming_message);
            
            switch (recv_status)
            {
            case LWMSG_STATUS_SUCCESS:
                task->recv_partial = LWMSG_FALSE;
                BAIL_ON_ERROR(status = lwmsg_peer_task_dispatch_incoming_message(task));
                break;
            case LWMSG_STATUS_PENDING:
                task->incoming = LWMSG_TRUE;
                break;
            default:
                BAIL_ON_ERROR(status = recv_status);
            }
        }

        /* Try to send any outgoing call requests or incoming call replies */
        if (!task->outgoing && !task->send_blocked)
        {           
            send_status = lwmsg_peer_task_dispatch_calls(task);
            switch (send_status)
            {
            case LWMSG_STATUS_SUCCESS:
            case LWMSG_STATUS_PENDING:
                break;
            default:
                BAIL_ON_ERROR(status = send_status);
            }
        }

        /* If we could not make any progress finishing pended I/O,
           dispatching outgoing calls or incoming call replies,
           or receiving new calls, then we should go to sleep */
        if (finish_status == LWMSG_STATUS_PENDING &&
            send_status == LWMSG_STATUS_PENDING &&
            recv_status == LWMSG_STATUS_PENDING)
        {
            if ((!task->incoming || !task->recv_partial) && !task->outgoing)
            {
                timeout = &peer->timeout.idle;
            }
            else
            {
                timeout = &peer->timeout.message;
            }
            lwmsg_peer_task_set_timeout(peer, task, timeout, *trigger, next_trigger, next_timeout);
            BAIL_ON_ERROR(status = LWMSG_STATUS_PENDING);
        }
    }

error:

    pthread_mutex_unlock(&task->session->lock);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
    case LWMSG_STATUS_PENDING:
        break;
    default:
        status = lwmsg_peer_task_handle_assoc_error(
            peer,
            task,
            status);
        break;
    }
    
    return status;
}

static
LWMsgStatus
lwmsg_peer_task_run_drop(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int fd = CONNECTION_PRIVATE(task->assoc)->fd;

    BAIL_ON_ERROR(status = lwmsg_peer_task_rundown(peer, task, trigger, next_trigger, next_timeout));

    if (fd >= 0)
    {
        lwmsg_task_unset_trigger_fd(task->event_task, fd);
    }

    task->type = PEER_TASK_DONE;

error:

    return status;
}

static
void
lwmsg_peer_task_run(
    LWMsgTask* _task,
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTaskTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerAssocTask* task = (PeerAssocTask*) data;
    LWMsgPeer* peer = task->session->peer;
    LWMsgTime my_timeout = {-1, -1};
    siginfo_t siginfo = {0};

    if (*next_timeout >= 0)
    {
        my_timeout.seconds = *next_timeout / 1000000000ll;
        my_timeout.microseconds = (*next_timeout % 1000000000ll) / 1000;
    }

    if (trigger & LWMSG_TASK_TRIGGER_INIT)
    {
        BAIL_ON_ERROR(status = lwmsg_task_set_unix_signal(
            _task,
            SIGUSR1,
            LWMSG_TRUE));
    }

    if (trigger & LWMSG_TASK_TRIGGER_UNIX_SIGNAL)
    {
        while (lwmsg_task_next_unix_signal(_task, &siginfo))
        {
            if (siginfo.si_signo == SIGUSR1 && task->assoc)
            {
                pthread_mutex_lock(&task->session->lock);
                BAIL_ON_ERROR(status = lwmsg_peer_log_state(task));
                pthread_mutex_unlock(&task->session->lock);
            }
        }
    }

    if (trigger & LWMSG_TASK_TRIGGER_FD_READABLE)
    {
        task->recv_blocked = LWMSG_FALSE;
        task->recv_partial = LWMSG_TRUE;
    }

    if (trigger & LWMSG_TASK_TRIGGER_FD_WRITABLE)
    {
        task->send_blocked = LWMSG_FALSE;
    }

    *next_trigger = LWMSG_TASK_TRIGGER_EXPLICIT;

    switch (task->type)
    {
    case PEER_TASK_BEGIN_ACCEPT:
        BAIL_ON_ERROR(status = lwmsg_peer_task_run_accept(
            peer,
            task,
            trigger,
            next_trigger,
            &my_timeout));
        break;
    case PEER_TASK_BEGIN_CONNECT:
        BAIL_ON_ERROR(status = lwmsg_peer_task_run_connect(
            peer,
            task,
            trigger,
            next_trigger,
            &my_timeout));
        break;
    case PEER_TASK_DISPATCH:
        BAIL_ON_ERROR(status = lwmsg_peer_task_run_dispatch(
            peer,
            task,
            &trigger,
            next_trigger,
            &my_timeout));
        break;
    case PEER_TASK_BEGIN_CLOSE:
    case PEER_TASK_BEGIN_RESET:
        BAIL_ON_ERROR(status = lwmsg_peer_task_run_shutdown(
            peer,
            task,
            trigger,
            next_trigger,
            &my_timeout));
        break;
    case PEER_TASK_FINISH_ACCEPT:
    case PEER_TASK_FINISH_CONNECT:
    case PEER_TASK_FINISH_CLOSE:
    case PEER_TASK_FINISH_RESET:
        BAIL_ON_ERROR(status = lwmsg_peer_task_run_finish(
            peer,
            task,
            trigger,
            next_trigger,
            &my_timeout));
        break;
    case PEER_TASK_DROP:
        BAIL_ON_ERROR(status = lwmsg_peer_task_run_drop(
            peer,
            task,
            trigger,
            next_trigger,
            &my_timeout));
        break;
    case PEER_TASK_DONE:
        lwmsg_peer_task_release(task);
        *next_trigger = 0;
        return;
    default:
        break;
    }

error:

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
    case LWMSG_STATUS_PENDING:
        lwmsg_peer_task_update_blocked(task);

        if (task->send_blocked)
        {
            *next_trigger |= LWMSG_TASK_TRIGGER_FD_WRITABLE;
        }

        if (task->recv_blocked)
        {
            *next_trigger |= LWMSG_TASK_TRIGGER_FD_READABLE;
        }

        if (status == LWMSG_STATUS_SUCCESS)
        {
            *next_trigger |= LWMSG_TASK_TRIGGER_YIELD;
        }

        *next_trigger |= LWMSG_TASK_TRIGGER_UNIX_SIGNAL;
        break;
    case LWMSG_STATUS_CANCELLED:
        task->type = PEER_TASK_DONE;
        *next_trigger = LWMSG_TASK_TRIGGER_YIELD;
        break;
    default:
        LWMSG_LOG_ERROR(peer->context, "Caught error: %s (%i)", lwmsg_status_name(status), status);

        task->type = PEER_TASK_DONE;
        *next_trigger = LWMSG_TASK_TRIGGER_YIELD;
        break;
    }

    *next_timeout = lwmsg_time_is_positive(&my_timeout) ?
        my_timeout.seconds * 1000000000ll + my_timeout.microseconds * 1000
        : 0;
}
