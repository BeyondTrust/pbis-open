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
 *        threadpool-common.h
 *
 * Abstract:
 *
 *        Thread pool API common elements
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWBASE_THREADPOOL_COMMON_H__
#define __LWBASE_THREADPOOL_COMMON_H__

#include <pthread.h>

typedef struct _RING
{
    struct _RING* pPrev;
    struct _RING* pNext;
} RING, *PRING;

typedef struct _CLOCK
{
    LONG64 llLastTime;
    LONG64 llAdjust;
} CLOCK, *PCLOCK;

struct _LW_THREAD_POOL_ATTRIBUTES
{
    BOOLEAN bDelegateTasks;
    LONG lTaskThreads;
    LONG lWorkThreads;
    ULONG ulTaskThreadStackSize;
    ULONG ulWorkThreadStackSize;
    ULONG ulWorkThreadTimeout;
};

typedef struct _LW_WORK_ITEM
{
    LW_WORK_ITEM_FUNCTION pfnFunc;
    PVOID pContext;
    RING Ring;
} LW_WORK_ITEM, *PLW_WORK_ITEM;

typedef struct _LW_WORK_THREAD
{
    struct _LW_WORK_THREADS* pThreads;
    pthread_t Thread;
    unsigned volatile bStarted:1;
} LW_WORK_THREAD, *PLW_WORK_THREAD;

typedef struct _LW_WORK_THREADS
{
    PLW_WORK_THREAD pWorkThreads;
    ULONG ulWorkThreadCount;
    ULONG ulWorkThreadStackSize;
    ULONG ulWorkThreadTimeout;
    /* Number of started threads */
    ULONG volatile ulStarted;
    /* Number of queued items */
    ULONG volatile ulQueued;
    /* Number of threads available to process an item */
    ULONG volatile ulAvailable;
    RING WorkItems;
    BOOLEAN volatile bShutdown;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    unsigned bDestroyLock:1;
    unsigned bDestroyEvent:1;
} LW_WORK_THREADS, *PLW_WORK_THREADS;

#define LOCK_THREADS(m) (pthread_mutex_lock(&(m)->Lock))
#define UNLOCK_THREADS(m) (pthread_mutex_unlock(&(m)->Lock))

#define INVALID_THREAD_HANDLE ((pthread_t) (size_t) - 1)

#define DEFAULT_WORK_THREAD_TIMEOUT 30

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
RingEnqueueFront(
    PRING pAnchor,
    PRING pElement
    )
{
    RingInsertAfter(pAnchor, pElement);
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

NTSTATUS
AcquireDelegatePool(
    PLW_THREAD_POOL* ppPool
    );

VOID
ReleaseDelegatePool(
    PLW_THREAD_POOL* ppPool
    );

static
inline
BOOLEAN
GetDelegateAttr(
    PLW_THREAD_POOL_ATTRIBUTES pAttrs
    )
{
    return pAttrs ? pAttrs->bDelegateTasks : TRUE;
}

static
inline
ULONG
GetTaskThreadsAttr(
    PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    int numCpus
    )
{
    LONG lCount = pAttrs ? pAttrs->lTaskThreads : -1;

    return lCount < 0 ? -lCount * numCpus : lCount;
}

static
inline
ULONG
GetWorkThreadsAttr(
    PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    int numCpus
    )
{
    LONG lCount = pAttrs ? pAttrs->lWorkThreads : -4;

    return lCount < 0 ? -lCount * numCpus : lCount;
}

static
inline
ULONG
GetWorkThreadTimeoutAttr(
    PLW_THREAD_POOL_ATTRIBUTES pAttrs
    )
{
    return pAttrs ? pAttrs->ulWorkThreadTimeout : DEFAULT_WORK_THREAD_TIMEOUT;
}

VOID
SetCloseOnExec(
    int Fd
    );

NTSTATUS
InitWorkThreads(
    PLW_WORK_THREADS pThreads,
    PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    int numCpus
    );

VOID
DestroyWorkThreads(
    PLW_WORK_THREADS pThreads
    );

NTSTATUS
QueueWorkItem(
    PLW_WORK_THREADS pThreads,
    LW_WORK_ITEM_FUNCTION pfnFunc,
    PVOID pContext,
    LW_WORK_ITEM_FLAGS Flags
    );

int
GetCpuCount(
    VOID
    );

NTSTATUS
SetThreadAttrAffinity(
    pthread_attr_t* pAttr,
    int cpuNum
    );
#endif
