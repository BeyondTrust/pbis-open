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
 *        readfile.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        ReadFile API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "rdr.h"

NTSTATUS
RdrTransactReadFile(
    PSMB_TREE pTree,
    USHORT usFid,
    ULONG64 ullFileReadOffset,
    PBYTE pReadBuffer,
    USHORT usReadLen,
    USHORT usMinReadLen,
    PUSHORT pusBytesRead
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    uint16_t packetByteCount = 0;
    READ_ANDX_REQUEST_HEADER_WC_12 *pRequestHeader = NULL;
    READ_ANDX_RESPONSE_HEADER *pResponseHeader = NULL;
    PSMB_RESPONSE pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    USHORT usBytesRead = 0;

    /* @todo: make initial length configurable */
    ntStatus = SMBPacketBufferAllocate(
                    pTree->pSession->pSocket->hPacketAllocator,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeAcquireMid(
                    pTree,
                    &usMid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                packet.pRawBuffer,
                packet.bufferLen,
                COM_READ_ANDX,
                0,
                0,
                pTree->tid,
                gRdrRuntime.SysPid,
                pTree->pSession->uid,
                usMid,
                TRUE,
                &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    packet.pData = packet.pParams + sizeof(READ_ANDX_REQUEST_HEADER_WC_12);
    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(READ_ANDX_REQUEST_HEADER_WC_12);

    /* if LM 0.12 is used, word count is 12. Otherwise 10 */
    packet.pSMBHeader->wordCount = 12;

    pRequestHeader = (PREAD_ANDX_REQUEST_HEADER_WC_12) packet.pParams;

    pRequestHeader->fid = usFid;
    pRequestHeader->offset = ullFileReadOffset & 0x00000000FFFFFFFFLL;
    pRequestHeader->maxCount = usReadLen;
    pRequestHeader->minCount = usReadLen; /* blocking read */
    /* @todo: if CAP_LARGE_READX is set, what are the high 16 bits? */
    pRequestHeader->maxCountHigh = 0;
    pRequestHeader->remaining = 0; /* obsolete */
    pRequestHeader->offsetHigh = (ullFileReadOffset & 0xFFFFFFFF00000000LL) >> 32;
    pRequestHeader->byteCount = 0;
    
    packet.bufferUsed += packetByteCount;

    // byte order conversions
    SMB_HTOL16_INPLACE(pRequestHeader->fid);
    SMB_HTOL32_INPLACE(pRequestHeader->offset);
    SMB_HTOL16_INPLACE(pRequestHeader->maxCount);
    SMB_HTOL16_INPLACE(pRequestHeader->minCount);
    SMB_HTOL32_INPLACE(pRequestHeader->maxCountHigh);
    SMB_HTOL16_INPLACE(pRequestHeader->remaining);
    SMB_HTOL32_INPLACE(pRequestHeader->offsetHigh);
    SMB_HTOL16_INPLACE(pRequestHeader->byteCount);

    ntStatus = SMBPacketMarshallFooter(&packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBResponseCreate(usMid, &pResponse);
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

    pResponseHeader = (PREAD_ANDX_RESPONSE_HEADER) pResponsePacket->pParams;
    
    // byte order conversions
    SMB_LTOH16_INPLACE(pResponseHeader->dataLength);
    SMB_LTOH16_INPLACE(pResponseHeader->dataOffset);
    SMB_LTOH16_INPLACE(pResponseHeader->dataLengthHigh);

    if (pResponseHeader->dataLength)
    {
        usBytesRead = pResponseHeader->dataLength;
        
        if (usBytesRead > usReadLen)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        
        // TODO -- VERIFY PACKET LENGTH!!!!
#if 0
        if (pResponseHeader->dataOffset + usBytesRead > pResponsePacket->pNetBIOSHeader->len)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
#endif
        
        memcpy(pReadBuffer,
               (uint8_t*)pResponsePacket->pSMBHeader + pResponseHeader->dataOffset,
               usBytesRead);
    }

    *pusBytesRead = usBytesRead;

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

    *pusBytesRead = 0;

    goto cleanup;
}
