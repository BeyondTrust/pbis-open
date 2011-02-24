#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

#include "fserver.h"
#include "protocol.h"

struct
{
    int trace;
} global;

static void
blocked_signal_set(sigset_t* set)
{
    sigemptyset(set);
    sigaddset(set, SIGTERM);
    sigaddset(set, SIGINT);
    sigaddset(set, SIGPIPE);
}

static void
wait_signal_set(sigset_t* set)
{
    sigemptyset(set);
    sigaddset(set, SIGTERM);
    sigaddset(set, SIGINT);
}

static void
block_signals(void)
{
    sigset_t blocked;

    signal(SIGPIPE, SIG_IGN);

    blocked_signal_set(&blocked);

    pthread_sigmask(SIG_BLOCK, &blocked, NULL);
}

static int
wait_signal(void)
{
    sigset_t wait;
    int sig = 0;

    wait_signal_set(&wait);

    sigwait(&wait, &sig);

    return sig;
}

static int
run(LWMsgPeer* server)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int ret = 0;
    int done = 0;
    int sig = 0;

    block_signals();

    /* Begin listening for calls */
    status = lwmsg_peer_start_listen(server);
    if (status)
    {
        goto error;
    }

    while (!done)
    {
        sig = wait_signal();

        switch (sig)
        {
        case SIGINT:
        case SIGTERM:
            done = 1;
            break;
        default:
            break;
        }
    }

    /* Stop listening */
    status = lwmsg_peer_stop_listen(server);
    if (status)
    {
        goto error;
    }
    
error:

    if (status != LWMSG_STATUS_SUCCESS)
    {
        ret = -1;
    }

    return ret;
}

static
LWMsgBool
log_message(
    LWMsgLogLevel level,
    const char* message,
    const char* function,
    const char* filename,
    unsigned int line,
    void* data
    )
{
    if (message)
    {
        fprintf(stderr, "[lwmsg] %s\n", message);
    }

    return LWMSG_TRUE;
}


int main(int argc, char** argv)
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgContext* context = NULL;
    LWMsgProtocol* protocol = NULL;
    LWMsgPeer* server = NULL;

    /* Create context */
    status = lwmsg_context_new(NULL, &context);
    if (status)
    {
        goto error;
    }

    /* Set log function */
    lwmsg_context_set_log_function(context, log_message, NULL);

    /* Create protocol */
    status = lwmsg_protocol_new(context, &protocol);
    if (status)
    {
        goto error;
    }

    /* Add protocol spec */
    status = lwmsg_protocol_add_protocol_spec(protocol, fserv_get_protocol());
    if (status)
    {
        goto error;
    }

    /* Create peer */
    status = lwmsg_peer_new(context, protocol, &server);
    if (status)
    {
        goto error;
    }

    /* Add dispatch spec */
    status = lwmsg_peer_add_dispatch_spec(server, fserver_get_dispatch());
    if (status)
    {
        goto error;
    }

    /* Add listen endpoint */
    status = lwmsg_peer_add_listen_endpoint(server, LWMSG_ENDPOINT_LOCAL, FSERV_SOCKET_PATH, (S_IRWXU | S_IRWXG | S_IRWXO));
    if (status)
    {
        goto error;
    }

    ret = run(server);

error:

    if (server)
    {
        lwmsg_peer_delete(server);
    }

    if (protocol)
    {
        lwmsg_protocol_delete(protocol);
    }

    if (status != LWMSG_STATUS_SUCCESS)
    {
        ret = -1;
    }

    exit(ret ? 1 : 0);
}

