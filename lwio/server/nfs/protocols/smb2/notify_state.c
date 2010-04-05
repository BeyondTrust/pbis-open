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
 *        notify.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Change Notify
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
NfsNotifyAsyncCB_SMB_V2(
    PVOID pContext
    );

static
NTSTATUS
NfsNotifyBuildExecContext_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState,
    PNFS_EXEC_CONTEXT*       ppExecContext
    );

static
VOID
NfsNotifyStateFree_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    );

NTSTATUS
NfsNotifyCreateState_SMB_V2(
    ULONG64                   ullAsyncId,
    PLWIO_NFS_CONNECTION      pConnection,
    PLWIO_NFS_SESSION_2       pSession,
    PLWIO_NFS_TREE_2          pTree,
    PLWIO_NFS_FILE_2          pFile,
    USHORT                    usEpoch,
    ULONG64                   ullCommandSequence,
    ULONG                     ulPid,
    ULONG                     ulCompletionFilter,
    BOOLEAN                   bWatchTree,
    ULONG                     ulMaxBufferSize,
    PNFS_NOTIFY_STATE_SMB_V2* ppNotifyState
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_NOTIFY_STATE_SMB_V2),
                    (PVOID*)&pNotifyState);
    BAIL_ON_NT_STATUS(ntStatus);

    pNotifyState->refCount = 1;

    pthread_mutex_init(&pNotifyState->mutex, NULL);
    pNotifyState->pMutex = &pNotifyState->mutex;

    pNotifyState->ullAsyncId = ullAsyncId;

    pNotifyState->pConnection = NfsConnectionAcquire(pConnection);
    pNotifyState->pFile       = NfsFile2Acquire(pFile);

    pNotifyState->ulCompletionFilter = ulCompletionFilter;
    pNotifyState->bWatchTree         = bWatchTree;

    pNotifyState->usEpoch      = usEpoch;
    pNotifyState->ullSessionId = pSession->ullUid;
    pNotifyState->ulTid        = pTree->ulTid;
    pNotifyState->ulPid        = ulPid;

    pNotifyState->ullCommandSequence = ullCommandSequence;

    pNotifyState->ulMaxBufferSize   = ulMaxBufferSize;

    if (ulMaxBufferSize)
    {
        ntStatus = NfsAllocateMemory(
                        ulMaxBufferSize,
                        (PVOID*)&pNotifyState->pBuffer);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNotifyState->ulBufferLength = ulMaxBufferSize;

    *ppNotifyState = pNotifyState;

cleanup:

    return ntStatus;

error:

    *ppNotifyState = NULL;

    if (pNotifyState)
    {
        NfsNotifyStateFree_SMB_V2(pNotifyState);
    }

    goto cleanup;
}

VOID
NfsPrepareNotifyStateAsync_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    )
{
    pNotifyState->acb.Callback        = &NfsNotifyAsyncCB_SMB_V2;

    pNotifyState->acb.CallbackContext = pNotifyState;
    InterlockedIncrement(&pNotifyState->refCount);

    pNotifyState->acb.AsyncCancelContext = NULL;

    pNotifyState->pAcb = &pNotifyState->acb;
}

static
VOID
NfsNotifyAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS          ntStatus     = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT pExecContext = NULL;
    BOOLEAN           bInLock      = FALSE;
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState =
                            (PNFS_NOTIFY_STATE_SMB_V2)pContext;

    LWIO_LOCK_MUTEX(bInLock, &pNotifyState->mutex);

    if (pNotifyState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pNotifyState->pAcb->AsyncCancelContext);
    }

    pNotifyState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pNotifyState->mutex);

    ntStatus = NfsNotifyBuildExecContext_SMB_V2(
                    pNotifyState,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsProdConsEnqueue(
                    gProtocolGlobals_SMB_V2.pWorkQueue,
                    pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pExecContext = NULL;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pNotifyState->mutex);

    if (pNotifyState)
    {
        NfsNotifyStateRelease_SMB_V2(pNotifyState);
    }

    if (pExecContext)
    {
        NfsReleaseExecContext(pExecContext);
    }

    return;

error:

    LWIO_LOG_ERROR("Error: Failed processing change notify call back "
                   "[status:0x%x]",
                   ntStatus);

    // TODO: indicate error on file handle somehow

    goto cleanup;
}

static
NTSTATUS
NfsNotifyBuildExecContext_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState,
    PNFS_EXEC_CONTEXT*       ppExecContext
    )
{
    NTSTATUS                 ntStatus         = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT        pExecContext     = NULL;
    PSMB_PACKET              pSmbRequest      = NULL;
    PSMB2_HEADER             pHeader          = NULL; // Do not free
    ULONG                    ulHeaderSize     = 0L;
    PBYTE                    pBuffer          = NULL;
    ULONG                    ulBytesAvailable = 0;
    ULONG                    ulOffset         = 0;
    ULONG                    ulTotalBytesUsed = 0;
    SMB2_NOTIFY_CHANGE_HEADER notifyRequestHeader = {0};

    ntStatus = SMBPacketAllocate(
                    pNotifyState->pConnection->hPacketAllocator,
                    &pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pNotifyState->pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pSmbRequest->pRawBuffer,
                    &pSmbRequest->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2InitPacket(pSmbRequest, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsBuildExecContext(
                    pNotifyState->pConnection,
                    pSmbRequest,
                    TRUE,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          = pSmbRequest->pRawBuffer;
    ulBytesAvailable = pSmbRequest->bufferLen;

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
                COM2_NOTIFY,
                pNotifyState->usEpoch,
                0, /* credits */
                pNotifyState->ulPid,
                pNotifyState->ullCommandSequence,
                pNotifyState->ulTid,
                pNotifyState->ullSessionId,
                pNotifyState->ullAsyncId,
                pNotifyState->ioStatusBlock.Status,
                FALSE,  /* is response       */
                FALSE, /* related operation */
                &pHeader,
                &ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += ulHeaderSize;
    ulOffset         += ulHeaderSize;
    ulBytesAvailable -= ulHeaderSize;
    ulTotalBytesUsed += ulHeaderSize;

    notifyRequestHeader.usLength = sizeof(notifyRequestHeader);
    notifyRequestHeader.fid = pNotifyState->pFile->fid;

    if (ulBytesAvailable < sizeof(notifyRequestHeader))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pBuffer, (PBYTE)&notifyRequestHeader, sizeof(notifyRequestHeader));

    // pBuffer          += sizeof(notifyRequestHeader);
    // ulOffset         += sizeof(notifyRequestHeader);
    // ulBytesAvailable -= sizeof(notifyRequestHeader);
    ulTotalBytesUsed += sizeof(notifyRequestHeader);

    pSmbRequest->bufferUsed += ulTotalBytesUsed;

    ntStatus = SMB2MarshalFooter(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppExecContext = pExecContext;

cleanup:

    if (pSmbRequest)
    {
        SMBPacketRelease(
                pNotifyState->pConnection->hPacketAllocator,
                pSmbRequest);
    }

    return ntStatus;

error:

    *ppExecContext = NULL;

    if (pExecContext)
    {
        NfsReleaseExecContext(pExecContext);
    }

    goto cleanup;
}

VOID
NfsReleaseNotifyStateAsync_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    )
{
    if (pNotifyState->pAcb)
    {
        pNotifyState->acb.Callback        = NULL;

        if (pNotifyState->pAcb->CallbackContext)
        {
            InterlockedDecrement(&pNotifyState->refCount);

            pNotifyState->pAcb->CallbackContext = NULL;
        }

        if (pNotifyState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pNotifyState->pAcb->AsyncCancelContext);
        }

        pNotifyState->pAcb = NULL;
    }
}

PNFS_NOTIFY_STATE_SMB_V2
NfsNotifyStateAcquire_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    )
{
    InterlockedIncrement(&pNotifyState->refCount);

    return pNotifyState;
}

VOID
NfsNotifyStateReleaseHandle_SMB_V2(
    HANDLE hNotifyState
    )
{
    return NfsNotifyStateRelease_SMB_V2((PNFS_NOTIFY_STATE_SMB_V2)hNotifyState);
}

VOID
NfsNotifyStateRelease_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    )
{
    if (InterlockedDecrement(&pNotifyState->refCount) == 0)
    {
        NfsNotifyStateFree_SMB_V2(pNotifyState);
    }
}

static
VOID
NfsNotifyStateFree_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    )
{
    if (pNotifyState->pAcb && pNotifyState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pNotifyState->pAcb->AsyncCancelContext);
    }

    if (pNotifyState->pConnection)
    {
        NfsConnectionRelease(pNotifyState->pConnection);
    }

    if (pNotifyState->pFile)
    {
        NfsFile2Release(pNotifyState->pFile);
    }

    if (pNotifyState->pBuffer)
    {
        NfsFreeMemory(pNotifyState->pBuffer);
    }

    if (pNotifyState->pMutex)
    {
        pthread_mutex_destroy(&pNotifyState->mutex);
    }

    NfsFreeMemory(pNotifyState);
}

