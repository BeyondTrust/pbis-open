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
 *        threadpool.c
 *
 * Abstract:
 *
 *        Thread pool API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"
#include "threadpool-select.h"

#define EVENT_THREAD_COUNT 1
#define WORK_THREAD_COUNT 8
#define TASK_COMPLETE_MASK 0xFFFFFFFF

static
VOID
TaskDelete(
    PSELECT_TASK pTask
    )
{
    RTL_FREE(&pTask->pUnixSignal);
    RtlMemoryFree(pTask);
}

static
VOID
SignalThread(
    PSELECT_THREAD pThread
    )
{
    char c = 0;
    int res = 0;

    if (!pThread->bSignalled)
    {
        res = write(pThread->SignalFds[1], &c, sizeof(c));
        assert(res == sizeof(c));
        pThread->bSignalled = TRUE;
    }
}

static
NTSTATUS
TaskProcessTrigger(
    PSELECT_TASK pTask,
    LONG64 llNow
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LONG64 llNewTime = 0;

    /* If task had a deadline, set the time we pass into
       the function to the time remaining */
    if (pTask->llDeadline != 0)
    {
        llNewTime = pTask->llDeadline - llNow;

        if (llNewTime < 0)
        {
            llNewTime = 0;
        }
    }

    pTask->pfnFunc(
        pTask,
        pTask->pFuncContext,
        pTask->TriggerArgs,
        &pTask->TriggerWait,
        &llNewTime);

    /* Unset trigger arguments */
    pTask->TriggerArgs = 0;

    /* If the function gave us a valid time,
       update the task deadline */
    if (llNewTime != 0)
    {
        pTask->llDeadline = llNow + llNewTime;
    }
    else
    {
        pTask->llDeadline = 0;
    }

    return status;
}

static
VOID
UpdateTriggerWait(
    PSELECT_TASK pTask,
    int* pNfds,
    fd_set* pReadSet,
    fd_set* pWriteSet,
    fd_set* pExceptSet,
    PLONG64 pllNextDeadline
    )
{
    if (pTask->TriggerWait & LW_TASK_EVENT_FD_READABLE)
    {
        if (pTask->FdWaitMask & LW_TASK_EVENT_FD_READABLE)
        {
            FD_SET(pTask->Fd, pReadSet);
            if (pTask->Fd >= *pNfds)
            {
                *pNfds = pTask->Fd + 1;
            }
        }
    }

    if (pTask->TriggerWait & LW_TASK_EVENT_FD_WRITABLE)
    {
        if (pTask->FdWaitMask & LW_TASK_EVENT_FD_WRITABLE)
        {
            FD_SET(pTask->Fd, pWriteSet);
            if (pTask->Fd >= *pNfds)
            {
                *pNfds = pTask->Fd + 1;
            }
        }
    }

    if (pTask->TriggerWait & LW_TASK_EVENT_FD_EXCEPTION)
    {
        if (pTask->FdWaitMask & LW_TASK_EVENT_FD_EXCEPTION)
        {
            FD_SET(pTask->Fd, pExceptSet);
            if (pTask->Fd >= *pNfds)
            {
                *pNfds = pTask->Fd + 1;
            }
        }
    }

    if (pTask->TriggerWait & LW_TASK_EVENT_TIME &&
        pTask->llDeadline != 0)
    {
        if (*pllNextDeadline == 0 ||
            pTask->llDeadline < *pllNextDeadline)
        {
            *pllNextDeadline = pTask->llDeadline;
        }
    }
}

static
VOID
UpdateTriggerSet(
    PSELECT_TASK pTask,
    fd_set* pReadSet,
    fd_set* pWriteSet,
    fd_set* pExceptSet,
    LONG64 llNow
    )
{
    if (pTask->Fd >= 0)
    {
        pTask->FdSetMask = 0;

        if (FD_ISSET(pTask->Fd, pReadSet))
        {
            FD_CLR(pTask->Fd, pReadSet);
            pTask->TriggerSet |= LW_TASK_EVENT_FD_READABLE;
            pTask->FdSetMask |= LW_TASK_EVENT_FD_READABLE;
        }
        
        if (FD_ISSET(pTask->Fd, pWriteSet))
        {
            FD_CLR(pTask->Fd, pWriteSet);
            pTask->TriggerSet |= LW_TASK_EVENT_FD_WRITABLE;
            pTask->FdSetMask |= LW_TASK_EVENT_FD_WRITABLE;
        }
        
        if (FD_ISSET(pTask->Fd, pExceptSet))
        {
            FD_CLR(pTask->Fd, pExceptSet);
            pTask->TriggerSet |= LW_TASK_EVENT_FD_EXCEPTION;
            pTask->FdSetMask |= LW_TASK_EVENT_FD_EXCEPTION;
        }
    }

    /* If the task's deadline has expired, set the time event bit */
    if (pTask->TriggerWait & LW_TASK_EVENT_TIME &&
        pTask->llDeadline != 0 &&
        pTask->llDeadline <= llNow)
    {
        pTask->TriggerSet |= LW_TASK_EVENT_TIME;
    }
}

static
NTSTATUS
Sleep(
    IN PCLOCK pClock,
    IN OUT PLONG64 pllNow,
    IN int nfds,
    IN OUT fd_set* pReadSet,
    IN OUT fd_set* pWriteSet,
    IN OUT fd_set* pExceptSet,
    IN LONG64 llNextDeadline,
    OUT int* pReady
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LONG64 llDiff = 0;
    struct timeval timeout = {0};
    int ready = 0;

    if (llNextDeadline != 0)
    {
        /* We have a pending deadline, so set up a timeout for select() */
        do
        {
            llDiff = llNextDeadline - *pllNow;

            if (llDiff >= 0)
            {
                /* Calculate whole seconds */
                timeout.tv_sec = llDiff / 1000000000ll;
                /* Convert nanosecond remainder to microseconds */
                timeout.tv_usec = (llDiff % 1000000000ll) / 1000;
            }
            else
            {
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
            }
            
            ready = select(nfds, pReadSet, pWriteSet, pExceptSet, &timeout);
            
            if (ready < 0 && errno == EINTR)
            {
                /* Update current time so the next timeout calculation is correct */
                GOTO_ERROR_ON_STATUS(status = ClockGetMonotonicTime(pClock, pllNow));
            }
        } while (ready < 0 && errno == EINTR);
    }
    else
    {
        /* No deadline is pending, so select() indefinitely */
        do
        {
            ready = select(nfds, pReadSet, pWriteSet, pExceptSet, NULL);
        } while (ready < 0 && errno == EINTR);
    }

    if (ready < 0)
    {
        GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(errno));
    }

    *pReady = ready;

error:

    return status;
}

static
NTSTATUS
EventLoop(
    PSELECT_THREAD pThread
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    RING tasks;
    RING runnable;
    PRING pRing = NULL;
    PRING pNext = NULL;
    PSELECT_TASK pTask = NULL;
    CLOCK clock = {0};
    LONG64 llNow;
    LONG64 llNextDeadline;
    fd_set readSet;
    fd_set writeSet;
    fd_set exceptSet;
    int ready = 0;
    int nfds = 0;
    char c = 0;
    int res = 0;
    BOOLEAN bShutdown = FALSE;
    PLW_TASK_GROUP pGroup = NULL;
    BOOLEAN bYielding = FALSE;

    RingInit(&tasks);
    RingInit(&runnable);

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    FD_ZERO(&exceptSet);

    LOCK_THREAD(pThread);

    while (!bShutdown || !RingIsEmpty(&tasks))
    {
        /* Reset variables */
        llNextDeadline = 0;
        nfds = 0;
        bYielding = FALSE;

        /* Get current time for this iteration */
        GOTO_ERROR_ON_STATUS(status = ClockGetMonotonicTime(&clock, &llNow));
        
        /* Figure out which tasks are runnable */
        for (pRing = tasks.pNext; pRing != &tasks; pRing = pNext)
        {
            pNext = pRing->pNext;

            pTask = LW_STRUCT_FROM_FIELD(pRing, SELECT_TASK, EventRing);

            /* Update trigger set with results from select() */
            UpdateTriggerSet(
                pTask,
                &readSet,
                &writeSet,
                &exceptSet,
                llNow);
            
            /* Schedule tasks to run if they have been triggered or were yielding */
            if ((pTask->TriggerWait & LW_TASK_EVENT_YIELD) ||
                ((pTask->TriggerWait | LW_TASK_EVENT_EXPLICIT) & pTask->TriggerSet))
            {
                /* Put task on a separate list to run its trigger function */
                RingRemove(&pTask->EventRing);
                RingInsertBefore(&runnable, &pTask->EventRing);
                /* Update the trigger args with the trigger set */
                pTask->TriggerArgs |= pTask->TriggerSet;
                /* Turn off bits (except cancel) now that we have copied them */
                pTask->TriggerSet &= (LW_TASK_EVENT_CANCEL);

            }
            else
            {
                /* Update select parameters to wait for task to trigger */
                UpdateTriggerWait(
                    pTask,
                    &nfds,
                    &readSet,
                    &writeSet,
                    &exceptSet,
                    &llNextDeadline);
            }
        }

        UNLOCK_THREAD(pThread);

        for (pRing = runnable.pNext; pRing != &runnable; pRing = pNext)
        {
            pNext = pRing->pNext;
            
            pTask = LW_STRUCT_FROM_FIELD(pRing, SELECT_TASK, EventRing);
            
            GOTO_ERROR_ON_STATUS(status = TaskProcessTrigger(pTask, llNow));
            
            if (pTask->TriggerWait != 0)
            {
                /* Task is still waiting to be runnable, update select parameters
                   and put it back in the task list */
                UpdateTriggerWait(
                    pTask,
                    &nfds,
                    &readSet,
                    &writeSet,
                    &exceptSet,
                    &llNextDeadline);
            
                if (pTask->TriggerWait & LW_TASK_EVENT_YIELD)
                {
                    /* Task is yielding temporarily.  Set the yield flag on
                       its trigger arguments.   Leave it on the runnable list */
                    pTask->TriggerArgs |= LW_TASK_EVENT_YIELD;
                }
                else
                {    
                    RingRemove(&pTask->EventRing);
                    RingInsertBefore(&tasks, &pTask->EventRing);
                }
            }
            else
            {
                /* Task is complete, notify and remove from task group
                   if it is in one */
                
                RingRemove(&pTask->EventRing);
                
                /* Unregister task from global signal loop */
                if (pTask->pUnixSignal)
                {
                    RegisterTaskUnixSignal(pTask, 0, FALSE);
                }

                pGroup = pTask->pGroup;
                
                if (pGroup)
                {
                    LOCK_GROUP(pGroup);
                    pTask->pGroup = NULL;
                    RingRemove(&pTask->GroupRing);
                    pthread_cond_broadcast(&pGroup->Event);
                    UNLOCK_GROUP(pGroup);
                }
                
                LOCK_THREAD(pThread);
                if (--pTask->ulRefCount)
                {
                    pTask->TriggerSet = TASK_COMPLETE_MASK;
                    pthread_cond_broadcast(&pThread->Event);
                    UNLOCK_THREAD(pThread);
                }
                else
                {
                    UNLOCK_THREAD(pThread);
                    TaskDelete(pTask);
                }
            }
        }

        if (!RingIsEmpty(&runnable))
        {
            /* We have runnable tasks that are yielding.  Move them
               back to the event list and note the fact. */
            bYielding = TRUE;
            RingMove(&runnable, &tasks);
        }

        if (!bShutdown)
        {
            /* Also wait for a poke on the thread's signal fd */
            FD_SET(pThread->SignalFds[0], &readSet);
            if (pThread->SignalFds[0] >= nfds)
            {
                nfds = pThread->SignalFds[0] + 1;
            }
        }

        if (nfds)
        {
            /* If there are still runnable tasks due to
               LW_TASK_EVENT_YIELD, set the next deadline
               to now so we wake immediately.  This gives other
               tasks the chance to become runnable before we
               proceed */
            if (bYielding)
            {
                llNextDeadline = llNow;
            }

            /* Wait for a task to be runnable */
            GOTO_ERROR_ON_STATUS(status = Sleep(
                              &clock,
                              &llNow,
                              nfds,
                              &readSet,
                              &writeSet,
                              &exceptSet,
                              llNextDeadline,
                              &ready));
        }
        
        LOCK_THREAD(pThread);

        /* Check for a signal to the thread */
        if (FD_ISSET(pThread->SignalFds[0], &readSet))
        {
            FD_CLR(pThread->SignalFds[0], &readSet);
            pThread->bSignalled = FALSE;
            
            res = read(pThread->SignalFds[0], &c, sizeof(c));
            assert(res == sizeof(c));
            
            /* Move all tasks in queue into local task list */
            RingMove(&pThread->Tasks, &tasks);

            if (pThread->bShutdown && !bShutdown)
            {
                bShutdown = pThread->bShutdown;
                
                /* Cancel all outstanding tasks */
                for (pRing = tasks.pNext; pRing != &tasks; pRing = pRing->pNext)
                {
                    pTask = LW_STRUCT_FROM_FIELD(pRing, SELECT_TASK, EventRing);
                    pTask->TriggerSet |= LW_TASK_EVENT_CANCEL | LW_TASK_EVENT_EXPLICIT;
                }
            }
        }
    }

error:

    UNLOCK_THREAD(pThread);

    return status;
}
    
static
PVOID
EventThread(
    PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = EventLoop((PSELECT_THREAD) pContext);
    if (!NT_SUCCESS(status))
    {
        LW_RTL_LOG_ERROR(
            "Task thread exiting with fatal error: %s (0x%x)",
            LwNtStatusToName(status),
            status);
        abort();
    }

    return NULL;
}

NTSTATUS
LwRtlCreateTask(
    PLW_THREAD_POOL pPool,
    PLW_TASK* ppTask,
    PLW_TASK_GROUP pGroup,
    LW_TASK_FUNCTION pfnFunc,
    PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSELECT_TASK pTask = NULL;

    if (pPool->pDelegate)
    {
        return LwRtlCreateTask(pPool->pDelegate, ppTask, pGroup, pfnFunc, pContext);
    }

    GOTO_ERROR_ON_STATUS(status = LW_RTL_ALLOCATE_AUTO(&pTask));

    RingInit(&pTask->GroupRing);
    RingInit(&pTask->EventRing);
    pTask->pPool = pPool;
    pTask->pGroup = pGroup;
    pTask->ulRefCount = 2;
    pTask->pfnFunc = pfnFunc;
    pTask->pFuncContext = pContext;
    pTask->Fd = -1;
    pTask->TriggerSet = LW_TASK_EVENT_INIT;
    pTask->TriggerWait = LW_TASK_EVENT_EXPLICIT;
    pTask->llDeadline = 0;

    LOCK_POOL(pPool);
    if (pGroup)
    {
        LOCK_GROUP(pGroup);
        if (pGroup->bCancelled)
        {
            UNLOCK_GROUP(pGroup);
            UNLOCK_POOL(pPool);
            status = STATUS_CANCELLED;
            GOTO_ERROR_ON_STATUS(status);
        }
        RingInsertBefore(&pGroup->Tasks, &pTask->GroupRing);
    }

    pTask->pThread = &pPool->pEventThreads[pPool->ulNextEventThread];
    pPool->ulNextEventThread = (pPool->ulNextEventThread + 1) % pPool->ulEventThreadCount;
    UNLOCK_POOL(pPool);

    LOCK_THREAD(pTask->pThread);
    RingInsertBefore(&pTask->pThread->Tasks, &pTask->EventRing);
    /* It's not necessary to signal the thread about the new task here
       since it won't be run anyway */
    UNLOCK_THREAD(pTask->pThread);

    if (pGroup)
    {
        UNLOCK_GROUP(pGroup);
    }

    *ppTask = pTask;

error:

    return status;
}

NTSTATUS
LwRtlCreateTaskGroup(
    PLW_THREAD_POOL pPool,
    PLW_TASK_GROUP* ppGroup
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_TASK_GROUP pGroup = NULL;

    if (pPool->pDelegate)
    {
        return LwRtlCreateTaskGroup(pPool->pDelegate, ppGroup);
    }

    GOTO_ERROR_ON_STATUS(status = LW_RTL_ALLOCATE_AUTO(&pGroup));

    RingInit(&pGroup->Tasks);
    pGroup->pPool = pPool;

    GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(pthread_mutex_init(&pGroup->Lock, NULL)));
    pGroup->bLockInit = TRUE;
    GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(pthread_cond_init(&pGroup->Event, NULL)));
    pGroup->bEventInit = TRUE;

    *ppGroup = pGroup;

cleanup:

    return status;

error:

    LwRtlFreeTaskGroup(&pGroup);
    *ppGroup = NULL;

    goto cleanup;
}

VOID
LwRtlReleaseTask(
    PLW_TASK* ppTask
    )
{
    PLW_TASK pTask = *ppTask;
    int ulRefCount = 0;

    if (pTask)
    {
        LOCK_THREAD(pTask->pThread);
        ulRefCount = --pTask->ulRefCount;
        UNLOCK_THREAD(pTask->pThread);
        
        if (ulRefCount == 0)
        {
            TaskDelete(pTask);
        }
        
        *ppTask = NULL;
    }
}

VOID
RetainTask(
    PLW_TASK pTask
    )
{
    if (pTask)
    {
        LOCK_THREAD(pTask->pThread);
        ++pTask->ulRefCount;
        UNLOCK_THREAD(pTask->pThread);
    }
}


VOID
LwRtlFreeTaskGroup(
    PLW_TASK_GROUP* ppGroup
    )
{
    PLW_TASK_GROUP pGroup = *ppGroup;

    if (pGroup)
    {
        if (pGroup->bLockInit)
        {
            pthread_mutex_destroy(&pGroup->Lock);
        }

        if (pGroup->bEventInit)
        {
            pthread_cond_destroy(&pGroup->Event);
        }

        RtlMemoryFree(pGroup);

        *ppGroup = NULL;
    }
}

LW_NTSTATUS
LwRtlSetTaskFd(
    PLW_TASK pTask,
    int Fd,
    LW_TASK_EVENT_MASK Mask
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (Fd < 0 || Fd >= FD_SETSIZE)
    {
        status = STATUS_INVALID_HANDLE;
        GOTO_ERROR_ON_STATUS(status);
    }

    if (Fd == pTask->Fd)
    {
        if (Mask == 0)
        {
            pTask->FdWaitMask = 0;
            pTask->FdSetMask = 0;
            pTask->Fd = -1;
        }
        else
        {
            pTask->FdWaitMask = Mask;
        }
    }
    else if (Mask)
    {
        if (pTask->Fd >= 0)
        {
            /* Only one fd is supported per task at the moment */
            status = STATUS_INSUFFICIENT_RESOURCES;
            GOTO_ERROR_ON_STATUS(status);
        }

        pTask->Fd = Fd;
        pTask->FdWaitMask = Mask;
        pTask->FdSetMask = 0;
    }

error:

    return status;
}

NTSTATUS
LwRtlQueryTaskFd(
    PLW_TASK pTask,
    int Fd,
    PLW_TASK_EVENT_MASK pMask
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (Fd < 0 || Fd != pTask->Fd)
    {
        status = STATUS_INVALID_HANDLE;
        GOTO_ERROR_ON_STATUS(status);
    }

    *pMask = pTask->FdSetMask;

cleanup:

    return status;

error:

    *pMask = 0;

    goto cleanup;
}

VOID
LwRtlWakeTask(
    PLW_TASK pTask
    )
{
    LOCK_THREAD(pTask->pThread);
    pTask->TriggerSet |= LW_TASK_EVENT_EXPLICIT;
    SignalThread(pTask->pThread);
    UNLOCK_THREAD(pTask->pThread);
}

LW_NTSTATUS
LwRtlSetTaskUnixSignal(
    LW_IN PLW_TASK pTask,
    LW_IN int Sig,
    LW_IN LW_BOOLEAN bSubscribe
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (bSubscribe && !pTask->pUnixSignal)
    {
        status = LW_RTL_ALLOCATE_AUTO(&pTask->pUnixSignal);
        GOTO_ERROR_ON_STATUS(status);
    }

    status = RegisterTaskUnixSignal(pTask, Sig, bSubscribe);
    GOTO_ERROR_ON_STATUS(status);

error:

    return status;
}

void
NotifyTaskUnixSignal(
    PLW_TASK pTask,
    siginfo_t* pInfo
    )
{
    LOCK_THREAD(pTask->pThread);

    if (pTask->TriggerSet != TASK_COMPLETE_MASK)
    {
        while (pTask->pUnixSignal->si_signo)
        {
            pthread_cond_wait(&pTask->pThread->Event, &pTask->pThread->Lock);
            if (pTask->TriggerSet == TASK_COMPLETE_MASK)
            {
                goto cleanup;
            }
        }

        *pTask->pUnixSignal = *pInfo;
        pTask->TriggerSet |= LW_TASK_EVENT_UNIX_SIGNAL;
        SignalThread(pTask->pThread);
    }

cleanup:

    UNLOCK_THREAD(pTask->pThread);
}

LW_BOOLEAN
LwRtlNextTaskUnixSignal(
    LW_IN PLW_TASK pTask,
    LW_OUT siginfo_t* pInfo
    )
{
    BOOLEAN bResult = FALSE;

    LOCK_THREAD(pTask->pThread);

    if (pTask->pUnixSignal == NULL || pTask->pUnixSignal->si_signo == 0)
    {
        bResult = FALSE;
    }
    else
    {
        if (pInfo)
        {
            *pInfo = *pTask->pUnixSignal;
        }
        pTask->pUnixSignal->si_signo = 0;
        pthread_cond_broadcast(&pTask->pThread->Event);
        bResult = TRUE;
    }

    UNLOCK_THREAD(pTask->pThread);

    return bResult;
}

VOID
LwRtlCancelTask(
    PLW_TASK pTask
    )
{
    LOCK_THREAD(pTask->pThread);

    pTask->TriggerSet |= LW_TASK_EVENT_EXPLICIT | LW_TASK_EVENT_CANCEL;
    SignalThread(pTask->pThread);

    UNLOCK_THREAD(pTask->pThread);
}

VOID
LwRtlWaitTask(
    PLW_TASK pTask
    )
{
    LOCK_THREAD(pTask->pThread);

    while (pTask->TriggerSet != TASK_COMPLETE_MASK)
    {
        pthread_cond_wait(&pTask->pThread->Event, &pTask->pThread->Lock);
    }

    UNLOCK_THREAD(pTask->pThread);
}

VOID
LwRtlWakeTaskGroup(
    PLW_TASK_GROUP group
    )
{
    PRING ring = NULL;
    PLW_TASK pTask = NULL;

    LOCK_GROUP(group);

    for (ring = group->Tasks.pNext; ring != &group->Tasks; ring = ring->pNext)
    {
        pTask = LW_STRUCT_FROM_FIELD(ring, SELECT_TASK, GroupRing);

        LwRtlWakeTask(pTask);
    }

    UNLOCK_GROUP(group);
}

VOID
LwRtlCancelTaskGroup(
    PLW_TASK_GROUP group
    )
{
    PRING ring = NULL;
    PLW_TASK pTask = NULL;
    ULONG i = 0;

    LOCK_GROUP(group);

    group->bCancelled = TRUE;

    for (i = 0; i < group->pPool->ulEventThreadCount; i++)
    {
        LOCK_THREAD(&group->pPool->pEventThreads[i]);
    }

    for (ring = group->Tasks.pNext; ring != &group->Tasks; ring = ring->pNext)
    {
        pTask = LW_STRUCT_FROM_FIELD(ring, SELECT_TASK, GroupRing);

        pTask->TriggerSet |= LW_TASK_EVENT_EXPLICIT | LW_TASK_EVENT_CANCEL;        
    }

    for (i = 0; i < group->pPool->ulEventThreadCount; i++)
    {
        SignalThread(&group->pPool->pEventThreads[i]);
        UNLOCK_THREAD(&group->pPool->pEventThreads[i]);
    }

    UNLOCK_GROUP(group);
}

VOID
LwRtlWaitTaskGroup(
    PLW_TASK_GROUP group
    )
{
    PRING pRing = NULL;
    PLW_TASK pTask = NULL;
    BOOLEAN bStillAlive = TRUE;

    LOCK_GROUP(group);

    while (bStillAlive)
    {
        bStillAlive = FALSE;

        for (pRing = group->Tasks.pNext; !bStillAlive && pRing != &group->Tasks; pRing = pRing->pNext)
        {
            pTask = LW_STRUCT_FROM_FIELD(pRing, SELECT_TASK, GroupRing);
         
            LOCK_THREAD(pTask->pThread);
            if (pTask->TriggerSet != TASK_COMPLETE_MASK)
            {
                bStillAlive = TRUE;
            }
            UNLOCK_THREAD(pTask->pThread);         
        }

        if (bStillAlive)
        {
            pthread_cond_wait(&group->Event, &group->Lock);
        }
    }

    UNLOCK_GROUP(group);
}

static
NTSTATUS
SelectThreadInit(
    PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    PSELECT_THREAD pThread
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    pthread_attr_t pthreadAttr;
    BOOLEAN bAttrInit = FALSE;

    status = LwErrnoToNtStatus(pthread_attr_init(&pthreadAttr));
    GOTO_ERROR_ON_STATUS(status);

    bAttrInit = TRUE;

    GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(pthread_mutex_init(&pThread->Lock, NULL)));
    GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(pthread_cond_init(&pThread->Event, NULL)));

    if (pipe(pThread->SignalFds) < 0)
    {
        GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(errno));
    }

    SetCloseOnExec(pThread->SignalFds[0]);
    SetCloseOnExec(pThread->SignalFds[1]);

    RingInit(&pThread->Tasks);

    if (pAttrs && pAttrs->ulTaskThreadStackSize)
    {
        GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(
                                 pthread_attr_setstacksize(&pthreadAttr, pAttrs->ulTaskThreadStackSize)));
    }

    GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(
                             pthread_create(
                                 &pThread->Thread,
                                 &pthreadAttr,
                                 EventThread,
                                 pThread)));

error:

    if (bAttrInit)
    {
        pthread_attr_destroy(&pthreadAttr);
    }

    return status;
}

static
VOID
SelectThreadDestroy(
    PSELECT_THREAD pThread
    )
{
    pthread_mutex_destroy(&pThread->Lock);
    pthread_cond_destroy(&pThread->Event);
}

static
NTSTATUS
GetFdLimit(
    PULONG pulLimit
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct rlimit limit = {0};

    if (getrlimit(RLIMIT_NOFILE, &limit) != 0)
    {
        status = LwErrnoToNtStatus(errno);
        GOTO_ERROR_ON_STATUS(status);
    }

    if (limit.rlim_cur == RLIM_INFINITY)
    {
        *pulLimit = (ULONG) 0xFFFFFFFF;
    }
    else
    {
        *pulLimit = (ULONG) limit.rlim_cur;
    }

error:

    return status;
}

NTSTATUS
LwRtlCreateThreadPool(
    PLW_THREAD_POOL* ppPool,
    PLW_THREAD_POOL_ATTRIBUTES pAttrs
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_THREAD_POOL pPool = NULL;
    int i = 0;
    int numCpus = 0;
    ULONG ulFdLimit = 0;

#if defined(_SC_NPROCESSORS_ONLN)
    numCpus = sysconf(_SC_NPROCESSORS_ONLN);

    if (numCpus < 0)
    {
        numCpus = 1;
    }
#else
    numCpus = 1;
#endif
   

    GOTO_ERROR_ON_STATUS(status = LW_RTL_ALLOCATE_AUTO(&pPool));
    
    GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(pthread_mutex_init(&pPool->Lock, NULL)));
    GOTO_ERROR_ON_STATUS(status = LwErrnoToNtStatus(pthread_cond_init(&pPool->Event, NULL)));

    if (GetDelegateAttr(pAttrs))
    {
        status = AcquireDelegatePool(&pPool->pDelegate);
        GOTO_ERROR_ON_STATUS(status);
    }
    else
    {
        pPool->ulEventThreadCount = GetTaskThreadsAttr(pAttrs, numCpus);
        pPool->ulNextEventThread = 0;

        status = GetFdLimit(&ulFdLimit);
        GOTO_ERROR_ON_STATUS(status);

        /*
         * Each thread needs 2 fds for the notification pipe.
         * This check will limit us to 1/4 of the process fd limit.
         * This is particularly important on Solaris which has a very
         * low default limit of 256.
         */
        if (pPool->ulEventThreadCount > ulFdLimit / 8)
        {
            pPool->ulEventThreadCount = ulFdLimit / 8;
        }

        if (pPool->ulEventThreadCount)
        {
            GOTO_ERROR_ON_STATUS(status = LW_RTL_ALLOCATE_ARRAY_AUTO(
                                     &pPool->pEventThreads,
                                     pPool->ulEventThreadCount));
            for (i = 0; i < pPool->ulEventThreadCount; i++)
            {
                GOTO_ERROR_ON_STATUS(status = SelectThreadInit(pAttrs, &pPool->pEventThreads[i]));
            }
        }
    }

    status = InitWorkThreads(&pPool->WorkThreads, pAttrs, numCpus);
    GOTO_ERROR_ON_STATUS(status);

    *ppPool = pPool;

error:

    return status;
}

VOID
LwRtlFreeThreadPool(
    PLW_THREAD_POOL* ppPool
    )
{
    PLW_THREAD_POOL pPool = *ppPool;
    PSELECT_THREAD pThread = NULL;
    int i = 0;

    if (pPool)
    {
        LOCK_POOL(pPool);
        pPool->bShutdown = TRUE;
        pthread_cond_broadcast(&pPool->Event);
        UNLOCK_POOL(pPool);
        
        if (pPool->pEventThreads)
        {
            for (i = 0; i < pPool->ulEventThreadCount; i++)
            {
                pThread = &pPool->pEventThreads[i];
                LOCK_THREAD(pThread);
                pThread->bShutdown = TRUE;
                SignalThread(pThread);
                UNLOCK_THREAD(pThread);
                pthread_join(pThread->Thread, NULL);
                SelectThreadDestroy(pThread);
            }
            
            RtlMemoryFree(pPool->pEventThreads);
        }
        
        if (pPool->pDelegate)
        {
            ReleaseDelegatePool(&pPool->pDelegate);
        }

        pthread_cond_destroy(&pPool->Event);
        pthread_mutex_destroy(&pPool->Lock);
        
        DestroyWorkThreads(&pPool->WorkThreads);

        RtlMemoryFree(pPool);

        *ppPool = NULL;
    }
}

LW_NTSTATUS
LwRtlCreateWorkItem(
    LW_IN PLW_THREAD_POOL pPool,
    LW_OUT PLW_WORK_ITEM* ppWorkItem,
    LW_IN LW_WORK_ITEM_FUNCTION pfnFunc,
    LW_IN PVOID pContext
    )
{
    return CreateWorkItem(&pPool->WorkThreads, ppWorkItem, pfnFunc, pContext);
}

LW_VOID
LwRtlFreeWorkItem(
    LW_IN LW_OUT PLW_WORK_ITEM* ppWorkItem
    )
{
    FreeWorkItem(ppWorkItem);
}

LW_VOID
LwRtlScheduleWorkItem(
    LW_IN PLW_WORK_ITEM pWorkItem,
    LW_IN LW_SCHEDULE_FLAGS Flags
    )
{
    ScheduleWorkItem(NULL, pWorkItem, Flags);
}

LW_VOID
LwRtlWaitWorkItems(
    LW_IN PLW_THREAD_POOL pPool
    )
{
    WaitWorkItems(&pPool->WorkThreads);
}
