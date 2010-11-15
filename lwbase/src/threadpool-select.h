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
    /* Latest signal event */
    siginfo_t* pUnixSignal;
    /* Wait mask for fd */
    LW_TASK_EVENT_MASK FdWaitMask;
    /* Set mask for fd */
    LW_TASK_EVENT_MASK FdSetMask;
    /* Link to siblings in task group */
    RING GroupRing;
    /* Link to siblings in event loop */
    RING EventRing;
} SELECT_TASK, *PSELECT_TASK;

typedef struct _LW_TASK_GROUP
{
    PLW_THREAD_POOL pPool;
    RING Tasks;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    unsigned bCancelled:1;
    unsigned bLockInit:1;
    unsigned bEventInit:1;
} SELECT_TASK_GROUP, *PSELECT_TASK_GROUP;

typedef struct _LW_THREAD_POOL
{
    PLW_THREAD_POOL pDelegate;
    PSELECT_THREAD pEventThreads;
    ULONG ulEventThreadCount;
    ULONG ulNextEventThread;
    BOOLEAN volatile bShutdown;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    LW_WORK_THREADS WorkThreads;
} SELECT_POOL, *PSELECT_POOL;

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

#endif
