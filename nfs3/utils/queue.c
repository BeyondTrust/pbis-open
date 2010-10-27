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
 *        queue.c
 *
 * Abstract:
 *
 *        NFS3
 *
 *        Queue implementation
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 */

#include "includes.h"


struct __NFS3_QUEUE_ITEM
{
    PVOID pItem;

    struct __NFS3_QUEUE_ITEM * pNext;
};

struct __NFS3_QUEUE
{
    PNFS3_QUEUE_ITEM pHead;
    PNFS3_QUEUE_ITEM pTail;
};


NTSTATUS
Nfs3QueueCreate(
    PNFS3_QUEUE* ppQueue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS3_QUEUE pQueue = NULL;

    ntStatus = Nfs3AllocateMemory(
                    sizeof(*pQueue),
                    (PVOID*)&pQueue);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppQueue = pQueue;

cleanup:

    return ntStatus;

error:

    *ppQueue = NULL;

    goto cleanup;
}

NTSTATUS
Nfs3QueueEnqueue(
    PNFS3_QUEUE pQueue,
    PVOID       pItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS3_QUEUE_ITEM pQueueItem = NULL;

    ntStatus = Nfs3AllocateMemory(
                    sizeof(*pQueueItem),
                    (PVOID*)&pQueueItem);
    BAIL_ON_NT_STATUS(ntStatus);

    pQueueItem->pItem = pItem;

    if (!pQueue->pHead)
    {
        pQueue->pHead = pQueue->pTail = pQueueItem;
    }
    else
    {
        pQueue->pTail->pNext = pQueueItem;
        pQueue->pTail = pQueueItem;
    }

cleanup:

    return ntStatus;

error:

    NFS3_SAFE_FREE_MEMORY(pQueueItem);

    goto cleanup;
}

NTSTATUS
Nfs3QueueEnqueueFront(
    PNFS3_QUEUE pQueue,
    PVOID       pItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS3_QUEUE_ITEM pQueueItem = NULL;

    ntStatus = Nfs3AllocateMemory(
                    sizeof(*pQueueItem),
                    (PVOID*)&pQueueItem);
    BAIL_ON_NT_STATUS(ntStatus);

    pQueueItem->pItem = pItem;

    if (!pQueue->pHead)
    {
        pQueue->pHead = pQueue->pTail = pQueueItem;
    }
    else
    {
        pQueueItem->pNext = pQueue->pHead;
        pQueue->pHead = pQueueItem;
    }

cleanup:

    return ntStatus;

error:

    NFS3_SAFE_FREE_MEMORY(pQueueItem);

    goto cleanup;
}

PVOID
Nfs3QueueDequeue(
    PNFS3_QUEUE pQueue
    )
{
    PVOID pItem = NULL;

    if (pQueue->pHead)
    {
        PNFS3_QUEUE_ITEM pQueueItem = pQueue->pHead;

        pQueue->pHead = pQueue->pHead->pNext;
        if (!pQueue->pHead)
        {
            pQueue->pTail = NULL;
        }

        pItem = pQueueItem->pItem;

        Nfs3FreeMemory(pQueueItem);
    }

    return pItem;
}

BOOLEAN
Nfs3QueueIsEmpty(
    PNFS3_QUEUE pQueue
    )
{
    return (pQueue->pHead == NULL);
}

NTSTATUS
Nfs3QueueForeach(
    PNFS3_QUEUE pQueue,
    PFNNFS3_FOREACH_QUEUE_ITEM pfnAction,
    PVOID pUserData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS3_QUEUE_ITEM pQueueItem = pQueue->pHead;

    for (; pQueueItem; pQueueItem = pQueueItem->pNext)
    {
        ntStatus = pfnAction(pQueueItem->pItem, pUserData);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

VOID
Nfs3QueueFree(
    PNFS3_QUEUE pQueue
    )
{
    Nfs3FreeMemory(pQueue);
}
