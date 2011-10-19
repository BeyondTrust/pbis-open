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
RdrTransactRenameFile(
    PSMB_TREE pTree,
    USHORT usSearchAttributes,
    PCWSTR pwszSourceFile,
    PCWSTR pwszDestFile
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    PSMB_RENAME_REQUEST_HEADER pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    PBYTE pCursor = NULL;
    ULONG ulSourceFileLength = 0;
    ULONG ulDestFileLength = 0;

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
        COM_RENAME,
        0,
        0,
        pTree->tid,
        gRdrRuntime.SysPid,
        pTree->pSession->uid,
        usMid,
        TRUE,
        &packet);
    BAIL_ON_NT_STATUS(ntStatus);
    
    packet.pData = packet.pParams + sizeof(SMB_RENAME_REQUEST_HEADER);

    packet.bufferUsed += sizeof(SMB_RENAME_REQUEST_HEADER);
    packet.pSMBHeader->wordCount = 1;

    pHeader = (PSMB_RENAME_REQUEST_HEADER) packet.pParams;
    pHeader->usSearchAttributes = usSearchAttributes;

    pCursor = packet.pData;

    ulSourceFileLength = LwRtlWC16StringNumChars(pwszSourceFile);
    ulDestFileLength = LwRtlWC16StringNumChars(pwszDestFile);

    /* Old filename format */
    *(pCursor++) = 0x04;
    packet.bufferUsed += 1;
    /* Align old filename */
    if ((pCursor - (PBYTE) packet.pSMBHeader) % 2)
    {
        pCursor++;
        packet.bufferUsed++;
    }
    /* Write old filename */
    SMB_HTOLWSTR(pCursor, pwszSourceFile, ulSourceFileLength);
    pCursor += (ulSourceFileLength + 1) * sizeof(WCHAR);
    packet.bufferUsed += (ulSourceFileLength + 1) * sizeof(WCHAR);

    /* New filename format */
    *(pCursor++) = 0x04;
    packet.bufferUsed += 1;
    /* Align new filename */
    if ((pCursor - (PBYTE) packet.pSMBHeader) % 2)
    {
        pCursor++;
        packet.bufferUsed++;
    }
    /* Write new filename */
    SMB_HTOLWSTR(pCursor, pwszDestFile, ulDestFileLength);
    pCursor += (ulDestFileLength + 1) * sizeof(WCHAR);
    packet.bufferUsed += (ulDestFileLength + 1) * sizeof(WCHAR);
    
    pHeader->usByteCount = (USHORT) (pCursor - packet.pData);

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

NTSTATUS
RdrTransactNTRenameFile(
    PSMB_TREE pTree,
    USHORT usSearchAttributes,
    USHORT usInfoLevel,
    ULONG ulClusterCount,
    PCWSTR pwszSourceFile,
    PCWSTR pwszDestFile
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    PSMB_NT_RENAME_REQUEST_HEADER pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    PBYTE pCursor = NULL;
    ULONG ulSourceFileLength = 0;
    ULONG ulDestFileLength = 0;

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
        COM_NT_RENAME,
        0,
        0,
        pTree->tid,
        gRdrRuntime.SysPid,
        pTree->pSession->uid,
        usMid,
        TRUE,
        &packet);
    BAIL_ON_NT_STATUS(ntStatus);
    
    packet.pData = packet.pParams + sizeof(SMB_NT_RENAME_REQUEST_HEADER);

    packet.bufferUsed += sizeof(SMB_NT_RENAME_REQUEST_HEADER);
    packet.pSMBHeader->wordCount = 4;

    pHeader = (PSMB_NT_RENAME_REQUEST_HEADER) packet.pParams;
    pHeader->usSearchAttributes = usSearchAttributes;
    pHeader->usInfoLevel = usInfoLevel;
    pHeader->ulClusterCount = ulClusterCount;

    pCursor = packet.pData;

    ulSourceFileLength = LwRtlWC16StringNumChars(pwszSourceFile);
    ulDestFileLength = LwRtlWC16StringNumChars(pwszDestFile);

    /* Old filename format */
    *(pCursor++) = 0x04;
    packet.bufferUsed += 1;
    /* Align old filename */
    if ((pCursor - (PBYTE) packet.pSMBHeader) % 2)
    {
        pCursor++;
        packet.bufferUsed++;
    }
    /* Write old filename */
    SMB_HTOLWSTR(pCursor, pwszSourceFile, ulSourceFileLength);
    pCursor += (ulSourceFileLength + 1) * sizeof(WCHAR);
    packet.bufferUsed += (ulSourceFileLength + 1) * sizeof(WCHAR);

    /* New filename format */
    *(pCursor++) = 0x04;
    packet.bufferUsed += 1;
    /* Align new filename */
    if ((pCursor - (PBYTE) packet.pSMBHeader) % 2)
    {
        pCursor++;
        packet.bufferUsed++;
    }
    /* Write new filename */
    SMB_HTOLWSTR(pCursor, pwszDestFile, ulDestFileLength);
    pCursor += (ulDestFileLength + 1) * sizeof(WCHAR);
    packet.bufferUsed += (ulDestFileLength + 1) * sizeof(WCHAR);
    
    pHeader->usByteCount = (USHORT) (pCursor - packet.pData);

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

NTSTATUS
RdrTransactTrans2RenameFile(
    PSMB_TREE pTree,
    USHORT usFid,
    USHORT usFlags,
    PCWSTR pwszPath
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    NT_TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    USHORT usSetup[0];
    SMB_TRANSACT_RENAME_HEADER renameHeader = {0};
    USHORT usSetHeaderOffset = 0;
    USHORT usSetDataOffset = 0;
    ULONG ulPathLength = LwRtlWC16StringNumChars(pwszPath);
    PBYTE pCursor = NULL;

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
        COM_NT_TRANSACT,
        0,
        0,
        pTree->tid,
        gRdrRuntime.SysPid,
        pTree->pSession->uid,
        usMid,
        TRUE,
        &packet);
    BAIL_ON_NT_STATUS(ntStatus);
    
    packet.pData = packet.pParams + sizeof(NT_TRANSACTION_REQUEST_HEADER);
    packet.bufferUsed += sizeof(NT_TRANSACTION_REQUEST_HEADER);
    packet.pSMBHeader->wordCount = 19 + sizeof(usSetup)/sizeof(USHORT);

    pHeader = (NT_TRANSACTION_REQUEST_HEADER *) packet.pParams;

    renameHeader.usFid = usFid;
    renameHeader.usFlags = usFlags;

    ntStatus = WireMarshallTransactionRequestData(
        packet.pData,
        packet.bufferLen - packet.bufferUsed,
        &packetByteCount,
        usSetup, 
        sizeof(usSetup)/sizeof(USHORT),
        NULL,
        (PBYTE) &renameHeader,
        sizeof(renameHeader) + (ulPathLength + 1) * sizeof(WCHAR),
        &usSetHeaderOffset,
        NULL,
        0,
        &usSetDataOffset);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(packetByteCount <= UINT16_MAX);
    packet.bufferUsed += packetByteCount;

    pCursor = packet.pData + usSetHeaderOffset + sizeof(renameHeader);

    if ((pCursor - (PBYTE) packet.pSMBHeader) % 2)
    {
        pCursor++;
    }

    SMB_HTOLWSTR(pCursor, pwszPath, ulPathLength);

    pHeader->usFunction = SMB_SUB_COMMAND_NT_TRANSACT_RENAME;
    pHeader->ulTotalParameterCount = sizeof(renameHeader) + (ulPathLength + 1) * sizeof(WCHAR);
    pHeader->ulTotalDataCount = 0;
    pHeader->ulMaxParameterCount = sizeof(renameHeader);
    pHeader->ulMaxDataCount = 0;
    pHeader->ucMaxSetupCount = sizeof(usSetup)/sizeof(USHORT);
    pHeader->ulParameterCount = sizeof(renameHeader) + (ulPathLength + 1) * sizeof(WCHAR);
    pHeader->ulParameterOffset = usSetHeaderOffset + (packet.pData - (PBYTE) packet.pSMBHeader);
    pHeader->ulDataCount = 0;
    pHeader->ulDataOffset = 0;
    pHeader->ucSetupCount = sizeof(usSetup)/sizeof(USHORT);

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
