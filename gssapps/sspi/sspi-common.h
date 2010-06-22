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

#define NTLM_FLAG_UNICODE               0x00000001  /* unicode charset */
#define NTLM_FLAG_OEM                   0x00000002  /* oem charset */
#define NTLM_FLAG_REQUEST_TARGET        0x00000004  /* ret trgt in challenge */
#define NTLM_FLAG_UNDEFINED_00000008    0x00000008

#define NTLM_FLAG_SIGN                  0x00000010  /* sign requested */
#define NTLM_FLAG_SEAL                  0x00000020  /* encryption requested */
#define NTLM_FLAG_DATAGRAM              0x00000040  /* udp message */
#define NTLM_FLAG_LM_KEY                0x00000080  /* use LM key for crypto */

#define NTLM_FLAG_NETWARE               0x00000100  /* netware - unsupported */
#define NTLM_FLAG_NTLM                  0x00000200  /* use NTLM auth */
#define NTLM_FLAG_UNDEFINED_00000400    0x00000400
#define NTLM_FLAG_ANONYMOUS             0x00000800

#define NTLM_FLAG_DOMAIN                0x00001000  /* domain supplied */
#define NTLM_FLAG_WORKSTATION           0x00002000  /* wks supplied */
#define NTLM_FLAG_LOCAL_CALL            0x00004000  /* loopback auth */
#define NTLM_FLAG_ALWAYS_SIGN           0x00008000  /* use dummy sig */

#define NTLM_FLAG_TYPE_DOMAIN           0x00010000  /* domain authenticator */
#define NTLM_FLAG_TYPE_SERVER           0x00020000  /* server authenticator */
#define NTLM_FLAG_TYPE_SHARE            0x00040000  /* share authenticator */
#define NTLM_FLAG_NTLM2                 0x00080000  /* use NTLMv2 key */

#define NTLM_FLAG_INIT_RESPONSE         0x00100000  /* unknown */
#define NTLM_FLAG_ACCEPT_RESPONSE       0x00200000  /* unknown */
#define NTLM_FLAG_NON_NT_SESSION_KEY    0x00400000  /* unknown */
#define NTLM_FLAG_TARGET_INFO           0x00800000  /* target info used */

#define NTLM_FLAG_UNDEFINED_01000000    0x01000000
#define NTLM_FLAG_UNKNOWN_02000000      0x02000000  /* needed, for what? */
#define NTLM_FLAG_UNDEFINED_04000000    0x04000000
#define NTLM_FLAG_UNDEFINED_08000000    0x08000000

#define NTLM_FLAG_UNDEFINED_10000000    0x10000000
#define NTLM_FLAG_128                   0x20000000  /* 128-bit encryption */
#define NTLM_FLAG_KEY_EXCH              0x40000000  /* perform key exchange */
#define NTLM_FLAG_56                    0x80000000  /* 56-bit encryption */

#define DUMP_FLAG(Flags, Flag) \
    do { \
        if ((Flags) & (Flag)) \
        { \
            printf("    %-35s (0x%08X)\n", # Flag, Flag); \
            (Flags) &= ~(Flag); \
        } \
    } while (0)

typedef struct _NTLM_SEC_BUFFER
{
    USHORT usLength;    // number of bytes used
    USHORT usMaxLength; // true size of buffer in bytes
    DWORD  dwOffset;
} NTLM_SEC_BUFFER, *PNTLM_SEC_BUFFER;

typedef struct _NTLM_MESSAGE
{
    UCHAR NtlmSignature[8];
    DWORD MessageType;
} NTLM_MESSAGE, *PNTLM_MESSAGE;

typedef struct _NTLM_NEGOTIATE_MESSAGE
{
    UCHAR NtlmSignature[8];
    DWORD MessageType;
    DWORD NtlmFlags;
    // Optional Supplied Domain NTLM_SEC_BUFFER
    // Optional Supplied Workstation NTLM_SEC_BUFFER
    // Optional OS Version 8 bytes
    // Optional Data
} NTLM_NEGOTIATE_MESSAGE, *PNTLM_NEGOTIATE_MESSAGE;

typedef struct _NTLM_CHALLENGE_MESSAGE
{
    UCHAR NtlmSignature[8];
    DWORD MessageType;
    NTLM_SEC_BUFFER Target;
    DWORD NtlmFlags;
    UCHAR Challenge[8];
    // Optional Context 8 bytes
    // Optional Target Information NTLM_SEC_BUFFER
    // Optional OS Version 8 bytes
    // Optional Data
} NTLM_CHALLENGE_MESSAGE, *PNTLM_CHALLENGE_MESSAGE;

typedef struct _NTLM_RESPONSE_MESSAGE
{
    UCHAR NtlmSignature[8];
    DWORD MessageType;
    NTLM_SEC_BUFFER LmResponse;
    NTLM_SEC_BUFFER NtResponse;
    NTLM_SEC_BUFFER AuthTargetName;
    NTLM_SEC_BUFFER UserName;
    NTLM_SEC_BUFFER Workstation;
    // Optional
    NTLM_SEC_BUFFER SessionKey;
    // Optional
    DWORD NtlmFlags;
    // Optional OS Version 8 bytes
    // Optional Data
} NTLM_RESPONSE_MESSAGE, *PNTLM_RESPONSE_MESSAGE;

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
