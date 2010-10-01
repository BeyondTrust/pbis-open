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
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

static
int
SMBTreeHashResponseCompare(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
SMBTreeHashResponse(
    PCVOID vp
    );

static
NTSTATUS
SMBTreeDestroyContents(
    PSMB_TREE pTree
    );

NTSTATUS
SMBTreeCreate(
    PSMB_TREE* ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_TREE pTree = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_t* pMutexAttr = NULL;

    ntStatus = LwIoAllocateMemory(
                sizeof(SMB_TREE),
                (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = pthread_mutexattr_init(&mutexAttr);
    BAIL_ON_NT_STATUS(ntStatus);

    pMutexAttr = &mutexAttr;

    ntStatus = pthread_mutexattr_settype(pMutexAttr, PTHREAD_MUTEX_RECURSIVE);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pTree->mutex, pMutexAttr);
    bDestroyMutex = TRUE;

    ntStatus = pthread_cond_init(&pTree->event, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroyCondition = TRUE;

    pTree->refCount = 1;

    /* @todo: find a portable time call which is immune to host date and time
       changes, such as made by ntpd */
    if (time((time_t*) &pTree->lastActiveTime) == (time_t)-1)
    {
        ntStatus = ErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pTree->pSession = NULL;
    pTree->tid = 0;
    pTree->pszPath = NULL;

    ntStatus = SMBHashCreate(
                19,
                &SMBTreeHashResponseCompare,
                &SMBTreeHashResponse,
                NULL,
                &pTree->pResponseHash);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppTree = pTree;

cleanup:

    if (pMutexAttr)
    {
        pthread_mutexattr_destroy(pMutexAttr);
    }

    return ntStatus;

error:

    if (bDestroyCondition)
    {
        pthread_cond_destroy(&pTree->event);
    }

    if (bDestroyMutex)
    {
        pthread_mutex_destroy(&pTree->mutex);
    }

    if (pTree)
    {
        SMBTreeDestroyContents(pTree);
    }
    LWIO_SAFE_FREE_MEMORY(pTree);

    *ppTree = NULL;

    goto cleanup;
}

static
int
SMBTreeHashResponseCompare(
    PCVOID vp1,
    PCVOID vp2)
{
    uint16_t mid1 = *((uint16_t *) vp1);
    uint16_t mid2 = *((uint16_t *) vp2);

    if (mid1 == mid2)
    {
        return 0;
    }
    else if (mid1 > mid2)
    {
        return 1;
    }

    return -1;
}

static
size_t
SMBTreeHashResponse(
    PCVOID vp)
{
    return *((uint16_t *) vp);
}

/* This must be called with the parent hash lock held to avoid a race with the
   reaper. */
VOID
SMBTreeAddReference(
    PSMB_TREE pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->pSession->mutex);

    pTree->refCount++;

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
}

NTSTATUS
SMBTreeAcquireMid(
    PSMB_TREE pTree,
    uint16_t* pwMid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    WORD wMid = 0;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);

    wMid = pTree->mid++;

    LWIO_LOG_DEBUG("Acquired mid [%d] from Tree [0x%x]", wMid, pTree);

    *pwMid = wMid;

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

NTSTATUS
SMBTreeSetState(
    PSMB_TREE pTree,
    SMB_RESOURCE_STATE state
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);

    pTree->state = state;

    pthread_cond_broadcast(&pTree->event);

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

NTSTATUS
SMBTreeInvalidate(
    PSMB_TREE pTree,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bInSessionLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);
    
    pTree->state = RDR_TREE_STATE_ERROR;
    pTree->error = ntStatus;
    
    LWIO_LOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);
    if (pTree->bParentLink)
    {
        SMBHashRemoveKey(
            pTree->pSession->pTreeHashByPath,
            pTree->pszPath);
        SMBHashRemoveKey(
            pTree->pSession->pTreeHashByTID,
            &pTree->tid);
        pTree->bParentLink = FALSE;
    }
    LWIO_UNLOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);

    pthread_cond_broadcast(&pTree->event);
    
    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

VOID
SMBTreeRelease(
    SMB_TREE *pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->pSession->mutex);

    assert(pTree->refCount > 0);

    if (--pTree->refCount == 0)
    {
        if (pTree->state != RDR_TREE_STATE_READY)
        {
            if (pTree->bParentLink)
            {
                SMBHashRemoveKey(
                    pTree->pSession->pTreeHashByPath,
                    pTree->pszPath);
                SMBHashRemoveKey(
                    pTree->pSession->pTreeHashByTID,
                    &pTree->tid);
                pTree->bParentLink = FALSE;
            }
            LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
            SMBTreeFree(pTree);
        }
        else
        {
            LWIO_LOG_VERBOSE("Tree %p is eligible for reaping", pTree);
            LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
            RdrReaperPoke(&gRdrRuntime, pTree->lastActiveTime);
        }            
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
    }
}

VOID
SMBTreeFree(
    PSMB_TREE pTree
    )
{
    assert(!pTree->refCount);

    pthread_cond_destroy(&pTree->event);
    pthread_mutex_destroy(&pTree->mutex);

    SMBTreeDestroyContents(pTree);

    if (pTree->pSession)
    {
        SMBSessionRelease(pTree->pSession);
    }

    LwIoFreeMemory(pTree);
}

static
NTSTATUS
SMBTreeDestroyContents(
    PSMB_TREE pTree
    )
{
    LWIO_SAFE_FREE_MEMORY(pTree->pszPath);

    /* @todo: assert that the session hash is empty */
    SMBHashSafeFree(&pTree->pResponseHash);

    return 0;
}

NTSTATUS
SMBTreeReceiveResponse(
    IN PSMB_TREE pTree,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    IN PSMB_RESPONSE pResponse,
    OUT PSMB_PACKET* ppResponsePacket
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bResponseInLock = FALSE;
    BOOLEAN bTreeInLock = FALSE;
    struct timespec ts = {0, 0};
    int err = 0;

    // TODO-This function should really just get use pSocket instead ofpTree and
    // use MID allocation from the socket...  so it should become
    // SMBSocketReceiveResponse.

    LWIO_LOCK_MUTEX(bResponseInLock, &pResponse->mutex);

    while (!(pResponse->state == SMB_RESOURCE_STATE_VALID))
    {
        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

        LWIO_LOG_DEBUG("Waiting for response for [mid: %d] and Tree [0x%x]", pResponse->mid, pTree);

retry_wait:

        /* @todo: always verify non-error state after acquiring mutex */
        err = pthread_cond_timedwait(
            &pResponse->event,
            &pResponse->mutex,
            &ts);
        if (err == ETIMEDOUT)
        {
            if (time(NULL) < ts.tv_sec)
            {
                err = 0;
                goto retry_wait;
            }

            /* As long as the socket is active, continue to wait.
             * otherwise, mark the socket as bad and return
             */
            if (SMBSocketTimedOut(pTree->pSession->pSocket))
            {
                ntStatus = STATUS_IO_TIMEOUT;
                SMBSocketInvalidate(
                            pTree->pSession->pSocket,
                            ntStatus);

                SMBResponseInvalidate_InLock(pResponse, ntStatus);
            }
            else
            {
                // continue waiting
                ntStatus = STATUS_SUCCESS;
            }
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_UNLOCK_MUTEX(bResponseInLock, &pResponse->mutex);

    LWIO_LOCK_MUTEX(bTreeInLock, &pTree->mutex);

    LWIO_LOG_DEBUG("Removing response [mid: %d] from Tree [0x%x]", pResponse->mid, pTree);

    ntStatus = SMBHashRemoveKey(
                    pTree->pResponseHash,
                    &pResponse->mid);
    BAIL_ON_NT_STATUS(ntStatus);

    /* @todo: this need be set only when the hash is empty */
    pTree->lastActiveTime = time(NULL);

    ntStatus = SMBPacketDecodeHeader(
                    pResponse->pPacket,
                    bVerifySignature && !RdrSocketGetIgnoreServerSignatures(pTree->pSession->pSocket),
                    dwExpectedSequence,
                    pTree->pSession->pSocket->pSessionKey,
                    pTree->pSession->pSocket->dwSessionKeyLength);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Could be NULL on error */
    *ppResponsePacket = pResponse->pPacket;
    pResponse->pPacket = NULL;

cleanup:

    LWIO_UNLOCK_MUTEX(bTreeInLock, &pTree->mutex);

    LWIO_UNLOCK_MUTEX(bResponseInLock, &pResponse->mutex);

    return ntStatus;

error:

    *ppResponsePacket = NULL;

    goto cleanup;
}

NTSTATUS
SMBTreeFindLockedResponseByMID(
    PSMB_TREE      pTree,
    uint16_t       wMid,
    PSMB_RESPONSE* ppResponse
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bResponseInLock = FALSE;
    PSMB_RESPONSE pResponse = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);

    LWIO_LOG_DEBUG("Trying to find response [mid: %d] in Tree [0x%x]", wMid, pTree);

    ntStatus = SMBHashGetValue(
                    pTree->pResponseHash,
                    &wMid,
                    (PVOID *) &pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    LWIO_LOG_DEBUG("Locking response [mid: %d] in Tree [0x%x]", wMid, pTree);

    LWIO_LOCK_MUTEX(bResponseInLock, &pResponse->mutex);

    LWIO_LOG_DEBUG("Locked response [mid: %d] in Tree [0x%x]", wMid, pTree);

    *ppResponse = pResponse;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    *ppResponse = NULL;

    goto cleanup;
}

NTSTATUS
SMBTreeWaitReady(
    PSMB_TREE pTree
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    while (pTree->state < RDR_TREE_STATE_READY)
    {
        if (pTree->state == RDR_TREE_STATE_ERROR)
        {
            ntStatus = pTree->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        
        pthread_cond_wait(&pTree->event, &pTree->mutex);
    }

error:

    return ntStatus;
}
