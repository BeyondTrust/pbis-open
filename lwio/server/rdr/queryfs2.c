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
 * Module Name:
 *
 *        queryfs2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 query filesystem
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveQueryFsInfo2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    FILE_INFORMATION_CLASS infoClass,
    ULONG ulInfoLength
    );

static
BOOLEAN
RdrQueryFsInfo2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrDecodeFsInformation2(
    ULONG ulInfoLevel,
    PBYTE pData,
    USHORT usDataCount,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

static
NTSTATUS
RdrDecodeFsSizeInformation2(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_FS_SIZE_INFORMATION pAllocInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

static
VOID
RdrCancelQueryFsInfo2(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrQueryVolumeInformation2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB2 pFile = NULL;
    ULONG ulInfoLength = 0;

    pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.QuerySetVolumeInformation.FsInformationClass)
    {
    case FileFsSizeInformation:
        ulInfoLength = sizeof(RDR_SMB2_FILE_FS_SIZE_INFORMATION);
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(status);
        break;
    }

    if (pFile->pTree->pSession->pSocket->ulMaxTransactSize < ulInfoLength)
    {
        status = STATUS_BUFFER_OVERFLOW;
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelQueryFsInfo2, pContext);

    pContext->Continue = RdrQueryFsInfo2Complete;

    status = RdrTransceiveQueryFsInfo2(
        pContext,
        pFile,
        pIrp->Args.QuerySetVolumeInformation.FsInformationClass,
        ulInfoLength);
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
RdrTransceiveQueryFsInfo2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    FILE_INFORMATION_CLASS infoClass,
    ULONG ulInfoLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_QUERY_INFO_SIZE(0));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_GETINFO,
        0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeQueryInfoRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        SMB2_INFO_TYPE_FILE_SYSTEM,
        (UCHAR) infoClass,
        ulInfoLength,
        0, /* additional info */
        0, /* flags */
        &pFile->Fid, /* fid */
        NULL); /* input buffer length */
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2FinishCommand(&pContext->Packet, &pCursor, &ulRemaining);
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
RdrQueryFsInfo2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PBYTE pOutput = NULL;
    ULONG ulOutputSize = 0;

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMB2Header->error;
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2DecodeQueryInfoResponse(
        pPacket,
        &pOutput,
        &ulOutputSize);
    BAIL_ON_NT_STATUS(status);

    status = RdrDecodeFsInformation2(
        pContext->pIrp->Args.QuerySetVolumeInformation.FsInformationClass,
        pOutput,
        (USHORT) ulOutputSize,
        pContext->pIrp->Args.QuerySetVolumeInformation.FsInformation,
        pContext->pIrp->Args.QuerySetVolumeInformation.Length,
        &pContext->pIrp->IoStatusBlock.BytesTransferred);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RdrFreePacket(pPacket);

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
RdrDecodeFsInformation2(
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
        status = RdrDecodeFsSizeInformation2(
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
RdrDecodeFsSizeInformation2(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_FS_SIZE_INFORMATION pAllocInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_FILE_FS_SIZE_INFORMATION pAllocInfoPacked = NULL;

    pAllocInfoPacked = (PRDR_SMB2_FILE_FS_SIZE_INFORMATION) pData;

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

    pAllocInfo->TotalAllocationUnits = SMB_LTOH64(pAllocInfoPacked->llTotalAllocationUnits);
    pAllocInfo->AvailableAllocationUnits = SMB_LTOH64(pAllocInfoPacked->llAvailableAllocationUnits);
    pAllocInfo->SectorsPerAllocationUnit = SMB_LTOH32(pAllocInfoPacked->ulSectorsPerAllocationUnit);
    pAllocInfo->BytesPerSector = SMB_LTOH32(pAllocInfoPacked->ulBytesPerSector);

    *pulInfoLengthUsed = sizeof(*pAllocInfo);

error:

    return status;
}
