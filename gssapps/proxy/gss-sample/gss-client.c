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
 * Copyright (C) 2003, 2004 by the Massachusetts Institute of Technology.
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
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_ext.h>
#include "gss-misc.h"

#include <ntlm/sspintlm.h>
#include <ntlm/gssntlm.h>

static int verbose = 1;

static void usage(void)
{
    fprintf(stderr, "Usage: gssproxy-client [-port port] [-mech mechanism]\n");
    fprintf(stderr, "       [-d] [-iov message_sequence]\n");
    fprintf(stderr, "       [-ntlmcred user password domain]\n");
    fprintf(stderr, "       [-seq] [-noreplay] [-nomutual]\n");
    fprintf(stderr, "       [-f] [-q] [-ccount count] [-mcount count]\n");
    fprintf(stderr, "       [-na] [-nw] [-nx] [-nm] host service msg\n");
    exit(1);
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
    char *service_name,
    char *ntlm_user,
    char *ntlm_password,
    char *ntlm_domain,
    OM_uint32 gss_flags,
    int auth_flag,
    gss_OID oid,
    gss_ctx_id_t *gss_context,
    OM_uint32 *ret_flags
    )
{
    int ret = 0;
    gss_buffer_desc name_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc send_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc recv_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc *token_ptr = GSS_C_NO_BUFFER;
    gss_buffer_desc auth_data_tok = GSS_C_EMPTY_BUFFER;
    gss_name_t target_name = GSS_C_NO_NAME;
    gss_name_t user_name = GSS_C_NO_NAME;
    gss_cred_id_t cred = GSS_C_NO_CREDENTIAL;
    gss_OID_set desired_mechs = GSS_C_NO_OID_SET;
    OM_uint32 maj_stat = 0, min_stat = 0, init_sec_min_stat = 0;
    unsigned int token_flags = 0;
    gss_OID_desc GssCredOptionPasswordOidDesc = {
        .length = GSS_CRED_OPT_PW_LEN,
        .elements = GSS_CRED_OPT_PW
    };
    gss_OID GssCredOptionPasswordOid = &GssCredOptionPasswordOidDesc;
    SEC_WINNT_AUTH_IDENTITY auth_data;

    *gss_context = GSS_C_NO_CONTEXT;

    if (auth_flag)
    {
        /*
         * Import the name into target_name.  Use name_tok to save
         * local variable space.
         */
        name_tok.value = service_name;
        name_tok.length = strlen(service_name) ;
        maj_stat = gss_import_name(&min_stat, &name_tok,
                                   (gss_OID) gss_nt_service_name, &target_name);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("parsing name", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        if (ntlm_user && ntlm_password && ntlm_domain)
        {
            name_tok.value = ntlm_user;
            name_tok.length = strlen(ntlm_user) ;
            maj_stat = gss_import_name(&min_stat, &name_tok,
                                       GSS_C_NT_USER_NAME,
                                       &user_name);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("parsing name", maj_stat, min_stat);
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

            maj_stat = gss_acquire_cred(&min_stat,
                                        user_name,
                                        0,
                                        desired_mechs,
                                        GSS_C_INITIATE,
                                        &cred,
                                        NULL,
                                        NULL);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("acquiring credentials", maj_stat, min_stat);
                ret = -1;
                goto error;
            }

            auth_data.User = ntlm_user;
            auth_data.UserLength = strlen(ntlm_user);
            auth_data.Domain = ntlm_domain;
            auth_data.DomainLength = strlen(ntlm_domain);
            auth_data.Password = ntlm_password;
            auth_data.PasswordLength = strlen(ntlm_password);
            auth_data.Flags = 0;

            auth_data_tok.length = sizeof(auth_data);
            auth_data_tok.value = &auth_data;

            maj_stat = gssspi_set_cred_option(&min_stat,
                                              cred,
                                              GssCredOptionPasswordOid,
                                              &auth_data_tok);

            if (maj_stat != GSS_S_COMPLETE)
            {
                ret = -1;
                goto error;
            }
        }

        if (send_token(s, TOKEN_NOOP|TOKEN_CONTEXT_NEXT, empty_token) < 0)
        {
            ret = -1;
            goto error;
        }

        /*
         * Perform the context-establishment loop.
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
                                     cred,
                                     gss_context,
                                     target_name,
                                     oid,
                                     gss_flags,
                                     0,
                                     NULL, /* no channel bindings */
                                     token_ptr,
                                     NULL, /* ignore mech type */
                                     &send_tok,
                                     ret_flags,
                                     NULL); /* ignore time_rec */

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
                if (*gss_context != GSS_C_NO_CONTEXT)
                    gss_delete_sec_context(&min_stat, gss_context,
                                           GSS_C_NO_BUFFER);
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
    }
    else
    {
        if (send_token(s, TOKEN_NOOP, empty_token) < 0)
            ret = -1;
            goto error;
    }

error:

    if (ret)
    {
        gss_delete_sec_context(&min_stat, gss_context,
                               GSS_C_NO_BUFFER);
    }
    (void) gss_release_buffer(&min_stat, &send_tok);
    (void) gss_release_cred(&min_stat, &cred);
    (void) gss_release_oid_set(&min_stat, &desired_mechs);
    (void) gss_release_name(&min_stat, &user_name);
    (void) gss_release_name(&min_stat, &target_name);

    return ret;
}

static void read_file(
    char *file_name,
    gss_buffer_t in_buf
    )
{
    int fd, count;
    struct stat stat_buf;

    if ((fd = open(file_name, O_RDONLY, 0)) < 0)
    {
        perror("open");
        fprintf(stderr, "Couldn't open file %s\n", file_name);
        exit(1);
    }
    if (fstat(fd, &stat_buf) < 0)
    {
        perror("fstat");
        exit(1);
    }
    in_buf->length = stat_buf.st_size;

    if (in_buf->length == 0)
    {
        in_buf->value = NULL;
        return;
    }

    if ((in_buf->value = malloc(in_buf->length)) == 0)
    {
        fprintf(stderr, "Couldn't allocate %d byte buffer for reading file\n",
                (int) in_buf->length);
        exit(1);
    }

    /* this code used to check for incomplete reads, but you can't get
       an incomplete read on any file for which fstat() is meaningful */

    count = read(fd, in_buf->value, in_buf->length);
    if (count < 0)
    {
        perror("read");
        exit(1);
    }
    if (count < in_buf->length)
        fprintf(stderr, "Warning, only read in %d bytes, expected %d\n",
                count, (int) in_buf->length);
}

static int do_non_iov_message(
    gss_ctx_id_t context,
    int s,
    int use_file,
    int mcount,
    int wrap_flag,
    int encrypt_flag,
    int mic_flag,
    char *msg
    )
{
    int ret = 0;
    gss_buffer_desc in_buf = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc out_buf = GSS_C_EMPTY_BUFFER;
    unsigned int i = 0;
    OM_uint32 maj_stat = 0;
    OM_uint32 min_stat = 0;
    gss_qop_t qop_state = 0;
    int state = 0;
    unsigned int token_flags = 0;

    if (use_file)
    {
        read_file(msg, &in_buf);
    } else
    {
        /* Seal the message */
        in_buf.value = msg;
        in_buf.length = strlen(msg);
    }

    for (i = 0; i < mcount; i++)
    {
        if (wrap_flag)
        {
            maj_stat = gss_wrap(&min_stat, context, encrypt_flag, GSS_C_QOP_DEFAULT,
                                &in_buf, &state, &out_buf);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("wrapping message", maj_stat, min_stat);
                return -1;
            } else if (encrypt_flag && ! state)
            {
                fprintf(stderr, "Warning!  Message not encrypted.\n");
            }
        }
        else
        {
            out_buf = in_buf;
        }

        /* Send to server */
        if (send_token(s, (TOKEN_DATA |
                             (wrap_flag ? TOKEN_WRAPPED : 0) |
                             (encrypt_flag ? TOKEN_ENCRYPTED : 0) |
                             (mic_flag ? TOKEN_SEND_MIC : 0)), &out_buf) < 0)
        {
            ret = -1;
            goto error;
        }
        if (out_buf.value != in_buf.value)
            (void) gss_release_buffer(&min_stat, &out_buf);

        /* Read signature block into out_buf */
        if (recv_token(s, &token_flags, &out_buf) < 0)
        {
            ret = -1;
            goto error;
        }

        if (mic_flag)
        {
            /* Verify signature block */
            maj_stat = gss_verify_mic(&min_stat, context, &in_buf,
                                      &out_buf, &qop_state);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("verifying signature", maj_stat, min_stat);
                ret = -1;
                goto error;
            }

            if (verbose)
                printf("Signature verified.\n");
        }
        else
        {
            if (verbose)
                printf("Response received.\n");
        }

        free (out_buf.value);
    }

     if (use_file)
         free(in_buf.value);

error:

    return ret;
}

static int do_iov_message(
    gss_ctx_id_t context,
    int s,
    int wrap_flag,
    int encrypt_flag,
    int mic_flag,
    char *iov_seq,
    char **iovbufs
    )
{
    int ret = 0;
    gss_buffer_desc in_buf = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc out_buf = GSS_C_EMPTY_BUFFER;
    gss_iov_buffer_desc wrap_bufs[21];
    size_t wrap_buf_count = 0;
    unsigned char wrap_buf_types[21];
    unsigned int i = 0, j = 0;
    int need_padding = 0;
    OM_uint32 maj_stat = 0;
    OM_uint32 min_stat = 0;
    int state = 0;
    gss_qop_t qop_state = 0;
    unsigned int token_flags = 0;

    if (wrap_flag)
    {

        wrap_bufs[0].type = GSS_IOV_BUFFER_TYPE_HEADER; // |
                            // GSS_IOV_BUFFER_FLAG_ALLOCATE;
        wrap_bufs[0].buffer.length = 0;
        wrap_bufs[0].buffer.value = NULL;

        for (i = 0, j = 1 ; iov_seq[i] ; i++)
        {
            if (iov_seq[i] == 'e')
            {
                wrap_bufs[j].type = GSS_IOV_BUFFER_TYPE_DATA;
                wrap_bufs[j].buffer.length = strlen(iovbufs[i]) + 1;
                wrap_bufs[j].buffer.value = malloc(wrap_bufs[j].buffer.length);
                if (wrap_bufs[j].buffer.value == NULL)
                {
                    fprintf(stderr, "memory allocation failed\n");
                    ret = -1;
                    goto error;
                }

                memcpy(wrap_bufs[j].buffer.value, iovbufs[i], wrap_bufs[j].buffer.length);

                in_buf.value = iovbufs[i];
                in_buf.length = wrap_bufs[j].buffer.length;

                need_padding = 1;

                j++;
            }
            else
            {
                wrap_bufs[j].type = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
                wrap_bufs[j].buffer.length = strlen(iovbufs[i]) + 1;
                wrap_bufs[j].buffer.value = malloc(wrap_bufs[j].buffer.length);
                if (wrap_bufs[j].buffer.value == NULL)
                {
                    fprintf(stderr, "memory allocation failed\n");
                    ret = -1;
                    goto error;
                }

                memcpy(wrap_bufs[j].buffer.value, iovbufs[i], wrap_bufs[j].buffer.length);

                j++;
            }
        }

        if (need_padding)
        {
            wrap_bufs[j].type = GSS_IOV_BUFFER_TYPE_PADDING; // |
                                // GSS_IOV_BUFFER_FLAG_ALLOCATE;
            wrap_bufs[j].buffer.length = 0;
            wrap_bufs[j].buffer.value = NULL;

            j++;
        }
           
        wrap_buf_count = j;

        maj_stat = gss_wrap_iov_length(&min_stat, context, encrypt_flag, GSS_C_QOP_DEFAULT,
			     &state, wrap_bufs, wrap_buf_count);
        if (maj_stat != GSS_S_COMPLETE) {
            display_status("getting wrap iov buffer lengths", maj_stat, min_stat);
            ret = -1;
            goto error;
        }

        for (j = 0 ; j < wrap_buf_count ; j++)
        {
            if (wrap_bufs[j].buffer.length && (wrap_bufs[j].buffer.value == NULL))
            {
                fprintf( stderr, "allocating buffer %u, type %u, size %u\n", j, wrap_bufs[j].type, (unsigned int)(wrap_bufs[j].buffer.length));
                wrap_bufs[j].buffer.value = malloc(wrap_bufs[j].buffer.length);
                if (wrap_bufs[j].buffer.value == NULL)
                {
                    fprintf(stderr, "memory allocation failed\n");
                    ret = -1;
                    goto error;
                }
            }
        }

        maj_stat = gss_wrap_iov(&min_stat, context, encrypt_flag, GSS_C_QOP_DEFAULT,
			     &state, wrap_bufs, wrap_buf_count);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("wrapping iov message", maj_stat, min_stat);
            ret = -1;
            goto error;
        } else if (encrypt_flag && ! state)
        {
            fprintf(stderr, "Warning!  Message not encrypted.\n");
        }

        //
        // Create the message to send to server
        //

        out_buf.length = sizeof(out_buf.length) + wrap_buf_count;
        for (i = 0 ; i < wrap_buf_count ; i++)
        {
            out_buf.length += (sizeof(wrap_bufs[i].buffer.length) + wrap_bufs[i].buffer.length);

            switch (GSS_IOV_BUFFER_TYPE(wrap_bufs[i].type))
            {
                case GSS_IOV_BUFFER_TYPE_HEADER:
                    wrap_buf_types[i] = 1;
                    break;
                case GSS_IOV_BUFFER_TYPE_DATA:
                    wrap_buf_types[i] = 2;
                    break;
                case GSS_IOV_BUFFER_TYPE_SIGN_ONLY:
                    wrap_buf_types[i] = 3;
                    break;
                case GSS_IOV_BUFFER_TYPE_PADDING:
                    wrap_buf_types[i] = 4;
                    break;
                default:
                    wrap_buf_types[i] = 0;
                    fprintf(stderr, "Unknown buffer type\n");
                    return -1;
                    break;
            }
        }

        out_buf.value = malloc(out_buf.length);
        if (out_buf.value == NULL)
        {
            fprintf(stderr, "memory allocation failed\n");
            ret = -1;
            goto error;
        }

        i = 0;
        ((unsigned char *)out_buf.value)[i++] = (unsigned char)((wrap_buf_count >> 24) & 0xff);
        ((unsigned char *)out_buf.value)[i++] = (unsigned char)((wrap_buf_count >> 16) & 0xff);
        ((unsigned char *)out_buf.value)[i++] = (unsigned char)((wrap_buf_count >> 8) & 0xff);
        ((unsigned char *)out_buf.value)[i++] = (unsigned char)(wrap_buf_count & 0xff);

        fprintf(stderr, "There are %u buffers\n", (unsigned int)wrap_buf_count);

        memcpy(&(((unsigned char *)out_buf.value)[i]),
               wrap_buf_types,
               wrap_buf_count);
        i += wrap_buf_count;

        for (j = 0 ; j < wrap_buf_count ; j++)
        {
            ((unsigned char *)out_buf.value)[i++] = (unsigned char)((wrap_bufs[j].buffer.length >> 24) & 0xff);
            ((unsigned char *)out_buf.value)[i++] = (unsigned char)((wrap_bufs[j].buffer.length >> 16) & 0xff);
            ((unsigned char *)out_buf.value)[i++] = (unsigned char)((wrap_bufs[j].buffer.length >> 8) & 0xff);
            ((unsigned char *)out_buf.value)[i++] = (unsigned char)(wrap_bufs[j].buffer.length & 0xff);

            memcpy(&(((char *)out_buf.value)[i]),
                   wrap_bufs[j].buffer.value,
                   wrap_bufs[j].buffer.length);
            i += wrap_bufs[j].buffer.length;

            fprintf(stderr, "Buffer %u is type %u and is %u bytes\n", j, wrap_buf_types[j], (unsigned int)(wrap_bufs[j].buffer.length));

            free(wrap_bufs[j].buffer.value);
            wrap_bufs[j].buffer.value = NULL;
            wrap_bufs[j].buffer.length = 0;
        }

        /* Send to server */
        if (send_token(s, (TOKEN_DATA | TOKEN_SEND_IOV |
                           (wrap_flag ? TOKEN_WRAPPED : 0) |
                           (encrypt_flag ? TOKEN_ENCRYPTED : 0) |
                           (mic_flag ? TOKEN_SEND_MIC : 0)), &out_buf) < 0)
        {
            ret = -1;
            goto error;
        }
        if (out_buf.value != in_buf.value)
        {
            free(out_buf.value);
            out_buf.value = NULL;
        }

        /* Read signature block into out_buf */
        if (recv_token(s, &token_flags, &out_buf) < 0)
        {
            ret = -1;
            goto error;
        }

        if (mic_flag)
        {
            /* Verify signature block */
            maj_stat = gss_verify_mic(&min_stat, context, &in_buf,
                                      &out_buf, &qop_state);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("verifying signature", maj_stat, min_stat);
                ret = -1;
                goto error;
            }

            if (verbose)
                printf("Signature verified.\n");
        }
        else
        {
            if (verbose)
                printf("Response received.\n");
        }

        free (out_buf.value);
    }

error:
    return ret;
}

/*
 * Function: call_server
 *
 * Purpose: Call the "sign" service.
 *
 * Arguments:
 *
 *      host            (r) the host providing the service
 *      port            (r) the port to connect to on host
 *      service_name    (r) the GSS-API service name to authenticate to
 *      gss_flags       (r) GSS-API delegation flag (if any)
 *      auth_flag       (r) whether to do authentication
 *      wrap_flag       (r) whether to do message wrapping at all
 *      encrypt_flag    (r) whether to do encryption while wrapping
 *      mic_flag        (r) whether to request a MIC from the server
 *      msg             (r) the message to have "signed"
 *      use_file        (r) whether to treat msg as an input file name
 *      mcount          (r) the number of times to send the message
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * call_server opens a TCP connection to <host:port> and establishes a
 * GSS-API context with service_name over the connection.  It then
 * seals msg in a GSS-API token with gss_wrap, sends it to the server,
 * reads back a GSS-API signature block for msg from the server, and
 * verifies it with gss_verify.  -1 is returned if any step fails,
 * otherwise 0 is returned.  */
static int call_server(
    char *host,
    u_short port,
    gss_OID oid,
    char *service_name,
    char *ntlm_user,
    char *ntlm_password,
    char *ntlm_domain,
    OM_uint32 gss_flags,
    int auth_flag,
    int wrap_flag,
    int encrypt_flag,
    int mic_flag,
    char *msg,
    int use_file,
    int mcount,
    char *iov_seq,
    char **iovbufs
    )
{
    int ret = 0;
    gss_ctx_id_t context = GSS_C_NO_CONTEXT;
    int s = -1;
    OM_uint32 ret_flags = 0;
    OM_uint32 maj_stat = 0;
    OM_uint32 min_stat = 0;
    gss_name_t src_name = GSS_C_NO_NAME;
    gss_name_t targ_name = GSS_C_NO_NAME;
    gss_buffer_desc sname = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc tname = GSS_C_EMPTY_BUFFER;
    OM_uint32 lifetime = 0;
    gss_OID mechanism = GSS_C_NULL_OID;
    gss_OID name_type = GSS_C_NULL_OID;
    int is_local = 0;
    OM_uint32 context_flags = 0;
    int is_open = 0;
    gss_OID_set mech_names = GSS_C_NO_OID_SET;
    gss_buffer_desc oid_name = GSS_C_EMPTY_BUFFER;
    unsigned int i = 0;

    /* Open connection */
    if ((s = connect_to_server(host, port)) < 0)
    {
        ret = -1;
        goto error;
    }

    /* Establish context */
    if (client_establish_context(s, service_name, ntlm_user, ntlm_password,
                                 ntlm_domain, gss_flags, auth_flag,
                                 oid, &context,
                                 &ret_flags) < 0)
    {
        ret = -1;
        goto error;
    }

    if (auth_flag)
    {
        if (verbose)
        {
            /* display the flags */
            display_ctx_flags(ret_flags);

            /* Get context information */
            maj_stat = gss_inquire_context(&min_stat, context,
                                           &src_name, &targ_name, &lifetime,
                                           &mechanism, &context_flags,
                                           NULL,
                                           NULL);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("inquiring context", maj_stat, min_stat);
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
            printf("Context mechanism is %.*s.\n",
            (int) oid_name.length, (char *) oid_name.value);
            (void) gss_release_buffer(&min_stat, &oid_name);

            maj_stat = gss_inquire_context(&min_stat, context,
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           &is_local,
                                           &is_open);
            if (maj_stat != GSS_S_COMPLETE)
            {
                /* NTLM doesn't support all fields */
                display_status("inquiring context", maj_stat, min_stat);
                printf("Failed to retrieve whether is locally initiated and whether is open\n");
            }
         
            maj_stat = gss_display_name(&min_stat, src_name, &sname,
                                        &name_type);
            if (maj_stat != GSS_S_COMPLETE)
            {
                display_status("displaying source name", maj_stat, min_stat);
                ret = -1;
                goto error;
            }

            if (targ_name)
            {
                maj_stat = gss_display_name(&min_stat, targ_name, &tname,
                                            (gss_OID *) NULL);
                if (maj_stat != GSS_S_COMPLETE)
                {
                    display_status("displaying target name", maj_stat, min_stat);
                    ret = -1;
                    goto error;
                }
            }
            printf("\"%.*s\" to \"%.*s\", lifetime %d, flags %x, %s, %s\n",
                   (int) sname.length, (char *) sname.value,
                   (int) tname.length,
                   targ_name ? (char *) tname.value : "unknown target",
                   lifetime,
                   context_flags,
                   (is_local) ? "locally initiated" : "remotely initiated",
                   (is_open) ? "open" : "closed");

            (void) gss_release_name(&min_stat, &src_name);
            (void) gss_release_name(&min_stat, &targ_name);
            (void) gss_release_buffer(&min_stat, &sname);
            (void) gss_release_buffer(&min_stat, &tname);

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

            /* Now get the names supported by the mechanism */
            maj_stat = gss_inquire_names_for_mech(&min_stat,
                                                  mechanism,
                                                  &mech_names);
            if (maj_stat == GSS_S_COMPLETE)
            {
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

                printf("Mechanism %.*s supports %d names\n",
                       (int) oid_name.length, (char *) oid_name.value,
                       (int) mech_names->count);

                for (i=0; i<mech_names->count; i++)
                {
                    (void) gss_release_buffer(&min_stat, &oid_name);
                    maj_stat = gss_oid_to_str(&min_stat,
                                              &mech_names->elements[i],
                                              &oid_name);
                    if (maj_stat != GSS_S_COMPLETE)
                    {
                        display_status("converting oid->string", maj_stat, min_stat);
                        ret = -1;
                        goto error;
                    }
                    printf("  %d: %.*s\n", (int) i,
                           (int) oid_name.length, (char *) oid_name.value);

                    (void) gss_release_buffer(&min_stat, &oid_name);
                }
                (void) gss_release_oid_set(&min_stat, &mech_names);
            }
            else
            {
                /* NTLM doesn't support gss_inquire_names_for_mech */
                display_status("inquiring mech names", maj_stat, min_stat);
            }
            (void) gss_release_buffer(&min_stat, &oid_name);
        }
    }
    
    if (iov_seq)
    {
        ret = do_iov_message(
                  context,
                  s,
                  wrap_flag,
                  encrypt_flag,
                  mic_flag,
                  iov_seq,
                  iovbufs);
    }
    else
    {
        ret = do_non_iov_message(
                  context,
                  s,
                  use_file,
                  mcount,
                  wrap_flag,
                  encrypt_flag,
                  mic_flag,
                  msg);
    }
    if (ret)
    {
        goto error;
    }
 
    /* Send NOOP */
    (void) send_token(s, TOKEN_NOOP, empty_token);

    if (auth_flag)
    {
        /* Delete context */
        maj_stat = gss_delete_sec_context(&min_stat, &context, GSS_C_NO_BUFFER);
        if (maj_stat != GSS_S_COMPLETE)
        {
            display_status("deleting context", maj_stat, min_stat);
            ret = -1;
            goto error;
        }
    }

error:

    (void) gss_release_oid_set(&min_stat, &mech_names);
    (void) gss_delete_sec_context(&min_stat, &context, GSS_C_NO_BUFFER);
    (void) gss_release_buffer(&min_stat, &oid_name);

    if (s != -1)
    {
        (void) close(s);
    }

    return ret;
}

static void parse_oid(char *mechanism, gss_OID *oid)
{
    char *mechstr = 0, *cp;
    gss_buffer_desc tok;
    OM_uint32 maj_stat, min_stat;

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
        {
            if (*cp == '.')
                *cp = ' ';
        }
        tok.value = mechstr;
    } else
    {
        tok.value = mechanism;
    }
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

    return;
}

int main(
    int argc,
    char **argv
    )
{
    int ret = 0;
    char *service_name = NULL;
    char *server_host = NULL;
    char *msg = NULL;
    char *mechanism = 0;
    char *ntlm_user = 0;
    char *ntlm_password = 0;
    char *ntlm_domain = 0;
    char *iov_seq = 0;
    char *iovbufs[10] = {NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL};
    u_short port = 4444;
    int use_file = 0;
    OM_uint32 gss_flags = GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG |
        GSS_C_CONF_FLAG | GSS_C_INTEG_FLAG;
    OM_uint32 min_stat = 0;
    gss_OID oid = GSS_C_NULL_OID;
    int mcount = 1, ccount = 1;
    int i = 0;
    int auth_flag = 0;
    int wrap_flag = 0;
    int encrypt_flag = 0;
    int mic_flag = 0;

    display_file = stdout;
    auth_flag = wrap_flag = encrypt_flag = mic_flag = 1;

    /* Parse arguments. */
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
        else if (strcmp(*argv, "-iov") == 0)
        {
              argc--; argv++;
              if (!argc) usage();
              iov_seq = *argv;
              gss_flags |= GSS_C_DCE_STYLE;
        }
        else if (strcmp(*argv, "-ntlmcred") == 0)
        {
            argc--; argv++;
            if (!argc) usage();
            ntlm_user = *argv;
            argc--; argv++;
            if (!argc) usage();
            ntlm_password = *argv;
            argc--; argv++;
            if (!argc) usage();
            ntlm_domain = *argv;
        }
        else if (strcmp(*argv, "-d") == 0)
        {
            gss_flags |= GSS_C_DELEG_FLAG;
        }
        else if (strcmp(*argv, "-seq") == 0)
        {
            gss_flags |= GSS_C_SEQUENCE_FLAG;
        }
        else if (strcmp(*argv, "-noreplay") == 0)
        {
            gss_flags &= ~GSS_C_REPLAY_FLAG;
        }
        else if (strcmp(*argv, "-nomutual") == 0)
        {
            gss_flags &= ~GSS_C_MUTUAL_FLAG;
        }
        else if (strcmp(*argv, "-f") == 0)
        {
            use_file = 1;
        }
        else if (strcmp(*argv, "-q") == 0)
        {
            verbose = 0;
        }
        else if (strcmp(*argv, "-ccount") == 0)
        {
            argc--; argv++;
            if (!argc) usage();
            ccount = atoi(*argv);
            if (ccount <= 0) usage();
        }
        else if (strcmp(*argv, "-mcount") == 0)
        {
            argc--; argv++;
            if (!argc) usage();
            mcount = atoi(*argv);
            if (mcount < 0) usage();
        }
        else if (strcmp(*argv, "-na") == 0)
        {
            auth_flag = wrap_flag = encrypt_flag = mic_flag = 0;
        }
        else if (strcmp(*argv, "-nw") == 0)
        {
            wrap_flag = 0;
        }
        else if (strcmp(*argv, "-nx") == 0)
        {
            encrypt_flag = 0;
        }
        else if (strcmp(*argv, "-nm") == 0)
        {
            mic_flag = 0;
        }
        else
        {
            break;
        }
        argc--; argv++;
    }
    if (iov_seq && (use_file || (mcount != 1)|| !wrap_flag))
    {
        fprintf(stderr, "-iov doesn't support -mcount, -na, -nw, or -f\n");
        exit(1);
    }
    if (argc < 3)
        usage();

    server_host = *argv++;
    service_name = *argv++;
    argc -= 2;

    if (iov_seq)
    {
        for (i = 0 ;  argc && i < (sizeof(iovbufs)/sizeof(iovbufs[0])) && iov_seq[i] ; i++, argc--, argv++)
        {
            if (iov_seq[i] == 'e' || iov_seq[i] == 's')
            {
                iovbufs[i] = *argv;
                fprintf(stderr, "Input buffer %u, %s\n", i, iovbufs[i]);
            }
            else
            {
                usage();
            }
        }
        if (i == 0 || argc != 0 || iov_seq[i]) usage();

        /*
         * If there are two or more buffers, assume the second one is
         * message to be encrypted since that is the most common pattern
         */
        if (i > 1)
            msg = iovbufs[1];
        else
            msg = iovbufs[0];
    }
    else
    {
        msg = *argv++;
    }

    if (mechanism)
        parse_oid(mechanism, &oid);

    for (i = 0; i < ccount; i++)
    {
        if (call_server(server_host, port, oid, service_name,
                        ntlm_user, ntlm_password, ntlm_domain,
                        gss_flags, auth_flag, wrap_flag, encrypt_flag, mic_flag,
                        msg, use_file, mcount, iov_seq, iovbufs) < 0)
        {
            ret = 1;
            goto error;
        }
    }

error:
    if (oid != GSS_C_NULL_OID)
    {
        (void) gss_release_oid(&min_stat, &oid);
    }

    return ret;
}
