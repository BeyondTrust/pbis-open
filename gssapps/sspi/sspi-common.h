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

#ifdef UNICODE
#undef UNICODE
#endif

#if defined(_WIN32)

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

#else

#include "config.h"
#include <lw/base.h>
#include <ntlm/sspintlm.h>
#include <wc16str.h>
#include <lwdef.h>
#include <lwerror.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>

#endif

#define BAIL_ON_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            printf("Error %u (0x%08x) at %s() [%s:%d]\n", \
                   dwError, dwError, \
                   __FUNCTION__, GetBasename(__FILE__), __LINE__); \
            goto error; \
        } \
    } while (0)

typedef struct _FLAGMAPPING
{
    PCSTR name;
    ULONG value;
    PCSTR realname;
} FLAGMAPPING, *PFLAGMAPPING;

#define _INIT_FLAGMAPPING(Prefix, Flag) { "-" #Flag, Prefix ## Flag, #Prefix #Flag }
#define END_FLAGMAPPING() { NULL, 0, NULL }

PCSTR
GetBasename(
    IN PCSTR pszPath
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
DumpNtlmMessage(
    PVOID pBuffer,
    DWORD dwSize
    );

VOID
DumpBuffer(
    PVOID pBuffer,
    DWORD dwSize
    );

DWORD
SendToken(
    IN INT nSocket,
    IN PSecBuffer pToken
    );

DWORD
RecvToken(
    IN INT nSocket,
    OUT PSecBuffer pToken
    );
