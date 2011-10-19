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

NTSTATUS
RdrTransactFindNext2(
    PSMB_TREE pTree,
    USHORT usSearchId,
    USHORT usSearchCount,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulResumeKey,
    USHORT usFlags,
    PWSTR pwszFileName,
    PUSHORT pusSearchCount,
    PUSHORT pusEndOfSearch,
    PUSHORT pusEaErrorOffset,
    PUSHORT pusLastNameOffset,
    PVOID pResult,
    ULONG ulResultLength,
    PULONG pulResultLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    TRANSACTION_SECONDARY_RESPONSE_HEADER *pResponseHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_FIND_NEXT2;
    PSMB_FIND_NEXT2_REQUEST_PARAMETERS pFindParameters = NULL;
    USHORT usFindParametersLength = 0;
    USHORT usFindParametersOffset = 0;
    USHORT usFindDataOffset = 0;
    ULONG ulOffset = 0;
    PUSHORT pusReplySetup = NULL;
    PUSHORT pusReplyByteCount = NULL;
    PSMB_FIND_NEXT2_RESPONSE_PARAMETERS pReplyParameters = NULL;
    PBYTE pReplyData = NULL;
    USHORT usReplyByteCount = 0;
    USHORT usReplyDataCount = 0;

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
    
    packet.pData = packet.pParams + sizeof(TRANSACTION_REQUEST_HEADER);
    packet.bufferUsed += sizeof(TRANSACTION_REQUEST_HEADER);
    packet.pSMBHeader->wordCount = 14 + sizeof(usSetup)/sizeof(USHORT);

    pHeader = (TRANSACTION_REQUEST_HEADER *) packet.pParams;
    
    usFindParametersLength = 
        sizeof(*pFindParameters) + 
        (LwRtlWC16StringNumChars(pwszFileName) + 1) * sizeof(WCHAR);

    ntStatus = RTL_ALLOCATE(&pFindParameters,
                            SMB_FIND_NEXT2_REQUEST_PARAMETERS,
                            usFindParametersLength);
    BAIL_ON_NT_STATUS(ntStatus);
    
    pFindParameters->usSearchId          = SMB_HTOL16(usSearchId);
    pFindParameters->usSearchCount       = SMB_HTOL16(usSearchCount);
    pFindParameters->infoLevel           = SMB_HTOL16(infoLevel);
    pFindParameters->ulResumeKey         = SMB_HTOL32(ulResumeKey);
    pFindParameters->usFlags             = SMB_HTOL16(usFlags);

    if (pwszFileName)
    {
        SMB_HTOLWSTR(pFindParameters->pwszFileName,
                     pwszFileName,
                     LwRtlWC16StringNumChars(pwszFileName));
    }

    ntStatus = WireMarshallTransactionRequestData(
        packet.pData,
        packet.bufferLen - packet.bufferUsed,
        &packetByteCount,
        &usSetup, 
        sizeof(usSetup)/sizeof(USHORT),
        NULL,
        (PBYTE) pFindParameters,
        usFindParametersLength,
        &usFindParametersOffset,
        NULL,
        0,
        &usFindDataOffset);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(packetByteCount <= UINT16_MAX);
    packet.bufferUsed += packetByteCount;

    pHeader->totalParameterCount = SMB_HTOL16(usFindParametersLength);
    pHeader->totalDataCount      = SMB_HTOL16(0);
    pHeader->maxParameterCount   = SMB_HTOL16(10 * sizeof(USHORT));
    pHeader->maxDataCount        = SMB_HTOL16((USHORT) ulResultLength);
    pHeader->maxSetupCount       = SMB_HTOL8(sizeof(usSetup)/sizeof(USHORT));
    pHeader->flags               = SMB_HTOL16(0);
    pHeader->timeout             = SMB_HTOL32(0);
    pHeader->parameterCount      = SMB_HTOL16(usFindParametersLength);
    pHeader->parameterOffset     = SMB_HTOL16(usFindParametersOffset + (packet.pData - (PBYTE) packet.pSMBHeader));
    pHeader->dataCount           = SMB_HTOL16(0);
    pHeader->dataOffset          = SMB_HTOL16(usFindDataOffset + (packet.pData - (PBYTE) packet.pSMBHeader));
    pHeader->setupCount          = SMB_HTOL8(sizeof(usSetup)/sizeof(USHORT));

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

    ulOffset = (PBYTE)pResponsePacket->pParams - (PBYTE)pResponsePacket->pSMBHeader;
    
    ntStatus = WireUnmarshallTransactionSecondaryResponse(
        pResponsePacket->pParams,
        pResponsePacket->pNetBIOSHeader->len - ulOffset,
        ulOffset,
        &pResponseHeader,
        &pusReplySetup,
        &pusReplyByteCount,
        NULL,
        (PBYTE*) (void*) &pReplyParameters,
        &pReplyData,
        0);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Unmarshal reply byte count */
    ntStatus = UnmarshalUshort((PBYTE*) &pusReplyByteCount, NULL, &usReplyByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    usReplyDataCount = SMB_LTOH16(pResponseHeader->dataCount);

    if (usReplyByteCount > (pResponsePacket->bufferUsed - ((PBYTE) pusReplyByteCount  - (PBYTE) pResponsePacket->pRawBuffer) - sizeof(USHORT)) ||
        usReplyDataCount > (pResponsePacket->bufferUsed - ((PBYTE) pReplyData - (PBYTE) pResponsePacket->pRawBuffer)))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (usReplyDataCount > ulResultLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pResult, pReplyData, usReplyDataCount);
    if (pulResultLengthUsed) *pulResultLengthUsed = usReplyDataCount;
    if (pusSearchCount) *pusSearchCount = SMB_LTOH16(pReplyParameters->usSearchCount);
    if (pusEndOfSearch) *pusEndOfSearch = SMB_LTOH16(pReplyParameters->usEndOfSearch);
    if (pusEaErrorOffset) *pusEaErrorOffset = SMB_LTOH16(pReplyParameters->usEaErrorOffset);
    if (pusLastNameOffset) *pusLastNameOffset = SMB_LTOH16(pReplyParameters->usLastNameOffset);

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

    RTL_FREE(&pFindParameters);

    return ntStatus;

error:

    goto cleanup;
}
