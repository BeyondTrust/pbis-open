#include "fserv-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

static LWMsgProtocol* protocol = NULL;
static LWMsgPeer* client = NULL;
static LWMsgSession* session = NULL;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static LWMsgStatus volatile once_status = LWMSG_STATUS_SUCCESS;

static
void
__fserv_construct(
    void
    )
{
    /* Create protocol structure */
    once_status = lwmsg_protocol_new(NULL, &protocol);
    if (once_status)
    {
        goto error;
    }
    
    /* Add protocol spec to protocol structure */
    once_status = lwmsg_protocol_add_protocol_spec(protocol, fserv_get_protocol());
    if (once_status)
    {
        goto error;
    }

    /* Create peer */
    once_status = lwmsg_peer_new(NULL, protocol, &client);
    if (once_status)
    {
        goto error;
    }
    
    /* Add connect endpoint */
    once_status = lwmsg_peer_add_connect_endpoint(
        client,
        LWMSG_ENDPOINT_LOCAL,
        FSERV_SOCKET_PATH);
    if (once_status)
    {
        goto error;
    }

    /* Connect */
    once_status = lwmsg_peer_connect(client, &session);
    if (once_status)
    {
        goto error;
    }

error:
    
    return;
}

static
LWMsgStatus
fserv_construct(
    void
    )
{
    pthread_once(&once, __fserv_construct);

    return once_status;
}

static
void
__attribute__((destructor))
fserv_destruct(
    void
    )
{
    if (client)
    {
        /* Disconnect and delete */
        lwmsg_peer_disconnect(client);
        lwmsg_peer_delete(client);
        session = NULL;
        client = NULL;
    }

    if (protocol)
    {
        lwmsg_protocol_delete(protocol);
        protocol = NULL;
    }
}

/* Open a file using an fserv connection */
int
fserv_open(
    const char* path,
    FServMode mode,
    FServFile** file
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    OpenRequest request;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    status = fserv_construct();
    if (status)
    {
        ret = -1;
        goto error;
    }

    /* Acquire call */
    status = lwmsg_peer_acquire_call(client, &call);
    if (status)
    {
        ret = -1;
        goto error;
    }

    /* Set up request parameters */
    request.mode = mode;
    request.path = (char*) path;   
    in.tag = FSERV_OPEN_REQ;
    in.data = &request;
    
    /* Make call */
    status = lwmsg_call_dispatch(call, &in, &out, NULL, NULL);
    if (status)
    {
        ret = -1;
        goto error;
    }

    switch (out.tag)
    {
    case FSERV_OPEN_RES:
        *file = out.data;
        out.data = NULL;
        break;
    case FSERV_ERROR_RES:
        /* Open failed -- extract the error code */
        ret = ((StatusReply*) out.data)->err;
        goto error;
    default:
        ret = EINVAL;
        goto error;
    }

done:

    if (call)
    {
        /* Clean up */
        lwmsg_call_destroy_params(call, &out);
        lwmsg_call_release(call);
    }

    return ret;

error:

    if (file)
    {
        free(file);
    }

    goto done;
}

/* Read from a file */
int
fserv_read(
    FServFile* file,
    unsigned long size,
    void* buffer,
    unsigned long* size_read
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ReadRequest request;
    ReadReply* reply;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    status = lwmsg_peer_acquire_call(client, &call);
    if (status)
    {
        ret = -1;
        goto error;
    }

    /* Set up request parameters */
    request.handle = (LWMsgHandle*) file;
    request.size = size;
    in.tag = FSERV_READ_REQ;
    in.data = &request;
    
    /* Send message and receive reply */
    status = lwmsg_call_dispatch(call, &in, &out, NULL, NULL);
    if (status)
    {
        ret = -1;
        goto error;
    }

    switch (out.tag)
    {
    case FSERV_READ_RES:
        /* Read succeeded -- copy the data into the buffer */
        reply = (ReadReply*) out.data;
        memcpy(buffer, reply->data, reply->size);
        *size_read = reply->size;
        break;
    case FSERV_ERROR_RES:
        /* Read failed -- extract the error code */
        ret = ((StatusReply*) out.data)->err;
        goto error;
    default:
        ret = EINVAL;
        goto error;
    }

done:

    if (call)
    {
        lwmsg_call_destroy_params(call, &out);
        lwmsg_call_release(call);
    }

    return ret;

error:
    
    goto done;
}

/* Write to a file */
int
fserv_write(
    FServFile* file,
    unsigned long size,
    void* buffer
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    WriteRequest request;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    status = lwmsg_peer_acquire_call(client, &call);
    if (status)
    {
        ret = -1;
        goto error;
    }

    /* Set up request parameters */
    request.handle = (LWMsgHandle*) file;
    request.size = size;
    request.data = (char*) buffer;
    in.tag = FSERV_WRITE_REQ;
    in.data = &request;
    
    /* Send message and receive reply */
    status = lwmsg_call_dispatch(call, &in, &out, NULL, NULL);
    if (status)
    {
        ret = -1;
        goto error;
    }

    switch (out.tag)
    {
    case FSERV_VOID_RES:
        /* Write succeeded */
        break;
    case FSERV_ERROR_RES:
        /* Write failed -- extract the error code */
        ret = ((StatusReply*) out.data)->err;
        goto error;
    default:
        ret = EINVAL;
        goto error;
    }

done:

    if (call)
    {
        lwmsg_call_destroy_params(call, &out);
        lwmsg_call_release(call);
    }

    return ret;

error:

    goto done;
}

/* Close a file */
int
fserv_close(
    FServFile* file
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* call = NULL;

    status = lwmsg_peer_acquire_call(client, &call);
    if (status)
    {
        ret = -1;
        goto error;
    }

    in.tag = FSERV_CLOSE_REQ;
    in.data = file;

    /* Send message and receive reply */
    status = lwmsg_call_dispatch(call, &in, &out, NULL, NULL);
    if (status)
    {
        ret = -1;
        goto error;
    }

    switch (out.tag)
    {
    case FSERV_VOID_RES:
        break;
    case FSERV_ERROR_RES:
        /* Extract the status code */
        ret = ((StatusReply*) out.data)->err;
        if (ret)
        {
            goto error;
        }
        break;
    default:
        ret = EINVAL;
        goto error;
    }
    
error:

    /* Release the handle even on failure */
    lwmsg_session_release_handle(session, (LWMsgHandle*) file);

    if (call)
    {
        lwmsg_call_destroy_params(call, &out);
        lwmsg_call_release(call);
    }
    
    return ret;
}
