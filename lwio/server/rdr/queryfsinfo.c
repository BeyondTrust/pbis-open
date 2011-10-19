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

static
NTSTATUS
RdrUnmarshalQueryFsInfoReply(
    SMB_INFO_LEVEL infoLevel,
    PBYTE pData,
    USHORT usDataCount,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );
    
static
NTSTATUS
RdrUnmarshalQueryFsAllocationReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_FS_SIZE_INFORMATION pAllocInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

/* FIXME: This function assumes that both the request and response will
   fit in a single packet.  This is probably true in practice, but in
   theory we need to handle multi-packet transactions */
NTSTATUS
RdrTransactQueryFsInfo(
    PSMB_TREE pTree,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_QUERY_FS_INFORMATION;
    SMB_QUERY_FS_INFO_HEADER queryHeader = {0};
    PBYTE pRequestParameters = NULL;
    PBYTE pReplyData = NULL;
    USHORT usReplyDataCount = 0;
    PBYTE pCursor = NULL;
    PBYTE pByteCount = NULL;
    ULONG ulRemainingSpace = 0;
    
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
        COM_TRANSACTION2,
        0,
        0,
        pTree->tid,
        gRdrRuntime.SysPid,
        pTree->pSession->uid,
        usMid,
        TRUE,
        &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    pCursor = packet.pParams;
    ulRemainingSpace = packet.bufferLen - (pCursor - packet.pRawBuffer);

    ntStatus = WireMarshalTrans2RequestSetup(
        packet.pSMBHeader,
        &pCursor,
        &ulRemainingSpace,
        &usSetup,
        1,
        &pHeader,
        &pByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Remember start of the trans2 parameter block */
    pRequestParameters = pCursor;

    /* Write parameters */
    queryHeader.infoLevel = SMB_HTOL16(infoLevel);

    ntStatus = MarshalData(&pCursor, &ulRemainingSpace, (PBYTE) &queryHeader, sizeof(queryHeader));
    BAIL_ON_NT_STATUS(ntStatus);
    
    /* The cursor now points exactly past the end of the packet */
    
    /* Update fields in trans request header */
    pHeader->totalParameterCount = SMB_HTOL16(sizeof(queryHeader));
    pHeader->totalDataCount      = SMB_HTOL16(0);
    pHeader->maxParameterCount   = SMB_HTOL16(sizeof(USHORT)); /* Reply parameters consist of a USHORT */
    pHeader->maxDataCount        = SMB_HTOL16(ulInfoLength + 100);   /* FIXME: magic value */
    pHeader->maxSetupCount       = SMB_HTOL8(1);
    pHeader->flags               = SMB_HTOL16(0);
    pHeader->timeout             = SMB_HTOL32(0);
    pHeader->parameterCount      = SMB_HTOL16(sizeof(queryHeader));
    pHeader->parameterOffset     = SMB_HTOL16(pRequestParameters - (PBYTE) packet.pSMBHeader);
    pHeader->dataCount           = SMB_HTOL16(0);
    pHeader->dataOffset          = SMB_HTOL16(0);
    pHeader->setupCount          = SMB_HTOL8(1);

    /* Update byte count */
    ntStatus = MarshalUshort(&pByteCount, NULL, (pCursor - pByteCount) - 2);

    /* Update used length */
    packet.bufferUsed += (pCursor - packet.pParams);

    ntStatus = SMBPacketMarshallFooter(&packet);
    BAIL_ON_NT_STATUS(ntStatus);
    
    ntStatus = SMBResponseCreate(usMid, &pResponse);
    BAIL_ON_NT_STATUS(ntStatus);
    
    ntStatus = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

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

    pCursor = pResponsePacket->pParams;
    ulRemainingSpace = pResponsePacket->pNetBIOSHeader->len -
        ((PBYTE)pResponsePacket->pParams - (PBYTE)pResponsePacket->pSMBHeader);

    ntStatus = WireUnmarshalTrans2ReplySetup(
        pResponsePacket->pSMBHeader,
        &pCursor,
        &ulRemainingSpace,
        NULL, /* ppResponseHeader */
        NULL, /* pusTotalParameterCount */
        NULL, /* pusTotalDataCount */
        NULL, /* ppusSetupWords */
        NULL, /* pusSetupWordCount */
        NULL, /* pusByteCount */
        NULL, /* pParameterBlock */
        NULL, /* pusParameterCount */
        &pReplyData,
        &usReplyDataCount);
    BAIL_ON_NT_STATUS(ntStatus);
    
    ntStatus = RdrUnmarshalQueryFsInfoReply(
        infoLevel,
        pReplyData,
        usReplyDataCount,
        pInfo,
        ulInfoLength,
        pulInfoLengthUsed);

cleanup:

    if (pResponsePacket)
    {
        SMBPacketRelease(
            pTree->pSession->pSocket->hPacketAllocator,
            pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(pTree->pSession->pSocket->hPacketAllocator,
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
    
static
NTSTATUS
RdrUnmarshalQueryFsInfoReply(
    SMB_INFO_LEVEL infoLevel,
    PBYTE pData,
    USHORT usDataCount,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    switch (infoLevel)
    {
    case SMB_INFO_ALLOCATION:
        ntStatus = RdrUnmarshalQueryFsAllocationReply(
            pData,
            usDataCount,
            pInfo,
            ulInfoLength,
            pulInfoLengthUsed);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

error:

    return ntStatus;
}

    
static
NTSTATUS
RdrUnmarshalQueryFsAllocationReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_FS_SIZE_INFORMATION pAllocInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_FS_INFO_ALLOCATION pAllocInfoPacked = NULL;

    pAllocInfoPacked = (PSMB_FS_INFO_ALLOCATION) pData;

    if (usDataCount != sizeof(*pAllocInfoPacked))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    
    if (ulInfoLength < sizeof(*pAllocInfo))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    
    pAllocInfo->TotalAllocationUnits = SMB_LTOH32(pAllocInfoPacked->ulNumAllocationUnits);
    pAllocInfo->AvailableAllocationUnits = SMB_LTOH32(pAllocInfoPacked->ulNumUnitsAvailable);
    pAllocInfo->SectorsPerAllocationUnit = SMB_LTOH32(pAllocInfoPacked->ulNumSectorsPerAllocationUnit);
    pAllocInfo->BytesPerSector = SMB_LTOH16(pAllocInfoPacked->usNumBytesPerSector);

    *pulInfoLengthUsed = sizeof(*pAllocInfo);

error:
    
    return ntStatus;
}
