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

#include "rdr.h"

static
NTSTATUS
RdrTransceiveQueryFsInfo(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulInfoLength
    );

static
BOOLEAN
RdrQueryFsInfoComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrUnmarshalQueryFsInfoReply(
    ULONG ulInfoLevel,
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

static
VOID
RdrCancelQueryFsInfo(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrQueryVolumeInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB pFile = NULL;
    SMB_INFO_LEVEL infoLevel = 0;

    switch(pIrp->Args.QuerySetVolumeInformation.FsInformationClass)
    {
    case FileFsSizeInformation:
        infoLevel = SMB_INFO_ALLOCATION;
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(status);
        break;
    }

    pFile = IoFileGetContext(pIrp->FileHandle);

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelQueryFsInfo, pContext);

    pContext->Continue = RdrQueryFsInfoComplete;

    status = RdrTransceiveQueryFsInfo(
        pContext,
        pFile->pTree,
        infoLevel,
        pIrp->Args.QuerySetVolumeInformation.Length);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
        RdrFreeContext(pContext);
        status = STATUS_PENDING;
    }

    return status;

error:

    goto cleanup;
}


static
NTSTATUS
RdrTransceiveQueryFsInfo(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulInfoLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_QUERY_FS_INFORMATION;
    SMB_QUERY_FS_INFO_HEADER queryHeader = {0};
    PBYTE pRequestParameters = NULL;
    PBYTE pCursor = NULL;
    PBYTE pByteCount = NULL;
    ULONG ulRemainingSpace = 0;

    status = RdrAllocateContextPacket(pContext, 1024*64);
    BAIL_ON_NT_STATUS(status);
    
    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_TRANSACTION2,
        0,
        0,
        pTree->tid,
        gRdrRuntime.SysPid,
        pTree->pSession->uid,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pCursor = pContext->Packet.pParams;
    ulRemainingSpace = pContext->Packet.bufferLen - (pCursor - pContext->Packet.pRawBuffer);

    status = WireMarshalTrans2RequestSetup(
        pContext->Packet.pSMBHeader,
        &pCursor,
        &ulRemainingSpace,
        &usSetup,
        1,
        &pHeader,
        &pByteCount);
    BAIL_ON_NT_STATUS(status);

    /* Remember start of the trans2 parameter block */
    pRequestParameters = pCursor;

    /* Write parameters */
    queryHeader.infoLevel = SMB_HTOL16(infoLevel);

    status = MarshalData(&pCursor, &ulRemainingSpace, (PBYTE) &queryHeader, sizeof(queryHeader));
    BAIL_ON_NT_STATUS(status);
    
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
    pHeader->parameterOffset     = SMB_HTOL16(pRequestParameters - (PBYTE) pContext->Packet.pSMBHeader);
    pHeader->dataCount           = SMB_HTOL16(0);
    pHeader->dataOffset          = SMB_HTOL16(0);
    pHeader->setupCount          = SMB_HTOL8(1);

    /* Update byte count */
    status = MarshalUshort(&pByteCount, NULL, (pCursor - pByteCount) - 2);

    /* Update used length */
    pContext->Packet.bufferUsed += (pCursor - pContext->Packet.pParams);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);
    
    status = RdrSocketTransceive(pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);
    
cleanup:

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrQueryFsInfoComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pResponsePacket = pParam;
    PBYTE pCursor = NULL;
    PBYTE pReplyData = NULL;
    USHORT usReplyDataCount = 0;
    ULONG ulRemainingSpace = 0;

    BAIL_ON_NT_STATUS(status);
    
    status = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

    pCursor = pResponsePacket->pParams;
    ulRemainingSpace = pResponsePacket->pNetBIOSHeader->len -
        ((PBYTE)pResponsePacket->pParams - (PBYTE)pResponsePacket->pSMBHeader);
    
    status = WireUnmarshalTrans2ReplySetup(
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
    BAIL_ON_NT_STATUS(status);
    
    status = RdrUnmarshalQueryFsInfoReply(
        pContext->pIrp->Args.QuerySetVolumeInformation.FsInformationClass,
        pReplyData,
        usReplyDataCount,
        pContext->pIrp->Args.QuerySetVolumeInformation.FsInformation,
        pContext->pIrp->Args.QuerySetVolumeInformation.Length,
        &pContext->pIrp->IoStatusBlock.BytesTransferred);

cleanup:

    RdrFreePacket(pResponsePacket);

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}
   
static
NTSTATUS
RdrUnmarshalQueryFsInfoReply(
    ULONG ulInfoLevel,
    PBYTE pData,
    USHORT usDataCount,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    switch (ulInfoLevel)
    {
    case FileFsSizeInformation:
        status = RdrUnmarshalQueryFsAllocationReply(
            pData,
            usDataCount,
            pInfo,
            ulInfoLength,
            pulInfoLengthUsed);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(status);
        break;
    }

error:

    return status;
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
    NTSTATUS status = STATUS_SUCCESS;
    PSMB_FS_INFO_ALLOCATION pAllocInfoPacked = NULL;

    pAllocInfoPacked = (PSMB_FS_INFO_ALLOCATION) pData;

    if (usDataCount != sizeof(*pAllocInfoPacked))
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }
    
    if (ulInfoLength < sizeof(*pAllocInfo))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }
    
    pAllocInfo->TotalAllocationUnits = SMB_LTOH32(pAllocInfoPacked->ulNumAllocationUnits);
    pAllocInfo->AvailableAllocationUnits = SMB_LTOH32(pAllocInfoPacked->ulNumUnitsAvailable);
    pAllocInfo->SectorsPerAllocationUnit = SMB_LTOH32(pAllocInfoPacked->ulNumSectorsPerAllocationUnit);
    pAllocInfo->BytesPerSector = SMB_LTOH16(pAllocInfoPacked->usNumBytesPerSector);

    *pulInfoLengthUsed = sizeof(*pAllocInfo);

error:
    
    return status;
}
