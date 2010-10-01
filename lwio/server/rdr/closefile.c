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

void
RdrReleaseFile(
    PSMB_CLIENT_FILE_HANDLE pFile
    )
{
    if (pFile->pTree)
    {
        SMBTreeRelease(pFile->pTree);
    }
    
    if (pFile->pMutex)
    {
        pthread_mutex_destroy(pFile->pMutex);
    }
    
    RTL_FREE(&pFile->pwszPath);
    RTL_FREE(&pFile->find.pBuffer);
    
    LwIoFreeMemory(pFile);
}

NTSTATUS
RdrTransactCloseFile(
    PSMB_TREE pTree,
    USHORT usFid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    PCLOSE_REQUEST_HEADER pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;

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
        COM_CLOSE,
        0,
        0,
        pTree->tid,
        gRdrRuntime.SysPid,
        pTree->pSession->uid,
        usMid,
        TRUE,
        &packet);
    BAIL_ON_NT_STATUS(ntStatus);
    
    packet.pData = packet.pParams + sizeof(CLOSE_REQUEST_HEADER);

    packet.bufferUsed += sizeof(CLOSE_REQUEST_HEADER);

    packet.pSMBHeader->wordCount = 3;

    pHeader = (PCLOSE_REQUEST_HEADER) packet.pParams;

    pHeader->fid = SMB_HTOL16(usFid);
    pHeader->ulLastWriteTime = SMB_HTOL32(0);
    pHeader->byteCount = SMB_HTOL16(0);

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
