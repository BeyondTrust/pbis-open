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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (RDR)
 *
 *        Driver Entry Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

static
BOOLEAN
RdrQueryInfoFileComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrUnmarshalQueryFileBasicInfoReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_BASIC_INFORMATION pBasicInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

static
NTSTATUS
RdrUnmarshalQueryFileStandardInfoReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_STANDARD_INFORMATION pStandardInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

static
NTSTATUS
RdrTransceiveQueryInfoFile(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulInfoLength
    );

static
VOID
RdrCancelQueryInfo(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrQueryInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB pFile = NULL;
    SMB_INFO_LEVEL infoLevel = 0;

    pFile = IoFileGetContext(pIrp->FileHandle);

    if (!pFile->fid)
    {
        status = STATUS_ACCESS_VIOLATION;
        BAIL_ON_NT_STATUS(status);
    }

    switch (pIrp->Args.QuerySetInformation.FileInformationClass)
    {
    case FileBasicInformation:
        infoLevel = SMB_QUERY_FILE_BASIC_INFO;
        break;
    case FileStandardInformation:
        infoLevel = SMB_QUERY_FILE_STANDARD_INFO;
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(status);
        break;
    }

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelQueryInfo, pContext);

    pContext->Continue = RdrQueryInfoFileComplete;

    status = RdrTransceiveQueryInfoFile(
        pContext,
        pFile,
        infoLevel,
        pIrp->Args.QuerySetInformation.Length);
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

/* FIXME: This function assumes that both the request and response will
   fit in a single packet.  This is probably true in practice, but in
   theory we need to handle multi-packet transactions */
static
NTSTATUS
RdrTransceiveQueryInfoFile(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulInfoLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    static USHORT usSetup = SMB_SUB_COMMAND_TRANS2_QUERY_FILE_INFORMATION;
    SMB_QUERY_FILE_INFO_HEADER queryHeader = {0};
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
        pFile->pTree->tid,
        gRdrRuntime.SysPid,
        pFile->pTree->pSession->uid,
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
    queryHeader.usFid = SMB_HTOL16(pFile->fid);
    queryHeader.infoLevel = SMB_HTOL16(infoLevel);

    status = MarshalData(&pCursor, &ulRemainingSpace, (PBYTE) &queryHeader, sizeof(queryHeader));
    BAIL_ON_NT_STATUS(status);
    
    /* The cursor now points exactly past the end of the packet */
    
    /* Update fields in trans request header */
    pHeader->totalParameterCount = SMB_HTOL16(sizeof(queryHeader));
    pHeader->totalDataCount      = SMB_HTOL16(0);
    pHeader->maxParameterCount   = SMB_HTOL16(sizeof(USHORT)); /* Reply parameters consist of a USHORT */
    pHeader->maxDataCount        = SMB_HTOL16(ulInfoLength + 100);   /* FIXME: magic number */
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
    
    status = RdrSocketTransceive(pFile->pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}
    
static
BOOLEAN
RdrQueryInfoFileComplete(
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
    
    status = RdrUnmarshalQueryFileInfoReply(
        pContext->pIrp->Args.QuerySetInformation.FileInformationClass,
        pReplyData,
        usReplyDataCount,
        pContext->pIrp->Args.QuerySetInformation.FileInformation,
        pContext->pIrp->Args.QuerySetInformation.Length,
        &pContext->pIrp->IoStatusBlock.BytesTransferred);
    BAIL_ON_NT_STATUS(status);

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

NTSTATUS
RdrTransceiveQueryInfoPath(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    PCWSTR pwszPath,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulInfoLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    static USHORT usSetup = SMB_SUB_COMMAND_TRANS2_QUERY_PATH_INFORMATION;
    SMB_QUERY_PATH_INFO_HEADER queryHeader = {0};
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

    if (pTree->usSupportFlags & SMB_SHARE_IS_IN_DFS)
    {
        pContext->Packet.pSMBHeader->flags2 |= FLAG2_DFS;
    }

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
    queryHeader.ulReserved = 0;
    queryHeader.infoLevel = SMB_HTOL16(infoLevel);

    status = MarshalData(&pCursor, &ulRemainingSpace, (PBYTE) &queryHeader, sizeof(queryHeader));
    BAIL_ON_NT_STATUS(status);

    status = Align((PBYTE) pContext->Packet.pSMBHeader, &pCursor, &ulRemainingSpace, sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    status = MarshalPwstr(&pCursor, &ulRemainingSpace, pwszPath, -1);
    BAIL_ON_NT_STATUS(status);

    /* The cursor now points exactly past the end of the packet */

    /* Update fields in trans request header */
    pHeader->totalParameterCount = SMB_HTOL16((USHORT) (pCursor - pRequestParameters));
    pHeader->totalDataCount      = SMB_HTOL16(0);
    pHeader->maxParameterCount   = SMB_HTOL16(sizeof(USHORT)); /* Reply parameters consist of a USHORT */
    pHeader->maxDataCount        = SMB_HTOL16(ulInfoLength + 100);   /* FIXME: magic number */
    pHeader->maxSetupCount       = SMB_HTOL8(1);
    pHeader->flags               = SMB_HTOL16(0);
    pHeader->timeout             = SMB_HTOL32(0);
    pHeader->parameterCount      = SMB_HTOL16((USHORT) (pCursor - pRequestParameters));
    pHeader->parameterOffset     = SMB_HTOL16((USHORT) (pRequestParameters - (PBYTE) pContext->Packet.pSMBHeader));
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

NTSTATUS
RdrUnmarshalQueryFileInfoReply(
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
    case FileBasicInformation:
        status = RdrUnmarshalQueryFileBasicInfoReply(
            pData,
            usDataCount,
            pInfo,
            ulInfoLength,
            pulInfoLengthUsed);
        BAIL_ON_NT_STATUS(status);
        break;
    case FileStandardInformation:
        status = RdrUnmarshalQueryFileStandardInfoReply(
            pData,
            usDataCount,
            pInfo,
            ulInfoLength,
            pulInfoLengthUsed);
        BAIL_ON_NT_STATUS(status);
        break;
    }

error:

    return status;
}

    
static
NTSTATUS
RdrUnmarshalQueryFileBasicInfoReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_BASIC_INFORMATION pBasicInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PTRANS2_FILE_BASIC_INFORMATION pBasicInfoPacked = NULL;

    pBasicInfoPacked = (PTRANS2_FILE_BASIC_INFORMATION) pData;

    if (usDataCount != sizeof(*pBasicInfoPacked))
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }
    
    if (ulInfoLength < sizeof(*pBasicInfo))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }
    
    pBasicInfo->ChangeTime = pBasicInfoPacked->ChangeTime;
    pBasicInfo->FileAttributes = pBasicInfoPacked->FileAttributes;
    pBasicInfo->LastAccessTime = pBasicInfoPacked->LastAccessTime;
    pBasicInfo->LastWriteTime = pBasicInfoPacked->LastWriteTime;

    *pulInfoLengthUsed = sizeof(*pBasicInfo);

error:
    
    return status;
}

static
NTSTATUS
RdrUnmarshalQueryFileStandardInfoReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_STANDARD_INFORMATION pStandardInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PTRANS2_FILE_STANDARD_INFORMATION pStandardInfoPacked = NULL;
    
    pStandardInfoPacked = (PTRANS2_FILE_STANDARD_INFORMATION) pData;
    
    if (usDataCount != sizeof(*pStandardInfoPacked))
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }
    
    if (ulInfoLength < sizeof(*pStandardInfo))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }
    
    pStandardInfo->AllocationSize = pStandardInfoPacked->AllocationSize;
    pStandardInfo->EndOfFile = pStandardInfoPacked->EndOfFile;
    pStandardInfo->NumberOfLinks = pStandardInfoPacked->NumberOfLinks;
    pStandardInfo->DeletePending = pStandardInfoPacked->bDeletePending;
    pStandardInfo->Directory = pStandardInfoPacked->bDirectory;

    *pulInfoLengthUsed = sizeof(*pStandardInfo);

error:
    
    return status;
}
