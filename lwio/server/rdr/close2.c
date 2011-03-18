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
 *        close2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 close code
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveClose2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile
    );

static
BOOLEAN
RdrFinishClose2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

void
RdrReleaseFile2(
    PRDR_CCB2 pFile
    )
{
    if (pFile->pTree)
    {
        RdrTree2Release(pFile->pTree);
    }

    if (pFile->bMutexInitialized)
    {
        pthread_mutex_destroy(&pFile->mutex);
    }

    RTL_FREE(&pFile->pwszPath);
    RTL_FREE(&pFile->pwszCanonicalPath);

    LwIoFreeMemory(pFile);
}

static
VOID
RdrCancelClose2(
    PIRP pIrp,
    PVOID pContext
    )
{
    return;
}

NTSTATUS
RdrClose2(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_CCB2 pFile = IoFileGetContext(pIrp->FileHandle);
    PRDR_OP_CONTEXT pContext = NULL;

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelClose2, pContext);

    pContext->Continue = RdrFinishClose2;

    status = RdrTransceiveClose2(pContext, pFile);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        RdrReleaseFile2(pFile);
        pIrp->IoStatusBlock.Status = STATUS_SUCCESS;
        IoIrpComplete(pIrp);
        RdrFreeContext(pContext);
        status = STATUS_PENDING;
    }

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrFinishClose2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PIRP pIrp = pContext->pIrp;
    PRDR_CCB2 pFile = IoFileGetContext(pIrp->FileHandle);

    RdrFreePacket(pPacket);

    /*
     * We must release the file before completing the IRP.  This guarantees that any tree
     * this file might have been holding open will have its reference count decremented.
     * This satisfies the assumption in RdrShutdown() that all open trees have a reference
     * count of zero.
     */
    RdrReleaseFile2(pFile);

    pIrp->IoStatusBlock.Status = STATUS_SUCCESS;
    IoIrpComplete(pIrp);
    RdrFreeContext(pContext);

    return FALSE;
}

static
NTSTATUS
RdrTransceiveClose2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile
    )
{
    NTSTATUS status = 0;
    PRDR_SOCKET pSocket = pFile->pTree->pSession->pSocket;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_CLOSE_SIZE);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_CLOSE,
        0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeCloseRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        0, /* flags */
        &pFile->Fid);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2FinishCommand(&pContext->Packet, &pCursor, &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

 cleanup:

    return status;

 error:

    goto cleanup;
}
