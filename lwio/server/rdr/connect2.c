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
 * Module Name:
 *
 *        connect2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 connection setup
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
VOID
RdrNegotiateGssContextWorkItem2(
    PVOID pParam
    );

static
BOOLEAN
RdrProcessSessionSetupResponse2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrSessionSetupComplete2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrTransceiveSessionSetup2(
    PRDR_OP_CONTEXT pContext,
    PRDR_SESSION2 pSession,
    PBYTE pBlob,
    DWORD dwBlobLength
    );

static
NTSTATUS
RdrTransceiveTreeConnect2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE2 pTree,
    PCWSTR pwszPath
    );

static
BOOLEAN
RdrFinishTreeConnect2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrTreeConnect2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

BOOLEAN
RdrProcessNegotiateResponse2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_SOCKET pSocket = pContext->State.TreeConnect.pSocket;
    PSMB_PACKET pPacket = pParam;
    BOOLEAN bFreeContext = FALSE;
    BOOLEAN bSocketLocked = FALSE;
    PRDR_SMB2_NEGOTIATE_RESPONSE_HEADER pHeader = NULL;
    PBYTE pNegHint = NULL;
    ULONG ulNegHintLength = 0;

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMB2Header->error;
    BAIL_ON_NT_STATUS(status);

    LWIO_LOCK_MUTEX(bSocketLocked, &pSocket->mutex);

    status = RdrSmb2DecodeNegotiateResponse(
        pPacket,
        &pHeader,
        &pNegHint,
        &ulNegHintLength);
    BAIL_ON_NT_STATUS(status);

    pSocket->ulMaxTransactSize = pHeader->ulMaxTransactionSize;
    pSocket->ulMaxReadSize = pHeader->ulMaxReadSize;
    pSocket->ulMaxWriteSize = pHeader->ulMaxWriteSize;
    pSocket->capabilities = pHeader->ulCapabilities;
    pSocket->ucSecurityMode = pHeader->ucSecurityMode;

    pSocket->securityBlobLen = ulNegHintLength;

    status = LwIoAllocateMemory(
                    pSocket->securityBlobLen,
                    (PVOID *) &pSocket->pSecurityBlob);
    BAIL_ON_NT_STATUS(status);

    memcpy(pSocket->pSecurityBlob,
           pNegHint,
           pSocket->securityBlobLen);

    status = RdrSocketSetProtocol(pSocket, SMB_PROTOCOL_VERSION_2);
    BAIL_ON_NT_STATUS(status);

    pSocket->state = RDR_SOCKET_STATE_READY;

    RdrNotifyContextList(
        &pSocket->StateWaiters,
        bSocketLocked,
        &pSocket->mutex,
        STATUS_SUCCESS,
        pSocket);

    LWIO_UNLOCK_MUTEX(bSocketLocked, &pSocket->mutex);

    RdrNegotiateComplete2(pContext, STATUS_SUCCESS, pSocket);
    status = STATUS_PENDING;
    BAIL_ON_NT_STATUS(status);

 cleanup:

    LWIO_UNLOCK_MUTEX(bSocketLocked, &pSocket->mutex);

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext->State.TreeConnect.pContinue, status, NULL);
        bFreeContext = TRUE;
    }

    if (bFreeContext)
    {
        RdrFreeTreeConnectContext(pContext);
    }

    RdrFreePacket(pPacket);

    return FALSE;

error:

    if (status != STATUS_PENDING && pSocket)
    {
        LWIO_UNLOCK_MUTEX(bSocketLocked, &pSocket->mutex);
        RdrSocketInvalidate(pSocket, status);
        RdrSocketRelease(pSocket);
    }

    goto cleanup;
}

BOOLEAN
RdrNegotiateComplete2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_SOCKET pSocket = pParam;
    PRDR_SESSION2 pSession = NULL;
    BOOLEAN bSessionLocked = FALSE;
    BOOLEAN bFreeContext = FALSE;
    PIO_CREDS pCreds = pContext->State.TreeConnect.pCreds;

    BAIL_ON_NT_STATUS(status);

    if (pContext->State.TreeConnect.bStopOnDfs &&
        pSocket->capabilities & RDR_SMB2_CAP_DFS)
    {
        status = STATUS_DFS_EXIT_PATH_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrSession2FindOrCreate(
        &pSocket,
        pContext->State.TreeConnect.pCreds,
        pContext->State.TreeConnect.Uid,
        &pSession);
    BAIL_ON_NT_STATUS(status);

    pContext->State.TreeConnect.pSession2 = pSession;

    LWIO_LOCK_MUTEX(bSessionLocked, &pSession->mutex);

    switch (pSession->state)
    {
    case RDR_SESSION_STATE_NOT_READY:
        pSession->state = RDR_SESSION_STATE_INITIALIZING;

        switch (pCreds->type)
        {
        case IO_CREDS_TYPE_KRB5_TGT:
            status = SMBCredTokenToKrb5CredCache(
                pCreds,
                &pContext->State.TreeConnect.pszCachePath);
            BAIL_ON_NT_STATUS(status);
            break;
        case IO_CREDS_TYPE_PLAIN:
            break;
        default:
            status = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(status);
        }

        LWIO_UNLOCK_MUTEX(bSessionLocked, &pSession->mutex);
        RdrProcessSessionSetupResponse2(pContext, STATUS_SUCCESS, NULL);
        status = STATUS_PENDING;
        BAIL_ON_NT_STATUS(status);
        break;
    case RDR_SESSION_STATE_INITIALIZING:
        pContext->Continue = RdrSessionSetupComplete2;
        LwListInsertTail(&pSession->StateWaiters, &pContext->Link);
        status = STATUS_PENDING;
        BAIL_ON_NT_STATUS(status);
        break;
    case RDR_SESSION_STATE_READY:
        LWIO_UNLOCK_MUTEX(bSessionLocked, &pSession->mutex);
        RdrSessionSetupComplete2(pContext, status, pSession);
        status = STATUS_PENDING;
        BAIL_ON_NT_STATUS(status);
        break;
    case RDR_SESSION_STATE_ERROR:
        status = pSession->error;
        BAIL_ON_NT_STATUS(status);
        break;
    }

 cleanup:

     LWIO_UNLOCK_MUTEX(bSessionLocked, &pSession->mutex);

     if (status != STATUS_PENDING)
     {
         RdrContinueContext(pContext->State.TreeConnect.pContinue, status, NULL);
         bFreeContext = TRUE;
     }

     if (bFreeContext)
     {
         RdrFreeTreeConnectContext(pContext);
     }

     return FALSE;

 error:

     if (status != STATUS_PENDING && pSession)
     {
         LWIO_UNLOCK_MUTEX(bSessionLocked, &pSession->mutex);
         if (status != STATUS_DFS_EXIT_PATH_FOUND)
         {
             RdrSession2Invalidate(pSession, status);
         }
         RdrSession2Release(pSession);
     }

     if (status != STATUS_PENDING && pSocket)
     {
         if (status != STATUS_DFS_EXIT_PATH_FOUND)
         {
             RdrSocketInvalidate(pSocket, status);
         }
         RdrSocketRelease(pSocket);
     }

     goto cleanup;
}

static
BOOLEAN
RdrProcessSessionSetupResponse2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_SESSION2 pSession = pContext->State.TreeConnect.pSession2;
    PSMB_PACKET pPacket = pParam;
    BOOLEAN bSessionLocked = FALSE;
    BOOLEAN bFreeContext = FALSE;

    BAIL_ON_NT_STATUS(status);

    LWIO_LOCK_MUTEX(bSessionLocked, &pSession->mutex);

    if (pPacket)
    {
        status = pPacket->pSMB2Header->error;
        if (status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            status = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(status);

        pSession->ullSessionId = pPacket->pSMB2Header->ullSessionId;
    }

    /* Save the packet on the context for RdrNegotiateGssContextWorkItem2 */
    pContext->State.TreeConnect.pPacket = pPacket;
    pPacket = NULL;

    /* Dispatch a work item to negotiate the GSS context in a separate thread.
       Because GSS-API could potentially block in a network call (KRB5)
       or an IPC call (NTLM), calling into it directly from the socket task
       could cause a deadlock. */
    status = LwRtlQueueWorkItem(
        gRdrRuntime.pThreadPool,
        RdrNegotiateGssContextWorkItem2,
        pContext,
        0);
    BAIL_ON_NT_STATUS(status);

    status = STATUS_PENDING;
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWIO_UNLOCK_MUTEX(bSessionLocked, &pSession->mutex);

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext->State.TreeConnect.pContinue, status, NULL);
        bFreeContext = TRUE;
    }

    if (bFreeContext)
    {
        RdrFreeTreeConnectContext(pContext);
    }

    RdrFreePacket(pPacket);

    return FALSE;

error:

    if (status != STATUS_PENDING && pSession)
    {
        LWIO_UNLOCK_MUTEX(bSessionLocked, &pSession->mutex);
        RdrSession2Invalidate(pSession, status);
        RdrSession2Release(pSession);
    }

    goto cleanup;
}

static
VOID
RdrNegotiateGssContextWorkItem2(
    PVOID pParam
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = pParam;
    PRDR_SESSION2 pSession = pContext->State.TreeConnect.pSession2;
    PRDR_SOCKET pSocket = pSession->pSocket;
    PSMB_PACKET pPacket = pContext->State.TreeConnect.pPacket;
    PBYTE pInBlob = NULL;
    DWORD dwInBlobLength = 0;
    PBYTE pOutBlob = NULL;
    DWORD dwOutBlobLength = 0;
    BOOLEAN bSessionLocked = FALSE;

    if (pPacket)
    {
        /* Capture session id */

        if (pSession->ullSessionId == 0)
        {
            pSession->ullSessionId = pPacket->pSMB2Header->ullSessionId;
        }
        else if (pSession->ullSessionId != pPacket->pSMB2Header->ullSessionId)
        {
            status = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(status);
        }

        status = RdrSmb2DecodeSessionSetupResponse(
            pPacket,
            NULL,
            &pInBlob,
            &dwInBlobLength);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        pInBlob = pSession->pSocket->pSecurityBlob;
        dwInBlobLength = pSession->pSocket->securityBlobLen;
    }

    if (pContext->State.TreeConnect.pszCachePath)
    {
        status = SMBKrb5SetDefaultCachePath(
            pContext->State.TreeConnect.pszCachePath,
            NULL);
        BAIL_ON_NT_STATUS(status);
    }

    if (!pContext->State.TreeConnect.hGssContext)
    {
        status = SMBGSSContextBuild(
            pSocket->pwszCanonicalName,
            pContext->State.TreeConnect.pCreds,
            &pContext->State.TreeConnect.hGssContext);
        BAIL_ON_NT_STATUS(status);
    }

    status = SMBGSSContextNegotiate(
        pContext->State.TreeConnect.hGssContext,
        pInBlob,
        dwInBlobLength,
        &pOutBlob,
        &dwOutBlobLength);
    BAIL_ON_NT_STATUS(status);

    if (!SMBGSSContextNegotiateComplete(pContext->State.TreeConnect.hGssContext))
    {
        pContext->Continue = RdrProcessSessionSetupResponse2;

        status = RdrTransceiveSessionSetup2(
            pContext,
            pSession,
            pOutBlob,
            dwOutBlobLength);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        LWIO_LOCK_MUTEX(bSessionLocked, &pSession->mutex);

        status = SMBGSSContextGetSessionKey(
            pContext->State.TreeConnect.hGssContext,
            &pSession->pSessionKey,
            &pSession->dwSessionKeyLength);
        BAIL_ON_NT_STATUS(status);

        if (!pSocket->pSessionKey && pSession->pSessionKey)
        {
            status = LwIoAllocateMemory(
                pSession->dwSessionKeyLength,
                OUT_PPVOID(&pSocket->pSessionKey));
            BAIL_ON_NT_STATUS(status);

            memcpy(pSocket->pSessionKey, pSession->pSessionKey, pSession->dwSessionKeyLength);
            pSocket->dwSessionKeyLength = pSession->dwSessionKeyLength;
        }

        status = RdrSocketAddSession2ById(pSocket, pSession);
        BAIL_ON_NT_STATUS(status);

        pSession->state = RDR_SESSION_STATE_READY;

        RdrNotifyContextList(
            &pSession->StateWaiters,
            bSessionLocked,
            &pSession->mutex,
            status,
            pSession);

        LWIO_UNLOCK_MUTEX(bSessionLocked, &pSession->mutex);

        RdrSessionSetupComplete2(pContext, status, pSession);

        status = STATUS_PENDING;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bSessionLocked, &pSession->mutex);

    RTL_FREE(&pOutBlob);

    if (status != STATUS_PENDING)
    {
        if (pContext->State.TreeConnect.hGssContext)
        {
            SMBGSSContextFree(pContext->State.TreeConnect.hGssContext);
        }

        if (pSession)
        {
            RdrSession2Invalidate(pSession, status);
            RdrSession2Release(pSession);
        }

        RdrSessionSetupComplete2(pContext, status, NULL);
    }

    return;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveSessionSetup2(
    PRDR_OP_CONTEXT pContext,
    PRDR_SESSION2 pSession,
    PBYTE pBlob,
    DWORD dwBlobLength
    )
{
    NTSTATUS status = 0;
    PRDR_SOCKET pSocket = pSession->pSocket;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_SESSION_SETUP_SIZE(dwBlobLength));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_SESSION_SETUP,
        0, /* flags */
        gRdrRuntime.SysPid,
        0, /* tid */
        pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeSessionSetupRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        gRdrRuntime.config.bSigningEnabled,
        gRdrRuntime.config.bSigningRequired,
        TRUE,
        pBlob,
        dwBlobLength);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2FinishCommand(&pContext->Packet, &pCursor, &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTree2FindOrCreate(
    IN OUT PRDR_SESSION2* ppSession,
    IN PCWSTR pwszPath,
    OUT PRDR_TREE2* ppTree
    )
{
    NTSTATUS  ntStatus = 0;
    PRDR_TREE2 pTree = NULL;
    BOOLEAN   bInLock = FALSE;
    PRDR_SESSION2 pSession = *ppSession;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    ntStatus = SMBHashGetValue(
                pSession->pTreeHashByPath,
                pwszPath,
                (PVOID *) &pTree);

    if (!ntStatus)
    {
        pTree->refCount++;
        RdrTree2Revive(pTree);
        RdrSession2Release(pSession);
        *ppSession = NULL;
    }
    else
    {
        ntStatus = RdrTree2Create(&pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->pSession = pSession;

        ntStatus = RtlWC16StringDuplicate(
            &pTree->pwszPath,
            pwszPath);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBHashSetValue(
            pSession->pTreeHashByPath,
            pTree->pwszPath,
            pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->bParentLink = TRUE;

        *ppSession = NULL;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    *ppTree = NULL;

    if (pTree)
    {
        RdrTree2Release(pTree);
    }

    goto cleanup;
}


static
BOOLEAN
RdrSessionSetupComplete2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_SESSION2 pSession = pParam;
    PRDR_TREE2 pTree = NULL;
    BOOLEAN bTreeLocked = FALSE;
    BOOLEAN bFreeContext = FALSE;

    BAIL_ON_NT_STATUS(status);

    status = RdrTree2FindOrCreate(
        &pSession,
        pContext->State.TreeConnect.pwszSharename,
        &pTree);
    BAIL_ON_NT_STATUS(status);

    pContext->State.TreeConnect.pTree2 = pTree;

    LWIO_LOCK_MUTEX(bTreeLocked, &pTree->mutex);

    switch (pTree->state)
    {
    case RDR_TREE_STATE_NOT_READY:
        pTree->state = RDR_TREE_STATE_INITIALIZING;

        pContext->Continue = RdrFinishTreeConnect2;

        status = RdrTransceiveTreeConnect2(pContext, pTree, pTree->pwszPath);
        BAIL_ON_NT_STATUS(status);
        break;
    case RDR_TREE_STATE_INITIALIZING:
        pContext->Continue = RdrTreeConnect2Complete;
        LwListInsertTail(&pTree->StateWaiters, &pContext->Link);
        bFreeContext = TRUE;
        status = STATUS_PENDING;
        break;
    case RDR_TREE_STATE_READY:
        RdrTreeConnect2Complete(pContext, status, pTree);
        status = STATUS_PENDING;
        BAIL_ON_NT_STATUS(status);
        break;
    case RDR_TREE_STATE_ERROR:
        status = pTree->error;
        BAIL_ON_NT_STATUS(status);
        break;
    }

 cleanup:

    LWIO_UNLOCK_MUTEX(bTreeLocked, &pTree->mutex);

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext->State.TreeConnect.pContinue, status, NULL);
        bFreeContext = TRUE;
    }

    if (bFreeContext)
    {
        RdrFreeTreeConnectContext(pContext);
    }

    return FALSE;

 error:

    if (status != STATUS_PENDING && pTree)
    {
        LWIO_UNLOCK_MUTEX(bTreeLocked, &pTree->mutex);
        RdrTree2Invalidate(pTree, status);
        RdrTree2Release(pTree);
    }

    if (status != STATUS_PENDING && pSession)
    {
        RdrSession2Release(pSession);
    }

    goto cleanup;
}

static
BOOLEAN
RdrTreeConnect2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_TREE2 pTree = pParam;

    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext->State.TreeConnect.pContinue, status, pTree);
        RdrFreeTreeConnectContext(pContext);
    }

    return FALSE;

error:

    if (status != STATUS_PENDING && pTree)
    {
        RdrTree2Release(pTree);
        pTree = NULL;
    }

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveTreeConnect2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE2 pTree,
    PCWSTR pwszPath
    )
{
    NTSTATUS status = 0;
    PRDR_SOCKET pSocket = pTree->pSession->pSocket;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;

    status = RdrAllocateContextPacket(
        pContext,
        RDR_SMB2_TREE_CONNECT_SIZE(RDR_SMB2_MAX_SHARE_PATH_LENGTH));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_TREE_CONNECT,
        0, /* flags */
        gRdrRuntime.SysPid,
        0, /* tid */
        pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeTreeConnectRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        pwszPath);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2FinishCommand(&pContext->Packet, &pCursor, &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrFinishTreeConnect2(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_TREE2 pTree = pContext->State.TreeConnect.pTree2;
    PSMB_PACKET pResponsePacket = pParam;
    BOOLEAN bTreeLocked = FALSE;
    PRDR_SMB2_TREE_CONNECT_RESPONSE_HEADER pHeader = NULL;

    LWIO_LOCK_MUTEX(bTreeLocked, &pTree->mutex);

    BAIL_ON_NT_STATUS(status);

    status = pResponsePacket->pSMB2Header->error;
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2DecodeTreeConnectResponse(pResponsePacket, &pHeader);
    BAIL_ON_NT_STATUS(status);

    pTree->ulTid = pResponsePacket->pSMB2Header->ulTid;
    pTree->ulCapabilities = pHeader->ulShareCapabilities;
    pTree->state = RDR_TREE_STATE_READY;

cleanup:

    RdrFreePacket(pResponsePacket);

    RdrNotifyContextList(&pTree->StateWaiters, bTreeLocked, &pTree->mutex, status, pTree);

    LWIO_UNLOCK_MUTEX(bTreeLocked, &pTree->mutex);

    return RdrTreeConnect2Complete(pContext, status, pTree);

error:

    LWIO_UNLOCK_MUTEX(bTreeLocked, &pTree->mutex);
    RdrTree2Invalidate(pTree, status);

    goto cleanup;
}
