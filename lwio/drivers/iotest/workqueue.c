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
 *        irpcontext.c
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lwio/iodriver.h>
#include "workqueue.h"
#include "lwthreads.h"
#include "lwlist.h"
#include <lw/rtlgoto.h>
#include "lwioutils.h"
#include "ntlogmacros.h"

typedef ULONG IOTEST_WORK_ITEM_FLAGS;

#define IOTEST_WORK_ITEM_FLAG_IN_WORK_QUEUE 0x00000001

typedef struct _IOTEST_WORK_ITEM {
    IOTEST_WORK_ITEM_FLAGS Flags;
    LW_RTL_WINDOWS_TIME EventTime;
    PVOID pContext;
    PIOTEST_WORK_CALLBACK Callback;
    LW_LIST_LINKS QueueLinks;
    LW_LIST_LINKS RunQueueLinks;
} IOTEST_WORK_ITEM;

typedef struct _IOTEST_WORK_QUEUE {
    // List of PIOTEST_WORK_ITEM
    LW_LIST_LINKS Head;
    PLW_RTL_THREAD pThread;
    BOOLEAN IsShutdown;
    LW_RTL_MUTEX Mutex;
    LW_RTL_CONDITION_VARIABLE Condition;
} IOTEST_WORK_QUEUE;

static
BOOLEAN
ItpRemoveLockedWorkQueue(
    IN PIOTEST_WORK_ITEM pWorkItem
    );

NTSTATUS
ItCreateWorkItem(
    OUT PIOTEST_WORK_ITEM* ppWorkItem
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIOTEST_WORK_ITEM pWorkItem = NULL;

    status = RTL_ALLOCATE(&pWorkItem, IOTEST_WORK_ITEM, sizeof(*pWorkItem));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    *ppWorkItem = pWorkItem;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

VOID
ItDestroyWorkItem(
    IN OUT PIOTEST_WORK_ITEM* ppWorkItem
    )
{
    RTL_FREE(ppWorkItem);
}

static
PVOID
ItpRunWorkQueue(
    IN PVOID pContext
    )
{
    PIOTEST_WORK_QUEUE pWorkQueue = (PIOTEST_WORK_QUEUE) pContext;
    LW_LIST_LINKS runListHead = { 0 };
    BOOLEAN isAcquired = FALSE;

    LwListInit(&runListHead);

    for (;;)
    {
        PLW_RTL_WINDOWS_TIME pTimeout = NULL;
        PIOTEST_WORK_ITEM pWorkItem = NULL;
        BOOLEAN isSignalled = FALSE;
        NTSTATUS status = STATUS_SUCCESS;
        LW_RTL_WINDOWS_TIME now = 0;
        PLW_LIST_LINKS pLinks = NULL;
        PLW_LIST_LINKS pNextLinks = NULL;

        if (!isAcquired)
        {
            LwRtlLockMutex(&pWorkQueue->Mutex);
            isAcquired = TRUE;
        }

        // Find the timeout for the earliest event, if any.

        if (!LwListIsEmpty(&pWorkQueue->Head))
        {
            pLinks = pWorkQueue->Head.Next;
            pWorkItem = LW_STRUCT_FROM_FIELD(pLinks, IOTEST_WORK_ITEM, QueueLinks);

            pTimeout = &pWorkItem->EventTime;
            pWorkItem = NULL;
        }

        isSignalled = LwRtlWaitConditionVariable(&pWorkQueue->Condition, &pWorkQueue->Mutex, pTimeout);
        if (pWorkQueue->IsShutdown)
        {
            break;
        }

        // Find any events that need to be run and put
        // them in a separate local queue.

        LWIO_ASSERT(isSignalled || pTimeout);

        status = LwRtlGetCurrentWindowsTime(&now);
        LWIO_ASSERT(!status);

        for (pLinks = pWorkQueue->Head.Next;
             pLinks != &pWorkQueue->Head;
             pLinks = pNextLinks)
        {
            pNextLinks = pLinks->Next;
            pWorkItem = LW_STRUCT_FROM_FIELD(pLinks, IOTEST_WORK_ITEM, QueueLinks);

            if (pWorkItem->EventTime > now)
            {
                break;
            }

            ItpRemoveLockedWorkQueue(pWorkItem);
            LwListInsertTail(&runListHead, &pWorkItem->QueueLinks);
        }

        // Run events w/o holding the queue lock.

        LwRtlUnlockMutex(&pWorkQueue->Mutex);
        isAcquired = FALSE;

        while (!LwListIsEmpty(&runListHead))
        {
            pLinks = LwListRemoveHead(&runListHead);
            pWorkItem = LW_STRUCT_FROM_FIELD(pLinks, IOTEST_WORK_ITEM, QueueLinks);

            pWorkItem->Callback(pWorkItem, pWorkItem->pContext);
        }
    }

    return NULL;
}

NTSTATUS
ItCreateWorkQueue(
    OUT PIOTEST_WORK_QUEUE* ppWorkQueue
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIOTEST_WORK_QUEUE pWorkQueue = NULL;

    status = LW_RTL_ALLOCATE(&pWorkQueue, IOTEST_WORK_QUEUE, sizeof(*pWorkQueue));
    GOTO_CLEANUP_ON_STATUS(status);

    LwListInit(&pWorkQueue->Head);

    status = LwRtlInitializeConditionVariable(&pWorkQueue->Condition);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlInitializeMutex(&pWorkQueue->Mutex, FALSE);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlCreateThread(&pWorkQueue->pThread, ItpRunWorkQueue, pWorkQueue);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (status)
    {
        ItDestroyWorkQueue(&pWorkQueue);
    }

    *ppWorkQueue = pWorkQueue;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

VOID
ItDestroyWorkQueue(
    IN OUT PIOTEST_WORK_QUEUE* ppWorkQueue
    )
{
    PIOTEST_WORK_QUEUE pWorkQueue = *ppWorkQueue;

    if (pWorkQueue)
    {
        if (pWorkQueue->pThread)
        {
            pWorkQueue->IsShutdown = TRUE;
            LwRtlSignalConditionVariable(&pWorkQueue->Condition);
            LwRtlJoinThread(pWorkQueue->pThread);
        }
        LwRtlCleanupConditionVariable(&pWorkQueue->Condition);
        LwRtlCleanupMutex(&pWorkQueue->Mutex);
        LW_RTL_FREE(&pWorkQueue);
        *ppWorkQueue = NULL;
    }
}

NTSTATUS
ItAddWorkQueue(
    IN PIOTEST_WORK_QUEUE pWorkQueue,
    IN PIOTEST_WORK_ITEM pWorkItem,
    IN PVOID pContext,
    IN ULONG WaitSeconds,
    IN PIOTEST_WORK_CALLBACK Callback
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PLW_LIST_LINKS pLinks = NULL;
    LW_RTL_WINDOWS_TIME now = 0;

    status = LwRtlGetCurrentWindowsTime(&now);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pWorkItem->pContext = pContext;
    pWorkItem->EventTime = now + LW_RTL_WINDOWS_TIMESPAN_SECOND * WaitSeconds;
    pWorkItem->Callback = Callback;

    if (pWorkItem->EventTime < now)
    {
        status = STATUS_INTEGER_OVERFLOW;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    LwRtlLockMutex(&pWorkQueue->Mutex);

    for (pLinks = pWorkQueue->Head.Next;
         pLinks != &pWorkQueue->Head;
         pLinks = pLinks->Next)
    {
        PIOTEST_WORK_ITEM pCurrentItem = LW_STRUCT_FROM_FIELD(pLinks, IOTEST_WORK_ITEM, QueueLinks);
        if (pCurrentItem->EventTime > pWorkItem->EventTime)
        {
            break;
        }
    }

    SetFlag(pWorkItem->Flags, IOTEST_WORK_ITEM_FLAG_IN_WORK_QUEUE);
    LwListInsertBefore(pLinks, &pWorkItem->QueueLinks);

    LwRtlSignalConditionVariable(&pWorkQueue->Condition);

    LwRtlUnlockMutex(&pWorkQueue->Mutex);

cleanup:
    LWIO_ASSERT(!status);
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
BOOLEAN
ItpRemoveLockedWorkQueue(
    IN PIOTEST_WORK_ITEM pWorkItem
    )
{
    BOOLEAN isFound = FALSE;

    if (IsSetFlag(pWorkItem->Flags, IOTEST_WORK_ITEM_FLAG_IN_WORK_QUEUE))
    {
        ClearFlag(pWorkItem->Flags, IOTEST_WORK_ITEM_FLAG_IN_WORK_QUEUE);
        LwListRemove(&pWorkItem->QueueLinks);
        isFound = TRUE;
    }

    return isFound;
}

BOOLEAN
ItRemoveWorkQueue(
    IN PIOTEST_WORK_QUEUE pWorkQueue,
    IN PIOTEST_WORK_ITEM pWorkItem
    )
{
    BOOLEAN isFound = FALSE;

    LwRtlLockMutex(&pWorkQueue->Mutex);
    isFound = ItpRemoveLockedWorkQueue(pWorkItem);
    LwRtlUnlockMutex(&pWorkQueue->Mutex);

    return isFound;
}
