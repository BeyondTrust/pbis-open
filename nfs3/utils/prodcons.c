/*
 * Copyright Likewise Software    2004-2010
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
 *        prodcons.c
 *
 * Abstract:
 *
 *        NFS3
 *
 *        Producer/consumer queue implementation
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 */

#include "includes.h"


struct __NFS3_PRODCONS_QUEUE
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    PNFS3_QUEUE      pQueue;

    ULONG            ulNumMaxItems;
    ULONG            ulNumItems;

    PFN_PRODCONS_QUEUE_FREE_ITEM pfnFreeItem;

    pthread_cond_t  event;
    pthread_cond_t* pEvent;
};


NTSTATUS
Nfs3ProdConsInit(
    ULONG                        ulNumMaxItems,
    PFN_PRODCONS_QUEUE_FREE_ITEM pfnFreeItem,
    PNFS3_PRODCONS_QUEUE*        ppQueue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS3_PRODCONS_QUEUE pQueue = NULL;

    if (!ulNumMaxItems)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = Nfs3AllocateMemory(
                    sizeof(*pQueue),
                    (PVOID*)&pQueue);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = Nfs3ProdConsInitContents(
                    pQueue,
                    ulNumMaxItems,
                    pfnFreeItem);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppQueue = pQueue;

cleanup:

    return ntStatus;

error:

    *ppQueue = NULL;

    if (pQueue)
    {
        Nfs3ProdConsFree(&pQueue);
    }

    goto cleanup;
}

NTSTATUS
Nfs3ProdConsInitContents(
    PNFS3_PRODCONS_QUEUE         pQueue,
    ULONG                        ulNumMaxItems,
    PFN_PRODCONS_QUEUE_FREE_ITEM pfnFreeItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    memset(pQueue, 0, sizeof(*pQueue));

    ntStatus = Nfs3QueueCreate(&pQueue->pQueue);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pQueue->mutex, NULL);
    pQueue->pMutex = &pQueue->mutex;

    pQueue->ulNumMaxItems = ulNumMaxItems;
    pQueue->pfnFreeItem = pfnFreeItem;

    pthread_cond_init(&pQueue->event, NULL);
    pQueue->pEvent = &pQueue->event;

cleanup:

    return ntStatus;

error:

    Nfs3FreeMemory((PVOID*)&pQueue->pQueue);

    goto cleanup;
}

NTSTATUS
Nfs3ProdConsEnqueue(
    PNFS3_PRODCONS_QUEUE pQueue,
    PVOID                pItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;

    NFS3_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    ntStatus = Nfs3QueueEnqueue(pQueue->pQueue, pItem);
    BAIL_ON_NT_STATUS(ntStatus);

    pQueue->ulNumItems++;

    pthread_cond_signal(&pQueue->event);

cleanup:

    NFS3_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
Nfs3ProdConsEnqueueFront(
    PNFS3_PRODCONS_QUEUE pQueue,
    PVOID                pItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;

    NFS3_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    ntStatus = Nfs3QueueEnqueueFront(pQueue->pQueue, pItem);
    BAIL_ON_NT_STATUS(ntStatus);

    pQueue->ulNumItems++;

    pthread_cond_signal(&pQueue->event);

cleanup:

    NFS3_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
Nfs3ProdConsDequeue(
    PNFS3_PRODCONS_QUEUE pQueue,
    PVOID*               ppItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;
    PVOID    pItem = NULL;

    NFS3_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (!pQueue->ulNumItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    pItem = Nfs3QueueDequeue(pQueue->pQueue);

    if (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        // Unblock any threads that may be waiting to queue.
        pthread_cond_broadcast(&pQueue->event);
    }

    pQueue->ulNumItems--;

    NFS3_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    *ppItem = pItem;

    return ntStatus;
}

NTSTATUS
Nfs3ProdConsTimedDequeue(
    PNFS3_PRODCONS_QUEUE pQueue,
    struct timespec*     pTimespec,
    PVOID*               ppItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;
    PVOID    pItem = NULL;

    NFS3_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (!pQueue->ulNumItems)
    {
        BOOLEAN bRetryWait = FALSE;

        do
        {
            bRetryWait = FALSE;

            int unixErrorCode = pthread_cond_timedwait(
                                    &pQueue->event,
                                    &pQueue->mutex,
                                    pTimespec);
            if (unixErrorCode == ETIMEDOUT)
            {
                if (time(NULL) < pTimespec->tv_sec)
                {
                    bRetryWait = TRUE;
                    continue;
                }
            }

            ntStatus = LwErrnoToNtStatus(unixErrorCode);

            /* Don't use BAIL_ON_XXX() here to reduce unnecessary noise */
            if (ntStatus != STATUS_SUCCESS)
            {
                goto error;
            }
        } while (bRetryWait);
    }

    pItem = Nfs3QueueDequeue(pQueue->pQueue);

    if (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        // Unblock any threads that may be waiting to queue.
        pthread_cond_broadcast(&pQueue->event);
    }

    pQueue->ulNumItems--;

    NFS3_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    *ppItem = pItem;

cleanup:

    NFS3_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return ntStatus;

error:

    *ppItem = NULL;

    goto cleanup;
}

VOID
Nfs3ProdConsFree(
    PNFS3_PRODCONS_QUEUE* ppQueue
    )
{
    if (*ppQueue)
    {
        Nfs3ProdConsFreeContents(*ppQueue);
        Nfs3FreeMemory((PVOID*)ppQueue);
    }
}

VOID
Nfs3ProdConsFreeContents(
    PNFS3_PRODCONS_QUEUE pQueue
    )
{
    if (pQueue->pMutex)
    {
        pthread_mutex_lock(pQueue->pMutex);
    }

    if (pQueue->pEvent)
    {
        pthread_cond_destroy(&pQueue->event);
        pQueue->pEvent = NULL;
    }

    if (pQueue->pfnFreeItem)
    {
        PVOID pItem = NULL;

        while ((pItem = Nfs3QueueDequeue(pQueue->pQueue)) != NULL)
        {
            pQueue->pfnFreeItem(pItem);
        }
    }

    if (pQueue->pQueue)
    {
        Nfs3QueueFree(pQueue->pQueue);
    }

    if (pQueue->pMutex)
    {
        pthread_mutex_unlock(&pQueue->mutex);
        pthread_mutex_destroy(pQueue->pMutex);
        pQueue->pMutex = NULL;
    }
}
