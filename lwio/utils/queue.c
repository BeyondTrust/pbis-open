/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        queue.c
 *
 * Abstract:
 *
 *        BeyondTrust Server Message Block (LSMB)
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


