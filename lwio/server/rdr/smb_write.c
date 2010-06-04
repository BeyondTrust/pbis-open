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

#include "rdr.h"

uint32_t
WireWriteFile(
    PSMB_TREE pTree,
    uint16_t  fid,
    uint64_t  llFileWriteOffset,
    uint16_t  writeMode,
    uint8_t*  pWriteBuffer,
    uint16_t  wWriteLen,
    uint16_t* pwWritten,
    void*     pOverlapped
    )
{
    uint32_t ntStatus = 0;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    WRITE_ANDX_REQUEST_HEADER_WC_14 *pRequestHeader = NULL;
    WRITE_ANDX_RESPONSE_HEADER *pResponseHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    SMB_PACKET *pResponsePacket = NULL;
    uint16_t wMid = 0;
    uint16_t wNumBytesWriteable = 0;
    uint16_t dataOffset = 0;

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
                COM_WRITE_ANDX,
                0,
                0,
                pTree->tid,
                gRdrRuntime.SysPid,
                pTree->pSession->uid,
                wMid,
                TRUE,
                &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    packet.pData = packet.pParams + sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14);
    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14);

    packet.pSMBHeader->wordCount = 14;

    pRequestHeader = (WRITE_ANDX_REQUEST_HEADER_WC_14 *) packet.pParams;

    pRequestHeader->fid = fid;
    pRequestHeader->offset = llFileWriteOffset & 0x00000000FFFFFFFFLL;
    pRequestHeader->reserved = 0;
    pRequestHeader->writeMode = writeMode;
    pRequestHeader->remaining = 0;

    /* ignored if CAP_LARGE_WRITEX is set */
    wNumBytesWriteable = UINT16_MAX - (packet.pParams - (uint8_t*)packet.pSMBHeader) - sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14);
    // And, then the alignment
    wNumBytesWriteable -= (packet.pData - (uint8_t *) pRequestHeader) % 2;
    if (wWriteLen > wNumBytesWriteable)
    {
        wWriteLen = wNumBytesWriteable;
    }

    pRequestHeader->dataLength = wWriteLen;
    /* @todo: what is this value if CAP_LARGE_WRITEX is set? */
    pRequestHeader->dataLengthHigh = 0;
    pRequestHeader->dataOffset = 0;
    /* only present if wordCount = 14 and not 12 */
    pRequestHeader->offsetHigh = (llFileWriteOffset & 0xFFFFFFFF00000000LL) >> 32;
    pRequestHeader->byteCount = wWriteLen;

    ntStatus = MarshallWriteRequestData(
                packet.pData,
                packet.bufferLen - packet.bufferUsed,
                (packet.pData - (uint8_t *) pRequestHeader) % 2,
                &packetByteCount,
                &dataOffset,
                pWriteBuffer,
                wWriteLen);
    BAIL_ON_NT_STATUS(ntStatus);

    packet.bufferUsed += packetByteCount;

    pRequestHeader->dataOffset = dataOffset;
    pRequestHeader->dataOffset += packet.pData - (uint8_t *) packet.pSMBHeader;

    // byte order conversions
    SMB_HTOL16_INPLACE(pRequestHeader->fid);
    SMB_HTOL32_INPLACE(pRequestHeader->offset);
    SMB_HTOL16_INPLACE(pRequestHeader->writeMode);
    SMB_HTOL16_INPLACE(pRequestHeader->remaining);
    SMB_HTOL16_INPLACE(pRequestHeader->dataLengthHigh);
    SMB_HTOL16_INPLACE(pRequestHeader->dataLength);
    SMB_HTOL16_INPLACE(pRequestHeader->dataOffset);
    SMB_HTOL32_INPLACE(pRequestHeader->offsetHigh);
    SMB_HTOL16_INPLACE(pRequestHeader->byteCount);

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

    pResponseHeader = (WRITE_ANDX_RESPONSE_HEADER*) pResponsePacket->pParams;

    // byte order conversions
    SMB_LTOH16_INPLACE(pResponseHeader->count);
    //SMB_LTOH16_INPLACE(pResponseHeader->remaining);
    SMB_LTOH16_INPLACE(pResponseHeader->countHigh);
    //SMB_LTOH16_INPLACE(pResponseHeader->reserved);
    //SMB_LTOH16_INPLACE(pResponseHeader->byteCount);

    *pwWritten = pResponseHeader->count;

cleanup:

    if (pResponsePacket)
    {
        SMBPacketRelease(pTree->pSession->pSocket->hPacketAllocator,
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

    *pwWritten = 0;

    goto cleanup;
}
