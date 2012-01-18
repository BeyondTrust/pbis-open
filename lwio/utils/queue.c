/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
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
 *        Likewise Server Message Block (LSMB)
 *
 *        Utilities
 *
 *        Queue
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
LWIOQueueCreate(
    PLWIO_QUEUE* ppQueue
    )
{
    DWORD dwError = 0;
    PLWIO_QUEUE pQueue = NULL;

    dwError = LwIoAllocateMemory(
                    sizeof(LWIO_QUEUE),
                    (PVOID*)&pQueue);
    BAIL_ON_LWIO_ERROR(dwError);

    *ppQueue = pQueue;

cleanup:

    return dwError;

error:

    *ppQueue = NULL;

    goto cleanup;
}

DWORD
SMBEnqueue(
    PLWIO_QUEUE pQueue,
    PVOID      pItem
    )
{
    DWORD dwError = 0;
    PLWIO_QUEUE_ITEM pQueueItem = NULL;

    dwError = LwIoAllocateMemory(
                    sizeof(LWIO_QUEUE_ITEM),
                    (PVOID*)&pQueueItem);
    BAIL_ON_LWIO_ERROR(dwError);

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

    return dwError;

error:

    LWIO_SAFE_FREE_MEMORY(pQueueItem);

    goto cleanup;
}

DWORD
SMBEnqueueFront(
    PLWIO_QUEUE pQueue,
    PVOID      pItem
    )
{
    DWORD dwError = 0;
    PLWIO_QUEUE_ITEM pQueueItem = NULL;

    dwError = LwIoAllocateMemory(
                    sizeof(LWIO_QUEUE_ITEM),
                    (PVOID*)&pQueueItem);
    BAIL_ON_LWIO_ERROR(dwError);

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

    return dwError;

error:

    LWIO_SAFE_FREE_MEMORY(pQueueItem);

    goto cleanup;
}

PVOID
SMBDequeue(
    PLWIO_QUEUE pQueue
    )
{
    PVOID pItem = NULL;

    if (pQueue->pHead)
    {
        PLWIO_QUEUE_ITEM pQueueItem = pQueue->pHead;

        pQueue->pHead = pQueue->pHead->pNext;
        if (!pQueue->pHead)
        {
            pQueue->pTail = NULL;
        }

        pItem = pQueueItem->pItem;

        LwIoFreeMemory(pQueueItem);
    }

    return pItem;
}

BOOLEAN
LWIOQueueIsEmpty(
    PLWIO_QUEUE pQueue
    )
{
    return (pQueue->pHead == NULL);
}

DWORD
LWIOQueueForeach(
    PLWIO_QUEUE pQueue,
    PFNLWIO_FOREACH_QUEUE_ITEM pfnAction,
    PVOID pUserData
    )
{
    DWORD dwError = 0;
    PLWIO_QUEUE_ITEM pQueueItem = pQueue->pHead;

    for (; pQueueItem; pQueueItem = pQueueItem->pNext)
    {
        dwError = pfnAction(pQueueItem->pItem, pUserData);
        BAIL_ON_LWIO_ERROR(dwError);
    }

error:

    return dwError;
}

VOID
LWIOQueueFree(
    PLWIO_QUEUE pQueue
    )
{
    LwIoFreeMemory(pQueue);
}


