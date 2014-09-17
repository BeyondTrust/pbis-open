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
 *        write2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 write
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveWrite2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    ULONG64 ullOffset,
    PBYTE pData,
    ULONG ulLength
    );

static
BOOLEAN
RdrFinishWriteChunk2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrFinishWrite2(
    PRDR_OP_CONTEXT pContexts,
    NTSTATUS status,
    PVOID pParam
    );

static
VOID
RdrCancelWrite2(
    PIRP pIrp,
    PVOID _pContext
    )
{
}

NTSTATUS
RdrWrite2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_CCB2 pFile = IoFileGetContext(pIrp->FileHandle);
    PRDR_OP_CONTEXT pContexts = NULL;
    USHORT usOpCount = 0;
    USHORT usIndex = 0;
    ULONG ulRemainder = 0;
    BOOLEAN bLocked = FALSE;
    ULONG ulChunkOffset = 0;
    LONG64 llOffset = 0;
    BOOLEAN bIsPipe = RdrShareIsIpc(pFile->pTree->pwszPath);

    if (pIrp->Args.ReadWrite.ByteOffset)
    {
        llOffset = *pIrp->Args.ReadWrite.ByteOffset;
        pFile->llOffset = llOffset;
    }
    else
    {
        llOffset = pFile->llOffset;
    }

    usOpCount = pIrp->Args.ReadWrite.Length / pFile->pTree->pSession->pSocket->ulMaxWriteSize;
    ulRemainder = pIrp->Args.ReadWrite.Length % pFile->pTree->pSession->pSocket->ulMaxWriteSize;

    if (ulRemainder)
    {
        usOpCount++;
    }

    status = RdrCreateContextArray(
        pIrp,
        usOpCount + 1,
        &pContexts);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelWrite2, pContexts);

    pContexts[0].Continue = RdrFinishWrite2;

    LWIO_LOCK_MUTEX(bLocked, &pFile->mutex);

    for (usIndex = 0; usIndex < usOpCount; usIndex++)
    {
        pContexts[usIndex+1].State.Write2Chunk.usIndex = usIndex+1;
        pContexts[usIndex+1].Continue = RdrFinishWriteChunk2;

        ulChunkOffset = pFile->pTree->pSession->pSocket->ulMaxWriteSize * usIndex;

        if (ulRemainder && usIndex == usOpCount - 1)
        {
            pContexts[usIndex+1].State.Write2Chunk.ulChunkLength = ulRemainder;
        }
        else
        {
            pContexts[usIndex+1].State.Write2Chunk.ulChunkLength =
                pFile->pTree->pSession->pSocket->ulMaxWriteSize;
        }

        status = RdrTransceiveWrite2(
            &pContexts[usIndex+1],
            pFile,
            bIsPipe ?
                0 :
                llOffset + ulChunkOffset,
            ((PBYTE) pIrp->Args.ReadWrite.Buffer) + ulChunkOffset,
            pContexts[usIndex+1].State.Write2Chunk.ulChunkLength);
        if (status == STATUS_PENDING)
        {
            status = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(status);
    }

    status = STATUS_PENDING;

cleanup:

    if (pContexts)
    {
        pContexts[0].State.Write2.usOpCount = usIndex;

        if (status != STATUS_PENDING)
        {
            if (pContexts[0].State.Write2.usOpCount)
            {
                /* If we failed in the middle of queuing requests, we need
                 * to wait for the ones we've dispatched to fail, then complete
                 * the call.  Save the error for later.
                 */
                pContexts[0].State.Write2.Status = status;
            }
            else
            {
                RdrContinueContext(&pContexts[0], status, NULL);
            }
            status = STATUS_PENDING;
        }
    }

    LWIO_UNLOCK_MUTEX(bLocked, &pFile->mutex);

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveWrite2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    ULONG64 ullOffset,
    PBYTE pData,
    ULONG ulLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;
    PULONG pulLength = NULL;

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_WRITE_SIZE(ulLength));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_WRITE,
        0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeWriteRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        ullOffset,
        &pFile->Fid,
        0, /* write caching */
        0, /* flags */
        &pulLength);
    BAIL_ON_NT_STATUS(status);

    status = MarshalData(&pCursor, &ulRemaining, pData, ulLength);
    BAIL_ON_NT_STATUS(status);

    *pulLength = SMB_HTOL32(ulLength);

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
RdrFinishWrite2(
    PRDR_OP_CONTEXT pContexts,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB2 pFile = IoFileGetContext(pContexts[0].pIrp->FileHandle);
    USHORT usIndex = 0;
    ULONG ulTotalLength = 0;

    BAIL_ON_NT_STATUS(status);

    status = pContexts[0].State.Write2.Status;
    BAIL_ON_NT_STATUS(status);

    for (usIndex = 0; usIndex < pContexts[0].State.Write2.usOpCount; usIndex++)
    {
        status = pContexts[usIndex+1].State.Write2Chunk.Status;
        BAIL_ON_NT_STATUS(status);

        if (pContexts[usIndex+1].State.Write2Chunk.ulDataLength <
            pContexts[usIndex+1].State.Write2Chunk.ulChunkLength)
        {
            status = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(status);
        }

        ulTotalLength += pContexts[usIndex+1].State.Write2Chunk.ulDataLength;
    }

    pContexts[0].pIrp->IoStatusBlock.BytesTransferred = ulTotalLength;
    pFile->llOffset += ulTotalLength;

cleanup:

    if (status != STATUS_PENDING)
    {
        pContexts->pIrp->IoStatusBlock.Status = status;


        IoIrpComplete(pContexts->pIrp);
        RdrFreeContextArray(pContexts, pContexts->State.Write2.usOpCount + 1);
    }

    return FALSE;

error:

    goto cleanup;
}

static
BOOLEAN
RdrFinishWriteChunk2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB pFile = IoFileGetContext(pContext->pIrp->FileHandle);
    PSMB_PACKET pPacket = pParam;
    BOOLEAN bLocked = 0;
    PRDR_OP_CONTEXT pMaster = &pContext[-pContext->State.Write2Chunk.usIndex];

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMB2Header->error;
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2DecodeWriteResponse(
        pPacket,
        &pContext->State.Write2Chunk.ulDataLength);
    BAIL_ON_NT_STATUS(status);

    if (pContext->State.Write2Chunk.ulDataLength >
        pContext->State.Write2Chunk.ulChunkLength)
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    RdrFreePacket(pPacket);

    if (status != STATUS_PENDING)
    {
        LWIO_LOCK_MUTEX(bLocked, &pFile->mutex);
        if (++pMaster->State.Write2.usComplete == pMaster->State.Write2.usOpCount)
        {
            RdrContinueContext(pMaster, status, NULL);
        }
        LWIO_UNLOCK_MUTEX(bLocked, &pFile->mutex);
    }

    return FALSE;

error:

    goto cleanup;
}
