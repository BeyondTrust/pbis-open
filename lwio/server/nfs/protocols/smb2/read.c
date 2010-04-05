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
 *        read.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Read
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsBuildReadState_SMB_V2(
    PSMB2_READ_REQUEST_HEADER pRequestHeader,
    PLWIO_NFS_FILE_2          pFile,
    PNFS_READ_STATE_SMB_V2*   ppReadState
    );

static
VOID
NfsPrepareReadStateAsync_SMB_V2(
    PNFS_READ_STATE_SMB_V2 pReadState,
    PNFS_EXEC_CONTEXT      pExecContext
    );

static
VOID
NfsExecuteReadAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
NfsReleaseReadStateAsync_SMB_V2(
    PNFS_READ_STATE_SMB_V2 pReadState
    );

static
VOID
NfsReleaseReadStateHandle_SMB_V2(
    HANDLE hReadState
    );

static
VOID
NfsReleaseReadState_SMB_V2(
    PNFS_READ_STATE_SMB_V2 pReadState
    );

static
VOID
NfsFreeReadState_SMB_V2(
    PNFS_READ_STATE_SMB_V2 pReadState
    );

static
NTSTATUS
NfsBuildReadResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsProcessRead_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection  = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PNFS_READ_STATE_SMB_V2     pReadState   = NULL;
    PLWIO_NFS_SESSION_2        pSession     = NULL;
    PLWIO_NFS_TREE_2           pTree        = NULL;
    PLWIO_NFS_FILE_2           pFile        = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pReadState = (PNFS_READ_STATE_SMB_V2)pCtxSmb2->hState;
    if (pReadState)
    {
        InterlockedIncrement(&pReadState->refCount);
    }
    else
    {
        ULONG                     iMsg           = pCtxSmb2->iMsg;
        PNFS_MESSAGE_SMB_V2       pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
        PSMB2_READ_REQUEST_HEADER pRequestHeader = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalReadRequest(pSmbRequest, &pRequestHeader);
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

        ntStatus = NfsBuildReadState_SMB_V2(
                        pRequestHeader,
                        pFile,
                        &pReadState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pReadState;
        InterlockedIncrement(&pReadState->refCount);
        pCtxSmb2->pfnStateRelease = &NfsReleaseReadStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pReadState->mutex);

    switch (pReadState->stage)
    {
        case NFS_READ_STAGE_SMB_V2_INITIAL:

            pReadState->llByteOffset =
                                pReadState->pRequestHeader->ullFileOffset;

            if (pReadState->pRequestHeader->ulDataLength > 0)
            {
                ntStatus = NfsAllocateMemory(
                            pReadState->pRequestHeader->ulDataLength,
                            (PVOID*)&pReadState->pData);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pReadState->stage = NFS_READ_STAGE_SMB_V2_ATTEMPT_READ;

            // Intentional fall through

        case NFS_READ_STAGE_SMB_V2_ATTEMPT_READ:

            pReadState->stage = NFS_READ_STAGE_SMB_V2_ATTEMPT_READ_COMPLETED;

            NfsPrepareReadStateAsync_SMB_V2(pReadState, pExecContext);

            ntStatus = IoReadFile(
                            pReadState->pFile->hFile,
                            pReadState->pAcb,
                            &pReadState->ioStatusBlock,
                            pReadState->pData,
                            pReadState->pRequestHeader->ulDataLength,
                            &pReadState->llByteOffset,
                            &pReadState->ulKey);
            BAIL_ON_NT_STATUS(ntStatus);

            NfsReleaseReadStateAsync_SMB_V2(pReadState); // completed synchronously

            // intentional fall through

        case NFS_READ_STAGE_SMB_V2_ATTEMPT_READ_COMPLETED:

            ntStatus = pReadState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->ulBytesRead = pReadState->ioStatusBlock.BytesTransferred;
            if (pReadState->ulBytesRead <
                            pReadState->pRequestHeader->ulMinimumCount)
            {
                ntStatus = STATUS_END_OF_FILE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pReadState->stage = NFS_READ_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case NFS_READ_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = NfsBuildReadResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->stage = NFS_READ_STAGE_SMB_V2_DONE;

            // intentional fall through

        case NFS_READ_STAGE_SMB_V2_DONE:

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

    if (pReadState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pReadState->mutex);

        NfsReleaseReadState_SMB_V2(pReadState);
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

        case STATUS_FILE_IS_A_DIRECTORY:

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

            break;

        default:

            if (pReadState)
            {
                NfsReleaseReadStateAsync_SMB_V2(pReadState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildReadState_SMB_V2(
    PSMB2_READ_REQUEST_HEADER pRequestHeader,
    PLWIO_NFS_FILE_2          pFile,
    PNFS_READ_STATE_SMB_V2*   ppReadState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_READ_STATE_SMB_V2 pReadState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_READ_STATE_SMB_V2),
                    (PVOID*)&pReadState);
    BAIL_ON_NT_STATUS(ntStatus);

    pReadState->refCount = 1;

    pthread_mutex_init(&pReadState->mutex, NULL);
    pReadState->pMutex = &pReadState->mutex;

    pReadState->stage = NFS_READ_STAGE_SMB_V2_INITIAL;

    pReadState->pRequestHeader = pRequestHeader;

    pReadState->pFile = NfsFile2Acquire(pFile);

    *ppReadState = pReadState;

cleanup:

    return ntStatus;

error:

    *ppReadState = NULL;

    if (pReadState)
    {
        NfsFreeReadState_SMB_V2(pReadState);
    }

    goto cleanup;
}

static
VOID
NfsPrepareReadStateAsync_SMB_V2(
    PNFS_READ_STATE_SMB_V2 pReadState,
    PNFS_EXEC_CONTEXT      pExecContext
    )
{
    pReadState->acb.Callback        = &NfsExecuteReadAsyncCB_SMB_V2;

    pReadState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pReadState->acb.AsyncCancelContext = NULL;

    pReadState->pAcb = &pReadState->acb;
}

static
VOID
NfsExecuteReadAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT          pExecContext     = (PNFS_EXEC_CONTEXT)pContext;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_READ_STATE_SMB_V2     pReadState       = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pReadState =
        (PNFS_READ_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pReadState->mutex);

    if (pReadState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pReadState->pAcb->AsyncCancelContext);
    }

    pReadState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pReadState->mutex);

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
NfsReleaseReadStateAsync_SMB_V2(
    PNFS_READ_STATE_SMB_V2 pReadState
    )
{
    if (pReadState->pAcb)
    {
        pReadState->acb.Callback = NULL;

        if (pReadState->pAcb->CallbackContext)
        {
            PNFS_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PNFS_EXEC_CONTEXT)pReadState->pAcb->CallbackContext;

            NfsReleaseExecContext(pExecContext);

            pReadState->pAcb->CallbackContext = NULL;
        }

        if (pReadState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pReadState->pAcb->AsyncCancelContext);
        }

        pReadState->pAcb = NULL;
    }
}

static
VOID
NfsReleaseReadStateHandle_SMB_V2(
    HANDLE hReadState
    )
{
    NfsReleaseReadState_SMB_V2((PNFS_READ_STATE_SMB_V2)hReadState);
}

static
VOID
NfsReleaseReadState_SMB_V2(
    PNFS_READ_STATE_SMB_V2 pReadState
    )
{
    if (InterlockedDecrement(&pReadState->refCount) == 0)
    {
        NfsFreeReadState_SMB_V2(pReadState);
    }
}

static
VOID
NfsFreeReadState_SMB_V2(
    PNFS_READ_STATE_SMB_V2 pReadState
    )
{
    if (pReadState->pAcb && pReadState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pReadState->pAcb->AsyncCancelContext);
    }

    if (pReadState->pFile)
    {
        NfsFile2Release(pReadState->pFile);
    }

    if (pReadState->pMutex)
    {
        pthread_mutex_destroy(&pReadState->mutex);
    }

    if (pReadState->pData)
    {
        NfsFreeMemory(pReadState->pData);
    }

    NfsFreeMemory(pReadState);
}

static
NTSTATUS
NfsBuildReadResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PNFS_READ_STATE_SMB_V2     pReadState       = NULL;
    ULONG                      ulDataOffset     = 0L;
    PBYTE                      pOutBuffer       = pSmbResponse->pBuffer;
    ULONG                      ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG                      ulOffset         = 0;
    ULONG                      ulBytesUsed      = 0;
    ULONG                      ulTotalBytesUsed = 0;

    pReadState = (PNFS_READ_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_READ,
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

    ntStatus = SMB2MarshalReadResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    NULL,
                    pReadState->pRequestHeader->ulDataLength,
                    0,
                    &ulDataOffset,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalReadResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pReadState->pData,
                    pReadState->ulBytesRead,
                    pReadState->ulRemaining,
                    &ulDataOffset,
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

