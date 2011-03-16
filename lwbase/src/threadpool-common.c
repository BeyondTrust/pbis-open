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

/* List of signals we leave alone */
static const int SignalBlacklist[] =
{
    SIGILL,
    SIGABRT,
    SIGFPE,
    SIGKILL,
    SIGSEGV,
    SIGCONT,
    SIGSTOP,
#ifdef SIGBUS
    SIGBUS,
#endif
#ifdef SIGEMT
    SIGEMT,
#endif
#ifdef SIGTRAP
    SIGTRAP,
#endif
#ifdef SIGSYS
    SIGSYS,
#endif
#ifdef SIGTSTP
    SIGTSTP,
#endif
#ifdef SIGALRM1
    SIGALRM1,
#endif
#ifdef SIGWAITING
    SIGWAITING,
#endif
    0
};

/* Global thread pool to which tasks are usually delegated */
static PLW_THREAD_POOL gpDelegatePool = NULL;
static ULONG gpDelegatePoolRefCount = 0;
static pthread_mutex_t gpDelegateLock = PTHREAD_MUTEX_INITIALIZER;

static LW_SIGNAL_MULTIPLEX gSignal =
{
    .pSubscribers = NULL,
    .Lock = PTHREAD_MUTEX_INITIALIZER,
    .Status = STATUS_SUCCESS,
    .bExit = FALSE
};

static BOOLEAN volatile gRealSigInt = FALSE;

static
NTSTATUS
StartWorkThread(
    PLW_WORK_THREADS pThreads,
    PLW_WORK_THREAD pThread
    );

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

    if (getenv("LW_GLOBAL_TASK_THREADS"))
    {
        attrs.lTaskThreads = (LONG) atoi(getenv("LW_GLOBAL_TASK_THREADS"));
    }

    pthread_mutex_lock(&gpDelegateLock);

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

    pthread_mutex_unlock(&gpDelegateLock);

    return status;

error:

    goto cleanup;
}

VOID
ReleaseDelegatePool(
    PLW_THREAD_POOL* ppPool
    )
{
    BOOLEAN bFree = FALSE;

    if (*ppPool)
    {
        pthread_mutex_lock(&gpDelegateLock);
        
        assert(*ppPool == gpDelegatePool);
        
        if (--gpDelegatePoolRefCount == 0)
        {
            gpDelegatePool = NULL;
            bFree = TRUE;
        }
        
        pthread_mutex_unlock(&gpDelegateLock);
    }

    if (bFree)
    {
        LwRtlFreeThreadPool(ppPool);
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
    pAttrs->ulWorkThreadTimeout = DEFAULT_WORK_THREAD_TIMEOUT;

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

    if (pThreads->ulWorkThreadTimeout == 0)
    {
        for (i = 0; i < pThreads->ulWorkThreadCount; i++)
        {
            status = StartWorkThread(pThreads, &pThreads->pWorkThreads[i]);
            GOTO_ERROR_ON_STATUS(status);
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
        WaitWorkItems(pThreads);

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
        UNLOCK_THREADS(pThreads);

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
    LONG64 llDeadline = 0;
    int err = 0;
    BOOLEAN bLastThread = FALSE;
    PLW_WORK_THREADS pThreads = pThread->pThreads;
    
    while (pThreads->ulQueued == 0)
    {
        if (pThreads->bShutdown)
        {
            status = STATUS_CANCELLED;
            GOTO_ERROR_ON_STATUS(status);
        }

        if (pThread->pThreads->ulWorkThreadTimeout && !bLastThread)
        {
            status = TimeNow(&llDeadline);
            GOTO_ERROR_ON_STATUS(status);

            llDeadline += (LONG64) 1000000000ll * pThread->pThreads->ulWorkThreadTimeout;
            ts.tv_sec = llDeadline / 1000000000ll;
            ts.tv_nsec = llDeadline % 1000000000ll;
            err = pthread_cond_timedwait(&pThread->pThreads->Event, &pThread->pThreads->Lock, &ts);
        }
        else
        {
            err = pthread_cond_wait(&pThread->pThreads->Event, &pThread->pThreads->Lock);
        }

        bLastThread = pThreads->ulWorkItemCount && pThreads->ulStarted == 1;

        switch(err)
        {
        case ETIMEDOUT:
            if (!bLastThread)
            {
                status = STATUS_TIMEOUT;
                GOTO_ERROR_ON_STATUS(status);
            }
            break;
        default:
            status = LwErrnoToNtStatus(err);
            GOTO_ERROR_ON_STATUS(status);
        }
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

        status = WorkWait(pThread);
        GOTO_ERROR_ON_STATUS(status);

        RingDequeue(&pThreads->WorkItems, &pRing);
        pThreads->ulQueued--;
        pThreads->ulAvailable--;

        UNLOCK_THREADS(pThreads);

        pItem = LW_STRUCT_FROM_FIELD(pRing, LW_WORK_ITEM, Ring);
        pItem->pfnFunc(pItem, pItem->pContext);

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

        status = LwRtlResetAffinityThreadAttribute(pThreadAttr);
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
CreateWorkItem(
    LW_IN PLW_WORK_THREADS pThreads,
    LW_OUT PLW_WORK_ITEM* ppWorkItem,
    LW_WORK_ITEM_FUNCTION pfnFunc,
    PVOID pContext
    )
{
    PLW_WORK_ITEM pItem = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    LOCK_THREADS(pThreads);

    if (pThreads->ulStarted == 0)
    {
        /* Make sure at least one thread is running */
        status = StartWorkThread(pThreads, &pThreads->pWorkThreads[0]);
        GOTO_ERROR_ON_STATUS(status);
    }

    status = LW_RTL_ALLOCATE_AUTO(&pItem);
    GOTO_ERROR_ON_STATUS(status);

    RingInit(&pItem->Ring);
    pItem->pThreads = pThreads;
    pItem->pfnFunc = pfnFunc;
    pItem->pContext = pContext;

    pThreads->ulWorkItemCount++;

error:

    UNLOCK_THREADS(pThreads);

    *ppWorkItem = pItem;

    return status;
}

VOID
FreeWorkItem(
    LW_IN LW_OUT PLW_WORK_ITEM* ppWorkItem
    )
{
    if (*ppWorkItem)
    {
        LOCK_THREADS((*ppWorkItem)->pThreads);
        (*ppWorkItem)->pThreads->ulWorkItemCount--;
        if ((*ppWorkItem)->pThreads->bWaiting ||
            ((*ppWorkItem)->pThreads->ulWorkItemCount == 0 &&
            (*ppWorkItem)->pThreads->ulStarted == 0))
        {
            pthread_cond_broadcast(&(*ppWorkItem)->pThreads->Event);
        }
        UNLOCK_THREADS((*ppWorkItem)->pThreads);
    }

    RTL_FREE(ppWorkItem);
}

VOID
ScheduleWorkItem(
    PLW_WORK_THREADS pThreads,
    PLW_WORK_ITEM pItem,
    LW_SCHEDULE_FLAGS Flags
    )
{
    size_t i = 0;

    if (pThreads == NULL)
    {
        pThreads = pItem->pThreads;
    }

    LOCK_THREADS(pThreads);
    
    assert(pThreads->ulStarted > 0);

    /* Enqueue work item */
    if (Flags & LW_SCHEDULE_HIGH_PRIORITY)
    {
        RingEnqueueFront(&pThreads->WorkItems, &pItem->Ring);
    }
    else
    {
        RingEnqueue(&pThreads->WorkItems, &pItem->Ring);
    }

    pThreads->ulQueued++;

    /*
     * If there are more pending work items than there
     * are available threads, and not all threads are started,
     * try to start another one to handle the additional load
     */
    if (pThreads->ulAvailable < pThreads->ulQueued &&
        pThreads->ulStarted < pThreads->ulWorkThreadCount)
    {
        for (i = 0; i < pThreads->ulWorkThreadCount; i++)
        {
            if (!pThreads->pWorkThreads[i].bStarted)
            {
                if (StartWorkThread(pThreads, &pThreads->pWorkThreads[i]) != STATUS_SUCCESS)
                {
                    LW_RTL_LOG_WARNING("Could not start work item thread");
                    /* Signal an existing thread instead */
                    pthread_cond_signal(&pThreads->Event);
                }
                break;
            }
        }
    }
    else if (pThreads->ulAvailable)
    {
        /* Signal an existing thread */
        pthread_cond_signal(&pThreads->Event);
    }


    UNLOCK_THREADS(pThreads);
}

VOID
WaitWorkItems(
    PLW_WORK_THREADS pThreads
    )
{
    LOCK_THREADS(pThreads);

    pThreads->bWaiting = TRUE;

    while (pThreads->ulWorkItemCount)
    {
        pthread_cond_wait(&pThreads->Event, &pThreads->Lock);
    }

    pThreads->bWaiting = FALSE;

    UNLOCK_THREADS(pThreads);
}

#if defined(_SC_NPROCESSORS_ONLN)
ULONG
LwRtlGetCpuCount(
    VOID
    )
{
    int numCpus = sysconf(_SC_NPROCESSORS_ONLN);

    return numCpus >= 1 ? numCpus : 1;
}
#else
ULONG
LwRtlGetCpuCount(
    VOID
    )
{
    return 1;
}
#endif

#if defined(HAVE_PTHREAD_ATTR_SETAFFINITY_NP)
NTSTATUS
LwRtlSetAffinityThreadAttribute(
    pthread_attr_t* pAttr,
    ULONG CpuNumber
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CPU_SET_TYPE cpuSet;

    CPU_ZERO(&cpuSet);
    (void) CPU_SET(CpuNumber, &cpuSet);

    status = LwErrnoToNtStatus(
        pthread_attr_setaffinity_np(pAttr, sizeof(cpuSet), &cpuSet));
    GOTO_ERROR_ON_STATUS(status);

error:

    return status;
}

NTSTATUS
LwRtlResetAffinityThreadAttribute(
    pthread_attr_t* pAttr
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CPU_SET_TYPE cpuSet;
    ULONG numCpus = 0;
    int i = 0;

    CPU_ZERO(&cpuSet);

    numCpus = LwRtlGetCpuCount();
    
    for (i = 0; i < numCpus; i++)
    {
        (void) CPU_SET(i, &cpuSet);
    }
    
    status = LwErrnoToNtStatus(
        pthread_attr_setaffinity_np(pAttr, sizeof(cpuSet), &cpuSet));
    GOTO_ERROR_ON_STATUS(status);

error:

    return status;
}
#else
NTSTATUS
LwRtlSetAffinityThreadAttribute(
    pthread_attr_t* pAttr,
    ULONG CpuNumber
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
LwRtlResetAffinityThreadAttribute(
    pthread_attr_t* pAttr
    )
{
    return STATUS_SUCCESS;
}
#endif

static
VOID
DummyHandler(
    int sig
    )
{
    return;
}

NTSTATUS
RegisterTaskUnixSignal(
    LW_IN PLW_TASK pTask,
    LW_IN int Sig,
    LW_IN LW_BOOLEAN bSubscribe
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t i = 0;
    PRING pBase = NULL;
    PRING pRing = NULL;
    PLW_SIGNAL_SUBSCRIPTION pExisting = NULL;
    PLW_SIGNAL_SUBSCRIPTION pSub = NULL;
    struct sigaction action;
#ifdef SIGRTMAX
    int maxSig = SIGRTMAX;
#else
    int maxSig = SIGUSR2;
#endif

    if (Sig == 0)
    {
        for (i = 1; i < maxSig + 1; i++)
        {
            status = RegisterTaskUnixSignal(pTask, (int) i, bSubscribe);
            if (status)
            {
                return status;
            }
        }

        return STATUS_SUCCESS;
    }

    LOCK_SIGNAL();

    if (Sig > maxSig || Sig < 0)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_ERROR_ON_STATUS(status);
    }

    if (!gSignal.pSubscribers)
    {
        status = LW_RTL_ALLOCATE_ARRAY_AUTO(&gSignal.pSubscribers, maxSig + 1);
        GOTO_ERROR_ON_STATUS(status);

        for (i = 0; i < maxSig + 1; i++)
        {
            RingInit(&gSignal.pSubscribers[i]);
        }
    }

    pBase = &gSignal.pSubscribers[Sig];
    for (pRing = pBase->pNext; pRing != pBase; pRing = pRing->pNext)
    {
        pSub = LW_STRUCT_FROM_FIELD(pRing, LW_SIGNAL_SUBSCRIPTION, Ring);
        if (pSub->pTask == pTask)
        {
            pExisting = pSub;
            break;
        }
    }

    if (bSubscribe && !pExisting)
    {
        if (Sig != SIGINT)
        {
            memset(&action, 0, sizeof(action));
            
            /* Make sure there is a dummy handler for the signal
               so it is actually delivered to the process */
            action.sa_handler = DummyHandler;
            action.sa_flags = 0;
            
            if (sigaction(Sig, &action, NULL) < 0)
            {
                status = LwErrnoToNtStatus(errno);
                GOTO_ERROR_ON_STATUS(status);
            }
        }

        status = LW_RTL_ALLOCATE_AUTO(&pSub);
        GOTO_ERROR_ON_STATUS(status);

        pSub->pTask = pTask;
        pSub->ucRefCount = 1;
        RingInit(&pSub->Ring);
        RingInit(&pSub->DispatchRing);

        RingEnqueue(pBase, &pSub->Ring);

        RetainTask(pTask);
    }
    else if (!bSubscribe && pExisting)
    {
        RingRemove(&pExisting->Ring);

        if (--pExisting->ucRefCount == 0)
        {
            LwRtlReleaseTask(&pExisting->pTask);
            LwRtlMemoryFree(pExisting);
        }
    }

error:

    UNLOCK_SIGNAL();

    return status;
}

static
VOID
DispatchSignal(
    siginfo_t* pInfo
    )
{
    RING dispatch;
    PRING pBase = NULL;
    PRING pRing = NULL;
    PRING pNext = NULL;
    PLW_SIGNAL_SUBSCRIPTION pSub = NULL;

    if (!gSignal.pSubscribers)
    {
        return;
    }

    RingInit(&dispatch);

    pBase = &gSignal.pSubscribers[pInfo->si_signo];
    for (pRing = pBase->pNext; pRing != pBase; pRing = pRing->pNext)
    {
        pSub = LW_STRUCT_FROM_FIELD(pRing, LW_SIGNAL_SUBSCRIPTION, Ring);
        
        pSub->ucRefCount++;
        RingInit(&pSub->DispatchRing);
        RingEnqueue(&dispatch, &pSub->DispatchRing);
    }

    UNLOCK_SIGNAL();
    for (pRing = dispatch.pNext; pRing != &dispatch; pRing = pRing->pNext)
    {
        pSub = LW_STRUCT_FROM_FIELD(pRing, LW_SIGNAL_SUBSCRIPTION, DispatchRing);
        
        NotifyTaskUnixSignal(pSub->pTask, pInfo);
    }
    LOCK_SIGNAL();

    for (pRing = dispatch.pNext; pRing != &dispatch; pRing = pNext)
    {
        pNext = pRing->pNext;
        pSub = LW_STRUCT_FROM_FIELD(pRing, LW_SIGNAL_SUBSCRIPTION, DispatchRing);
        
        if (--pSub->ucRefCount == 0)
        {
            RingRemove(&pSub->Ring);
            LwRtlReleaseTask(&pSub->pTask);
            LwRtlMemoryFree(pSub);
        }
    }
}

static
VOID
InterruptHandler(
    int sig
    )
{
    gRealSigInt = TRUE;
}

LW_NTSTATUS
LwRtlMain(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    sigset_t waitSet;
    sigset_t intSet;
    siginfo_t info = {0};
    struct sigaction action;
    int ret = 0;
    int i = 0;

    memset(&action, 0, sizeof(action));

    LOCK_SIGNAL();

    sigfillset(&waitSet);
    sigfillset(&intSet);
    sigdelset(&intSet, SIGINT);

    for (i = 0; SignalBlacklist[i]; i++)
    {
        sigdelset(&waitSet, SignalBlacklist[i]);
        sigdelset(&intSet, SignalBlacklist[i]);
    }    

    /* Set a special handler for SIGINT */
    action.sa_handler = InterruptHandler;
    action.sa_flags = 0;
        
    if (sigaction(SIGINT, &action, NULL) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        GOTO_ERROR_ON_STATUS(status);
    }

    status = LwRtlBlockSignals();
    GOTO_ERROR_ON_STATUS(status);

    for (;;)
    {
        UNLOCK_SIGNAL();
        // sigwaitinfo on HPUX 11.11 does not fill-in info.si_signo.
#if defined(HAVE_SIGWAITINFO) && !defined(__LWI_HP_UX__)
        ret = sigwaitinfo(&waitSet, &info);
#else
        ret = sigwait(&waitSet, &info.si_signo);
#endif
        LOCK_SIGNAL();

        if (ret < 0 && errno == EINTR)
        {
            continue;
        }

        if (ret < 0)
        {
            status = LwErrnoToNtStatus(errno);
            GOTO_ERROR_ON_STATUS(status);
        }

        if (gSignal.bExit)
        {
            status = gSignal.Status;
            gSignal.bExit = FALSE;
            gSignal.Status = STATUS_SUCCESS;
            break;
        }

        if (info.si_signo == SIGINT)
        {
            /* Make hitting ^C to break into a process in gdb work.
             *  
             * gdb can't trap SIGINT when we receive it with sigwaitinfo(),
             * so we reraise it against the process and then unblock it to
             * give the debugger a chance to intercept it.  If it is not
             * intercepted, InterruptHandler() will run and set gRealSigInt,
             * and we will forward the original SIGINT to all subscribed
             * tasks.  If the signal is intercepted, we pretend the original
             * SIGINT never happened, and the desired debugging behavior 
             * is preserved.
             */
            gRealSigInt = FALSE;
            kill(getpid(), SIGINT);
            status = LwErrnoToNtStatus(pthread_sigmask(SIG_SETMASK, &intSet, NULL));
            GOTO_ERROR_ON_STATUS(status);
            status = LwRtlBlockSignals();
            GOTO_ERROR_ON_STATUS(status);

            if (!gRealSigInt)
            {
                continue;
            }
        }

        DispatchSignal(&info);
    }

error:

    UNLOCK_SIGNAL();

    return status;
}

LW_VOID
LwRtlExitMain(
    LW_NTSTATUS Status
    )
{
    LOCK_SIGNAL();
    
    gSignal.Status = Status;
    gSignal.bExit = TRUE;
    kill(getpid(), SIGINT);

    UNLOCK_SIGNAL();
}

LW_NTSTATUS
LwRtlBlockSignals(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    sigset_t blockSet;
    size_t i = 0;

    sigfillset(&blockSet);

    /* Don't block blacklisted signals */
    for (i = 0; SignalBlacklist[i]; i++)
    {
        sigdelset(&blockSet, SignalBlacklist[i]);
    }

    status = LwErrnoToNtStatus(pthread_sigmask(SIG_SETMASK, &blockSet, NULL));
    GOTO_ERROR_ON_STATUS(status);

error:

    return status;
}

typedef struct COMPAT_WORK_ITEM
{
    LW_WORK_ITEM_FUNCTION_COMPAT pfnFunc;
    PVOID pContext;
} COMPAT_WORK_ITEM, *PCOMPAT_WORK_ITEM;

static
VOID
CompatWorkItem(
    PLW_WORK_ITEM pItem,
    PVOID pContext
    )
{
    PCOMPAT_WORK_ITEM pCompat = pContext;

    pCompat->pfnFunc(pCompat->pContext);

    LwRtlFreeWorkItem(&pItem);
    RTL_FREE(&pCompat);
}

LW_NTSTATUS
LwRtlQueueWorkItem(
    LW_IN PLW_THREAD_POOL pPool,
    LW_IN LW_WORK_ITEM_FUNCTION_COMPAT pfnFunc,
    LW_IN LW_PVOID pContext,
    LW_IN LW_WORK_ITEM_FLAGS Flags
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PCOMPAT_WORK_ITEM pCompat = NULL;
    PLW_WORK_ITEM pItem = NULL;

    status = LW_RTL_ALLOCATE_AUTO(&pCompat);
    GOTO_ERROR_ON_STATUS(status);

    pCompat->pfnFunc = pfnFunc;
    pCompat->pContext = pContext;

    status = LwRtlCreateWorkItem(pPool, &pItem, CompatWorkItem, pCompat);
    GOTO_ERROR_ON_STATUS(status);
    pCompat = NULL;

    LwRtlScheduleWorkItem(pItem, Flags);
    pItem = NULL;

error:

    RTL_FREE(&pCompat);
    LwRtlFreeWorkItem(&pItem);

    return status;
}

static
__attribute__((destructor))
VOID
LwRtlThreadpoolDestructor(
    VOID
    )
{
    RTL_FREE(&gSignal.pSubscribers);
}
