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
 *        lockasync.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Byte range locking (Asynchronous)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
NfsExecuteAsyncLockRequest_SMB_V2(
    PNFS_EXEC_CONTEXT                    pExecContext,
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    );

static
VOID
NfsClearAsyncLocks_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    );

static
VOID
NfsClearAsyncLocks_SMB_V2_inlock(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    );

static
NTSTATUS
NfsBuildAsyncLockResponse_SMB_V2(
    PNFS_EXEC_CONTEXT      pExecContext
    );

static
VOID
NfsCancelAsyncLockState_SMB_V2_inlock(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    );

static
VOID
NfsPrepareAsyncLockStateAsync_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState,
    PNFS_EXEC_CONTEXT                    pExecContext
    );

static
VOID
NfsExecuteAsyncLockContextAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
NfsReleaseAsyncLockStateAsync_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    );

static
VOID
NfsFreeAsyncLockState_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    );

NTSTATUS
NfsBuildAsyncLockState_SMB_V2(
    ULONG64                               ullAsyncId,
    PNFS_EXEC_CONTEXT                     pExecContext,
    PNFS_LOCK_REQUEST_STATE_SMB_V2        pLockRequestState,
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2* ppAsyncLockState
    )
{
    NTSTATUS  ntStatus = STATUS_SUCCESS;
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2),
                    (PVOID*)&pAsyncLockState);
    BAIL_ON_NT_STATUS(ntStatus);

    pAsyncLockState->refCount = 1;

    pthread_mutex_init(&pAsyncLockState->mutex, NULL);
    pAsyncLockState->pMutex = &pAsyncLockState->mutex;

    pAsyncLockState->stage  = NFS_NOTIFY_STAGE_SMB_V2_INITIAL;

    pAsyncLockState->ullAsyncId = ullAsyncId;

    pAsyncLockState->ulTid = pLockRequestState->ulTid;

    *ppAsyncLockState = pAsyncLockState;

cleanup:

    return ntStatus;

error:

    if (pAsyncLockState)
    {
        NfsReleaseAsyncLockState_SMB_V2(pAsyncLockState);
    }

    *ppAsyncLockState = NULL;

    goto cleanup;
}

NTSTATUS
NfsBuildExecContextAsyncLock_SMB_V2(
    PNFS_EXEC_CONTEXT              pExecContext,
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState,
    ULONG64                        ullAsyncId,
    PNFS_EXEC_CONTEXT*             ppExecContextAsync
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection    = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2       = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg           = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
    PNFS_EXEC_CONTEXT pExecContextAsync   = NULL;
    PSMB_PACKET       pSmbRequest2        = NULL;
    PBYTE             pBuffer             = NULL;
    ULONG             ulBytesAvailable    = 0;
    ULONG             ulOffset            = 0;
    ULONG             ulBytesUsed         = 0;
    ULONG             ulTotalBytesUsed    = 0;
    PSMB2_HEADER      pHeader             = NULL; // Do not free
    PSMB2_LOCK_REQUEST_HEADER pLockHeader = NULL; // Do not free
    ULONG             iLock               = 0;

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbRequest2);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    pSmbRequest->ulMessageSize + sizeof(NETBIOS_HEADER),
                    &pSmbRequest2->pRawBuffer,
                    &pSmbRequest2->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2InitPacket(pSmbRequest2, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsBuildExecContext(
                    pConnection,
                    pSmbRequest2,
                    TRUE,
                    &pExecContextAsync);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          = pSmbRequest2->pRawBuffer;
    ulBytesAvailable = pSmbRequest2->bufferLen;

    if (ulBytesAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer          += sizeof(NETBIOS_HEADER);
    ulBytesAvailable -= sizeof(NETBIOS_HEADER);

    ntStatus = SMB2MarshalHeader(
                    pBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_LOCK,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pSmbRequest->pHeader->ulTid,
                    pSmbRequest->pHeader->ullSessionId,
                    ullAsyncId,
                    STATUS_SUCCESS,
                    FALSE,              /* is response */
                    FALSE,              /* chained message */
                    &pHeader,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    if (ulBytesAvailable < sizeof(SMB2_LOCK_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pLockHeader = (PSMB2_LOCK_REQUEST_HEADER)pBuffer;

    pLockHeader->usLength    = pLockRequestState->pRequestHeader->usLength;
    pLockHeader->usLockCount = pLockRequestState->pRequestHeader->usLockCount;
    pLockHeader->ulLockSequence =
                    pLockRequestState->pRequestHeader->ulLockSequence;
    pLockHeader->fid         = pLockRequestState->pRequestHeader->fid;

    pBuffer          += sizeof(SMB2_LOCK_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_LOCK_REQUEST_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_LOCK_REQUEST_HEADER);

    if (ulBytesAvailable < (sizeof(SMB2_LOCK) * (pLockHeader->usLockCount - 1)))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iLock = 0; iLock < pLockHeader->usLockCount; iLock++)
    {
        pLockHeader->locks[iLock] =
                    pLockRequestState->pRequestHeader->locks[iLock];
    }

    // pBuffer          += (sizeof(SMB2_LOCK) * (pLockHeader->usLockCount - 1));
    // ulBytesAvailable -= (sizeof(SMB2_LOCK) * (pLockHeader->usLockCount - 1));
    ulTotalBytesUsed += (sizeof(SMB2_LOCK) * (pLockHeader->usLockCount - 1));

    pSmbRequest2->bufferUsed += ulTotalBytesUsed;

    ntStatus = SMB2MarshalFooter(pSmbRequest2);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppExecContextAsync = pExecContextAsync;

cleanup:

    if (pSmbRequest2)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pSmbRequest2);
    }

    return ntStatus;

error:

    *ppExecContextAsync = NULL;

    if (pExecContextAsync)
    {
        NfsReleaseExecContext(pExecContextAsync);
    }

    goto cleanup;
}

PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2
NfsAcquireAsyncLockState_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    )
{
    InterlockedIncrement(&pAsyncLockState->refCount);

    return pAsyncLockState;
}

NTSTATUS
NfsCancelLock_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection  = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    BOOLEAN                    bInLock      = FALSE;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullAsyncId   = 0LL;
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2     pLockState   = NULL;

    ntStatus = SMB2GetAsyncId(pSmbRequest->pHeader, &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsConnection2FindAsyncState(pConnection, ullAsyncId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pLockState = (PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    NfsCancelAsyncLockState_SMB_V2_inlock(pLockState);

cleanup:

    if (pLockState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pLockState->mutex);
    }

    if (pAsyncState)
    {
        NfsAsyncStateRelease(pAsyncState);
    }

    return ntStatus;

error:

    goto cleanup;
}

VOID
NfsCancelAsyncLockState_SMB_V2(
    HANDLE hLockState
    )
{
    BOOLEAN bInLock = FALSE;
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState =
            (PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2)hLockState;

    LWIO_LOCK_MUTEX(bInLock, &pAsyncLockState->mutex);

    NfsCancelAsyncLockState_SMB_V2_inlock(pAsyncLockState);

    LWIO_UNLOCK_MUTEX(bInLock, &pAsyncLockState->mutex);
}

NTSTATUS
NfsProcessAsyncLockRequest_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection  = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PLWIO_NFS_SESSION_2        pSession     = NULL;
    PLWIO_NFS_TREE_2           pTree        = NULL;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullAsyncId   = 0LL;
    BOOLEAN                    bInLock      = FALSE;
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState = NULL;

    ntStatus = NfsConnection2FindSession_SMB_V2(
                    pCtxSmb2,
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2GetAsyncId(pSmbRequest->pHeader, &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsConnection2FindAsyncState(pConnection, ullAsyncId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pAsyncLockState =
            (PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pAsyncLockState->mutex);

    switch (pAsyncLockState->stage)
    {
        case NFS_LOCK_STAGE_SMB_V2_INITIAL:

            ntStatus = NfsSession2FindTree_SMB_V2(
                            pCtxSmb2,
                            pSession,
                            pAsyncLockState->ulTid,
                            &pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SMB2UnmarshalLockRequest(
                            pSmbRequest,
                            &pAsyncLockState->pRequestHeader);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = NfsTree2FindFile_SMB_V2(
                                pCtxSmb2,
                                pTree,
                                &pAsyncLockState->pRequestHeader->fid,
                                LwIsSetFlag(
                                    pSmbRequest->pHeader->ulFlags,
                                    SMB2_FLAGS_RELATED_OPERATION),
                                &pAsyncLockState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = NfsDetermineLocks_SMB_V2(
                            pAsyncLockState->pRequestHeader,
                            &pAsyncLockState->ppLockArray,
                            &pAsyncLockState->ulNumLocks);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = NfsDetermineUnlocks_SMB_V2(
                            pAsyncLockState->pRequestHeader,
                            &pAsyncLockState->ppUnlockArray,
                            &pAsyncLockState->ulNumUnlocks);
            BAIL_ON_NT_STATUS(ntStatus);

            pAsyncLockState->bFailImmediately = FALSE;

            pCtxSmb2->hState          = pAsyncLockState;
            pCtxSmb2->pfnStateRelease = &NfsReleaseAsyncLockStateHandle_SMB_V2;
            NfsAcquireAsyncLockState_SMB_V2(pAsyncLockState);

            pAsyncLockState->stage = NFS_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK;

            // intentional fall through

        case NFS_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK:

            ntStatus = NfsExecuteAsyncLockRequest_SMB_V2(
                            pExecContext,
                            pAsyncLockState);
            BAIL_ON_NT_STATUS(ntStatus);

            pAsyncLockState->stage = NFS_LOCK_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case NFS_LOCK_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = NfsBuildAsyncLockResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pAsyncLockState->stage = NFS_LOCK_STAGE_SMB_V2_DONE;

            // intentional fall through

        case NFS_LOCK_STAGE_SMB_V2_DONE:

            ntStatus = NfsConnection2RemoveAsyncState(
                                pConnection,
                                pAsyncLockState->ullAsyncId);
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pAsyncLockState->mutex);

    if (pAsyncState)
    {
        NfsAsyncStateRelease(pAsyncState);
    }

    if (pSession)
    {
        NfsSession2Release(pSession);
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

            if (pAsyncLockState)
            {
                NTSTATUS ntStatus1 = STATUS_SUCCESS;

                ntStatus1 = NfsBuildErrorResponse_SMB_V2(
                                pExecContext,
                                pAsyncLockState->ullAsyncId,
                                ntStatus);
                if (ntStatus1 != STATUS_SUCCESS)
                {
                    LWIO_LOG_ERROR( "Failed to build error message for "
                                    "lock request [code:0x%X]",
                                    ntStatus1);
                }

                NfsReleaseAsyncLockStateAsync_SMB_V2(pAsyncLockState);

                if (bInLock)
                {
                    NfsClearAsyncLocks_SMB_V2_inlock(pAsyncLockState);
                }
                else
                {
                    NfsClearAsyncLocks_SMB_V2(pAsyncLockState);
                }

                if (pSession)
                {
                    ntStatus1 = NfsConnection2RemoveAsyncState(
                                        pConnection,
                                        pAsyncLockState->ullAsyncId);
                    if (ntStatus1 != STATUS_SUCCESS)
                    {
                        LWIO_LOG_ERROR( "Failed to remove async lock state"
                                        " from session [code:0x%X]",
                                        ntStatus1);
                    }
                }
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsExecuteAsyncLockRequest_SMB_V2(
    PNFS_EXEC_CONTEXT                    pExecContext,
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pAsyncLockRequestState->bUnlockPending)
    {
        ntStatus = pAsyncLockRequestState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        pAsyncLockRequestState->iUnlock++;
        pAsyncLockRequestState->bUnlockPending = FALSE;
    }

    if (pAsyncLockRequestState->bLockPending)
    {
        ntStatus = pAsyncLockRequestState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        pAsyncLockRequestState->iLock++;
        pAsyncLockRequestState->bLockPending = FALSE;
    }

    // Unlock requests
    for (;  pAsyncLockRequestState->iUnlock < pAsyncLockRequestState->ulNumUnlocks;
            pAsyncLockRequestState->iUnlock++)
    {
        PSMB2_LOCK pLock =
                pAsyncLockRequestState->ppUnlockArray[pAsyncLockRequestState->iUnlock];

        NfsPrepareAsyncLockStateAsync_SMB_V2(pAsyncLockRequestState, pExecContext);

        ntStatus = IoUnlockFile(
                        pAsyncLockRequestState->pFile->hFile,
                        pAsyncLockRequestState->pAcb,
                        &pAsyncLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pAsyncLockRequestState->pRequestHeader->ulLockSequence);
        if (ntStatus == STATUS_PENDING)
        {
            pAsyncLockRequestState->bUnlockPending = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseAsyncLockStateAsync_SMB_V2(pAsyncLockRequestState); // sync
    }

    // Lock requests
    for (;  pAsyncLockRequestState->iLock < pAsyncLockRequestState->ulNumLocks;
            pAsyncLockRequestState->iLock++)
    {
        PSMB2_LOCK pLock =
                pAsyncLockRequestState->ppLockArray[pAsyncLockRequestState->iLock];

        NfsPrepareAsyncLockStateAsync_SMB_V2(pAsyncLockRequestState, pExecContext);

        ntStatus = IoLockFile(
                        pAsyncLockRequestState->pFile->hFile,
                        pAsyncLockRequestState->pAcb,
                        &pAsyncLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pAsyncLockRequestState->pRequestHeader->ulLockSequence,
                        pAsyncLockRequestState->bFailImmediately,
                        LwIsSetFlag(pLock->ulFlags,
                                    SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK));
        if (ntStatus == STATUS_PENDING)
        {
            pAsyncLockRequestState->bLockPending = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseAsyncLockStateAsync_SMB_V2(pAsyncLockRequestState); // sync
    }

error:

    return ntStatus;
}

static
VOID
NfsClearAsyncLocks_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pAsyncLockRequestState->mutex);

    NfsClearAsyncLocks_SMB_V2_inlock(pAsyncLockRequestState);

    LWIO_UNLOCK_MUTEX(bInLock, &pAsyncLockRequestState->mutex);
}

static
VOID
NfsClearAsyncLocks_SMB_V2_inlock(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    )
{
    //
    // Unlock (backwards) locks that have already been acquired
    //
    while (pAsyncLockRequestState->iLock-- > 0)
    {
        NTSTATUS   ntStatus1 = STATUS_SUCCESS;
        PSMB2_LOCK pLock     =
                pAsyncLockRequestState->ppLockArray[pAsyncLockRequestState->iLock];

        pAsyncLockRequestState->pAcb = NULL;

        ntStatus1 = IoUnlockFile(
                        pAsyncLockRequestState->pFile->hFile,
                        pAsyncLockRequestState->pAcb,
                        &pAsyncLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pAsyncLockRequestState->pRequestHeader->ulLockSequence);
        if (ntStatus1)
        {
            LWIO_LOG_ERROR("Failed in unlock. error code [%d]", ntStatus1);
        }
    }

    return;
}

static
NTSTATUS
NfsBuildAsyncLockResponse_SMB_V2(
    PNFS_EXEC_CONTEXT      pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState = NULL;
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    pAsyncLockRequestState =
                (PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM2_LOCK,
                pSmbRequest->pHeader->usEpoch,
                pSmbRequest->pHeader->usCredits,
                pSmbRequest->pHeader->ulPid,
                pSmbRequest->pHeader->ullCommandSequence,
                pCtxSmb2->pTree->ulTid,
                pCtxSmb2->pSession->ullUid,
                pAsyncLockRequestState->ullAsyncId,
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

    ntStatus = SMB2MarshalLockResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
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
NfsCancelAsyncLockState_SMB_V2_inlock(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    )
{
    if (pAsyncLockState->pAcb && pAsyncLockState->pAcb->AsyncCancelContext)
    {
        IoCancelAsyncCancelContext(pAsyncLockState->pAcb->AsyncCancelContext);
    }
}

static
VOID
NfsPrepareAsyncLockStateAsync_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState,
    PNFS_EXEC_CONTEXT                    pExecContext
    )
{
    pAsyncLockRequestState->acb.Callback =
                &NfsExecuteAsyncLockContextAsyncCB_SMB_V2;

    pAsyncLockRequestState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pAsyncLockRequestState->acb.AsyncCancelContext = NULL;

    pAsyncLockRequestState->pAcb = &pAsyncLockRequestState->acb;
}

static
VOID
NfsExecuteAsyncLockContextAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT          pExecContext = (PNFS_EXEC_CONTEXT)pContext;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState = NULL;
    BOOLEAN bInLock = FALSE;

    pAsyncLockRequestState =
            (PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;

    LWIO_LOCK_MUTEX(bInLock, &pAsyncLockRequestState->mutex);

    if (pAsyncLockRequestState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pAsyncLockRequestState->pAcb->AsyncCancelContext);
    }

    pAsyncLockRequestState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pAsyncLockRequestState->mutex);

    ntStatus = NfsProdConsEnqueue(
                    gProtocolGlobals_SMB_V2.pWorkQueue,
                    pExecContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                        ntStatus);

        NfsReleaseExecContext(pExecContext);
    }
}

static
VOID
NfsReleaseAsyncLockStateAsync_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    )
{
    if (pAsyncLockRequestState->pAcb)
    {
        pAsyncLockRequestState->acb.Callback        = NULL;

        if (pAsyncLockRequestState->pAcb->CallbackContext)
        {
            PNFS_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PNFS_EXEC_CONTEXT)pAsyncLockRequestState->pAcb->CallbackContext;

            NfsReleaseExecContext(pExecContext);

            pAsyncLockRequestState->pAcb->CallbackContext = NULL;
        }

        if (pAsyncLockRequestState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pAsyncLockRequestState->pAcb->AsyncCancelContext);
        }

        pAsyncLockRequestState->pAcb = NULL;
    }
}

VOID
NfsReleaseAsyncLockStateHandle_SMB_V2(
    HANDLE hAsyncLockState
    )
{
    NfsReleaseAsyncLockState_SMB_V2((PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2)hAsyncLockState);
}

VOID
NfsReleaseAsyncLockState_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    )
{
    if (InterlockedDecrement(&pAsyncLockState->refCount) == 0)
    {
        NfsFreeAsyncLockState_SMB_V2(pAsyncLockState);
    }
}

static
VOID
NfsFreeAsyncLockState_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    )
{
    if (pAsyncLockState->pAcb && pAsyncLockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pAsyncLockState->pAcb->AsyncCancelContext);
    }

    if (pAsyncLockState->pFile)
    {
        NfsFile2Release(pAsyncLockState->pFile);
    }

    NFS_SAFE_FREE_MEMORY(pAsyncLockState->ppLockArray);
    NFS_SAFE_FREE_MEMORY(pAsyncLockState->ppUnlockArray);

    if (pAsyncLockState->pMutex)
    {
        pthread_mutex_destroy(&pAsyncLockState->mutex);
    }
}
