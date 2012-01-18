/*
 * echo_server      : demo DCE RPC application
 *
 * Jim Doyle, jrd@bu.edu  09-05-1998
 *
 *
 */
#ifdef _MK_HOST
#include <config.h>
#endif

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <compat/dcerpc.h>
#include "echo.h"
#include "misc.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include "getopt_internal.h"
#endif

#ifndef _WIN32
static void wait_for_signals();
#endif

/*
 *
 * A template DCE RPC server
 *
 * main() contains the basic calls needed to register an interface,
 * get communications endpoints, and register the endpoints
 * with the endpoint mapper.
 *
 * ReverseIt() implements the interface specified in echo.idl
 *
 */

static void
bind_server(
    rpc_binding_vector_p_t * server_binding,
    rpc_if_handle_t interface_spec,
    char * protocol,
    char * endpoint
    )
{
    char * function = "n/a";
    unsigned32 status;

    /*
     * Prepare the server binding handle
     * use all avail protocols (UDP and TCP). This basically allocates
     * new sockets for us and associates the interface UUID and
     * object UUID of with those communications endpoints.
     */

#if 0
    rpc_server_use_all_protseqs_if(0, interface_spec, &status);
#else
    if (!endpoint)
    {
        if (!protocol)
        {
            function = "rpc_server_use_all_protseqs()";
            rpc_server_use_all_protseqs(rpc_c_protseq_max_calls_default, &status);
        }
        else
        {
            function = "rpc_server_use_protseq()";
            rpc_server_use_protseq((unsigned_char_p_t)protocol, rpc_c_protseq_max_calls_default, &status);
        }
    }
    else
    {
        function = "rpc_server_use_protseq_ep()";
        rpc_server_use_protseq_ep((unsigned_char_p_t)protocol, rpc_c_protseq_max_calls_default, (unsigned_char_p_t)endpoint, &status);
    }
#endif

    chk_dce_err(status, function, "", 1);
    rpc_server_inq_bindings(server_binding, &status);
    chk_dce_err(status, "rpc_server_inq_bindings()", "", 1);
}

static void usage()
{
    printf("usage: echo_server [-a name] [-e endpoint] [-n] [-u] [-t]\n");
    printf("         -a:  specify authentication identity\n");
    printf("         -e:  specify endpoint\n");
    printf("         -n:  use named pipe protocol\n");
    printf("         -u:  use UDP protocol\n");
    printf("         -t:  use TCP protocol (default)\n");
    printf("\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    unsigned32 status;
    rpc_binding_vector_p_t server_binding;
    unsigned_char_p_t string_binding;
    unsigned32 i;
    char * protocol = NULL;
    char * endpoint = NULL;
    int c;
    char * spn = NULL;

    /*
     * Process the cmd line args
     */

    while ((c = getopt(argc, argv, "a:e:nut")) != EOF)
    {
        switch (c)
        {
        case 'a':
            spn = optarg;
            break;
        case 'e':
            endpoint = optarg;
            break;
        case 'n':
            protocol = PROTOCOL_NP;
            break;
        case 'u':
            protocol = PROTOCOL_UDP;
            break;
        case 't':
            protocol = PROTOCOL_TCP;
            break;
        default:
            usage();
        }
    }

    if (endpoint && !protocol)
    {
        printf("ERROR: protocol is required when endpoint is specified\n");
        exit(1);
    }

#ifndef _WIN32
    /* Temporarily disable using all protocols because something is currently busted on Unix */
    if (!protocol)
    {
        protocol = PROTOCOL_TCP;
    }
#endif

    if (spn)
    {
        rpc_server_register_auth_info(
            (unsigned_char_p_t)spn,
            rpc_c_authn_gss_negotiate,
            NULL,
            NULL,
            &status);
        if (status)
        {
            printf ("Couldn't set auth info. exiting.\n");
            exit(1);
        }
    }

    /*
     * Register the Interface with the local endpoint mapper (rpcd)
     */

    printf ("Registering server.... \n");
    rpc_server_register_if(echo_v1_0_s_ifspec,
                           NULL,
                           NULL,
                           &status);
    chk_dce_err(status, "rpc_server_register_if()", "", 1);

    printf("registered.\nPreparing binding handle...\n");

    bind_server(&server_binding, echo_v1_0_s_ifspec, protocol, endpoint);

    /*
     * Register bindings with the endpoint mapper
     */

    printf("registering bindings with endpoint mapper\n");

    rpc_ep_register(echo_v1_0_s_ifspec,
                    server_binding,
                    NULL,
                    (unsigned char *)"QDA application server",
                    &status);
    chk_dce_err(status, "rpc_ep_register()", "", 1);

    printf("registered.\n");

    /*
     * Print out the servers endpoints (TCP and UDP port numbers)
     */

    printf ("Server's communications endpoints are:\n");

    for (i=0; i<RPC_FIELD_COUNT(server_binding); i++)
    {
        rpc_binding_to_string_binding(RPC_FIELD_BINDING_H(server_binding)[i],
                                      &string_binding,
                                      &status);
        if (string_binding)
            printf("\t%s\n", string_binding);
    }

#ifndef _WIN32
    /*
     * Start the signal waiting thread in background. This thread will
     * Catch SIGINT and gracefully shutdown the server.
     */

    wait_for_signals();
#endif

    /*
     * Begin listening for calls
     */

    printf ("listening for calls....\n");

    DCETHREAD_TRY
    {
        rpc_server_listen(rpc_c_listen_max_calls_default, &status);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        printf ("Server stoppped listening\n");
    }
    DCETHREAD_ENDTRY;

    /*
     * If we reached this point, then the server was stopped, most likely
     * by the signal handler thread called rpc_mgmt_stop_server().
     * gracefully cleanup and unregister the bindings from the
     * endpoint mapper.
     */

#ifndef _WIN32
    /*
     * Kill the signal handling thread
     */

#endif

    printf ("Unregistering server from the endpoint mapper....\n");
    rpc_ep_unregister(echo_v1_0_s_ifspec,
                      server_binding,
                      NULL,
                      &status);
    chk_dce_err(status, "rpc_ep_unregister()", "", 0);

    /*
     * retire the binding information
     */

    printf("Cleaning up communications endpoints...\n");
    rpc_server_unregister_if(echo_v1_0_s_ifspec,
                             NULL,
                             &status);
    chk_dce_err(status, "rpc_server_unregister_if()", "", 0);

    exit(0);
}


/*=========================================================================
 *
 * Server implementation of ReverseIt()
 *
 *=========================================================================*/

idl_boolean
ReverseIt(
    rpc_binding_handle_t h,
    args * in_text,
    args ** out_text,
    error_status_t * status
    )
{

    unsigned_char_p_t binding_info;
    error_status_t e;
    unsigned result_size;
    args * result;
    unsigned32 i,j,l;
#if 0
    rpc_transport_info_handle_t transport_info = NULL;
    unsigned32 rpcstatus = 0;
    unsigned char* sesskey = NULL;
    unsigned32 sesskey_len = 0;
    unsigned char* principal_name = NULL;
#endif

    /*
     * Get some info about the client binding
     */

    rpc_binding_to_string_binding(h, &binding_info, &e);
    if (e == rpc_s_ok)
    {
        printf ("ReverseIt() called by client: %s\n", binding_info);
	rpc_string_free(&binding_info, &e);
    }

#if 0
    rpc_binding_inq_transport_info(h, &transport_info, &rpcstatus);

    if (transport_info)
    {
        rpc_smb_transport_info_inq_session_key(transport_info, &sesskey, &sesskey_len);

        printf ("Session key: ");

        for (i = 0; i < sesskey_len; i++)
        {
            printf("%X", sesskey[i]);
        }

        printf ("\n");
    }
#endif

    if (in_text == NULL) return 0;

    /*
     *  Print the in_text
     */

    printf("\n\nFunction ReverseIt() -- input argments\n");

    for (i=0; i<in_text->argc; i++)
        printf("\t[arg %lu]: %s\n", (unsigned long)i, in_text->argv[i]);

    printf ("\n=========================================\n");

    /*
     * Allocate the output args as dynamic storage bound
     * to this RPC. The output args are the same size as the
     * input args since we are simply reversing strings.
     */

    result_size = sizeof(args) + in_text->argc * sizeof(string_t *);
    result = (args * )rpc_ss_allocate(result_size);
    result->argc = in_text->argc;

    for (i=0; i < in_text->argc; i++)
    {
        result->argv[i] =
            (string_t)rpc_ss_allocate(strlen((PCSTR)in_text->argv[i]) + 1);
    }

    /*
     * do the string reversal
     */

    for (i=0; i < in_text->argc; i++)
    {
        l = strlen((PCSTR)in_text->argv[i]);
        for (j=0; j<l; j++)
        {
            result->argv[i][j] = in_text->argv[i][l-j-1];
        }
        result->argv[i][l]=0;           /* make sure its null terminated! */
    }

    *out_text = result;
    *status = error_status_ok;

    return 1;
}


#ifndef _WIN32
/*=========================================================================
 *
 * wait_for_signals()
 *
 *
 * Set up the process environment to properly deal with signals.
 * By default, we isolate all threads from receiving asynchronous
 * signals. We create a thread that handles all async signals.
 * The signal handling actions are handled in the handler thread.
 *
 * For AIX, we cant use a thread that sigwaits() on a specific signal,
 * we use a plain old, lame old Unix signal handler.
 *
 *=========================================================================*/

void
wait_for_signals()
{
    sigset_t signals;

    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);

    dcethread_signal_to_interrupt(&signals, dcethread_self());
}

#endif
