/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        tree.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Common Tree Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 */

#include "rdr.h"

static
VOID
RdrTreeFree(
    PRDR_TREE pTree
    );

static
NTSTATUS
RdrTreeDestroyContents(
    PRDR_TREE pTree
    );

static
NTSTATUS
RdrTransceiveTreeDisconnect(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree
    );

NTSTATUS
RdrTreeCreate(
    PRDR_TREE* ppTree
    )
{
    NTSTATUS status = 0;
    PRDR_TREE pTree = NULL;
    BOOLEAN bDestroyMutex = FALSE;
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_t* pMutexAttr = NULL;

    status = LwIoAllocateMemory(
                sizeof(RDR_TREE),
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

    status = RdrAllocateContextPacket(pTree->pDisconnectContext, 64 * 1024);
    BAIL_ON_NT_STATUS(status);

    pTree->refCount = 1;
    pTree->pSession = NULL;
    pTree->tid = 0;
    pTree->pwszPath = NULL;
    pTree->version = SMB_PROTOCOL_VERSION_1;

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
        RdrTreeDestroyContents(pTree);
    }
    LWIO_SAFE_FREE_MEMORY(pTree);

    *ppTree = NULL;

    goto cleanup;
}

VOID
RdrTreeRevive(
    PRDR_TREE pTree
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
RdrTreeUnlink(
    PRDR_TREE pTree
    )
{
    if (pTree->bParentLink)
    {
        SMBHashRemoveKey(
            pTree->pSession->pTreeHashByPath,
            pTree->pwszPath);
        SMBHashRemoveKey(
            pTree->pSession->pTreeHashByTID,
            &pTree->tid);
        pTree->bParentLink = FALSE;
    }
}

NTSTATUS
RdrTreeInvalidate(
    PRDR_TREE pTree,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bInSessionLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);
    
    pTree->state = RDR_TREE_STATE_ERROR;
    pTree->error = ntStatus;
    
    LWIO_LOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);
    RdrTreeUnlink(pTree);
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
RdrTreeDisconnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PRDR_TREE pTree = pContext->State.TreeConnect.pTree;

    RdrFreePacket(pPacket);
    RdrTreeFree(pTree);

    /* We don't explicitly free pContext because RdrTreeFree() does it */
    return FALSE;
}

static
VOID
RdrTreeTimeout(
    PLW_TASK pTask,
    LW_PVOID _pTree,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_TREE pTree = _pTree;
    BOOLEAN bLocked = FALSE;
    PRDR_OP_CONTEXT pContext = NULL;

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        *pWaitMask = LW_TASK_EVENT_TIME;
        *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
    }

    if ((WakeMask & LW_TASK_EVENT_TIME) ||
        ((WakeMask & LW_TASK_EVENT_CANCEL) && RdrIsShutdownSet()))
    {
        LWIO_LOCK_MUTEX(bLocked, &pTree->pSession->mutex);

        if (pTree->refCount == 0)
        {
            RdrTreeUnlink(pTree);
        
            pContext = pTree->pDisconnectContext;
            pContext->Continue = RdrTreeDisconnectComplete;
            pContext->State.TreeConnect.pTree = pTree;

            LWIO_UNLOCK_MUTEX(bLocked, &pTree->pSession->mutex);

            status = RdrTransceiveTreeDisconnect(pContext, pTree);
            if (status != STATUS_PENDING)
            {
                /* Give up and free the tree now */
                RdrTreeFree(pTree);
            }

            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
        }
    }
    else if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }

    LWIO_UNLOCK_MUTEX(bLocked, &pTree->pSession->mutex);

    return;
}

VOID
RdrTreeRelease(
    RDR_TREE *pTree
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
            RdrTreeUnlink(pTree);
            LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
            RdrTreeFree(pTree);
        }
        else
        {
            LWIO_LOG_VERBOSE("Tree %p is eligible for reaping", pTree);
            
            LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);

            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pTree->pTimeout,
                    gRdrRuntime.pTreeTimerGroup,
                    RdrTreeTimeout,
                    pTree) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pTree->pTimeout);
            }
            else
            {
                LWIO_LOG_ERROR("Could not create timer for tree %p; disconnecting immediately");
                RdrTreeTimeout(NULL, pTree, LW_TASK_EVENT_TIME, &dummy, &llDummy);
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
RdrTreeFree(
    PRDR_TREE pTree
    )
{
    assert(!pTree->refCount);

    pthread_mutex_destroy(&pTree->mutex);

    RdrTreeDestroyContents(pTree);

    if (pTree->pSession)
    {
        RdrSessionRelease(pTree->pSession);
    }

    LwIoFreeMemory(pTree);
}

static
NTSTATUS
RdrTreeDestroyContents(
    PRDR_TREE pTree
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
RdrTransceiveTreeDisconnect(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    /* Packet should have been pre-allocated */
    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_TREE_DISCONNECT,
        0,
        0,
        pTree->tid,
        gRdrRuntime.SysPid,
        pTree->pSession->uid,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Packet.pSMBHeader->wordCount = 0;

    pContext->Packet.pData = pContext->Packet.pParams;          /* ByteCount */
    pContext->Packet.bufferUsed += sizeof(uint16_t);
    memset(pContext->Packet.pData, 0, sizeof(uint16_t));

    // no byte order conversions necessary (due to zeros)

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}
