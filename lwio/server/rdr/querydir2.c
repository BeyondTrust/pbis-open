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
 *        querydir2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 directory query
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveQueryDirectory2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    FILE_INFORMATION_CLASS InfoClass,
    BOOLEAN bReturnSingleEntry,
    PIO_MATCH_FILE_SPEC FileSpec,
    BOOLEAN bRestartScan,
    ULONG ulOutputBufferLength
    );

static
BOOLEAN
RdrQueryDirectory2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrDecodeDirectoryInfo(
    PBYTE* ppCursor,
    PULONG pulRemaining,
    PVOID pFileInformation,
    ULONG ulLength,
    FILE_INFORMATION_CLASS fileInformationClass,
    PULONG pulLengthUsed
    );

static
NTSTATUS
RdrDecodeFileBothDirectoryInformation(
    PBYTE* ppCursor,
    PULONG pulRemaining,
    PVOID pFileInformation,
    ULONG ulLength,
    PULONG pulLengthUsed,
    PULONG* ppulNextOffset
    );

static
VOID
RdrCancelQueryDirectory2(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrQueryDirectory2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    pContext->State.QueryDirectory.pInformation = pIrp->Args.QueryDirectory.FileInformation;
    pContext->State.QueryDirectory.ulLength = pIrp->Args.QueryDirectory.Length;
    pContext->State.QueryDirectory.bRestart = pIrp->Args.QueryDirectory.RestartScan;

    IoIrpMarkPending(pIrp, RdrCancelQueryDirectory2, pContext);

    RdrQueryDirectory2Complete(pContext, STATUS_SUCCESS, NULL);
    status = STATUS_PENDING;
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
RdrTransceiveQueryDirectory2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    FILE_INFORMATION_CLASS InfoClass,
    BOOLEAN bReturnSingleEntry,
    PIO_MATCH_FILE_SPEC pFileSpec,
    BOOLEAN bRestartScan,
    ULONG ulOutputBufferLength
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
        COM2_FIND,
        0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeQueryDirectoryRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        (UCHAR) InfoClass,
        (UCHAR) (
            (bReturnSingleEntry ? SMB2_SEARCH_FLAGS_RETURN_SINGLE_ENTRY : 0) |
            (bRestartScan ? SMB2_SEARCH_FLAGS_RESTART_SCAN : 0)),
        0, /* file index */
        &pFile->Fid,
        NULL, /* FIXME: support pattern */
        ulOutputBufferLength);
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
RdrQueryDirectory2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PIRP pIrp = pContext->pIrp;
    PRDR_CCB2 pFile = IoFileGetContext(pIrp->FileHandle);
    ULONG ulSpaceUsed = 0;

    BAIL_ON_NT_STATUS(status);

    /* If we have a packet, decode it */
    if (pPacket)
    {
        status = pPacket->pSMB2Header->error;
        BAIL_ON_NT_STATUS(status);

        pFile->Enum.pPacket = pPacket;
        pPacket = NULL;

        status = RdrSmb2DecodeQueryDirectoryResponse(
            pFile->Enum.pPacket,
            &pFile->Enum.pCursor,
            &pFile->Enum.ulRemaining);
        BAIL_ON_NT_STATUS(status);
    }

    /*
     * If we have remaining data from the last packet, unpack it
     * into the provided space in the IRP
     */
    if (pFile->Enum.ulRemaining)
    {
        status = RdrDecodeDirectoryInfo(
            &pFile->Enum.pCursor,
            &pFile->Enum.ulRemaining,
            pContext->State.QueryDirectory.pInformation,
            pContext->State.QueryDirectory.ulLength,
            pIrp->Args.QueryDirectory.FileInformationClass,
            &ulSpaceUsed);
        BAIL_ON_NT_STATUS(status);

        if (ulSpaceUsed == 0)
        {
            /* Not enough space left to fit another entry */
            if (pIrp->Args.QueryDirectory.Length - pContext->State.QueryDirectory.ulLength != 0)
            {
                /* We managed to fit at least one entry, so return success now */
                goto cleanup;
            }
            else
            {
                /* Not enough space for even one entry */
                status = STATUS_BUFFER_TOO_SMALL;
                BAIL_ON_NT_STATUS(status);
            }
        }

        pContext->State.QueryDirectory.pInformation =
            (PBYTE) pContext->State.QueryDirectory.pInformation + ulSpaceUsed;

        pContext->State.QueryDirectory.ulLength -= ulSpaceUsed;
    }

    /*
     * If we still haven't filled the info buffer in the IRP
     * and we don't have any available data left, request a packet
     */
    if (pContext->State.QueryDirectory.ulLength > 0 &&
        !pFile->Enum.ulRemaining)
    {
        if (pFile->Enum.pPacket)
        {
            RdrFreePacket(pFile->Enum.pPacket);
            pFile->Enum.pPacket = NULL;
        }

        pContext->Continue = RdrQueryDirectory2Complete;

        status = RdrTransceiveQueryDirectory2(
            pContext,
            pFile,
            pIrp->Args.QueryDirectory.FileInformationClass,
            pIrp->Args.QueryDirectory.ReturnSingleEntry,
            pIrp->Args.QueryDirectory.FileSpec,
            pContext->State.QueryDirectory.bRestart,
            pFile->pTree->pSession->pSocket->ulMaxTransactSize);
        /* Clear the restart flag now so we don't send it on any subsequent packets */
        pContext->State.QueryDirectory.bRestart = FALSE;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    RdrFreePacket(pPacket);

    /*
     * If we ran out of matches but did fill part of the info structure, swallow
     * the error.  Otherwise, map it to the status code expected by iomgr.
     */
    if (status == STATUS_NO_MORE_FILES)
    {
        if (pIrp->Args.QueryDirectory.Length - pContext->State.QueryDirectory.ulLength != 0)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_NO_MORE_MATCHES;
        }
    }

    if (status == STATUS_SUCCESS)
    {
        /*
         * Success!  Calculate bytes transferred as the difference between
         * the size of the buffer and the size left after decoding into it
         */
        pIrp->IoStatusBlock.BytesTransferred =
            pIrp->Args.QueryDirectory.Length - pContext->State.QueryDirectory.ulLength;
    }

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
RdrDecodeDirectoryInfo(
    PBYTE* ppCursor,
    PULONG pulRemaining,
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
            status = RdrDecodeFileBothDirectoryInformation(
                ppCursor,
                pulRemaining,
                pFileInformation,
                ulLength,
                &ulLengthUsed,
                &pulNextOffset);
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
    } while (ulLengthUsed && *pulRemaining);

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
RdrDecodeFileBothDirectoryInformation(
    PBYTE* ppCursor,
    PULONG pulRemaining,
    PVOID pFileInformation,
    ULONG ulLength,
    PULONG pulLengthUsed,
    PULONG* ppulNextOffset
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_BOTH_DIR_INFORMATION pBothInfo = pFileInformation;
    PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER pBothInfoPacked = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER) *ppCursor;
    ULONG ulFileNameLength = 0;
    ULONG ulLengthUsed = 0;

    status = Advance(ppCursor, pulRemaining, sizeof(*pBothInfoPacked));
    BAIL_ON_NT_STATUS(status);

    ulFileNameLength = SMB_LTOH32(pBothInfoPacked->FileNameLength);

    /* Do we have space to decode the information? */
    if (ulFileNameLength + 1 + sizeof(*pBothInfo) <= ulLength)
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

        /* Advance past filename */
        status = Advance(ppCursor, pulRemaining, ulFileNameLength);
        BAIL_ON_NT_STATUS(status);

        SMB_LTOHWSTR(pBothInfo->ShortName, pBothInfoPacked->ShortName, sizeof(pBothInfo->ShortName) / sizeof(WCHAR) - 1);
        SMB_LTOHWSTR(pBothInfo->FileName, pBothInfoPacked->FileName, ulFileNameLength / sizeof(WCHAR));

        if (SMB_LTOH32(pBothInfoPacked->NextEntryOffset) != 0)
        {
            status = AdvanceTo(ppCursor, pulRemaining,
                (PBYTE) pBothInfoPacked + SMB_LTOH32(pBothInfoPacked->NextEntryOffset));
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            /* No more entries in this block -- consume any padding bytes so the outer
             * loop knows to exit
             */
            status = Advance(ppCursor, pulRemaining, *pulRemaining);
            LWIO_ASSERT(status == STATUS_SUCCESS);
        }

        ulLengthUsed = ulFileNameLength + 1 + sizeof(*pBothInfo);

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
        /* Rewind to start of header */
        *ppCursor -= sizeof(*pBothInfoPacked);
        *pulRemaining += sizeof(*pBothInfoPacked);
        *pulLengthUsed = 0;
    }

error:

    return status;
}
