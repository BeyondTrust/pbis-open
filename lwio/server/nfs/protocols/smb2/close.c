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
 *        close.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Close
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsBuildCloseState_SMB_V2(
    PSMB2_CLOSE_REQUEST_HEADER pRequestHeader,
    PLWIO_NFS_TREE_2           pTree,
    PLWIO_NFS_FILE_2           pFile,
    PNFS_CLOSE_STATE_SMB_V2*   ppCloseState
    );

static
NTSTATUS
NfsGetFileInformation_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildCloseResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
VOID
NfsPrepareCloseStateAsync_SMB_V2(
    PNFS_CLOSE_STATE_SMB_V2 pCloseState,
    PNFS_EXEC_CONTEXT       pExecContext
    );

static
VOID
NfsExecuteCloseAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
NfsReleaseCloseStateAsync_SMB_V2(
    PNFS_CLOSE_STATE_SMB_V2 pCloseState
    );

static
VOID
NfsReleaseCloseStateHandle_SMB_V2(
    HANDLE hCloseState
    );

static
VOID
NfsReleaseCloseState_SMB_V2(
    PNFS_CLOSE_STATE_SMB_V2 pCloseState
    );

static
VOID
NfsFreeCloseState_SMB_V2(
    PNFS_CLOSE_STATE_SMB_V2 pCloseState
    );

NTSTATUS
NfsProcessClose_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection      = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2         = pProtocolContext->pSmb2Context;
    ULONG                      iMsg             = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest      = &pCtxSmb2->pRequests[iMsg];
    BOOLEAN                    bRelated         = FALSE;
    PNFS_CLOSE_STATE_SMB_V2    pCloseState      = NULL;
    PLWIO_NFS_SESSION_2        pSession         = NULL;
    PLWIO_NFS_TREE_2           pTree            = NULL;
    PLWIO_NFS_FILE_2           pFile            = NULL;
    BOOLEAN                    bInLock          = FALSE;

    bRelated = LwIsSetFlag(
                    pSmbRequest->pHeader->ulFlags,
                    SMB2_FLAGS_RELATED_OPERATION);

    pCloseState = (PNFS_CLOSE_STATE_SMB_V2)pCtxSmb2->hState;
    if (pCloseState)
    {
        InterlockedIncrement(&pCloseState->refCount);
    }
    else
    {
        PSMB2_CLOSE_REQUEST_HEADER pRequestHeader = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalCloseRequest(pSmbRequest, &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        if (bRelated && !pCtxSmb2->llNumSuccessfulCreates)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = NfsTree2FindFile_SMB_V2(
                        pCtxSmb2,
                        pTree,
                        &pRequestHeader->fid,
                        bRelated,
                        &pFile);
        if ((ntStatus == STATUS_FILE_CLOSED) && bRelated)
        {
            switch (pCtxSmb2->lastCloseStatus)
            {
                case STATUS_SUCCESS:

                    // File was closed successfully
                    // Another Close request was chained
                    pCtxSmb2->lastCloseStatus = STATUS_FILE_CLOSED;

                    break;

                case STATUS_FILE_CLOSED:

                    // This is the second chained close request after
                    // a successful close followed by a chained close request.
                    pCtxSmb2->lastCloseStatus = STATUS_INVALID_PARAMETER;

                    ntStatus = STATUS_INVALID_PARAMETER;

                    break;

                default:

                    ntStatus = pCtxSmb2->lastCloseStatus;

                    break;
            }
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsBuildCloseState_SMB_V2(
                        pRequestHeader,
                        pTree,
                        pFile,
                        &pCloseState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pCloseState;
        InterlockedIncrement(&pCloseState->refCount);
        pCtxSmb2->pfnStateRelease = &NfsReleaseCloseStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pCloseState->mutex);

    switch (pCloseState->stage)
    {
        case NFS_CLOSE_STAGE_SMB_V2_INITIAL:

            if (pCloseState->pRequestHeader->usFlags &
                    SMB2_CLOSE_FLAGS_GET_FILE_ATTRIBUTES)
            {
                ntStatus = NfsGetFileInformation_SMB_V2(pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pCloseState->stage = NFS_CLOSE_STAGE_SMB_V2_ATTEMPT_IO;

            // intentional fall through

        case NFS_CLOSE_STAGE_SMB_V2_ATTEMPT_IO:

            NfsFile2ResetOplockState(pCloseState->pFile);

            ntStatus = NfsTree2RemoveFile(
                            pCloseState->pTree,
                            &pCloseState->pFile->fid);
            BAIL_ON_NT_STATUS(ntStatus);

            pCloseState->stage = NFS_CLOSE_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case NFS_CLOSE_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = NfsBuildCloseResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCloseState->stage = NFS_CLOSE_STAGE_SMB_V2_DONE;

            // intentional fall through

        case NFS_CLOSE_STAGE_SMB_V2_DONE:

            if (pCloseState->pFile)
            {
                NfsFile2Rundown(pCloseState->pFile);
            }

            if (bRelated && pCtxSmb2->pFile)
            {
                NfsFile2Release(pCtxSmb2->pFile);
                pCtxSmb2->pFile = NULL;
            }

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

    if (pCloseState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCloseState->mutex);

        NfsReleaseCloseState_SMB_V2(pCloseState);
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

            if (pCloseState)
            {
                NfsReleaseCloseStateAsync_SMB_V2(pCloseState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildCloseState_SMB_V2(
    PSMB2_CLOSE_REQUEST_HEADER pRequestHeader,
    PLWIO_NFS_TREE_2           pTree,
    PLWIO_NFS_FILE_2           pFile,
    PNFS_CLOSE_STATE_SMB_V2*   ppCloseState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PNFS_CLOSE_STATE_SMB_V2 pCloseState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_CLOSE_STATE_SMB_V2),
                    (PVOID*)&pCloseState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCloseState->refCount = 1;

    pthread_mutex_init(&pCloseState->mutex, NULL);
    pCloseState->pMutex = &pCloseState->mutex;

    pCloseState->pRequestHeader = pRequestHeader;

    pCloseState->pTree = NfsTree2Acquire(pTree);

    pCloseState->pFile = NfsFile2Acquire(pFile);

    *ppCloseState = pCloseState;

cleanup:

    return ntStatus;

error:

    *ppCloseState = NULL;

    if (pCloseState)
    {
        NfsFreeCloseState_SMB_V2(pCloseState);
    }

    goto cleanup;
}

static
NTSTATUS
NfsGetFileInformation_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2         = pProtocolContext->pSmb2Context;
    PNFS_CLOSE_STATE_SMB_V2    pCloseState      = NULL;

    pCloseState = (PNFS_CLOSE_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pCloseState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pCloseState->pFileBasicInfo)
    {
        pCloseState->pFileBasicInfo = &pCloseState->fileBasicInfo;

        NfsPrepareCloseStateAsync_SMB_V2(pCloseState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCloseState->pFile->hFile,
                        pCloseState->pAcb,
                        &pCloseState->ioStatusBlock,
                        pCloseState->pFileBasicInfo,
                        sizeof(pCloseState->fileBasicInfo),
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseCloseStateAsync_SMB_V2(pCloseState);
    }

    if (!pCloseState->pFileStdInfo)
    {
        pCloseState->pFileStdInfo = &pCloseState->fileStdInfo;

        NfsPrepareCloseStateAsync_SMB_V2(pCloseState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCloseState->pFile->hFile,
                        pCloseState->pAcb,
                        &pCloseState->ioStatusBlock,
                        pCloseState->pFileStdInfo,
                        sizeof(pCloseState->fileStdInfo),
                        FileStandardInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseCloseStateAsync_SMB_V2(pCloseState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
NfsBuildCloseResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2         = pProtocolContext->pSmb2Context;
    PNFS_CLOSE_STATE_SMB_V2    pCloseState      = NULL;
    ULONG                      iMsg             = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest      = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse     = &pCtxSmb2->pResponses[iMsg];
    PSMB2_CLOSE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE  pOutBuffer       = pSmbResponse->pBuffer;
    ULONG  ulOffset         = 0;
    ULONG  ulTotalBytesUsed = 0;
    ULONG  ulBytesUsed      = 0;
    ULONG  ulBytesAvailable = pSmbResponse->ulBytesAvailable;

    pCloseState = (PNFS_CLOSE_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_CLOSE,
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

    if (ulBytesAvailable < sizeof(SMB2_CLOSE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_CLOSE_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulBytesUsed       = sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_CLOSE_RESPONSE_HEADER);

    if (pCloseState->pRequestHeader->usFlags &
            SMB2_CLOSE_FLAGS_GET_FILE_ATTRIBUTES)
    {
        pResponseHeader->ullCreationTime   =
                        pCloseState->fileBasicInfo.CreationTime;
        pResponseHeader->ullLastAccessTime =
                        pCloseState->fileBasicInfo.LastAccessTime;
        pResponseHeader->ullLastWriteTime  =
                        pCloseState->fileBasicInfo.LastWriteTime;
        pResponseHeader->ullLastChangeTime =
                        pCloseState->fileBasicInfo.ChangeTime;
        pResponseHeader->ulFileAttributes  =
                        pCloseState->fileBasicInfo.FileAttributes;
        pResponseHeader->ullAllocationSize =
                        pCloseState->fileStdInfo.AllocationSize;
        pResponseHeader->ullEndOfFile      =
                        pCloseState->fileStdInfo.EndOfFile;

        pResponseHeader->usFlags |= SMB2_CLOSE_FLAGS_GET_FILE_ATTRIBUTES;
    }

    pResponseHeader->usLength          = ulBytesUsed;

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
NfsPrepareCloseStateAsync_SMB_V2(
    PNFS_CLOSE_STATE_SMB_V2 pCloseState,
    PNFS_EXEC_CONTEXT       pExecContext
    )
{
    pCloseState->acb.Callback        = &NfsExecuteCloseAsyncCB_SMB_V2;

    pCloseState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pCloseState->acb.AsyncCancelContext = NULL;

    pCloseState->pAcb = &pCloseState->acb;
}

static
VOID
NfsExecuteCloseAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT          pExecContext     = (PNFS_EXEC_CONTEXT)pContext;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_CLOSE_STATE_SMB_V2    pCloseState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pCloseState = (PNFS_CLOSE_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pCloseState->mutex);

    if (pCloseState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pCloseState->pAcb->AsyncCancelContext);
    }

    pCloseState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pCloseState->mutex);

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
NfsReleaseCloseStateAsync_SMB_V2(
    PNFS_CLOSE_STATE_SMB_V2 pCloseState
    )
{
    if (pCloseState->pAcb)
    {
        pCloseState->acb.Callback        = NULL;

        if (pCloseState->pAcb->CallbackContext)
        {
            PNFS_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PNFS_EXEC_CONTEXT)pCloseState->pAcb->CallbackContext;

            NfsReleaseExecContext(pExecContext);

            pCloseState->pAcb->CallbackContext = NULL;
        }

        if (pCloseState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCloseState->pAcb->AsyncCancelContext);
        }

        pCloseState->pAcb = NULL;
    }
}

static
VOID
NfsReleaseCloseStateHandle_SMB_V2(
    HANDLE hCloseState
    )
{
    return NfsReleaseCloseState_SMB_V2((PNFS_CLOSE_STATE_SMB_V2)hCloseState);
}

static
VOID
NfsReleaseCloseState_SMB_V2(
    PNFS_CLOSE_STATE_SMB_V2 pCloseState
    )
{
    if (InterlockedDecrement(&pCloseState->refCount) == 0)
    {
        NfsFreeCloseState_SMB_V2(pCloseState);
    }
}

static
VOID
NfsFreeCloseState_SMB_V2(
    PNFS_CLOSE_STATE_SMB_V2 pCloseState
    )
{
    if (pCloseState->pAcb && pCloseState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pCloseState->pAcb->AsyncCancelContext);
    }

    if (pCloseState->pFile)
    {
        NfsFile2Release(pCloseState->pFile);
    }

    if (pCloseState->pTree)
    {
        NfsTree2Release(pCloseState->pTree);
    }

    if (pCloseState->pMutex)
    {
        pthread_mutex_destroy(&pCloseState->mutex);
    }

    NfsFreeMemory(pCloseState);
}

