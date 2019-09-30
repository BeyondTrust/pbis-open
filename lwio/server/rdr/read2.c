/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Module Name:
 *
 *        smb2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        Common SMB2 Code
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveRead2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    ULONG64 ullOffset,
    ULONG ulLength
    );

static
BOOLEAN
RdrFinishReadChunk2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrFinishRead2(
    PRDR_OP_CONTEXT pContexts,
    NTSTATUS status,
    PVOID pParam
    );

static
VOID
RdrCancelRead2(
    PIRP pIrp,
    PVOID _pContext
    )
{
}

NTSTATUS
RdrRead2(
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

    usOpCount = pIrp->Args.ReadWrite.Length / pFile->pTree->pSession->pSocket->ulMaxReadSize;
    ulRemainder = pIrp->Args.ReadWrite.Length % pFile->pTree->pSession->pSocket->ulMaxReadSize;

    if (ulRemainder)
    {
        usOpCount++;
    }

    status = RdrCreateContextArray(
        pIrp,
        usOpCount + 1,
        &pContexts);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelRead2, pContexts);

    pContexts[0].Continue = RdrFinishRead2;

    LWIO_LOCK_MUTEX(bLocked, &pFile->mutex);

    for (usIndex = 0; usIndex < usOpCount; usIndex++)
    {
        pContexts[usIndex+1].State.Read2Chunk.usIndex = usIndex+1;
        pContexts[usIndex+1].Continue = RdrFinishReadChunk2;

        pContexts[usIndex+1].State.Read2Chunk.ulChunkOffset =
            pFile->pTree->pSession->pSocket->ulMaxReadSize * usIndex;

        if (ulRemainder && usIndex == usOpCount - 1)
        {
            pContexts[usIndex+1].State.Read2Chunk.ulChunkLength = ulRemainder;
        }
        else
        {
            pContexts[usIndex+1].State.Read2Chunk.ulChunkLength =
                pFile->pTree->pSession->pSocket->ulMaxReadSize;
        }

        status = RdrTransceiveRead2(
            &pContexts[usIndex+1],
            pFile,
            bIsPipe ?
                0 :
                pContexts[usIndex+1].State.Read2Chunk.ulChunkOffset + llOffset,
            pContexts[usIndex+1].State.Read2Chunk.ulChunkLength);
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
        pContexts[0].State.Read2.usOpCount = usIndex;

        if (status != STATUS_PENDING)
        {
            if (pContexts[0].State.Read2.usOpCount)
            {
                /* If we failed in the middle of queuing requests, we need
                 * to wait for the ones we've dispatched to fail, then complete
                 * the call.  Save the error for later.
                 */
                pContexts[0].State.Read2.Status = status;
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
RdrTransceiveRead2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    ULONG64 ullOffset,
    ULONG ulLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_READ_SIZE);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_READ,
        0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeReadRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        ulLength,
        ullOffset,
        &pFile->Fid,
        0, /* minimum count */
        0); /* readahead */
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
RdrFinishRead2(
    PRDR_OP_CONTEXT pContexts,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB2 pFile = IoFileGetContext(pContexts[0].pIrp->FileHandle);
    USHORT usIndex = 0;
    BOOLEAN bExpectEof = FALSE;
    ULONG ulTotalLength = 0;

    BAIL_ON_NT_STATUS(status);

    status = pContexts[0].State.Read2.Status;
    BAIL_ON_NT_STATUS(status);

    for (usIndex = 0; usIndex < pContexts[0].State.Read2.usOpCount; usIndex++)
    {
        status = pContexts[usIndex+1].State.Read2Chunk.Status;
        switch (status)
        {
        case STATUS_SUCCESS:
            if (bExpectEof)
            {
                status = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(status);
            }
            break;
        case STATUS_END_OF_FILE:
            bExpectEof = TRUE;
            status = STATUS_SUCCESS;
            break;
        default:
            BAIL_ON_NT_STATUS(status);
        }

        if (pContexts[usIndex+1].State.Read2Chunk.ulDataLength <
            pFile->pTree->pSession->pSocket->ulMaxReadSize)
        {
            bExpectEof = TRUE;
        }

        ulTotalLength += pContexts[usIndex+1].State.Read2Chunk.ulDataLength;
    }

    if (ulTotalLength == 0)
    {
        status = STATUS_END_OF_FILE;
        BAIL_ON_NT_STATUS(status);
    }

    pFile->llOffset += (LONG64) ulTotalLength;
    pContexts[0].pIrp->IoStatusBlock.BytesTransferred = ulTotalLength;

cleanup:

    if (status != STATUS_PENDING)
    {
        pContexts->pIrp->IoStatusBlock.Status = status;

        IoIrpComplete(pContexts->pIrp);
        RdrFreeContextArray(pContexts, pContexts->State.Read2.usOpCount + 1);
    }

    return FALSE;

error:

    goto cleanup;
}

static
BOOLEAN
RdrFinishReadChunk2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB pFile = IoFileGetContext(pContext->pIrp->FileHandle);
    PSMB_PACKET pPacket = pParam;
    PBYTE pData = NULL;
    BOOLEAN bLocked = 0;
    PRDR_OP_CONTEXT pMaster = &pContext[-pContext->State.Read2Chunk.usIndex];
    BOOLEAN bResult = FALSE;

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMB2Header->error;
    switch (status)
    {
    case STATUS_PENDING:
        /* Interim response -- remain queued to receive actual response */
        bResult = TRUE;
    }
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2DecodeReadResponse(
        pPacket,
        &pData,
        &pContext->State.Read2Chunk.ulDataLength);
    BAIL_ON_NT_STATUS(status);

    if (pContext->State.Read2Chunk.ulDataLength >
        pContext->State.Read2Chunk.ulChunkLength)
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(
        (PBYTE) pContext->pIrp->Args.ReadWrite.Buffer +
        pContext->State.Read2Chunk.ulChunkOffset,
        pData,
        pContext->State.Read2Chunk.ulDataLength);

cleanup:

    RdrFreePacket(pPacket);

    if (status != STATUS_PENDING)
    {
        LWIO_LOCK_MUTEX(bLocked, &pFile->mutex);
        if (++pMaster->State.Read2.usComplete == pMaster->State.Read2.usOpCount)
        {
            RdrContinueContext(pMaster, status, NULL);
        }
        LWIO_UNLOCK_MUTEX(bLocked, &pFile->mutex);
    }

    return bResult;

error:

    goto cleanup;
}
