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
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

/* need struct timeval */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <gssapi/gssapi_generic.h>
#include "gss-misc.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
extern char *malloc();
#endif

FILE *display_file;

gss_buffer_desc empty_token_buf = { 0, (void *) "" };
gss_buffer_t empty_token = &empty_token_buf;

static void display_status_1(char *m, OM_uint32 code, int type);

static int write_all(int fildes, char *buf, unsigned int nbyte)
{
    int ret;
    char *ptr;

    for (ptr = buf; nbyte; ptr += ret, nbyte -= ret)
    {
        ret = send(fildes, ptr, nbyte, 0);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            return(ret);
        }
        else if (ret == 0)
        {
            return(ptr-buf);
        }
    }

    return(ptr-buf);
}

static int read_all(int fildes, char *buf, unsigned int nbyte)
{
    int ret = 0;
    char *ptr;
    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(fildes, &rfds);
    tv.tv_sec = 1200;
    tv.tv_usec = 0;

    for (ptr = buf; nbyte; ptr += ret, nbyte -= ret)
    {
        if (select(FD_SETSIZE, &rfds, NULL, NULL, &tv) <= 0
            || !FD_ISSET(fildes, &rfds))
            return(ptr-buf);
        ret = recv(fildes, ptr, nbyte, 0);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            return(ret);
        }
        else if (ret == 0)
        {
            return(ptr-buf);
        }
    }

    return(ptr-buf);
}

typedef struct _BCURSOR {
    void* buffer;
    size_t size;
    size_t used;
} BCURSOR, *PBCURSOR;

static void init_buffer(PBCURSOR pCursor, size_t size);
static void destroy_buffer(PBCURSOR pCursor);
static void add_buffer(PBCURSOR pCursor, void* data, unsigned int bytes);

static void init_buffer(PBCURSOR pCursor, size_t size)
{
    void* buffer = malloc(size);
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate %u bytes\n", (int) size);
        abort();
    }

    pCursor->buffer = buffer;
    pCursor->size = size;
    pCursor->used = 0;
}

static void destroy_buffer(PBCURSOR pCursor)
{
    if (pCursor->buffer)
    {
        free(pCursor->buffer);
    }

    pCursor->buffer = NULL;
    pCursor->size = 0;
    pCursor->used = 0;
}

static void add_buffer(PBCURSOR pCursor, void* data, unsigned int bytes)
{
    if (pCursor->used > pCursor->size)
    {
        fprintf(stderr, "Buffer full\n");
        abort();
    }
    if (bytes > (pCursor->used - pCursor->size))
    {
        fprintf(stderr, "Tried to add too many bytes to buffer\n");
        abort();
    }
    memcpy((char*)pCursor->buffer + pCursor->used, data, bytes);
    pCursor->used += bytes;
}

/*
 * Function: send_token
 *
 * Purpose: Writes a token to a file descriptor.
 *
 * Arguments:
 *
 *      s               (r) an open file descriptor
 *      flags           (r) the flags to write
 *      tok             (r) the token to write
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * If the flags are non-null, send_token writes the token flags (a
 * single byte, even though they're passed in in an integer). Next,
 * the token length (as a network long) and then the token data are
 * written to the file descriptor s.  It returns 0 on success, and -1
 * if an error occurs or if it could not write all the data.
 */
int send_token(
    int s,
    unsigned int flags,
    gss_buffer_t tok
    )
{
    int ret = 0;
    unsigned char flagbuf[4];
    unsigned char lenbuf[4];
#define USE_BUFFER
#ifdef USE_BUFFER
    BCURSOR cursor = { 0 };
    size_t size = 0;

    size = sizeof(flagbuf) + sizeof(lenbuf);
    if (tok->length <= 0xffffffffUL)
    {
        size += tok->length;
    }
    init_buffer(&cursor, size);
#endif

    flagbuf[0] = (flags >> 24) & 0xff;
    flagbuf[1] = (flags >> 16) & 0xff;
    flagbuf[2] = (flags >> 8) & 0xff;
    flagbuf[3] = flags & 0xff;

#ifdef USE_BUFFER
    add_buffer(&cursor, flagbuf, 4);
#else
    ret = write_all(s, flagbuf, 4);
    if (ret != 1)
    {
        perror("sending token flags");
        ret = -1;
        goto error;
    }
#endif

    if (tok->length > 0xffffffffUL)
        abort();
    lenbuf[0] = (tok->length >> 24) & 0xff;
    lenbuf[1] = (tok->length >> 16) & 0xff;
    lenbuf[2] = (tok->length >> 8) & 0xff;
    lenbuf[3] = tok->length & 0xff;

#ifdef USE_BUFFER
    add_buffer(&cursor, lenbuf, 4);
#else
    ret = write_all(s, lenbuf, 4);
    if (ret < 0)
    {
        perror("sending token length");
        return -1;
    }
    else if (ret != 4)
    {
        if (display_file)
            fprintf(display_file,
                    "sending token length: %d of %d bytes written\n",
                    ret, 4);
        ret = -1;
        goto error;
    }
#endif

#ifdef USE_BUFFER
    add_buffer(&cursor, tok->value, tok->length);
    ret = write_all(s, cursor.buffer, cursor.used);
    if (ret < 0)
    {
        perror("sending token");
        ret = -1;
        goto error;
    }
    else if (ret != cursor.used)
    {
        if (display_file)
            fprintf(display_file,
                    "sending token: %d of %d bytes written\n",
                    ret, (int)cursor.used);
        ret = -1;
        goto error;
    }
#else
    ret = write_all(s, tok->value, tok->length);
    if (ret < 0)
    {
        perror("sending token data");
        ret = -1;
        goto error;
    }
    else if (ret != tok->length)
    {
        if (display_file)
            fprintf(display_file,
                    "sending token data: %d of %d bytes written\n",
                    ret, (int) tok->length);
        ret = -1;
        goto error;
    }
#endif

error:

#ifdef USE_BUFFER
    destroy_buffer(&cursor);
#endif

    return ret;
}

/*
 * Function: recv_token
 *
 * Purpose: Reads a token from a file descriptor.
 *
 * Arguments:
 *
 *      s               (r) an open file descriptor
 *      flags           (w) the read flags
 *      tok             (w) the read token
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * recv_token reads the token flags (a single byte, even though
 * they're stored into an integer, then reads the token length (as a
 * network long), allocates memory to hold the data, and then reads
 * the token data from the file descriptor s.  It blocks to read the
 * length and data, if necessary.  On a successful return, the token
 * should be freed with gss_release_buffer.  It returns 0 on success,
 * and -1 if an error occurs or if it could not read all the data.
 */
int recv_token(
    int s,
    unsigned int *pFlags,
    gss_buffer_t tok
    )
{
    int ret = 0;
    unsigned int flags = 0;
    unsigned char flagbuf[4];
    unsigned char lenbuf[4];

    ret = read_all(s, (char*) flagbuf, 4);
    if (ret < 0)
    {
        perror("reading token flags");
        ret = -1;
        goto error;
    }
    else if (! ret)
    {
        if (display_file)
            fputs("reading token flags: 0 bytes read\n", display_file);
        ret = -1;
        goto error;
    }
    else
    {
        flags = ((flagbuf[0] << 24)
                 | (flagbuf[1] << 16)
                 | (flagbuf[2] << 8)
                 | flagbuf[3]);
        *pFlags = flags;
    }

    ret = read_all(s, (char*) lenbuf, 4);
    if (ret < 0)
    {
        perror("reading token length");
        ret = -1;
        goto error;
    }
    else if (ret != 4)
    {
        if (display_file)
            fprintf(display_file,
                    "reading token length: %d of %d bytes read\n",
                    ret, 4);
        ret = -1;
        goto error;
    }

    tok->length = ((lenbuf[0] << 24)
                   | (lenbuf[1] << 16)
                   | (lenbuf[2] << 8)
                   | lenbuf[3]);
    tok->value = (char *) malloc(tok->length ? tok->length : 1);
    if (tok->length && tok->value == NULL)
    {
        if (display_file)
            fprintf(display_file,
                    "Out of memory allocating token data\n");
        ret = -1;
        goto error;
    }

    ret = read_all(s, (char *) tok->value, tok->length);
    if (ret < 0)
    {
        perror("reading token data");
        free(tok->value);
        ret = -1;
        goto error;
    }
    else if (ret != tok->length)
    {
        fprintf(stderr, "sending token data: %d of %d bytes written\n",
                ret, (int) tok->length);
        free(tok->value);
        ret = -1;
        goto error;
    }

error:
    return ret;
}

static void display_status_1(
    char *m,
    OM_uint32 code,
    int type
    )
{
    OM_uint32 maj_stat = 0, min_stat = 0;
    gss_buffer_desc msg = GSS_C_EMPTY_BUFFER;
    OM_uint32 msg_ctx = 0;

    while (1) {
        maj_stat = gss_display_status(&min_stat, code,
                                      type, GSS_C_NULL_OID,
                                      &msg_ctx, &msg);
        if (maj_stat)
        {
              if (display_file)
                  fprintf(display_file, "GSS-API error %s: %u, %d, 0x%08x\n", m,
                          code, code, code); 
        }
        else
        {
            if (display_file)
                fprintf(display_file, "GSS-API error %s: %s (%u, %d, 0x%08x)\n", m,
                        (char *)msg.value, code, code, code);
            (void) gss_release_buffer(&min_stat, &msg);
        }

        if (!msg_ctx)
            break;
    }
}

/*
 * Function: display_status
 *
 * Purpose: displays GSS-API messages
 *
 * Arguments:
 *
 *      msg             a string to be displayed with the message
 *      maj_stat        the GSS-API major status code
 *      min_stat        the GSS-API minor status code
 *
 * Effects:
 *
 * The GSS-API messages associated with maj_stat and min_stat are
 * displayed on stderr, each preceeded by "GSS-API error <msg>: " and
 * followed by a newline.
 */
void display_status(
    char *msg,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    )
{
    display_status_1(msg, maj_stat, GSS_C_GSS_CODE);
    display_status_1(msg, min_stat, GSS_C_MECH_CODE);
}

/*
 * Function: display_ctx_flags
 *
 * Purpose: displays the flags returned by context initation in
 *          a human-readable form
 *
 * Arguments:
 *
 *      int             ret_flags
 *
 * Effects:
 *
 * Strings corresponding to the context flags are printed on
 * stdout, preceded by "context flag: " and followed by a newline
 */

#define DUMP_FLAG(Flags, Flag) \
    do \
    { \
        if ((Flags) & (Flag)) \
        { \
            fprintf(display_file, "    %-35s (0x%08X)\n", # Flag, Flag); \
            (Flags) &= ~(Flag); \
        } \
    } while (0)

#define DUMP_FLAGS_REMAINIG(Flags) \
    do \
    { \
        if (Flags) \
        { \
            fprintf(display_file, "    Unknown flags: 0x%08X\n", Flags); \
        } \
    } while (0)

void display_ctx_flags(
    OM_uint32 flags
    )
{
    OM_uint32 remaining = flags;

    fprintf(display_file, "Context Flags: 0x%08x\n", flags);
    DUMP_FLAG(remaining, GSS_C_DELEG_FLAG);
    DUMP_FLAG(remaining, GSS_C_MUTUAL_FLAG);
    DUMP_FLAG(remaining, GSS_C_REPLAY_FLAG);
    DUMP_FLAG(remaining, GSS_C_SEQUENCE_FLAG);
    DUMP_FLAG(remaining, GSS_C_CONF_FLAG);
    DUMP_FLAG(remaining, GSS_C_INTEG_FLAG);
    DUMP_FLAG(remaining, GSS_C_ANON_FLAG);
    DUMP_FLAG(remaining, GSS_C_PROT_READY_FLAG);
    DUMP_FLAG(remaining, GSS_C_TRANS_FLAG);
    DUMP_FLAG(remaining, GSS_C_DELEG_POLICY_FLAG);
    DUMP_FLAGS_REMAINIG(remaining);
}

void print_token(
    gss_buffer_t tok
    )
{
    int i;
    unsigned char *p = tok->value;

    if (!display_file)
        return;
    for (i=0; i < tok->length; i++, p++)
    {
        fprintf(display_file, "%02x ", *p);
        if ((i % 16) == 15)
        {
            fprintf(display_file, "\n");
        }
    }
    fprintf(display_file, "\n");
    fflush(display_file);
}

#ifdef _WIN32
#include <sys\timeb.h>
#include <time.h>

int gettimeofday (struct timeval *tv, void *ignore_tz)
{
    struct _timeb tb;
    _tzset();
    _ftime(&tb);
    if (tv)
    {
        tv->tv_sec = tb.time;
        tv->tv_usec = tb.millitm * 1000;
    }
    return 0;
}
#endif /* _WIN32 */

const char* oid_to_label(gss_buffer_t oid)
{
    const char* value = (const char*) oid->value;
    size_t length = oid->length;

    if (!strncmp(MECH_OID_STRING_NTLM, value, length))
    {
        return "NTLM";
    }
    else if (!strncmp(MECH_OID_STRING_SPNEGO, value, length))
    {
        return "SPNEGO";
    }
    else if (!strncmp(MECH_OID_STRING_KRB5, value, length))
    {
        return "KRB5";
    }
    else if (!strncmp(MECH_OID_STRING_KRB5_DRAFT, value, length))
    {
        return "KRB5_DRAFT";
    }
    else if (!strncmp(MECH_OID_STRING_KRB5_V2, value, length))
    {
        return "KRB5_V2";
    }
    else if (!strncmp(NT_OID_STRING_KRB5_PRINCIPAL_NAME, value, length))
    {
        return "KRB5_PRINCIPAL_NAME";
    }
    else
    {
        return NULL;
    }
}
