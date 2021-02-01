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
    /* Pending UNIX signal (protected by thread lock) */
    siginfo_t* pUnixSignal;
    /* Link to siblings in task group (protected by group lock) */
    RING GroupRing;
    /* Link to siblings in scheduler queue (owned by thread) */
    RING QueueRing;
    /* Link to siblings in signal queue (protected by thread lock) */
    RING SignalRing;
} EPOLL_TASK, *PEPOLL_TASK;

typedef struct _LW_TASK_GROUP
{
    PLW_THREAD_POOL pPool;
    RING Tasks;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    unsigned bCancelled:1;
    unsigned bLockInit:1;
    unsigned bEventInit:1;
} EPOLL_TASK_GROUP, *PEPOLL_TASK_GROUP;

typedef struct _LW_THREAD_POOL
{
    struct _LW_THREAD_POOL* pDelegate;
    PEPOLL_THREAD pEventThreads;
    ULONG ulEventThreadCount;
    BOOLEAN volatile bShutdown;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    LW_WORK_THREADS WorkThreads;
} EPOLL_POOL, *PEPOLL_POOL;

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

#endif
