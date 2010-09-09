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

#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>

static
LWMsgStatus
lwmsg_peer_task_run(
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_wakeup
    );

static
LWMsgStatus
lwmsg_peer_task_run_listen(
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    );

static
void
lwmsg_peer_task_notify_status(
    PeerAssocTask* task,
    LWMsgStatus status
    )
{
    pthread_mutex_lock(&task->call_lock);
    task->status = status;
    pthread_cond_broadcast(&task->call_event);
    pthread_mutex_unlock(&task->call_lock);
}

static
void
lwmsg_peer_session_string_for_session(
    LWMsgSession* session,
    LWMsgSessionString string
    )
{
    const LWMsgSessionID* rsmid = NULL;

    rsmid = lwmsg_session_get_id(session);
    lwmsg_session_id_to_string(rsmid, string);
}

static
void
lwmsg_peer_session_string_for_assoc(
    LWMsgAssoc* assoc,
    LWMsgSessionString string
    )
{
    LWMsgSession* session = NULL;

    if (lwmsg_assoc_get_session(assoc, &session) == LWMSG_STATUS_SUCCESS)
    {
        lwmsg_peer_session_string_for_session(session, string);
    }
    else
    {
        strncpy(string, "<null session>", sizeof (string));
    }
}

static
LWMsgStatus
lwmsg_peer_log_message(
    LWMsgPeer* peer,
    LWMsgAssoc* assoc,
    LWMsgMessage* message,
    LWMsgBool is_outgoing
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    const char* data_direction = NULL;
    const char* message_direction = NULL;
    const char* message_type = NULL;
    char* msg_text = NULL;
    const char* status_text = NULL;

    if (lwmsg_context_would_log(peer->context, LWMSG_LOGLEVEL_TRACE))
    {
        if (message->flags & LWMSG_MESSAGE_FLAG_REPLY)
        {
            if (message->flags & LWMSG_MESSAGE_FLAG_SYNTHETIC)
            {
                message_direction = "SRS";
            }
            else
            {
                message_direction = "RES";
            }
        }
        else
        {
            if (message->flags & LWMSG_MESSAGE_FLAG_SYNTHETIC)
            {
                message_direction = "SRQ";
            }
            else
            {
                message_direction = "REQ";
            }
        }

        if (message->flags & LWMSG_MESSAGE_FLAG_CONTROL)
        {
            message_type = "CTRL";
        }
        else
        {
            message_type = "CALL";
        }

        if (is_outgoing)
        {
            data_direction = "<<";
        }
        else
        {
            data_direction = ">>";
        }
    
        if (message->status != LWMSG_STATUS_SUCCESS)
        {
            status_text = lwmsg_string_without_prefix(
                lwmsg_error_name(message->status),
                "LWMSG_STATUS_");
        }

        if (message->tag != LWMSG_TAG_INVALID)
        {
            BAIL_ON_ERROR(lwmsg_assoc_print_message_alloc(assoc, message, &msg_text));
        }

        if (msg_text)
        {
            if (status_text)
            {
                LWMSG_LOG_TRACE(
                    peer->context,
                    "(assoc:0x%lx %s %u) %s %s [%s] %s",
                    LWMSG_POINTER_AS_ULONG(assoc),
                    data_direction,
                    message->cookie,
                    message_type,
                    message_direction,
                    status_text,
                    msg_text);
            }
            else
            {
                LWMSG_LOG_TRACE(
                    peer->context,
                    "(assoc:0x%lx %s %u) %s %s %s",
                    LWMSG_POINTER_AS_ULONG(assoc),
                    data_direction,
                    message->cookie,
                    message_type,
                    message_direction,
                    msg_text);
            }
        }
        else
        {
            if (status_text)
            {
                LWMSG_LOG_TRACE(
                    peer->context,
                    "(assoc:0x%lx %s %u) %s %s [%s]",
                    LWMSG_POINTER_AS_ULONG(assoc),
                    data_direction,
                    message->cookie,
                    message_type,
                    message_direction,
                    status_text);
            }
            else
            {
                LWMSG_LOG_TRACE(
                    peer->context,
                    "(assoc:0x%lx %s %u) %s %s",
                    LWMSG_POINTER_AS_ULONG(assoc),
                    data_direction,
                    message->cookie,
                    message_type,
                    message_direction);
            }
        }
    }

cleanup:

    if (msg_text)
    {
        lwmsg_context_free(peer->context, msg_text);
    }

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
lwmsg_peer_log_accept(
    LWMsgPeer* peer,
    LWMsgAssoc* assoc
    )
{
    LWMsgSessionString buffer;

    if (lwmsg_context_would_log(peer->context, LWMSG_LOGLEVEL_VERBOSE))
    {
        lwmsg_peer_session_string_for_assoc(assoc, buffer);
            
        LWMSG_LOG_VERBOSE(peer->context, "(session:%s) Accepted association 0x%lx",
                          buffer, (unsigned long) (size_t) assoc);
    }

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
lwmsg_peer_log_connect(
    LWMsgPeer* peer,
    LWMsgAssoc* assoc
    )
{
    LWMsgSessionString buffer;

    if (lwmsg_context_would_log(peer->context, LWMSG_LOGLEVEL_VERBOSE))
    {
        lwmsg_peer_session_string_for_assoc(assoc, buffer);
            
        LWMSG_LOG_VERBOSE(peer->context, "(session:%s) Connected association 0x%lx",
                          buffer, (unsigned long) (size_t) assoc);
    }

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgTaskTrigger
lwmsg_peer_task_assoc_trigger(
    LWMsgAssoc* assoc
    )
{
    switch (lwmsg_assoc_get_state(assoc))
    {
    case LWMSG_ASSOC_STATE_IDLE:
        return LWMSG_TASK_TRIGGER_FD_READABLE;
    case LWMSG_ASSOC_STATE_BLOCKED_SEND:
        return LWMSG_TASK_TRIGGER_FD_WRITABLE;
    case LWMSG_ASSOC_STATE_BLOCKED_RECV:
        return LWMSG_TASK_TRIGGER_FD_READABLE;
    case LWMSG_ASSOC_STATE_BLOCKED_SEND_RECV:
        return LWMSG_TASK_TRIGGER_FD_READABLE | LWMSG_TASK_TRIGGER_FD_WRITABLE;
    default:
        return 0;
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
        lwmsg_peer_release_client_slot(task->peer);
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

    pthread_mutex_destroy(&task->call_lock);
    pthread_cond_destroy(&task->call_event);

    free(task);
}

void
lwmsg_peer_task_cancel_and_unref(
    PeerAssocTask* task
    )
{
    lwmsg_task_cancel(task->event_task);
    lwmsg_peer_task_unref(task);
}

void
lwmsg_peer_task_unref(
    PeerAssocTask* task
    )
{
    LWMsgBool delete = LWMSG_FALSE;
    lwmsg_peer_lock(task->peer);
    if (--task->refcount == 0)
    {
        delete = LWMSG_TRUE;
    }
    lwmsg_peer_unlock(task->peer);

    if (delete)
    {
        lwmsg_peer_task_delete(task);
    }
}

void
lwmsg_peer_task_ref(
    PeerAssocTask* task
    )
{
    lwmsg_peer_lock(task->peer);
    ++task->refcount;
    lwmsg_peer_unlock(task->peer);
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

LWMsgStatus
lwmsg_peer_assoc_task_new(
    LWMsgPeer* peer,
    LWMsgTaskGroup* group,
    PeerAssocTaskType type,
    PeerAssocTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerAssocTask* my_task = NULL;
    pthread_mutexattr_t mutex_attr;
    LWMsgBool free_mutexattr = LWMSG_FALSE;
    LWMsgBool free_mutex = LWMSG_FALSE;
    LWMsgBool free_cond = LWMSG_FALSE;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_task));

    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutexattr_init(&mutex_attr)));
    free_mutexattr = LWMSG_TRUE;
    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE)));
    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutex_init(&my_task->call_lock, &mutex_attr)));
    free_mutex = LWMSG_TRUE;
    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_cond_init(&my_task->call_event, NULL)));
    free_cond = LWMSG_TRUE;

    BAIL_ON_ERROR(status = lwmsg_task_new(
                      peer->task_manager,
                      group,
                      lwmsg_peer_task_run,
                      my_task,
                      &my_task->event_task));

    my_task->peer = peer;
    my_task->type = type;
    my_task->refcount = 2;

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

    if (free_mutexattr)
    {
        pthread_mutexattr_destroy(&mutex_attr);
    }

    if (status && my_task)
    {
        if (!free_mutex || !free_cond)
        {
            if (free_mutex)
            {
                pthread_mutex_destroy(&my_task->call_lock);
            }
            
            if (free_cond)
            {
                pthread_cond_destroy(&my_task->call_event);
            }

            free(my_task);
        }
        else
        {
            lwmsg_peer_task_delete(my_task);
        }
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
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(errno));
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
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(errno));
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
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(errno));
    }
    
    /* Set non-blocking flag */
    opts |= O_NONBLOCK;
    
    /* Set socket flags */
    if (fcntl(task->fd, F_SETFL, opts) < 0)
    {
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(errno));
    }
    
    if (listen(task->fd, task->peer->max_backlog))
    {
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(errno));
    }
    
    if (task->endpoint)
    {
        LWMSG_LOG_INFO(task->peer->context, "Listening on endpoint %s", task->endpoint);
    }
    else
    {
        LWMSG_LOG_INFO(task->peer->context, "Listening on fd %i", task->fd);  
    }

    lwmsg_task_set_trigger_fd(task->event_task, task->fd);

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

static
LWMsgStatus
lwmsg_peer_assoc_task_new_accept(
    LWMsgPeer* peer,
    LWMsgAssoc* assoc,
    PeerAssocTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerAssocTask* my_task = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_peer_assoc_task_new(peer, peer->listen_tasks, PEER_TASK_BEGIN_ACCEPT, &my_task));

    my_task->assoc = assoc;

    *task = my_task;

error:

    return status;
}

LWMsgStatus
lwmsg_peer_assoc_task_new_connect(
    LWMsgPeer* peer,
    LWMsgAssoc* assoc,
    LWMsgSession* session,
    PeerAssocTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerAssocTask* my_task = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_peer_assoc_task_new(peer, peer->connect_tasks, PEER_TASK_BEGIN_CONNECT, &my_task));

    my_task->assoc = assoc;
    my_task->session = session;

    *task = my_task;

error:

    return status;
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
        handle_count = lwmsg_session_get_handle_count(task->session);
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
    *next_trigger = lwmsg_peer_task_assoc_trigger(task->assoc);

    if (lwmsg_time_is_positive(timeout) &&
        lwmsg_peer_task_subject_to_timeout(peer, task, trigger))
    {
        *next_trigger |= LWMSG_TASK_TRIGGER_TIME;
    }

    lwmsg_task_set_trigger_fd(task->event_task, CONNECTION_PRIVATE(task->assoc)->fd);
}

static
LWMsgStatus
lwmsg_peer_task_handle_assoc_error(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgStatus status
    )
{
    if (status)
    {
        pthread_mutex_lock(&task->call_lock);
        task->status = status;
        pthread_cond_broadcast(&task->call_event);
        pthread_mutex_unlock(&task->call_lock);
    }

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        break;
    case LWMSG_STATUS_CONNECTION_REFUSED:
    case LWMSG_STATUS_PEER_CLOSE:
    case LWMSG_STATUS_PEER_RESET:
    case LWMSG_STATUS_PEER_ABORT:
        LWMSG_LOG_VERBOSE(peer->context, "(assoc:0x%lx) Dropping: %s",
                          LWMSG_POINTER_AS_ULONG(task->assoc),
                          lwmsg_assoc_get_error_message(task->assoc, status));
        task->type = PEER_TASK_DROP;
        status = LWMSG_STATUS_SUCCESS;
        break;
    case LWMSG_STATUS_TIMEOUT:
        LWMSG_LOG_VERBOSE(peer->context, "(assoc:0x%lx) Resetting: %s",
                          LWMSG_POINTER_AS_ULONG(task->assoc),
                          lwmsg_assoc_get_error_message(task->assoc, status));
        task->type = PEER_TASK_BEGIN_RESET;
        status = LWMSG_STATUS_SUCCESS;
    default:
        LWMSG_LOG_VERBOSE(peer->context, "(assoc:0x%lx) Dropping: %s",
                          LWMSG_POINTER_AS_ULONG(task->assoc),
                          lwmsg_assoc_get_error_message(task->assoc, status));
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
    call->cookie = task->incoming_message.cookie;
    call->status = status;
    call->state = PEER_CALL_DISPATCHED | PEER_CALL_COMPLETED;
    call->params.incoming.out.tag = LWMSG_TAG_INVALID;
    call->params.incoming.out.data = NULL;

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
lwmsg_peer_task_run_listen(
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerListenTask* task = data;
    int client_fd = -1;
    LWMsgAssoc* assoc = NULL;
    PeerAssocTask* client_task = NULL;
    LWMsgBool slot = LWMSG_FALSE;
    int err = 0;
    
    while (status == LWMSG_STATUS_SUCCESS)
    {
        assoc = NULL;
        client_task = NULL;
        slot = LWMSG_FALSE;
        client_fd = -1;
        
        if (trigger & LWMSG_TASK_TRIGGER_CANCEL)
        {
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
                        BAIL_ON_ERROR(status = lwmsg_error_map_errno(err));
                    }
                }
            } while (client_fd < 0);
            
            BAIL_ON_ERROR(status = lwmsg_set_close_on_exec(client_fd));
            
            /* Create new connection with client fd, put it into task, schedule task */
            BAIL_ON_ERROR(status = lwmsg_connection_new(task->peer->context, task->peer->protocol, &assoc));
            BAIL_ON_ERROR(status = lwmsg_connection_set_fd(assoc, LWMSG_CONNECTION_MODE_LOCAL, client_fd));
            BAIL_ON_ERROR(status = lwmsg_assoc_set_nonblock(assoc, LWMSG_TRUE));
            BAIL_ON_ERROR(status = lwmsg_peer_assoc_task_new_accept(task->peer, assoc, &client_task));
            assoc = NULL;
            slot = LWMSG_FALSE;
            
            /* Release a reference so the task will be freed immediately
               when it is cancelled */
            lwmsg_peer_task_unref(client_task);
            lwmsg_task_wake(client_task->event_task);
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

    return LWMSG_STATUS_SUCCESS;

error:
    
    if (client_fd >= 0 && !assoc)
    {
        close(client_fd);
    }
    
    if (assoc)
    {
        lwmsg_assoc_delete(assoc);
    }
    
    if (client_task)
    {
        lwmsg_peer_task_cancel_and_unref(client_task);
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

    status = lwmsg_assoc_accept(task->assoc, peer->session_manager, &task->session);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        task->type = PEER_TASK_DISPATCH;
        BAIL_ON_ERROR(status = lwmsg_peer_log_accept(peer, task->assoc));
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

    status = lwmsg_assoc_connect(task->assoc, task->session);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        task->type = PEER_TASK_DISPATCH;
        BAIL_ON_ERROR(status = lwmsg_peer_log_connect(peer, task->assoc));
        lwmsg_peer_task_notify_status(task, LWMSG_STATUS_SUCCESS);
        break;
    case LWMSG_STATUS_PENDING:
        task->type = PEER_TASK_FINISH_CONNECT;
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

    pthread_mutex_lock(&task->call_lock);

    lwmsg_hash_iter_begin(&task->incoming_calls, &iter);
    while ((call = lwmsg_hash_iter_next(&task->incoming_calls, &iter)))
    {
        if (call->state & PEER_CALL_COMPLETED)
        {
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

    pthread_mutex_unlock(&task->call_lock);

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

    /* Make sure all calls are run down before we close the association because it
       will destroy the session and thus any handles that are open */
    BAIL_ON_ERROR(status = lwmsg_peer_task_rundown(peer, task, trigger, next_trigger, next_timeout));

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
        task->type = PEER_TASK_FINISH_CLOSE;
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
        lwmsg_peer_log_message(task->peer, task->assoc, &task->incoming_message, LWMSG_FALSE);
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
        lwmsg_peer_log_message(task->peer, task->assoc, &task->incoming_message, LWMSG_TRUE);
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
    LWMsgPeer* peer = task->peer;
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

    lwmsg_peer_log_message(peer, task->assoc, &task->incoming_message, LWMSG_FALSE);

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
    /* The call structure holds a reference to the task */
    lwmsg_peer_task_ref(task);

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
        /* Message data is now owned by the call parameters */
        lwmsg_message_init(&task->incoming_message);
        break;
    case LWMSG_STATUS_PENDING:
        /* Message data is now owned by the call parameters */
        lwmsg_message_init(&task->incoming_message);
        status = LWMSG_STATUS_SUCCESS;
        break;
    default:
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

    /* When the peer runs out of available client slots, it will
       wake all of its tasks so that it can begin enforcing timeouts
       where it previously did not bother.  Tell the task manager to
       begin waking us up on timeout events if this is the case */
    if ((trigger & LWMSG_TASK_TRIGGER_EXPLICIT) &&
        lwmsg_peer_task_subject_to_timeout(peer, task, trigger))
    {
        *next_trigger |= LWMSG_TASK_TRIGGER_TIME;
    }

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
    else if (trigger & lwmsg_peer_task_assoc_trigger(task->assoc))
    {
        status = lwmsg_assoc_finish(task->assoc, NULL);
        
        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            switch (task->type)
            {
            case PEER_TASK_FINISH_ACCEPT:
                BAIL_ON_ERROR(status = lwmsg_peer_log_accept(peer, task->assoc));
                task->type = PEER_TASK_DISPATCH;
                break;
            case PEER_TASK_FINISH_CONNECT:
                BAIL_ON_ERROR(status = lwmsg_peer_log_connect(peer, task->assoc));
                lwmsg_peer_task_notify_status(task, LWMSG_STATUS_SUCCESS);
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
    PeerAssocTask* task,
    LWMsgTaskTrigger* trigger
    )
{
    LWMsgStatus status = LWMSG_STATUS_PENDING;
    LWMsgMessage* message = NULL;

    while ((*trigger & LWMSG_TASK_TRIGGER_FD_READABLE && task->incoming) ||
           (*trigger & LWMSG_TASK_TRIGGER_FD_WRITABLE && task->outgoing))
    {
        status = lwmsg_assoc_finish(task->assoc, &message);
        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            if (message == &task->incoming_message)
            {
                task->incoming = LWMSG_FALSE;
                BAIL_ON_ERROR(status = lwmsg_peer_task_dispatch_incoming_message(task));
            }
            else if (message == &task->outgoing_message)
            {
                lwmsg_peer_log_message(task->peer, task->assoc, &task->outgoing_message, LWMSG_FALSE);
                task->outgoing = LWMSG_FALSE;
                if (task->destroy_outgoing)
                {
                    lwmsg_assoc_destroy_message(task->assoc, &task->outgoing_message);
                    task->destroy_outgoing = LWMSG_FALSE;
                }
            }
            break;
        case LWMSG_STATUS_PENDING:
            *trigger &= ~(LWMSG_TASK_TRIGGER_FD_READABLE | LWMSG_TASK_TRIGGER_FD_WRITABLE);
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
            
            lwmsg_peer_log_message(task->peer, task->assoc, &task->outgoing_message, LWMSG_TRUE);
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
            
            lwmsg_peer_log_message(task->peer, task->assoc, &task->outgoing_message, LWMSG_TRUE);
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
                    lwmsg_peer_log_message(task->peer, task->assoc, &task->outgoing_message, LWMSG_FALSE);
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
                    LWMSG_LOG_WARNING(task->peer->context,
                                      "(assoc:0x%lx >> %u) Response payload could not be sent: %s",
                                      LWMSG_POINTER_AS_ULONG(task->assoc),
                                      cookie,
                                      lwmsg_assoc_get_error_message(task->assoc, status));
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

    pthread_mutex_lock(&task->call_lock);

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
        finish_status = lwmsg_peer_task_finish_transceive(peer, task, trigger);
        switch (finish_status)
        {
        case LWMSG_STATUS_SUCCESS:
        case LWMSG_STATUS_PENDING:
            break;
        default:
            BAIL_ON_ERROR(status = finish_status);
        }

        /* Try to receive and dispatch the next message */
        if (!task->incoming && *trigger & LWMSG_TASK_TRIGGER_FD_READABLE)
        {
            recv_status = lwmsg_assoc_recv_message(task->assoc, &task->incoming_message);
            
            switch (recv_status)
            {
            case LWMSG_STATUS_SUCCESS:
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
        if (!task->outgoing)
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
            if (!task->incoming && !task->outgoing)
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

    pthread_mutex_unlock(&task->call_lock);

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

    BAIL_ON_ERROR(status = lwmsg_peer_task_rundown(peer, task, trigger, next_trigger, next_timeout));

    lwmsg_peer_task_unref(task);
    BAIL_ON_ERROR(status = LWMSG_STATUS_CANCELLED);

error:

    return status;
}

static
LWMsgStatus
lwmsg_peer_task_run(
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTime* next_timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerAssocTask* task = (PeerAssocTask*) data;
    LWMsgPeer* peer = task->peer;

    while (status == LWMSG_STATUS_SUCCESS)
    {
        switch (task->type)
        {
        case PEER_TASK_BEGIN_ACCEPT:
            BAIL_ON_ERROR(status = lwmsg_peer_task_run_accept(
                              peer,
                              task,
                              trigger,
                              next_trigger,
                              next_timeout));
            break;
        case PEER_TASK_BEGIN_CONNECT:
            BAIL_ON_ERROR(status = lwmsg_peer_task_run_connect(
                              peer,
                              task,
                              trigger,
                              next_trigger,
                              next_timeout));
            break;
        case PEER_TASK_DISPATCH:
            BAIL_ON_ERROR(status = lwmsg_peer_task_run_dispatch(
                              peer,
                              task,
                              &trigger,
                              next_trigger,
                              next_timeout));
            break;
        case PEER_TASK_BEGIN_CLOSE:
        case PEER_TASK_BEGIN_RESET:
            BAIL_ON_ERROR(status = lwmsg_peer_task_run_shutdown(
                              peer,
                              task,
                              trigger,
                              next_trigger,
                              next_timeout));
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
                              next_timeout));
            break;
        case PEER_TASK_DROP:
            BAIL_ON_ERROR(status = lwmsg_peer_task_run_drop(
                              peer,
                              task,
                              trigger,
                              next_trigger,
                              next_timeout));
            break;
        default:
            break;
        }
    }

error:

    switch (status)
    {
    case LWMSG_STATUS_CANCELLED:
        *next_trigger = 0;
        break;            
    case LWMSG_STATUS_PENDING:
        break;
    default:
        LWMSG_LOG_ERROR(peer->context, "Caught error: %s", lwmsg_error_name(status));
        if (peer->except)
        {
            peer->except(peer, status, peer->except_data);
        }
        break;
    }

    /* Don't cause the task manager to bail out
       under any circumstances */
    return LWMSG_STATUS_SUCCESS;
}
