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
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <ctype.h>

#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_ext.h>
#include "gss-misc.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

static void usage(void)
{
    fprintf(stderr, "Usage: gssproxy-server [-port port] [-verbose] [-once]\n");
    fprintf(stderr, "       [-inetd] [-export] [-logfile file] service_name\n");
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
 *      service_name    (r) the ASCII service name
 *      server_creds    (w) the GSS-API service credentials
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * The service name is imported with gss_import_name, and service
 * credentials are acquired with gss_acquire_cred.  If either opertion
 * fails, an error message is displayed and -1 is returned; otherwise,
 * 0 is returned.
 */
static int server_acquire_creds(
    char *service_name,
    gss_cred_id_t *server_creds
    )
{
    int ret = 0;
    gss_buffer_desc name_buf = GSS_C_EMPTY_BUFFER;
    gss_name_t server_name = GSS_C_NO_NAME;
    OM_uint32 maj_stat = 0, min_stat = 0;

    name_buf.value = service_name;
    name_buf.length = strlen(name_buf.value) + 1;
    maj_stat = gss_import_name(&min_stat, &name_buf,
                               (gss_OID) gss_nt_service_name, &server_name);
    if (maj_stat != GSS_S_COMPLETE)
    {
        display_status("importing name", maj_stat, min_stat);
        ret = -1;
        goto error;
    }

    maj_stat = gss_acquire_cred(&min_stat, server_name, 0,
                                GSS_C_NULL_OID_SET, GSS_C_ACCEPT,
                                server_creds, NULL, NULL);
    if (maj_stat != GSS_S_COMPLETE)
    {
        display_status("acquiring credentials", maj_stat, min_stat);
        ret = -1;
        goto error;
    }

error:
    (void) gss_release_name(&min_stat, &server_name);

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
    gss_cred_id_t server_creds,
    gss_ctx_id_t *context,
    gss_buffer_t client_name,
    OM_uint32 *ret_flags
    )
{
    int ret = 0;
    gss_buffer_desc send_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc recv_tok = GSS_C_EMPTY_BUFFER;
    gss_name_t client = GSS_C_NO_NAME;
    gss_OID doid = GSS_C_NO_OID;
    OM_uint32 maj_stat = 0;
    OM_uint32 min_stat = 0;
    OM_uint32 acc_sec_min_stat = 0;
    gss_buffer_desc oid_name = GSS_C_EMPTY_BUFFER;
    unsigned int token_flags = 0;

    *context = GSS_C_NO_CONTEXT;

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
                                       server_creds,
                                       &recv_tok,
                                       GSS_C_NO_CHANNEL_BINDINGS,
                                       &client,
                                       &doid,
                                       &send_tok,
                                       ret_flags,
                                       NULL,  /* ignore time_rec */
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
                if (*context != GSS_C_NO_CONTEXT)
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

        /* display the flags */
        display_ctx_flags(*ret_flags);

        if (verbose && log_file)
        {
            maj_stat = gss_oid_to_str(&min_stat, doid, &oid_name);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("converting oid->string", maj_stat, min_stat);
                ret = -1;
                goto error;
            }
            fprintf(log_file, "Accepted connection using mechanism OID %.*s.\n",
                    (int) oid_name.length, (char *) oid_name.value);
        }

        maj_stat = gss_display_name(&min_stat, client, client_name, &doid);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("displaying name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }
    }
    else
    {
        client_name->length = *ret_flags = 0;

        if (log_file)
            fprintf(log_file, "Accepted unauthenticated connection.\n");
    }

error:
    if (ret)
    {
        gss_delete_sec_context(&min_stat, context,
                               GSS_C_NO_BUFFER);
    }
    (void) gss_release_buffer(&min_stat, &send_tok);
    (void) gss_release_name(&min_stat, &client);
    (void) gss_release_buffer(&min_stat, &oid_name);

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
        ret = 1;
        goto error;
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


static int process_non_iov_message(
    gss_ctx_id_t context,
    unsigned int token_flags,
    gss_buffer_desc xmit_buf,
    gss_buffer_t sign_buf
    )
{
    int ret = 0;
    OM_uint32 maj_stat = 0, min_stat = 0;
    gss_buffer_desc msg_buf = GSS_C_EMPTY_BUFFER;
    int conf_state = 0;
    char *cp = NULL;

    if (verbose && log_file)
    {
        fprintf(log_file, "Message token (flags=%d):\n", token_flags);
        print_token(&xmit_buf);
    }

    if (token_flags & TOKEN_WRAPPED)
    {
        maj_stat = gss_unwrap(&min_stat, context, &xmit_buf, &msg_buf,
                              &conf_state, (gss_qop_t *) NULL);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("unsealing message", maj_stat, min_stat);
            ret = -1;
            goto error;
        }
        else if (! conf_state && (token_flags & TOKEN_ENCRYPTED))
        {
            fprintf(stderr, "Warning!  Message not encrypted.\n");
        }
    }
    else
    {
        msg_buf = xmit_buf;
    }

    if (msg_buf.length > 3 && !memcmp(msg_buf.value, "stop", 4))
    {
        stop = 1;
    }

    if (log_file) {
        fprintf(log_file, "Received message: ");
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

    if (token_flags & TOKEN_SEND_MIC)
    {
        /* Produce a signature block for the message */
        maj_stat = gss_get_mic(&min_stat, context, GSS_C_QOP_DEFAULT,
                               &msg_buf, sign_buf);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("signing message", maj_stat, min_stat);
            return(-1);
        }

        if (msg_buf.value && (token_flags & TOKEN_WRAPPED))
        {
            free (msg_buf.value);
            msg_buf.value = 0;
        }
    }

error:

    return ret;
}

static int process_iov_message(
    gss_ctx_id_t context,
    unsigned int token_flags,
    gss_buffer_desc xmit_buf,
    gss_buffer_t sign_buf
    )
{
    int ret = 0;
    OM_uint32 maj_stat = 0, min_stat = 0;
    gss_buffer_desc msg_buf = GSS_C_EMPTY_BUFFER;
    // do not free
    gss_buffer_t sign_msg_buf = GSS_C_NO_BUFFER;
    int i = 0, j = 0;
    int conf_state = 0;
    char *cp = NULL;
    OM_uint32 wrap_buf_count = 0;
    unsigned char *wrap_buf_types = NULL;
    gss_iov_buffer_desc wrap_bufs[21];
    unsigned char lenbuf[4];

    if (verbose && log_file)
    {
        fprintf(log_file, "Message iov token (flags=%d):\n", token_flags);
        print_token(&xmit_buf);
    }

    i = 0;
    if (token_flags & TOKEN_WRAPPED)
    {
        memcpy(lenbuf, &((unsigned char *)xmit_buf.value)[i], sizeof(lenbuf));
        wrap_buf_count = ((lenbuf[0] << 24)
                         | (lenbuf[1] << 16)
                         | (lenbuf[2] << 8)
                         | lenbuf[3]);
        i += 4;
        fprintf(stderr, "There are %u buffers\n", wrap_buf_count);

        wrap_buf_types = &((unsigned char *)xmit_buf.value)[i];
        i+= wrap_buf_count;

        for (j = 0 ; j < wrap_buf_count ; j++)
        {
            memcpy(lenbuf, &((unsigned char *)xmit_buf.value)[i], sizeof(lenbuf));
            wrap_bufs[j].buffer.length = ((lenbuf[0] << 24)
                                         | (lenbuf[1] << 16)
                                         | (lenbuf[2] << 8)
                                         | lenbuf[3]);
            i += 4;

            wrap_bufs[j].buffer.value = &((unsigned char *)xmit_buf.value)[i];
            i += wrap_bufs[j].buffer.length;

            switch (wrap_buf_types[j])
            {
                case 1:
                    wrap_bufs[j].type = GSS_IOV_BUFFER_TYPE_HEADER;
                    break;
                case 2:
                    wrap_bufs[j].type = GSS_IOV_BUFFER_TYPE_DATA;
                    break;
                case 3:
                    wrap_bufs[j].type = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
                    break;
                case 4:
                    wrap_bufs[j].type = GSS_IOV_BUFFER_TYPE_PADDING;
                    break;
                default:
                    fprintf(stderr,"Unknown buffer type received %u\n", wrap_buf_types[j]);
                    return -1;
            }

            fprintf(stderr, "Buffer %u is type %u and is %u bytes\n", j, (unsigned int)wrap_buf_types[j], (unsigned int)(wrap_bufs[j].buffer.length));
        }

        maj_stat = gss_unwrap_iov(&min_stat, context,&conf_state,
                                  (gss_qop_t *) NULL, wrap_bufs,
                                  wrap_buf_count);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("unsealing message", maj_stat, min_stat);
            return(-1);
        }
        else if (! conf_state && (token_flags & TOKEN_ENCRYPTED))
        {
            fprintf(stderr, "Warning!  Message not encrypted.\n");
        }
    }
    else
    {
        msg_buf = xmit_buf;
    }


    for (i = 1 ; i < wrap_buf_count ; i++)
    {
        if ((wrap_bufs[i].type == GSS_IOV_BUFFER_TYPE_DATA) || (wrap_bufs[i].type == GSS_IOV_BUFFER_TYPE_SIGN_ONLY))
        {
            if (wrap_bufs[i].type == GSS_IOV_BUFFER_TYPE_DATA)
            {
                sign_msg_buf = &wrap_bufs[i].buffer;
            }
            msg_buf = wrap_bufs[i].buffer;
            fprintf(stderr, "Buffer type %u, %u\n", wrap_bufs[i].type, (unsigned int)msg_buf.length);

            if (log_file)
            {
                fprintf(log_file, "Received message: ");
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
        }
    }

    if (token_flags & TOKEN_SEND_MIC)
    {
        /* Produce a signature block for the message */
        maj_stat = gss_get_mic(&min_stat, context, GSS_C_QOP_DEFAULT,
                               sign_msg_buf, sign_buf);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("signing message", maj_stat, min_stat);
            ret = -1;
            goto error;
        }
    }

error:

    return ret;
}

/*
 * Function: sign_server
 *
 * Purpose: Performs the "sign" service.
 *
 * Arguments:
 *
 *      s               (r) a TCP socket on which a connection has been
 *                      accept()ed
 *      service_name    (r) the ASCII name of the GSS-API service to
 *                      establish a context as
 *      export          (r) whether to test context exporting
 *
 * Returns: -1 on error
 *
 * Effects:
 *
 * sign_server establishes a context, and performs a single sign request.
 *
 * A sign request is a single GSS-API sealed token.  The token is
 * unsealed and a signature block, produced with gss_sign, is returned
 * to the sender.  The context is the destroyed and the connection
 * closed.
 *
 * If any error occurs, -1 is returned.
 */
static int sign_server(
    int s,
    gss_cred_id_t server_creds,
    int export
    )
{
    int ret = 0;
    gss_buffer_desc client_name = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc xmit_buf = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc sign_buf = GSS_C_EMPTY_BUFFER;
    gss_ctx_id_t context = GSS_C_NO_CONTEXT;
    OM_uint32 min_stat = 0;
    int i = 0;
    OM_uint32 ret_flags = 0;
    OM_uint32 token_flags = 0;

    /* Establish a context with the client */
    if (server_establish_context(s, server_creds, &context,
                                 &client_name, &ret_flags) < 0)
    {
        ret = -1;
        goto error;
    }

    if (context == GSS_C_NO_CONTEXT)
    {
        printf("Accepted unauthenticated connection.\n");
    }
    else
    {
        printf("Accepted connection: \"%.*s\"\n",
               (int) client_name.length, (char *) client_name.value);

        if (export)
        {
            for (i=0; i < 3; i++)
            {
                if (test_import_export_context(&context))
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
        if (recv_token(s, &token_flags, &xmit_buf) < 0)
        {
            ret = -1;
            goto error;
        }

        if (token_flags & TOKEN_NOOP)
        {
            if (log_file)
                fprintf(log_file, "NOOP token\n");
            if(xmit_buf.value)
            {
                free(xmit_buf.value);
                xmit_buf.value = 0;
            }
            break;
        }

        if ((context == GSS_C_NO_CONTEXT) &&
            (    token_flags & (TOKEN_WRAPPED|TOKEN_ENCRYPTED|TOKEN_SEND_MIC)))
        {
            if (log_file)
                fprintf(log_file,
                        "Unauthenticated client requested authenticated services!\n");
            ret = -1;
            goto error;
        }

        if (token_flags & TOKEN_SEND_IOV)
        {
            ret = process_iov_message(
                      context,
                      token_flags,
                      xmit_buf,
                      &sign_buf);
            if (ret)
            {
                goto error;
            }
        }
        else
        {
            ret = process_non_iov_message(
                      context,
                      token_flags,
                      xmit_buf,
                      &sign_buf);
            if (ret)
            {
                goto error;
            }
        }

        if (token_flags & TOKEN_SEND_MIC)
        {
            /* Send the signature block to the client */
            if (send_token(s, TOKEN_MIC, &sign_buf) < 0)
            {
                ret = -1;
                goto error;
            }

            if (sign_buf.value)
            {
                free (sign_buf.value);
                sign_buf.value = 0;
            }
        }
        else
        {
            if (send_token(s, TOKEN_NOOP, empty_token) < 0)
            {
                ret = -1;
                goto error;
            }
        }

        gss_release_buffer(&min_stat, &sign_buf);

        if (xmit_buf.value)
        {
            free (xmit_buf.value);
            xmit_buf.value = 0;
        }
    } while (1 /* loop will break if NOOP received */);

error:
    if (xmit_buf.value)
    {
        free (xmit_buf.value);
        xmit_buf.value = 0;
    }

    gss_delete_sec_context(&min_stat, &context, NULL);
    (void) gss_release_buffer(&min_stat, &client_name);

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
    int ret = 0;
    char *service_name = NULL;
    gss_cred_id_t server_creds = GSS_C_NO_CREDENTIAL;
    OM_uint32 min_stat = 0;
    u_short port = 4444;
    int s = -1;
    int once = 0;
    int do_inetd = 0;
    int export = 0;

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
    if (argc != 1)
        usage();

    if ((*argv)[0] == '-')
        usage();

    service_name = *argv;

    if (server_acquire_creds(service_name, &server_creds) < 0)
    {
        ret = -1;
        goto error;
    }

    if (do_inetd)
    {
        close(1);
        close(2);

        sign_server(0, server_creds, export);
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
                if ((s = accept(stmp, NULL, 0)) < 0)
                {
                    perror("accepting connection");
                    continue;
                }
                /* this return value is not checked, because there's
                   not really anything to do if it fails */
                sign_server(s, server_creds, export);
                close(s);
            } while (!stop && !once);

            close(stmp);
        }
    }

error:
    (void) gss_release_cred(&min_stat, &server_creds);

    return ret;
}
