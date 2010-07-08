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

    RingInit(&pThreads->WorkItems);

    status = LwErrnoToNtStatus(pthread_mutex_init(&pThreads->Lock, NULL));
    GOTO_ERROR_ON_STATUS(status);
    pThreads->bDestroyLock = TRUE;

    status = LwErrnoToNtStatus(pthread_cond_init(&pThreads->Event, NULL));
    GOTO_ERROR_ON_STATUS(status);
    pThreads->bDestroyEvent = TRUE;

    pThreads->ulWorkThreadCount = GetWorkThreadsAttr(pAttrs, numCpus);
    pThreads->ulWorkThreadStackSize = pAttrs ? pAttrs->ulWorkThreadStackSize : 0;

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
        UNLOCK_THREADS(pThreads);
        
        for (i = 0; i < pThreads->ulWorkThreadCount; i++)
        {
            pthread_join(pThreads->pWorkThreads[i].Thread, NULL);
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
WorkLoop(
    PLW_WORK_THREAD pThread
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRING pRing = NULL;
    PLW_WORK_ITEM pItem = NULL;

    LOCK_THREADS(pThread->pThreads);

    for(;;)
    {
        while (!pThread->pThreads->bShutdown && RingIsEmpty(&pThread->pThreads->WorkItems))
        {
            pthread_cond_wait(&pThread->pThreads->Event, &pThread->pThreads->Lock);
        }

        if (pThread->pThreads->bShutdown)
        {
            break;
        }

        RingDequeue(&pThread->pThreads->WorkItems, &pRing);

        UNLOCK_THREADS(pThread->pThreads);

        pItem = LW_STRUCT_FROM_FIELD(pRing, LW_WORK_ITEM, Ring);
        
        pItem->pfnFunc(pItem->pContext);
        RtlMemoryFree(pItem);

        LOCK_THREADS(pThread->pThreads);
    }

    UNLOCK_THREADS(pThread->pThreads);

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

error:
    
    if (pThreadAttr)
    {
        pthread_attr_destroy(pThreadAttr);
    }

    return status;
}

/*
 * Called with pThreads->Lock held
 */
static
NTSTATUS
StartWorkThreads(
    PLW_WORK_THREADS pThreads
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t i = 0;
    size_t n = 0;

    if (pThreads->ulWorkThreadCount && !pThreads->pWorkThreads)
    {
        status = LW_RTL_ALLOCATE_ARRAY_AUTO(
            &pThreads->pWorkThreads,
            pThreads->ulWorkThreadCount);
        GOTO_ERROR_ON_STATUS(status);
        
        for (i = 0; i < pThreads->ulWorkThreadCount; i++)
        {
            status = StartWorkThread(pThreads, &pThreads->pWorkThreads[i]);
            GOTO_ERROR_ON_STATUS(status);
        }
    }

cleanup:

    return status;
    
error:
    
    /* If we failed in the middle of starting work threads,
       we need to tear down any we started so far */
    if (pThreads->pWorkThreads)
    {
        /* Save index of thread that we failed to create */
        n = i;

        /* Tell threads to exit */
        pThreads->bShutdown = TRUE;
        pthread_cond_broadcast(&pThreads->Event);

        /* We need to join the threads outside the lock
           since the threads need to acquire it to check
           bShutdown */
        UNLOCK_THREADS(pThreads);
        for (i = 0; i < n; i++)
        {
            pthread_join(pThreads->pWorkThreads[i].Thread, NULL);
        }
        LOCK_THREADS(pThreads);
        
        RtlMemoryFree(pThreads->pWorkThreads);
        pThreads->pWorkThreads = NULL;
        pThreads->bShutdown = FALSE;
    }

    goto cleanup;
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

    LOCK_THREADS(pThreads);

    status = LW_RTL_ALLOCATE_AUTO(&pItem);
    GOTO_ERROR_ON_STATUS(status);
    
    RingInit(&pItem->Ring);
    pItem->pfnFunc = pfnFunc;
    pItem->pContext = pContext;
    
    status = StartWorkThreads(pThreads);
    GOTO_ERROR_ON_STATUS(status);
    
    RingEnqueue(&pThreads->WorkItems, &pItem->Ring);
    pthread_cond_signal(&pThreads->Event);
    pItem = NULL;

error:

    UNLOCK_THREADS(pThreads);

    if (pItem)
    {
        RtlMemoryFree(pItem);
    }

    return status;
}
