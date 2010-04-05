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
 *        queryinfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Query Information
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsBuildGetInfoState_SMB_V2(
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_NFS_FILE_2              pFile,
    PNFS_GET_INFO_STATE_SMB_V2*   ppGetInfoState
    );

static
NTSTATUS
NfsQueryInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildGetInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
VOID
NfsReleaseGetInfoStateHandle_SMB_V2(
    HANDLE hGetInfoState
    );

static
VOID
NfsExecuteGetInfoAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
NfsReleaseGetInfoState_SMB_V2(
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState
    );

static
VOID
NfsFreeGetInfoState_SMB_V2(
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState
    );

NTSTATUS
NfsProcessGetInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection   = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    PLWIO_NFS_SESSION_2        pSession      = NULL;
    PLWIO_NFS_TREE_2           pTree         = NULL;
    PLWIO_NFS_FILE_2           pFile         = NULL;
    BOOLEAN                    bInLock       = FALSE;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    if (pGetInfoState)
    {
        InterlockedIncrement(&pGetInfoState->refCount);
    }
    else
    {
        ULONG                      iMsg          = pCtxSmb2->iMsg;
        PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
        PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalGetInfoRequest(pSmbRequest, &pRequestHeader);
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

        ntStatus = NfsBuildGetInfoState_SMB_V2(
                            pRequestHeader,
                            pFile,
                            &pGetInfoState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pGetInfoState;
        InterlockedIncrement(&pGetInfoState->refCount);
        pCtxSmb2->pfnStateRelease = &NfsReleaseGetInfoStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pGetInfoState->mutex);

    switch (pGetInfoState->stage)
    {
        case NFS_GET_INFO_STAGE_SMB_V2_INITIAL:

            pGetInfoState->stage = NFS_GET_INFO_STAGE_SMB_V2_ATTEMPT_IO;

            // Intentional fall through

        case NFS_GET_INFO_STAGE_SMB_V2_ATTEMPT_IO:

            ntStatus = NfsQueryInfo_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pGetInfoState->stage = NFS_GET_INFO_STAGE_SMB_V2_BUILD_RESPONSE;

            // Intentional fall through

        case NFS_GET_INFO_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = NfsBuildGetInfoResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pGetInfoState->stage = NFS_GET_INFO_STAGE_SMB_V2_DONE;

            // Intentional fall through

        case NFS_GET_INFO_STAGE_SMB_V2_DONE:

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

    if (pGetInfoState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pGetInfoState->mutex);

        NfsReleaseGetInfoState_SMB_V2(pGetInfoState);
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

            if (pGetInfoState)
            {
                NfsReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildGetInfoState_SMB_V2(
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_NFS_FILE_2              pFile,
    PNFS_GET_INFO_STATE_SMB_V2*   ppGetInfoState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_GET_INFO_STATE_SMB_V2),
                    (PVOID*)&pGetInfoState);
    BAIL_ON_NT_STATUS(ntStatus);

    pGetInfoState->refCount = 1;

    pthread_mutex_init(&pGetInfoState->mutex, NULL);
    pGetInfoState->pMutex = &pGetInfoState->mutex;

    pGetInfoState->stage = NFS_GET_INFO_STAGE_SMB_V2_INITIAL;

    pGetInfoState->pRequestHeader = pRequestHeader;

    pGetInfoState->pFile = NfsFile2Acquire(pFile);

    *ppGetInfoState = pGetInfoState;

cleanup:

    return ntStatus;

error:

    *ppGetInfoState = NULL;

    if (pGetInfoState)
    {
        NfsFreeGetInfoState_SMB_V2(pGetInfoState);
    }

    goto cleanup;
}

static
NTSTATUS
NfsQueryInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = NfsGetFileInfo_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = NfsGetFileSystemInfo_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = NfsGetSecurityInfo_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
NfsBuildGetInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PNFS_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = NfsBuildFileInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = NfsBuildFileSystemInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = NfsBuildSecurityInfoResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

VOID
NfsPrepareGetInfoStateAsync_SMB_V2(
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState,
    PNFS_EXEC_CONTEXT          pExecContext
    )
{
    pGetInfoState->acb.Callback        = &NfsExecuteGetInfoAsyncCB_SMB_V2;

    pGetInfoState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pGetInfoState->acb.AsyncCancelContext = NULL;

    pGetInfoState->pAcb = &pGetInfoState->acb;
}

static
VOID
NfsExecuteGetInfoAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT          pExecContext     = (PNFS_EXEC_CONTEXT)pContext;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState    = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pGetInfoState =
        (PNFS_GET_INFO_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pGetInfoState->mutex);

    if (pGetInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pGetInfoState->pAcb->AsyncCancelContext);
    }

    pGetInfoState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pGetInfoState->mutex);

    ntStatus = NfsProdConsEnqueue(gProtocolGlobals_SMB_V2.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        NfsReleaseExecContext(pExecContext);
    }
}

VOID
NfsReleaseGetInfoStateAsync_SMB_V2(
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState
    )
{
    if (pGetInfoState->pAcb)
    {
        pGetInfoState->acb.Callback = NULL;

        if (pGetInfoState->pAcb->CallbackContext)
        {
            PNFS_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PNFS_EXEC_CONTEXT)pGetInfoState->pAcb->CallbackContext;

            NfsReleaseExecContext(pExecContext);

            pGetInfoState->pAcb->CallbackContext = NULL;
        }

        if (pGetInfoState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pGetInfoState->pAcb->AsyncCancelContext);
        }

        pGetInfoState->pAcb = NULL;
    }
}

static
VOID
NfsReleaseGetInfoStateHandle_SMB_V2(
    HANDLE hGetInfoState
    )
{
    return NfsReleaseGetInfoState_SMB_V2(
                    (PNFS_GET_INFO_STATE_SMB_V2)hGetInfoState);
}

static
VOID
NfsReleaseGetInfoState_SMB_V2(
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState
    )
{
    if (InterlockedDecrement(&pGetInfoState->refCount) == 0)
    {
        NfsFreeGetInfoState_SMB_V2(pGetInfoState);
    }
}

static
VOID
NfsFreeGetInfoState_SMB_V2(
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState
    )
{
    if (pGetInfoState->pAcb && pGetInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pGetInfoState->pAcb->AsyncCancelContext);
    }

    if (pGetInfoState->pData2)
    {
        NfsFreeMemory(pGetInfoState->pData2);
    }

    if (pGetInfoState->pFile)
    {
        NfsFile2Release(pGetInfoState->pFile);
    }

    if (pGetInfoState->pMutex)
    {
        pthread_mutex_destroy(&pGetInfoState->mutex);
    }

    NfsFreeMemory(pGetInfoState);
}

