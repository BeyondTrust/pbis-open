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

#ifndef __LWBASE_THREADPOOL_EPOLL_H__
#define __LWBASE_THREADPOOL_EPOLL_H__

#include <lw/threadpool.h>
#include <lw/rtlgoto.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sched.h>

#include "threadpool-common.h"

#define TASK_COMPLETE_MASK 0xFFFFFFFF

typedef struct _RING
{
    struct _RING* pPrev;
    struct _RING* pNext;
} RING, *PRING;

typedef struct _EPOLL_THREAD
{
    PLW_THREAD_POOL pPool;
    pthread_t Thread;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    int SignalFds[2];
    int EpollFd;
    RING Tasks;
    /* Thread load (protected by thread pool lock) */
    ULONG volatile ulLoad;
    BOOLEAN volatile bSignalled;
    BOOLEAN volatile bShutdown;
} EPOLL_THREAD, *PEPOLL_THREAD;

typedef struct _WORK_ITEM_THREAD
{
    PLW_THREAD_POOL pPool;
    pthread_t Thread;
} WORK_ITEM_THREAD, *PWORK_ITEM_THREAD;

typedef struct _LW_TASK
{
    /* Owning thread */
    PEPOLL_THREAD pThread;
    /* Owning group */
    PLW_TASK_GROUP pGroup;
    /* Ref count (protected by thread lock) */
    ULONG volatile ulRefCount;
    /* Events task is waiting for (owned by thread) */
    LW_TASK_EVENT_MASK EventWait;
    /* Last set of events task waited for (owned by thread) */
    LW_TASK_EVENT_MASK EventLastWait;
    /* Events that will be passed on the next wakeup (owned by thread) */
    LW_TASK_EVENT_MASK EventArgs;
    /* Event conditions signalled between owning thread and
       external callers (protected by thread lock) */
    LW_TASK_EVENT_MASK volatile EventSignal;
    /* Absolute time of next time wake event (owned by thread) */
    LONG64 llDeadline;
    /* Callback function and context (immutable) */
    LW_TASK_FUNCTION pfnFunc;
    PVOID pFuncContext;
    /* File descriptor for fd-based events (owned by thread) */
    int Fd;
    /* Link to siblings in task group (protected by group lock) */
    RING GroupRing;
    /* Link to siblings in scheduler queue (owned by thread) */
    RING QueueRing;
    /* Link to siblings in signal queue (protected by thread lock) */
    RING SignalRing;
} EPOLL_TASK, *PEPOLL_TASK;

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
} EPOLL_TASK_GROUP, *PEPOLL_TASK_GROUP;

typedef struct _LW_THREAD_POOL
{
    struct _LW_THREAD_POOL* pDelegate;
    PEPOLL_THREAD pEventThreads;
    ULONG ulEventThreadCount;
    PWORK_ITEM_THREAD pWorkThreads;
    ULONG ulWorkThreadCount;
    RING WorkItems;
    BOOLEAN volatile bShutdown;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
} EPOLL_POOL, *PEPOLL_POOL;

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
 * Always lock threads at a lower index first
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
RingInsertAfter(
    PRING pAnchor,
    PRING pElement
    )
{
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

    if (pFrom->pNext != pFrom)
    {
        pToLast->pNext = pFromFirst;
        pFromFirst->pPrev = pToLast;
        
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

    for (iter = ring->pNext; iter != ring; iter = iter->pNext, count++);

    return count;
}

static inline
BOOLEAN
RingIsEmpty(
    PRING ring
    )
{
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
    else if (llNow <= pClock->llLastTime)
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
