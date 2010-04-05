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

VOID
SMBTreeAddReference(
    PSMB_TREE pTree
    );

VOID
SMBTreeRelease(
    PSMB_TREE pTree
    );

NTSTATUS
SMBTreeCreate(
    PSMB_TREE* ppTree
    );

NTSTATUS
SMBTreeAcquireMid(
    PSMB_TREE pTree,
    uint16_t* pwMid
    );

NTSTATUS
SMBTreeSetState(
    PSMB_TREE pTree,
    SMB_RESOURCE_STATE state
    );

NTSTATUS
SMBTreeInvalidate(
    PSMB_TREE      pTree,
    NTSTATUS ntStatus
    );

NTSTATUS
SMBSrvClientTreeAddResponse(
    PSMB_TREE pTree,
    SMB_RESPONSE *pResponse
    );

NTSTATUS
SMBTreeReceiveResponse(
    IN PSMB_TREE pTree,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    IN PSMB_RESPONSE pResponse,
    OUT PSMB_PACKET* ppResponsePacket
    );

NTSTATUS
SMBTreeFindLockedResponseByMID(
    PSMB_TREE      pTree,
    uint16_t       wMid,
    PSMB_RESPONSE* ppResponse
    );

VOID
SMBTreeFree(
    PSMB_TREE pTree
    );
