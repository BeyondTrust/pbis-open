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
 *        peer-direct.c
 *
 * Abstract:
 *
 *        Peer direct call support
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>

#include <lwmsg/data.h>

#include "peer-private.h"
#include "call-private.h"
#include "util-private.h"
#include "connection-private.h"

#define LOCK_ENDPOINTS() (pthread_mutex_lock(&endpoint_lock))
#define UNLOCK_ENDPOINTS() (pthread_mutex_unlock(&endpoint_lock))
#define LOCK_SESSION(s) (pthread_mutex_lock(&(s)->session->lock))
#define UNLOCK_SESSION(s) (pthread_mutex_unlock(&(s)->session->lock))

static pthread_mutex_t endpoint_lock = PTHREAD_MUTEX_INITIALIZER;
static LWMsgRing endpoints = {&endpoints, &endpoints};

static
DirectEndpoint*
lwmsg_direct_find(
    const char* name
)
{
    LWMsgRing* iter = NULL;
    DirectEndpoint* endpoint = NULL;

    for (iter = endpoints.next; iter != &endpoints; iter = iter->next)
    {
        endpoint = LWMSG_OBJECT_FROM_MEMBER(iter, DirectEndpoint, ring);

        if (!strcmp(name, endpoint->name))
        {
            return endpoint;
        }
    }

    return NULL;
}

LWMsgStatus
lwmsg_direct_listen(
    const char* name,
    LWMsgPeer* server
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectEndpoint* endpoint = NULL;

    LOCK_ENDPOINTS();

    endpoint = lwmsg_direct_find(name);

    if (endpoint)
    {
        /* FIXME: better status code */
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
    }

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&endpoint));

    lwmsg_ring_init(&endpoint->ring);
    lwmsg_ring_init(&endpoint->sessions);

    endpoint->name = name;
    endpoint->server = server;

    lwmsg_ring_insert_before(&endpoints, &endpoint->ring);

error:

    UNLOCK_ENDPOINTS();

    return status;
}

static
void
lwmsg_direct_cancel_call_in_lock(
    DirectCall* call
    )
{
    if (!(call->state & DIRECT_CALL_CANCELED))
    {
        call->state |= DIRECT_CALL_CANCELED;
        if ((call->state & DIRECT_CALL_DISPATCHED) && call->cancel)
        {
            call->cancel(&call->callee, call->cancel_data);
        }
    }
}

static
void
lwmsg_direct_session_rundown_in_lock(
    DirectSession* session
    )
{
    LWMsgRing* iter = NULL;
    DirectCall* call = NULL;
    LWMsgRing calls;
    LWMsgPeer* server = session->endpoint->server;

    lwmsg_ring_init(&calls);

    /* This will prevent further calls from being allocated */
    session->endpoint = NULL;

    while (!lwmsg_ring_is_empty(&session->calls))
    {
        /* Move call onto local ring.
         * This is necessary because we release the
         * session lock while running a cancel function,
         * during which session->calls can change.  By
         * always dequeuing from the front of the list,
         * we can visit every call safely.
         */
        lwmsg_ring_dequeue(&session->calls, &iter);
        call = LWMSG_OBJECT_FROM_MEMBER(iter, DirectCall, ring);
        lwmsg_ring_enqueue(&calls, &call->ring);
        lwmsg_direct_cancel_call_in_lock(call);
    }

    /* Move remaining calls back */
    lwmsg_ring_move(&calls, &session->calls);

    /* Wait for calls to finish */
    while (!lwmsg_ring_is_empty(&session->calls))
    {
        pthread_cond_wait(&session->session->event, &session->session->lock);
    }

    /* Now we can run down all handles on the main session */
    lwmsg_peer_session_reset(session->session);

    /* Release session data */
    if (session->data && server->session_destruct)
    {
        server->session_destruct(session->token, session->data);
    }
}

void
lwmsg_direct_shutdown(
    const char* name,
    LWMsgPeer* server
    )
{
    DirectEndpoint* endpoint = NULL;
    LWMsgRing* iter = NULL;
    DirectSession* session = NULL;

    LOCK_ENDPOINTS();

    /* Find and remove endpoint from list to prevent further connections */
    endpoint = lwmsg_direct_find(name);
    LWMSG_ASSERT(endpoint && endpoint->server == server);
    lwmsg_ring_remove(&endpoint->ring);

    UNLOCK_ENDPOINTS();

    /* Run down all connected sessions */
    for (iter = endpoint->sessions.next; iter != &endpoint->sessions; iter = iter->next)
    {
        session = LWMSG_OBJECT_FROM_MEMBER(iter, DirectSession, ring);

        LOCK_SESSION(session);
        lwmsg_direct_session_rundown_in_lock(session);
        UNLOCK_SESSION(session);
    }

    free(endpoint);
}

LWMsgStatus
lwmsg_direct_connect(
    const char* name,
    DirectSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectEndpoint* endpoint = NULL;

    LOCK_ENDPOINTS();

    endpoint = lwmsg_direct_find(name);

    if (!endpoint)
    {
        /* FIXME: better status code */
        UNLOCK_ENDPOINTS();
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    lwmsg_ring_insert_before(&endpoint->sessions, &session->ring);

    UNLOCK_ENDPOINTS();

    session->endpoint = endpoint;

    if (endpoint->server->session_construct)
    {
        BAIL_ON_ERROR(status = endpoint->server->session_construct(
            session->token,
            endpoint->server->session_construct_data,
            &session->data));
    }

error:

    return status;
}

void
lwmsg_direct_disconnect(
    DirectSession* session
    )
{
    if (session->endpoint)
    {
        LOCK_ENDPOINTS();
        lwmsg_ring_remove(&session->ring);
        UNLOCK_ENDPOINTS();
        LOCK_SESSION(session);
        lwmsg_direct_session_rundown_in_lock(session);
        UNLOCK_SESSION(session);
    }
}

static
LWMsgSecurityToken*
direct_get_security_token (
    LWMsgSession* session
    )
{
    DirectSession* dsession = (DirectSession*) session;

    return dsession->token;
}

static
LWMsgStatus
direct_register_handle_local(
    LWMsgSession* session,
    const char* type,
    void* data,
    void (*cleanup)(void* ptr),
    LWMsgHandle** handle
    )
{
    DirectSession* dsession = (DirectSession*) session;

    return dsession->session->base.sclass->register_handle_local(
        (LWMsgSession*) dsession->session,
        type,
        data,
        cleanup,
        handle);
}

static
void
direct_retain_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    DirectSession* dsession = (DirectSession*) session;

    return dsession->session->base.sclass->retain_handle(
        (LWMsgSession*) dsession->session,
        handle);
}

static
void
direct_release_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    DirectSession* dsession = (DirectSession*) session;

    return dsession->session->base.sclass->release_handle(
        (LWMsgSession*) dsession->session,
        handle);
}

static
LWMsgStatus
direct_unregister_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    DirectSession* dsession = (DirectSession*) session;

    return dsession->session->base.sclass->unregister_handle(
        (LWMsgSession*) dsession->session,
        handle);
}

static
LWMsgStatus
direct_resolve_handle_to_id(
    LWMsgSession* session,
    LWMsgHandle* handle,
    const char** type,
    LWMsgHandleType* htype,
    LWMsgHandleID* hid
    )
{
    DirectSession* dsession = (DirectSession*) session;

    return dsession->session->base.sclass->resolve_handle_to_id(
        (LWMsgSession*) dsession->session,
        handle,
        type,
        htype,
        hid);
}

static
LWMsgStatus
direct_get_handle_data(
    LWMsgSession* session,
    LWMsgHandle* handle,
    void** data
    )
{
    DirectSession* dsession = (DirectSession*) session;

    return dsession->session->base.sclass->get_handle_data(
        (LWMsgSession*) dsession->session,
        handle,
        data);
}

static
void*
direct_get_data (
    LWMsgSession* session
    )
{
    DirectSession* dsession = (DirectSession*) session;

    return dsession->data;
}

static
LWMsgStatus
direct_acquire_call(
    LWMsgSession* session,
    LWMsgCall** call
    )
{
    return lwmsg_direct_call_new((DirectSession*) session, LWMSG_TRUE, (DirectCall**)(void*) call);
}

static LWMsgSessionClass direct_session_class =
{
    .register_handle_local = direct_register_handle_local,
    .retain_handle = direct_retain_handle,
    .release_handle = direct_release_handle,
    .unregister_handle = direct_unregister_handle,
    .resolve_handle_to_id = direct_resolve_handle_to_id,
    .get_handle_data = direct_get_handle_data,
    .get_data = direct_get_data,
    .get_peer_security_token = direct_get_security_token,
    .acquire_call = direct_acquire_call
};

static
LWMsgStatus
lwmsg_direct_call_invoke_dispatch(
    DirectCall* call,
    LWMsgPeerCallFunction func
    )
{
    LWMsgPeer* source_peer = !call->is_callback ? call->session->session->peer : call->session->endpoint->server;
    LWMsgPeer* target_peer = call->is_callback ? call->session->session->peer : call->session->endpoint->server;

    if (source_peer->trace_begin)
    {
        source_peer->trace_begin(&call->caller, call->in, LWMSG_STATUS_SUCCESS, source_peer->trace_data);
    }

    if (target_peer->trace_begin)
    {
        target_peer->trace_begin(&call->callee, call->in, LWMSG_STATUS_SUCCESS, target_peer->trace_data);
    }

    return func(&call->callee, call->in, call->out, target_peer->dispatch_data);
}

static
void
lwmsg_direct_call_invoke_complete(
    DirectCall* call,
    LWMsgStatus status,
    LWMsgBool do_complete
    )
{
    LWMsgPeer* source_peer = !call->is_callback ? call->session->session->peer : call->session->endpoint->server;
    LWMsgPeer* target_peer = call->is_callback ? call->session->session->peer : call->session->endpoint->server;

    if (source_peer->trace_end)
    {
        source_peer->trace_end(&call->caller, call->out, status, source_peer->trace_data);
    }

    if (target_peer->trace_end)
    {
        target_peer->trace_end(&call->callee, call->out, status, target_peer->trace_data);
    }

    if (do_complete && call->complete)
    {
        call->complete(&call->caller, status, call->complete_data);
    }
}

static
void
lwmsg_direct_call_release(
    LWMsgCall* call
    )
{
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, caller);
    DirectSession* session = dcall->session;
    uint32_t refs = 0;

    LOCK_SESSION(session);
    refs = --dcall->refs;
    UNLOCK_SESSION(session);

    if (refs == 0)
    {
        lwmsg_ring_remove(&dcall->ring);
        lwmsg_direct_session_release(dcall->session);
        free(dcall);
    }
}

static
void
lwmsg_direct_call_remove_in_lock(
    DirectCall* call
    )
{
    lwmsg_ring_remove(&call->ring);
    if (!call->session->endpoint &&
        lwmsg_ring_is_empty(&call->session->calls))
    {
        pthread_cond_signal(&call->session->session->event);
    }
}

static
void
lwmsg_direct_call_worker(
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectCall* dcall = data;
    DirectSession* session = dcall->session;
    DirectEndpoint* endpoint = dcall->session->endpoint;
    LWMsgPeerCallFunction func = NULL;
    LWMsgPeer* target_peer = NULL;
    LWMsgBool completed = LWMSG_FALSE;
    LWMsgBool canceled = LWMSG_FALSE;
    LWMsgBool waiting = LWMSG_FALSE;

    target_peer = dcall->is_callback ? session->session->peer : endpoint->server;
    func = target_peer->dispatch.vector[dcall->in->tag]->data;

    status = lwmsg_direct_call_invoke_dispatch(dcall, func);

    LOCK_SESSION(session);
    completed = dcall->state & DIRECT_CALL_COMPLETED;
    canceled = dcall->state & DIRECT_CALL_CANCELED;
    waiting = dcall->state & DIRECT_CALL_WAITING;
    dcall->state |= DIRECT_CALL_DISPATCHED;
    if (completed || status != LWMSG_STATUS_PENDING)
    {
        if (status == LWMSG_STATUS_PENDING)
        {
            status = dcall->status;
        }
        lwmsg_direct_call_remove_in_lock(dcall);
    }
    else if (canceled && dcall->cancel)
    {
        dcall->cancel(&dcall->caller, dcall->cancel_data);
    }
    UNLOCK_SESSION(session);

    switch (status)
    {
    case LWMSG_STATUS_PENDING:
        break;
    default:
        if (dcall->complete)
        {
            lwmsg_direct_call_invoke_complete(dcall, status, LWMSG_TRUE);
            lwmsg_direct_call_release(&dcall->caller);
        }

        if (waiting)
        {
            pthread_cond_broadcast(&dcall->session->session->event);
        }
    }
}

static
LWMsgStatus
lwmsg_direct_call_dispatch(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    LWMsgCompleteFunction complete,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, caller);
    DirectSession* session = dcall->session;
    DirectEndpoint* endpoint = NULL;
    LWMsgPeerCallFunction func = NULL;
    LWMsgBool canceled = LWMSG_FALSE;
    LWMsgBool completed = LWMSG_FALSE;
    LWMsgPeer* target_peer = NULL;
    LWMsgDispatchSpec* spec = NULL;
    LWMsgBool unref = LWMSG_FALSE;

    LOCK_SESSION(session);
    endpoint = session->endpoint;
    if (endpoint)
    {
        lwmsg_ring_enqueue(&session->calls, &dcall->ring);
    }
    dcall->state = 0;
    dcall->complete = complete;
    dcall->complete_data = data;
    dcall->refs++;
    dcall->in = in;
    dcall->out = out;
    UNLOCK_SESSION(session);

    unref = LWMSG_TRUE;

    if (!endpoint)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_CANCELLED);
    }

    target_peer = dcall->is_callback ? session->session->peer : endpoint->server;

    if (in->tag >= target_peer->dispatch.vector_length)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
    }

    spec = target_peer->dispatch.vector[in->tag];
    func = spec->data;

    if (!func)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
    }

    if (complete && spec->type == LWMSG_DISPATCH_TYPE_BLOCK)
    {
        /*
         * The caller doesn't want to block but the callee does,
         * so dispatch a work item and return LWMSG_STATUS_PENDING
         */

        BAIL_ON_ERROR(status = lwmsg_task_dispatch_work_item(
            target_peer->task_manager,
            lwmsg_direct_call_worker,
            dcall));
        unref = LWMSG_FALSE;
        BAIL_ON_ERROR(status = LWMSG_STATUS_PENDING);
    }

    unref = LWMSG_FALSE;
    status = lwmsg_direct_call_invoke_dispatch(dcall, func);

    switch (status)
    {
    case LWMSG_STATUS_PENDING:

        LOCK_SESSION(session);
        completed = dcall->state & DIRECT_CALL_COMPLETED;
        canceled = dcall->state & DIRECT_CALL_CANCELED;
        dcall->state |= DIRECT_CALL_DISPATCHED;

        /* If sync call, we have to wait for completion */
        if (!dcall->complete)
        {
            dcall->state |= DIRECT_CALL_WAITING;

            while (!completed)
            {
                pthread_cond_wait(&session->session->event, &session->session->lock);
                completed = dcall->state & DIRECT_CALL_COMPLETED;
            }
        }

        if (completed)
        {
            lwmsg_direct_call_remove_in_lock(dcall);
            unref = LWMSG_TRUE;
            status = dcall->status;
        }
        else if (canceled && dcall->cancel)
        {
            dcall->cancel(call, dcall->cancel_data);
        }
        UNLOCK_SESSION(session);

        BAIL_ON_ERROR(status);
        break;
    default:
        LOCK_SESSION(session);
        lwmsg_direct_call_remove_in_lock(dcall);
        UNLOCK_SESSION(session);
        unref = LWMSG_TRUE;
        BAIL_ON_ERROR(status);
    }

    if (status != LWMSG_STATUS_PENDING)
    {
        lwmsg_direct_call_invoke_complete(dcall, status, LWMSG_FALSE);
    }

error:

    if (unref)
    {
        lwmsg_direct_call_release(&dcall->caller);
    }

    return status;
}

static
LWMsgStatus
lwmsg_direct_call_pend(
    LWMsgCall* call,
    LWMsgCancelFunction cancel,
    void* data
    )
{
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, callee);

    dcall->cancel = cancel;
    dcall->cancel_data = data;

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
lwmsg_direct_call_complete(
    LWMsgCall* call,
    LWMsgStatus status
    )
{
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, callee);
    LWMsgBool dispatched = LWMSG_FALSE;
    LWMsgBool waiting = LWMSG_FALSE;

    LOCK_SESSION(dcall->session);
    dcall->state |= DIRECT_CALL_COMPLETED;
    dcall->status = status;
    dispatched = dcall->state & DIRECT_CALL_DISPATCHED;
    waiting = dcall->state & DIRECT_CALL_WAITING;
    lwmsg_direct_call_remove_in_lock(dcall);
    UNLOCK_SESSION(dcall->session);

    if (dispatched && dcall->complete)
    {
        dcall->complete(&dcall->caller, status, dcall->complete_data);
        lwmsg_direct_call_release(&dcall->caller);
    }

    if (waiting)
    {
        pthread_cond_broadcast(&dcall->session->session->event);
    }

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
lwmsg_direct_call_cancel(
    LWMsgCall* call
    )
{
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, caller);
    LWMsgBool cancel = LWMSG_FALSE;

    LOCK_SESSION(dcall->session);
    if (!(dcall->state & DIRECT_CALL_CANCELED))
    {
        dcall->state |= DIRECT_CALL_CANCELED;
        cancel = dcall->state & DIRECT_CALL_DISPATCHED;
    }

    if (cancel)
    {
        dcall->cancel(call, dcall->cancel_data);
    }
    UNLOCK_SESSION(dcall->session);

    return LWMSG_STATUS_SUCCESS;

}

static
LWMsgStatus
lwmsg_direct_call_wait(
    LWMsgCall* call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, caller);

    LOCK_SESSION(dcall->session);
    dcall->state |= DIRECT_CALL_WAITING;

    while (!(dcall->state & DIRECT_CALL_COMPLETED))
    {
        pthread_cond_wait(&dcall->session->session->event, &dcall->session->session->lock);
    }
    status = dcall->status;

    UNLOCK_SESSION(dcall->session);

    return status;
}


static
LWMsgStatus
lwmsg_direct_call_destroy_params(
    LWMsgCall* call,
    LWMsgParams* params
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, caller);
    LWMsgTypeSpec* spec = NULL;

    if (params->data && params->tag != LWMSG_TAG_INVALID)
    {
        BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(
            dcall->session->session->peer->protocol,
            params->tag,
            &spec));

        lwmsg_data_free_graph_cleanup(
            dcall->session->session->peer->context,
            spec,
            params->data);

        params->tag = LWMSG_TAG_INVALID;
        params->data = NULL;
    }

error:

    return status;
}

static
LWMsgSession*
lwmsg_direct_call_get_session_caller(
    LWMsgCall* call
    )
{
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, caller);

    return (LWMsgSession*) dcall->session->session;
}

static
LWMsgSession*
lwmsg_direct_call_get_session_callee(
    LWMsgCall* call
    )
{
    DirectCall* dcall = LWMSG_OBJECT_FROM_MEMBER(call, DirectCall, callee);

    return (LWMsgSession*) dcall->session;
}

static LWMsgCallClass direct_caller_class =
{
    .release = lwmsg_direct_call_release,
    .dispatch = lwmsg_direct_call_dispatch,
    .cancel = lwmsg_direct_call_cancel,
    .wait = lwmsg_direct_call_wait,
    .destroy_params = lwmsg_direct_call_destroy_params,
    .get_session = lwmsg_direct_call_get_session_caller
};

static LWMsgCallClass direct_callee_class =
{
    .pend = lwmsg_direct_call_pend,
    .complete = lwmsg_direct_call_complete,
    .get_session = lwmsg_direct_call_get_session_callee
};

LWMsgStatus
lwmsg_direct_call_new(
    DirectSession* session,
    LWMsgBool is_callback,
    DirectCall** call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectCall* dcall = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&dcall));

    dcall->caller.vtbl = &direct_caller_class;
    dcall->callee.vtbl = &direct_callee_class;
    lwmsg_ring_init(&dcall->ring);
    dcall->session = session;
    dcall->refs = 1;
    dcall->is_callback = is_callback;
    dcall->caller.is_outgoing = LWMSG_TRUE;
    dcall->callee.is_outgoing = LWMSG_FALSE;

error:

    *call = dcall;

    return status;
}

static
void
lwmsg_direct_session_delete(
    DirectSession* my_session
    )
{
    lwmsg_peer_session_release(my_session->session);

    if (my_session->token)
    {
        lwmsg_security_token_delete(my_session->token);
    }

    free(my_session);
}

void
lwmsg_direct_session_release(
    DirectSession* session
    )
{
    uint32_t refs = 0;

    LOCK_SESSION(session);
    refs = --session->refs;
    UNLOCK_SESSION(session);

    if (refs == 0)
    {
        lwmsg_direct_session_delete(session);
    }
}

LWMsgStatus
lwmsg_direct_session_new(
    PeerSession* session,
    DirectSession** direct_session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectSession* my_session = NULL;
    LWMsgBool attr_destroy = LWMSG_FALSE;
    pthread_mutexattr_t attr;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_session));

    my_session->base.sclass = &direct_session_class;

    BAIL_ON_ERROR(status = lwmsg_local_token_new(geteuid(), getegid(), getpid(), &my_session->token));
    my_session->session = session;
    my_session->refs = 1;

    lwmsg_ring_init(&my_session->calls);
    lwmsg_ring_init(&my_session->ring);

done:

    *direct_session = my_session;

    if (attr_destroy)
    {
        pthread_mutexattr_destroy(&attr);
    }

    return status;

error:

    if (my_session)
    {
        lwmsg_direct_session_delete(my_session);
        my_session = NULL;
    }

    goto done;
}

