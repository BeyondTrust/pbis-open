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
 *        test-client-server.c
 *
 * Abstract:
 *
 *        Multi-threaded client/server unit tests
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include <lwmsg/lwmsg.h>
#include <moonunit/moonunit.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "util-private.h"
#include "test-private.h"

/* Dispatch functions can be marked as blocking or
   nonblocking, with the only distinction being that
   functions marked as blocking may block indefinately
   when called.  Both blocking and nonblocking functions
   may complete synchronously (returning LWMSG_STATUS_SUCCESS)
   or asynchronously (returning LWMSG_STATUS_PENDING), so
   all 4 combinations must be tested.  We also test that
   asynchronously completed calls can be cancelled by
   explicit client request, by disconnecting the client,
   or by shutting down the server. */

#define DELAY_IN_MS 50

typedef enum AsyncTag
{
    BLOCK_REQUEST_SYNCHRONOUS,
    BLOCK_REQUEST_ASYNCHRONOUS,
    NONBLOCK_REQUEST_SYNCHRONOUS,
    NONBLOCK_REQUEST_ASYNCHRONOUS,
    GENERIC_RESPONSE
} AsyncTag;

LWMsgProtocolSpec async_protocol_spec[] =
{
    LWMSG_MESSAGE(BLOCK_REQUEST_SYNCHRONOUS, NULL),
    LWMSG_MESSAGE(BLOCK_REQUEST_ASYNCHRONOUS, NULL),
    LWMSG_MESSAGE(NONBLOCK_REQUEST_SYNCHRONOUS, NULL),
    LWMSG_MESSAGE(NONBLOCK_REQUEST_ASYNCHRONOUS, NULL),
    LWMSG_MESSAGE(GENERIC_RESPONSE, NULL),
    LWMSG_PROTOCOL_END
};

typedef struct
{
    LWMsgCall* call;
    LWMsgParams* response;
    LWMsgBool interrupt;
    pthread_mutex_t lock;
    pthread_cond_t event;
} AsyncRequest;

static pthread_mutex_t async_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t async_event = PTHREAD_COND_INITIALIZER;
static LWMsgBool async_cancelled = LWMSG_FALSE;
static LWMsgBool async_cancel_received = LWMSG_FALSE;

static
void
async_wait_cancelled(
    void
    )
{
    pthread_mutex_lock(&async_lock);
    while (!async_cancelled)
    {
        pthread_cond_wait(&async_event, &async_lock);
    }
    pthread_mutex_unlock(&async_lock);
}

static
void
async_wait_cancel_received(
    void
    )
{
    pthread_mutex_lock(&async_lock);
    while (!async_cancel_received)
    {
        pthread_cond_wait(&async_event, &async_lock);
    }
    pthread_mutex_unlock(&async_lock);
}


static
void*
async_response_thread(
    void* data
    )
{
    AsyncRequest* request = data;
    LWMsgTime now = {-1, -1};
    LWMsgTime wait = {0, DELAY_IN_MS * 1000};
    LWMsgTime abs = {-1, -1};
    struct timespec ts = {0, 0};

    MU_TRY(lwmsg_time_now(&now));
    lwmsg_time_sum(&now, &wait, &abs);

    ts.tv_sec = abs.seconds;
    ts.tv_nsec = abs.microseconds * 1000;

    pthread_mutex_lock(&request->lock);

    while (!request->interrupt)
    {
        if (pthread_cond_timedwait(&request->event, &request->lock, &ts) == ETIMEDOUT)
        {
            request->response->tag = GENERIC_RESPONSE;
            lwmsg_call_complete(request->call, LWMSG_STATUS_SUCCESS);
            goto done;
        }
    }

    MU_VERBOSE("Request interrupted");

    lwmsg_call_complete(request->call, LWMSG_STATUS_CANCELLED);

    pthread_mutex_lock(&async_lock);
    async_cancelled = LWMSG_TRUE;
    pthread_cond_signal(&async_event);
    pthread_mutex_unlock(&async_lock);

done:

    pthread_mutex_unlock(&request->lock);
    pthread_mutex_destroy(&request->lock);
    pthread_cond_destroy(&request->event);
    free(request);

    return NULL;
}

static
void
async_interrupt(
    LWMsgCall* call,
    void* data
    )
{
    AsyncRequest* request = data;

    pthread_mutex_lock(&request->lock);
    request->interrupt = LWMSG_TRUE;
    pthread_cond_signal(&request->event);
    pthread_mutex_unlock(&request->lock);
}

static
LWMsgStatus
async_request(
    LWMsgCall* call,
    const LWMsgParams* request,
    LWMsgParams* response
    )
{
    AsyncRequest* req = NULL;
    pthread_t thread = (pthread_t) -1;

    switch (request->tag)
    {
    case BLOCK_REQUEST_ASYNCHRONOUS:
    case NONBLOCK_REQUEST_ASYNCHRONOUS:
        MU_TRY(LWMSG_ALLOC(&req));
        
        pthread_mutex_init(&req->lock, NULL);
        pthread_cond_init(&req->event, NULL);
        
        req->call = call;
        req->response = response;
        
        lwmsg_call_pend(call, async_interrupt, req);
        
        pthread_create(&thread, NULL, async_response_thread, req);
        pthread_detach(thread);
        return LWMSG_STATUS_PENDING;
    case BLOCK_REQUEST_SYNCHRONOUS:
    case NONBLOCK_REQUEST_SYNCHRONOUS:
        response->tag = GENERIC_RESPONSE;
        return LWMSG_STATUS_SUCCESS;
    default:
        return LWMSG_STATUS_INTERNAL;
    }
}

static
void
async_dummy_complete(
    LWMsgCall* call,
    LWMsgStatus status,
    void* data
    )
{
    return;
}

static
void
async_notify_cancelled(
    LWMsgCall* call,
    LWMsgStatus status,
    void* data
    )
{
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_CANCELLED);
    pthread_mutex_lock(&async_lock);
    async_cancel_received = LWMSG_TRUE;
    pthread_cond_broadcast(&async_event);
    pthread_mutex_unlock(&async_lock);

    return;
}


LWMsgDispatchSpec async_dispatch_spec[] =
{
    LWMSG_DISPATCH_BLOCK(BLOCK_REQUEST_SYNCHRONOUS, async_request),
    LWMSG_DISPATCH_BLOCK(BLOCK_REQUEST_ASYNCHRONOUS, async_request),
    LWMSG_DISPATCH_NONBLOCK(NONBLOCK_REQUEST_SYNCHRONOUS, async_request),
    LWMSG_DISPATCH_NONBLOCK(NONBLOCK_REQUEST_ASYNCHRONOUS, async_request),
    LWMSG_DISPATCH_END
};

static LWMsgContext* context;
static LWMsgProtocol* protocol;
static LWMsgPeer* client;
static LWMsgPeer* server;

MU_FIXTURE_SETUP(async_indirect)
{
    MU_TRY(lwmsg_context_new(NULL, &context));
    lwmsg_context_set_log_function(context, lwmsg_test_log_function, NULL);

    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, async_protocol_spec));

    MU_TRY(lwmsg_peer_new(context, protocol, &server));
    MU_TRY(lwmsg_peer_add_dispatch_spec(server, async_dispatch_spec));
    MU_TRY(lwmsg_peer_add_listen_endpoint(server, LWMSG_ENDPOINT_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_peer_start_listen(server));

    MU_TRY(lwmsg_peer_new(NULL, protocol, &client));
    MU_TRY(lwmsg_peer_add_connect_endpoint(client, LWMSG_ENDPOINT_LOCAL, TEST_ENDPOINT));
    MU_TRY(lwmsg_peer_connect(client, NULL));
}

MU_FIXTURE_TEARDOWN(async_indirect)
{
    MU_TRY(lwmsg_peer_disconnect(client));
    lwmsg_peer_delete(client);

    MU_TRY(lwmsg_peer_stop_listen(server));
    lwmsg_peer_delete(server);

    lwmsg_protocol_delete(protocol);
    lwmsg_context_delete(context);
}

MU_FIXTURE_SETUP(async_direct)
{
    MU_TRY(lwmsg_context_new(NULL, &context));
    lwmsg_context_set_log_function(context, lwmsg_test_log_function, NULL);

    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, async_protocol_spec));
    
    MU_TRY(lwmsg_peer_new(context, protocol, &server));
    MU_TRY(lwmsg_peer_add_dispatch_spec(server, async_dispatch_spec));
    MU_TRY(lwmsg_peer_add_listen_endpoint(server, LWMSG_ENDPOINT_DIRECT, "test", 0));
    MU_TRY(lwmsg_peer_start_listen(server));
    
    MU_TRY(lwmsg_peer_new(NULL, protocol, &client));
    MU_TRY(lwmsg_peer_add_connect_endpoint(client, LWMSG_ENDPOINT_DIRECT, "test"));
    MU_TRY(lwmsg_peer_connect(client, NULL));
}

MU_FIXTURE_TEARDOWN(async_direct)
{
    MU_TRY(lwmsg_peer_disconnect(client));
    lwmsg_peer_delete(client);

    MU_TRY(lwmsg_peer_stop_listen(server));
    lwmsg_peer_delete(server);

    lwmsg_protocol_delete(protocol);
    lwmsg_context_delete(context);
}

static
void
setup(
    LWMsgBool nonblock,
    LWMsgBool async,
    LWMsgParams* in
    )
{
    switch (nonblock)
    {
    case LWMSG_TRUE:
        switch (async)
        {
        case LWMSG_TRUE:
            in->tag = NONBLOCK_REQUEST_ASYNCHRONOUS;
            break;
        case LWMSG_FALSE:
            in->tag = NONBLOCK_REQUEST_SYNCHRONOUS;
            break;
        }
        break;
    case LWMSG_FALSE:
        switch (async)
        {
        case LWMSG_TRUE:
            in->tag = BLOCK_REQUEST_ASYNCHRONOUS;
            break;
        case LWMSG_FALSE:
            in->tag = BLOCK_REQUEST_SYNCHRONOUS;
            break;
        }
    }
}

static
void
test_basic(
    LWMsgBool nonblock,
    LWMsgBool async
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    setup(nonblock, async, &in);

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, GENERIC_RESPONSE);

    lwmsg_call_destroy_params(call, &out);
    lwmsg_call_release(call);
}

static
void
test_disconnect(
    LWMsgBool nonblock,
    LWMsgBool async
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;
    struct timespec ts = {0, (DELAY_IN_MS * 1000000) / 2};

    setup(nonblock, async, &in);

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_call_dispatch(call, &in, &out, async_dummy_complete, NULL),
        LWMSG_STATUS_PENDING);
    nanosleep(&ts, NULL);
    MU_TRY(lwmsg_peer_disconnect(client));
    lwmsg_call_wait(call);
    lwmsg_call_release(call);

    async_wait_cancelled();
}

static
void
test_shutdown(
    LWMsgBool nonblock,
    LWMsgBool async
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;
    struct timespec ts = {0, (DELAY_IN_MS * 1000000) / 2};

    setup(nonblock, async, &in);

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_call_dispatch(call, &in, &out, async_dummy_complete, NULL),
        LWMSG_STATUS_PENDING);
    nanosleep(&ts, NULL);
    MU_TRY(lwmsg_peer_stop_listen(server));

    async_wait_cancelled();

    lwmsg_call_wait(call);
    lwmsg_call_release(call);
}

static
void
test_cancel(
    LWMsgBool nonblock,
    LWMsgBool async
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;
    struct timespec ts = {0, (DELAY_IN_MS * 1000000) / 2};

    setup(nonblock, async, &in);

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_call_dispatch(call, &in, &out, async_notify_cancelled, NULL),
        LWMSG_STATUS_PENDING);
    nanosleep(&ts, NULL);
    lwmsg_call_cancel(call);

    async_wait_cancelled();
    async_wait_cancel_received();

    lwmsg_call_wait(call);
    lwmsg_call_release(call);
}

MU_TEST(async_indirect, nonblock_synchrounous)
{
    test_basic(LWMSG_TRUE, LWMSG_FALSE);
}

MU_TEST(async_indirect, nonblock_asynchronous)
{
    test_basic(LWMSG_TRUE, LWMSG_TRUE);
}

MU_TEST(async_indirect, block_synchrounous)
{
    test_basic(LWMSG_FALSE, LWMSG_FALSE);
}

MU_TEST(async_indirect, block_asynchronous)
{
    test_basic(LWMSG_FALSE, LWMSG_TRUE);
}

MU_TEST(async_direct, nonblock_synchrounous)
{
    test_basic(LWMSG_TRUE, LWMSG_FALSE);
}

MU_TEST(async_direct, nonblock_asynchronous)
{
    test_basic(LWMSG_TRUE, LWMSG_TRUE);
}

MU_TEST(async_direct, block_synchrounous)
{
    test_basic(LWMSG_FALSE, LWMSG_FALSE);
}

MU_TEST(async_direct, block_asynchronous)
{
    test_basic(LWMSG_FALSE, LWMSG_TRUE);
}

MU_TEST(async_indirect, nonblock_asynchronous_disconnect)
{
    test_disconnect(LWMSG_TRUE, LWMSG_TRUE);
}

MU_TEST(async_indirect, block_asynchronous_disconnect)
{
    test_disconnect(LWMSG_FALSE, LWMSG_TRUE);
}

MU_TEST(async_direct, nonblock_asynchronous_disconnect)
{
    test_disconnect(LWMSG_TRUE, LWMSG_TRUE);
}

MU_TEST(async_direct, block_asynchronous_disconnect)
{
    test_disconnect(LWMSG_FALSE, LWMSG_TRUE);
}

MU_TEST(async_indirect, nonblock_asynchronous_shutdown)
{
    test_shutdown(LWMSG_TRUE, LWMSG_TRUE);
}

MU_TEST(async_indirect, block_asynchronous_shutdown)
{
    test_shutdown(LWMSG_FALSE, LWMSG_TRUE);
}

MU_TEST(async_direct, nonblock_asynchronous_shutdown)
{
    test_shutdown(LWMSG_TRUE, LWMSG_TRUE);
}

MU_TEST(async_direct, block_asynchronous_shutdown)
{
    test_shutdown(LWMSG_FALSE, LWMSG_TRUE);
}

MU_TEST(async_indirect, nonblock_asynchronous_cancel)
{
    test_cancel(LWMSG_TRUE, LWMSG_TRUE);
}

MU_TEST(async_indirect, block_asynchronous_cancel)
{
    test_cancel(LWMSG_FALSE, LWMSG_TRUE);
}

MU_TEST(async_direct, nonblock_asynchronous_cancel)
{
    test_cancel(LWMSG_TRUE, LWMSG_TRUE);
}

MU_TEST(async_direct, block_asynchronous_cancel)
{
    test_cancel(LWMSG_FALSE, LWMSG_TRUE);
}
