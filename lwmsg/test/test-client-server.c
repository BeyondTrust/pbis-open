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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "util-private.h"
#include "test-private.h"
#include "data-private.h"
#include "protocol-private.h"

typedef struct CounterHandle
{
    pthread_mutex_t lock;
    int counter;
} CounterHandle;

typedef struct CounterRequest
{
    int counter;
} CounterRequest;

typedef struct CounterReply
{
    int counter;
} CounterReply;

typedef struct CounterAdd
{
    LWMsgHandle* handle;
    int delta;
} CounterAdd;

LWMsgTypeSpec counterhandle_spec[] =
{
    LWMSG_HANDLE(CounterHandle),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_TYPE_END
};

LWMsgTypeSpec counterrequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(CounterRequest),
    LWMSG_MEMBER_INT16(CounterRequest, counter),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec counteradd_spec[] =
{
    LWMSG_STRUCT_BEGIN(CounterAdd),
    LWMSG_MEMBER_HANDLE(CounterAdd, handle, CounterHandle),
    LWMSG_MEMBER_INT16(CounterAdd, delta),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec counterreply_spec[] =
{
    LWMSG_STRUCT_BEGIN(CounterReply),
    LWMSG_MEMBER_INT16(CounterReply, counter),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


typedef enum CounterType
{
    COUNTER_OPEN,
    COUNTER_OPEN_SUCCESS,
    COUNTER_CLOSE,
    COUNTER_CLOSE_SUCCESS,
    COUNTER_ADD,
    COUNTER_ADD_SUCCESS,
    COUNTER_READ,
    COUNTER_READ_SUCCESS
} CounterType;

LWMsgProtocolSpec counterprotocol_spec[] =
{
    LWMSG_MESSAGE(COUNTER_OPEN, counterrequest_spec),
    LWMSG_MESSAGE(COUNTER_OPEN_SUCCESS, counterhandle_spec),
    LWMSG_MESSAGE(COUNTER_ADD, counteradd_spec),
    LWMSG_MESSAGE(COUNTER_ADD_SUCCESS, counterreply_spec),
    LWMSG_MESSAGE(COUNTER_READ, counterhandle_spec),
    LWMSG_MESSAGE(COUNTER_READ_SUCCESS, counterreply_spec),
    LWMSG_MESSAGE(COUNTER_CLOSE, counterhandle_spec),
    LWMSG_MESSAGE(COUNTER_CLOSE_SUCCESS, counterreply_spec),
    LWMSG_PROTOCOL_END
};

static LWMsgStatus
counter_srv_open(LWMsgCall* call, const LWMsgParams* request_msg, LWMsgParams* reply_msg, void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    CounterHandle* handle_data = malloc(sizeof(*handle_data));
    CounterRequest* request = request_msg->data;
    LWMsgSession* session = lwmsg_call_get_session(call);
    LWMsgHandle* handle = NULL;

    pthread_mutex_init(&handle_data->lock, NULL);

    handle_data->counter = request->counter;

    MU_TRY(lwmsg_session_register_handle(session, "CounterHandle", handle_data, free, &handle));

    reply_msg->tag = COUNTER_OPEN_SUCCESS;
    reply_msg->data = handle;

    lwmsg_session_retain_handle(session, handle);

    return status;
}
static LWMsgStatus
counter_srv_add(LWMsgCall* call, const LWMsgParams* request_msg, LWMsgParams* reply_msg, void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSession* session = lwmsg_call_get_session(call);
    CounterAdd* add = request_msg->data;
    CounterHandle* handle = NULL;
    CounterReply* reply = malloc(sizeof(*reply));

    MU_TRY(lwmsg_session_get_handle_data(session, add->handle, (void**)(void*)&handle));

    pthread_mutex_lock(&handle->lock);
    reply->counter = handle->counter;
    handle->counter += add->delta;
    pthread_mutex_unlock(&handle->lock);

    reply_msg->tag = COUNTER_ADD_SUCCESS;
    reply_msg->data = reply;

    return status;
}

static LWMsgStatus
counter_srv_read(LWMsgCall* call, const LWMsgParams* request_msg, LWMsgParams* reply_msg, void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSession* session = lwmsg_call_get_session(call);
    CounterHandle* handle = NULL;
    CounterReply* reply = malloc(sizeof(*reply));

    MU_TRY(lwmsg_session_get_handle_data(session, (LWMsgHandle*) request_msg->data, (void**)(void*)&handle));

    pthread_mutex_lock(&handle->lock);
    reply->counter = handle->counter;
    pthread_mutex_unlock(&handle->lock);

    reply_msg->tag = COUNTER_READ_SUCCESS;
    reply_msg->data = reply;

    return status;
}

static LWMsgStatus
counter_srv_close(LWMsgCall* call, const LWMsgParams* request_msg, LWMsgParams* reply_msg, void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    CounterHandle* handle = NULL;
    CounterReply* reply = malloc(sizeof(*reply));
    LWMsgSession* session = lwmsg_call_get_session(call);

    MU_TRY(lwmsg_session_get_handle_data(session, (LWMsgHandle*) request_msg->data, (void**)(void*)&handle));

    pthread_mutex_lock(&handle->lock);
    reply->counter = handle->counter;
    pthread_mutex_unlock(&handle->lock);
    
    pthread_mutex_destroy(&handle->lock);
    lwmsg_session_unregister_handle(session, request_msg->data);

    reply_msg->tag = COUNTER_CLOSE_SUCCESS;
    reply_msg->data = reply;

    return status;
}

LWMsgDispatchSpec counter_dispatch[] =
{
    LWMSG_DISPATCH_BLOCK(COUNTER_OPEN, counter_srv_open),
    LWMSG_DISPATCH_BLOCK(COUNTER_ADD, counter_srv_add),
    LWMSG_DISPATCH_BLOCK(COUNTER_READ, counter_srv_read),
    LWMSG_DISPATCH_BLOCK(COUNTER_CLOSE, counter_srv_close),
    LWMSG_DISPATCH_END
};

typedef struct
{
    LWMsgPeer* client;
    LWMsgHandle* handle;
    int iters;
    pthread_mutex_t lock;
    pthread_cond_t event;
    int volatile go;
} Data;

static void*
add_thread(void* _data)
{
    Data *data = _data;
    int i;
    LWMsgCall* call = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    CounterAdd add;
    CounterReply* reply = NULL;

    pthread_mutex_lock(&data->lock);
    while (!data->go)
    {
        pthread_cond_wait(&data->event, &data->lock);
    }
    pthread_mutex_unlock(&data->lock);

    add.handle = data->handle;
    add.delta = 1;

    for (i = 0; i < data->iters; i++)
    {
        MU_TRY(lwmsg_peer_acquire_call(data->client, &call));

        in.tag = COUNTER_ADD;
        in.data = &add;
        
        MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));

        MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, COUNTER_ADD_SUCCESS);
        reply = (CounterReply*) out.data;
        
        MU_VERBOSE("(0x%lx) counter: %i -> %i",
                   (unsigned long) (pthread_self()), 
                   reply->counter,
                   reply->counter+1);
        
        lwmsg_call_destroy_params(call, &out);
        lwmsg_call_release(call);
    }

    return NULL;
}

#define MAX_CLIENTS 16
#define MAX_DISPATCH 4
#define NUM_THREADS 16
#define NUM_ITERS 10

MU_TEST(stress, parallel)
{
    Data data;
    pthread_t threads[NUM_THREADS];
    int i;
    LWMsgContext* context = NULL;
    LWMsgProtocol* protocol = NULL;
    LWMsgPeer* client = NULL;
    LWMsgPeer* server = NULL;
    CounterRequest request;
    CounterReply* reply;
    LWMsgCall* call;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgTime timeout = {1, 0};

    MU_TRY(lwmsg_context_new(NULL, &context));
    lwmsg_context_set_log_function(context, lwmsg_test_log_function, NULL);

    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, counterprotocol_spec));

    MU_TRY(lwmsg_peer_new(context, protocol, &server));
    MU_TRY(lwmsg_peer_add_dispatch_spec(server, counter_dispatch));
    MU_TRY(lwmsg_peer_add_listen_endpoint(server, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_peer_set_max_listen_clients(server, MAX_CLIENTS));
    MU_TRY(lwmsg_peer_set_timeout(server, LWMSG_TIMEOUT_IDLE, &timeout));
    MU_TRY(lwmsg_peer_start_listen(server));

    MU_TRY(lwmsg_peer_new(context, protocol, &client));
    MU_TRY(lwmsg_peer_add_connect_endpoint(client, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT));
    MU_TRY(lwmsg_peer_connect(client, NULL));

    request.counter = 0;

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    in.tag = COUNTER_OPEN;
    in.data = &request;

    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, COUNTER_OPEN_SUCCESS);
    lwmsg_call_release(call);

    data.client = client;
    data.handle = out.data;
    data.iters = NUM_ITERS;
    data.go = 0;
    
    pthread_mutex_init(&data.lock, NULL);
    pthread_cond_init(&data.event, NULL);

    pthread_mutex_lock(&data.lock);
    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, add_thread, &data);
    }
    data.go = 1;
    pthread_cond_broadcast(&data.event);
    pthread_mutex_unlock(&data.lock);

    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    in.tag = COUNTER_READ;
    in.data = data.handle;

    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, COUNTER_READ_SUCCESS);
    reply = out.data;

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply->counter, NUM_THREADS * NUM_ITERS);

    lwmsg_call_destroy_params(call, &out);
    lwmsg_call_release(call);
    
    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    in.tag = COUNTER_CLOSE;
    in.data = data.handle;

    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, COUNTER_CLOSE_SUCCESS);

    lwmsg_call_destroy_params(call, &out);
    lwmsg_call_release(call);

    MU_TRY(lwmsg_peer_disconnect(client));
    lwmsg_peer_delete(client);

    MU_TRY(lwmsg_peer_stop_listen(server));
    lwmsg_peer_delete(server);

    pthread_mutex_destroy(&data.lock);
    pthread_cond_destroy(&data.event);
}

MU_TEST(stress, parallel_print_protocol)
{
    LWMsgContext* context = NULL;
    LWMsgDataContext* dcontext = NULL;
    LWMsgProtocol* protocol = NULL;
    char* text = NULL;

    MU_TRY(lwmsg_context_new(NULL, &context));
    MU_TRY(lwmsg_data_context_new(context, &dcontext));

    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, counterprotocol_spec));

    MU_TRY(lwmsg_protocol_print_alloc(protocol, 4, &text));

    MU_VERBOSE("\n%s", text);
}

MU_TEST(client_server, handle_invalidation)
{
    LWMsgContext* context = NULL;
    LWMsgProtocol* protocol = NULL;
    LWMsgPeer* client = NULL;
    LWMsgPeer* server = NULL;
    LWMsgHandle* handle = NULL;
    CounterRequest request;
    LWMsgCall* call;
    LWMsgSession* session = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgHandleType locality = 0;
    struct timespec ts = {0, 50000000};

    MU_TRY(lwmsg_context_new(NULL, &context));
    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, counterprotocol_spec));

    MU_TRY(lwmsg_peer_new(context, protocol, &server));
    MU_TRY(lwmsg_peer_add_dispatch_spec(server, counter_dispatch));
    MU_TRY(lwmsg_peer_add_listen_endpoint(server, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_peer_start_listen(server));

    MU_TRY(lwmsg_peer_new(context, protocol, &client));
    MU_TRY(lwmsg_peer_add_connect_endpoint(client, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT));
    MU_TRY(lwmsg_peer_connect(client, NULL));

    request.counter = 0;

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    session = lwmsg_call_get_session(call);

    in.tag = COUNTER_OPEN;
    in.data = &request;

    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, COUNTER_OPEN_SUCCESS);
    lwmsg_call_release(call);

    handle = out.data;

    MU_TRY(lwmsg_peer_stop_listen(server));
    nanosleep(&ts, NULL);
    MU_TRY(lwmsg_peer_start_listen(server));

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    nanosleep(&ts, NULL);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, lwmsg_session_get_handle_location(session, handle, &locality), LWMSG_STATUS_INVALID_HANDLE);
    
    lwmsg_session_release_handle(session, handle);

    MU_TRY(lwmsg_peer_disconnect(client));
    lwmsg_peer_delete(client);

    MU_TRY(lwmsg_peer_stop_listen(server));
    lwmsg_peer_delete(server);
}


typedef int (*IntFunction) (int value);

typedef struct MulticallRequest
{
    LWMsgHandle* func;
    int num_values;
    int* values;
} MulticallRequest;

typedef struct MulticallReply
{
    int num_values;
    int* values;
} MulticallReply;

typedef struct InvokeCallbackRequest
{
    LWMsgHandle* func;
    int value;
} InvokeRequest;

typedef enum Command
{
    MULTICALL_REQUEST,
    MULTICALL_REPLY,
    INVOKE_REQUEST,
    INVOKE_REPLY,
    BOGUS_REQUEST,
    PING_REQUEST,
    PING_REPLY
} Command;

LWMsgTypeSpec intfunction_spec[] =
{
    LWMSG_HANDLE(IntFunction),
    LWMSG_TYPE_END
};

LWMsgTypeSpec multicallrequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(MulticallRequest),
    LWMSG_MEMBER_TYPESPEC(MulticallRequest, func, intfunction_spec),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,
    LWMSG_MEMBER_UINT32(MulticallRequest, num_values),
    LWMSG_MEMBER_POINTER(MulticallRequest, values, LWMSG_INT32(int)),
    LWMSG_ATTR_LENGTH_MEMBER(MulticallRequest, num_values),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec multicallreply_spec[] =
{
    LWMSG_STRUCT_BEGIN(MulticallReply),
    LWMSG_MEMBER_UINT32(MulticallReply, num_values),
    LWMSG_MEMBER_POINTER(MulticallReply, values, LWMSG_INT32(int)),
    LWMSG_ATTR_LENGTH_MEMBER(MulticallReply, num_values),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec invokerequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(InvokeRequest),
    LWMSG_MEMBER_TYPESPEC(InvokeRequest, func, intfunction_spec),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,
    LWMSG_MEMBER_INT32(InvokeRequest, value),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec invokereply_spec[] =
{
    LWMSG_INT32(int),
    LWMSG_TYPE_END
};

LWMsgProtocolSpec multicall_spec[] =
{
    LWMSG_MESSAGE(MULTICALL_REQUEST, multicallrequest_spec),
    LWMSG_MESSAGE(MULTICALL_REPLY, multicallreply_spec),
    LWMSG_MESSAGE(INVOKE_REQUEST, invokerequest_spec),
    LWMSG_MESSAGE(INVOKE_REPLY, invokereply_spec),
    LWMSG_MESSAGE(BOGUS_REQUEST, NULL),
    LWMSG_MESSAGE(PING_REQUEST, NULL),
    LWMSG_MESSAGE(PING_REPLY, NULL),
    LWMSG_PROTOCOL_END
};

static
LWMsgStatus
Multicall(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out
    )
{
    LWMsgCall* callback = NULL;
    MulticallRequest* req = in->data;
    MulticallReply* rep = NULL;
    unsigned int i;
    InvokeRequest invoke;
    LWMsgParams invoke_in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams invoke_out = LWMSG_PARAMS_INITIALIZER;
    LWMsgSession* session = lwmsg_call_get_session(call);
    
    MU_TRY(lwmsg_session_acquire_call(session, &callback));

    rep = malloc(sizeof(*rep));
    rep->values = malloc(sizeof(int) * req->num_values);
    rep->num_values = req->num_values;

    for (i = 0; i < req->num_values; i++)
    {
        invoke.func = req->func;
        invoke.value = req->values[i];
        invoke_in.tag = INVOKE_REQUEST;
        invoke_in.data = &invoke;
        
        MU_TRY(lwmsg_call_dispatch(callback, &invoke_in, &invoke_out, NULL, NULL));
        MU_ASSERT_EQUAL(MU_TYPE_INTEGER, invoke_out.tag, INVOKE_REPLY);

        rep->values[i] = *(int*) invoke_out.data;

        lwmsg_call_destroy_params(callback, &invoke_out);
    }

    out->tag = MULTICALL_REPLY;
    out->data = rep;

    return LWMSG_STATUS_SUCCESS;
}
    
static
LWMsgStatus
Invoke(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out
    )
{
    InvokeRequest* req = in->data;
    int* rep = NULL;
    LWMsgSession* session = lwmsg_call_get_session(call);
    IntFunction func = NULL;

    MU_TRY(lwmsg_session_get_handle_data(session, req->func, (void**)(void*)&func));

    rep = malloc(sizeof(*rep));
    *rep = func(req->value);

    out->tag = INVOKE_REPLY;
    out->data = rep;

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
Bogus(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out
    )
{
    out->tag = -42;
    out->data = NULL;

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
Ping(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out
    )
{
    out->tag = PING_REPLY;
    out->data = NULL;

    return LWMSG_STATUS_SUCCESS;
}


LWMsgDispatchSpec multicall_dispatch[] =
{
    LWMSG_DISPATCH_BLOCK(MULTICALL_REQUEST, Multicall),
    LWMSG_DISPATCH_NONBLOCK(BOGUS_REQUEST, Bogus),
    LWMSG_DISPATCH_NONBLOCK(PING_REQUEST, Ping),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec invoke_dispatch[] =
{
    LWMSG_DISPATCH_NONBLOCK(INVOKE_REQUEST, Invoke),
    LWMSG_DISPATCH_END
};

static
int
times_two(int value)
{
    return 2 * value;
}

/* The following test suite checks for proper error reporting,
   error recovery, and state tracking behavior */
LWMsgContext* context = NULL;
LWMsgProtocol* protocol = NULL;
LWMsgPeer* server = NULL;
LWMsgPeer* client = NULL;

MU_FIXTURE_SETUP(client_server_indirect)
{
    LWMsgTime timeout = {0, 50 * 1000};

    MU_TRY(lwmsg_context_new(NULL, &context));
    lwmsg_context_set_log_function(context, lwmsg_test_log_function, NULL);

    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, multicall_spec));

    MU_TRY(lwmsg_peer_new(context, protocol, &server));
    MU_TRY(lwmsg_peer_add_dispatch_spec(server, multicall_dispatch));
    MU_TRY(lwmsg_peer_add_listen_endpoint(server, LWMSG_ENDPOINT_LOCAL, TEST_ENDPOINT, 0600));

    MU_TRY(lwmsg_peer_new(NULL, protocol, &client));
    MU_TRY(lwmsg_peer_add_dispatch_spec(client, invoke_dispatch));
    MU_TRY(lwmsg_peer_add_connect_endpoint(client, LWMSG_ENDPOINT_LOCAL, TEST_ENDPOINT));
    MU_TRY(lwmsg_peer_set_timeout(client, LWMSG_TIMEOUT_ESTABLISH, &timeout));
}

MU_FIXTURE_TEARDOWN(client_server_indirect)
{
    lwmsg_peer_delete(client);
    lwmsg_peer_delete(server);
    lwmsg_protocol_delete(protocol);
    lwmsg_context_delete(context);
}

MU_FIXTURE_SETUP(client_server_direct)
{
    LWMsgTime timeout = {0, 50 * 1000};

    MU_TRY(lwmsg_context_new(NULL, &context));
    lwmsg_context_set_log_function(context, lwmsg_test_log_function, NULL);

    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, multicall_spec));

    MU_TRY(lwmsg_peer_new(context, protocol, &server));
    MU_TRY(lwmsg_peer_add_dispatch_spec(server, multicall_dispatch));
    MU_TRY(lwmsg_peer_add_listen_endpoint(server, LWMSG_ENDPOINT_DIRECT, "test", 0));

    MU_TRY(lwmsg_peer_new(NULL, protocol, &client));
    MU_TRY(lwmsg_peer_add_dispatch_spec(client, invoke_dispatch));
    MU_TRY(lwmsg_peer_add_connect_endpoint(client, LWMSG_ENDPOINT_DIRECT, "test"));
    MU_TRY(lwmsg_peer_set_timeout(client, LWMSG_TIMEOUT_ESTABLISH, &timeout));
}

MU_FIXTURE_TEARDOWN(client_server_direct)
{
    lwmsg_peer_delete(client);
    lwmsg_peer_delete(server);
    lwmsg_protocol_delete(protocol);
    lwmsg_context_delete(context);
}

/* Basic call test */
static
void
test_ping(
    void
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    in.tag = PING_REQUEST;

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_connect(client, NULL));
    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);

    lwmsg_call_release(call);

    MU_TRY(lwmsg_peer_disconnect(client));
    MU_TRY(lwmsg_peer_stop_listen(server));
}

/*
 * Test that making a call after restarting
 * the server succeeds without restarting
 * the client.
 */
static
void
test_call_restart_call(
    void
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;
    struct timespec ts = {0, 50000000};

    in.tag = PING_REQUEST;

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_connect(client, NULL));
    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);

    lwmsg_call_release(call);

    MU_TRY(lwmsg_peer_stop_listen(server));
    MU_TRY(lwmsg_peer_start_listen(server));

    nanosleep(&ts, NULL);

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);

    lwmsg_call_release(call);
}

/*
 * Test that making a call after reconnecting
 * the client succeeds without restarting
 * the server.
 */
static
void
test_call_reconnect_call(
    void
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    in.tag = PING_REQUEST;

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_connect(client, NULL));
    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);

    lwmsg_call_release(call);

    MU_TRY(lwmsg_peer_disconnect(client));
    MU_TRY(lwmsg_peer_connect(client, NULL));

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);

    lwmsg_call_release(call);
}

/*
 * Test that making a call after failing
 * to make an earlier call because the server
 * was not yet started succeeds.
 */
static
void
test_call_fail_start_call(
    void
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    in.tag = PING_REQUEST;

    MU_TRY(lwmsg_peer_connect(client, NULL));
    if (lwmsg_peer_acquire_call(client, &call) == LWMSG_STATUS_SUCCESS)
    {
        MU_ASSERT_NOT_EQUAL(
            MU_TYPE_INTEGER,
            lwmsg_call_dispatch(call, &in, &out, NULL, NULL),
            LWMSG_STATUS_SUCCESS);
    }

    lwmsg_call_release(call);

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);

    lwmsg_call_release(call);
}

/*
 * Test that making a call after the server
 * shuts down fails.
 */
static
void
test_call_shutdown_call_fail(
    void
    )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    in.tag = PING_REQUEST;

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_connect(client, NULL));
    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));

    lwmsg_call_release(call);

    MU_TRY(lwmsg_peer_stop_listen(server));

    if (lwmsg_peer_acquire_call(client, &call) == LWMSG_STATUS_SUCCESS)
    {
        MU_ASSERT_NOT_EQUAL(
            MU_TYPE_INTEGER,
            lwmsg_call_dispatch(call, &in, &out, NULL, NULL),
            LWMSG_STATUS_SUCCESS);
    }

    lwmsg_call_release(call);
}

/*
 * Test callback support.
 */
static
void
test_callback(
    void
    )
{
    LWMsgSession* session = NULL;
    LWMsgCall* call = NULL;
    MulticallRequest req;
    MulticallReply* rep;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    int values[] = {1, 2, 3, 4, 5};
    unsigned int i;
    LWMsgHandle* handle = NULL;

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_connect(client, &session));

    MU_TRY(lwmsg_session_register_handle(session, "IntFunction", times_two, NULL, &handle));

    req.func = handle;
    req.num_values = sizeof(values) / sizeof(*values);
    req.values = values;
    in.tag = MULTICALL_REQUEST;
    in.data = &req;

    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    rep = (MulticallReply*) out.data;

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, MULTICALL_REPLY);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, rep->num_values, req.num_values);

    for (i = 0; i < rep->num_values; i++)
    {
        MU_ASSERT_EQUAL(MU_TYPE_INTEGER, rep->values[i], times_two(req.values[i]));
    }

    MU_TRY(lwmsg_session_unregister_handle(session, handle));
    MU_TRY(lwmsg_call_destroy_params(call, &out));
    lwmsg_call_release(call);
    MU_TRY(lwmsg_peer_disconnect(client));
    MU_TRY(lwmsg_peer_stop_listen(server));
}


typedef struct trace_info
{
    LWMsgBool outgoing_begin, outgoing_end;
    LWMsgBool incoming_begin, incoming_end;
} trace_info;

static
void
trace_begin(
    LWMsgCall* call,
    const LWMsgParams* params,
    LWMsgStatus status,
    void* data
    )
{
    trace_info* info = data;

    if (lwmsg_call_get_direction(call) == LWMSG_CALL_INCOMING)
    {
        MU_VERBOSE("Begin in call %i\n", params->tag);
        info->incoming_begin = LWMSG_TRUE;
    }
    else
    {
        MU_VERBOSE("Begin out call %i\n", params->tag);
        info->outgoing_begin = LWMSG_TRUE;
    }
}

static
void
trace_end(
    LWMsgCall* call,
    const LWMsgParams* params,
    LWMsgStatus status,
    void* data
    )
{
    trace_info* info = data;

    if (lwmsg_call_get_direction(call) == LWMSG_CALL_INCOMING)
    {
        MU_VERBOSE("End in call %i\n", params->tag);
        info->incoming_end = LWMSG_TRUE;
    }
    else
    {
        MU_VERBOSE("End out call %i\n", params->tag);
        info->outgoing_end = LWMSG_TRUE;
    }
}

static
void
test_tracing(
   void
   )
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;
    trace_info info = {0};

    in.tag = PING_REQUEST;
    in.data = NULL;

    MU_TRY(lwmsg_peer_set_trace_functions(server, trace_begin, trace_end, &info));
    MU_TRY(lwmsg_peer_set_trace_functions(client, trace_begin, trace_end, &info));

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_connect(client, NULL));
    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);
    MU_ASSERT(info.incoming_begin);
    MU_ASSERT(info.incoming_end);
    MU_ASSERT(info.outgoing_begin);
    MU_ASSERT(info.outgoing_end);

    MU_TRY(lwmsg_peer_disconnect(client));
    MU_TRY(lwmsg_peer_stop_listen(server));
}

MU_TEST(client_server_indirect, ping)
{
    test_ping();
}

MU_TEST(client_server_direct, ping)
{
    test_ping();
}

MU_TEST(client_server_indirect, call_restart_call)
{
    test_call_restart_call();
}

MU_TEST(client_server_direct, call_restart_call)
{
    test_call_restart_call();
}

MU_TEST(client_server_indirect, call_reconnect_call)
{
    test_call_reconnect_call();
}

MU_TEST(client_server_direct, call_reconnect_call)
{
    test_call_reconnect_call();
}

MU_TEST(client_server_indirect, call_fail_start_call)
{
    test_call_fail_start_call();
}

MU_TEST(client_server_direct, call_fail_start_call)
{
    test_call_fail_start_call();
}

MU_TEST(client_server_indirect, call_shutdown_call_fail)
{
    test_call_shutdown_call_fail();
}

MU_TEST(client_server_direct, call_shutdown_call_fail)
{
    test_call_shutdown_call_fail();
}

MU_TEST(client_server_indirect, callback)
{
    test_callback();
}

MU_TEST(client_server_direct, callback)
{
    test_callback();
}

MU_TEST(client_server_indirect, tracing)
{
    test_tracing();
}

MU_TEST(client_server_direct, tracing)
{
    test_tracing();
}

/* Ensure that the server removes its domain socket file
   when it is stopped */
MU_TEST(client_server_indirect, stop_listen_removes_endpoint)
{
    struct stat statbuf;
    int ret = -1;

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_stop_listen(server));

    ret = stat(TEST_ENDPOINT, &statbuf);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, ret, -1);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, errno, ENOENT);
}

/* Ensure that attempting to listen on an endpoint that cannot
   be created (e.g. due to the directory not existing) fails */
MU_TEST(client_server_indirect, start_listen_on_bad_endpoint_fails)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_peer_add_listen_endpoint(
                      server,
                      LWMSG_CONNECTION_MODE_LOCAL,
                      "/bogus/endpoint/",
                      0600));
    BAIL_ON_ERROR(status = lwmsg_peer_start_listen(server));

error:

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_FILE_NOT_FOUND);
}

MU_TEST(client_server_indirect, bogus_reply_fail_ping_succeed)
{
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    in.tag = BOGUS_REQUEST;
    in.data = NULL;

    MU_TRY(lwmsg_peer_start_listen(server));
    MU_TRY(lwmsg_peer_connect(client, NULL));
    MU_TRY(lwmsg_peer_acquire_call(client, &call));
    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_call_dispatch(call, &in, &out, NULL, NULL),
        LWMSG_STATUS_MALFORMED);
    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_call_dispatch(call, &in, &out, NULL, NULL),
        LWMSG_STATUS_MALFORMED);

    in.tag = PING_REQUEST;

    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);

    MU_TRY(lwmsg_peer_disconnect(client));
    MU_TRY(lwmsg_peer_stop_listen(server));
}

#define NUM_ASSOCS 128

MU_TEST(client_server_indirect, client_limit_timeout)
{
    LWMsgAssoc* assocs[NUM_ASSOCS] = {0};
    LWMsgTime idle = {1, 0};
    int i = 0;

    /* This test shouldn't take more than 5 seconds */
    MU_TIMEOUT(5000);

    MU_TRY(lwmsg_peer_set_timeout(server, LWMSG_TIMEOUT_IDLE, &idle));
    MU_TRY(lwmsg_peer_set_max_listen_clients(server, NUM_ASSOCS / 2));
    MU_TRY(lwmsg_peer_start_listen(server));

    for (i = 0; i < NUM_ASSOCS; i++)
    {
        MU_TRY(lwmsg_connection_new(context, protocol, &assocs[i]));
        MU_TRY(lwmsg_connection_set_endpoint(assocs[i], LWMSG_ENDPOINT_LOCAL, TEST_ENDPOINT));
        MU_TRY(lwmsg_assoc_connect(assocs[i], NULL));
    }
}

MU_TEST(client_server_pair, basic)
{
    int sockets[2];
    LWMsgSession* session = NULL;
    LWMsgCall* call = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_context_new(NULL, &context));
    lwmsg_context_set_log_function(context, lwmsg_test_log_function, NULL);

    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, multicall_spec));

    MU_TRY(lwmsg_peer_new(context, protocol, &server));
    MU_TRY(lwmsg_peer_add_dispatch_spec(server, multicall_dispatch));

    MU_TRY(lwmsg_peer_new(NULL, protocol, &client));
    MU_TRY(lwmsg_peer_add_dispatch_spec(client, invoke_dispatch));

    MU_TRY(lwmsg_peer_accept_fd(server, LWMSG_ENDPOINT_PAIR, sockets[0]));
    MU_TRY(lwmsg_peer_connect_fd(server, LWMSG_ENDPOINT_PAIR, sockets[1], &session));

    in.tag = PING_REQUEST;

    MU_TRY(lwmsg_session_acquire_call(session, &call));
    MU_TRY(lwmsg_call_dispatch(call, &in, &out, NULL, NULL));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, PING_REPLY);

    lwmsg_call_release(call);

    MU_TRY(lwmsg_peer_disconnect(client));
    MU_TRY(lwmsg_peer_stop_listen(server));
}
