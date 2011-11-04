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
 *        querydir.c
 *
 * Abstract:
 *
 *        Likewise IO Redirector (RDR)
 *
 *        Query Directory Information
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "rdr.h"

#define MAX_FIND_BUFFER 4096

static
NTSTATUS
RdrTransceiveFindFirst2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    USHORT usSearchAttrs,
    USHORT usSearchCount,
    USHORT usFlags,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulSearchStorageType,
    PCWSTR pwszSearchPattern,
    ULONG ulResultLength
    );

static
BOOLEAN
RdrFindFirst2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrTransceiveFindNext2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    USHORT usSearchId,
    USHORT usSearchCount,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulResumeKey,
    USHORT usFlags,
    PWSTR pwszFileName,
    ULONG ulResultLength
    );

static
BOOLEAN
RdrFindNext2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrQueryDirComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrFileSpecToSearchPattern(
    PCWSTR pwszPath,
    PIO_MATCH_FILE_SPEC pFileSpec,
    PWSTR* ppwszSearchPattern
    );

static
NTSTATUS
RdrUnmarshalFindResults(
    PRDR_CCB pHandle,
    BOOLEAN bReturnSingleEntry,
    PVOID pFileInformation,
    ULONG ulLength,
    FILE_INFORMATION_CLASS fileInformationClass,
    PULONG pulLengthUsed
    );

static
NTSTATUS
RdrUnmarshalFileBothDirectoryInformation(
    PRDR_CCB pHandle,
    PVOID pFileInformation,
    ULONG ulLength,
    PULONG pulLengthUsed,
    PULONG* ppulNextOffset
    );

static
VOID
RdrCancelQueryDirectory(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrQueryDirectory(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_CCB pFile = NULL;
    SMB_INFO_LEVEL infoLevel = 0;
    PWSTR pwszPattern = NULL;
    PRDR_OP_CONTEXT pContext = NULL;
    BOOLEAN bLocked = FALSE;

    pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.QueryDirectory.FileInformationClass)
    {
    case FileBothDirectoryInformation:
        infoLevel = SMB_FIND_FILE_BOTH_DIRECTORY_INFO;
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(status);
        break;
    }

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelQueryDirectory, pContext);

    LWIO_LOCK_MUTEX(bLocked, &pFile->mutex);
    if (pFile->find.bInProgress)
    {
        /* FIXME: better status code? */
        status = STATUS_DEVICE_BUSY;
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        pFile->find.bInProgress = TRUE;
    }

    if (pFile->find.pBuffer && pFile->find.usSearchCount == 0)
    {
        if (pFile->find.usEndOfSearch)
        {
            /* We are out of of buffered entries and
               the server has no more results for us */
            status = STATUS_NO_MORE_MATCHES;
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            /* Perform a find next */
            pContext->Continue = RdrFindNext2Complete;

            status = RdrTransceiveFindNext2(
                pContext,
                pFile->pTree,
                pFile->find.usSearchId,
                512, /* Search count */
                infoLevel,
                0, /* ulResumeKey */
                0x2, /* Search flags */
                NULL, /* Filename */
                pFile->find.ulBufferCapacity);
            BAIL_ON_NT_STATUS(status);
        }
    }
    else if (!pFile->find.pBuffer)
    {
        /* This is the first query, so we start a find */
        pFile->find.ulBufferCapacity = MAX_FIND_BUFFER;

        status = RTL_ALLOCATE(&pFile->find.pBuffer,
                                BYTE,
                                pFile->find.ulBufferCapacity);
        BAIL_ON_NT_STATUS(status);

        status = RdrFileSpecToSearchPattern(
            RDR_CCB_PATH(pFile),
            pIrp->Args.QueryDirectory.FileSpec,
            &pwszPattern);
        BAIL_ON_NT_STATUS(status);

        pContext->Continue = RdrFindFirst2Complete;

        status = RdrTransceiveFindFirst2(
            pContext,
            pFile->pTree,
            0x7f, /* Search attributes */
            512, /* Search count */
            0x2, /* Search flags */
            infoLevel, /* Info level */
            0, /* Search storage type */
            pwszPattern, /* Search pattern */
            pFile->find.ulBufferCapacity);
        BAIL_ON_NT_STATUS(status);
    }

error:

    LWIO_UNLOCK_MUTEX(bLocked, &pFile->mutex);

    if (status != STATUS_PENDING && pContext)
    {
        RdrQueryDirComplete(pContext, status, pFile);
        status = STATUS_PENDING;
    }

    RTL_FREE(&pwszPattern);

    return status;
}

static
NTSTATUS
RdrTransceiveFindFirst2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    USHORT usSearchAttrs,
    USHORT usSearchCount,
    USHORT usFlags,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulSearchStorageType,
    PCWSTR pwszSearchPattern,
    ULONG ulResultLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32_t packetByteCount = 0;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_FIND_FIRST2;
    PSMB_FIND_FIRST2_REQUEST_PARAMETERS pFindParameters = NULL;
    USHORT usFindParametersLength = 0;
    USHORT usFindParametersOffset = 0;
    USHORT usFindDataOffset = 0;
    PBYTE pCursor = NULL;

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

    pContext->Packet.pData = pContext->Packet.pParams + sizeof(TRANSACTION_REQUEST_HEADER);
    pContext->Packet.bufferUsed += sizeof(TRANSACTION_REQUEST_HEADER);
    pContext->Packet.pSMBHeader->wordCount = 14 + sizeof(usSetup)/sizeof(USHORT);

    pHeader = (TRANSACTION_REQUEST_HEADER *) pContext->Packet.pParams;
    
    usFindParametersLength = sizeof(*pFindParameters) + (LwRtlWC16StringNumChars(pwszSearchPattern) + 1) * sizeof(WCHAR);

    status = RTL_ALLOCATE(&pFindParameters,
                            SMB_FIND_FIRST2_REQUEST_PARAMETERS,
                            usFindParametersLength);
    BAIL_ON_NT_STATUS(status);
    
    pFindParameters->usSearchAttrs       = SMB_HTOL16(usSearchAttrs);
    pFindParameters->usSearchCount       = SMB_HTOL16(usSearchCount);
    pFindParameters->usFlags             = SMB_HTOL16(usFlags);
    pFindParameters->infoLevel           = SMB_HTOL16(infoLevel);
    pFindParameters->ulSearchStorageType = SMB_HTOL32(ulSearchStorageType);

    status = WireMarshallTransactionRequestData(
        pContext->Packet.pData,
        pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
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
    BAIL_ON_NT_STATUS(status);

    assert(packetByteCount <= UINT16_MAX);
    pContext->Packet.bufferUsed += packetByteCount;

    pCursor = pContext->Packet.pData + usFindParametersOffset + offsetof(SMB_FIND_FIRST2_REQUEST_PARAMETERS, pwszSearchPattern);

    if ((pCursor - (PBYTE) pContext->Packet.pSMBHeader) % 2)
    {
        /* Align cursor to 2 byte boundary before writing string */
        pCursor += 1;
    }

    SMB_HTOLWSTR(pCursor,
                 pwszSearchPattern,
                 LwRtlWC16StringNumChars(pwszSearchPattern));
    
    pHeader->totalParameterCount = SMB_HTOL16(usFindParametersLength);
    pHeader->totalDataCount      = SMB_HTOL16(0);
    pHeader->maxParameterCount   = SMB_HTOL16(10 * sizeof(USHORT) + 256);
    pHeader->maxDataCount        = SMB_HTOL16((USHORT) ulResultLength);
    pHeader->maxSetupCount       = SMB_HTOL8(sizeof(usSetup)/sizeof(USHORT));
    pHeader->flags               = SMB_HTOL16(0);
    pHeader->timeout             = SMB_HTOL32(0);
    pHeader->parameterCount      = SMB_HTOL16(usFindParametersLength);
    pHeader->parameterOffset     = SMB_HTOL16(usFindParametersOffset + (pContext->Packet.pData - (PBYTE) pContext->Packet.pSMBHeader));
    pHeader->dataCount           = SMB_HTOL16(0);
    pHeader->dataOffset          = SMB_HTOL16(0);
    pHeader->setupCount          = SMB_HTOL8(sizeof(usSetup)/sizeof(USHORT));

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);
    
    status = RdrSocketTransceive(pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);
    
cleanup:

    RTL_FREE(&pFindParameters);

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrFindFirst2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pResponsePacket = pParam;
    ULONG ulOffset = 0;
    PRDR_CCB pFile = IoFileGetContext(pContext->pIrp->FileHandle);
    TRANSACTION_SECONDARY_RESPONSE_HEADER *pResponseHeader = NULL;
    PUSHORT pusReplySetup = NULL;
    PUSHORT pusReplyByteCount = NULL;
    PSMB_FIND_FIRST2_RESPONSE_PARAMETERS pReplyParameters = NULL;
    PBYTE pReplyData = NULL;
    USHORT usReplyByteCount = 0;
    USHORT usReplyDataCount = 0;

    BAIL_ON_NT_STATUS(status);

    status = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

    ulOffset = (PBYTE)pResponsePacket->pParams - (PBYTE)pResponsePacket->pSMBHeader;
    
    status = WireUnmarshallTransactionSecondaryResponse(
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
    BAIL_ON_NT_STATUS(status);

    pFile->find.usSearchId = SMB_LTOH16(pReplyParameters->usSearchId);
    pFile->find.usSearchCount = SMB_LTOH16(pReplyParameters->usSearchCount);
    pFile->find.usEndOfSearch = SMB_LTOH16(pReplyParameters->usEndOfSearch);
    pFile->find.usLastNameOffset = SMB_LTOH16(pReplyParameters->usLastNameOffset);

    /* Unmarshal reply byte count */
    status = UnmarshalUshort((PBYTE*) &pusReplyByteCount, NULL, &usReplyByteCount);
    BAIL_ON_NT_STATUS(status);

    usReplyDataCount = SMB_LTOH16(pResponseHeader->dataCount);

    if (usReplyByteCount > (pResponsePacket->bufferUsed - ((PBYTE) pusReplyByteCount  - (PBYTE) pResponsePacket->pRawBuffer) - sizeof(USHORT)) ||
        usReplyDataCount > (pResponsePacket->bufferUsed - ((PBYTE) pReplyData - (PBYTE) pResponsePacket->pRawBuffer)))
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }

    if (usReplyDataCount > pFile->find.ulBufferCapacity)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pFile->find.pBuffer, pReplyData, usReplyDataCount);
    pFile->find.ulBufferLength = usReplyDataCount;
    pFile->find.pCursor = pFile->find.pBuffer;

cleanup:

    RdrFreePacket(pResponsePacket);

    return RdrQueryDirComplete(pContext, status, pFile);

error:

    goto cleanup;
}

NTSTATUS
RdrTransceiveFindNext2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    USHORT usSearchId,
    USHORT usSearchCount,
    SMB_INFO_LEVEL infoLevel,
    ULONG ulResumeKey,
    USHORT usFlags,
    PWSTR pwszFileName,
    ULONG ulResultLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32_t packetByteCount = 0;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_FIND_NEXT2;
    PSMB_FIND_NEXT2_REQUEST_PARAMETERS pFindParameters = NULL;
    USHORT usFindParametersLength = 0;
    USHORT usFindParametersOffset = 0;
    USHORT usFindDataOffset = 0;

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
    
    pContext->Packet.pData = pContext->Packet.pParams + sizeof(TRANSACTION_REQUEST_HEADER);
    pContext->Packet.bufferUsed += sizeof(TRANSACTION_REQUEST_HEADER);
    pContext->Packet.pSMBHeader->wordCount = 14 + sizeof(usSetup)/sizeof(USHORT);

    pHeader = (TRANSACTION_REQUEST_HEADER *) pContext->Packet.pParams;
    
    usFindParametersLength = 
        sizeof(*pFindParameters) + 
        (LwRtlWC16StringNumChars(pwszFileName) + 1) * sizeof(WCHAR);

    status = RTL_ALLOCATE(&pFindParameters,
                            SMB_FIND_NEXT2_REQUEST_PARAMETERS,
                            usFindParametersLength);
    BAIL_ON_NT_STATUS(status);
    
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

    status = WireMarshallTransactionRequestData(
        pContext->Packet.pData,
        pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
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
    BAIL_ON_NT_STATUS(status);

    assert(packetByteCount <= UINT16_MAX);
    pContext->Packet.bufferUsed += packetByteCount;

    pHeader->totalParameterCount = SMB_HTOL16(usFindParametersLength);
    pHeader->totalDataCount      = SMB_HTOL16(0);
    pHeader->maxParameterCount   = SMB_HTOL16(10 * sizeof(USHORT));
    pHeader->maxDataCount        = SMB_HTOL16((USHORT) ulResultLength);
    pHeader->maxSetupCount       = SMB_HTOL8(sizeof(usSetup)/sizeof(USHORT));
    pHeader->flags               = SMB_HTOL16(0);
    pHeader->timeout             = SMB_HTOL32(0);
    pHeader->parameterCount      = SMB_HTOL16(usFindParametersLength);
    pHeader->parameterOffset     = SMB_HTOL16(usFindParametersOffset + (pContext->Packet.pData - (PBYTE) pContext->Packet.pSMBHeader));
    pHeader->dataCount           = SMB_HTOL16(0);
    pHeader->dataOffset          = SMB_HTOL16(usFindDataOffset + (pContext->Packet.pData - (PBYTE) pContext->Packet.pSMBHeader));
    pHeader->setupCount          = SMB_HTOL8(sizeof(usSetup)/sizeof(USHORT));

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);
    
    status = RdrSocketTransceive(pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);
    
cleanup:

    RTL_FREE(&pFindParameters);

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrFindNext2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pResponsePacket = pParam;
    ULONG ulOffset = 0;
    PRDR_CCB pFile = IoFileGetContext(pContext->pIrp->FileHandle);
    TRANSACTION_SECONDARY_RESPONSE_HEADER *pResponseHeader = NULL;
    PUSHORT pusReplySetup = NULL;
    PUSHORT pusReplyByteCount = NULL;
    PSMB_FIND_NEXT2_RESPONSE_PARAMETERS pReplyParameters = NULL;
    PBYTE pReplyData = NULL;
    USHORT usReplyByteCount = 0;
    USHORT usReplyDataCount = 0;

    BAIL_ON_NT_STATUS(status);

    status = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

    ulOffset = (PBYTE)pResponsePacket->pParams - (PBYTE)pResponsePacket->pSMBHeader;
    
    status = WireUnmarshallTransactionSecondaryResponse(
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
    BAIL_ON_NT_STATUS(status);

    /* Unmarshal reply byte count */
    status = UnmarshalUshort((PBYTE*) &pusReplyByteCount, NULL, &usReplyByteCount);
    BAIL_ON_NT_STATUS(status);

    usReplyDataCount = SMB_LTOH16(pResponseHeader->dataCount);

    if (usReplyByteCount > (pResponsePacket->bufferUsed - ((PBYTE) pusReplyByteCount  - (PBYTE) pResponsePacket->pRawBuffer) - sizeof(USHORT)) ||
        usReplyDataCount > (pResponsePacket->bufferUsed - ((PBYTE) pReplyData - (PBYTE) pResponsePacket->pRawBuffer)))
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }

    if (usReplyDataCount > pFile->find.ulBufferCapacity)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pFile->find.pBuffer, pReplyData, usReplyDataCount);
    pFile->find.ulBufferLength = usReplyDataCount;
    pFile->find.usSearchCount = SMB_LTOH16(pReplyParameters->usSearchCount);
    pFile->find.usEndOfSearch = SMB_LTOH16(pReplyParameters->usEndOfSearch);
    pFile->find.usLastNameOffset = SMB_LTOH16(pReplyParameters->usLastNameOffset);
    pFile->find.pCursor = pFile->find.pBuffer;

cleanup:

    RdrFreePacket(pResponsePacket);

    return RdrQueryDirComplete(pContext, status, pFile);

error:

    goto cleanup;
}

static
BOOLEAN
RdrQueryDirComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB pFile = pParam;
    BOOLEAN bLocked = FALSE;

    BAIL_ON_NT_STATUS(status);

    LWIO_LOCK_MUTEX(bLocked, &pFile->mutex);

    if (pFile->find.usSearchCount == 0)
    {
        status = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrUnmarshalFindResults(
        pFile,
        pContext->pIrp->Args.QueryDirectory.ReturnSingleEntry,
        pContext->pIrp->Args.QueryDirectory.FileInformation,
        pContext->pIrp->Args.QueryDirectory.Length,
        pContext->pIrp->Args.QueryDirectory.FileInformationClass,
        &pContext->pIrp->IoStatusBlock.BytesTransferred
        );
    BAIL_ON_NT_STATUS(status);

cleanup:

    pFile->find.bInProgress = FALSE;

    LWIO_UNLOCK_MUTEX(bLocked, &pFile->mutex);

    pContext->pIrp->IoStatusBlock.Status = status;

    IoIrpComplete(pContext->pIrp);
    RdrFreeContext(pContext);

    return FALSE;

error:

    goto cleanup;
}

static
NTSTATUS
RdrFileSpecToSearchPattern(
    PCWSTR pwszPath,
    PIO_MATCH_FILE_SPEC pFileSpec,
    PWSTR* ppwszSearchPattern
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszPattern = NULL;
    size_t pathLength = 0;

    if (pFileSpec)
    {
        status = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        pathLength = LwRtlWC16StringNumChars(pwszPath);

        status = RTL_ALLOCATE(
            &pwszPattern,
            WCHAR,
            (pathLength + 1 + 1 + 1) * sizeof(WCHAR));
        BAIL_ON_NT_STATUS(status);
        memcpy(pwszPattern, pwszPath, pathLength * sizeof(WCHAR));
        pwszPattern[pathLength] = '\\';
        pwszPattern[pathLength+1] = '*';
        pwszPattern[pathLength+2] = '\0';
    }

    *ppwszSearchPattern = pwszPattern;

cleanup:

    return status;

error:

    RTL_FREE(&pwszPattern);

    goto cleanup;
}

static
NTSTATUS
RdrUnmarshalFindResults(
    PRDR_CCB pFile,
    BOOLEAN bReturnSingleEntry,
    PVOID pFileInformation,
    ULONG ulLength,
    FILE_INFORMATION_CLASS fileInformationClass,
    PULONG pulLengthUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulLengthUsed = 0;
    ULONG ulTotalLengthUsed = 0;
    PULONG pulNextOffset = NULL;

    do
    {
        switch (fileInformationClass)
        {
        case FileBothDirectoryInformation:
            status = RdrUnmarshalFileBothDirectoryInformation(
                pFile,
                pFileInformation,
                ulLength,
                &ulLengthUsed,
                &pulNextOffset
                );
            BAIL_ON_NT_STATUS(status);
            break;
        default:
            status = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(status);
            break;
        }

        if (ulLengthUsed)
        {
            ulTotalLengthUsed += ulLengthUsed;
            pFileInformation = ((PBYTE) pFileInformation) + ulLengthUsed;
            ulLength -= ulLengthUsed;
            *pulNextOffset = ulLengthUsed;
        }
    } while (!bReturnSingleEntry && ulLengthUsed);

    if (pulNextOffset)
    {
        *pulNextOffset = 0;
    }

    *pulLengthUsed = ulTotalLengthUsed;

error:

    return status;
}

static
NTSTATUS
RdrUnmarshalFileBothDirectoryInformation(
    PRDR_CCB pFile,
    PVOID pFileInformation,
    ULONG ulLength,
    PULONG pulLengthUsed,
    PULONG* ppulNextOffset
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_BOTH_DIR_INFORMATION pBothInfo = pFileInformation;
    PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER pBothInfoPacked = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER) pFile->find.pCursor;
    ULONG ulFileNameLength = SMB_LTOH32(pBothInfoPacked->FileNameLength);
    ULONG ulLengthUsed = 0;

    if ((ulFileNameLength + 1) * sizeof(WCHAR) + sizeof(*pBothInfo) <= ulLength &&
        pFile->find.usSearchCount)
    {
        pBothInfo->FileIndex         = SMB_LTOH32(pBothInfoPacked->FileIndex);
        pBothInfo->CreationTime      = SMB_LTOH64(pBothInfoPacked->CreationTime);
        pBothInfo->LastAccessTime    = SMB_LTOH64(pBothInfoPacked->LastAccessTime);
        pBothInfo->LastWriteTime     = SMB_LTOH64(pBothInfoPacked->LastWriteTime);
        pBothInfo->ChangeTime        = SMB_LTOH64(pBothInfoPacked->ChangeTime);
        pBothInfo->EndOfFile         = SMB_LTOH64(pBothInfoPacked->EndOfFile);
        pBothInfo->AllocationSize    = SMB_LTOH64(pBothInfoPacked->AllocationSize);
        pBothInfo->FileAttributes    = SMB_LTOH32(pBothInfoPacked->FileAttributes);
        pBothInfo->FileNameLength    = ulFileNameLength;
        pBothInfo->EaSize            = SMB_LTOH32(pBothInfoPacked->EaSize);
        pBothInfo->ShortNameLength   = SMB_LTOH8(pBothInfoPacked->ShortNameLength);

        SMB_LTOHWSTR(pBothInfo->ShortName, pBothInfoPacked->ShortName, sizeof(pBothInfo->ShortName) / sizeof(WCHAR) - 1);
        SMB_LTOHWSTR(pBothInfo->FileName, pBothInfoPacked->FileName, ulFileNameLength / sizeof(WCHAR));
        
        pFile->find.pCursor += SMB_LTOH32(pBothInfoPacked->NextEntryOffset);
        pFile->find.usSearchCount--;

        ulLengthUsed = (ulFileNameLength + 1) * sizeof(WCHAR) + sizeof(*pBothInfo);

        /* Align next entry to 8 byte boundary */
        if (ulLengthUsed % 8)
        {
            ulLengthUsed += (8 - ulLengthUsed % 8);
        }

        *ppulNextOffset = &pBothInfo->NextEntryOffset;
        *pulLengthUsed = ulLengthUsed;
    }
    else
    {
        *pulLengthUsed = 0;
    }

    return status;
}
