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

FILE *display_file;

SecBuffer  empty_token_buf = { 0, 0 };
PSecBuffer empty_token = &empty_token_buf;

void display_status_1
(char *m, ULONG code, int type);

int write_all(int fildes, unsigned char *buf, unsigned int nbyte)
{
    int ret;
    char *ptr;

    for (ptr = buf; nbyte; ptr += ret, nbyte -= ret)
    {
        ret = send(fildes, ptr, nbyte,0);
        if (ret < 0)
        {
            return (ret);
        }
        else if (ret == 0)
            break;
    }

    return (int)(ptr-buf);
}

int read_all(int fildes, char *buf, unsigned int nbyte)
{
    int ret;
    char *ptr;
    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET((DWORD)fildes, &rfds);
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    for (ptr = buf; nbyte; ptr += ret, nbyte -= ret)
    {
        if ( (select(FD_SETSIZE, &rfds, NULL, NULL, &tv) <= 0) || !FD_ISSET(fildes, &rfds) ) {
            perror("read_all select failed");
            return(ptr-buf);
        }
        ret = recv(fildes, ptr, nbyte,0);
        if (ret < 0)
        {
            return (ret);
        }
        else if (ret == 0) {
            break;
        }
    }

    return (int)(ptr-buf);
}

typedef struct _BCURSOR {
    void* buffer;
    size_t size;
    size_t used;
} BCURSOR, *PBCURSOR;

void init_buffer(PBCURSOR pCursor, size_t size)
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

void destroy_buffer(PBCURSOR pCursor)
{
    if (pCursor->buffer)
    {
        free(pCursor->buffer);
    }

    pCursor->buffer = NULL;
    pCursor->size = 0;
    pCursor->used = 0;
}

void add_buffer(PBCURSOR pCursor, void* data, unsigned int bytes)
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
    PSecBuffer tok
    )
{
    int ret;
    unsigned char flagbuf[4];
    unsigned char lenbuf[4];
#define USE_BUFFER
#ifdef USE_BUFFER
    BCURSOR cursor = { 0 };
    size_t size = 0;

    if (flags) {
        size += 4;
    }
    size += 4;
    if (tok->cbBuffer <= 0xffffffffUL) {
        size += tok->cbBuffer;
    }
    init_buffer(&cursor, size);
#endif

    if (flags) {
        flagbuf[0] = (unsigned char)((flags >> 24) & 0xff);
        flagbuf[1] = (unsigned char)((flags >> 16) & 0xff);
        flagbuf[2] = (unsigned char)((flags >> 8) & 0xff);
        flagbuf[3] = (unsigned char)(flags & 0xff);

#ifdef USE_BUFFER
        add_buffer(&cursor, flagbuf, 4);
#else
        ret = write_all(s, flagbuf, 4);
        if (ret != 1) {
            perror("sending token flags");
            return -1;
        }
#endif
    }
    if (tok->cbBuffer > 0xffffffffUL)
        abort();
    lenbuf[0] = (unsigned char)((tok->cbBuffer >> 24) & 0xff);
    lenbuf[1] = (unsigned char)((tok->cbBuffer >> 16) & 0xff);
    lenbuf[2] = (unsigned char)((tok->cbBuffer >> 8) & 0xff);
    lenbuf[3] = (unsigned char)(tok->cbBuffer & 0xff);

#ifdef USE_BUFFER
    add_buffer(&cursor, lenbuf, 4);
#else
    ret = write_all(s, lenbuf, 4);
    if (ret < 0) {
        perror("sending token length");
        return -1;
    } else if (ret != 4) {
        if (display_file)
            fprintf(display_file,
                    "sending token length: %d of %d bytes written\n",
                    ret, 4);
        return -1;
    }
#endif

#ifdef USE_BUFFER
    add_buffer(&cursor, tok->pvBuffer, tok->cbBuffer);
    ret = write_all(s, cursor.buffer, cursor.used);
    if (ret < 0)
    {
        perror("sending token");
        return -1;
    }
    else if ((ULONG)ret != (ULONG)cursor.used)
    {
        if (display_file)
            fprintf(display_file,
                    "sending token: %d of %d bytes written\n",
                    ret, cursor.used);
        return -1;
    }
#else
    ret = write_all(s, tok->pvBuffer, tok->cbBuffer);
    if (ret < 0)
    {
        perror("sending token data");
        return -1;
    }
    else if ((ULONG)ret != tok->cbBuffer)
    {
        if (display_file)
            fprintf(display_file,
                    "sending token data: %d of %d bytes written\n",
                    ret, tok->cbBuffer);
        return -1;
    }
#endif

#ifdef USE_BUFFER
    destroy_buffer(&cursor);
#endif

    return 0;
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
    PSecBuffer tok
    )
{
    int ret;
    unsigned int flags = 0;
    unsigned char flagbuf[4];
    unsigned char lenbuf[4];

    ret = read_all(s, flagbuf, 4);
    if (ret < 0) {
        perror("reading token flags");
        return -1;
    } else if (! ret) {
        if (display_file)
            fputs("reading token flags: 0 bytes read\n", display_file);
        return -1;
    } else {
        flags = ((flagbuf[0] << 24)
                 | (flagbuf[1] << 16)
                 | (flagbuf[2] << 8)
                 | flagbuf[3]);
        *pFlags = flags;
    }

    ret = read_all(s, lenbuf, 4);
    if (ret < 0) {
        perror("reading token length");
        return -1;
    } else if (ret != 4) {
        if (display_file)
            fprintf(display_file,
                    "reading token length: %d of %d bytes read\n",
                    ret, 4);
        return -1;
    }

    tok->cbBuffer = ((lenbuf[0] << 24)
                     | (lenbuf[1] << 16)
                     | (lenbuf[2] << 8)
                     | lenbuf[3]);
    tok->pvBuffer = (char *) malloc(tok->cbBuffer ? tok->cbBuffer : 1);
    if (tok->cbBuffer && tok->pvBuffer == NULL) {
        if (display_file)
            fprintf(display_file,
                    "Out of memory allocating token data\n");
        return -1;
    }

    ret = read_all(s, (char *) tok->pvBuffer, tok->cbBuffer);
    if (ret < 0)
    {
        perror("reading token data");
        free(tok->pvBuffer);
        return -1;
    }
    else if ((ULONG)ret != tok->cbBuffer)
    {
        fprintf(display_file, "sending token data: %d of %d bytes written\n",
                ret, tok->cbBuffer);
        free(tok->pvBuffer);
        return -1;
    }

    return 0;
}

void display_status_1(
    char *m,
    ULONG code,
    int type
    )
{

    if (display_file)
    {
        if (code & 0x80000000)
        {
            fprintf(display_file, "Error 0x%08x while %s\n",
                    code, m);
        }
        else
        {
            fprintf(display_file, "Error %u while %s\n",
                    code, m);
        }
        PrintWinError(code);

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
 * displayed on display_file, each preceeded by "GSS-API error <msg>: " and
 * followed by a newline.
 */
void display_status(
    char *msg,
    ULONG maj_stat,
    ULONG min_stat
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

void display_ctx_flags(
    ULONG flags
    )
{
    if (flags & GSS_C_DELEG_FLAG)
        fprintf(display_file, "context flag: GSS_C_DELEG_FLAG\n");
    if (flags & GSS_C_MUTUAL_FLAG)
        fprintf(display_file, "context flag: GSS_C_MUTUAL_FLAG\n");
    if (flags & GSS_C_REPLAY_FLAG)
        fprintf(display_file, "context flag: GSS_C_REPLAY_FLAG\n");
    if (flags & GSS_C_SEQUENCE_FLAG)
        fprintf(display_file, "context flag: GSS_C_SEQUENCE_FLAG\n");
    if (flags & GSS_C_CONF_FLAG )
        fprintf(display_file, "context flag: GSS_C_CONF_FLAG \n");
    if (flags & GSS_C_INTEG_FLAG )
        fprintf(display_file, "context flag: GSS_C_INTEG_FLAG \n");
}

void print_token(
    PSecBuffer tok
    )
{
    ULONG i;
    unsigned char *p = (PUCHAR) tok->pvBuffer;

    if (!display_file)
        return;
    for (i=0; i < tok->cbBuffer; i++, p++)
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

void
PrintWinError(
    DWORD wStatus
    )
{
    if (wStatus)
    {
        PSTR message = NULL;
        if (!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            wStatus,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (PSTR)&message,
                            0,
                            NULL))
        {
            printf("Error %u while formatting error message\n", GetLastError());
        }
        else
        {
            printf("Error Message: %s\n", message);
            LocalFree(message);
        }
    }
}

#define __DUMP_FLAG(Prefix, Flags, Flag, FlagName) \
    do { \
        if ((Flags) & (Flag)) \
        { \
            printf("%s%-35s (0x%08X)\n", Prefix, FlagName, Flag); \
            (Flags) &= ~(Flag); \
        } \
    } while (0)

#define _DUMP_FLAG(Prefix, Flags, Flag) \
    do { \
        if ((Flags) & (Flag)) \
        { \
            printf("%s%-35s (0x%08X)\n", Prefix, # Flag, Flag); \
            (Flags) &= ~(Flag); \
        } \
    } while (0)

#define _DUMP_FLAGS_REMAINIG(Prefix, Flags) \
    do { \
        if (Flags) \
        { \
            printf("%sUnknown flags: 0x%08X\n", Prefix, Flags); \
        } \
    } while (0)
    

#define DUMP_FLAG(Flags, Flag) __DUMP_FLAG("    ", Flags, Flag, # Flag)
#define DUMP_FLAGS_REMAINING(Flags) _DUMP_FLAGS_REMAINIG("    ", Flags)

VOID
DumpIscReqFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;

    printf("ISC Req Flags = 0x%08x\n", dwFlags);
    DUMP_FLAG(dwRemainder, ISC_REQ_DELEGATE);
    DUMP_FLAG(dwRemainder, ISC_REQ_MUTUAL_AUTH);
    DUMP_FLAG(dwRemainder, ISC_REQ_REPLAY_DETECT);
    DUMP_FLAG(dwRemainder, ISC_REQ_SEQUENCE_DETECT);
    DUMP_FLAG(dwRemainder, ISC_REQ_CONFIDENTIALITY);
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_SESSION_KEY);
    DUMP_FLAG(dwRemainder, ISC_REQ_PROMPT_FOR_CREDS);
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_SUPPLIED_CREDS);
    DUMP_FLAG(dwRemainder, ISC_REQ_ALLOCATE_MEMORY);
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_DCE_STYLE);
    DUMP_FLAG(dwRemainder, ISC_REQ_DATAGRAM);
    DUMP_FLAG(dwRemainder, ISC_REQ_CONNECTION);
    DUMP_FLAG(dwRemainder, ISC_REQ_CALL_LEVEL);
    DUMP_FLAG(dwRemainder, ISC_REQ_FRAGMENT_SUPPLIED);
    DUMP_FLAG(dwRemainder, ISC_REQ_EXTENDED_ERROR);
    DUMP_FLAG(dwRemainder, ISC_REQ_STREAM);
    DUMP_FLAG(dwRemainder, ISC_REQ_INTEGRITY);
    DUMP_FLAG(dwRemainder, ISC_REQ_IDENTIFY);
    DUMP_FLAG(dwRemainder, ISC_REQ_NULL_SESSION);
    DUMP_FLAG(dwRemainder, ISC_REQ_MANUAL_CRED_VALIDATION);
    DUMP_FLAG(dwRemainder, ISC_REQ_RESERVED1);
    DUMP_FLAG(dwRemainder, ISC_REQ_FRAGMENT_TO_FIT);
    DUMP_FLAG(dwRemainder, ISC_REQ_FORWARD_CREDENTIALS);
    DUMP_FLAG(dwRemainder, ISC_REQ_NO_INTEGRITY);
#ifdef ISC_REQ_USE_HTTP_STYLE
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_HTTP_STYLE);
#endif
    DUMP_FLAGS_REMAINING(dwRemainder);
}

VOID
DumpIscRetFlagsEx(
    DWORD dwFlags,
    BOOL bShowHeader
    )
{
    DWORD dwRemainder = dwFlags;

    if (bShowHeader)
    {
        printf("ISC Ret Flags = 0x%08x\n", dwFlags);
    }
    DUMP_FLAG(dwRemainder, ISC_RET_DELEGATE);
    DUMP_FLAG(dwRemainder, ISC_RET_MUTUAL_AUTH);
    DUMP_FLAG(dwRemainder, ISC_RET_REPLAY_DETECT);
    DUMP_FLAG(dwRemainder, ISC_RET_SEQUENCE_DETECT);
    DUMP_FLAG(dwRemainder, ISC_RET_CONFIDENTIALITY);
    DUMP_FLAG(dwRemainder, ISC_RET_USE_SESSION_KEY);
    DUMP_FLAG(dwRemainder, ISC_RET_USED_COLLECTED_CREDS);
    DUMP_FLAG(dwRemainder, ISC_RET_USED_SUPPLIED_CREDS);
    DUMP_FLAG(dwRemainder, ISC_RET_ALLOCATED_MEMORY);
    DUMP_FLAG(dwRemainder, ISC_RET_USED_DCE_STYLE);
    DUMP_FLAG(dwRemainder, ISC_RET_DATAGRAM);
    DUMP_FLAG(dwRemainder, ISC_RET_CONNECTION);
    DUMP_FLAG(dwRemainder, ISC_RET_INTERMEDIATE_RETURN);
    DUMP_FLAG(dwRemainder, ISC_RET_CALL_LEVEL);
    DUMP_FLAG(dwRemainder, ISC_RET_EXTENDED_ERROR);
    DUMP_FLAG(dwRemainder, ISC_RET_STREAM);
    DUMP_FLAG(dwRemainder, ISC_RET_INTEGRITY);
    DUMP_FLAG(dwRemainder, ISC_RET_IDENTIFY);
    DUMP_FLAG(dwRemainder, ISC_RET_NULL_SESSION);
    DUMP_FLAG(dwRemainder, ISC_RET_MANUAL_CRED_VALIDATION);
    DUMP_FLAG(dwRemainder, ISC_RET_RESERVED1);
    DUMP_FLAG(dwRemainder, ISC_RET_FRAGMENT_ONLY);
    DUMP_FLAG(dwRemainder, ISC_RET_FORWARD_CREDENTIALS);
#ifdef ISC_RET_USED_HTTP_STYLE
    DUMP_FLAG(dwRemainder, ISC_RET_USED_HTTP_STYLE);
#endif
    DUMP_FLAG(dwRemainder, ISC_RET_NO_ADDITIONAL_TOKEN);
#ifdef ISC_RET_REAUTHENTICATION
    DUMP_FLAG(dwRemainder, ISC_RET_REAUTHENTICATION);
#endif
    DUMP_FLAGS_REMAINING(dwRemainder);
}

VOID
DumpIscRetFlags(
    DWORD dwFlags
    )
{
    DumpIscRetFlagsEx(dwFlags, TRUE);
}

VOID
DumpAscReqFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;

    printf("ASC Req Flags = 0x%08x\n", dwFlags);
    DUMP_FLAG(dwRemainder, ASC_REQ_DELEGATE);
    DUMP_FLAG(dwRemainder, ASC_REQ_MUTUAL_AUTH);
    DUMP_FLAG(dwRemainder, ASC_REQ_REPLAY_DETECT);
    DUMP_FLAG(dwRemainder, ASC_REQ_SEQUENCE_DETECT);
    DUMP_FLAG(dwRemainder, ASC_REQ_CONFIDENTIALITY);
    DUMP_FLAG(dwRemainder, ASC_REQ_USE_SESSION_KEY);
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOCATE_MEMORY);
    DUMP_FLAG(dwRemainder, ASC_REQ_USE_DCE_STYLE);
    DUMP_FLAG(dwRemainder, ASC_REQ_DATAGRAM);
    DUMP_FLAG(dwRemainder, ASC_REQ_CONNECTION);
    DUMP_FLAG(dwRemainder, ASC_REQ_CALL_LEVEL);
    DUMP_FLAG(dwRemainder, ASC_REQ_EXTENDED_ERROR);
    DUMP_FLAG(dwRemainder, ASC_REQ_STREAM);
    DUMP_FLAG(dwRemainder, ASC_REQ_INTEGRITY);
    DUMP_FLAG(dwRemainder, ASC_REQ_LICENSING);
    DUMP_FLAG(dwRemainder, ASC_REQ_IDENTIFY);
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOW_NULL_SESSION);
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOW_NON_USER_LOGONS);
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOW_CONTEXT_REPLAY);
    DUMP_FLAG(dwRemainder, ASC_REQ_FRAGMENT_TO_FIT);
    DUMP_FLAG(dwRemainder, ASC_REQ_FRAGMENT_SUPPLIED);
    DUMP_FLAG(dwRemainder, ASC_REQ_NO_TOKEN);
#ifdef ASC_REQ_PROXY_BINDINGS
    DUMP_FLAG(dwRemainder, ASC_REQ_PROXY_BINDINGS);
#endif
#ifdef ASC_REQ_ALLOW_MISSING_BINDINGS
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOW_MISSING_BINDINGS);
#endif
    DUMP_FLAGS_REMAINING(dwRemainder);
}

static
VOID
DumpAscRetFlagsEx(
    DWORD dwFlags,
    BOOL bShowHeader
    )
{
    DWORD dwRemainder = dwFlags;

    if (bShowHeader)
    {
        printf("ASC Ret Flags = 0x%08x\n", dwFlags);
    }
    DUMP_FLAG(dwRemainder, ASC_RET_DELEGATE);
    DUMP_FLAG(dwRemainder, ASC_RET_MUTUAL_AUTH);
    DUMP_FLAG(dwRemainder, ASC_RET_REPLAY_DETECT);
    DUMP_FLAG(dwRemainder, ASC_RET_SEQUENCE_DETECT);
    DUMP_FLAG(dwRemainder, ASC_RET_CONFIDENTIALITY);
    DUMP_FLAG(dwRemainder, ASC_RET_USE_SESSION_KEY);
    DUMP_FLAG(dwRemainder, ASC_RET_ALLOCATED_MEMORY);
    DUMP_FLAG(dwRemainder, ASC_RET_USED_DCE_STYLE);
    DUMP_FLAG(dwRemainder, ASC_RET_DATAGRAM);
    DUMP_FLAG(dwRemainder, ASC_RET_CONNECTION);
    DUMP_FLAG(dwRemainder, ASC_RET_CALL_LEVEL);
    DUMP_FLAG(dwRemainder, ASC_RET_THIRD_LEG_FAILED);
    DUMP_FLAG(dwRemainder, ASC_RET_EXTENDED_ERROR);
    DUMP_FLAG(dwRemainder, ASC_RET_STREAM);
    DUMP_FLAG(dwRemainder, ASC_RET_INTEGRITY);
    DUMP_FLAG(dwRemainder, ASC_RET_LICENSING);
    DUMP_FLAG(dwRemainder, ASC_RET_IDENTIFY);
    DUMP_FLAG(dwRemainder, ASC_RET_NULL_SESSION);
    DUMP_FLAG(dwRemainder, ASC_RET_ALLOW_NON_USER_LOGONS);
    DUMP_FLAG(dwRemainder, ASC_RET_ALLOW_CONTEXT_REPLAY);
    DUMP_FLAG(dwRemainder, ASC_RET_FRAGMENT_ONLY);
    DUMP_FLAG(dwRemainder, ASC_RET_NO_TOKEN);
    DUMP_FLAG(dwRemainder, ASC_RET_NO_ADDITIONAL_TOKEN);
#ifdef ASC_RET_NO_PROXY_BINDINGS
    DUMP_FLAG(dwRemainder, ASC_RET_NO_PROXY_BINDINGS);
#endif
#ifdef ASC_RET_MISSING_BINDINGS
    DUMP_FLAG(dwRemainder, ASC_RET_MISSING_BINDINGS);
#endif
    DUMP_FLAGS_REMAINING(dwRemainder);
}

VOID
DumpAscRetFlags(
    DWORD dwFlags
    )
{
    DumpAscRetFlagsEx(dwFlags, TRUE);
}

#define QUERY_CTX(Context, Type, Buffer) \
    do { \
        DWORD dwError = QueryContextAttributes(Context, Type, Buffer); \
        if (dwError && (dwError != SEC_E_UNSUPPORTED_FUNCTION)) \
        { \
            if (0x80000000 & dwError) \
            { \
                printf("Error 0x%08x querying context attributes for " #Type "\n", dwError); \
            } \
            else \
            { \
                printf("Error %u querying context attributes for " #Type "\n", dwError); \
            } \
            PrintWinError(dwError); \
            exit(1); \
        } \
    } while (0)

static
VOID
DumpSecPkgFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;
    PCSTR pszPrefix = "      ";

    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_INTEGRITY);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_PRIVACY);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_TOKEN_ONLY);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_DATAGRAM);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_CONNECTION);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_MULTI_REQUIRED);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_CLIENT_ONLY);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_EXTENDED_ERROR);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_IMPERSONATION);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_ACCEPT_WIN32_NAME);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_STREAM);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_NEGOTIABLE);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_GSS_COMPATIBLE);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_LOGON);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_ASCII_BUFFERS);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_FRAGMENT);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_MUTUAL_AUTH);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_DELEGATION);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_READONLY_WITH_CHECKSUM);
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_RESTRICTED_TOKENS);
#ifdef SECPKG_FLAG_NEGO_EXTENDER
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_NEGO_EXTENDER);
#endif
#ifdef SECPKG_FLAG_NEGOTIABLE2
    _DUMP_FLAG(pszPrefix, dwRemainder, SECPKG_FLAG_NEGOTIABLE2);
#endif
    _DUMP_FLAGS_REMAINIG(pszPrefix, dwRemainder);
}

VOID
DumpContext(
    PCtxtHandle Context,
    BOOL bIsAccept
    )
{
    SecPkgContext_PackageInfo packageInfo = { 0 };
    SecPkgContext_Flags flags = { 0 };
    SecPkgContext_Names names = { 0 };
    SecPkgContext_NativeNames nativeNames = { 0 };
    SecPkgContext_SessionKey sessionKey = { 0 };
    SecPkgContext_KeyInfo keyInfo = { 0 };
    SecPkgContext_Authority authority = { 0 };
    SecPkgContext_Sizes sizes = { 0 };

    QUERY_CTX(Context, SECPKG_ATTR_PACKAGE_INFO, &packageInfo);
    QUERY_CTX(Context, SECPKG_ATTR_FLAGS, &flags);
    QUERY_CTX(Context, SECPKG_ATTR_NAMES, &names);
    QUERY_CTX(Context, SECPKG_ATTR_NATIVE_NAMES, &nativeNames);
    // QUERY_CTX(Context, SECPKG_ATTR_SESSION_KEY, &sessionKey);
    QUERY_CTX(Context, SECPKG_ATTR_KEY_INFO, &keyInfo);
    QUERY_CTX(Context, SECPKG_ATTR_AUTHORITY, &authority);
    QUERY_CTX(Context, SECPKG_ATTR_SIZES, &sizes);

    if (packageInfo.PackageInfo)
    {
        printf("  PackageInfo = {\n"
               "    Caps = 0x%08x,\n"
               "    Version = 0x%04x,\n"
               "    RPC ID = 0x%04x,\n"
               "    Max Token = %u,\n"
               "    Name = \"%s\",\n"
               "    Comment = \"%s\",\n"
               "  }\n",
               packageInfo.PackageInfo->fCapabilities,
               packageInfo.PackageInfo->wVersion,
               packageInfo.PackageInfo->wRPCID,
               packageInfo.PackageInfo->cbMaxToken,
               packageInfo.PackageInfo->Name,
               packageInfo.PackageInfo->Comment);
        DumpSecPkgFlags(packageInfo.PackageInfo->fCapabilities);
    }
    else
    {
        printf("  PackageInfo = <null>\n");
    }
    printf("  Flags = 0x%08x\n", flags.Flags);
    if (bIsAccept)
    {
        DumpAscRetFlagsEx(flags.Flags, FALSE);
    }
    else
    {
        DumpIscRetFlagsEx(flags.Flags, FALSE);
    }
    if (names.sUserName)
    {
        printf("  UserName = \"%s\"\n", names.sUserName);
    }
    if (nativeNames.sClientName)
    {
        printf("  ClientName = \"%s\"\n", nativeNames.sClientName);
    }
    if (nativeNames.sServerName)
    {
        printf("  ServerName = \"%s\"\n", nativeNames.sServerName);
    }
    if (sessionKey.SessionKeyLength)
    {
        DWORD i = 0;
        printf("  SessionKey = { %d, { ", sessionKey.SessionKeyLength);
        for (i = 0; i < sessionKey.SessionKeyLength; i++)
        {
            printf("0x%02 ", sessionKey.SessionKey[i]);
        }
        printf("}\n");
    }
    if (keyInfo.sSignatureAlgorithmName || keyInfo.sEncryptAlgorithmName ||
        keyInfo.KeySize || keyInfo.SignatureAlgorithm || keyInfo.EncryptAlgorithm)
    {
        printf("  KeyInfo = {\n"
               "    Signature Algorithm Name = \"%s\"\n"
               "    Encrypt Algorithm Name = \"%s\"\n"
               "    Key Size = %u\n"
               "    Signature Algorithm ID = %u\n"
               "    Encrypt Algorithm ID = %u\n"
               "  }\n",
               keyInfo.sSignatureAlgorithmName,
               keyInfo.sEncryptAlgorithmName,
               keyInfo.KeySize,
               keyInfo.SignatureAlgorithm,
               keyInfo.EncryptAlgorithm);
    }
    if (authority.sAuthorityName)
    {
        printf("  AuthorityName = \"%s\"\n", authority.sAuthorityName);
    }
    printf("  Sizes = {\n"
           "    Max Token = %u\n"
           "    Max Signature = %u\n"
           "    Block = %u\n"
           "    Security Trailer = %u\n"
           "  }\n",
           sizes.cbMaxToken,
           sizes.cbMaxSignature,
           sizes.cbBlockSize,
           sizes.cbSecurityTrailer);
}

VOID
DumpMiniContext(
    PCtxtHandle Context,
    BOOL bIsAccept
    )
{
    SecPkgContext_PackageInfo packageInfo = { 0 };
    SecPkgContext_Flags flags = { 0 };
    SecPkgContext_Names names = { 0 };
    SecPkgContext_NativeNames nativeNames = { 0 };

    QUERY_CTX(Context, SECPKG_ATTR_PACKAGE_INFO, &packageInfo);
    QUERY_CTX(Context, SECPKG_ATTR_FLAGS, &flags);
    QUERY_CTX(Context, SECPKG_ATTR_NAMES, &names);
    QUERY_CTX(Context, SECPKG_ATTR_NATIVE_NAMES, &nativeNames);

    if (packageInfo.PackageInfo)
    {
        printf("  Package = \"%s\"\n",
               packageInfo.PackageInfo->Name);
    }
    printf("  Flags = 0x%08x\n", flags.Flags);
    if (bIsAccept)
    {
        DumpAscRetFlagsEx(flags.Flags, FALSE);
    }
    else
    {
        DumpIscRetFlagsEx(flags.Flags, FALSE);
    }
    if (names.sUserName)
    {
        printf("  UserName = \"%s\"\n", names.sUserName);
    }
    if (nativeNames.sClientName)
    {
        printf("  ClientName = \"%s\"\n", nativeNames.sClientName);
    }
    if (nativeNames.sServerName)
    {
        printf("  ServerName = \"%s\"\n", nativeNames.sServerName);
    }
}
