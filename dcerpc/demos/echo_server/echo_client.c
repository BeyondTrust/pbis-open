/* ex: set shiftwidth=4 softtabstop=4 expandtab: */
/*
 * echo_client  : demo DCE RPC application
 *
 * Jim Doyle, jrd@bu.edu, 09-05-1998
 *
 *
 */
#ifdef _MK_HOST
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <compat/dcerpc.h>
#include "echo.h"
#include "echo_encoding.h"
#include <misc.h>

#ifndef _WIN32
#define PUBLIC
#define PRIVATE
#define EXTERNAL extern
#include <rpcdbg.h>
#include <dce/ntlmssp_types.h>
#include <termios.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include "getopt_internal.h"
#define SECURITY_WIN32
#include <security.h>
#endif

#define MAX_USER_INPUT 128
#define MAX_LINE 100 * 1024

#ifdef _WIN32
#define strcasecmp stricmp

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
    char * endpoint
    );

/*
 * usage()
 */

static void usage()
{
    printf("usage: echo_client [-S service] [-h hostname] [{-a name | -i} [-s] [-p level]] [-e endpoint] [-n] [-u] [-t] [-g count] [-c count] [-U user] [-D domain] [-P password | -] [-w]\n");
    printf("         -h:  specify host of RPC server (default is localhost)\n");
    printf("         -a:  specify authentication identity\n");
    printf("         -i:  inquire authentication identity from host\n");
    printf("         -s:  enable header signing\n");
    printf("         -p:  specify protection level\n");
    printf("         -e:  specify endpoint for protocol\n");
    printf("         -n:  use named pipe protocol\n");
    printf("         -u:  use UDP protocol\n");
    printf("         -t:  use TCP protocol (default)\n");
    printf("         -g:  instead of prompting, generate a data string of the specified length\n");
    printf("         -c:  call the function the specified number of times (default 1)\n");
    printf("         -d:  turn on debugging\n");
    printf("         -U:  Specify username for NTLM authentication\n");
    printf("         -P:  Specify password for NTLM authentication or - to prompt\n");
    printf("         -D:  Specify domain for NTLM authentication\n");
    printf("         -S:  Specify authentication service (krb5, negotiate, winnt)\n");
    printf("         -w:  Wrap the string in a DCE encoded packet before sending over in an RPC call\n");
    printf("\n");
    exit(1);
}

idl_boolean
ReverseWrappedWrapper(	
    rpc_binding_handle_t echo_server,
    args *inargs,
    args **outargs,
    unsigned32 *status
    )
{
    idl_es_handle_t encoding_handle = NULL;
    buffer in = { 0 };
    buffer out = { 0 };
    idl_boolean ok = 0;
    encoded_string_t decoded = NULL;
    error_status_t e;

    *outargs = NULL;
    *status = 0;

    idl_es_encode_dyn_buffer(
        (idl_byte **)(void *)&in.bytes,
        &in.size,
        &encoding_handle,
        status);
    if (*status != 0)
    {
        goto error;
    }

    DCETHREAD_TRY
    {
        if (inargs->argc >= 1)
        {
            string_encode(encoding_handle, inargs->argv[0]);
        }
        else
        {
            string_encode(encoding_handle, (idl_char *)"");
        }
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        printf("error encoding buffer\n");
        *status = dcethread_exc_getstatus(THIS_CATCH);
    }
    DCETHREAD_ENDTRY;
    if (*status != 0)
    {
        goto error;
    }
    idl_es_handle_free(&encoding_handle, status);
    if (*status != 0)
    {
        goto error;
    }

    ok = ReverseWrapped(	
        echo_server,
        &in,
        &out,
        status);
    if (!ok || *status != 0)
    {
        printf("ReverseWrapped failed %x\n", (unsigned int)*status);
        goto error;
    }

    idl_es_decode_buffer(
            (idl_byte *)out.bytes,
            out.size,
            &encoding_handle,
            status);
    if (*status != 0)
    {
        goto error;
    }

    DCETHREAD_TRY
    {
        string_decode(encoding_handle, &decoded);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        printf("\n\nFunction ReverseWrappedWrapper() -- error decoding size %d buffer\n", (unsigned int)out.size);
        *status = dcethread_exc_getstatus(THIS_CATCH);
    }
    DCETHREAD_ENDTRY;
    if (*status != 0)
    {
        goto error;
    }

    *outargs = (args *)malloc(sizeof(args));

    if (outargs == NULL)
    {
        exit(1);
    }

    (*outargs)->argc = 1;
    (*outargs)->argv[0] = decoded;
    decoded = NULL;

error:
    if (encoding_handle != NULL)
    {
        idl_es_handle_free(&encoding_handle, &e);
    }
    rpc_ss_client_free(decoded);
    rpc_ss_client_free(in.bytes);
    rpc_ss_client_free(out.bytes);
    return (*status == 0);
}

idl_boolean
AddOneWrapper(	
    rpc_binding_handle_t echo_server,
    args *inargs,
    args **outargs,
    unsigned32 *status
    )
{
    idl_es_handle_t encoding_handle = NULL;
    buffer in = { 0 };
    buffer out = { 0 };
    idl_boolean ok = 0;
    error_status_t e;
    idl_long_int num = 0;
    int j = 0;

    *outargs = NULL;
    *status = 0;

    idl_es_encode_dyn_buffer(
        (idl_byte **)(void *)&in.bytes,
        &in.size,
        &encoding_handle,
        status);
    if (*status != 0)
    {
        goto error;
    }

    if (inargs->argc >= 1)
    {
        num = strtol((char *)inargs->argv[0], NULL, 10);
    }
    else
    {
        num = 0;
    }

    DCETHREAD_TRY
    {
        int_encode(encoding_handle, num);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        printf("error encoding buffer\n");
        *status = dcethread_exc_getstatus(THIS_CATCH);
    }
    DCETHREAD_ENDTRY;
    if (*status != 0)
    {
        goto error;
    }
    idl_es_handle_free(&encoding_handle, status);
    if (*status != 0)
    {
        goto error;
    }

    printf("Sending buffer\n");

    for (j = 0; j < in.size; j++)
    {
        if (j % 16 == 0 && j != 0)
        {
            printf("\n");
        }
        printf("%3X", in.bytes[j] & 0xFF);
    }
    printf("\n");

    ok = AddOne(
        echo_server,
        &in,
        &out,
        status);
    if (!ok || *status != 0)
    {
        printf("AddOne failed %x\n", (unsigned int)*status);
        goto error;
    }

    printf("Received buffer\n");

    for (j = 0; j < out.size; j++)
    {
        if (j % 16 == 0 && j != 0)
        {
            printf("\n");
        }
        printf("%3X", out.bytes[j] & 0xFF);
    }
    printf("\n");

    idl_es_decode_buffer(
            (idl_byte *)out.bytes,
            out.size,
            &encoding_handle,
            status);
    if (*status != 0)
    {
        goto error;
    }

    DCETHREAD_TRY
    {
        int_decode(encoding_handle, &num);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        printf("\n\nFunction AddOneWrapped() -- error decoding size %d buffer\n", (unsigned int)out.size);
        *status = dcethread_exc_getstatus(THIS_CATCH);
    }
    DCETHREAD_ENDTRY;
    if (*status != 0)
    {
        goto error;
    }

    *outargs = (args *)malloc(sizeof(args));

    if (outargs == NULL)
    {
        exit(1);
    }

    (*outargs)->argc = 1;
    (*outargs)->argv[0] = malloc(100);
    sprintf((char *)(*outargs)->argv[0], "%d", (int)num);

error:
    if (encoding_handle != NULL)
    {
        idl_es_handle_free(&encoding_handle, &e);
    }
    rpc_ss_client_free(in.bytes);
    rpc_ss_client_free(out.bytes);
    return (*status == 0);
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
    unsigned_char_p_t spn = NULL;
    unsigned_char_p_t inquired_spn = NULL;
    int inquire_spn = FALSE;
    unsigned32 protect_level = rpc_c_protect_level_pkt_integ;
    unsigned32 flags = 0;

    char buf[MAX_LINE+1];
    char password_buffer[MAX_LINE+1];

    /*
     * stuff needed to make RPC calls
     */

    unsigned32 status;
    rpc_binding_handle_t echo_server;
    args * inargs;
    args * outargs;
    int ok;
    unsigned32 i;
    int generate_length = -1;
    int call_count = 1;
    int call = 0;
    int wrap = 0;
    int inc_number = 0;
    unsigned32 authn_svc = rpc_c_authn_gss_negotiate;

    char * nl;
    rpc_ntlmssp_auth_ident_t winnt = { 0 };
    winnt.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    /*
     * Process the cmd line args
     */

    while ((c = getopt(argc, argv, "S:sc:h:a:ip:e:nutdg:U:D:P:wI")) != EOF)
    {
        switch (c)
        {
        case 's':
#ifdef _WIN32
	    printf("This option is only supported on Linux. It is automatically enabled on Windows\n");
#else
            flags |= rpc_c_protect_flags_header_sign;
#endif
            break;
        case 'S':
            if (!strcasecmp(optarg, "gss_negotiate") ||
                !strcasecmp(optarg, "spnego") ||
                !strcasecmp(optarg, "negotiate") ||
                !strcasecmp(optarg, "rpc_c_authn_gss_negotiate") ||
                !strcasecmp(optarg, "authn_gss_negotiate") ||
                !strcasecmp(optarg, "authn_negotiate"))
            {
                authn_svc = rpc_c_authn_gss_negotiate;
            }
            else if (!strcasecmp(optarg, "winnt") ||
                !strcasecmp(optarg, "rpc_c_authn_winnt") ||
                !strcasecmp(optarg, "ntlm") ||
                !strcasecmp(optarg, "authn_ntlm") ||
                !strcasecmp(optarg, "authn_winnt"))
            {
                authn_svc = rpc_c_authn_winnt;
            }
            else if (!strcasecmp(optarg, "gss_mskrb") ||
                !strcasecmp(optarg, "gss_kerberos") ||
                !strcasecmp(optarg, "mskrb") ||
                !strcasecmp(optarg, "krb") ||
                !strcasecmp(optarg, "kerberos") ||
                !strcasecmp(optarg, "krb5") ||
                !strcasecmp(optarg, "rpc_c_authn_gss_mskrb") ||
                !strcasecmp(optarg, "authn_gss_mskrb"))
            {
                authn_svc = rpc_c_authn_gss_mskrb;
            }
            else if (!strcasecmp(optarg, "gss_tls") ||
                !strcasecmp(optarg, "tls") ||
                !strcasecmp(optarg, "schannel") ||
                !strcasecmp(optarg, "rpc_c_authn_gss_tls") ||
                !strcasecmp(optarg, "authn_gss_tls"))
            {
                authn_svc = rpc_c_authn_gss_tls;
            }
            else if (!strcasecmp(optarg, "default") ||
                !strcasecmp(optarg, "rpc_c_authn_default"))
            {
                authn_svc = rpc_c_authn_default;
            }
            else
            {
                printf ("Unknown authentication service %s\n", optarg);
                exit(1);
            }
            break;
        case 'h':
            rpc_host = optarg;
            break;
        case 'a':
            spn = (unsigned_char_p_t)optarg;
            break;
        case 'i':
            inquire_spn = TRUE;
            break;
        case 'p':
            protect_level = atoi(optarg);
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
        case 'd':
#ifdef _WIN32
	    printf("This option is only supported on Linux.\n");
#else
            rpc__dbg_set_switches("0-19.10", &status);
            //Skip 20, which is memory allocs and frees
            rpc__dbg_set_switches("21-24.10", &status);
            //Skip 25, which is fault injection for connection errors
            rpc__dbg_set_switches("26-43.10", &status);
#endif
            break;
        case 'c':
            call_count = strtol(optarg, NULL, 10);
            break;
        case 'w':
            wrap = 1;
            break;
        case 'I':
            inc_number = 1;
            break;
        case 'g':
            generate_length = strtol(optarg, NULL, 10);
            break;
        case 'U':
            winnt.User = optarg;
            winnt.UserLength = strlen(optarg);
            break;
        case 'P':
            if (!strcmp(optarg, "-") || !strcmp(optarg, "*"))
            {
                char *result = NULL;
#ifdef _WIN32
                FILE *tty = stdin;
                HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
                DWORD old, new;
                GetConsoleMode(hStdin, &old);

                new = old & ~ENABLE_ECHO_INPUT;

                SetConsoleMode(hStdin, new );
                fprintf(stdout, "%s", "Password: ");
                fflush(stdout);
#else
                struct termios old, new;
                FILE *tty = fopen("/dev/tty", "r+");

                tcgetattr(fileno(tty), &old);
                memcpy(&new, &old, sizeof(old));
                new.c_lflag &= ~(ECHO);
                tcsetattr(fileno(tty), TCSANOW, &new);
                fprintf(tty, "%s", "Password: ");
                fflush(tty);
#endif

                result = fgets(password_buffer, sizeof(password_buffer), tty);

#ifdef _WIN32
                fprintf(stdout, "\n");
                SetConsoleMode(hStdin, old );
#else
                fprintf(tty, "\n");
                tcsetattr(fileno(tty), TCSANOW, &old);
                fclose(tty);
#endif

                winnt.Password = password_buffer;
                winnt.PasswordLength = strlen(password_buffer);
                if (winnt.PasswordLength > 0 && winnt.Password[winnt.PasswordLength - 1] == '\r')
                {
                    winnt.PasswordLength--;
                    winnt.Password[winnt.PasswordLength] = 0;
                }
                if (winnt.PasswordLength > 0 && winnt.Password[winnt.PasswordLength - 1] == '\n')
                {
                    winnt.PasswordLength--;
                    winnt.Password[winnt.PasswordLength] = 0;
                }
            }
            else
            {
                winnt.Password = optarg;
                winnt.PasswordLength = strlen(optarg);
            }
            break;
        case 'D':
            winnt.Domain = optarg;
            winnt.DomainLength = strlen(optarg);
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

    if (get_client_rpc_binding(&echo_server,
                               echo_v1_0_c_ifspec,
                               rpc_host,
                               protocol,
                               endpoint) == 0)
    {
        printf ("Couldnt obtain RPC server binding. exiting.\n");
        exit(1);
    }

    if (inquire_spn)
    {
        rpc_mgmt_inq_server_princ_name(
            echo_server,
            authn_svc,
            &inquired_spn,
            &status);
        if (status)
        {
            printf ("Unable to inquire SPN %x. exiting.\n", (unsigned int)status);
            exit(1);
        }
        printf("Found SPN %s\n", inquired_spn);
        spn = inquired_spn;
    }
    if (spn)
    {
        rpc_binding_set_auth_info_2(
            echo_server,
            spn,
            protect_level,
            authn_svc,
            flags,
            authn_svc == rpc_c_authn_winnt ? (rpc_auth_identity_handle_t)(void*) &winnt : NULL,
            rpc_c_authz_name, &status);
        if (status)
        {
            printf ("Couldn't set auth info %u. exiting.\n", (unsigned int)status);
            exit(1);
        }
    }

    /*
     * Allocate an "args" struct with enough room to accomodate
     * the max number of lines of text we can can from stdin.
     */

    inargs = (args *)malloc(sizeof(args) + MAX_USER_INPUT * sizeof(string_t));
    if (inargs == NULL) printf("FAULT. Didnt allocate inargs.\n");

    if (generate_length < 0)
    {
        /*
         * Get text from the user and pack into args.
         */

        printf ("enter stuff (%s on an empty line when done):\n\n\n", EOF_STRING);
        i = 0;
        while (!feof(stdin) && i < MAX_USER_INPUT )
        {
            if (NULL==fgets(buf, MAX_LINE, stdin))
                break;
            if ((nl=strchr(buf, '\n')))                   /* strip the newline */
                *nl=0;
            inargs->argv[i] = (string_t)strdup(buf);      /* copy from buf */
            i++;
        }
        inargs->argc = i;
    }
    else
    {
        inargs->argv[0] = malloc(generate_length + 1);
        inargs->argv[0][0] = 's';

        for(i = 1; i < (unsigned long)generate_length; i++)
        {
            inargs->argv[0][i] = i%10 + '0';
        }

        if(generate_length > 0)
            inargs->argv[0][generate_length - 1] = 'e';
        inargs->argv[0][generate_length] = '\0';
        inargs->argc = 1;
    }

    /*
     * Do the RPC call
     */

    for (call = 0; call < call_count; call++)
    {
        printf ("calling server\n");
        if (inc_number)
        {
            ok = AddOneWrapper(echo_server, inargs, &outargs, &status);
        }
        else if (wrap)
        {
            ok = ReverseWrappedWrapper(echo_server, inargs, &outargs, &status);
        }
        else
        {
            ok = ReverseIt(echo_server, inargs, &outargs, &status);
        }

        /*
         * Print the results
         */

        if (ok && status == error_status_ok)
        {
            printf ("got response from server. results: \n");
            for (i=0; i<outargs->argc; i++)
                printf("\t[%lu]: %s\n", (unsigned long)i, outargs->argv[i]);
            printf("\n===================================\n");

        }

        if (status != error_status_ok)
            chk_dce_err(status, "ReverseIt()", "main()", 1);
    }

    /*
     * Done. Now gracefully teardown the RPC binding to the server
     */

    if (inquired_spn)
    {
        rpc_string_free((unsigned char **)&inquired_spn, &status);
    }
    rpc_binding_free(&echo_server, &status);
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
    char * endpoint
    )
{
    unsigned char * string_binding = NULL;
    error_status_t status;

    /*
     * create a string binding given the command line parameters and
     * resolve it into a full binding handle using the endpoint mapper.
     *  The binding handle resolution is handled by the runtime library
     */

    rpc_string_binding_compose(NULL,
			       (unsigned_char_p_t)protocol,
			       (unsigned_char_p_t)hostname,
			       (unsigned_char_p_t)endpoint,
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

