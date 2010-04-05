/*
 * Copyright 1993 by OpenVision Technologies, Inc.
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

#ifndef _NTLM_SERVER_PROTOTYPES_H_
#define _NTLM_SERVER_PROTOTYPES_H_

DWORD
Usage(VOID);

DWORD
ServerAcquireCreds(
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN PCHAR pSecPkgName,
    OUT NTLM_CRED_HANDLE *pServerCreds
    );

DWORD
CreateSocket(
    IN USHORT uPort,
    OUT PINT pSocket
    );

DWORD
SignServer(
    IN INT nSocket,
    IN NTLM_CRED_HANDLE *pServerCreds,
    IN DWORD AscFlags
    );

DWORD
FreeContextBuffer(
    IN PVOID pBuffer
    );

DWORD
ServerEstablishContext(
    IN INT nSocket,
    IN NTLM_CRED_HANDLE *pServerCreds,
    OUT NTLM_CONTEXT_HANDLE *pContext,
    IN DWORD AscFlags
    );

DWORD
SendToken(
    IN INT nSocket,
    IN PSecBuffer pToken
    );

DWORD
WriteAll(
    IN INT nSocket,
    IN PCHAR pBuffer,
    IN UINT nBytes,
    OUT PINT nBytesWritten
    );

DWORD
RecvToken(
    IN INT nSocket,
    OUT PSecBuffer pToken
    );

DWORD
ReadAll(
    IN INT nSocket,
    OUT PCHAR pBuffer,
    IN UINT nBytes,
    OUT PINT nBytesRead
    );

VOID
PrintHexDump(
    DWORD length,
    PBYTE pBuffer
    );

#endif  //_NTLM_SERVER_PROTOTYPES_H_
