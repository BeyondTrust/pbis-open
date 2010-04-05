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
 *        oplock.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Opportunistic locks
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
NfsBuildOplockBreakNotification_SMB_V2(
    PNFS_EXEC_CONTEXT        pExecContext,
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState,
    UCHAR                    ucOplockLevel
    );

static
NTSTATUS
NfsBuildOplockBreakResponse_SMB_V2(
    PNFS_EXEC_CONTEXT        pExecContext,
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState,
    UCHAR                    ucOplockLevel
    );

static
VOID
NfsOplockExpiredCB_SMB_V2(
    PNFS_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    );

static
NTSTATUS
NfsEnqueueOplockAckTask_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    );

static
VOID
NfsFreeOplockState_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    );

static
VOID
NfsOplockAsyncCB_SMB_V2(
    PVOID pContext
    );

static
NTSTATUS
NfsBuildOplockExecContext_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState,
    USHORT                   usOplockAction,
    PNFS_EXEC_CONTEXT*       ppExecContext
    );

NTSTATUS
NfsBuildOplockState_SMB_V2(
    PLWIO_NFS_CONNECTION      pConnection,
    PLWIO_NFS_SESSION_2       pSession,
    PLWIO_NFS_TREE_2          pTree,
    PLWIO_NFS_FILE_2          pFile,
    PNFS_OPLOCK_STATE_SMB_V2* ppOplockState
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_OPLOCK_STATE_SMB_V2),
                    (PVOID*)&pOplockState);
    BAIL_ON_NT_STATUS(ntStatus);

    pOplockState->refCount = 1;

    pthread_mutex_init(&pOplockState->mutex, NULL);
    pOplockState->pMutex = &pOplockState->mutex;

    pOplockState->pConnection = NfsConnectionAcquire(pConnection);

    pOplockState->ullUid = pSession->ullUid;

    pOplockState->ulTid = pTree->ulTid;

    pOplockState->fid = pFile->fid;

    *ppOplockState = pOplockState;

cleanup:

    return ntStatus;

error:

    *ppOplockState = NULL;

    if (pOplockState)
    {
        NfsFreeOplockState_SMB_V2(pOplockState);
    }

    goto cleanup;
}

NTSTATUS
NfsProcessOplock_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_NFS_CONNECTION       pConnection  = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PSMB2_OPLOCK_BREAK_HEADER  pRequestHeader = NULL; // Do not free
    PLWIO_NFS_SESSION_2        pSession      = NULL;
    PLWIO_NFS_TREE_2           pTree         = NULL;
    PLWIO_NFS_FILE_2           pFile         = NULL;
    PNFS_OPLOCK_STATE_SMB_V2   pOplockState  = NULL;
    UCHAR                      ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;

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

    ntStatus = SMB2UnmarshalOplockBreakRequest(
                    pSmbRequest,
                    &pRequestHeader);
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

    switch (pRequestHeader->ulReserved)
    {
        case LW_SMB2_OPLOCK_ACTION_SEND_BREAK:

            pOplockState =
                    (PNFS_OPLOCK_STATE_SMB_V2)pFile->hOplockState;

            if (!pOplockState)
            {
                ntStatus = STATUS_INTERNAL_ERROR;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            switch (pOplockState->oplockBuffer_out.OplockBreakResult)
            {
                case IO_OPLOCK_BROKEN_TO_NONE:

                    ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;

                    break;

                case IO_OPLOCK_BROKEN_TO_LEVEL_2:

                    ucOplockLevel = SMB_OPLOCK_LEVEL_II;

                    break;

                default:

                    ntStatus = STATUS_INTERNAL_ERROR;
                    BAIL_ON_NT_STATUS(ntStatus);

                    break;
            }
            
            ntStatus = NfsBuildOplockBreakNotification_SMB_V2(
                            pExecContext,
                            pOplockState,
                            ucOplockLevel);
            BAIL_ON_NT_STATUS(ntStatus);

            pOplockState->bBreakRequestSent = TRUE;

            switch (NfsFile2GetOplockLevel(pFile)) // current op-lock level
            {
                case SMB_OPLOCK_LEVEL_I:
                case SMB_OPLOCK_LEVEL_BATCH:
                    {
                        LONG64 llExpiry = 0LL;

                        ntStatus = WireGetCurrentNTTime(&llExpiry);
                        BAIL_ON_NT_STATUS(ntStatus);

                        /* configured timeout will be in milliseconds */
                        llExpiry +=
                            (NfsConfigGetOplockTimeout_SMB_V2() *
                                WIRE_FACTOR_MILLISECS_TO_HUNDREDS_OF_NANOSECS);

                        ntStatus = NfsTimerPostRequest(
                                        llExpiry,
                                        pOplockState,
                                        &NfsOplockExpiredCB_SMB_V2,
                                        &pOplockState->pTimerRequest);
                        BAIL_ON_NT_STATUS(ntStatus);

                        InterlockedIncrement(&pOplockState->refCount);
                    }

                    break;

                case SMB_OPLOCK_LEVEL_II:

                    /* Level2 can only break to none. No Ack needed.
                       Remove any remaining oplock state */

                    pOplockState = (PNFS_OPLOCK_STATE_SMB_V2)NfsFile2RemoveOplockState(pFile);
                    if (pOplockState)
                    {
                        NfsReleaseOplockState_SMB_V2(pOplockState);
                        pOplockState = NULL;
                    }

                    ntStatus = STATUS_SUCCESS;

                    break;

                case SMB_OPLOCK_LEVEL_NONE:
                default:

                    ntStatus = STATUS_INTERNAL_ERROR;
                    BAIL_ON_NT_STATUS(ntStatus);

                    break;
            }

            break;

        case LW_SMB2_OPLOCK_ACTION_PROCESS_ACK:

            pOplockState =
                    (PNFS_OPLOCK_STATE_SMB_V2)pFile->hOplockState;

            if (pOplockState)
            {
                ntStatus = NfsAcknowledgeOplockBreak_SMB_V2(pOplockState,
                                                            NULL,
                                                            FALSE);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
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

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsProcessOplockBreak_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_NFS_CONNECTION       pConnection  = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PSMB2_OPLOCK_BREAK_HEADER  pRequestHeader = NULL; // Do not free
    PLWIO_NFS_SESSION_2        pSession      = NULL;
    PLWIO_NFS_TREE_2           pTree         = NULL;
    PLWIO_NFS_FILE_2           pFile         = NULL;
    PNFS_OPLOCK_STATE_SMB_V2   pOplockState  = NULL;

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

    ntStatus = SMB2UnmarshalOplockBreakRequest(
                    pSmbRequest,
                    &pRequestHeader);
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

    pOplockState = (PNFS_OPLOCK_STATE_SMB_V2)pFile->hOplockState;

    if (pOplockState)
    {
        UCHAR ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;

        // Release the timer on the break ack now

        if (pOplockState->pTimerRequest)
        {
            PNFS_OPLOCK_STATE_SMB_V2 pOplockState2 = NULL;

            NfsTimerCancelRequest(
                pOplockState->pTimerRequest,
                (PVOID*)&pOplockState2);

            if (pOplockState2)
            {
                NfsReleaseOplockState_SMB_V2(pOplockState2);
            }

            NfsTimerRelease(pOplockState->pTimerRequest);
            pOplockState->pTimerRequest = NULL;
        }

        ntStatus = NfsAcknowledgeOplockBreak_SMB_V2(pOplockState,
                    &pRequestHeader->ucOplockLevel,
                    FALSE);
        BAIL_ON_NT_STATUS(ntStatus);

        switch (pRequestHeader->ucOplockLevel)
        {
            case SMB2_OPLOCK_LEVEL_BATCH:

                    ucOplockLevel = SMB_OPLOCK_LEVEL_BATCH;

                    break;

            case SMB2_OPLOCK_LEVEL_I:

                ucOplockLevel = SMB_OPLOCK_LEVEL_I;

                break;

            case SMB2_OPLOCK_LEVEL_II:

                ucOplockLevel = SMB_OPLOCK_LEVEL_II;

                break;

            default:

                ucOplockLevel = SMB2_OPLOCK_LEVEL_NONE;

                break;
        }

        ntStatus = NfsBuildOplockBreakResponse_SMB_V2(
                        pExecContext,
                        pOplockState,
                        ucOplockLevel);
        BAIL_ON_NT_STATUS(ntStatus);
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

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsAcknowledgeOplockBreak_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState,
    PUCHAR                   pucNewOplockLevel,
    BOOLEAN                  bFileIsClosed
    )
{
    NTSTATUS            ntStatus      = STATUS_SUCCESS;
    UCHAR               ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;
    PLWIO_NFS_SESSION_2 pSession      = NULL;
    PLWIO_NFS_TREE_2    pTree         = NULL;
    PLWIO_NFS_FILE_2    pFile         = NULL;

    ntStatus = NfsConnection2FindSession(
                    pOplockState->pConnection,
                    pOplockState->ullUid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsSession2FindTree(pSession, pOplockState->ulTid, &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsTree2FindFile(pTree, &pOplockState->fid, &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Clear flag indicating we are waiting an ACK from the client */

    pOplockState->bBreakRequestSent = FALSE;

    switch (pOplockState->oplockBuffer_out.OplockBreakResult)
    {
        case IO_OPLOCK_BROKEN_TO_NONE:

            // ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;

            break;

        case IO_OPLOCK_BROKEN_TO_LEVEL_2:

            ucOplockLevel = SMB_OPLOCK_LEVEL_II;

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (bFileIsClosed)
    {
        pOplockState->oplockBuffer_ack.Response = IO_OPLOCK_BREAK_CLOSE_PENDING;
    } 
    else 
    {
        if (pucNewOplockLevel &&
            (*pucNewOplockLevel == SMB_OPLOCK_LEVEL_NONE) &&
            (ucOplockLevel == SMB_OPLOCK_LEVEL_II))
        {
            pOplockState->oplockBuffer_ack.Response =
                                                IO_OPLOCK_BREAK_ACK_NO_LEVEL_2;
        }
        else
        {
            pOplockState->oplockBuffer_ack.Response =
                                                IO_OPLOCK_BREAK_ACKNOWLEDGE;
        }
    }
    
    NfsPrepareOplockStateAsync_SMB_V2(pOplockState);

    ntStatus = IoFsControlFile(
                    pFile->hFile,
                    pOplockState->pAcb,
                    &pOplockState->ioStatusBlock,
                    IO_FSCTL_OPLOCK_BREAK_ACK,
                    &pOplockState->oplockBuffer_ack,
                    sizeof(pOplockState->oplockBuffer_ack),
                    &pOplockState->oplockBuffer_out,
                    sizeof(pOplockState->oplockBuffer_out));
    switch (ntStatus)
    {
        case STATUS_PENDING:

            ntStatus = NfsFile2SetOplockState(
                           pFile,
                           pOplockState,
                           &NfsReleaseOplockStateHandle_SMB_V2);
            BAIL_ON_NT_STATUS(ntStatus);

            InterlockedIncrement(&pOplockState->refCount);

            NfsFile2SetOplockLevel(pFile, ucOplockLevel);

            ntStatus = STATUS_SUCCESS;

            break;

        default:

            NfsReleaseOplockStateAsync_SMB_V2(pOplockState); // completed sync

            BAIL_ON_NT_STATUS(ntStatus);

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

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
NfsBuildOplockBreakNotification_SMB_V2(
    PNFS_EXEC_CONTEXT        pExecContext,
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState,
    UCHAR                    ucOplockLevel
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PSMB2_OPLOCK_BREAK_HEADER  pOplockBreakHeader = NULL; // Do not free
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SMB2MarshalHeader(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM2_BREAK,
                        pSmbRequest->pHeader->usEpoch,
                        0,  /* Credits     */
                        0L, /* Process Id  */
                        pSmbRequest->pHeader->ullCommandSequence,
                        0L,  /* Tree Id    */
                        0LL, /* Session Id */
                        0LL, /* Async Id   */
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

    if (ulBytesAvailable < sizeof(SMB2_OPLOCK_BREAK_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pOplockBreakHeader = (PSMB2_OPLOCK_BREAK_HEADER)pOutBuffer;

    pOplockBreakHeader->usLength = sizeof(SMB2_OPLOCK_BREAK_HEADER);
    pOplockBreakHeader->fid      = pOplockState->fid;

    switch (ucOplockLevel)
    {
        case SMB_OPLOCK_LEVEL_BATCH:

            pOplockBreakHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_BATCH;

            break;

        case SMB_OPLOCK_LEVEL_I:

            pOplockBreakHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_I;

            break;

        case SMB_OPLOCK_LEVEL_II:

            pOplockBreakHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_II;

            break;

        default:

            pOplockBreakHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_NONE;

            break;
    }

    // pOutBuffer       += sizeof(SMB2_OPLOCK_BREAK_HEADER);
    // ulBytesUsed       = sizeof(SMB2_OPLOCK_BREAK_HEADER);
    // ulOffset         += sizeof(SMB2_OPLOCK_BREAK_HEADER);
    ulBytesAvailable -= sizeof(SMB2_OPLOCK_BREAK_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_OPLOCK_BREAK_HEADER);

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader      = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsBuildOplockBreakResponse_SMB_V2(
    PNFS_EXEC_CONTEXT        pExecContext,
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState,
    UCHAR                    ucOplockLevel
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PSMB2_OPLOCK_BREAK_HEADER  pOplockBreakHeader = NULL; // Do not free
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SMB2MarshalHeader(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM2_BREAK,
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

    if (ulBytesAvailable < sizeof(SMB2_OPLOCK_BREAK_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pOplockBreakHeader = (PSMB2_OPLOCK_BREAK_HEADER)pOutBuffer;

    pOplockBreakHeader->usLength = sizeof(SMB2_OPLOCK_BREAK_HEADER);
    pOplockBreakHeader->fid      = pOplockState->fid;

    switch (ucOplockLevel)
    {
        case SMB_OPLOCK_LEVEL_BATCH:

            pOplockBreakHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_BATCH;

            break;

        case SMB_OPLOCK_LEVEL_I:

            pOplockBreakHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_I;

            break;

        case SMB_OPLOCK_LEVEL_II:

            pOplockBreakHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_II;

            break;

        default:

            pOplockBreakHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_NONE;

            break;
    }

    // pOutBuffer       += sizeof(SMB2_OPLOCK_BREAK_HEADER);
    // ulBytesUsed       = sizeof(SMB2_OPLOCK_BREAK_HEADER);
    // ulOffset         += sizeof(SMB2_OPLOCK_BREAK_HEADER);
    // ulBytesAvailable -= sizeof(SMB2_OPLOCK_BREAK_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_OPLOCK_BREAK_HEADER);

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader      = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
VOID
NfsOplockExpiredCB_SMB_V2(
    PNFS_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState = (PNFS_OPLOCK_STATE_SMB_V2)pUserData;

    ntStatus = NfsEnqueueOplockAckTask_SMB_V2(pOplockState);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pOplockState)
    {
        NfsReleaseOplockState_SMB_V2(pOplockState);
    }

    return;

error:

    goto cleanup;
}

static
NTSTATUS
NfsEnqueueOplockAckTask_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    )
{
    NTSTATUS          ntStatus     = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT pExecContext = NULL;

    ntStatus = NfsBuildOplockExecContext_SMB_V2(
                    pOplockState,
                    LW_SMB2_OPLOCK_ACTION_PROCESS_ACK,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsProdConsEnqueue(
                    gProtocolGlobals_SMB_V2.pWorkQueue,
                    pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pExecContext = NULL;

cleanup:

    if (pExecContext)
    {
        NfsReleaseExecContext(pExecContext);
    }

    return ntStatus;

error:

    goto cleanup;
}

VOID
NfsReleaseOplockStateHandle_SMB_V2(
    HANDLE hOplockState
    )
{
    NfsReleaseOplockState_SMB_V2((PNFS_OPLOCK_STATE_SMB_V2)hOplockState);
}

VOID
NfsReleaseOplockState_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    )
{
    if (InterlockedDecrement(&pOplockState->refCount) == 0)
    {
        NfsFreeOplockState_SMB_V2(pOplockState);
    }
}

static
VOID
NfsFreeOplockState_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    )
{
    if (pOplockState->pAcb && pOplockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pOplockState->pAcb->AsyncCancelContext);
    }

    if (pOplockState->pConnection)
    {
        NfsConnectionRelease(pOplockState->pConnection);
    }

    if (pOplockState->pTimerRequest)
    {
        NfsTimerRelease(pOplockState->pTimerRequest);
    }

    if (pOplockState->pMutex)
    {
        pthread_mutex_destroy(&pOplockState->mutex);
    }

    NfsFreeMemory(pOplockState);
}

VOID
NfsPrepareOplockStateAsync_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    )
{
    pOplockState->acb.Callback        = &NfsOplockAsyncCB_SMB_V2;

    pOplockState->acb.CallbackContext = pOplockState;
    InterlockedIncrement(&pOplockState->refCount);

    pOplockState->acb.AsyncCancelContext = NULL;

    pOplockState->pAcb = &pOplockState->acb;
}

static
VOID
NfsOplockAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState = (PNFS_OPLOCK_STATE_SMB_V2)pContext;
    PNFS_EXEC_CONTEXT        pExecContext = NULL;
    BOOLEAN                  bInLock      = FALSE;
    PLWIO_NFS_SESSION_2      pSession     = NULL;
    PLWIO_NFS_TREE_2         pTree        = NULL;
    PLWIO_NFS_FILE_2         pFile        = NULL;

    /* Nothing to do if this was cancelled */

    if (pOplockState->ioStatusBlock.Status == STATUS_CANCELLED)
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    LWIO_LOCK_MUTEX(bInLock, &pOplockState->mutex);

    if (pOplockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pOplockState->pAcb->AsyncCancelContext);
    }

    pOplockState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pOplockState->mutex);

    ntStatus = pOplockState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsConnection2FindSession(
                    pOplockState->pConnection,
                    pOplockState->ullUid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsSession2FindTree(
                    pSession,
                    pOplockState->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsTree2FindFile(
                    pTree,
                    &pOplockState->fid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsBuildOplockExecContext_SMB_V2(
                    pOplockState,
                    LW_SMB2_OPLOCK_ACTION_SEND_BREAK,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsProdConsEnqueue(
                    gProtocolGlobals_SMB_V2.pWorkQueue,
                    pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pExecContext = NULL;

cleanup:

    if (pOplockState)
    {
        NfsReleaseOplockState_SMB_V2(pOplockState);
    }

    if (pExecContext)
    {
        NfsReleaseExecContext(pExecContext);
    }

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

    return;

error:

    LWIO_LOG_ERROR("Error: Failed processing oplock break [status:0x%x]",
                   ntStatus);

    // TODO: indicate error on file handle somehow

    goto cleanup;
}

static
NTSTATUS
NfsBuildOplockExecContext_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState,
    USHORT                   usOplockAction,
    PNFS_EXEC_CONTEXT*       ppExecContext
    )
{
    NTSTATUS                  ntStatus           = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT         pExecContext       = NULL;
    PSMB_PACKET               pSmbRequest        = NULL;
    PBYTE                     pBuffer            = NULL;
    ULONG                     ulBytesAvailable   = 0;
    ULONG                     ulOffset           = 0;
    ULONG                     ulBytesUsed        = 0;
    ULONG                     ulTotalBytesUsed   = 0;
    PSMB2_HEADER              pHeader            = NULL; // Do not free
    PSMB2_OPLOCK_BREAK_HEADER pOplockBreakHeader = NULL; // Do not free

    ntStatus = SMBPacketAllocate(
                    pOplockState->pConnection->hPacketAllocator,
                    &pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pOplockState->pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pSmbRequest->pRawBuffer,
                    &pSmbRequest->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2InitPacket(pSmbRequest, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsBuildExecContext(
                    pOplockState->pConnection,
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
                    COM2_BREAK,
                    0,                  /* epoch       */
                    1,                  /* credits     */
                    0,                  /* pid         */
                    0xFFFFFFFFFFFFFFFFLL, /* mid = -1    */
                    pOplockState->ulTid,
                    pOplockState->ullUid,
                    0LL, /* Async Id */
                    STATUS_SUCCESS,
                    FALSE,              /* is response */
                    FALSE,              /* chained message */
                    &pHeader,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    if (ulBytesAvailable < sizeof(SMB2_OPLOCK_BREAK_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pOplockBreakHeader = (PSMB2_OPLOCK_BREAK_HEADER)pBuffer;

    pOplockBreakHeader->fid         = pOplockState->fid;
    pOplockBreakHeader->usLength = sizeof(SMB2_OPLOCK_BREAK_HEADER);

    pOplockBreakHeader->ulReserved = usOplockAction;

    // pBuffer          += sizeof(SMB2_OPLOCK_BREAK_HEADER);
    // ulBytesAvailable -= sizeof(SMB2_OPLOCK_BREAK_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_OPLOCK_BREAK_HEADER);

    pSmbRequest->bufferUsed += ulTotalBytesUsed;

    ntStatus = SMB2MarshalFooter(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppExecContext = pExecContext;

cleanup:

    if (pSmbRequest)
    {
        SMBPacketRelease(
                pOplockState->pConnection->hPacketAllocator,
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
NfsReleaseOplockStateAsync_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    )
{
    if (pOplockState->pAcb)
    {
        pOplockState->acb.Callback        = NULL;

        if (pOplockState->pAcb->CallbackContext)
        {
            InterlockedDecrement(&pOplockState->refCount);

            pOplockState->pAcb->CallbackContext = NULL;
        }

        if (pOplockState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pOplockState->pAcb->AsyncCancelContext);
        }

        pOplockState->pAcb = NULL;
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
