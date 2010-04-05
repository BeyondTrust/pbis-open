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
 *        write.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Write
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsBuildWriteState_SMB_V2(
    PSMB2_WRITE_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_NFS_FILE_2           pFile,
    ULONG                      ulKey,
    PNFS_WRITE_STATE_SMB_V2*   ppWriteState
    );

static
VOID
NfsPrepareWriteStateAsync_SMB_V2(
    PNFS_WRITE_STATE_SMB_V2 pWriteState,
    PNFS_EXEC_CONTEXT       pExecContext
    );

static
VOID
NfsExecuteWriteAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
NfsReleaseWriteStateAsync_SMB_V2(
    PNFS_WRITE_STATE_SMB_V2 pWriteState
    );

static
VOID
NfsReleaseWriteStateHandle_SMB_V2(
    HANDLE hWriteState
    );

static
VOID
NfsReleaseWriteState_SMB_V2(
    PNFS_WRITE_STATE_SMB_V2 pWriteState
    );

static
VOID
NfsFreeWriteState_SMB_V2(
    PNFS_WRITE_STATE_SMB_V2 pWriteState
    );

static
NTSTATUS
NfsBuildWriteResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsProcessWrite_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection  = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PNFS_WRITE_STATE_SMB_V2    pWriteState  = NULL;
    PLWIO_NFS_SESSION_2        pSession     = NULL;
    PLWIO_NFS_TREE_2           pTree        = NULL;
    PLWIO_NFS_FILE_2           pFile        = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pWriteState = (PNFS_WRITE_STATE_SMB_V2)pCtxSmb2->hState;
    if (pWriteState)
    {
        InterlockedIncrement(&pWriteState->refCount);
    }
    else
    {
        ULONG                      iMsg           = pCtxSmb2->iMsg;
        PNFS_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
        PSMB2_WRITE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PBYTE                      pData          = NULL; // Do not free

        ntStatus = NfsConnection2FindSession_SMB_V2(
                        pCtxSmb2,
                        pConnection,
                        pSmbRequest->pHeader->ullSessionId,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsSession2FindTree_SMB_V2(
                        pCtxSmb2,
                        pSession,
                        pSmbRequest->pHeader->ulTid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMB2UnmarshalWriteRequest(
                        pSmbRequest,
                        &pRequestHeader,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsTree2FindFile_SMB_V2(
                            pCtxSmb2,
                            pTree,
                            &pRequestHeader->fid,
                            LwIsSetFlag(
                                pSmbRequest->pHeader->ulFlags,
                                SMB2_FLAGS_RELATED_OPERATION),
                            &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsBuildWriteState_SMB_V2(
                            pRequestHeader,
                            pData,
                            pFile,
                            pSmbRequest->pHeader->ulPid,
                            &pWriteState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pWriteState;
        InterlockedIncrement(&pWriteState->refCount);
        pCtxSmb2->pfnStateRelease = &NfsReleaseWriteStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);

    switch (pWriteState->stage)
    {
        case NFS_WRITE_STAGE_SMB_V2_INITIAL:

            pWriteState->llDataOffset =
                            pWriteState->pRequestHeader->ullFileOffset;

            pWriteState->stage = NFS_WRITE_STAGE_SMB_V2_ATTEMPT_WRITE;

            // intentional fall through

        case NFS_WRITE_STAGE_SMB_V2_ATTEMPT_WRITE:

            pWriteState->stage = NFS_WRITE_STAGE_SMB_V2_BUILD_RESPONSE;

            NfsPrepareWriteStateAsync_SMB_V2(pWriteState, pExecContext);

            ntStatus = IoWriteFile(
                            pWriteState->pFile->hFile,
                            pWriteState->pAcb,
                            &pWriteState->ioStatusBlock,
                            pWriteState->pData,
                            pWriteState->pRequestHeader->ulDataLength,
                            &pWriteState->llDataOffset,
                            &pWriteState->ulKey);
            BAIL_ON_NT_STATUS(ntStatus);

            NfsReleaseWriteStateAsync_SMB_V2(pWriteState); // completed sync

            // intentional fall through

        case NFS_WRITE_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = pWriteState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->ulBytesWritten =
                            pWriteState->ioStatusBlock.BytesTransferred;

            ntStatus = NfsBuildWriteResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->stage = NFS_WRITE_STAGE_SMB_V2_DONE;

            // intentional fall through

        case NFS_WRITE_STAGE_SMB_V2_DONE:

            break;
    }

cleanup:

    if (pFile)
    {
        NfsFile2Release(pFile);
    }

    if (pTree)
    {
        NfsTree2Release(pTree);
    }

    if (pSession)
    {
        NfsSession2Release(pSession);
    }

    if (pWriteState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

        NfsReleaseWriteState_SMB_V2(pWriteState);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed

            break;

        default:

            if (pWriteState)
            {
                NfsReleaseWriteStateAsync_SMB_V2(pWriteState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildWriteState_SMB_V2(
    PSMB2_WRITE_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_NFS_FILE_2           pFile,
    ULONG                      ulKey,
    PNFS_WRITE_STATE_SMB_V2*   ppWriteState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_WRITE_STATE_SMB_V2 pWriteState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_WRITE_STATE_SMB_V2),
                    (PVOID*)&pWriteState);
    BAIL_ON_NT_STATUS(ntStatus);

    pWriteState->refCount = 1;

    pthread_mutex_init(&pWriteState->mutex, NULL);
    pWriteState->pMutex = &pWriteState->mutex;

    pWriteState->stage = NFS_WRITE_STAGE_SMB_V2_INITIAL;

    pWriteState->pRequestHeader = pRequestHeader;
    pWriteState->pData          = pData;

    pWriteState->pFile          = NfsFile2Acquire(pFile);

    pWriteState->ulKey          = ulKey;

    *ppWriteState = pWriteState;

cleanup:

    return ntStatus;

error:

    *ppWriteState = NULL;

    if (pWriteState)
    {
        NfsFreeWriteState_SMB_V2(pWriteState);
    }

    goto cleanup;
}

static
VOID
NfsPrepareWriteStateAsync_SMB_V2(
    PNFS_WRITE_STATE_SMB_V2 pWriteState,
    PNFS_EXEC_CONTEXT       pExecContext
    )
{
    pWriteState->acb.Callback        = &NfsExecuteWriteAsyncCB_SMB_V2;

    pWriteState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pWriteState->acb.AsyncCancelContext = NULL;

    pWriteState->pAcb = &pWriteState->acb;
}

static
VOID
NfsExecuteWriteAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT          pExecContext     = (PNFS_EXEC_CONTEXT)pContext;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_WRITE_STATE_SMB_V2    pWriteState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pWriteState =
        (PNFS_WRITE_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);

    if (pWriteState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pWriteState->pAcb->AsyncCancelContext);
    }

    pWriteState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

    ntStatus = NfsProdConsEnqueue(gProtocolGlobals_SMB_V2.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        NfsReleaseExecContext(pExecContext);
    }
}

static
VOID
NfsReleaseWriteStateAsync_SMB_V2(
    PNFS_WRITE_STATE_SMB_V2 pWriteState
    )
{
    if (pWriteState->pAcb)
    {
        pWriteState->acb.Callback = NULL;

        if (pWriteState->pAcb->CallbackContext)
        {
            PNFS_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PNFS_EXEC_CONTEXT)pWriteState->pAcb->CallbackContext;

            NfsReleaseExecContext(pExecContext);

            pWriteState->pAcb->CallbackContext = NULL;
        }

        if (pWriteState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pWriteState->pAcb->AsyncCancelContext);
        }

        pWriteState->pAcb = NULL;
    }
}

static
VOID
NfsReleaseWriteStateHandle_SMB_V2(
    HANDLE hWriteState
    )
{
    NfsReleaseWriteState_SMB_V2((PNFS_WRITE_STATE_SMB_V2)hWriteState);
}

static
VOID
NfsReleaseWriteState_SMB_V2(
    PNFS_WRITE_STATE_SMB_V2 pWriteState
    )
{
    if (InterlockedDecrement(&pWriteState->refCount) == 0)
    {
        NfsFreeWriteState_SMB_V2(pWriteState);
    }
}

static
VOID
NfsFreeWriteState_SMB_V2(
    PNFS_WRITE_STATE_SMB_V2 pWriteState
    )
{
    if (pWriteState->pAcb && pWriteState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pWriteState->pAcb->AsyncCancelContext);
    }

    if (pWriteState->pFile)
    {
        NfsFile2Release(pWriteState->pFile);
    }

    if (pWriteState->pMutex)
    {
        pthread_mutex_destroy(&pWriteState->mutex);
    }

    NfsFreeMemory(pWriteState);
}

static
NTSTATUS
NfsBuildWriteResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_WRITE_STATE_SMB_V2    pWriteState   = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    pWriteState = (PNFS_WRITE_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_WRITE,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL, /* Async Id */
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

    ntStatus = SMB2MarshalWriteResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pWriteState->ulBytesWritten,
                    (pWriteState->pRequestHeader->ulDataLength -
                     pWriteState->ulBytesWritten),
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulBytesUsed;
    // ulOffset         += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

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
