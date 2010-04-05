/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

NTSTATUS
SMBSocketCreate(
    IN PCWSTR pwszHostname,
    IN BOOLEAN bUseSignedMessagesIfSupported,
    OUT PSMB_SOCKET* ppSocket
    );

NTSTATUS
SMBSocketConnect(
    PSMB_SOCKET      pSocket
    );

VOID
SMBSocketAddReference(
    PSMB_SOCKET pSocket
    );

VOID
SMBSocketInvalidate(
    PSMB_SOCKET    pSocket,
    NTSTATUS ntStatus
    );

VOID
SMBSocketInvalidate_InLock(
    PSMB_SOCKET pSocket,
    NTSTATUS ntStatus
    );

VOID
SMBSocketSetState(
    PSMB_SOCKET        pSocket,
    RDR_SOCKET_STATE   state
    );

RDR_SOCKET_STATE
SMBSocketGetState(
    PSMB_SOCKET        pSocket
    );

BOOLEAN
SMBSocketTimedOut(
    PSMB_SOCKET pSocket
    );

BOOLEAN
SMBSocketTimedOut_InLock(
    PSMB_SOCKET pSocket
    );

ULONG
SMBSocketGetNextSequence(
    PSMB_SOCKET pSocket
    );

VOID
SMBSocketUpdateLastActiveTime(
    PSMB_SOCKET pSocket
    );

NTSTATUS
SMBSocketSend(
    IN PSMB_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    );

NTSTATUS
SMBSocketReceiveAndUnmarshall(
    IN PSMB_SOCKET pSocket,
    OUT PSMB_PACKET pPacket
    );

NTSTATUS
SMBSocketRead(
    PSMB_SOCKET pSocket,
    uint8_t    *buffer,
    uint32_t    dwLen,
    uint32_t   *actualLen
    );

NTSTATUS
SMBSocketReceiveResponse(
    IN PSMB_SOCKET pSocket,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    OUT PSMB_PACKET* ppPacket
    );

NTSTATUS
SMBSocketFindSessionByPrincipal(
    IN PSMB_SOCKET pSocket,
    IN PCSTR pszPrincipal,
    OUT PSMB_SESSION* ppSession
    );

VOID
SMBSocketRelease(
    PSMB_SOCKET pSocket
    );

VOID
SMBSocketFree(
    PSMB_SOCKET pSocket
    );

VOID
RdrSocketSetIgnoreServerSignatures(
    PSMB_SOCKET pSocket,
    BOOLEAN bValue
    );

BOOLEAN
RdrSocketGetIgnoreServerSignatures(
    PSMB_SOCKET pSocket
    );

VOID
SMBSocketBeginSequence(
    PSMB_SOCKET pSocket
    );

#endif /* __SOCKET_H__ */
