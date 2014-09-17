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
 *        tree2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 tree management
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
VOID
RdrTree2Free(
    PRDR_TREE2 pTree
    );

static
NTSTATUS
RdrTree2DestroyContents(
    PRDR_TREE2 pTree
    );

static
NTSTATUS
RdrTransceiveTree2Disconnect(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE2 pTree
    );

NTSTATUS
RdrTree2Create(
    PRDR_TREE2* ppTree
    )
{
    NTSTATUS status = 0;
    PRDR_TREE2 pTree = NULL;
    BOOLEAN bDestroyMutex = FALSE;
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_t* pMutexAttr = NULL;

    status = LwIoAllocateMemory(
                sizeof(RDR_TREE2),
                (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(status);

    LwListInit(&pTree->StateWaiters);

    status = pthread_mutexattr_init(&mutexAttr);
    BAIL_ON_NT_STATUS(status);

    pMutexAttr = &mutexAttr;

    status = pthread_mutexattr_settype(pMutexAttr, PTHREAD_MUTEX_RECURSIVE);
    BAIL_ON_NT_STATUS(status);

    pthread_mutex_init(&pTree->mutex, pMutexAttr);
    bDestroyMutex = TRUE;

    /* Pre-allocate resources to send a tree disconnect */
    status = RdrCreateContext(NULL, &pTree->pDisconnectContext);
    BAIL_ON_NT_STATUS(status);

    status = RdrAllocateContextPacket(pTree->pDisconnectContext, RDR_SMB2_STUB_SIZE);
    BAIL_ON_NT_STATUS(status);

    pTree->refCount = 1;
    pTree->pSession = NULL;
    pTree->ulTid = 0;
    pTree->pwszPath = NULL;
    pTree->version = SMB_PROTOCOL_VERSION_2;

    *ppTree = pTree;

cleanup:

    if (pMutexAttr)
    {
        pthread_mutexattr_destroy(pMutexAttr);
    }

    return status;

error:

    if (bDestroyMutex)
    {
        pthread_mutex_destroy(&pTree->mutex);
    }

    if (pTree)
    {
        RdrTree2DestroyContents(pTree);
    }
    LWIO_SAFE_FREE_MEMORY(pTree);

    *ppTree = NULL;

    goto cleanup;
}

VOID
RdrTree2Revive(
    PRDR_TREE2 pTree
    )
{
    if (pTree->pTimeout)
    {
        LwRtlCancelTask(pTree->pTimeout);
        LwRtlReleaseTask(&pTree->pTimeout);
    }
}

static
VOID
RdrTree2Unlink(
    PRDR_TREE2 pTree
    )
{
    if (pTree->bParentLink)
    {
        SMBHashRemoveKey(
            pTree->pSession->pTreeHashByPath,
            pTree->pwszPath);
        SMBHashRemoveKey(
            pTree->pSession->pTreeHashById,
            &pTree->ulTid);
        pTree->bParentLink = FALSE;
    }
}

NTSTATUS
RdrTree2Invalidate(
    PRDR_TREE2 pTree,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bInSessionLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);

    pTree->state = RDR_TREE_STATE_ERROR;
    pTree->error = ntStatus;

    LWIO_LOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);
    RdrTree2Unlink(pTree);
    LWIO_UNLOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);

    RdrNotifyContextList(
        &pTree->StateWaiters,
        bInLock,
        &pTree->mutex,
        ntStatus,
        NULL);

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

static
BOOLEAN
RdrTree2DisconnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PRDR_TREE2 pTree = pContext->State.TreeConnect.pTree2;

    RdrFreePacket(pPacket);
    RdrTree2Free(pTree);

    /* We don't explicitly free pContext because RdrTreeFree() does it */
    return FALSE;
}

static
VOID
RdrTree2Timeout(
    PLW_TASK pTask,
    LW_PVOID _pTree,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_TREE2 pTree = _pTree;
    BOOLEAN bLocked = FALSE;
    PRDR_OP_CONTEXT pContext = NULL;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }
    else if (WakeMask & LW_TASK_EVENT_INIT)
    {
        *pWaitMask = LW_TASK_EVENT_TIME;
        *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
    }

    if ((WakeMask & LW_TASK_EVENT_TIME) ||
        ((WakeMask & LW_TASK_EVENT_EXPLICIT) && RdrIsShutdownSet()))
    {
        LWIO_LOCK_MUTEX(bLocked, &pTree->pSession->mutex);

        if (pTree->refCount == 0)
        {
            RdrTree2Unlink(pTree);

            pContext = pTree->pDisconnectContext;
            pContext->Continue = RdrTree2DisconnectComplete;
            pContext->State.TreeConnect.pTree2 = pTree;

            LWIO_UNLOCK_MUTEX(bLocked, &pTree->pSession->mutex);

            status = RdrTransceiveTree2Disconnect(pContext, pTree);
            if (status != STATUS_PENDING)
            {
                /* Give up and free the tree now */
                RdrTree2Free(pTree);
            }

            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
        }
    }

    LWIO_UNLOCK_MUTEX(bLocked, &pTree->pSession->mutex);

    return;
}

VOID
RdrTree2Release(
    RDR_TREE2 *pTree
    )
{
    BOOLEAN bInLock = FALSE;
    LW_TASK_EVENT_MASK dummy = 0;
    LONG64 llDummy = 0;

    LWIO_LOCK_MUTEX(bInLock, &pTree->pSession->mutex);

    assert(pTree->refCount > 0);

    if (--pTree->refCount == 0)
    {
        if (pTree->state != RDR_TREE_STATE_READY || !RdrSocketIsValid(pTree->pSession->pSocket))
        {
            RdrTree2Unlink(pTree);
            LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
            RdrTree2Free(pTree);
        }
        else
        {
            LWIO_LOG_VERBOSE("Tree %p is eligible for reaping", pTree);

            LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);

            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pTree->pTimeout,
                    gRdrRuntime.pTreeTimerGroup,
                    RdrTree2Timeout,
                    pTree) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pTree->pTimeout);
            }
            else
            {
                LWIO_LOG_ERROR("Could not create timer for tree %p; disconnecting immediately");
                RdrTree2Timeout(NULL, pTree, LW_TASK_EVENT_TIME, &dummy, &llDummy);
            }
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
    }
}

static
VOID
RdrTree2Free(
    PRDR_TREE2 pTree
    )
{
    assert(!pTree->refCount);

    pthread_mutex_destroy(&pTree->mutex);

    RdrTree2DestroyContents(pTree);

    if (pTree->pSession)
    {
        RdrSession2Release(pTree->pSession);
    }

    LwIoFreeMemory(pTree);
}

static
NTSTATUS
RdrTree2DestroyContents(
    PRDR_TREE2 pTree
    )
{
    LWIO_SAFE_FREE_MEMORY(pTree->pwszPath);

    if (pTree->pTimeout)
    {
        LwRtlCancelTask(pTree->pTimeout);
        LwRtlReleaseTask(&pTree->pTimeout);
    }

    if (pTree->pDisconnectContext)
    {
        RdrFreeContext(pTree->pDisconnectContext);
    }

    return 0;
}

static
NTSTATUS
RdrTransceiveTree2Disconnect(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE2 pTree
    )
{
    NTSTATUS status = 0;
    PRDR_SOCKET pSocket = pTree->pSession->pSocket;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_TREE_DISCONNECT,
        0, /* flags */
        gRdrRuntime.SysPid,
        pTree->ulTid, /* tid */
        pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeStubRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining);
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
