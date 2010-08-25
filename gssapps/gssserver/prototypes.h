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
 * Copyright (C) 2003, 2004, 2005 by the Massachusetts Institute of Technology.
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

#ifndef _GSS_SERVER_PROTOTYPES_H_
#define _GSS_SERVER_PROTOTYPES_H_

DWORD
Usage(void);

DWORD
ServerAcquireCreds(
    OUT gss_cred_id_t *ppServerCreds
    );

DWORD
CreateSocket(
    IN USHORT usPort,
    OUT PINT pSocket
    );

DWORD
SignServer(
    IN INT nSocket,
    IN gss_cred_id_t pServerCreds
    );

DWORD
ServerEstablishContext(
    IN INT nSocket,
    IN gss_cred_id_t pServerCreds,
    OUT gss_ctx_id_t *ppContext
    );

DWORD
RecvToken(
    IN INT nSocket,
    OUT gss_buffer_t pToken
    );

DWORD
ReadAll(
    IN INT nSocket,
    OUT PCHAR pBuffer,
    IN UINT nByte,
    OUT PINT pBytesRead
    );

DWORD
SendToken(
    IN INT nSocket,
    IN gss_buffer_t pToken
    );

DWORD
WriteAll(
    IN INT nSocket,
    IN PCHAR pBuffer,
    IN UINT nByte,
    OUT PINT pBytesWritten
    );

VOID
PrintHexDump(
    DWORD length,
    PVOID pBuffer,
    INT bIsNtlmMessage
    );

#endif //_GSS_SERVER_PROTOTYPES_H_
