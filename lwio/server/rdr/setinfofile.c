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
RdrMarshalFileInfo(
    PSMB_HEADER pSmbHeader,
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength
    );

static
NTSTATUS
RdrMarshalFileEndOfFileInfo(
    PSMB_HEADER pSmbHeader,
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo,
    ULONG ulInfoLength
    );

static
NTSTATUS
RdrMarshalFileDispositionInfo(
    PSMB_HEADER pSmbHeader,
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo,
    ULONG ulInfoLength
    );

/* FIXME: This function assumes that both the request and response will
   fit in a single packet.  This is probably true in practice, but in
   theory we need to handle multi-packet transactions */
NTSTATUS
RdrTransactSetInfoFile(
    PSMB_TREE pTree,
    USHORT usFid,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_SET_FILE_INFORMATION;
    SMB_SET_FILE_INFO_HEADER setHeader = {0};
    PBYTE pRequestParameters = NULL;
    PBYTE pRequestData = NULL;
    USHORT usRequestDataCount = 0;
    PBYTE pCursor = NULL;
    ULONG ulRemainingSpace = 0;
    PBYTE pByteCount = NULL;

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

    pRequestParameters = pCursor;

    setHeader.usFid = usFid;
    setHeader.infoLevel = infoLevel;

    ntStatus = MarshalData(&pCursor, &ulRemainingSpace, (PBYTE) &setHeader, sizeof(setHeader));
    BAIL_ON_NT_STATUS(ntStatus);

    pRequestData = pCursor;

    ntStatus = RdrMarshalFileInfo(
        packet.pSMBHeader,
        &pCursor,
        &ulRemainingSpace,
        infoLevel,
        pInfo,
        ulInfoLength);
    BAIL_ON_NT_STATUS(ntStatus);

    usRequestDataCount = pCursor - pRequestData;

    pHeader->totalParameterCount = SMB_HTOL16(sizeof(setHeader));
    pHeader->totalDataCount      = SMB_HTOL16(usRequestDataCount);
    pHeader->maxParameterCount   = SMB_HTOL16(sizeof(setHeader)); /* FIXME: really? */
    pHeader->maxDataCount        = SMB_HTOL16(0);
    pHeader->maxSetupCount       = SMB_HTOL16(0);
    pHeader->flags               = SMB_HTOL16(0);
    pHeader->timeout             = SMB_HTOL16(0);
    pHeader->parameterCount      = SMB_HTOL16(sizeof(setHeader));
    pHeader->parameterOffset     = SMB_HTOL16(pRequestParameters - (PBYTE) packet.pSMBHeader);
    pHeader->dataCount           = SMB_HTOL16(usRequestDataCount);
    pHeader->dataOffset          = SMB_HTOL16(pRequestData - (PBYTE) packet.pSMBHeader);
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
RdrMarshalFileInfo(
    PSMB_HEADER pSmbHeader,
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (infoLevel)
    {
    case SMB_SET_FILE_END_OF_FILE_INFO:
        ntStatus = RdrMarshalFileEndOfFileInfo(
            pSmbHeader,
            ppCursor,
            pulRemainingSpace,
            pInfo,
            ulInfoLength);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case SMB_SET_FILE_DISPOSITION_INFO:
        ntStatus = RdrMarshalFileDispositionInfo(
            pSmbHeader,
            ppCursor,
            pulRemainingSpace,
            pInfo,
            ulInfoLength);
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
RdrMarshalFileEndOfFileInfo(
    PSMB_HEADER pSmbHeader,
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo,
    ULONG ulInfoLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFILE_END_OF_FILE_INFORMATION pEndInfo = pInfo;
    PBYTE pCursor = *ppCursor;
    ULONG ulRemainingSpace = *pulRemainingSpace;
    PTRANS2_FILE_END_OF_FILE_INFORMATION pEndInfoPacked = (PTRANS2_FILE_END_OF_FILE_INFORMATION) *ppCursor;

    if (ulInfoLength < sizeof(*pEndInfo))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Advance cursor past info structure */
    ntStatus = Advance(&pCursor, &ulRemainingSpace, sizeof(*pEndInfoPacked));
    BAIL_ON_NT_STATUS(ntStatus);

    pEndInfoPacked->EndOfFile = SMB_HTOL64(pEndInfo->EndOfFile);

    *ppCursor = pCursor;
    *pulRemainingSpace = ulRemainingSpace;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
RdrMarshalFileDispositionInfo(
    PSMB_HEADER pSmbHeader,
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo,
    ULONG ulInfoLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pCursor = *ppCursor;
    ULONG ulRemainingSpace = *pulRemainingSpace;
    PFILE_DISPOSITION_INFORMATION pDispInfo = pInfo;
    PTRANS2_FILE_DISPOSITION_INFORMATION pDispInfoPacked = (PTRANS2_FILE_DISPOSITION_INFORMATION) pCursor;

    if (ulInfoLength < sizeof(*pDispInfo))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Advance cursor past info structure */
    ntStatus = Advance(&pCursor, &ulRemainingSpace, sizeof(*pDispInfoPacked));
    BAIL_ON_NT_STATUS(ntStatus);

    pDispInfoPacked->bFileIsDeleted = pDispInfo->DeleteFile;

    *ppCursor = pCursor;
    *pulRemainingSpace = ulRemainingSpace;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}
