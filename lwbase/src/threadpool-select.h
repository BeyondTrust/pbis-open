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
 *        threadpool-internal.h
 *
 * Abstract:
 *
 *        Thread pool API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWBASE_THREADPOOL_INTERNAL_H__
#define __LWBASE_THREADPOOL_INTERNAL_H__

#include <lw/threadpool.h>
#include <lw/rtlgoto.h>
#include <pthread.h>

#include "threadpool-common.h"

typedef struct _RING
{
    struct _RING* pPrev;
    struct _RING* pNext;
} RING, *PRING;

typedef struct _SELECT_THREAD
{
    pthread_t Thread;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    int SignalFds[2];
    RING Tasks;
    BOOLEAN volatile bSignalled;
    BOOLEAN volatile bShutdown;
} SELECT_THREAD, *PSELECT_THREAD;

typedef struct _WORK_ITEM_THREAD
{
    PLW_THREAD_POOL pPool;
    pthread_t Thread;
} WORK_ITEM_THREAD, *PWORK_ITEM_THREAD;

typedef struct _LW_TASK
{
    /* Owning thread pool */
    PLW_THREAD_POOL pPool;
    /* Owning thread */
    PSELECT_THREAD pThread;
    /* Owning group */
    PLW_TASK_GROUP pGroup;
    /* Ref count */
    ULONG volatile ulRefCount;
    /* Trigger conditions task is waiting for */
    LW_TASK_EVENT_MASK TriggerWait;
    /* Trigger conditions that have been satisfied */
    LW_TASK_EVENT_MASK volatile TriggerSet;
    /* Trigger conditions that will be passed to func() */
    LW_TASK_EVENT_MASK TriggerArgs;
    /* Absolute time of next time wake event */
    LONG64 llDeadline;
    /* Callback function and context */
    LW_TASK_FUNCTION pfnFunc;
    PVOID pFuncContext;
    /* File descriptor for fd-based events */
    int Fd;
    /* Wait mask for fd */
    LW_TASK_EVENT_MASK FdWaitMask;
    /* Set mask for fd */
    LW_TASK_EVENT_MASK FdSetMask;
    /* Link to siblings in task group */
    RING GroupRing;
    /* Link to siblings in event loop */
    RING EventRing;
} SELECT_TASK, *PSELECT_TASK;

typedef struct _WORK_ITEM
{
    LW_WORK_ITEM_FUNCTION pfnFunc;
    PVOID pContext;
    RING Ring;
} WORK_ITEM, *PWORK_ITEM;

typedef struct _LW_TASK_GROUP
{
    PLW_THREAD_POOL pPool;
    RING Tasks;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
} SELECT_TASK_GROUP, *PSELECT_TASK_GROUP;

typedef struct _LW_THREAD_POOL
{
    PLW_THREAD_POOL pDelegate;
    PSELECT_THREAD pEventThreads;
    ULONG ulEventThreadCount;
    ULONG ulNextEventThread;
    PWORK_ITEM_THREAD pWorkThreads;
    ULONG ulWorkThreadCount;
    RING WorkItems;
    BOOLEAN volatile bShutdown;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
} SELECT_POOL, *PSELECT_POOL;

typedef struct _CLOCK
{
    LONG64 llLastTime;
    LONG64 llAdjust;
} CLOCK, *PCLOCK;

/*
 * Lock order discipline:
 *
 * Always lock manager before locking a thread
 * Always lock a group before locking a thread
 */

#define LOCK_THREAD(st) (pthread_mutex_lock(&(st)->Lock))
#define UNLOCK_THREAD(st) (pthread_mutex_unlock(&(st)->Lock))
#define LOCK_GROUP(g) (pthread_mutex_lock(&(g)->Lock))
#define UNLOCK_GROUP(g) (pthread_mutex_unlock(&(g)->Lock))
#define LOCK_POOL(m) (pthread_mutex_lock(&(m)->Lock))
#define UNLOCK_POOL(m) (pthread_mutex_unlock(&(m)->Lock))

/* Ring functions */

static inline
VOID
RingInit(
    PRING pRing
    )
{
    pRing->pPrev = pRing->pNext = pRing;
}

static inline
VOID
RingSanity(
    PRING pRing
    )
{
    assert(pRing->pPrev->pNext == pRing && pRing->pNext->pPrev == pRing);
}

static inline
VOID
RingInsertAfter(
    PRING pAnchor,
    PRING pElement
    )
{
    RingSanity(pAnchor);
    RingSanity(pElement);
    assert(pElement->pPrev == pElement->pNext && pElement->pPrev == pElement);

    pElement->pNext = pAnchor->pNext;
    pElement->pPrev = pAnchor;
    
    pAnchor->pNext->pPrev = pElement;
    pAnchor->pNext = pElement;
}

static inline
VOID
RingInsertBefore(
    PRING pAnchor,
    PRING pElement
    )
{
    RingSanity(pAnchor);
    RingSanity(pElement);
    assert(pElement->pPrev == pElement->pNext && pElement->pPrev == pElement);

    pElement->pNext = pAnchor;
    pElement->pPrev = pAnchor->pPrev;

    pAnchor->pPrev->pNext = pElement;
    pAnchor->pPrev = pElement;
}

static inline
VOID
RingRemove(
    PRING pElement
    )
{
    RingSanity(pElement);
    pElement->pPrev->pNext = pElement->pNext;
    pElement->pNext->pPrev = pElement->pPrev;
    RingInit(pElement);
}

static inline
VOID
RingEnqueue(
    PRING pAnchor,
    PRING pElement
    )
{
    RingInsertBefore(pAnchor, pElement);
}

static inline
VOID
RingDequeue(
    PRING pAnchor,
    PRING* pElement
    )
{
    *pElement = pAnchor->pNext;
    RingRemove(*pElement);
}

static inline
VOID
RingMove(
    PRING pFrom,
    PRING pTo
    )
{
    PRING pFromFirst = pFrom->pNext;
    PRING pFromLast = pFrom->pPrev;
    PRING pToLast = pTo->pPrev;

    RingSanity(pFrom);
    RingSanity(pTo);

    if (pFrom->pNext != pFrom)
    {
        /* Link pFrom pToLast and pFromFirst */
        pToLast->pNext = pFromFirst;
        pFromFirst->pPrev = pToLast;
        
        /* Link pFromLast inpTo pTo */
        pFromLast->pNext = pTo;
        pTo->pPrev = pFromLast;
        
        pFrom->pNext = pFrom->pPrev = pFrom;
    }
}

static inline
size_t
RingCount(
    PRING ring
    )
{
    PRING iter = NULL;
    size_t count = 0;

    RingSanity(ring);

    for (iter = ring->pNext; iter != ring; iter = iter->pNext, count++);

    return count;
}

static inline
BOOLEAN
RingIsEmpty(
    PRING ring
    )
{
    RingSanity(ring);

    return ring->pNext == ring;
}

/* Time functions */

static inline
NTSTATUS
TimeNow(
    PLONG64 pllNow
    )
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL))
    {
        return LwErrnoToNtStatus(errno);
    }
    else
    {
        *pllNow = 
            tv.tv_sec * 1000000000ll +
            tv.tv_usec * 1000ll;

        return STATUS_SUCCESS;
    }
}

static inline
NTSTATUS
ClockUpdate(
    PCLOCK pClock
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LONG64 llNow = 0;

    status = TimeNow(&llNow);
    GOTO_ERROR_ON_STATUS(status);

    if (pClock->llLastTime == 0)
    {
        pClock->llAdjust = -llNow;
    }
    else if (llNow < pClock->llLastTime)
    {
        pClock->llAdjust += (pClock->llLastTime - llNow + 1);
    }

    pClock->llLastTime = llNow;
    
error:

    return status;
}

static inline
NTSTATUS
ClockGetMonotonicTime(
    PCLOCK pClock,
    PLONG64 pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = ClockUpdate(pClock);
    GOTO_ERROR_ON_STATUS(status);

    *pllTime = pClock->llLastTime + pClock->llAdjust;

error:
    
    return status; 
}

#endif
