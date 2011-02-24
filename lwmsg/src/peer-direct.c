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
            call->cancel(LWMSG_CALL(call), call->cancel_data);
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

    /* Now we can run down all handles */
    lwmsg_peer_session_reset(session->session);

    /* Release session reference */
    lwmsg_peer_session_release(session->session);
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
    LWMsgSessionCookie cookie = {{0}};
    LWMsgSecurityToken* token = NULL;

    BAIL_ON_ERROR(status = lwmsg_local_token_new(geteuid(), getegid(), getpid(), &token));

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

    BAIL_ON_ERROR(status = session->session->base.sclass->accept(
        &session->session->base,
        &cookie,
        token));
    token = NULL;

error:

    if (token)
    {
        lwmsg_security_token_delete(token);
    }

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
void
lwmsg_direct_call_release(
    LWMsgCall* call
    )
{
    DirectCall* dcall = (DirectCall*) call;
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

    target_peer = dcall->is_callback ? session->session->peer : endpoint->server;
    func = target_peer->dispatch.vector[dcall->in->tag]->data;

    status = func(LWMSG_CALL(dcall), dcall->in, dcall->out, target_peer->dispatch_data);

    LOCK_SESSION(session);
    completed = dcall->state & DIRECT_CALL_COMPLETED;
    canceled = dcall->state & DIRECT_CALL_CANCELED;
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
        dcall->cancel(LWMSG_CALL(dcall), dcall->cancel_data);
    }
    UNLOCK_SESSION(session);

    switch (status)
    {
    case LWMSG_STATUS_PENDING:
        break;
    default:
        if (dcall->complete)
        {
            dcall->complete(LWMSG_CALL(dcall), status, dcall->complete_data);
            lwmsg_direct_call_release(LWMSG_CALL(dcall));
        }
        else
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
    DirectCall* dcall = (DirectCall*) call;
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

        dcall->in = in;
        dcall->out = out;

        BAIL_ON_ERROR(status = lwmsg_task_dispatch_work_item(
            target_peer->task_manager,
            lwmsg_direct_call_worker,
            dcall));
        unref = LWMSG_FALSE;
        BAIL_ON_ERROR(status = LWMSG_STATUS_PENDING);
    }

    unref = LWMSG_FALSE;
    status = func(call, in, out, target_peer->dispatch_data);

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

error:

    if (unref)
    {
        lwmsg_direct_call_release(LWMSG_CALL(dcall));
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
    DirectCall* dcall = (DirectCall*) call;

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
    DirectCall* dcall = (DirectCall*) call;
    LWMsgBool dispatched = LWMSG_FALSE;

    LOCK_SESSION(dcall->session);
    dcall->state |= DIRECT_CALL_COMPLETED;
    dcall->status = status;
    dispatched = dcall->state & DIRECT_CALL_DISPATCHED;
    lwmsg_direct_call_remove_in_lock(dcall);
    UNLOCK_SESSION(dcall->session);

    if (dispatched && dcall->complete)
    {
        dcall->complete(call, status, dcall->complete_data);
        lwmsg_direct_call_release(LWMSG_CALL(dcall));
    }
    else
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
    DirectCall* dcall = (DirectCall*) call;
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
lwmsg_direct_call_destroy_params(
    LWMsgCall* call,
    LWMsgParams* params
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectCall* dcall = (DirectCall*) call;
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

LWMsgSession*
lwmsg_direct_call_get_session(
    LWMsgCall* call
    )
{
    DirectCall* dcall = (DirectCall*) call;

    return (LWMsgSession*) dcall->session->session;
}

LWMsgStatus
lwmsg_direct_call_acquire_callback(
    LWMsgCall* call,
    LWMsgCall** callback
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DirectCall* dcall = (DirectCall*) call;
    DirectCall* dcallback = NULL;

    BAIL_ON_ERROR(status = lwmsg_direct_call_new(
        dcall->session,
        !dcall->is_callback,
        &dcallback));

    *callback = LWMSG_CALL(dcallback);

error:

    return status;
}

static LWMsgCallClass direct_call_class =
{
    .release = lwmsg_direct_call_release,
    .dispatch = lwmsg_direct_call_dispatch,
    .pend = lwmsg_direct_call_pend,
    .complete = lwmsg_direct_call_complete,
    .cancel = lwmsg_direct_call_cancel,
    .destroy_params = lwmsg_direct_call_destroy_params,
    .get_session = lwmsg_direct_call_get_session,
    .acquire_callback = lwmsg_direct_call_acquire_callback
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

    dcall->base.vtbl = &direct_call_class;
    lwmsg_ring_init(&dcall->ring);
    dcall->session = session;
    dcall->refs = 1;
    dcall->is_callback = is_callback;

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

