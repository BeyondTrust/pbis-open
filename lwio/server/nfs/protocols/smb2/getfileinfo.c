/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        getfileinfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Query File Information
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsGetFileInternalInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileInternalInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileEAInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileEAInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileBasicInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileBasicInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileStandardInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileStandardInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileNetworkOpenInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileNetworkOpenInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileAccessInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileAccessInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFilePositionInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFilePositionInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileModeInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileModeInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileAllInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileAllInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileAlignmentInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileAlignmentInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileAltNameInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileAltNameInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsMarshallFileNameInfo_SMB_V2(
    PLWIO_NFS_TREE_2 pTree,
    PBYTE            pInfoBuffer,
    USHORT           usBytesAvailable,
    PBYTE*           ppData,
    PUSHORT          pusDataLen
    );

static
NTSTATUS
NfsGetFileAttrTagInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileAttrTagInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileStreamInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileStreamInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsMarshallFileStreamResponse(
    PBYTE  pOutBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulDataLength,
    PULONG pulAlignBytes,
    PULONG pulBytesUsed
    );

static
NTSTATUS
NfsGetFileFullEAInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileFullEAInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsMarshallFileFullEAResponse(
    PBYTE  pOutBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulDataLength,
    PULONG pulAlignBytes,
    PULONG pulBytesUsed
    );

static
NTSTATUS
NfsGetFileCompressionInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildFileCompressionInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsGetFileInfoGeneric_SMB_V2(
    PNFS_EXEC_CONTEXT      pExecContext,
    ULONG                  ulStructSize,
    FILE_INFORMATION_CLASS infoClass
    );

static
NTSTATUS
NfsGetFileInfoGeneric_WithString_SMB_V2(
    PNFS_EXEC_CONTEXT      pExecContext,
    ULONG                  ulStructSize,
    FILE_INFORMATION_CLASS infoClass
    );

NTSTATUS
NfsGetFileInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_INTERNAL :

            ntStatus = NfsGetFileInternalInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_EA :

            ntStatus = NfsGetFileEAInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = NfsGetFileBasicInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_STANDARD :

            ntStatus = NfsGetFileStandardInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_NETWORK_OPEN :

            if (NfsTree2IsNamedPipe(pCtxSmb2->pTree))
            {
                ntStatus = STATUS_INVALID_PARAMETER;
            }
            else
            {
                ntStatus = NfsGetFileNetworkOpenInfo_SMB_V2(pExecContext);
            }

            break;

        case SMB2_FILE_INFO_CLASS_ACCESS :

            ntStatus = NfsGetFileAccessInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_POSITION :

            ntStatus = NfsGetFilePositionInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_MODE :

            ntStatus = NfsGetFileModeInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALL :

            ntStatus = NfsGetFileAllInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALIGNMENT :

            ntStatus = NfsGetFileAlignmentInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALTERNATE_NAME :

            ntStatus = NfsGetFileAltNameInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ATTRIBUTE_TAG :

            ntStatus = NfsGetFileAttrTagInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_STREAM :

            ntStatus = NfsGetFileStreamInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_FULL_EA :

            ntStatus = NfsGetFileFullEAInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_COMPRESSION :

            ntStatus = NfsGetFileCompressionInfo_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

NTSTATUS
NfsBuildFileInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_INTERNAL :

            ntStatus = NfsBuildFileInternalInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_EA :

            ntStatus = NfsBuildFileEAInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = NfsBuildFileBasicInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_STANDARD :

            ntStatus = NfsBuildFileStandardInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_NETWORK_OPEN :

            ntStatus = NfsBuildFileNetworkOpenInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ACCESS :

            ntStatus = NfsBuildFileAccessInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_POSITION :

            ntStatus = NfsBuildFilePositionInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_MODE :

            ntStatus = NfsBuildFileModeInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALL :

            ntStatus = NfsBuildFileAllInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALIGNMENT :

            ntStatus = NfsBuildFileAlignmentInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALTERNATE_NAME :

            ntStatus = NfsBuildFileAltNameInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ATTRIBUTE_TAG :

            ntStatus = NfsBuildFileAttrTagInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_STREAM :

            ntStatus = NfsBuildFileStreamInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_FULL_EA :

            ntStatus = NfsBuildFileFullEAInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_COMPRESSION :

            ntStatus = NfsBuildFileCompressionInfoResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
NfsGetFileInternalInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_INTERNAL_INFORMATION),
                    FileInternalInformation);
}

static
NTSTATUS
NfsBuildFileInternalInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_INTERNAL_INFORMATION pFileInternalInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FILE_INTERNAL_INFO_HEADER pFileInternalInfoHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    pFileInternalInfo = (PFILE_INTERNAL_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FILE_INTERNAL_INFO_HEADER);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileInternalInfoHeader = (PSMB_FILE_INTERNAL_INFO_HEADER)pOutBuffer;

    // ulTotalBytesUsed += sizeof(SMB_FILE_INTERNAL_INFO_HEADER);
    // ulBytesAvailable -= sizeof(SMB_FILE_INTERNAL_INFO_HEADER);
    // pOutBuffer += sizeof(SMB_FILE_INTERNAL_INFO_HEADER);

    pFileInternalInfoHeader->ullIndex = pFileInternalInfo->IndexNumber;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileEAInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_EA_INFORMATION),
                    FileEaInformation);
}

static
NTSTATUS
NfsBuildFileEAInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_EA_INFORMATION           pFileEAInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FILE_EA_INFO_HEADER       pFileEAInfoHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileEAInfo = (PFILE_EA_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FILE_EA_INFO_HEADER);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileEAInfoHeader = (PSMB_FILE_EA_INFO_HEADER)pOutBuffer;

    // ulTotalBytesUsed += sizeof(SMB_FILE_EA_INFO_HEADER);
    // ulBytesAvailable -= sizeof(SMB_FILE_EA_INFO_HEADER);
    // pOutBuffer += sizeof(SMB_FILE_EA_INFO_HEADER);

    pFileEAInfoHeader->ulEaSize = pFileEAInfo->EaSize;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileBasicInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_BASIC_INFORMATION),
                    FileBasicInformation);
}

static
NTSTATUS
NfsBuildFileBasicInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_BASIC_INFORMATION        pFileBasicInfo = NULL;
    SMB2_FILE_BASIC_INFORMATION    fileBasicInfoPacked = {0};
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileBasicInfo = (PFILE_BASIC_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    fileBasicInfoPacked.ChangeTime     = pFileBasicInfo->ChangeTime;
    fileBasicInfoPacked.CreationTime   = pFileBasicInfo->CreationTime;
    fileBasicInfoPacked.FileAttributes = pFileBasicInfo->FileAttributes;
    fileBasicInfoPacked.LastAccessTime = pFileBasicInfo->LastAccessTime;
    fileBasicInfoPacked.LastWriteTime  = pFileBasicInfo->LastWriteTime;

    pGetInfoResponseHeader->ulOutBufferLength =
                                        sizeof(SMB2_FILE_BASIC_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, &fileBasicInfoPacked,
                                        sizeof(SMB2_FILE_BASIC_INFORMATION));

    // pOutBuffer += sizeof(FILE_BASIC_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_BASIC_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileStandardInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_STANDARD_INFORMATION),
                    FileStandardInformation);
}

static
NTSTATUS
NfsBuildFileStandardInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_STANDARD_INFORMATION     pFileStandardInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileStandardInfo = (PFILE_STANDARD_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                            sizeof(FILE_STANDARD_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileStandardInfo, sizeof(FILE_STANDARD_INFORMATION));

    // pOutBuffer += sizeof(FILE_STANDARD_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_STANDARD_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileNetworkOpenInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_NETWORK_OPEN_INFORMATION),
                    FileNetworkOpenInformation);
}

static
NTSTATUS
NfsBuildFileNetworkOpenInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_NETWORK_OPEN_INFORMATION pFileNetworkOpenInfo = NULL;
    SMB2_FILE_NETWORK_OPEN_INFORMATION fileNetworkOpenInfoPacked = {0};
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileNetworkOpenInfo = (PFILE_NETWORK_OPEN_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(SMB2_FILE_NETWORK_OPEN_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    fileNetworkOpenInfoPacked.llChangeTime        =
                            pFileNetworkOpenInfo->ChangeTime;
    fileNetworkOpenInfoPacked.llCreationTime      =
                            pFileNetworkOpenInfo->CreationTime;
    fileNetworkOpenInfoPacked.llLastAccessTime    =
                            pFileNetworkOpenInfo->LastAccessTime;
    fileNetworkOpenInfoPacked.llLastWriteTime     =
                            pFileNetworkOpenInfo->LastWriteTime;
    fileNetworkOpenInfoPacked.ullAllocationSize =
                            pFileNetworkOpenInfo->AllocationSize;
    fileNetworkOpenInfoPacked.ullEndOfFile      =
                            pFileNetworkOpenInfo->EndOfFile;
    fileNetworkOpenInfoPacked.ulFileAttributes    =
                            pFileNetworkOpenInfo->FileAttributes;

    memcpy(pOutBuffer, &fileNetworkOpenInfoPacked,
                sizeof(SMB2_FILE_NETWORK_OPEN_INFORMATION));

    // pOutBuffer += sizeof(FILE_NETWORK_OPEN_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileAccessInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_ACCESS_INFORMATION),
                    FileAccessInformation);
}

static
NTSTATUS
NfsBuildFileAccessInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ACCESS_INFORMATION pFileAccessInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileAccessInfo = (PFILE_ACCESS_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(FILE_ACCESS_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileAccessInfo, sizeof(FILE_ACCESS_INFORMATION));

    // pOutBuffer += sizeof(FILE_ACCESS_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_ACCESS_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFilePositionInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_POSITION_INFORMATION),
                    FilePositionInformation);
}

static
NTSTATUS
NfsBuildFilePositionInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_POSITION_INFORMATION pFilePositionInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFilePositionInfo = (PFILE_POSITION_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(FILE_POSITION_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFilePositionInfo, sizeof(FILE_POSITION_INFORMATION));

    // pOutBuffer += sizeof(FILE_POSITION_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_POSITION_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileModeInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_MODE_INFORMATION),
                    FileModeInformation);
}

static
NTSTATUS
NfsBuildFileModeInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_MODE_INFORMATION pFileModeInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileModeInfo = (PFILE_MODE_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(FILE_MODE_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileModeInfo, sizeof(FILE_MODE_INFORMATION));

    // pOutBuffer += sizeof(FILE_MODE_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_MODE_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileAllInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_WithString_SMB_V2(
                    pExecContext,
                    sizeof(FILE_ALL_INFORMATION),
                    FileAllInformation);
}

static
NTSTATUS
NfsBuildFileAllInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ALL_INFORMATION pFileAllInfo = NULL;
    PSMB2_FILE_ALL_INFORMATION_HEADER pFileAllInfoHeader = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileAllInfo = (PFILE_ALL_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                    sizeof(SMB2_FILE_ALL_INFORMATION_HEADER) +
                    pFileAllInfo->NameInformation.FileNameLength;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileAllInfoHeader = (PSMB2_FILE_ALL_INFORMATION_HEADER)pOutBuffer;
    pFileAllInfoHeader->llChangeTime =
                            pFileAllInfo->BasicInformation.ChangeTime;
    pFileAllInfoHeader->llCreationTime =
                            pFileAllInfo->BasicInformation.CreationTime;
    pFileAllInfoHeader->llLastAccessTime =
                            pFileAllInfo->BasicInformation.LastAccessTime;
    pFileAllInfoHeader->llLastWriteTime =
                            pFileAllInfo->BasicInformation.LastWriteTime;
    pFileAllInfoHeader->ulFileAttributes =
                            pFileAllInfo->BasicInformation.FileAttributes;
    pFileAllInfoHeader->ullAllocationSize =
                            pFileAllInfo->StandardInformation.AllocationSize;
    pFileAllInfoHeader->ullEndOfFile =
                            pFileAllInfo->StandardInformation.EndOfFile;
    pFileAllInfoHeader->ulNumberOfLinks =
                            pFileAllInfo->StandardInformation.NumberOfLinks;
    pFileAllInfoHeader->ucDeletePending =
                            pFileAllInfo->StandardInformation.DeletePending;
    pFileAllInfoHeader->ucIsDirectory =
                            pFileAllInfo->StandardInformation.Directory;
    pFileAllInfoHeader->ullIndexNumber =
                            pFileAllInfo->InternalInformation.IndexNumber;
    pFileAllInfoHeader->ulEaSize =
                            pFileAllInfo->EaInformation.EaSize;
    pFileAllInfoHeader->ulAccessMask =
                            pFileAllInfo->AccessInformation.AccessFlags;
    pFileAllInfoHeader->ullCurrentByteOffset =
                            pFileAllInfo->PositionInformation.CurrentByteOffset;
    pFileAllInfoHeader->ulMode =
                            pFileAllInfo->ModeInformation.Mode;
    pFileAllInfoHeader->ulAlignment =
                        pFileAllInfo->AlignmentInformation.AlignmentRequirement;

    pFileAllInfoHeader->ulFilenameLength =
            pFileAllInfo->NameInformation.FileNameLength;

    pOutBuffer += sizeof(SMB2_FILE_ALL_INFORMATION_HEADER);

    if (pFileAllInfoHeader->ulFilenameLength)
    {
        memcpy(
            pOutBuffer,
            (PBYTE)pFileAllInfo->NameInformation.FileName,
            pFileAllInfoHeader->ulFilenameLength);
    }

    // pOutBuffer += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileAlignmentInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_ALIGNMENT_INFORMATION),
                    FileAlignmentInformation);
}

static
NTSTATUS
NfsBuildFileAlignmentInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ALIGNMENT_INFORMATION pFileAlignmentInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileAlignmentInfo = (PFILE_ALIGNMENT_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(FILE_ALIGNMENT_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileAlignmentInfo, sizeof(FILE_ALIGNMENT_INFORMATION));

    // pOutBuffer += sizeof(FILE_ALIGNMENT_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_ALIGNMENT_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileAltNameInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_WithString_SMB_V2(
                    pExecContext,
                    sizeof(FILE_NAME_INFORMATION),
                    FileAlternateNameInformation);
}

static
NTSTATUS
NfsBuildFileAltNameInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE                      pData         = NULL;
    USHORT                     usDataLen     = 0;
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_NAME_INFORMATION pFileNameInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileNameInfo = (PFILE_NAME_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    if (pGetInfoState->ulActualDataLength)
    {
        ntStatus = NfsMarshallFileNameInfo_SMB_V2(
                        pCtxSmb2->pTree,
                        pGetInfoState->pData2,
                        pGetInfoState->ulActualDataLength,
                        &pData,
                        &usDataLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (usDataLen)
    {
        if (ulBytesAvailable < usDataLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pGetInfoResponseHeader->ulOutBufferLength =  usDataLen;

        memcpy(pOutBuffer, pData, usDataLen);
    }

    // pOutBuffer += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pData)
    {
        NfsFreeMemory(pData);
    }

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshallFileNameInfo_SMB_V2(
    PLWIO_NFS_TREE_2 pTree,
    PBYTE            pInfoBuffer,
    USHORT           usBytesAvailable,
    PBYTE*           ppData,
    PUSHORT          pusDataLen
    )
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;
    PBYTE    pData           = NULL;
    USHORT   usBytesRequired = 0;
    BOOLEAN  bShareInLock         = FALSE;
    PWSTR     pwszTreePath   = NULL; // Do not free
    PFILE_NAME_INFORMATION pFileNameInfo =
                                        (PFILE_NAME_INFORMATION)pInfoBuffer;
    PSMB2_FILE_NAME_INFORMATION pFileNameInfoPacked = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bShareInLock, &pTree->pShareInfo->mutex);

    ntStatus = NfsGetTreeRelativePath(
                    pTree->pShareInfo->pwszPath,
                    &pwszTreePath);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pTree->pShareInfo->mutex);

    if (STATUS_SUCCESS != NfsMatchPathPrefix(
                                pFileNameInfo->FileName,
                                pFileNameInfo->FileNameLength/sizeof(wchar16_t),
                                pwszTreePath))
    {
        ntStatus = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ULONG ulPrefixLength = wc16slen(pwszTreePath) * sizeof(wchar16_t);

        usBytesRequired = sizeof(SMB2_FILE_NAME_INFORMATION) +
                              pFileNameInfo->FileNameLength - ulPrefixLength;

        ntStatus = NfsAllocateMemory(usBytesRequired, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        pFileNameInfoPacked = (PSMB2_FILE_NAME_INFORMATION)pData;

        pFileNameInfoPacked->ulFileNameLength =
                    pFileNameInfo->FileNameLength - ulPrefixLength;

        memcpy((PBYTE)pFileNameInfoPacked->FileName,
               (PBYTE)pFileNameInfo->FileName + ulPrefixLength,
               pFileNameInfoPacked->ulFileNameLength);
    }

    *ppData = pData;
    *pusDataLen = usBytesRequired;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pTree->pShareInfo->mutex);

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;

    if (pData)
    {
        NfsFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
NfsGetFileAttrTagInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_ATTRIBUTE_TAG_INFORMATION),
                    FileAttributeTagInformation);
}

static
NTSTATUS
NfsBuildFileAttrTagInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ATTRIBUTE_TAG_INFORMATION pFileAttrTagInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileAttrTagInfo = (PFILE_ATTRIBUTE_TAG_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileAttrTagInfo, sizeof(FILE_ATTRIBUTE_TAG_INFORMATION));

    // pOutBuffer += sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileStreamInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_WithString_SMB_V2(
                    pExecContext,
                    sizeof(FILE_STREAM_INFORMATION),
                    FileStreamInformation);
}

static
NTSTATUS
NfsBuildFileStreamInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_STREAM_INFORMATION pFileStreamInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileStreamInfo = (PFILE_STREAM_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    if (pGetInfoState->ulActualDataLength)
    {
        ULONG ulAlignBytes = 0;

        ntStatus = NfsMarshallFileStreamResponse(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        pGetInfoState->pData2,
                        pGetInfoState->ulActualDataLength,
                        &ulAlignBytes,
                        &pGetInfoResponseHeader->ulOutBufferLength);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoResponseHeader->usOutBufferOffset += ulAlignBytes;
    }
    else
    {
        pGetInfoResponseHeader->ulOutBufferLength = 0;
    }

    // pOutBuffer       += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed    += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshallFileStreamResponse(
    PBYTE  pOutBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulDataLength,
    PULONG pulAlignBytes,
    PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus          = 0;
    ULONG    ulBytesUsed       = 0;
    ULONG    iInfoCount        = 0;
    ULONG    ulInfoCount       = 0;
    ULONG    ulOffset1         = 0;
    ULONG    ulBytesAvailable1 = 0;
    ULONG    ulAlignBytes      = 0;
    PFILE_STREAM_INFORMATION             pFileStreamInfoCursor = NULL;
    PSMB2_FILE_STREAM_INFORMATION_HEADER pInfoHeaderPrev       = NULL;
    PSMB2_FILE_STREAM_INFORMATION_HEADER pInfoHeaderCur        = NULL;

    if (ulOffset % 4)
    {
        USHORT usAlign = 4 - (ulOffset % 4);

        if (ulBytesAvailable < usAlign)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulAlignBytes       = usAlign;
        ulOffset          += usAlign;
        ulBytesUsed       += usAlign;
        ulBytesAvailable  -= usAlign;
    }

    if (ulBytesAvailable < ulDataLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable1 = ulDataLength;

    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pData;
    while (pFileStreamInfoCursor && (ulBytesAvailable1 > 0))
    {
        ULONG  ulInfoBytesRequired = 0;

        ulInfoBytesRequired += sizeof(SMB2_FILE_STREAM_INFORMATION_HEADER);
        ulInfoBytesRequired += pFileStreamInfoCursor->StreamNameLength;

        /* Null terminate all stream names but the last. */
        if (pFileStreamInfoCursor->NextEntryOffset != 0)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);

            if (ulInfoBytesRequired % 8)
            {
                ulInfoBytesRequired += 8 - (ulInfoBytesRequired % 8);
            }
        }

        if (ulBytesAvailable1 < ulInfoBytesRequired)
        {
            break;
        }

        ulInfoCount++;

        ulBytesAvailable1 -= ulInfoBytesRequired;

        if (pFileStreamInfoCursor->NextEntryOffset)
        {
            pFileStreamInfoCursor =
                (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfoCursor) +
                                        pFileStreamInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileStreamInfoCursor = NULL;
        }
    }

    pOutBuffer += ulAlignBytes;
    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pData;

    for (; iInfoCount < ulInfoCount; iInfoCount++)
    {
        pInfoHeaderPrev = pInfoHeaderCur;
        pInfoHeaderCur = (PSMB2_FILE_STREAM_INFORMATION_HEADER)pOutBuffer;

        /* Update next entry offset for previous entry. */
        if (pInfoHeaderPrev != NULL)
        {
            pInfoHeaderPrev->ulNextEntryOffset = ulOffset1;
        }

        /* Reset the offset to 0 since it's relative. */
        ulOffset1 = 0;

        /* Add the header info. */
        pInfoHeaderCur->ulNextEntryOffset = 0;
        pInfoHeaderCur->llStreamAllocationSize =
                            pFileStreamInfoCursor->StreamAllocationSize;
        pInfoHeaderCur->ullStreamSize = pFileStreamInfoCursor->StreamSize;
        pInfoHeaderCur->ulStreamNameLength =
                            pFileStreamInfoCursor->StreamNameLength;

        pOutBuffer  += sizeof(SMB2_FILE_STREAM_INFORMATION_HEADER);
        ulBytesUsed += sizeof(SMB2_FILE_STREAM_INFORMATION_HEADER);
        ulOffset1   += sizeof(SMB2_FILE_STREAM_INFORMATION_HEADER);

        memcpy( pOutBuffer,
                pFileStreamInfoCursor->StreamName,
                pFileStreamInfoCursor->StreamNameLength);

        pOutBuffer  += pFileStreamInfoCursor->StreamNameLength;
        ulBytesUsed += pFileStreamInfoCursor->StreamNameLength;
        ulOffset1   += pFileStreamInfoCursor->StreamNameLength;

        /* Null terminate all streams names but the last. */
        if (pFileStreamInfoCursor->NextEntryOffset != 0)
        {
            pOutBuffer  += sizeof(wchar16_t);
            ulBytesUsed += sizeof(wchar16_t);
            ulOffset1   += sizeof(wchar16_t);

            if (ulOffset1 % 8)
            {
                USHORT usAlign = 8 - (ulOffset1 % 8);

                pOutBuffer  += usAlign;
                ulBytesUsed += usAlign;
                ulOffset1   += usAlign;
            }
        }

        pFileStreamInfoCursor =
                    (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfoCursor) +
                                    pFileStreamInfoCursor->NextEntryOffset);
    }

    *pulAlignBytes = ulAlignBytes;
    *pulBytesUsed  = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulAlignBytes = 0;
    *pulBytesUsed  = 0;

    if (ulBytesUsed)
    {
        memset(pOutBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

static
NTSTATUS
NfsGetFileFullEAInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_WithString_SMB_V2(
                    pExecContext,
                    sizeof(FILE_FULL_EA_INFORMATION),
                    FileFullEaInformation);
}

static
NTSTATUS
NfsBuildFileFullEAInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_FULL_EA_INFORMATION pFileFullEaInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileFullEaInfo = (PFILE_FULL_EA_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    if (pGetInfoState->ulActualDataLength)
    {
        ULONG ulAlignBytes = 0;

        ntStatus = NfsMarshallFileFullEAResponse(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        pGetInfoState->pData2,
                        pGetInfoState->ulActualDataLength,
                        &ulAlignBytes,
                        &pGetInfoResponseHeader->ulOutBufferLength);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoResponseHeader->usOutBufferOffset += ulAlignBytes;
    }
    else
    {
        pGetInfoResponseHeader->ulOutBufferLength = 0;
    }

    // pOutBuffer       += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed    += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshallFileFullEAResponse(
    PBYTE  pOutBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulDataLength,
    PULONG pulAlignBytes,
    PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus          = 0;
    ULONG    ulBytesUsed       = 0;
    ULONG    iInfoCount        = 0;
    ULONG    ulInfoCount       = 0;
    ULONG    ulOffset1         = 0;
    ULONG    ulBytesAvailable1 = 0;
    ULONG    ulAlignBytes      = 0;
    PFILE_FULL_EA_INFORMATION             pFileFullEAInfoCursor = NULL;
    PSMB2_FILE_FULL_EA_INFORMATION_HEADER pInfoHeaderPrev       = NULL;
    PSMB2_FILE_FULL_EA_INFORMATION_HEADER pInfoHeaderCur        = NULL;

    if (ulOffset % 4)
    {
        USHORT usAlign = 4 - (ulOffset % 4);

        if (ulBytesAvailable < usAlign)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulAlignBytes       = usAlign;
        ulOffset          += usAlign;
        ulBytesUsed       += usAlign;
        ulBytesAvailable  -= usAlign;
    }

    if (ulBytesAvailable < ulDataLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable1 = ulDataLength;

    pFileFullEAInfoCursor = (PFILE_FULL_EA_INFORMATION)pData;
    while (pFileFullEAInfoCursor && (ulBytesAvailable1 > 0))
    {
        ULONG  ulInfoBytesRequired = 0;

        ulInfoBytesRequired += sizeof(SMB2_FILE_FULL_EA_INFORMATION_HEADER);
        ulInfoBytesRequired += pFileFullEAInfoCursor->EaNameLength + 1;
        ulInfoBytesRequired += pFileFullEAInfoCursor->EaValueLength;

		if (pFileFullEAInfoCursor->NextEntryOffset)
		{
    	    if (ulInfoBytesRequired % 8)
        	{
        	    ulInfoBytesRequired += 8 - (ulInfoBytesRequired % 8);
        	}
		}

        if (ulBytesAvailable1 < ulInfoBytesRequired)
        {
            break;
        }

        ulInfoCount++;

        ulBytesAvailable1 -= ulInfoBytesRequired;

        if (pFileFullEAInfoCursor->NextEntryOffset)
        {
            pFileFullEAInfoCursor =
                (PFILE_FULL_EA_INFORMATION)(((PBYTE)pFileFullEAInfoCursor) +
                                pFileFullEAInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileFullEAInfoCursor = NULL;
        }
    }

    pOutBuffer += ulAlignBytes;
    pFileFullEAInfoCursor = (PFILE_FULL_EA_INFORMATION)pData;

    for (; iInfoCount < ulInfoCount; iInfoCount++)
    {
        pInfoHeaderPrev = pInfoHeaderCur;
        pInfoHeaderCur = (PSMB2_FILE_FULL_EA_INFORMATION_HEADER)pOutBuffer;

        /* Update next entry offset for previous entry. */
        if (pInfoHeaderPrev != NULL)
        {
            pInfoHeaderPrev->ulNextEntryOffset = ulOffset1;
        }

        /* Reset the offset to 0 since it's relative. */
        ulOffset1 = 0;

        /* Add the header info. */
        pInfoHeaderCur->ulNextEntryOffset = 0;
        pInfoHeaderCur->ucFlags         = pFileFullEAInfoCursor->Flags;
        pInfoHeaderCur->ucEaNameLength  = pFileFullEAInfoCursor->EaNameLength;
        pInfoHeaderCur->usEaValueLength = pFileFullEAInfoCursor->EaValueLength;

        pOutBuffer  += sizeof(SMB2_FILE_FULL_EA_INFORMATION_HEADER);
        ulBytesUsed += sizeof(SMB2_FILE_FULL_EA_INFORMATION_HEADER);
        ulOffset1   += sizeof(SMB2_FILE_FULL_EA_INFORMATION_HEADER);

        if (pFileFullEAInfoCursor->EaNameLength)
        {
            memcpy( pOutBuffer,
                    &pFileFullEAInfoCursor->EaName[0],
                    pFileFullEAInfoCursor->EaNameLength);
        }

        pOutBuffer  += pFileFullEAInfoCursor->EaNameLength + 1;
        ulBytesUsed += pFileFullEAInfoCursor->EaNameLength + 1;
        ulOffset1   += pFileFullEAInfoCursor->EaNameLength + 1;

        if (pFileFullEAInfoCursor->EaValueLength)
        {
            PBYTE pEaValue = (PBYTE)&pFileFullEAInfoCursor->EaName[0] +
                                    pFileFullEAInfoCursor->EaNameLength;

            memcpy(pOutBuffer, pEaValue, pFileFullEAInfoCursor->EaValueLength);

            pOutBuffer  += pFileFullEAInfoCursor->EaValueLength;
            ulBytesUsed += pFileFullEAInfoCursor->EaValueLength;
            ulOffset1   += pFileFullEAInfoCursor->EaValueLength;
        }

        if (ulOffset1 % 8)
        {
            USHORT usAlign = 8 - (ulOffset1 % 8);

            pOutBuffer  += usAlign;
            ulOffset1   += usAlign;
            ulBytesUsed += usAlign;
        }

        pFileFullEAInfoCursor =
                    (PFILE_FULL_EA_INFORMATION)(((PBYTE)pFileFullEAInfoCursor) +
                                    pFileFullEAInfoCursor->NextEntryOffset);
    }

    *pulAlignBytes = ulAlignBytes;
    *pulBytesUsed  = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulAlignBytes = 0;
    *pulBytesUsed  = 0;

    if (ulBytesUsed)
    {
        memset(pOutBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

static
NTSTATUS
NfsGetFileCompressionInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return NfsGetFileInfoGeneric_SMB_V2(
                    pExecContext,
                    sizeof(FILE_COMPRESSION_INFORMATION),
                    FileCompressionInformation);
}

static
NTSTATUS
NfsBuildFileCompressionInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_COMPRESSION_INFORMATION pFileCompressionInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB2_FILE_COMPRESSION_INFORMATION_HEADER pFileCompressionInfoHeader = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileCompressionInfo = (PFILE_COMPRESSION_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                    sizeof(SMB2_FILE_COMPRESSION_INFORMATION_HEADER);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileCompressionInfoHeader =
                    (PSMB2_FILE_COMPRESSION_INFORMATION_HEADER)pOutBuffer;
    pFileCompressionInfoHeader->llCompressedFileSize =
                    pFileCompressionInfo->CompressedFileSize;
    pFileCompressionInfoHeader->ucChunkShift =
                    pFileCompressionInfo->ChunkShift;
    pFileCompressionInfoHeader->ucClusterShift =
                    pFileCompressionInfo->ClusterShift;
    pFileCompressionInfoHeader->ucCompressionUnitShift =
                    pFileCompressionInfo->CompressionUnitShift;
    pFileCompressionInfoHeader->usCompressionFormat =
                    pFileCompressionInfo->CompressionFormat;

    // pOutBuffer       += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed    += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetFileInfoGeneric_SMB_V2(
    PNFS_EXEC_CONTEXT      pExecContext,
    ULONG                  ulStructSize,
    FILE_INFORMATION_CLASS infoClass
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        if (pGetInfoState->pRequestHeader->ulOutputBufferLen < ulStructSize)
        {
            ntStatus = STATUS_INFO_LENGTH_MISMATCH;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = NfsAllocateMemory(
                        SMB_MIN(pGetInfoState->pRequestHeader->ulOutputBufferLen,
                                ulStructSize),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength =
                SMB_MIN(pGetInfoState->pRequestHeader->ulOutputBufferLen,
                        ulStructSize);

        NfsPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pGetInfoState->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        infoClass);
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseGetInfoStateAsync_SMB_V2(pGetInfoState); // sync completion
    }

    pGetInfoState->ulActualDataLength =
                            pGetInfoState->ioStatusBlock.BytesTransferred;

error:

    return ntStatus;
}

static
NTSTATUS
NfsGetFileInfoGeneric_WithString_SMB_V2(
    PNFS_EXEC_CONTEXT      pExecContext,
    ULONG                  ulStructSize,
    FILE_INFORMATION_CLASS infoClass
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    BOOLEAN                    bContinue     = TRUE;
    PBYTE                      pErrorMessage = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    do
    {
        ntStatus = pGetInfoState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    if (pGetInfoState->ulDataLength >=
                            pGetInfoState->pRequestHeader->ulOutputBufferLen)
                    {
                        bContinue = FALSE;
                    }
                    else
                    {
                        ULONG ulNewSize =  0;

                        if (!pGetInfoState->ulDataLength)
                        {
                            if (pGetInfoState->pRequestHeader->ulOutputBufferLen < ulStructSize)
                            {
                                ntStatus = STATUS_INFO_LENGTH_MISMATCH;
                                BAIL_ON_NT_STATUS(ntStatus);
                            }

                            ulNewSize = ulStructSize + 256 * sizeof(wchar16_t);
                        }
                        else
                        {
                            ulNewSize = pGetInfoState->ulDataLength +
                                            256 * sizeof(wchar16_t);
                        }

                        ulNewSize = SMB_MIN(ulNewSize,
                                            pGetInfoState->pRequestHeader->ulOutputBufferLen);

                        ntStatus = SMBReallocMemory(
                                        pGetInfoState->pData2,
                                        (PVOID*)&pGetInfoState->pData2,
                                        ulNewSize);
                        BAIL_ON_NT_STATUS(ntStatus);

                        pGetInfoState->ulDataLength = ulNewSize;
                    }

                    NfsPrepareGetInfoStateAsync_SMB_V2(
                                    pGetInfoState,
                                    pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            pGetInfoState->pFile->hFile,
                                            pGetInfoState->pAcb,
                                            &pGetInfoState->ioStatusBlock,
                                            pGetInfoState->pData2,
                                            pGetInfoState->ulDataLength,
                                            infoClass);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            NfsReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pGetInfoState->pData2)
                {
                    pGetInfoState->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;

                    bContinue = FALSE;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

cleanup:

    if (pErrorMessage)
    {
        NfsFreeMemory(pErrorMessage);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_BUFFER_TOO_SMALL:

            {
                NTSTATUS ntStatus2 = STATUS_SUCCESS;
                ULONG    ulLength  = 0;

                if (!pGetInfoState->ulDataLength)
                {
                    ulLength = ulStructSize + sizeof(wchar16_t) * 256;
                }
                else if (pGetInfoState->ulDataLength ==
                            pGetInfoState->pRequestHeader->ulOutputBufferLen)
                {
                    ulLength = pGetInfoState->ulDataLength +
                                        sizeof(wchar16_t) * 256;
                }
                else
                {
                    ulLength = pGetInfoState->ulDataLength;
                }

                ntStatus2 = NfsAllocateMemory(
                                sizeof(ULONG),
                                (PVOID*)&pErrorMessage);
                if (ntStatus2)
                {
                    LWIO_LOG_ERROR(
                        "Failed to allocate buffer for error message "
                        "[error:0x%08x]",
                        ntStatus2);
                }
                else
                {
                    memcpy(pErrorMessage, (PBYTE)&ulLength, sizeof(ulLength));

                    ntStatus2 = NfsSetErrorMessage_SMB_V2(
                                    pCtxSmb2,
                                    pErrorMessage,
                                    sizeof(ulLength));
                    if (ntStatus2 == STATUS_SUCCESS)
                    {
                        pErrorMessage = NULL;
                    }
                    else
                    {
                        LWIO_LOG_ERROR(
                        "Failed to set error message in exec context "
                        "[error:0x%08x]",
                        ntStatus2);
                    }
                }
            }

            break;

        default:

            break;
    }

    goto cleanup;
}



