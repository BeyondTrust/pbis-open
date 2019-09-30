/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

NTSTATUS
RdrSocketCreate(
    IN PCWSTR pwszHostname,
    OUT PRDR_SOCKET* ppSocket
    );

NTSTATUS
RdrSocketSetProtocol(
    PRDR_SOCKET pSocket,
    SMB_PROTOCOL_VERSION version
    );

NTSTATUS
RdrSocketConnect(
    PRDR_SOCKET      pSocket
    );

VOID
RdrSocketRevive(
    PRDR_SOCKET pSocket
    );

VOID
RdrSocketInvalidate(
    PRDR_SOCKET    pSocket,
    NTSTATUS ntStatus
    );

VOID
RdrSocketRelease(
    PRDR_SOCKET pSocket
    );

VOID
RdrSocketSetIgnoreServerSignatures(
    PRDR_SOCKET pSocket,
    BOOLEAN bValue
    );

VOID
RdrSocketBeginSequence(
    PRDR_SOCKET pSocket
    );

NTSTATUS
RdrSocketTransceive(
    IN OUT PRDR_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    );

VOID
RdrSocketCancel(
    IN PRDR_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    );

NTSTATUS
RdrSocketAddSessionByUID(
    PRDR_SOCKET  pSocket,
    PRDR_SESSION pSession
    );

NTSTATUS
RdrSocketAddSession2ById(
    PRDR_SOCKET  pSocket,
    PRDR_SESSION2 pSession
    );

NTSTATUS
RdrSocketFindOrCreate(
    IN PCWSTR pwszHostname,
    OUT PRDR_SOCKET* ppSocket
    );

BOOLEAN
RdrSocketIsValid(
    PRDR_SOCKET pSocket
    );

#endif /* __SOCKET_H__ */
