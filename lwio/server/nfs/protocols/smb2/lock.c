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
 *        lock.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Byte range locking
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsValidateLockRequest_SMB_V2(
    PSMB2_LOCK_REQUEST_HEADER pLockRequestHeader
    );

static
NTSTATUS
NfsAscertainIfLocksFailImmediately(
    PSMB2_LOCK* ppUnlockArray,
    ULONG       ulNumUnlocks,
    PSMB2_LOCK* ppLockArray,
    ULONG       ulNumLocks,
    PBOOLEAN    pbFailImmediately
    );

static
NTSTATUS
NfsBuildLockRequestState_SMB_V2(
    PNFS_EXEC_CONTEXT               pExecContext,
    PSMB2_LOCK_REQUEST_HEADER       pLockRequestHeader,
    PLWIO_NFS_TREE_2                pTree,
    PLWIO_NFS_FILE_2                pFile,
    PNFS_LOCK_REQUEST_STATE_SMB_V2* ppLockRequestState
    );

static
NTSTATUS
NfsProcessSyncLockRequest_SMB_V2(
    PNFS_EXEC_CONTEXT              pExecContext,
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
VOID
NfsPrepareSyncLockStateAsync_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState,
    PNFS_EXEC_CONTEXT              pExecContext
    );

static
VOID
NfsExecuteLockContextAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
NfsReleaseSyncLockStateAsync_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
VOID
NfsClearLocks_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
VOID
NfsClearLocks_SMB_V2_inlock(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
NTSTATUS
NfsBuildLockResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
VOID
NfsReleaseLockRequestStateHandle_SMB_V2(
    HANDLE hLockRequestState
    );

static
VOID
NfsReleaseLockRequestState_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
VOID
NfsFreeLockRequestState_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

NTSTATUS
NfsProcessLock_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection    = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2       = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg           = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
    PSMB2_LOCK_REQUEST_HEADER  pRequestHeader = NULL; // Do not free
    PLWIO_NFS_SESSION_2        pSession       = NULL;
    PLWIO_NFS_TREE_2           pTree          = NULL;
    PLWIO_NFS_FILE_2           pFile          = NULL;
    BOOLEAN                    bInLock        = FALSE;
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState = NULL;
    PNFS_EXEC_CONTEXT              pExecContextAsync = NULL;
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2         pAsyncLockState   = NULL;
    PLWIO_ASYNC_STATE              pAsyncState       = NULL;
    BOOLEAN                        bUnregisterAsync  = FALSE;

    pLockRequestState = (PNFS_LOCK_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;

    if (pLockRequestState)
    {
        InterlockedIncrement(&pLockRequestState->refCount);
    }
    else
    {
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

        ntStatus = SMB2UnmarshalLockRequest(pSmbRequest, &pRequestHeader);
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

        ntStatus = NfsBuildLockRequestState_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pTree,
                            pFile,
                            &pLockRequestState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pLockRequestState;
        pCtxSmb2->pfnStateRelease = &NfsReleaseLockRequestStateHandle_SMB_V2;
        InterlockedIncrement(&pLockRequestState->refCount);
    }

    LWIO_LOCK_MUTEX(bInLock, &pLockRequestState->mutex);

    switch (pLockRequestState->stage)
    {
        case NFS_LOCK_STAGE_SMB_V2_INITIAL:

            //
            // If all we have are unlocks, don't make this asynchronous
            //
            if (!pLockRequestState->bFailImmediately &&
                (pLockRequestState->ulNumLocks > 0))
            {
                // An asynchronous lock request can occur as the only message in
                // a request, or the last in a chained request
                if (pCtxSmb2->iMsg != (pCtxSmb2->ulNumRequests - 1))
                {
                    ntStatus = STATUS_INTERNAL_ERROR;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                ntStatus = NfsConnection2CreateAsyncState(
                                pConnection,
                                COM2_LOCK,
                                &NfsCancelAsyncLockState_SMB_V2,
                                &NfsReleaseAsyncLockStateHandle_SMB_V2,
                                &pAsyncState);
                BAIL_ON_NT_STATUS(ntStatus);

                bUnregisterAsync = TRUE;

                ntStatus = NfsBuildAsyncLockState_SMB_V2(
                                pAsyncState->ullAsyncId,
                                pExecContext,
                                pLockRequestState,
                                &pAsyncLockState);
                BAIL_ON_NT_STATUS(ntStatus);

                pAsyncState->hAsyncState =
                            NfsAcquireAsyncLockState_SMB_V2(pAsyncLockState);
            }

            pLockRequestState->stage = NFS_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK;

            // intentional fall through

        case NFS_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK:

            if (pAsyncLockState)
            {
                ntStatus = NfsBuildInterimResponse_SMB_V2(
                                    pExecContext,
                                    pAsyncLockState->ullAsyncId);
                BAIL_ON_NT_STATUS(ntStatus);

                bUnregisterAsync = FALSE;

                ntStatus = NfsBuildExecContextAsyncLock_SMB_V2(
                                pExecContext,
                                pLockRequestState,
                                pAsyncLockState->ullAsyncId,
                                &pExecContextAsync);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = NfsProdConsEnqueue(
                                gProtocolGlobals_SMB_V2.pWorkQueue,
                                pExecContextAsync);
                BAIL_ON_NT_STATUS(ntStatus);

                pExecContextAsync = NULL;

                ntStatus = STATUS_PENDING;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                ntStatus = NfsProcessSyncLockRequest_SMB_V2(
                                pExecContext,
                                pLockRequestState);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pLockRequestState->stage = NFS_LOCK_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case NFS_LOCK_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = NfsBuildLockResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pLockRequestState->stage = NFS_LOCK_STAGE_SMB_V2_DONE;

            // intentional fall through

        case NFS_LOCK_STAGE_SMB_V2_DONE:

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

    if (pLockRequestState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pLockRequestState->mutex);

        NfsReleaseLockRequestState_SMB_V2(pLockRequestState);
    }

    if (pAsyncLockState)
    {
        if (bUnregisterAsync)
        {
            NfsConnection2RemoveAsyncState(
                    pConnection,
                    pAsyncLockState->ullAsyncId);
        }

        NfsReleaseAsyncLockState_SMB_V2(pAsyncLockState);
    }

    if (pAsyncState)
    {
        NfsAsyncStateRelease(pAsyncState);
    }

    if (pExecContextAsync)
    {
        NfsReleaseExecContext(pExecContextAsync);
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

            if (pLockRequestState)
            {
                NfsReleaseSyncLockStateAsync_SMB_V2(pLockRequestState);

                if (bInLock)
                {
                    NfsClearLocks_SMB_V2_inlock(pLockRequestState);
                }
                else
                {
                    NfsClearLocks_SMB_V2(pLockRequestState);
                }
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildLockRequestState_SMB_V2(
    PNFS_EXEC_CONTEXT               pExecContext,
    PSMB2_LOCK_REQUEST_HEADER       pLockRequestHeader,
    PLWIO_NFS_TREE_2                pTree,
    PLWIO_NFS_FILE_2                pFile,
    PNFS_LOCK_REQUEST_STATE_SMB_V2* ppLockRequestState
    )
{
    NTSTATUS                       ntStatus          = STATUS_SUCCESS;
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState = NULL;

    ntStatus = NfsAllocateMemory(
                        sizeof(NFS_LOCK_REQUEST_STATE_SMB_V2),
                        (PVOID*)&pLockRequestState);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pLockRequestState->mutex, NULL);
    pLockRequestState->pMutex = &pLockRequestState->mutex;

    pLockRequestState->refCount = 1;

    pLockRequestState->stage = NFS_LOCK_STAGE_SMB_V2_INITIAL;

    ntStatus = NfsValidateLockRequest_SMB_V2(pLockRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    pLockRequestState->ulTid          = pTree->ulTid;
    pLockRequestState->pFile          = NfsFile2Acquire(pFile);
    pLockRequestState->pRequestHeader = pLockRequestHeader;

    ntStatus = NfsDetermineLocks_SMB_V2(
                    pLockRequestHeader,
                    &pLockRequestState->ppLockArray,
                    &pLockRequestState->ulNumLocks);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsDetermineUnlocks_SMB_V2(
                    pLockRequestHeader,
                    &pLockRequestState->ppUnlockArray,
                    &pLockRequestState->ulNumUnlocks);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsAscertainIfLocksFailImmediately(
                    pLockRequestState->ppUnlockArray,
                    pLockRequestState->ulNumUnlocks,
                    pLockRequestState->ppLockArray,
                    pLockRequestState->ulNumLocks,
                    &pLockRequestState->bFailImmediately);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppLockRequestState = pLockRequestState;

cleanup:

    return ntStatus;

error:

    *ppLockRequestState = NULL;

    if (pLockRequestState)
    {
        NfsReleaseLockRequestState_SMB_V2(pLockRequestState);
    }

    goto cleanup;
}

static
NTSTATUS
NfsValidateLockRequest_SMB_V2(
    PSMB2_LOCK_REQUEST_HEADER pLockRequestHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT      iLock            = 0;

    for (; iLock < pLockRequestHeader->usLockCount; iLock++)
    {
        PSMB2_LOCK pLock = &pLockRequestHeader->locks[iLock];

        if ((pLock->ullFileOffset == UINT64_MAX) &&
            (pLock->ullByteRange == UINT64_MAX))
        {
            ntStatus = STATUS_INVALID_LOCK_RANGE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        // Only one of these must be set
        switch (pLock->ulFlags & (SMB2_LOCK_FLAGS_SHARED_LOCK |
                                  SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK |
                                  SMB2_LOCK_FLAGS_UNLOCK))
        {
            case SMB2_LOCK_FLAGS_SHARED_LOCK:
            case SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK:
            case SMB2_LOCK_FLAGS_UNLOCK:

                break;

            default:

                ntStatus = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(ntStatus);
        }
    }

error:

    return ntStatus;
}

NTSTATUS
NfsDetermineLocks_SMB_V2(
    PSMB2_LOCK_REQUEST_HEADER pLockRequestHeader,
    PSMB2_LOCK**              pppLockArray,
    PULONG                    pulNumLocks
    )
{
    NTSTATUS    ntStatus         = STATUS_SUCCESS;
    USHORT      iLock            = 0;
    USHORT      usNumLocks       = 0;
    PSMB2_LOCK* ppLockArray      = NULL;

    for (; iLock < pLockRequestHeader->usLockCount; iLock++)
    {
        PSMB2_LOCK pLock = &pLockRequestHeader->locks[iLock];

        if (!LwIsSetFlag(pLock->ulFlags,SMB2_LOCK_FLAGS_UNLOCK))
        {
            usNumLocks++;
        }
    }

    if (usNumLocks)
    {
        ULONG iLock2 = 0;

        ntStatus = NfsAllocateMemory(
                        sizeof(PSMB2_LOCK*) * usNumLocks,
                        (PVOID*)&ppLockArray);
        BAIL_ON_NT_STATUS(ntStatus);

        for (iLock = 0; iLock < pLockRequestHeader->usLockCount; iLock++)
        {
            PSMB2_LOCK pLock = &pLockRequestHeader->locks[iLock];

            if (!LwIsSetFlag(pLock->ulFlags,SMB2_LOCK_FLAGS_UNLOCK))
            {
                ppLockArray[iLock2++] = pLock;
            }
        }
    }

    *pppLockArray      = ppLockArray;
    *pulNumLocks       = usNumLocks;

cleanup:

    return ntStatus;

error:

    if (ppLockArray)
    {
        NfsFreeMemory(ppLockArray);
    }

    *pppLockArray      = NULL;
    *pulNumLocks       = 0;

    goto cleanup;
}

NTSTATUS
NfsDetermineUnlocks_SMB_V2(
    PSMB2_LOCK_REQUEST_HEADER pLockRequestHeader,
    PSMB2_LOCK**              pppUnlockArray,
    PULONG                    pulNumUnlocks
    )
{
    NTSTATUS    ntStatus         = STATUS_SUCCESS;
    USHORT      iLock            = 0;
    USHORT      usNumUnlocks     = 0;
    PSMB2_LOCK* ppUnlockArray    = NULL;

    for (; iLock < pLockRequestHeader->usLockCount; iLock++)
    {
        PSMB2_LOCK pLock = &pLockRequestHeader->locks[iLock];

        if (LwIsSetFlag(pLock->ulFlags,SMB2_LOCK_FLAGS_UNLOCK))
        {
            usNumUnlocks++;
        }
    }

    if (usNumUnlocks)
    {
        ULONG iLock2 = 0;

        ntStatus = NfsAllocateMemory(
                        sizeof(PSMB2_LOCK*) * usNumUnlocks,
                        (PVOID*)&ppUnlockArray);
        BAIL_ON_NT_STATUS(ntStatus);

        for (iLock = 0; iLock < pLockRequestHeader->usLockCount; iLock++)
        {
            PSMB2_LOCK pLock = &pLockRequestHeader->locks[iLock];

            if (LwIsSetFlag(pLock->ulFlags,SMB2_LOCK_FLAGS_UNLOCK))
            {
                ppUnlockArray[iLock2++] = pLock;
            }
        }
    }

    *pppUnlockArray    = ppUnlockArray;
    *pulNumUnlocks     = usNumUnlocks;

cleanup:

    return ntStatus;

error:

    if (ppUnlockArray)
    {
        NfsFreeMemory(ppUnlockArray);
    }

    *pppUnlockArray    = NULL;
    *pulNumUnlocks     = 0;

    goto cleanup;
}

static
NTSTATUS
NfsAscertainIfLocksFailImmediately(
    PSMB2_LOCK* ppUnlockArray,
    ULONG       ulNumUnlocks,
    PSMB2_LOCK* ppLockArray,
    ULONG       ulNumLocks,
    PBOOLEAN    pbFailImmediately
    )
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    BOOLEAN  bFailImmediately = FALSE;
    ULONG    iLock            = 0;

    for (; iLock < ulNumLocks; iLock++)
    {
        if (LwIsSetFlag(ppLockArray[iLock]->ulFlags,
                        SMB2_LOCK_FLAGS_FAIL_IMMEDIATELY))
        {
            bFailImmediately = TRUE;
            break;
        }
    }

    for (iLock = 0; iLock < ulNumUnlocks; iLock++)
    {
        if (LwIsSetFlag(ppUnlockArray[iLock]->ulFlags,
                        SMB2_LOCK_FLAGS_FAIL_IMMEDIATELY))
        {
            bFailImmediately = TRUE;
            break;
        }
    }

    *pbFailImmediately = bFailImmediately;

    return ntStatus;
}

static
NTSTATUS
NfsProcessSyncLockRequest_SMB_V2(
    PNFS_EXEC_CONTEXT              pExecContext,
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pLockRequestState->bUnlockPending)
    {
        ntStatus = pLockRequestState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        pLockRequestState->iUnlock++;
        pLockRequestState->bUnlockPending = FALSE;
    }

    if (pLockRequestState->bLockPending)
    {
        ntStatus = pLockRequestState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        pLockRequestState->iLock++;
        pLockRequestState->bLockPending = FALSE;
    }

    // Unlock requests
    for (;  pLockRequestState->iUnlock < pLockRequestState->ulNumUnlocks;
            pLockRequestState->iUnlock++)
    {
        PSMB2_LOCK pLock =
                pLockRequestState->ppUnlockArray[pLockRequestState->iUnlock];

        NfsPrepareSyncLockStateAsync_SMB_V2(pLockRequestState, pExecContext);

        ntStatus = IoUnlockFile(
                        pLockRequestState->pFile->hFile,
                        pLockRequestState->pAcb,
                        &pLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pLockRequestState->pRequestHeader->ulLockSequence);
        if (ntStatus == STATUS_PENDING)
        {
            pLockRequestState->bUnlockPending = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseSyncLockStateAsync_SMB_V2(pLockRequestState); // completed sync
    }

    // Lock requests
    for (;  pLockRequestState->iLock < pLockRequestState->ulNumLocks;
            pLockRequestState->iLock++)
    {
        PSMB2_LOCK pLock =
                pLockRequestState->ppLockArray[pLockRequestState->iLock];

        NfsPrepareSyncLockStateAsync_SMB_V2(pLockRequestState, pExecContext);

        ntStatus = IoLockFile(
                        pLockRequestState->pFile->hFile,
                        pLockRequestState->pAcb,
                        &pLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pLockRequestState->pRequestHeader->ulLockSequence,
                        pLockRequestState->bFailImmediately,
                        LwIsSetFlag(pLock->ulFlags,
                                    SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK));
        if (ntStatus == STATUS_PENDING)
        {
            pLockRequestState->bLockPending = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseSyncLockStateAsync_SMB_V2(pLockRequestState); // completed sync
    }

error:

    return ntStatus;
}

static
VOID
NfsPrepareSyncLockStateAsync_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState,
    PNFS_EXEC_CONTEXT              pExecContext
    )
{
    pLockRequestState->acb.Callback = &NfsExecuteLockContextAsyncCB_SMB_V2;

    pLockRequestState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pLockRequestState->acb.AsyncCancelContext = NULL;

    pLockRequestState->pAcb = &pLockRequestState->acb;
}

static
VOID
NfsExecuteLockContextAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT          pExecContext = (PNFS_EXEC_CONTEXT)pContext;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState =
                            (PNFS_LOCK_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pLockRequestState->mutex);

    if (pLockRequestState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pLockRequestState->pAcb->AsyncCancelContext);
    }

    pLockRequestState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequestState->mutex);

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
NfsReleaseSyncLockStateAsync_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    if (pLockRequestState->pAcb)
    {
        pLockRequestState->acb.Callback        = NULL;

        if (pLockRequestState->pAcb->CallbackContext)
        {
            PNFS_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PNFS_EXEC_CONTEXT)pLockRequestState->pAcb->CallbackContext;

            NfsReleaseExecContext(pExecContext);

            pLockRequestState->pAcb->CallbackContext = NULL;
        }

        if (pLockRequestState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pLockRequestState->pAcb->AsyncCancelContext);
        }

        pLockRequestState->pAcb = NULL;
    }
}

static
VOID
NfsClearLocks_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pLockRequestState->mutex);

    NfsClearLocks_SMB_V2_inlock(pLockRequestState);

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequestState->mutex);
}

static
VOID
NfsClearLocks_SMB_V2_inlock(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    //
    // Unlock (backwards) locks that have already been acquired
    //
    while (pLockRequestState->iLock-- > 0)
    {
        NTSTATUS   ntStatus1 = STATUS_SUCCESS;
        PSMB2_LOCK pLock     =
                    pLockRequestState->ppLockArray[pLockRequestState->iLock];

        pLockRequestState->pAcb = NULL;

        ntStatus1 = IoUnlockFile(
                        pLockRequestState->pFile->hFile,
                        pLockRequestState->pAcb,
                        &pLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pLockRequestState->pRequestHeader->ulLockSequence);
        if (ntStatus1)
        {
            LWIO_LOG_ERROR("Failed in unlock. error code [%d]", ntStatus1);
        }
    }

    return;
}

static
NTSTATUS
NfsBuildLockResponse_SMB_V2(
    PNFS_EXEC_CONTEXT      pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

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
NfsReleaseLockRequestStateHandle_SMB_V2(
    HANDLE hLockRequestState
    )
{
    return NfsReleaseLockRequestState_SMB_V2(
                (PNFS_LOCK_REQUEST_STATE_SMB_V2)hLockRequestState);
}

static
VOID
NfsReleaseLockRequestState_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    if (InterlockedDecrement(&pLockRequestState->refCount) == 0)
    {
        NfsFreeLockRequestState_SMB_V2(pLockRequestState);
    }
}

static
VOID
NfsFreeLockRequestState_SMB_V2(
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    if (pLockRequestState->pAcb && pLockRequestState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pLockRequestState->pAcb->AsyncCancelContext);
    }

    if (pLockRequestState->pFile)
    {
        NfsFile2Release(pLockRequestState->pFile);
    }

    NFS_SAFE_FREE_MEMORY(pLockRequestState->ppLockArray);
    NFS_SAFE_FREE_MEMORY(pLockRequestState->ppUnlockArray);

    if (pLockRequestState->pMutex)
    {
        pthread_mutex_destroy(&pLockRequestState->mutex);
    }
}


