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
 *
 */

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


#include "gss-misc.h"

static int verbose = 0;

/*
 * -iov indicates use gss_wrap_iov.  The message sequence is a series of 
 *  "s" and "e" indicating whether buffers should be signed or encrypted
 *  and must correspond to the number of messages provided, maximum 10.
 */
void usage(void)
{
    fprintf(stderr, "Usage: gssclient [-port port] \n");
    fprintf(stderr, "       [-mech krb5|ntlm|spnego]\n");
    fprintf(stderr, "       [-cred user password domain]\n");
    fprintf(stderr, "       [-confidentiality] [-delegate] [-integrity] [-use_session_key]\n");
    fprintf(stderr, "       [-iov message_sequence]\n");
    fprintf(stderr, "       [-replay_detect] [-sequence_detect] [-mutual_auth]\n");
    fprintf(stderr, "       [-noreplay] [-nomutual]\n");
    fprintf(stderr, "       [-f] [-q] [-v] [-ccount count] [-mcount count]\n");
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
int connect_to_server(
    char *host,
    u_short port
    )
{
    struct sockaddr_in saddr;
    struct hostent *hp;
    int s;
    int err;
    WSADATA socket_data;
    USHORT version_required = 0x0101;


    err = WSAStartup(version_required, &socket_data);
    if (err)
    {
        fprintf(stderr,"Failed to initailize WSA: %d\n",err);
        return (-1);
    }


    if ((hp = gethostbyname(host)) == NULL)
    {
        fprintf(stderr, "Unknown host: %s\n", host);
        return -1;
    }

    saddr.sin_family = hp->h_addrtype;
    memcpy((char *)&saddr.sin_addr, hp->h_addr, sizeof(saddr.sin_addr));
    saddr.sin_port = htons(port);

    if ((s = (int)socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        DWORD dwError = WSAGetLastError();
        fprintf(stderr, "Error %u while creating socket\n", dwError);
        PrintWinError(dwError);
        return -1;
    }

    if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == SOCKET_ERROR)
    {
        DWORD dwError = WSAGetLastError();
        fprintf(stderr, "Error %u while connecting to server\n", dwError);
        PrintWinError(dwError);
        (void) closesocket(s);
        return -1;
    }
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
int client_establish_context(
    int s,
    char *service_name,
    OM_uint32 gss_flags,
    int auth_flag,
    char *mech,
    char *user,
    char *password,
    char *domain,
    CtxtHandle *gss_context,
    OM_uint32 *ret_flags
    )
{
    if ( auth_flag ) {
        SecBuffer send_tok, recv_tok;
        SecBufferDesc input_desc, output_desc;
        OM_uint32 maj_stat;
        CredHandle cred_handle;
        TimeStamp expiry;
        PCtxtHandle context_handle = NULL;
        unsigned int token_flags;
        SEC_WINNT_AUTH_IDENTITY AuthIdentity;
        PSEC_WINNT_AUTH_IDENTITY pAuthId = NULL;

        memset(&AuthIdentity, 0, sizeof(AuthIdentity));

        input_desc.cBuffers = 1;
        input_desc.pBuffers = &recv_tok;
        input_desc.ulVersion = SECBUFFER_VERSION;

        recv_tok.BufferType = SECBUFFER_TOKEN;
        recv_tok.cbBuffer = 0;
        recv_tok.pvBuffer = NULL;

        output_desc.cBuffers = 1;
        output_desc.pBuffers = &send_tok;
        output_desc.ulVersion = SECBUFFER_VERSION;

        send_tok.BufferType = SECBUFFER_TOKEN;
        send_tok.cbBuffer = 0;
        send_tok.pvBuffer = NULL;

        cred_handle.dwLower = 0;
        cred_handle.dwUpper = 0;

        if (user)
        {
            AuthIdentity.User = (PBYTE)user;
            AuthIdentity.UserLength = (DWORD)strlen(user);
            pAuthId = &AuthIdentity;
        }
        if (password)
        {
            AuthIdentity.Password = (PBYTE)password;
            AuthIdentity.PasswordLength = (DWORD)strlen(password);
            pAuthId = &AuthIdentity;
        }
        if (domain)
        {
            AuthIdentity.Domain = (PBYTE)domain;
            AuthIdentity.DomainLength = (DWORD)strlen(domain);
            pAuthId = &AuthIdentity;
        }
        AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

        maj_stat = AcquireCredentialsHandle(NULL,                       // no principal name
                                            mech,                       // package name
                                            SECPKG_CRED_OUTBOUND,
                                            NULL,                       // no logon id
                                            pAuthId,                    // no auth data
                                            NULL,                       // no get key fn
                                            NULL,                       // noget key arg
                                            &cred_handle,
                                            &expiry);
        if (maj_stat != SEC_E_OK)
        {
            display_status("acquiring credentials",maj_stat,0);
            return (-1);
        }

        if (send_token(s, TOKEN_NOOP|TOKEN_CONTEXT_NEXT, empty_token) < 0) {
            return -1;
        }

        /*
         * Perform the context-establishement loop.
         */

        gss_context->dwLower = 0;
        gss_context->dwUpper = 0;

        do {
            maj_stat = InitializeSecurityContext(&cred_handle,
                                                 context_handle,
                                                 service_name,
                                                 gss_flags,
                                                 0,          // reserved
                                                 SECURITY_NATIVE_DREP,
                                                 &input_desc,
                                                 0,          // reserved
                                                 gss_context,
                                                 &output_desc,
                                                 ret_flags,
                                                 &expiry);

            if (recv_tok.pvBuffer)
            {
                free(recv_tok.pvBuffer);
                recv_tok.pvBuffer = NULL;
                recv_tok.cbBuffer = 0;

            }

            context_handle = gss_context;

            if (maj_stat!=SEC_E_OK && maj_stat!=SEC_I_CONTINUE_NEEDED)
            {
                display_status("initializing context", maj_stat, 0);
                FreeCredentialsHandle(&cred_handle);
                return -1;
            }

            if (verbose > 1)
            {
                DumpIscRetFlags(*ret_flags);
            }

            if (send_tok.cbBuffer != 0)
            {
                if (verbose > 1)
                {
                    printf("Sending init_sec_context token (size=%d)...",
                           send_tok.cbBuffer);
                }
                if (send_token(s, TOKEN_CONTEXT, &send_tok) < 0)
                {
                    FreeContextBuffer(send_tok.pvBuffer);
                    FreeCredentialsHandle(&cred_handle);
                    return -1;
                }
            }

            FreeContextBuffer(send_tok.pvBuffer);
            send_tok.pvBuffer = NULL;
            send_tok.cbBuffer = 0;

            if (maj_stat == SEC_I_CONTINUE_NEEDED)
            {
                if (verbose > 1)
                {
                    printf("continue needed...");
                }
                if (recv_token(s, &token_flags, &recv_tok) < 0)
                {
                    FreeCredentialsHandle(&cred_handle);
                    return -1;
                }
            }
            if (verbose > 1)
            {
                printf("\n");
            }

        } while (maj_stat == SEC_I_CONTINUE_NEEDED);

        FreeCredentialsHandle(&cred_handle);
    } else {
        if (send_token(s, TOKEN_NOOP, empty_token) < 0)
            return -1;
    }
    return 0;
}

void read_file(
    char                *file_name,
    PSecBuffer          in_buf
    )
{
    int fd;
    ULONG bytes_in, count;
    UCHAR buf[100];

    //
    // readthrough once to get the size.
    //

    if ((fd = _open(file_name, O_RDONLY, 0)) < 0)
    {
        perror("open");
        fprintf(stderr, "Couldn't open file %s\n", file_name);
        exit(1);
    }

    for (bytes_in = 0; ; bytes_in += count)
    {
        count = _read(fd, buf, sizeof(buf));
        if (count < 0)
        {
            fprintf(stderr, "read - %x", GetLastError());
            exit(1);
        }
        if (count == 0)
            break;
    }
    _close(fd);

    if ((fd = _open(file_name, O_RDONLY, 0)) < 0)
    {
        perror("open");
        fprintf(stderr, "Couldn't open file %s\n", file_name);
        exit(1);
    }

    in_buf->cbBuffer = bytes_in;
    in_buf->pvBuffer = malloc(in_buf->cbBuffer);

    if (in_buf->pvBuffer == 0)
    {
        fprintf(stderr, "Couldn't allocate %d byte buffer for reading file\n",
                in_buf->cbBuffer);
        exit(1);
    }

    memset(in_buf->pvBuffer, 0, in_buf->cbBuffer);

    for (bytes_in = 0; bytes_in < in_buf->cbBuffer; bytes_in += count)
    {
        count = _read(fd, (PUCHAR) in_buf->pvBuffer+bytes_in, in_buf->cbBuffer-bytes_in);
        if (count < 0)
        {
            fprintf(stderr, "read - %x", GetLastError());
            exit(1);
        }
        if (count == 0)
            break;
    }

    if (bytes_in != count)
        fprintf(stderr, "Warning, only read in %d bytes, expected %d\n",
                bytes_in, count);
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
 * seals msg in a GSS-API token with gss_seal, sends it to the server,
 * reads back a GSS-API signature block for msg from the server, and
 * verifies it with gss_verify.  -1 is returned if any step fails,
 * otherwise 0 is returned.
 */
int call_server(
    char *host,
    u_short port,
    char *service_name,
    OM_uint32 gss_flags,
    int auth_flag,
    int wrap_flag,
    int encrypt_flag,
    int mic_flag,
    char *msg,
    int use_file,
    unsigned int mcount,
    char* mech,
    char *user,
    char *password,
    char *domain,
    char *iov_seq,
    char **iovbufs
    )
{
    CtxtHandle context;
    SecBuffer in_buf, out_buf;
    SecBuffer iov_in_buf[21];
    SecBuffer wrap_bufs[21];
    SecBufferDesc in_buf_desc;
    SecPkgContext_Sizes sizes;
    int s;
    unsigned int i, j;
    OM_uint32 ret_flags;
    OM_uint32 maj_stat;
    gss_qop_t qop_state;
    unsigned int token_flags;
    unsigned char wrap_buf_types[21];
    int need_padding = 0;

    /* Open connection */
    if ((s = connect_to_server(host, port)) < 0)
        return -1;

    /* Establish context */
    if (client_establish_context( s, service_name, gss_flags, auth_flag, 
                                  mech, user, password, domain,
                                  &context, &ret_flags) < 0)
    {
        (void) closesocket(s);
        return -1;
    }

    if (auth_flag)
    {
        printf("Authentication complete.\n");
        if (verbose)
        {
            DumpContext(&context, FALSE);
            DumpIscRetFlags(ret_flags);
        }
        else
        {
            DumpMiniContext(&context, FALSE);
        }
    }

    if ( auth_flag && (wrap_flag || mic_flag) ) {
        maj_stat = QueryContextAttributes( &context,
                                           SECPKG_ATTR_SIZES,
                                           &sizes
            );
        if (maj_stat != SEC_E_OK)
        {
            display_status("querying context attributes", maj_stat, 0);
            return (-1);
        }
    }

    if (iov_seq)
        goto do_iov;

    if (use_file)
    {
        read_file(msg, &in_buf);
    }
    else
    {
        /* Seal the message */
        in_buf.pvBuffer = msg;
        in_buf.cbBuffer = (ULONG)strlen(msg);
    }

    //
    // Prepare to encrypt the message
    //
    if (verbose > 1)
        printf("Block Size is %d, inbuffer = %d\n",
               sizes.cbBlockSize, in_buf.cbBuffer);


    for ( i = 0; i < mcount ; i++ ) {
        if ( auth_flag && wrap_flag ) {
            in_buf_desc.cBuffers = 3;
            in_buf_desc.pBuffers = wrap_bufs;
            in_buf_desc.ulVersion = SECBUFFER_VERSION;

            wrap_bufs[0].cbBuffer = sizes.cbSecurityTrailer;
            wrap_bufs[0].BufferType = SECBUFFER_TOKEN;
            wrap_bufs[0].pvBuffer = malloc(sizes.cbSecurityTrailer);

            if (wrap_bufs[0].pvBuffer == NULL)
            {
                fprintf(stderr,"Failed to allocate space for security trailer\n");
                if (use_file)
                    free(in_buf.pvBuffer);
                (void) closesocket(s);
                (void) DeleteSecurityContext(&context);
                return (-1);
            }

            wrap_bufs[1].BufferType = SECBUFFER_DATA;
            wrap_bufs[1].cbBuffer = in_buf.cbBuffer;
            wrap_bufs[1].pvBuffer = malloc(wrap_bufs[1].cbBuffer);

            if (wrap_bufs[1].pvBuffer == NULL)
            {
                free(wrap_bufs[0].pvBuffer);
                wrap_bufs[0].pvBuffer = NULL;
                fprintf(stderr,"Couldn't allocate space for wrap message\n");
                if (use_file)
                    free(in_buf.pvBuffer);
                (void) closesocket(s);
                (void) DeleteSecurityContext(&context);
                return (-1);
            }

            wrap_bufs[2].BufferType = SECBUFFER_PADDING;
            wrap_bufs[2].cbBuffer = sizes.cbBlockSize;
            wrap_bufs[2].pvBuffer = malloc(wrap_bufs[2].cbBuffer);

            if (wrap_bufs[2].pvBuffer == NULL)
            {
                free(wrap_bufs[0].pvBuffer);
                wrap_bufs[0].pvBuffer = NULL;
                free(wrap_bufs[1].pvBuffer);
                wrap_bufs[1].pvBuffer = NULL;
                fprintf(stderr,"Couldn't allocate space for wrap message\n");
                if (use_file)
                    free(in_buf.pvBuffer);
                (void) closesocket(s);
                (void) DeleteSecurityContext(&context);
                return (-1);
            }

            memcpy( wrap_bufs[1].pvBuffer,
                    in_buf.pvBuffer,
                    in_buf.cbBuffer
                );

            maj_stat = EncryptMessage( &context,
                                       encrypt_flag ? 0 : KERB_WRAP_NO_ENCRYPT,
                                       &in_buf_desc,
                                       0);

            if (maj_stat != SEC_E_OK)
            {
                display_status("sealing message", maj_stat, 0);

                free(wrap_bufs[0].pvBuffer);
                wrap_bufs[0].pvBuffer = NULL;
                free(wrap_bufs[1].pvBuffer);
                wrap_bufs[1].pvBuffer = NULL;
                free(wrap_bufs[2].pvBuffer);
                wrap_bufs[2].pvBuffer = NULL;
                if (use_file)
                    free(in_buf.pvBuffer);
                (void) closesocket(s);
                (void) DeleteSecurityContext(&context);
                return -1;
            }

            //
            // Create the message to send to server
            //

            out_buf.cbBuffer = wrap_bufs[0].cbBuffer + wrap_bufs[1].cbBuffer + wrap_bufs[2].cbBuffer;
            out_buf.pvBuffer = malloc(out_buf.cbBuffer);

            if (out_buf.pvBuffer == NULL)
            {
                free(wrap_bufs[0].pvBuffer);
                wrap_bufs[0].pvBuffer = NULL;
                free(wrap_bufs[1].pvBuffer);
                wrap_bufs[1].pvBuffer = NULL;
                free(wrap_bufs[2].pvBuffer);
                wrap_bufs[2].pvBuffer = NULL;
                fprintf(stderr,"Failed to allocate space for wrapped message\n");
                if (use_file)
                    free(in_buf.pvBuffer);
                (void) closesocket(s);
                (void) DeleteSecurityContext(&context);
                return (-1);
            }

            memcpy( out_buf.pvBuffer,
                    wrap_bufs[0].pvBuffer,
                    wrap_bufs[0].cbBuffer
                );
            memcpy( (PUCHAR) out_buf.pvBuffer + (int) wrap_bufs[0].cbBuffer,
                    wrap_bufs[1].pvBuffer,
                    wrap_bufs[1].cbBuffer
                );
            memcpy( (PUCHAR) out_buf.pvBuffer + wrap_bufs[0].cbBuffer + wrap_bufs[1].cbBuffer,
                    wrap_bufs[2].pvBuffer,
                    wrap_bufs[2].cbBuffer
                );
        } else {
            out_buf = in_buf;
        }

        /* Send to server */
        if (send_token(s, (TOKEN_DATA |
                             (wrap_flag ? TOKEN_WRAPPED : 0) |
                             (encrypt_flag ? TOKEN_ENCRYPTED : 0) |
                             (mic_flag ? TOKEN_SEND_MIC : 0)), &out_buf) < 0)
        {
            if ( auth_flag && wrap_flag ) {
                free(wrap_bufs[0].pvBuffer);
                wrap_bufs[0].pvBuffer = NULL;
                free(wrap_bufs[1].pvBuffer);
                wrap_bufs[1].pvBuffer = NULL;
                free(wrap_bufs[2].pvBuffer);
                wrap_bufs[2].pvBuffer = NULL;
                free(out_buf.pvBuffer);
            }
            out_buf.pvBuffer = NULL;
            out_buf.cbBuffer = 0;
            if (use_file)
                free(in_buf.pvBuffer);
            (void) closesocket(s);
            (void) DeleteSecurityContext(&context);
            return -1;
        }

        printf("Sent %s message to server.\n", encrypt_flag ? "encrypted" : (wrap_flag ? "wrapped" : "clear"));

        if ( auth_flag && wrap_flag ) {
            free(wrap_bufs[0].pvBuffer);
            wrap_bufs[0].pvBuffer = NULL;
            free(wrap_bufs[1].pvBuffer);
            wrap_bufs[1].pvBuffer = NULL;
            free(wrap_bufs[2].pvBuffer);
            wrap_bufs[2].pvBuffer = NULL;
            free(out_buf.pvBuffer);
        }
        out_buf.pvBuffer = NULL;
        out_buf.cbBuffer = 0;

        /* Read signature block into out_buf */
        if (recv_token(s, &token_flags, &out_buf) < 0)
        {
            if (use_file)
                free(in_buf.pvBuffer);
            (void) closesocket(s);
            (void) DeleteSecurityContext( &context);
            return -1;
        }

        if ( auth_flag && mic_flag ) {
            /* Verify signature block */
            in_buf_desc.cBuffers = 2;
            in_buf_desc.pBuffers = wrap_bufs;
            in_buf_desc.ulVersion = SECBUFFER_VERSION;
            wrap_bufs[0] = in_buf;
            wrap_bufs[0].BufferType = SECBUFFER_DATA;
            wrap_bufs[1] = out_buf;
            wrap_bufs[1].BufferType = SECBUFFER_TOKEN;

            maj_stat = VerifySignature(&context, &in_buf_desc, 0, &qop_state);
            if (maj_stat != SEC_E_OK)
            {
                display_status("verifying signature", maj_stat, 0);
                free(out_buf.pvBuffer);
                if (use_file)
                    free(in_buf.pvBuffer);
                (void) closesocket(s);
                (void) DeleteSecurityContext( &context);
                return -1;
            }

            printf("Signature verified.\n");
        } else {
            printf("Response received.\n");
        }
        free(out_buf.pvBuffer);
    }

    if (use_file)
        free(in_buf.pvBuffer);

    if (!(iov_seq && auth_flag && wrap_flag))
        goto skip_iov;

do_iov:
    //
    // Prepare to encrypt the message
    //
    if (verbose)
        printf("Block Size is %d, inbuffer = %d\n",
                sizes.cbBlockSize, in_buf.cbBuffer);

    if ( auth_flag && wrap_flag ) {
        in_buf_desc.pBuffers = wrap_bufs;
        in_buf_desc.ulVersion = SECBUFFER_VERSION;

        wrap_bufs[0].cbBuffer = sizes.cbSecurityTrailer;
        wrap_bufs[0].BufferType = SECBUFFER_TOKEN;
        wrap_bufs[0].pvBuffer = malloc(sizes.cbSecurityTrailer);
          fprintf(stderr, "Buffer %u, token, %u bytes\n", 0, wrap_bufs[0].cbBuffer);

        if (wrap_bufs[0].pvBuffer == NULL)
        {
            fprintf(stderr,"Failed to allocate space for security trailer\n");
            (void) closesocket(s);
            (void) DeleteSecurityContext(&context);
            return (-1);
        }

        for (i = 0, j = 1 ; iov_seq[i] ; i++)
        {
           fprintf(stderr, "iovbufs %u, %s, %u\n", i, iovbufs[i], strlen(iovbufs[i]));
             if (iov_seq[i] == 'e')
             {
                 need_padding = 1;

                 iov_in_buf[j].pvBuffer = iovbufs[i];
                 iov_in_buf[j].cbBuffer = (ULONG)strlen(iovbufs[i]) + 1;

                 in_buf.pvBuffer = iov_in_buf[j].pvBuffer;
                 in_buf.cbBuffer = iov_in_buf[j].cbBuffer;

                 wrap_bufs[j].BufferType = SECBUFFER_DATA;
                 wrap_bufs[j].cbBuffer = iov_in_buf[j].cbBuffer;
                 wrap_bufs[j].pvBuffer = malloc(wrap_bufs[j].cbBuffer);
          fprintf(stderr, "Buffer %u, encrypt, %u bytes, %s\n", j, wrap_bufs[j].cbBuffer, iov_in_buf[j].pvBuffer);

                 if (wrap_bufs[j].pvBuffer == NULL)
                 {
                     free(wrap_bufs[0].pvBuffer);
                     wrap_bufs[0].pvBuffer = NULL;
                     fprintf(stderr,"Couldn't allocate space for wrap message\n");
                     (void) closesocket(s);
                     (void) DeleteSecurityContext(&context);
                     return (-1);
                 }

                memcpy( wrap_bufs[j].pvBuffer,
                        iov_in_buf[j].pvBuffer,
                        iov_in_buf[j].cbBuffer
                    );

                 j++;
             }
             else
             {
                 iov_in_buf[j].pvBuffer = iovbufs[i];
                 iov_in_buf[j].cbBuffer = (ULONG)strlen(iovbufs[i]) + 1;

                 wrap_bufs[j].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
                 wrap_bufs[j].cbBuffer = iov_in_buf[j].cbBuffer;
                 wrap_bufs[j].pvBuffer = malloc(wrap_bufs[j].cbBuffer);
          fprintf(stderr, "Buffer %u, signed, %u bytes\n", j, wrap_bufs[j].cbBuffer);

                 if (wrap_bufs[j].pvBuffer == NULL)
                 {
                     free(wrap_bufs[0].pvBuffer);
                     wrap_bufs[0].pvBuffer = NULL;
                     fprintf(stderr,"Couldn't allocate space for wrap message\n");
                     (void) closesocket(s);
                     (void) DeleteSecurityContext(&context);
                     return (-1);
                 }

                 memcpy( wrap_bufs[j].pvBuffer,
                         iov_in_buf[j].pvBuffer,
                         iov_in_buf[j].cbBuffer
                     );

                 j++;
             }
        }

        if (need_padding)
        {
                 wrap_bufs[j].BufferType = SECBUFFER_PADDING;
                 wrap_bufs[j].cbBuffer = sizes.cbBlockSize;
                 wrap_bufs[j].pvBuffer = malloc(wrap_bufs[j].cbBuffer);
          fprintf(stderr, "Buffer %u, padding, %u bytes\n", j, wrap_bufs[j].cbBuffer);

                 if (wrap_bufs[j].pvBuffer == NULL)
                 {
                     free(wrap_bufs[0].pvBuffer);
                     wrap_bufs[0].pvBuffer = NULL;
                     fprintf(stderr,"Couldn't allocate space for wrap message\n");
                     (void) closesocket(s);
                     (void) DeleteSecurityContext(&context);
                     return (-1);
                 }

                 j++;
        }

        in_buf_desc.cBuffers = j;

        maj_stat = EncryptMessage( &context,
                                   encrypt_flag ? 0 : KERB_WRAP_NO_ENCRYPT,
                                   &in_buf_desc,
                                   0);

        if (maj_stat != SEC_E_OK)
        {
            display_status("sealing message", maj_stat, 0);

            free(wrap_bufs[0].pvBuffer);
            wrap_bufs[0].pvBuffer = NULL;
            (void) closesocket(s);
            (void) DeleteSecurityContext(&context);
            return -1;
        }

        //
        // Create the mesage to send to server
        //

        out_buf.cbBuffer = sizeof(out_buf.cbBuffer) + in_buf_desc.cBuffers;
        for (i = 0 ; i < in_buf_desc.cBuffers; i++)
        {
            out_buf.cbBuffer += (sizeof(wrap_bufs[i].cbBuffer) + wrap_bufs[i].cbBuffer);
            switch (wrap_bufs[i].BufferType)
            {
                case SECBUFFER_TOKEN:
                    wrap_buf_types[i] = 1;
                    break;
                case SECBUFFER_DATA:
                    wrap_buf_types[i] = 2;
                    break;
                case SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM:
                    wrap_buf_types[i] = 3;
                    break;
                case SECBUFFER_PADDING:
                    wrap_buf_types[i] = 4;
                    break;
                default:
                    wrap_buf_types[i] = 0;
                    fprintf(stderr, "Unknown buffer type\n");
                    (void) closesocket(s);
                    (void) DeleteSecurityContext(&context);
                    return (-1);
                    break;
            }
        }

        out_buf.pvBuffer = malloc(out_buf.cbBuffer);
        if (out_buf.pvBuffer == NULL)
        {
            free(wrap_bufs[0].pvBuffer);
            wrap_bufs[0].pvBuffer = NULL;
            fprintf(stderr,"Failed to allocate space for wrapped message\n");
            (void) closesocket(s);
            (void) DeleteSecurityContext(&context);
            return (-1);
        }

        i = 0;
        ((unsigned char *)out_buf.pvBuffer)[i++] = (unsigned char)((in_buf_desc.cBuffers >> 24) & 0xff);
        ((unsigned char *)out_buf.pvBuffer)[i++] = (unsigned char)((in_buf_desc.cBuffers >> 16) & 0xff);
        ((unsigned char *)out_buf.pvBuffer)[i++] = (unsigned char)((in_buf_desc.cBuffers >> 8) & 0xff);
        ((unsigned char *)out_buf.pvBuffer)[i++] = (unsigned char)(in_buf_desc.cBuffers & 0xff);

        fprintf(stderr, "There are %u buffers\n", in_buf_desc.cBuffers);

        memcpy(&(((unsigned char *)out_buf.pvBuffer)[i]),
               wrap_buf_types,
               in_buf_desc.cBuffers);
        i += in_buf_desc.cBuffers;

        for (j = 0 ; j < in_buf_desc.cBuffers ; j++)
        {
            ((unsigned char *)out_buf.pvBuffer)[i++] = (unsigned char)((wrap_bufs[j].cbBuffer >> 24) & 0xff);
            ((unsigned char *)out_buf.pvBuffer)[i++] = (unsigned char)((wrap_bufs[j].cbBuffer >> 16) & 0xff);
            ((unsigned char *)out_buf.pvBuffer)[i++] = (unsigned char)((wrap_bufs[j].cbBuffer >> 8) & 0xff);
            ((unsigned char *)out_buf.pvBuffer)[i++] = (unsigned char)(wrap_bufs[j].cbBuffer & 0xff);

            memcpy(&(((char *)out_buf.pvBuffer)[i]),
                   wrap_bufs[j].pvBuffer,
                   wrap_bufs[j].cbBuffer);
            i += wrap_bufs[j].cbBuffer;

            fprintf(stderr, "Buffer %u is type %u and is %u bytes\n", j, wrap_buf_types[j], wrap_bufs[j].cbBuffer);
        }

    } else {
        fprintf(stderr,"Didn't encrypt message buffers\n");
        (void) closesocket(s);
        (void) DeleteSecurityContext(&context);
        return -1;
    }

        /* Send to server */
        if (send_token(s, (TOKEN_DATA | TOKEN_SEND_IOV |
                               (wrap_flag ? TOKEN_WRAPPED : 0) |
                               (encrypt_flag ? TOKEN_ENCRYPTED : 0) |
                               (mic_flag ? TOKEN_SEND_MIC : 0)), &out_buf) < 0)
        {
            if ( auth_flag && wrap_flag ) {
                free(wrap_bufs[0].pvBuffer);
                wrap_bufs[0].pvBuffer = NULL;
                free(out_buf.pvBuffer);
            }
            out_buf.pvBuffer = NULL;
            out_buf.cbBuffer = 0;
            if (use_file)
                free(in_buf.pvBuffer);
            (void) closesocket(s);
            (void) DeleteSecurityContext(&context);
            return -1;
        }

        if ( auth_flag && wrap_flag ) {
            free(wrap_bufs[0].pvBuffer);
            wrap_bufs[0].pvBuffer = NULL;
            free(out_buf.pvBuffer);
        }
        out_buf.pvBuffer = NULL;
        out_buf.cbBuffer = 0;

        /* Read signature block into out_buf */
        if (recv_token(s, &token_flags, &out_buf) < 0)
        {
            if (use_file)
                free(in_buf.pvBuffer);
            (void) closesocket(s);
            (void) DeleteSecurityContext( &context);
            return -1;
        }

        if ( auth_flag && mic_flag ) {
            /* Verify signature block */
            in_buf_desc.cBuffers = 2;
            in_buf_desc.pBuffers = wrap_bufs;
            in_buf_desc.ulVersion = SECBUFFER_VERSION;
            wrap_bufs[0] = in_buf;
            wrap_bufs[0].BufferType = SECBUFFER_DATA;
            wrap_bufs[1] = out_buf;
            wrap_bufs[1].BufferType = SECBUFFER_TOKEN;

    fprintf(stderr, "Sig_buf %u, %s\n", wrap_bufs[0].cbBuffer, wrap_bufs[0].pvBuffer);

            maj_stat = VerifySignature(&context, &in_buf_desc, 0, &qop_state);
            if (maj_stat != SEC_E_OK)
            {
                display_status("verifying signature", maj_stat, 0);
                free(out_buf.pvBuffer);
                if (use_file)
                    free(in_buf.pvBuffer);
                (void) closesocket(s);
                (void) DeleteSecurityContext( &context);
                return -1;
            }

            if ( verbose )
                printf("Signature verified.\n");
        } else {
            if ( verbose )
                printf("Response received.\n");
        }
        free(out_buf.pvBuffer);

skip_iov:

    /* Send NOOP */
    (void) send_token(s, TOKEN_NOOP, empty_token);

    if ( auth_flag ) {
        /* Delete context */
        maj_stat = DeleteSecurityContext(&context);
        if (maj_stat != SEC_E_OK)
        {
            display_status("deleting context", maj_stat, 0);
            (void) closesocket(s);
            return -1;
        }
    }

    (void) closesocket(s);
    return 0;
}


int _cdecl main(
    int argc,
    char **argv
    )
{
    char *service_name, *server_host, *msg;
    u_short port = 4444;
    int use_file = 0;
    int mcount = 1, ccount = 1;
    int i;
    int auth_flag, wrap_flag, encrypt_flag, mic_flag;
    char *mech = "Kerberos";
    char *user = NULL;
    char *password = NULL;
    char *domain = NULL;
    char *iov_seq = NULL;
    char *iovbufs[10] = {NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL};

    OM_uint32 gss_flags =  ( ISC_REQ_MUTUAL_AUTH |
                             ISC_REQ_ALLOCATE_MEMORY |
                             ISC_REQ_CONFIDENTIALITY |
                             ISC_REQ_REPLAY_DETECT);
    FLAGMAPPING FlagMappings[] = {
#define DUPE( x ) { "-" #x, ISC_REQ_ ## x, "ISC_REQ_"#x }

        DUPE( CONFIDENTIALITY ),
        DUPE( DELEGATE ),
        DUPE( INTEGRITY ),
        DUPE( USE_SESSION_KEY ),
        DUPE( REPLAY_DETECT ),
        DUPE( SEQUENCE_DETECT ),
        DUPE( MUTUAL_AUTH )
    };

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
         port = (u_short)atoi(*argv);
      } else if (strcmp(*argv, "-noreplay") == 0) {
          gss_flags &= ~ISC_REQ_REPLAY_DETECT;
      } else if (strcmp(*argv, "-nomutual") == 0) {
          gss_flags &= ~ISC_REQ_MUTUAL_AUTH;
	  } else if (strcmp(*argv, "-f") == 0) {
	       use_file = 1;
        } else if (strcmp(*argv, "-v") == 0) {
            verbose++;
	  } else if (strcmp(*argv, "-q") == 0) {
	       verbose = 0;
	  } else if (strcmp(*argv, "-ccount") == 0) {
	    argc--; argv++;
	    if (!argc) usage();
	    ccount = atoi(*argv);
	    if (ccount <= 0) usage();
	  } else if (strcmp(*argv, "-mcount") == 0) {
	    argc--; argv++;
	    if (!argc) usage();
	    mcount = atoi(*argv);
	    if (mcount < 0) usage();
	  } else if (strcmp(*argv, "-na") == 0) {
	    auth_flag = wrap_flag = encrypt_flag = mic_flag = 0;
	  } else if (strcmp(*argv, "-nw") == 0) {
	    wrap_flag = 0;
	  } else if (strcmp(*argv, "-nx") == 0) {
	    encrypt_flag = 0;
	  } else if (strcmp(*argv, "-nm") == 0) {
	    mic_flag = 0;
	  } else if (strcmp(*argv, "-mech") == 0) {
	    argc--; argv++;
	    if (!argc) usage();
            if (strcmp(*argv, "krb5") == 0) {
                mech = "Kerberos";
            } else if (strcmp(*argv, "ntlm") == 0) {
                mech = NTLMSP_NAME_A;
            } else if (strcmp(*argv, "spnego") == 0) {
                mech = "Negotiate";
            } else {
                usage();
            }
	  } else if (strcmp(*argv, "-cred") == 0) {
	    argc--; argv++;
	    if (!argc) usage();
            user = *argv;
            if (user[0] == '-') usage();
	    argc--; argv++;
	    if (!argc) usage();
            password = *argv;
            if (password[0] == '-') usage();
	    argc--; argv++;
	    if (!argc) usage();
            domain = *argv;
            if (domain[0] == '-') usage();
	  } else if (strcmp(*argv, "-iov") == 0) {
	    argc--; argv++;
            iov_seq = *argv;
	  } else
      {

            /*
             * Search for flags to use along the command line.
             * We do this so that we can determine what, if any,
             * flags are busted.
             */

            BOOL found = FALSE;

            for ( i = 0 ;
                  i < ( sizeof( FlagMappings ) /
                        sizeof( FLAGMAPPING ) ) ;
                  i ++ )
            {

                if ( _strcmpi( *argv, FlagMappings[ i ].name ) == 0 )
                {
                    found = TRUE;
                    gss_flags |= FlagMappings[ i ].value ;
                    if (verbose > 2)
                    {
                        printf( "adding flag %hs (0x%x).\n",
                                FlagMappings[ i ].name +1,
                                FlagMappings[ i ].value );
                    }
                    break;

                }
            }

            if ( !found )
                break;
        }
        argc--; argv++;
    }
    if (iov_seq && (use_file || (mcount != 1) || !wrap_flag))
    {
        fprintf(stderr, "-iov doesn't support -mcount, -na, -nw, or -f\n");
        exit(1);
    }

    if ( argc < 3 ) usage();

    server_host = *argv++;
    service_name = *argv++;
    argc -=2;

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

    fprintf(stderr,
            "Calling host \"%s\"\n"
            "  with SPN \"%s\"\n"
            "  and message = \"%s\"\n",
            server_host,
            service_name,
            msg );

    if (verbose)
    {
        DumpIscReqFlags(gss_flags);
    }

    for (i = 0; i < ccount; i++) {
        if (call_server(server_host, port, service_name,
                         gss_flags, auth_flag, wrap_flag, encrypt_flag, mic_flag,
                         msg, use_file, mcount, mech, user, password,
                         domain, iov_seq, iovbufs) < 0)
            exit(1);
    }

    return 0;
}

