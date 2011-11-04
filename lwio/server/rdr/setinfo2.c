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
 *        setinfo2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 set info
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveSetInfoFile2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    FILE_INFORMATION_CLASS infoClass,
    PVOID pInfo
    );

static
BOOLEAN
RdrSetInfoFile2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrEncodeFileInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    FILE_INFORMATION_CLASS infoLevel,
    PVOID pInfo
    );

static
NTSTATUS
RdrEncodeFileEndOfFileInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo
    );

static
NTSTATUS
RdrEncodeFileBasicInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo
    );

static
NTSTATUS
RdrEncodeFileDispositionInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo
    );

static
NTSTATUS
RdrEncodeFileRenameInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo
    );

static
VOID
RdrCancelSetInfo2(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrSetInformation2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB2 pFile = NULL;
    ULONG ulInfoLength = 0;

    pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.QuerySetInformation.FileInformationClass)
    {
    case FileBasicInformation:
        ulInfoLength = sizeof(FILE_BASIC_INFORMATION);
        break;
    case FileEndOfFileInformation:
        ulInfoLength = sizeof(FILE_END_OF_FILE_INFORMATION);
        break;
    case FileRenameInformation:
        ulInfoLength = sizeof(FILE_RENAME_INFORMATION) - sizeof(WCHAR);
        if (pIrp->Args.QuerySetInformation.Length < ulInfoLength)
        {
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }
        ulInfoLength +=
            ((PFILE_RENAME_INFORMATION) pIrp->Args.QuerySetInformation.FileInformation)->
            FileNameLength;
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(status);
        break;
    }

    if (pIrp->Args.QuerySetInformation.Length < ulInfoLength)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelSetInfo2, pContext);

    pContext->Continue = RdrSetInfoFile2Complete;

    status = RdrTransceiveSetInfoFile2(
        pContext,
        pFile,
        pIrp->Args.QuerySetInformation.FileInformationClass,
        pIrp->Args.QuerySetInformation.FileInformation);
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
RdrTransceiveSetInfoFile2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    FILE_INFORMATION_CLASS infoClass,
    PVOID pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;
    ULONG ulPackedLength = 0;
    PULONG pulLength = NULL;
    PBYTE pInfoStart = NULL;

    switch (infoClass)
    {
    case FileBasicInformation:
        ulPackedLength = sizeof(TRANS2_FILE_BASIC_INFORMATION);
        break;
    case FileEndOfFileInformation:
        ulPackedLength = sizeof(TRANS2_FILE_END_OF_FILE_INFORMATION);
        break;
    case FileRenameInformation:
        ulPackedLength = sizeof(RDR_SMB2_FILE_RENAME_INFO_HEADER);
        ulPackedLength +=
            ((PFILE_RENAME_INFORMATION) pInfo)->FileNameLength;
        break;
    default:
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
        break;
    }

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_SET_INFO_SIZE(ulPackedLength));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_SETINFO,
        0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeSetInfoRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        SMB2_INFO_TYPE_FILE,
        (UCHAR) infoClass,
        0, /* additional info */
        &pFile->Fid,
        &pulLength); /* input buffer length */
    BAIL_ON_NT_STATUS(status);

    pInfoStart = pCursor;

    status = RdrEncodeFileInfo(
        &pCursor,
        &ulRemaining,
        infoClass,
        pInfo);
    BAIL_ON_NT_STATUS(status);

    *pulLength = SMB_HTOL32(pCursor - pInfoStart);

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
RdrSetInfoFile2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMB2Header->error;
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
RdrEncodeFileInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    FILE_INFORMATION_CLASS infoLevel,
    PVOID pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (infoLevel)
    {
    case FileBasicInformation:
        status = RdrEncodeFileBasicInfo(
            ppCursor,
            pulRemainingSpace,
            pInfo);
        BAIL_ON_NT_STATUS(status);
        break;
    case FileEndOfFileInformation:
        status = RdrEncodeFileEndOfFileInfo(
            ppCursor,
            pulRemainingSpace,
            pInfo);
        BAIL_ON_NT_STATUS(status);
        break;
    case FileDispositionInformation:
        status = RdrEncodeFileDispositionInfo(
            ppCursor,
            pulRemainingSpace,
            pInfo);
        BAIL_ON_NT_STATUS(status);
        break;
    case FileRenameInformation:
        status = RdrEncodeFileRenameInfo(
            ppCursor,
            pulRemainingSpace,
            pInfo);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
        break;
    }

error:

    return status;
}

static
NTSTATUS
RdrEncodeFileEndOfFileInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_END_OF_FILE_INFORMATION pEndInfo = pInfo;
    PBYTE pCursor = *ppCursor;
    ULONG ulRemainingSpace = *pulRemainingSpace;
    PTRANS2_FILE_END_OF_FILE_INFORMATION pEndInfoPacked = (PTRANS2_FILE_END_OF_FILE_INFORMATION) *ppCursor;

    /* Advance cursor past info structure */
    status = Advance(&pCursor, &ulRemainingSpace, sizeof(*pEndInfoPacked));
    BAIL_ON_NT_STATUS(status);

    pEndInfoPacked->EndOfFile = SMB_HTOL64(pEndInfo->EndOfFile);

    *ppCursor = pCursor;
    *pulRemainingSpace = ulRemainingSpace;

cleanup:

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrEncodeFileBasicInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_BASIC_INFORMATION pEndInfo = pInfo;
    PBYTE pCursor = *ppCursor;
    ULONG ulRemainingSpace = *pulRemainingSpace;
    PTRANS2_FILE_BASIC_INFORMATION pEndInfoPacked = (PTRANS2_FILE_BASIC_INFORMATION) *ppCursor;

    /* Advance cursor past info structure */
    status = Advance(&pCursor, &ulRemainingSpace, sizeof(*pEndInfoPacked));
    BAIL_ON_NT_STATUS(status);

    pEndInfoPacked->CreationTime = SMB_HTOL64(pEndInfo->CreationTime);
    pEndInfoPacked->ChangeTime = SMB_HTOL64(pEndInfo->ChangeTime);
    pEndInfoPacked->FileAttributes = SMB_HTOL32(pEndInfo->FileAttributes);
    pEndInfoPacked->LastAccessTime = SMB_HTOL64(pEndInfo->LastAccessTime);
    pEndInfoPacked->LastWriteTime = SMB_HTOL64(pEndInfo->LastWriteTime);

    *ppCursor = pCursor;
    *pulRemainingSpace = ulRemainingSpace;

cleanup:

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrEncodeFileDispositionInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = *ppCursor;
    ULONG ulRemainingSpace = *pulRemainingSpace;
    PFILE_DISPOSITION_INFORMATION pDispInfo = pInfo;
    PTRANS2_FILE_DISPOSITION_INFORMATION pDispInfoPacked = (PTRANS2_FILE_DISPOSITION_INFORMATION) pCursor;

    /* Advance cursor past info structure */
    status = Advance(&pCursor, &ulRemainingSpace, sizeof(*pDispInfoPacked));
    BAIL_ON_NT_STATUS(status);

    pDispInfoPacked->bFileIsDeleted = pDispInfo->DeleteFile;

    *ppCursor = pCursor;
    *pulRemainingSpace = ulRemainingSpace;

cleanup:

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrEncodeFileRenameInfo(
    PBYTE* ppCursor,
    PULONG pulRemainingSpace,
    PVOID pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_RENAME_INFORMATION pRenameInfo = pInfo;
    PBYTE pCursor = *ppCursor;
    ULONG ulRemainingSpace = *pulRemainingSpace;
    PRDR_SMB2_FILE_RENAME_INFO_HEADER pPacked = (PRDR_SMB2_FILE_RENAME_INFO_HEADER) *ppCursor;
    PRDR_CCB2 pRoot = NULL;
    PBYTE pFileName = NULL;
    PWSTR pwszFilename = NULL;
    ULONG ulFilenameLength = 0;

#if 0
    if (pRenameInfo->RootDirectory)
    {
        pRoot = IoFileGetContext(pRenameInfo->RootDirectory);
    }
    pRoot = NULL;
#endif

    status = RdrConvertPath(
           pRenameInfo->FileName,
           NULL,
           NULL,
           &pwszFilename);
    BAIL_ON_NT_STATUS(status);

    ulFilenameLength = LwRtlWC16StringNumChars(pwszFilename+1) * sizeof(WCHAR);

    /* Advance cursor past info structure */
    status = Advance(&pCursor, &ulRemainingSpace, sizeof(*pPacked));
    BAIL_ON_NT_STATUS(status);

    pPacked->ucReplaceIfExists = pRenameInfo->ReplaceIfExists;
    memset(pPacked->ucReserved, 0, sizeof(pPacked->ucReserved));
    pPacked->ullRootDir = pRoot ? SMB_HTOL64(pRoot->Fid.ullVolatileId) : 0;
    pPacked->ulFileNameLength = SMB_HTOL32(ulFilenameLength);

    pFileName = pCursor;

    status = Advance(&pCursor, &ulRemainingSpace, ulFilenameLength);
    BAIL_ON_NT_STATUS(status);

    SMB_HTOLWSTR(
        pFileName,
        pwszFilename+1,
        ulFilenameLength / sizeof(WCHAR));

    *ppCursor = pCursor;
    *pulRemainingSpace = ulRemainingSpace;

cleanup:

    RTL_FREE(&pwszFilename);

    return status;

error:

    goto cleanup;
}

