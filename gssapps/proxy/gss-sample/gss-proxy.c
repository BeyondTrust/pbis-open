/*
 * Copyright 1994 by OpenVision Technologies, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (C) 2004 by the Massachusetts Institute of Technology.
 * All rights reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */

#include <config.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <ctype.h>

#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include "gss-misc.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <ntlm/sspintlm.h>
#include <ntlm/gssntlm.h>

static void usage(void);

static void usage(void)
{
    fprintf(stderr, "Usage: gssproxy-proxy [-port port] [-mech mechanism]\n");
    fprintf(stderr, "       [-forceclientmech mechanism]\n");
    fprintf(stderr, "       [-domain domain]\n");
    fprintf(stderr, "       [-verbose] [-once] [-inetd] [-export] [-logfile file]\n");
    fprintf(stderr, "       remote_host remote_service\n");
    exit(1);
}

FILE *log_file;

int verbose = 0;

unsigned int stop = 0;

/*
 * Function: server_acquire_creds
 *
 * Purpose: imports a service name and acquires credentials for it
 *
 * Arguments:
 *
 *      server_creds    (w) the GSS-API service credentials
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * The service name is imported with gss_import_name, and service
 * credentials are acquired with gss_acquire_cred.  If either operation
 * fails, an error message is displayed and -1 is returned; otherwise,
 * 0 is returned.
 */
static int server_acquire_creds(
    gss_cred_id_t *server_creds
    )
{
    int ret = 0;
    OM_uint32 maj_stat = 0, min_stat = 0;
    gss_name_t src_name = GSS_C_NO_NAME;
    gss_buffer_desc sname = GSS_C_EMPTY_BUFFER;

    maj_stat = gss_acquire_cred(&min_stat, GSS_C_NO_NAME, 0,
                                GSS_C_NULL_OID_SET, GSS_C_BOTH,
                                server_creds, NULL, NULL);
    if (maj_stat != GSS_S_COMPLETE)
    {
        display_status("acquiring credentials", maj_stat, min_stat);
        ret = -1;
        goto error;
    }

    maj_stat = gss_inquire_cred(&min_stat, *server_creds, &src_name,
                                NULL, NULL, NULL);
    if (maj_stat != GSS_S_COMPLETE)
    {
        display_status("obtaining source name", maj_stat, min_stat);
        ret = -1;
        goto error;
    }

    maj_stat = gss_display_name(&min_stat, src_name, &sname,
                                NULL);
    if (maj_stat != GSS_S_COMPLETE)
    {
        display_status("displaying source name", maj_stat, min_stat);
        ret = -1;
        goto error;
    }

    printf("Proxy name: %.*s\n", (int)sname.length, (char *)sname.value);

error:
    (void)gss_release_name(&min_stat, &src_name);
    (void)gss_release_buffer(&min_stat, &sname);

    return ret;
}

/*
 * Function: server_establish_context
 *
 * Purpose: establishses a GSS-API context as a specified service with
 * an incoming client, and returns the context handle and associated
 * client name
 *
 * Arguments:
 *
 *      s               (r) an established TCP connection to the client
 *      service_creds   (r) server credentials, from gss_acquire_cred
 *      context         (w) the established GSS-API context
 *      client_name     (w) the client's ASCII name
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * Any valid client request is accepted.  If a context is established,
 * its handle is returned in context and the client name is returned
 * in client_name and 0 is returned.  If unsuccessful, an error
 * message is displayed and -1 is returned.
 */
static int server_establish_context(
    int s,
    gss_OID oid,
    char *domain,
    gss_cred_id_t server_creds,
    gss_ctx_id_t *context,
    gss_name_t *ret_client,
    OM_uint32 *ret_flags
    )
{
    int ret = 0;
    gss_buffer_desc send_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc recv_tok = GSS_C_EMPTY_BUFFER;
    gss_OID doid = GSS_C_NO_OID;
    OM_uint32 maj_stat = 0;
    OM_uint32 min_stat = 0;
    OM_uint32 acc_sec_min_stat = 0;
    gss_buffer_desc oid_name = GSS_C_EMPTY_BUFFER;
    gss_name_t client = GSS_C_NO_NAME;
    gss_name_t server = GSS_C_NO_NAME;
    gss_buffer_desc client_name = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc server_name = GSS_C_EMPTY_BUFFER;
    unsigned int token_flags = 0;
    gss_OID_set desired_mechs = GSS_C_NO_OID_SET;
    gss_cred_id_t cred = GSS_C_NO_CREDENTIAL;
    gss_OID_desc GssCredOptionDomainOidDesc = {
        .length = GSS_CRED_OPT_DOMAIN_LEN,
        .elements = GSS_CRED_OPT_DOMAIN
        };
    gss_OID GssCredOptionDomainOid = &GssCredOptionDomainOidDesc;
    gss_buffer_desc domain_tok = GSS_C_EMPTY_BUFFER;
    char *tmp_name = NULL;

    if (recv_token(s, &token_flags, &recv_tok) < 0)
    {
        ret = -1;
        goto error;
    }

    if (recv_tok.value)
    {
        free (recv_tok.value);
        recv_tok.value = NULL;
    }

    if (! (token_flags & TOKEN_NOOP))
    {
        if (log_file)
            fprintf(log_file, "Expected NOOP token, got %d token instead\n",
                    token_flags);
        ret = -1;
        goto error;
    }

    if (oid)
    {
        maj_stat = gss_create_empty_oid_set(&min_stat,
                                            &desired_mechs);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("creating OID set", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        maj_stat = gss_add_oid_set_member(&min_stat,
                                          oid,
                                          &desired_mechs);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("adding OID set member", maj_stat, min_stat);
            ret = -1;
            goto error;
        }
    }
    if (oid || domain)
    {
        maj_stat = gss_acquire_cred(&min_stat,
                                    GSS_C_NO_NAME,
                                    0,
                                    desired_mechs,
                                    GSS_C_ACCEPT,
                                    &cred,
                                    NULL,
                                    NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("acquiring credentials", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        (void) gss_release_oid_set(&min_stat, &desired_mechs);
    }
    if (domain)
    {
        domain_tok.length = strlen(domain);
        domain_tok.value = domain;

        maj_stat = gssspi_set_cred_option(&min_stat,
                                          cred,
                                          GssCredOptionDomainOid,
                                          &domain_tok);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("setting cred option", maj_stat, min_stat);
            fprintf(log_file, "The most likely reason is the -domain option was\n");
            fprintf(log_file, "used without -forceclientmech for either SPNEGO\n");
            fprintf(log_file, "or NTLM and it defaulted to KRB5 which doesn't\n");
            fprintf(log_file, "support this option.\n");
            ret = -1;
            goto error;
        }
    }

    *context = GSS_C_NO_CONTEXT;

    if (token_flags & TOKEN_CONTEXT_NEXT)
    {
        do
        {
            if (recv_token(s, &token_flags, &recv_tok) < 0)
            {
                ret = -1;
                goto error;
            }

            if (verbose && log_file)
            {
                fprintf(log_file, "Received token (size=%d): \n", (int) recv_tok.length);
                print_token(&recv_tok);
            }

            maj_stat =
                gss_accept_sec_context(&acc_sec_min_stat,
                                       context,
                                       cred,
                                       &recv_tok,
                                       GSS_C_NO_CHANNEL_BINDINGS,
                                       &client,
                                       &doid,
                                       &send_tok,
                                       ret_flags,
                                       NULL, /* ignore time_rec */
                                       NULL); /* ignore del_cred_handle */

            if (recv_tok.value)
            {
                free(recv_tok.value);
                recv_tok.value = NULL;
            }

            if (send_tok.length != 0)
            {
                if (verbose && log_file)
                {
                    fprintf(log_file,
                            "Sending accept_sec_context token (size=%d):\n",
                            (int) send_tok.length);
                    print_token(&send_tok);
                }
                if (send_token(s, TOKEN_CONTEXT, &send_tok) < 0)
                {
                    if (log_file)
                        fprintf(log_file, "failure sending token\n");
                    ret = -1;
                    goto error;
                }

                (void) gss_release_buffer(&min_stat, &send_tok);
            }

            if (maj_stat!=GSS_S_COMPLETE && maj_stat!=GSS_S_CONTINUE_NEEDED)
            {
                display_status("accepting context", maj_stat,
                               acc_sec_min_stat);
                ret = -1;
                goto error;
            }

            if (verbose && log_file)
            {
                if (maj_stat == GSS_S_CONTINUE_NEEDED)
                    fprintf(log_file, "continue needed...\n");
                else
                    fprintf(log_file, "\n");
                fflush(log_file);
            }
        } while (maj_stat == GSS_S_CONTINUE_NEEDED);

        (void) gss_release_cred(&min_stat, &cred);

        if (verbose && log_file)
        {
            maj_stat = gss_oid_to_str(&min_stat, doid, &oid_name);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("converting oid->string", maj_stat, min_stat);
                ret = -1;
                goto error;
            }
        }

        maj_stat = gss_inquire_context(&min_stat, *context, NULL, &server,
                                       NULL, NULL, NULL, NULL, NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("inquiring context", maj_stat, min_stat);
            ret = -1;
            goto error;
        }
        if (server)
        {
            maj_stat = gss_display_name(&min_stat, server, &server_name, NULL);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("displaying name", maj_stat, min_stat);
                ret = -1;
                goto error;
            }
        }
        maj_stat = gss_display_name(&min_stat, client, &client_name, NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("displaying name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        printf("Accepted authenticated connection.\n"
               "  with mech %.*s (%s)\n"
               "       from \"%.*s\"\n"
               "         as \"%.*s\"\n",
               (int) oid_name.length, (char *) oid_name.value,
               SAFE_STR(oid_to_label(&oid_name)),
               (int) client_name.length, (char *) client_name.value,
               (int) server_name.length, (char *) server_name.value);

        (void) gss_release_buffer(&min_stat, &server_name);
        (void) gss_release_name(&min_stat, &server);

        /* display the flags */
        display_ctx_flags(*ret_flags);
    }
    else
    {
        *ret_flags = 0;

        if (log_file)
            fprintf(log_file, "Accepted unauthenticated connection.\n");
    }

    *ret_client = GSS_C_NO_NAME;

    if (oid_name.length && !strncmp(MECH_OID_STRING_NTLM, oid_name.value, oid_name.length))
    {
        char *suffix = NULL;
        OM_uint32 offset = 0;

        for (suffix = client_name.value, offset = 0 ;
             *suffix != '\\' && offset < client_name.length ;
             suffix++, offset++);

        if (offset < client_name.length && *suffix == '\\')
        {
            gss_buffer_desc name_tok;
            size_t len = 0;

            suffix++;
            offset++;

            if (domain)
            {
                len += strlen(suffix) + strlen(domain) + 2;
                tmp_name = malloc(len);
                if (tmp_name == NULL)
                {
                    if (log_file)
                        fprintf(log_file, "Couldn't allocate memory for name.\n");
                    ret = 1;
                    goto error;
                }
                snprintf(tmp_name, len, "%s@%s", suffix, domain);
            }

            name_tok.value = tmp_name ? tmp_name : suffix;
            name_tok.length = tmp_name ? strlen(tmp_name) :
                                         client_name.length - offset;
            maj_stat = gss_import_name(&min_stat, &name_tok,
                                       (gss_OID) gss_nt_user_name,
                                       ret_client);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("parsing name", maj_stat, min_stat);
                ret = -1;
                goto error;
            }

        }

    }

    if (*ret_client == GSS_C_NO_NAME)
    {
        *ret_client = client;
    }
    else
    {
        (void) gss_release_name(&min_stat, &client);
    }

error:
    if (ret)
    {
        gss_delete_sec_context(&min_stat, context,
                               GSS_C_NO_BUFFER);
    }
    (void) gss_release_cred(&min_stat, &cred);
    (void) gss_release_buffer(&min_stat, &client_name);
    (void) gss_release_buffer(&min_stat, &oid_name);
    (void) gss_release_oid_set(&min_stat, &desired_mechs);

    if (tmp_name)
    {
        free(tmp_name);
    }

    return ret;
}

/*
 * Function: create_socket
 *
 * Purpose: Opens a listening TCP socket.
 *
 * Arguments:
 *
 *      port            (r) the port number on which to listen
 *
 * Returns: the listening socket file descriptor, or -1 on failure
 *
 * Effects:
 *
 * A listening socket on the specified port and created and returned.
 * On error, an error message is displayed and -1 is returned.
 */
static int create_socket(
    u_short port
    )
{
    struct sockaddr_in saddr;
    int s = -1;
    int on = 1;

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("creating socket");
        goto error;
    }
    /* Let the socket be reused right away */
    (void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    if (bind(s, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
    {
        perror("binding socket");
        (void) close(s);
        s = -1;
        goto error;
    }
    if (listen(s, 5) < 0)
    {
        perror("listening on socket");
        (void) close(s);
        s = -1;
        goto error;
    }

error:
    return s;
}

static float timeval_subtract(
    struct timeval *tv1,
    struct timeval *tv2
    )
{
    return ((tv1->tv_sec - tv2->tv_sec) +
            ((float) (tv1->tv_usec - tv2->tv_usec)) / 1000000);
}

/*
 * Yes, yes, this isn't the best place for doing this test.
 * DO NOT REMOVE THIS UNTIL A BETTER TEST HAS BEEN WRITTEN, THOUGH.
 *                                      -TYT
 */
static int test_import_export_context(
    gss_ctx_id_t *context
    )
{
    int ret = 0;
    OM_uint32 min_stat = 0, maj_stat = 0;
    gss_buffer_desc context_token = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc copied_token = GSS_C_EMPTY_BUFFER;
    struct timeval tm1, tm2;

    /*
     * Attempt to save and then restore the context.
     */
    gettimeofday(&tm1, (struct timezone *)0);

    maj_stat = gss_export_sec_context(&min_stat, context, &context_token);
    if (maj_stat != GSS_S_COMPLETE)
    {
        display_status("exporting context", maj_stat, min_stat);
        ret = 1;
        goto error;
    }

    gettimeofday(&tm2, (struct timezone *)0);
    if (verbose && log_file)
    {
        fprintf(log_file, "Exported context: %d bytes, %7.4f seconds\n",
                (int) context_token.length,
                timeval_subtract(&tm2, &tm1));
    }

    copied_token.length = context_token.length;
    copied_token.value = malloc(context_token.length);
    if (copied_token.value == 0)
    {
        if (log_file)
            fprintf(log_file, "Couldn't allocate memory to copy context token.\n");
        ret = 1;
        goto error;
    }

    memcpy(copied_token.value, context_token.value, copied_token.length);

    maj_stat = gss_import_sec_context(&min_stat, &copied_token, context);
    if (maj_stat != GSS_S_COMPLETE) {
        display_status("importing context", maj_stat, min_stat);
        return 1;
    }

    free(copied_token.value);

    gettimeofday(&tm1, (struct timezone *)0);
    if (verbose && log_file)
    {
        fprintf(log_file, "Importing context: %7.4f seconds\n",
                timeval_subtract(&tm1, &tm2));
    }

error:
    (void) gss_release_buffer(&min_stat, &context_token);

    return ret;
}

/*
 * Function: connect_to_server
 *
 * Purpose: Opens a TCP connection to the name host and port.
 *
 * Arguments:
 *
 *      host            (r) the target host name
 *      port            (r) the target port, in host byte order
 *
 * Returns: the established socket file desciptor, or -1 on failure
 *
 * Effects:
 *
 * The host name is resolved with gethostbyname(), and the socket is
 * opened and connected.  If an error occurs, an error message is
 * displayed and -1 is returned.
 */
static int connect_to_server(
    char *host,
    u_short port
    )
{
    struct sockaddr_in saddr;
    struct hostent *hp;
    int s = -1;

    if ((hp = gethostbyname(host)) == NULL)
    {
        fprintf(stderr, "Unknown host: %s\n", host);
        goto error;
    }

    saddr.sin_family = hp->h_addrtype;
    memcpy((char *)&saddr.sin_addr, hp->h_addr, sizeof(saddr.sin_addr));
    saddr.sin_port = htons(port);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("creating socket");
        goto error;
    }

    if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("connecting to server");
        (void) close(s);
        s = -1;
        goto error;
    }

error:

    return s;
}

/*
 * Function: client_establish_context
 *
 * Purpose: establishes a GSS-API context with a specified service and
 * returns the context handle
 *
 * Arguments:
 *
 *      s               (r) an established TCP connection to the service
 *      service_name    (r) the ASCII service name of the service
 *      gss_flags       (r) GSS-API delegation flag (if any)
 *      auth_flag       (r) whether to actually do authentication
 *      oid             (r) OID of the mechanism to use
 *      context         (w) the established GSS-API context
 *      ret_flags       (w) the returned flags from init_sec_context
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * service_name is imported as a GSS-API name and a GSS-API context is
 * established with the corresponding service; the service should be
 * listening on the TCP connection s.  The default GSS-API mechanism
 * is used, and mutual authentication and replay detection are
 * requested.
 *
 * If successful, the context handle is returned in context.  If
 * unsuccessful, the GSS-API error messages are displayed on stderr
 * and -1 is returned.
 */
static int client_establish_context(
    int s,
    gss_cred_id_t impersonator_creds,
    char *service_name,
    OM_uint32 gss_flags,
    int auth_flag,
    gss_OID oid,
    gss_ctx_id_t *gss_context,
    OM_uint32 *ret_flags
    )
{
    int ret = 0;
    gss_buffer_desc send_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc recv_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc *token_ptr = GSS_C_NO_BUFFER;
    gss_name_t target_name = GSS_C_NO_NAME;
    OM_uint32 maj_stat = 0;
    OM_uint32 min_stat = 0;
    OM_uint32 init_sec_min_stat = 0;
    unsigned int token_flags = 0;

    *gss_context = GSS_C_NO_CONTEXT;

    if (auth_flag)
    {

        /*
         * Import the name into target_name.  Use send_tok to save
         * local variable space.
         */
        send_tok.value = service_name;
        send_tok.length = strlen(service_name) ;
        maj_stat = gss_import_name(&min_stat, &send_tok,
                                   (gss_OID) gss_nt_service_name, &target_name);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("parsing name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        if (send_token(s, TOKEN_NOOP|TOKEN_CONTEXT_NEXT, empty_token) < 0)
        {
            ret = -1;
            goto error;
        }

        /*
         * Perform the context-establishement loop.
         *
         * On each pass through the loop, token_ptr points to the token
         * to send to the server (or GSS_C_NO_BUFFER on the first pass).
         * Every generated token is stored in send_tok which is then
         * transmitted to the server; every received token is stored in
         * recv_tok, which token_ptr is then set to, to be processed by
         * the next call to gss_init_sec_context.
         *
         * GSS-API guarantees that send_tok's length will be non-zero
         * if and only if the server is expecting another token from us,
         * and that gss_init_sec_context returns GSS_S_CONTINUE_NEEDED if
         * and only if the server has another token to send us.
         */

        do
        {
            maj_stat =
                gss_init_sec_context(&init_sec_min_stat,
                                     impersonator_creds,
                                     gss_context,
                                     target_name,
                                     oid,
                                     gss_flags,
                                     0,
                                     NULL,      /* no channel bindings */
                                     token_ptr,
                                     NULL,      /* ignore mech type */
                                     &send_tok,
                                     ret_flags,
                                     NULL);     /* ignore time_rec */

            if (token_ptr != GSS_C_NO_BUFFER)
                free (recv_tok.value);

            if (send_tok.length != 0)
            {
                if (verbose)
                    printf("Sending init_sec_context token (size=%d)...",
                           (int) send_tok.length);
                if (send_token(s, TOKEN_CONTEXT, &send_tok) < 0)
                {
                    ret = -1;
                    goto error;
                }
            }
            (void) gss_release_buffer(&min_stat, &send_tok);

            if (maj_stat!=GSS_S_COMPLETE && maj_stat!=GSS_S_CONTINUE_NEEDED)
            {
                display_status("initializing context", maj_stat,
                               init_sec_min_stat);
                ret = -1;
                goto error;
            }

            if (maj_stat == GSS_S_CONTINUE_NEEDED)
            {
                if (verbose)
                    printf("continue needed...");
                if (recv_token(s, &token_flags, &recv_tok) < 0)
                {
                    ret = -1;
                    goto error;
                }
                token_ptr = &recv_tok;
            }
            if (verbose)
                printf("\n");
        } while (maj_stat == GSS_S_CONTINUE_NEEDED);

        (void) gss_release_name(&min_stat, &target_name);
    }
    else {
        if (send_token(s, TOKEN_NOOP, empty_token) < 0)
        {
            ret = -1;
            goto error;
        }
    }

error:
    if (ret)
    {
        gss_delete_sec_context(&min_stat, gss_context,
                               GSS_C_NO_BUFFER);
    }
    (void) gss_release_buffer(&min_stat, &send_tok);
    (void) gss_release_name(&min_stat, &target_name);

    return ret;
}

static void parse_oid(char *mechanism, gss_OID *oid)
{
    char *mechstr = NULL, *cp = NULL;
    gss_buffer_desc tok = GSS_C_EMPTY_BUFFER;
    OM_uint32 maj_stat = 0, min_stat = 0;

    if (isdigit((int) mechanism[0]))
    {
        mechstr = malloc(strlen(mechanism)+5);
        if (!mechstr)
        {
            fprintf(stderr, "Couldn't allocate mechanism scratch!\n");
            goto error;
        }

        sprintf(mechstr, "{ %s }", mechanism);
        for (cp = mechstr; *cp; cp++)
            if (*cp == '.')
                *cp = ' ';
        tok.value = mechstr;
    }
    else
        tok.value = mechanism;

    tok.length = strlen(tok.value);

    maj_stat = gss_str_to_oid(&min_stat, &tok, oid);
    if (maj_stat != GSS_S_COMPLETE)
    {
        display_status("str_to_oid", maj_stat, min_stat);
        goto error;
    }

error:
    if (mechstr)
        free(mechstr);
}

static int begin_server_connection(
    gss_cred_id_t server_creds,
    gss_name_t client,
    char *remote_host,
    char *remote_service_name,
    u_short port,
    gss_OID oid,
    OM_uint32 proxy_ret_flags,
    int auth_flag,
    int *client_sock,
    gss_ctx_id_t *client_context
    )
{
    int ret = 0;
    OM_uint32 maj_stat = 0, min_stat = 0;
    int i = 0;
    OM_uint32 client_ret_flags = 0;
    gss_name_t src_name = GSS_C_NO_NAME;
    gss_name_t targ_name = GSS_C_NO_NAME;
    gss_buffer_desc sname = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc tname = GSS_C_EMPTY_BUFFER;
    OM_uint32 lifetime = 0;
    gss_OID mechanism = GSS_C_NO_OID;
    gss_OID name_type = GSS_C_NO_OID;
    int is_local = 0;
    OM_uint32 client_context_flags = 0;
    int is_open = 0;
    gss_OID_set mech_names = GSS_C_NO_OID_SET;
    gss_buffer_desc oid_name = GSS_C_EMPTY_BUFFER;
    gss_name_t server = GSS_C_NO_NAME;
    gss_buffer_desc client_name = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc server_name = GSS_C_EMPTY_BUFFER;
    gss_cred_id_t impersonator_creds = GSS_C_NO_CREDENTIAL;

    /* Open connection to server */
    if ((*client_sock = connect_to_server(remote_host, port)) < 0)
    {
        ret = -1;
        goto error;
    }

    if (auth_flag)
    {
        maj_stat = gss_acquire_cred_impersonate_name(&min_stat,
                                                 server_creds,
                                                 client,
                                                 GSS_C_INDEFINITE,
                                                 GSS_C_NULL_OID_SET,
                                                 GSS_C_INITIATE,
                                                 &impersonator_creds,
                                                 NULL,
                                                 NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("acquiring impersonate credentials", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        maj_stat = gss_inquire_cred(&min_stat, server_creds, &server,
                                    NULL, NULL, NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("displaying name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        maj_stat = gss_display_name(&min_stat, server, &server_name, NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("displaying name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        maj_stat = gss_display_name(&min_stat, client, &client_name, NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("displaying name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        printf("Acquired credentials: for \"%.*s\" as \"%.*s\"\n",
               (int) client_name.length, (char *) client_name.value,
               (int) server_name.length, (char *) server_name.value);
    }

    /* Establish context */
    if (client_establish_context(*client_sock, impersonator_creds,
                                 remote_service_name,
                                 proxy_ret_flags, auth_flag,
                                 oid, client_context,
                                 &client_ret_flags) < 0)
    {
        (void) close(*client_sock);
        *client_sock = -1;
        ret = -1;
        goto error;
    }

    if (auth_flag)
    {
        /* Get context information */
        maj_stat = gss_inquire_context(&min_stat, *client_context,
                                       &src_name, &targ_name, &lifetime,
                                       &mechanism, &client_context_flags,
                                       &is_local,
                                       &is_open);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("inquiring context", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        maj_stat = gss_display_name(&min_stat, src_name, &sname,
                                    &name_type);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("displaying source name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }
        maj_stat = gss_display_name(&min_stat, targ_name, &tname,
                                    (gss_OID *) NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("displaying target name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        maj_stat = gss_oid_to_str(&min_stat, mechanism, &oid_name);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("converting oid->string", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        printf("Authenticated to server.\n"
               "  with mech %.*s (%s)\n"
               "         as \"%.*s\"\n"
               "         to \"%.*s\"\n",
               (int) oid_name.length, (char *) oid_name.value,
               SAFE_STR(oid_to_label(&oid_name)),
               (int) sname.length, (char *) sname.value,
               (int) tname.length, (char *) tname.value);
        if (verbose)
        {
            printf("   lifetime %u\n"
                   "   %s\n"
                   "   %s\n",
            lifetime,
            (is_local) ? "locally initiated" : "remotely initiated",
            (is_open) ? "open" : "closed");
        }

        /* display the flags */
        display_ctx_flags(client_context_flags);
        if (verbose)
        {
            display_ctx_flags(client_ret_flags);
        }

        (void) gss_release_buffer(&min_stat, &oid_name);

        if (verbose)
        {
            maj_stat = gss_oid_to_str(&min_stat,
                                      name_type,
                                      &oid_name);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("converting oid->string", maj_stat, min_stat);
                ret = -1;
                goto error;
            }
            printf("Name type of source name is %.*s.\n",
                   (int) oid_name.length, (char *) oid_name.value);
            (void) gss_release_buffer(&min_stat, &oid_name);

            /* Now get the names supported by the mechanism */
            maj_stat = gss_inquire_names_for_mech(&min_stat,
                                                  mechanism,
                                                  &mech_names);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("inquiring mech names", maj_stat, min_stat);
                ret = -1;
                goto error;
            }

            maj_stat = gss_oid_to_str(&min_stat,
                                      mechanism,
                                      &oid_name);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("converting oid->string", maj_stat, min_stat);
                ret = -1;
                goto error;
            }
            printf("Mechanism %.*s supports %d names\n",
                   (int) oid_name.length, (char *) oid_name.value,
                   (int) mech_names->count);
            (void) gss_release_buffer(&min_stat, &oid_name);

            for (i=0; i<mech_names->count; i++)
            {
                maj_stat = gss_oid_to_str(&min_stat,
                                          &mech_names->elements[i],
                                          &oid_name);
                if (maj_stat != GSS_S_COMPLETE)
                {
                    display_status("converting oid->string", maj_stat, min_stat);
                    ret = -1;
                    goto error;
                }
                printf("  %d: %.*s (%s)\n", (int) i,
                       (int) oid_name.length, (char *) oid_name.value,
                       SAFE_STR(oid_to_label(&oid_name)));

                (void) gss_release_buffer(&min_stat, &oid_name);
            }
            (void) gss_release_oid_set(&min_stat, &mech_names);
        }
    }

error:
    if (ret)
    {
        gss_delete_sec_context(&min_stat, client_context,
                               GSS_C_NO_BUFFER);
    }
    (void) gss_release_oid_set(&min_stat, &mech_names);
    (void) gss_release_name(&min_stat, &src_name);
    (void) gss_release_name(&min_stat, &targ_name);
    (void) gss_release_buffer(&min_stat, &sname);
    (void) gss_release_buffer(&min_stat, &tname);
    (void) gss_release_buffer(&min_stat, &oid_name);
    (void) gss_release_cred(&min_stat, &impersonator_creds);
    (void) gss_release_buffer(&min_stat, &client_name);
    (void) gss_release_buffer(&min_stat, &server_name);
    (void) gss_release_name(&min_stat, &server);

    return ret;
}

static int send_recv_server(
    int client_sock,
    unsigned int proxy_token_flags,
    gss_ctx_id_t client_context,
    gss_buffer_desc msg_buf,
    int proxy_conf_state
    )
{
    int ret = 0;
    gss_buffer_desc client_xmit_buf = GSS_C_EMPTY_BUFFER;
    OM_uint32 maj_stat = 0, min_stat = 0;
    int client_conf_state = 0;
    gss_qop_t client_qop_state = 0;
    unsigned int client_token_flags = 0;

    if (proxy_token_flags & TOKEN_WRAPPED)
    {
        maj_stat = gss_wrap(&min_stat, client_context,
                            proxy_conf_state, GSS_C_QOP_DEFAULT,
                            &msg_buf, &client_conf_state, &client_xmit_buf);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("wrapping message", maj_stat, min_stat);
            ret = -1;
            goto error;
        }
        else if (proxy_conf_state && ! client_conf_state)
        {
            fprintf(stderr, "Warning!  Message not encrypted.\n");
        }
    }
    else
    {
        client_xmit_buf = msg_buf;
    }

    /* Send to server */
    if (send_token(client_sock, (TOKEN_DATA |
                                   ((proxy_token_flags & TOKEN_WRAPPED) ? TOKEN_WRAPPED : 0) |
                                   (proxy_conf_state ? TOKEN_ENCRYPTED : 0) |
                                   ((proxy_token_flags & TOKEN_SEND_MIC) ? TOKEN_SEND_MIC : 0)), &client_xmit_buf) < 0)
    {
        ret = -1;
        goto error;
    }
    
    printf("Sent %s message to server.\n",
           (proxy_token_flags & TOKEN_ENCRYPTED) ? "encrypted" :
           ((proxy_token_flags & TOKEN_WRAPPED) ? "wrapped" : "clear"));

    if (client_xmit_buf.value != msg_buf.value)
        (void) gss_release_buffer(&min_stat, &client_xmit_buf);

    /* Read signature block into client_xmit_buf */
    if (recv_token(client_sock, &client_token_flags, &client_xmit_buf) < 0)
    {
        ret = -1;
        goto error;
    }

    if (proxy_token_flags & TOKEN_SEND_MIC)
    {
        /* Verify signature block */
        maj_stat = gss_verify_mic(&min_stat, client_context, &msg_buf,
                                  &client_xmit_buf, &client_qop_state);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("verifying signature", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        printf("Signature verified.\n");
    }
    else
    {
        printf("Response received.\n");
    }


error:
    if (ret)
    {
        (void) close(client_sock);
        (void) gss_delete_sec_context(&min_stat, &client_context,
                                      GSS_C_NO_BUFFER);
    }
    free (client_xmit_buf.value);

    return ret;
}

/*
 * Function: sign_proxy
 *
 * Purpose: Performs the "sign" service.
 *
 * Arguments:
 *
 *      proxy_sock      (r) a TCP socket on which a connection has been
 *                      accept()ed
 *      service_name    (r) the ASCII name of the GSS-API service to
 *                      establish a context as
 *      export          (r) whether to test context exporting
 *
 * Returns: -1 on error
 *
 * Effects:
 *
 * sign_proxy establishes a context, and performs a single sign request.
 *
 * A sign request is a single GSS-API sealed token.  The token is
 * unsealed and a signature block, produced with gss_sign, is returned
 * to the sender.  The context is the destroyed and the connection
 * closed.
 *
 * If any error occurs, -1 is returned.
 */
static int sign_proxy(
    int proxy_sock,
    gss_cred_id_t server_creds,
    int export,
    u_short port,
    char *remote_host,
    char *remote_service_name,
    char *domain,
    gss_OID oid,
    gss_OID clientoid
    )
{
    int ret = 0;
    gss_buffer_desc proxy_xmit_buf = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc msg_buf = GSS_C_EMPTY_BUFFER;
    gss_ctx_id_t proxy_context = GSS_C_NO_CONTEXT;
    gss_ctx_id_t client_context = GSS_C_NO_CONTEXT;
    gss_name_t client = GSS_C_NO_NAME;
    OM_uint32 maj_stat = 0, min_stat = 0;
    int i = 0, proxy_conf_state = 0;
    OM_uint32 proxy_ret_flags = 0;
    char *cp = NULL;
    unsigned int proxy_token_flags = 0;
    int auth_flag = 0;
    int client_sock = -1;


    /* Establish a context with the client */
    if (server_establish_context(proxy_sock, clientoid, domain,
                                 server_creds,
                                 &proxy_context, &client,
                                 &proxy_ret_flags) < 0)
    {
        ret = -1;
        goto error;
    }

    if (proxy_context == GSS_C_NO_CONTEXT)
    {
        printf("Accepted unauthenticated connection.\n");
        auth_flag = 0;
    }
    else
    {
        auth_flag = 1;

        if (export)
        {
            for (i=0; i < 3; i++)
                if (test_import_export_context(&proxy_context))
                {
                    ret = -1;
                    goto error;
                }
        }
    }

    /* Begin a connection with the server */
    if (begin_server_connection(server_creds, client,
                                remote_host, remote_service_name, port, oid,
                                proxy_ret_flags,
                                auth_flag, &client_sock,
                                &client_context))
    {
        ret = -1;
        goto error;
    }

    if (client_context != GSS_C_NO_CONTEXT)
    {
        if (export)
        {
            for (i=0; i < 3; i++)
            {
                if (test_import_export_context(&client_context))
                {
                    ret = -1;
                    goto error;
                }
            }
        }
    }

    do
    {
        /* Receive the message token */
        if (recv_token(proxy_sock, &proxy_token_flags, &proxy_xmit_buf) < 0)
        {
            ret = -1;
            goto error;
        }

        if (proxy_token_flags & TOKEN_NOOP)
        {
            if (log_file)
                fprintf(log_file, "Client finished.\n");
            (void) send_token(client_sock, TOKEN_NOOP, empty_token);
            if (proxy_xmit_buf.value) {
                free(proxy_xmit_buf.value);
                proxy_xmit_buf.value = 0;
            }
            break;
        }
        else if (proxy_token_flags & TOKEN_SEND_IOV)
        {
            /* TODO - implemement support for iov buffers */
            fprintf(log_file,
                    "Proxy has no support for iov buffers.\n");
            ret = -1;
            goto error;
        }

        if (verbose && log_file)
        {
            fprintf(log_file, "Message token (flags=%d):\n", proxy_token_flags);
            print_token(&proxy_xmit_buf);
        }

        if ((proxy_context == GSS_C_NO_CONTEXT) &&
            (proxy_token_flags & (TOKEN_WRAPPED|TOKEN_ENCRYPTED|TOKEN_SEND_MIC)))
        {
            if (log_file)
            {
                fprintf(log_file,
                        "Unauthenticated client requested authenticated services!\n");
            }
            ret = -1;
            goto error;
        }

        if (proxy_token_flags & TOKEN_WRAPPED)
        {
            maj_stat = gss_unwrap(&min_stat, proxy_context, &proxy_xmit_buf, &msg_buf,
                                  &proxy_conf_state, (gss_qop_t *) NULL);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("unsealing message", maj_stat, min_stat);
                ret = -1;
                goto error;
            }
            else if (! proxy_conf_state && (proxy_token_flags & TOKEN_ENCRYPTED))
            {
                fprintf(stderr, "Warning!  Message not encrypted.\n");
            }

            if (proxy_xmit_buf.value)
            {
                free (proxy_xmit_buf.value);
                proxy_xmit_buf.value = 0;
            }
        }
        else
        {
            msg_buf = proxy_xmit_buf;
        }

        if (msg_buf.length > 3 && !memcmp(msg_buf.value, "stop", 4))
        {
            stop = 1;
        }

        if (log_file)
        {
            fprintf(log_file, "Received %s message: ",
                   (proxy_token_flags & TOKEN_ENCRYPTED) ? "encrypted" :
                   ((proxy_token_flags & TOKEN_WRAPPED) ? "wrapped" : "clear"));
            cp = msg_buf.value;
            if ((isprint((int) cp[0]) || isspace((int) cp[0])) &&
                (isprint((int) cp[1]) || isspace((int) cp[1])))
            {
                fprintf(log_file, "\"%.*s\"\n", (int) msg_buf.length,
                        (char *) msg_buf.value);
            }
            else
            {
                fprintf(log_file, "\n");
                print_token(&msg_buf);
            }
        }

        /* Forward message to server */
        if (send_recv_server(client_sock, proxy_token_flags, client_context,
                             msg_buf, proxy_conf_state))
        {
            ret = -1;
            goto error;
        }

        if (proxy_token_flags & TOKEN_SEND_MIC)
        {
            /* Produce a signature block for the message */
            maj_stat = gss_get_mic(&min_stat, proxy_context, GSS_C_QOP_DEFAULT,
                                   &msg_buf, &proxy_xmit_buf);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("signing message", maj_stat, min_stat);
                ret = -1;
                goto error;
            }

            if (msg_buf.value)
            {
                free (msg_buf.value);
                msg_buf.value = 0;
            }

            /* Send the signature block to the client */
            if (send_token(proxy_sock, TOKEN_MIC, &proxy_xmit_buf) < 0)
            {
                ret = -1;
                goto error;
            }

            printf("Signed reply sent to client.\n");

            if (proxy_xmit_buf.value)
            {
                free (proxy_xmit_buf.value);
                proxy_xmit_buf.value = 0;
            }
        }
        else {
            if (msg_buf.value)
            {
                free (msg_buf.value);
                msg_buf.value = 0;
            }
            if (send_token(proxy_sock, TOKEN_NOOP, empty_token) < 0)
            {
                ret = -1;
                goto error;
            }

            printf("Empty reply sent to client.\n");
        }
    } while (1 /* loop will break if NOOP received */);

error:
    if (client_sock != -1)
    {
        close(client_sock);
    }

    gss_delete_sec_context(&min_stat, &proxy_context, NULL);
    gss_delete_sec_context(&min_stat, &client_context, NULL);
    gss_release_name(&min_stat, &client);

    if (proxy_xmit_buf.value)
    {
        free (proxy_xmit_buf.value);
        proxy_xmit_buf.value = 0;
    }

    if (log_file)
        fflush(log_file);

    return ret;
}

int
main(
    int argc,
    char **argv
    )
{
    char *remote_service_name;
    char *remote_server_host;
    gss_cred_id_t server_creds;
    OM_uint32 min_stat;
    u_short port = 4444;
    int proxy_sock;
    int once = 0;
    int do_inetd = 0;
    int export = 0;
    char *mechanism = 0;
    char *clientmech = 0;
    char *domain = 0;
    gss_OID oid = GSS_C_NULL_OID;
    gss_OID clientoid = GSS_C_NULL_OID;

    log_file = stdout;
    display_file = stdout;

    argc--; argv++;
    while (argc)
    {
        if (strcmp(*argv, "-port") == 0)
        {
            argc--; argv++;
            if (!argc) usage();
            port = atoi(*argv);
        }
        else if (strcmp(*argv, "-mech") == 0)
        {
            argc--; argv++;
            if (!argc) usage();
            mechanism = *argv;
        }
        else if (strcmp(*argv, "-forceclientmech") == 0)
        {
            argc--; argv++;
            if (!argc) usage();
            clientmech = *argv;
        }
        else if (strcmp(*argv, "-domain") == 0)
        {
            argc--; argv++;
            if (!argc) usage();
            domain = *argv;
        }
        else if (strcmp(*argv, "-verbose") == 0)
        {
            verbose = 1;
        }
        else if (strcmp(*argv, "-once") == 0)
        {
            once = 1;
        }
        else if (strcmp(*argv, "-inetd") == 0)
        {
            do_inetd = 1;
        }
        else if (strcmp(*argv, "-export") == 0)
        {
            export = 1;
        }
        else if (strcmp(*argv, "-logfile") == 0)
        {
            argc--; argv++;
            if (!argc) usage();
            /* Gross hack, but it makes it unnecessary to add an
               extra argument to disable logging, and makes the code
               more efficient because it doesn't actually write data
               to /dev/null. */
            if (! strcmp(*argv, "/dev/null"))
            {
                log_file = display_file = NULL;
            }
            else
            {
                log_file = fopen(*argv, "a");
                display_file = log_file;
                if (!log_file)
                {
                    perror(*argv);
                    exit(1);
                }
            }
        }
        else
        {
            break;
        }
        argc--; argv++;
    }
    if (argc != 2)
        usage();

    if ((*argv)[0] == '-')
        usage();

    remote_server_host = *argv++;
    remote_service_name = *argv++;

    if (mechanism)
        parse_oid(mechanism, &oid);

    if (clientmech)
        parse_oid(clientmech, &clientoid);

    if (server_acquire_creds(&server_creds) < 0)
        return -1;

    if (do_inetd)
    {
        close(1);
        close(2);

        sign_proxy(0, server_creds, export, port, remote_server_host,
                   remote_service_name, domain, oid, clientoid);
        close(0);
    }
    else
    {
        int stmp;

        if ((stmp = create_socket(port)) >= 0)
        {
            do
            {
                /* Accept a TCP connection */
                if ((proxy_sock = accept(stmp, NULL, 0)) < 0)
                {
                    perror("accepting connection");
                    continue;
                }
                /* this return value is not checked, because there's
                   not really anything to do if it fails */
                sign_proxy(proxy_sock, server_creds, export, port,
                           remote_server_host, remote_service_name,
                           domain, oid, clientoid);
                close(proxy_sock);
            } while (!stop && !once);

            close(stmp);
        }
    }


    (void) gss_release_oid(&min_stat, &clientoid);
    (void) gss_release_cred(&min_stat, &server_creds);

    return 0;
}
