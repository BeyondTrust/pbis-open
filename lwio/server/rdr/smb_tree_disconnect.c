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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        tree_disconnect.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Tree Disconnect Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

uint32_t
TreeDisconnect(
    PSMB_TREE pTree
    )
{
    uint32_t ntStatus = 0;
    SMB_PACKET packet = {0};
    SMB_RESPONSE *pResponse = NULL;
    SMB_PACKET *pResponsePacket = NULL;
    uint16_t wMid = 0;

    /* @todo: make initial length configurable */
    ntStatus = SMBPacketBufferAllocate(
                    pTree->pSession->pSocket->hPacketAllocator,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeAcquireMid(
                    pTree,
                    &wMid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                packet.pRawBuffer,
                packet.bufferLen,
                COM_TREE_DISCONNECT,
                0,
                0,
                pTree->tid,
                gRdrRuntime.SysPid,
                pTree->pSession->uid,
                wMid,
                TRUE,
                &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    packet.pSMBHeader->wordCount = 0;

    /* @todo: to restart properly, we need a marshalling function to check the
       size of bytecount against the size of the remaining packet bytes. */
    packet.pData = packet.pParams;          /* ByteCount */
    packet.bufferUsed += sizeof(uint16_t);
    memset(packet.pData, 0, sizeof(uint16_t));

    // no byte order conversions necessary (due to zeros)

    ntStatus = SMBPacketMarshallFooter(&packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBResponseCreate(wMid, &pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    /* @todo: on send packet error, the response must be removed from the
       tree. */
    ntStatus = SMBSocketSend(pTree->pSession->pSocket, &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeReceiveResponse(
                    pTree,
                    packet.haveSignature,
                    packet.sequence + 1,
                    pResponse,
                    &pResponsePacket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pResponsePacket)
    {
        SMBPacketRelease(
             pTree->pSession->pSocket->hPacketAllocator,
             pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(
                        pTree->pSession->pSocket->hPacketAllocator,
                        packet.pRawBuffer,
                        packet.bufferLen);
    }

    if (pResponse)
    {
        SMBResponseFree(pResponse);
    }

    return ntStatus;

error:

    goto cleanup;
}
