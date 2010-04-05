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
 *        ioctl.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        IOCTL
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsBuildIOCTLState_SMB_V2(
    PLWIO_NFS_CONNECTION       pConnection,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_NFS_FILE_2           pFile,
    PNFS_IOCTL_STATE_SMB_V2*   ppIOCTLState
    );

static
NTSTATUS
NfsExecuteIOCTL_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildIOCTLResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
VOID
NfsPrepareIOCTLStateAsync_SMB_V2(
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState,
    PNFS_EXEC_CONTEXT       pExecContext
    );

static
VOID
NfsExecuteIOCTLAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
NfsReleaseIOCTLStateAsync_SMB_V2(
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState
    );

static
VOID
NfsReleaseIOCTLStateHandle_SMB_V2(
    HANDLE hIOCTLState
    );

static
VOID
NfsReleaseIOCTLState_SMB_V2(
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState
    );

static
VOID
NfsFreeIOCTLState_SMB_V2(
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState
    );

NTSTATUS
NfsProcessIOCTL_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection   = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;
    PLWIO_NFS_SESSION_2        pSession      = NULL;
    PLWIO_NFS_TREE_2           pTree         = NULL;
    PLWIO_NFS_FILE_2           pFile         = NULL;
    BOOLEAN                    bInLock       = FALSE;

    pIOCTLState = (PNFS_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

    if (pIOCTLState)
    {
        InterlockedIncrement(&pIOCTLState->refCount);
    }
    else
    {
        ULONG                      iMsg           = pCtxSmb2->iMsg;
        PNFS_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
        PSMB2_IOCTL_REQUEST_HEADER pRequestHeader = NULL; // Do not free
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

        ntStatus = SMB2UnmarshalIOCTLRequest(
                        pSmbRequest,
                        &pRequestHeader,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        switch (pRequestHeader->ulFunctionCode)
        {
            case IO_FSCTL_GET_DFS_REFERRALS:

                ntStatus = STATUS_FS_DRIVER_REQUIRED;

                break;

            case IO_FSCTL_PIPE_WAIT:

                ntStatus = STATUS_NOT_SUPPORTED;

                break;

            default:

                ntStatus = NfsTree2FindFile_SMB_V2(
                                pCtxSmb2,
                                pTree,
                                &pRequestHeader->fid,
                                LwIsSetFlag(
                                    pSmbRequest->pHeader->ulFlags,
                                    SMB2_FLAGS_RELATED_OPERATION),
                                &pFile);

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsBuildIOCTLState_SMB_V2(
                        pConnection,
                        pRequestHeader,
                        pData,
                        pFile,
                        &pIOCTLState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pIOCTLState;
        InterlockedIncrement(&pIOCTLState->refCount);
        pCtxSmb2->pfnStateRelease = &NfsReleaseIOCTLStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pIOCTLState->mutex);

    switch (pIOCTLState->stage)
    {
        case NFS_IOCTL_STAGE_SMB_V2_INITIAL:

            pIOCTLState->stage = NFS_IOCTL_STAGE_SMB_V2_ATTEMPT_IO;

            // intentional fall through

        case NFS_IOCTL_STAGE_SMB_V2_ATTEMPT_IO:

            ntStatus = NfsExecuteIOCTL_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pIOCTLState->stage = NFS_IOCTL_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case NFS_IOCTL_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = NfsBuildIOCTLResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pIOCTLState->stage = NFS_IOCTL_STAGE_SMB_V2_DONE;

            // intentional fall through

        case NFS_IOCTL_STAGE_SMB_V2_DONE:

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

    if (pIOCTLState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pIOCTLState->mutex);

        NfsReleaseIOCTLState_SMB_V2(pIOCTLState);
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

            if (pIOCTLState)
            {
                NfsReleaseIOCTLStateAsync_SMB_V2(pIOCTLState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildIOCTLState_SMB_V2(
    PLWIO_NFS_CONNECTION       pConnection,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_NFS_FILE_2           pFile,
    PNFS_IOCTL_STATE_SMB_V2*   ppIOCTLState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_IOCTL_STATE_SMB_V2),
                    (PVOID*)&pIOCTLState);
    BAIL_ON_NT_STATUS(ntStatus);

    pIOCTLState->refCount = 1;

    pthread_mutex_init(&pIOCTLState->mutex, NULL);
    pIOCTLState->pMutex = &pIOCTLState->mutex;

    pIOCTLState->pConnection = NfsConnectionAcquire(pConnection);

    pIOCTLState->pFile = NfsFile2Acquire(pFile);

    pIOCTLState->pRequestHeader = pRequestHeader;
    pIOCTLState->pData          = pData;

    *ppIOCTLState = pIOCTLState;

cleanup:

    return ntStatus;

error:

    *ppIOCTLState = NULL;

    if (pIOCTLState)
    {
        NfsFreeIOCTLState_SMB_V2(pIOCTLState);
    }

    goto cleanup;
}

static
NTSTATUS
NfsExecuteIOCTL_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection   = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;

    pIOCTLState = (PNFS_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pIOCTLState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pIOCTLState->pResponseBuffer)
    {
        // TODO: Should we just allocate 64 * 1024 bytes in this buffer?

        ntStatus = SMBPacketBufferAllocate(
                        pConnection->hPacketAllocator,
                        pIOCTLState->pRequestHeader->ulMaxOutLength,
                        &pIOCTLState->pResponseBuffer,
                        &pIOCTLState->sResponseBufferLen);
        BAIL_ON_NT_STATUS(ntStatus);

        pIOCTLState->ulResponseBufferLen =
            pIOCTLState->pRequestHeader->ulMaxOutLength > 0 ?
            pIOCTLState->pRequestHeader->ulMaxOutLength:
            pIOCTLState->sResponseBufferLen;

        NfsPrepareIOCTLStateAsync_SMB_V2(pIOCTLState, pExecContext);

        if (pIOCTLState->pRequestHeader->ulFlags & 0x1)
        {
            ntStatus = IoFsControlFile(
                                    pIOCTLState->pFile->hFile,
                                    pIOCTLState->pAcb,
                                    &pIOCTLState->ioStatusBlock,
                                    pIOCTLState->pRequestHeader->ulFunctionCode,
                                    pIOCTLState->pData,
                                    pIOCTLState->pRequestHeader->ulInLength,
                                    pIOCTLState->pResponseBuffer,
                                    pIOCTLState->ulResponseBufferLen);
        }
        else
        {
            ntStatus = IoDeviceIoControlFile(
                                    pIOCTLState->pFile->hFile,
                                    pIOCTLState->pAcb,
                                    &pIOCTLState->ioStatusBlock,
                                    pIOCTLState->pRequestHeader->ulFunctionCode,
                                    pIOCTLState->pData,
                                    pIOCTLState->pRequestHeader->ulInLength,
                                    pIOCTLState->pResponseBuffer,
                                    pIOCTLState->ulResponseBufferLen);
        }
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseIOCTLStateAsync_SMB_V2(pIOCTLState); // completed sync
    }

    pIOCTLState->ulResponseBufferLen =
                                    pIOCTLState->ioStatusBlock.BytesTransferred;

cleanup:

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_NOT_SUPPORTED:

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

            break;

        default:

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildIOCTLResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE                     pOutBuffer       = pSmbResponse->pBuffer;
    ULONG                     ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG                     ulOffset         = 0;
    ULONG                     ulBytesUsed      = 0;
    ULONG                     ulTotalBytesUsed = 0;

    pIOCTLState = (PNFS_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_IOCTL,
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

    ntStatus = SMB2MarshalIOCTLResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pIOCTLState->pRequestHeader,
                    pIOCTLState->pResponseBuffer,
                    SMB_MIN(pIOCTLState->ulResponseBufferLen,
                            pIOCTLState->pRequestHeader->ulMaxOutLength),
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

static
VOID
NfsPrepareIOCTLStateAsync_SMB_V2(
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState,
    PNFS_EXEC_CONTEXT       pExecContext
    )
{
    pIOCTLState->acb.Callback        = &NfsExecuteIOCTLAsyncCB_SMB_V2;

    pIOCTLState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pIOCTLState->acb.AsyncCancelContext = NULL;

    pIOCTLState->pAcb = &pIOCTLState->acb;
}

static
VOID
NfsExecuteIOCTLAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT          pExecContext     = (PNFS_EXEC_CONTEXT)pContext;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_IOCTL_STATE_SMB_V2    pIOCTLState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pIOCTLState = (PNFS_IOCTL_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pIOCTLState->mutex);

    if (pIOCTLState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pIOCTLState->pAcb->AsyncCancelContext);
    }

    pIOCTLState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pIOCTLState->mutex);

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
NfsReleaseIOCTLStateAsync_SMB_V2(
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (pIOCTLState->pAcb)
    {
        pIOCTLState->acb.Callback        = NULL;

        if (pIOCTLState->pAcb->CallbackContext)
        {
            PNFS_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PNFS_EXEC_CONTEXT)pIOCTLState->pAcb->CallbackContext;

            NfsReleaseExecContext(pExecContext);

            pIOCTLState->pAcb->CallbackContext = NULL;
        }

        if (pIOCTLState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pIOCTLState->pAcb->AsyncCancelContext);
        }

        pIOCTLState->pAcb = NULL;
    }
}

static
VOID
NfsReleaseIOCTLStateHandle_SMB_V2(
    HANDLE hIOCTLState
    )
{
    return NfsReleaseIOCTLState_SMB_V2((PNFS_IOCTL_STATE_SMB_V2)hIOCTLState);
}

static
VOID
NfsReleaseIOCTLState_SMB_V2(
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (InterlockedDecrement(&pIOCTLState->refCount) == 0)
    {
        NfsFreeIOCTLState_SMB_V2(pIOCTLState);
    }
}

static
VOID
NfsFreeIOCTLState_SMB_V2(
    PNFS_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (pIOCTLState->pAcb && pIOCTLState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pIOCTLState->pAcb->AsyncCancelContext);
    }

    if (pIOCTLState->pFile)
    {
        NfsFile2Release(pIOCTLState->pFile);
    }

    if (pIOCTLState->pResponseBuffer)
    {
        SMBPacketBufferFree(
            pIOCTLState->pConnection->hPacketAllocator,
            pIOCTLState->pResponseBuffer,
            pIOCTLState->sResponseBufferLen);
    }

    if (pIOCTLState->pConnection)
    {
        NfsConnectionRelease(pIOCTLState->pConnection);
    }

    if (pIOCTLState->pMutex)
    {
        pthread_mutex_destroy(&pIOCTLState->mutex);
    }

    NfsFreeMemory(pIOCTLState);
}
