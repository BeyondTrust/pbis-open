/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

NTSTATUS
SMBSessionCreate(
    PSMB_SESSION* ppSession
    );

VOID
SMBSessionAddReference(
    PSMB_SESSION pSession
    );

VOID
SMBSessionInvalidate(
    PSMB_SESSION   pSession,
    NTSTATUS ntStatus
    );

VOID
SMBSessionSetState(
    PSMB_SESSION pSession,
    SMB_RESOURCE_STATE state
    );

VOID
SMBSessionUpdateLastActiveTime(
    PSMB_SESSION pSession
    );

NTSTATUS
SMBSessionFindTreeByPath(
    IN PSMB_SESSION pSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    );

NTSTATUS
SMBSessionFindTreeById(
    PSMB_SESSION pSession,
    uint16_t     tid,
    PSMB_TREE*   ppTree
    );

NTSTATUS
SMBSessionReceiveResponse(
    IN PSMB_SESSION pSession,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    OUT PSMB_PACKET* ppPacket
    );

VOID
SMBSessionRelease(
    PSMB_SESSION pSession
    );

VOID
SMBSessionFree(
    PSMB_SESSION pSession
    );

