/* ex: set shiftwidth=4 softtabstop=4 expandtab: */
/*
 * access_client  : demo DCE RPC application
 *
 * Jim Doyle, jrd@bu.edu, 09-05-1998
 *
 *
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#define getopt getopt_system

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <compat/dcerpc.h>
#include "access.h"
#include <misc.h>

#undef getopt

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef _WIN32
#define EOF_STRING "^Z"
#else
#define EOF_STRING "^D"
#endif

/*
 * Forward declarations
 */

static int
get_client_rpc_binding(
    rpc_binding_handle_t * binding_handle,
    rpc_if_handle_t interface_spec,
    char * hostname,
    char * protocol,
    char * endpoint,
    const char* mech
    );

/*
 * usage()
 */

static void usage()
{
    printf("usage: access_client [-h hostname] [-e endpoint] [-n] [-u] [-t]\n");
    printf("         -h:  specify host of RPC server (default is localhost)\n");
    printf("         -e:  specify endpoint for protocol\n");
    printf("         -n:  use named pipe protocol\n");
    printf("         -u:  use UDP protocol\n");
    printf("         -t:  use TCP protocol (default)\n");
    printf("         -g:  instead of prompting, generate a data string of the specified length\n");
    printf("         -d:  turn on debugging\n");
    printf("\n");
    exit(1);
}

int
main(
    int argc,
    char *argv[]
    )
{

    /*
     * command line processing and options stuff
     */

    extern char *optarg;
    extern int optind, opterr, optopt;
    int c;

    char * rpc_host = "localhost";
    char * protocol = PROTOCOL_TCP;
    char * endpoint = NULL;
    char * mech = NULL;

    /*
     * stuff needed to make RPC calls
     */

    unsigned32 status;
    rpc_binding_handle_t access_server;
    int ok;
    unsigned32 i;
    int generate_length = -1;
    char * nl;
    string_t me = NULL;

    /*
     * Process the cmd line args
     */

    while ((c = getopt(argc, argv, "lh:e:nutdg:m:")) != EOF)
    {
        switch (c)
        {
        case 'h':
            rpc_host = optarg;
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
        case 'l':
            protocol = PROTOCOL_LRPC;
            break;
        case 'd':
#ifdef _WIN32
	    printf("This option is only supported on Linux.\n");
#else
            rpc__dbg_set_switches("0-19.10", &status);
            //Skip 20, which is memory allocs and frees
            rpc__dbg_set_switches("21-43.10", &status);
#endif
            break;
        case 'g':
            generate_length = strtol(optarg, NULL, 10);
            break;
        case 'm':
            mech = optarg;
            break;
        default:
            usage();
        }
    }

    /*
     * Get a binding handle to the server using the following params:
     *
     *  1. the hostname where the server lives
     *  2. the interface description structure of the IDL interface
     *  3. the desired transport protocol (UDP or TCP)
     */

    if (get_client_rpc_binding(&access_server,
                               access_v1_0_c_ifspec,
                               rpc_host,
                               protocol,
                               endpoint,
                               mech) == 0)
    {
        printf ("Couldnt obtain RPC server binding. exiting.\n");
        exit(1);
    }


    WhoAmI(access_server, &me);

    /*
     * Print the results
     */

    printf ("%s\n", me);

    /*
     * Done. Now gracefully teardown the RPC binding to the server
     */

    rpc_binding_free(&access_server, &status);
    exit(0);
}

/*==========================================================================
 *
 * get_client_rpc_binding()
 *
 *==========================================================================
 *
 * Gets a binding handle to an RPC interface.
 *
 * parameters:
 *
 *    [out]     binding_handle
 *    [in]      interface_spec <- DCE Interface handle for service
 *    [in]      hostname       <- Internet hostname where server lives
 *    [in]      protocol       <- "ncacn_ip_tcp", etc.
 *    [in]      endpoint       <- optional
 *
 *==========================================================================*/

static int
get_client_rpc_binding(
    rpc_binding_handle_t * binding_handle,
    rpc_if_handle_t interface_spec,
    char * hostname,
    char * protocol,
    char * endpoint,
    const char* mech
    )
{
    char * string_binding = NULL;
    error_status_t status;
    unsigned32 authn_protocol = rpc_c_authn_gss_negotiate;
    unsigned32 authn_level = rpc_c_authn_level_connect;
    char server_principal[512];

    /*
     * create a string binding given the command line parameters and
     * resolve it into a full binding handle using the endpoint mapper.
     *  The binding handle resolution is handled by the runtime library
     */

    rpc_string_binding_compose(NULL,
			       protocol,
			       hostname,
			       endpoint,
			       NULL,
			       &string_binding,
			       &status);
    chk_dce_err(status, "rpc_string_binding_compose()", "get_client_rpc_binding", 1);


    rpc_binding_from_string_binding((unsigned char *)string_binding,
                                    binding_handle,
                                    &status);
    chk_dce_err(status, "rpc_binding_from_string_binding()", "get_client_rpc_binding", 1);

    if (!endpoint)
    {
        /*
         * Resolve the partial binding handle using the endpoint mapper
         */

        rpc_ep_resolve_binding(*binding_handle,
                               interface_spec,
                               &status);
        chk_dce_err(status, "rpc_ep_resolve_binding()", "get_client_rpc_binding", 1);
    }

    if (mech)
    {
        if (!strcmp(mech, "spnego"))
        {
            authn_protocol = rpc_c_authn_gss_negotiate;
        }

        sprintf(server_principal, "host/%s", hostname);

        rpc_binding_set_auth_info(*binding_handle,
                                  server_principal,
                                  authn_level,
                                  authn_protocol,
                                  NULL,
                                  rpc_c_authz_name,
                                  &status);
        chk_dce_err(status, "rpc_binding_set_auth_info()", "get_client_rpc_binding", 1);
    }

    rpc_string_free(&string_binding, &status);
    chk_dce_err(status, "rpc_string_free()", "get_client_rpc_binding", 1);

    /*
     * Get a printable rendition of the binding handle and echo to
     * the user.
     */

    rpc_binding_to_string_binding(*binding_handle,
                                  (unsigned char **)&string_binding,
                                  &status);
    chk_dce_err(status, "rpc_binding_to_string_binding()", "get_client_rpc_binding", 1);

    printf("fully resolving binding for server is: %s\n", string_binding);

    rpc_string_free(&string_binding, &status);
    chk_dce_err(status, "rpc_string_free()", "get_client_rpc_binding", 1);

    return 1;
}
