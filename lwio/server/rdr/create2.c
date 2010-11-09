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
 *        create2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 create code
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveCreate2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions
    );

static
BOOLEAN
RdrFinishCreate2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrChaseCreateReferral2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

BOOLEAN
RdrCreateTreeConnected2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_TREE2 pTree = pParam;
    PIRP pIrp = pContext->pIrp;
    ACCESS_MASK DesiredAccess = pIrp->Args.Create.DesiredAccess;
    LONG64 AllocationSize = pIrp->Args.Create.AllocationSize;
    FILE_SHARE_FLAGS ShareAccess = pIrp->Args.Create.ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition = pIrp->Args.Create.CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions = pIrp->Args.Create.CreateOptions;
    FILE_ATTRIBUTES FileAttributes =  pIrp->Args.Create.FileAttributes;
    PRDR_CCB2 pFile = NULL;

    switch (status)
    {
    case STATUS_SUCCESS:
        break;
    default:
        if (RdrDfsStatusIsRetriable(status))
        {
            if (!pContext->State.Create.OrigStatus)
            {
                pContext->State.Create.OrigStatus = status;
            }
            pContext->Continue = RdrCreateTreeConnected2;
            status = RdrCreateTreeConnect(pContext, pContext->pIrp->Args.Create.FileName.FileName);
        }
        BAIL_ON_NT_STATUS(status);
    }

    status = LwIoAllocateMemory(
        sizeof(RDR_CCB2),
        (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(status);

    status = LwErrnoToNtStatus(pthread_mutex_init(&pFile->mutex, NULL));
    BAIL_ON_NT_STATUS(status);

    pFile->bMutexInitialized = TRUE;
    pFile->version = SMB_PROTOCOL_VERSION_2;
    pFile->pTree = pTree;
    pTree = NULL;

    status = LwRtlWC16StringDuplicate(&pFile->pwszPath, pContext->State.Create.pwszFilename);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringDuplicate(&pFile->pwszCanonicalPath, pContext->State.Create.pwszCanonicalPath);
    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrFinishCreate2;

    pContext->State.Create.pFile2 = pFile;

    status = RdrTransceiveCreate2(
        pContext,
        pFile,
        DesiredAccess,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions);
    if (RdrDfsStatusIsRetriable(status))
    {
        if (!pContext->State.Create.OrigStatus)
        {
            pContext->State.Create.OrigStatus = status;
        }

        RdrReleaseFile2(pFile);
        pContext->State.Create.pFile2 = pFile = NULL;

        pContext->Continue = RdrCreateTreeConnected2;
        status = RdrCreateTreeConnect(pContext, pContext->pIrp->Args.Create.FileName.FileName);
    }
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pContext->State.Create.pwszFilename);

    if (status != STATUS_PENDING)
    {
        RdrFreeContext(pContext);
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
    }

    return FALSE;

error:

    if (status != STATUS_PENDING && pFile)
    {
        RdrReleaseFile2(pFile);
    }

    if (status != STATUS_PENDING && pTree)
    {
        RdrTree2Release(pTree);
    }

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveCreate2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;
    PWSTR pwszPath = RDR_CCB2_PATH(pFile);

    status = RdrAllocateContextPacket(
        pContext,
        RDR_SMB2_CREATE_BASE_SIZE(LwRtlWC16StringNumChars(pwszPath)));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_CREATE,
        RDR_CCB2_IS_DFS(pFile) ? SMB2_FLAGS_DFS_OPERATIONS : 0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeCreateRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        0, /* FIXME: oplock level */
        0x2, /* FIXME: impersonation level */
        desiredAccess,
        fileAttributes,
        shareAccess,
        createDisposition,
        createOptions,
        pwszPath,
        NULL,  /* context offset */
        NULL); /* context length */
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
RdrFinishCreate2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB2 pFile = pContext->State.Create.pFile2;
    PSMB_PACKET pPacket = pParam;
    PRDR_SMB2_CREATE_RESPONSE_HEADER pResponseHeader = NULL;

    if (status == STATUS_SUCCESS)
    {
        status = pPacket->pSMB2Header->error;
    }

    switch (status)
    {
    case STATUS_SUCCESS:
        break;
    case STATUS_PATH_NOT_COVERED:
        pContext->usTry = 0;
        pContext->Continue = RdrChaseCreateReferral2Complete;
        status = RdrDfsChaseReferral2(
            pFile->pTree->pSession,
            IoSecurityGetCredentials(pContext->pIrp->Args.Create.SecurityContext),
            pFile->pwszCanonicalPath,
            pContext);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        if (RdrDfsStatusIsRetriable(status))
        {
            if (!pContext->State.Create.OrigStatus)
            {
                pContext->State.Create.OrigStatus = status;
            }

            RdrReleaseFile2(pFile);
            pContext->State.Create.pFile2 = pFile = NULL;

            pContext->Continue = RdrCreateTreeConnected2;
            status = RdrCreateTreeConnect(pContext, pContext->pIrp->Args.Create.FileName.FileName);
        }
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrSmb2DecodeCreateResponse(pPacket, &pResponseHeader);
    BAIL_ON_NT_STATUS(status);

    pFile->Fid = pResponseHeader->fid;

    status = IoFileSetContext(pContext->pIrp->FileHandle, pFile);
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

    RdrReleaseFile2(pFile);

    goto cleanup;
}

static
BOOLEAN
RdrChaseCreateReferral2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrCreateTreeConnected2;

    status = RdrCreateTreeConnect(pContext, pContext->pIrp->Args.Create.FileName.FileName);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RTL_FREE(&pContext->State.Create.pwszFilename);
        RTL_FREE(&pContext->State.Create.pwszCanonicalPath);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}
