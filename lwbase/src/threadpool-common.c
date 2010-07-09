/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * HAVE QUESTIONS, OR WISHTO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        threadpool-common.c
 *
 * Abstract:
 *
 *        Thread pool API (common)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"
#include "threadpool-common.h"

static PLW_THREAD_POOL gpDelegatePool = NULL;
static ULONG gpDelegatePoolRefCount = 0;
static pthread_mutex_t gpDelegatePoolLock = PTHREAD_MUTEX_INITIALIZER;

NTSTATUS
AcquireDelegatePool(
    PLW_THREAD_POOL* ppPool
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LW_THREAD_POOL_ATTRIBUTES attrs =
        {
            .bDelegateTasks = FALSE,
            .lTaskThreads = -1,
            .lWorkThreads = 0,
            .ulTaskThreadStackSize = 0,
            .ulWorkThreadStackSize = 0
        };
        

    pthread_mutex_lock(&gpDelegatePoolLock);

    if (!gpDelegatePool)
    {
        status = LwRtlCreateThreadPool(&gpDelegatePool, &attrs);
        GOTO_ERROR_ON_STATUS(status);

        gpDelegatePoolRefCount = 1;
    }
    else
    {
        gpDelegatePoolRefCount++;
    }

    *ppPool = gpDelegatePool;

cleanup:

    pthread_mutex_unlock(&gpDelegatePoolLock);

    return status;

error:

    goto cleanup;
}

VOID
ReleaseDelegatePool(
    PLW_THREAD_POOL* ppPool
    )
{
    if (*ppPool)
    {
        pthread_mutex_lock(&gpDelegatePoolLock);
        
        assert(*ppPool == gpDelegatePool);
        
        if (--gpDelegatePoolRefCount == 0)
        {
            LwRtlFreeThreadPool(&gpDelegatePool);
        }
        
        pthread_mutex_unlock(&gpDelegatePoolLock);
        *ppPool = NULL;
    }
}

LW_NTSTATUS
LwRtlCreateThreadPoolAttributes(
    LW_OUT PLW_THREAD_POOL_ATTRIBUTES* ppAttrs
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_THREAD_POOL_ATTRIBUTES pAttrs = NULL;

    status = LW_RTL_ALLOCATE_AUTO(&pAttrs);
    GOTO_ERROR_ON_STATUS(status);
    
    pAttrs->bDelegateTasks = TRUE;
    pAttrs->lTaskThreads = -1;
    pAttrs->lWorkThreads = -4;
    pAttrs->ulTaskThreadStackSize = 0;
    pAttrs->ulWorkThreadStackSize = 0;

    *ppAttrs = pAttrs;

cleanup:

    return status;

error:

    *ppAttrs = NULL;

    goto cleanup;
}

LW_NTSTATUS
LwRtlSetThreadPoolAttribute(
    LW_IN LW_OUT PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    LW_IN LW_THREAD_POOL_OPTION Option,
    ...
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    va_list ap;

    va_start(ap, Option);

    switch(Option)
    {
    case LW_THREAD_POOL_OPTION_DELEGATE_TASKS:
        pAttrs->bDelegateTasks = va_arg(ap, int);
        break;
    case LW_THREAD_POOL_OPTION_TASK_THREADS:
        pAttrs->lTaskThreads = va_arg(ap, LONG);
        break;
    case LW_THREAD_POOL_OPTION_WORK_THREADS:
        pAttrs->lWorkThreads = va_arg(ap, LONG);
        break;
    case LW_THREAD_POOL_OPTION_TASK_THREAD_STACK_SIZE:
        pAttrs->ulTaskThreadStackSize = va_arg(ap, ULONG);
        break;
    case LW_THREAD_POOL_OPTION_WORK_THREAD_STACK_SIZE:
        pAttrs->ulWorkThreadStackSize = va_arg(ap, ULONG);
        break;
    case LW_THREAD_POOL_OPTION_WORK_THREAD_TIMEOUT:
        pAttrs->ulWorkThreadTimeout = va_arg(ap, ULONG);
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        GOTO_ERROR_ON_STATUS(status);
    }

cleanup:
    
    va_end(ap);

    return status;

error:

    goto cleanup;
}

VOID
LwRtlFreeThreadPoolAttributes(
    LW_IN LW_OUT PLW_THREAD_POOL_ATTRIBUTES* ppAttrs
    )
{
    RTL_FREE(ppAttrs);
}

VOID
SetCloseOnExec(
    int Fd
    )
{
    fcntl(Fd, F_SETFD, FD_CLOEXEC);
}

NTSTATUS
InitWorkThreads(
    PLW_WORK_THREADS pThreads,
    PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    int numCpus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t i = 0;

    RingInit(&pThreads->WorkItems);

    status = LwErrnoToNtStatus(pthread_mutex_init(&pThreads->Lock, NULL));
    GOTO_ERROR_ON_STATUS(status);
    pThreads->bDestroyLock = TRUE;

    status = LwErrnoToNtStatus(pthread_cond_init(&pThreads->Event, NULL));
    GOTO_ERROR_ON_STATUS(status);
    pThreads->bDestroyEvent = TRUE;

    pThreads->ulWorkThreadCount = GetWorkThreadsAttr(pAttrs, numCpus);
    pThreads->ulWorkThreadStackSize = pAttrs ? pAttrs->ulWorkThreadStackSize : 0;
    pThreads->ulWorkThreadTimeout = GetWorkThreadTimeoutAttr(pAttrs);

    if (pThreads->ulWorkThreadCount)
    {
        status = LW_RTL_ALLOCATE_ARRAY_AUTO(
            &pThreads->pWorkThreads,
            pThreads->ulWorkThreadCount);
        GOTO_ERROR_ON_STATUS(status);

        for (i = 0; i < pThreads->ulWorkThreadCount; i++)
        {
            pThreads->pWorkThreads[i].Thread = INVALID_THREAD_HANDLE;
        }
    }
        
error:

    return status;
}

VOID
DestroyWorkThreads(
    PLW_WORK_THREADS pThreads
    )
{
    size_t i = 0;

    if (pThreads->pWorkThreads)
    {
        LOCK_THREADS(pThreads);
        pThreads->bShutdown = TRUE;
        pthread_cond_broadcast(&pThreads->Event);
        
        for (i = 0; i < pThreads->ulWorkThreadCount; i++)
        {
            if (pThreads->pWorkThreads[i].Thread != INVALID_THREAD_HANDLE)
            {
                /* We must pthread_join() outside of the lock */
                UNLOCK_THREADS(pThreads);
                pthread_join(pThreads->pWorkThreads[i].Thread, NULL);
                LOCK_THREADS(pThreads);
            }
        }
            
        RtlMemoryFree(pThreads->pWorkThreads);
    }

    if (pThreads->bDestroyLock)
    {        
        pthread_mutex_destroy(&pThreads->Lock);
    }

    if (pThreads->bDestroyEvent)
    {
        pthread_cond_destroy(&pThreads->Event);
    }
}

static
NTSTATUS
WorkWait(
    PLW_WORK_THREAD pThread
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct timespec ts = {0};
    struct timespec* pTs = NULL;
    LONG64 llDeadline = 0;
    int err = 0;

    if (pThread->pThreads->ulWorkThreadTimeout)
    {
        status = TimeNow(&llDeadline);
        GOTO_ERROR_ON_STATUS(status);

        llDeadline += (LONG64) 1000000000ll * pThread->pThreads->ulWorkThreadTimeout;
        ts.tv_sec = llDeadline / 1000000000ll;
        ts.tv_nsec = llDeadline % 1000000000ll;
        pTs = &ts;
    }
        
    err = pthread_cond_timedwait(&pThread->pThreads->Event, &pThread->pThreads->Lock, pTs);
    
    switch(err)
    {
    case ETIMEDOUT:
        status = STATUS_TIMEOUT;
        GOTO_ERROR_ON_STATUS(status);
    default:
        status = LwErrnoToNtStatus(err);
        GOTO_ERROR_ON_STATUS(status);
    }

error:

    return status;
}

static
NTSTATUS
WorkLoop(
    PLW_WORK_THREAD pThread
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRING pRing = NULL;
    PLW_WORK_ITEM pItem = NULL;
    PLW_WORK_THREADS pThreads = pThread->pThreads;

    LOCK_THREADS(pThread->pThreads);

    for(;;)
    {
        pThreads->ulAvailable++;

        while (!pThreads->bShutdown && pThreads->ulQueued == 0)
        {
            status = WorkWait(pThread);
            GOTO_ERROR_ON_STATUS(status);
        }

        if (pThreads->bShutdown)
        {
            break;
        }

        RingDequeue(&pThreads->WorkItems, &pRing);
        pThreads->ulQueued--;
        pThreads->ulAvailable--;

        UNLOCK_THREADS(pThreads);

        pItem = LW_STRUCT_FROM_FIELD(pRing, LW_WORK_ITEM, Ring);
        
        pItem->pfnFunc(pItem->pContext);
        RtlMemoryFree(pItem);

        LOCK_THREADS(pThreads);
    }

error:

    pThreads->ulAvailable--;
    pThreads->ulStarted--;
    pThread->bStarted = FALSE;

    /* If the thread pool is not being shut down, nothing is
       going to call pthread_join() on this thread, so call
       pthread_detach() now */
    if (!pThreads->bShutdown)
    {
        pthread_detach(pThread->Thread);
        pThread->Thread = INVALID_THREAD_HANDLE;
    }

    UNLOCK_THREADS(pThreads);

    return status;
}

static
PVOID
WorkThread(
    PVOID pContext
    )
{
    WorkLoop((PLW_WORK_THREAD) pContext);

    return NULL;
}

/*
 * Called with pThreads->Lock held
 */
static
NTSTATUS
StartWorkThread(
    PLW_WORK_THREADS pThreads,
    PLW_WORK_THREAD pThread
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    pthread_attr_t threadAttr;
    pthread_attr_t* pThreadAttr = NULL;

    pThread->pThreads = pThreads;

    if (pThreads->ulWorkThreadStackSize)
    {
        status = LwErrnoToNtStatus(pthread_attr_init(&threadAttr));
        GOTO_ERROR_ON_STATUS(status);
        
        pThreadAttr = &threadAttr;
        
        status = LwErrnoToNtStatus(
            pthread_attr_setstacksize(pThreadAttr, pThreads->ulWorkThreadStackSize));
        GOTO_ERROR_ON_STATUS(status);
    }

    status = LwErrnoToNtStatus(
        pthread_create(
            &pThread->Thread,
            pThreadAttr,
            WorkThread,
            pThread));
    GOTO_ERROR_ON_STATUS(status);

    pThread->bStarted = TRUE;
    pThreads->ulStarted++;

error:
    
    if (pThreadAttr)
    {
        pthread_attr_destroy(pThreadAttr);
    }

    return status;
}

NTSTATUS
QueueWorkItem(
    PLW_WORK_THREADS pThreads,
    LW_WORK_ITEM_FUNCTION pfnFunc,
    PVOID pContext,
    LW_WORK_ITEM_FLAGS Flags
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_WORK_ITEM pItem = NULL;
    size_t i = 0;

    LOCK_THREADS(pThreads);
    
    /* If there are not enough available threads
       to handle the item we are about to queue,
       and not all work threads are currently started,
       start a thread now */
    if (pThreads->ulAvailable < pThreads->ulQueued + 1 &&
        pThreads->ulStarted < pThreads->ulWorkThreadCount)
    {
        for (i = 0; i < pThreads->ulWorkThreadCount; i++)
        {
            if (!pThreads->pWorkThreads[i].bStarted)
            {
                status = StartWorkThread(pThreads, &pThreads->pWorkThreads[i]);
                GOTO_ERROR_ON_STATUS(status);
                break;
            }
        }
    }
    /* Otherwise, if a thread is available, signal
       one now so it picks up the item we are about to queue */
    else if (pThreads->ulAvailable)
    {
        pthread_cond_signal(&pThreads->Event);
    }

    status = LW_RTL_ALLOCATE_AUTO(&pItem);
    GOTO_ERROR_ON_STATUS(status);
    
    RingInit(&pItem->Ring);
    pItem->pfnFunc = pfnFunc;
    pItem->pContext = pContext;

    RingEnqueue(&pThreads->WorkItems, &pItem->Ring);
    pThreads->ulQueued++;
    pItem = NULL;

error:

    UNLOCK_THREADS(pThreads);

    if (pItem)
    {
        RtlMemoryFree(pItem);
    }

    return status;
}
