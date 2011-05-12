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

#ifndef _GSSMISC_H_
#define _GSSMISC_H_

#ifdef UNICODE
#undef UNICODE
#endif
#include <windows.h>
#include <winerror.h>
#include <rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>
#define SECURITY_WIN32
#include <security.h>
#include <ntsecapi.h>
#include <stddef.h>
#include <sys/types.h>
#include "gssapi.h"

extern FILE *display_file;

int send_token(int s, unsigned int flags, PSecBuffer tok);
int recv_token(int s,  unsigned int *flags, PSecBuffer tok);
void display_status(char *msg, ULONG maj_stat, ULONG min_stat);
void display_ctx_flags(ULONG flags);
void print_token(PSecBuffer tok);

void
PrintWinError(
    DWORD wStatus
    );

VOID
DumpIscReqFlags(
    DWORD dwFlags
    );

VOID
DumpIscRetFlags(
    DWORD dwFlags
    );

VOID
DumpAscReqFlags(
    DWORD dwFlags
    );

VOID
DumpAscRetFlags(
    DWORD dwFlags
    );

VOID
DumpContext(
    PCtxtHandle Context,
    BOOL bIsAccept
    );

VOID
DumpMiniContext(
    PCtxtHandle Context,
    BOOL bIsAccept
    );

#endif

typedef struct
{

    PCHAR     name;
    OM_uint32 value;
    PCHAR     realname;

} FLAGMAPPING, *PFLAGMAPPING;

#define TOKEN_NOOP              (1<<0)
#define TOKEN_CONTEXT           (1<<1)
#define TOKEN_DATA              (1<<2)
#define TOKEN_MIC               (1<<3)
#define TOKEN_CONTEXT_NEXT      (1<<4)
#define TOKEN_WRAPPED           (1<<5)
#define TOKEN_ENCRYPTED         (1<<6)
#define TOKEN_SEND_MIC          (1<<7)
#define TOKEN_SEND_IOV          (1<<8)

extern PSecBuffer empty_token;

