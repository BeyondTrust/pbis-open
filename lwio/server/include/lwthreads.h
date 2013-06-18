/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        threads.h
 *
 * Abstract:
 *
 *        Likewise Base
 *                    
 *        Thread Utilities
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LW_BASE_THREADS_H__
#define __LW_BASE_THREADS_H__

#include <lw/ntstatus.h>
#include <lw/attrs.h>
#include <pthread.h>

// Unix time_t is seconds since January 1, 1970.  Negative values
// are times before then.
typedef LONG64 LW_RTL_UNIX_TIME_SECONDS, *PLW_RTL_UNIX_TIME_SECONDS;

// Similar to WDK, where positive time is absolute time in 100ns since
// January 1, 1601 AD.
//
// Note that maximum time is therefore until around the year
// 30,000 AD.
//
// Negative time can vary in meaning.  In some APIs, negative is disallowed
// (or sometimes -1 is special). In others, it is relative time in 100ns
// increments.

typedef LONG64 LW_RTL_WINDOWS_TIME, *PLW_RTL_WINDOWS_TIME;

#define LW_RTL_WINDOWS_TIMESPAN_MICROSECOND ((LW_RTL_WINDOWS_TIME) 10)
#define LW_RTL_WINDOWS_TIMESPAN_MILLISECOND ((LW_RTL_WINDOWS_TIME) 1000 * LW_RTL_WINDOWS_TIMESPAN_MICROSECOND)
#define LW_RTL_WINDOWS_TIMESPAN_SECOND      ((LW_RTL_WINDOWS_TIME) 1000 * LW_RTL_WINDOWS_TIMESPAN_MILLISECOND)
#define LW_RTL_WINDOWS_TIMESPAN_MINUTE      ((LW_RTL_WINDOWS_TIME) 60 * LW_RTL_WINDOWS_TIMESPAN_SECOND)
#define LW_RTL_WINDOWS_TIMESPAN_HOUR        ((LW_RTL_WINDOWS_TIME) 60 * LW_RTL_WINDOWS_TIMESPAN_MINUTE)
#define LW_RTL_WINDOWS_TIMESPAN_DAY         ((LW_RTL_WINDOWS_TIME) 24 * LW_RTL_WINDOWS_TIMESPAN_HOUR)

NTSTATUS
LwRtlGetCurrentWindowsTime(
    OUT PLW_RTL_WINDOWS_TIME WindowsTime
    );

// Opaque, except that zero-memory init means uninitialized...
// and cleanup is always safe on uninitialized.

typedef LW_ULONG _LW_RTL_EVENT_FLAGS;
typedef struct _LW_RTL_EVENT {
    struct {
        _LW_RTL_EVENT_FLAGS Flags;
        pthread_cond_t Condition;
        pthread_mutex_t Mutex;
    } Private;
} LW_RTL_EVENT, *PLW_RTL_EVENT;
#define LW_RTL_EVENT_ZERO_INITIALIZER { { 0 } }

// TODO: Add IsManualReset
LW_NTSTATUS
LwRtlInitializeEvent(
    OUT PLW_RTL_EVENT pEvent
    );

LW_VOID
LwRtlCleanupEvent(
    IN OUT PLW_RTL_EVENT pEvent
    );

LW_BOOLEAN
LwRtlWaitEvent(
    IN PLW_RTL_EVENT pEvent,
    IN OPTIONAL PLW_RTL_WINDOWS_TIME Timeout
    );

LW_VOID
LwRtlSetEvent(
    IN PLW_RTL_EVENT pEvent
    );

typedef LW_ULONG _LW_RTL_MUTEX_FLAGS;
typedef struct _LW_RTL_MUTEX {
    struct {
        _LW_RTL_MUTEX_FLAGS Flags;
        pthread_mutex_t Mutex;
    } Private;
} LW_RTL_MUTEX, *PLW_RTL_MUTEX;
#define LW_RTL_MUTEX_ZERO_INITIALIZER { { 0 } }

LW_NTSTATUS
LwRtlInitializeMutex(
    OUT PLW_RTL_MUTEX pMutex,
    IN BOOLEAN IsRecursive
    );

LW_VOID
LwRtlCleanupMutex(
    IN OUT PLW_RTL_MUTEX pMutex
    );

LW_BOOLEAN
LwRtlTryLockMutex(
    IN PLW_RTL_MUTEX pMutex
    );

LW_VOID
LwRtlLockMutex(
    IN PLW_RTL_MUTEX pMutex
    );

LW_VOID
LwRtlUnlockMutex(
    IN PLW_RTL_MUTEX pMutex
    );

typedef LW_ULONG _LW_RTL_CONDITION_VARIABLE_FLAGS;
typedef struct _LW_RTL_CONDITION_VARIABLE {
    struct {
        _LW_RTL_CONDITION_VARIABLE_FLAGS Flags;
        pthread_cond_t Condition;
    } Private;
} LW_RTL_CONDITION_VARIABLE, *PLW_RTL_CONDITION_VARIABLE;
#define LW_RTL_CONDITION_VARIABLE_ZERO_INITIALIZER { { 0 } }

LW_NTSTATUS
LwRtlInitializeConditionVariable(
    OUT PLW_RTL_CONDITION_VARIABLE pConditionVariable
    );

LW_VOID
LwRtlCleanupConditionVariable(
    IN OUT PLW_RTL_CONDITION_VARIABLE pConditionVariable
    );

LW_BOOLEAN
LwRtlWaitConditionVariable(
    IN PLW_RTL_CONDITION_VARIABLE pConditionVariable,
    IN PLW_RTL_MUTEX pMutex,
    IN OPTIONAL PLW_RTL_WINDOWS_TIME Timeout
    );

LW_VOID
LwRtlSignalConditionVariable(
    IN PLW_RTL_CONDITION_VARIABLE pConditionVariable
    );

LW_VOID
LwRtlBroadcastConditionVariable(
    IN PLW_RTL_CONDITION_VARIABLE pConditionVariable
    );

struct _LW_RTL_THREAD;
typedef struct _LW_RTL_THREAD *PLW_RTL_THREAD;

typedef LW_PVOID (*LW_THREAD_PROC)(LW_PVOID ThreadContext);

LW_NTSTATUS
LwRtlCreateThread(
    OUT PLW_RTL_THREAD* ppThread,
    IN LW_THREAD_PROC ThreadRoutine,
    IN LW_PVOID ThreadContext
    );

LW_PVOID
LwRtlJoinThread(
    IN OUT PLW_RTL_THREAD pThread
    );

LW_VOID
LwRtlDetachThread(    
    IN OUT PLW_RTL_THREAD pThread
    );

#if 0
typedef struct _LW_RTL_THREAD {
    BOOLEAN IsInitialized;
    pthread_t Internal;
} LW_RTL_THREAD, *PLW_RTL_THREAD;

typedef struct _LW_RTL_MUTEX {
    BOOLEAN IsInitialized;
    pthread_mutex_t Internal;
} LW_RTL_MUTEX, *PLW_RTL_MUTEX;

typedef struct _LW_RTL_RWLOCK {
    BOOLEAN IsInitialized;
    pthread_rwlock_t Internal;
} LW_RTL_RWLOCK, *PLW_RTL_RWLOCK;

// Similar to WDK, negative is relative time and positive
// is abosolute time in 100ns since January 1, 1601.
typedef LONG64 LW_RTL_WINDOWS_TIME, *PW_RTL_WINDOWS_TIME;

#define LW_RTL_WINDOWS_TIMESPAN_MICROSECOND 10
#define LW_RTL_WINDOWS_TIMESPAN_MILLISECOND (1000 * LW_RTL_WINDOWS_TIMESPAN_MICROSECOND)
#define LW_RTL_WINDOWS_TIMESPAN_SECOND      (1000 * LW_RTL_WINDOWS_TIMESPAN_MILLISECOND)
#define LW_RTL_WINDOWS_TIMESPAN_MINUTE      (60 * LW_RTL_WINDOWS_TIMESPAN_SECOND)
#define LW_RTL_WINDOWS_TIMESPAN_HOUR        (60 * LW_RTL_WINDOWS_TIMESPAN_MINUTE)
#define LW_RTL_WINDOWS_TIMESPAN_DAY         (24 * LW_RTL_WINDOWS_TIMESPAN_HOUR)

BOOLEAN
LwRtlWaitTimeoutEvent(
    IN PLW_RTL_EVENT pEvent,
    IN PLW_RTL_MUTEX pMutex,
    IN LW_RTL_WINDOWS_TIMESPAN Timeout
    );

BOOLEAN
LwRtlWaitAbsoluteTimeEvent(
    IN PLW_RTL_EVENT pEvent,
    IN PLW_RTL_MUTEX pMutex,
    IN LW_RTL_WINDOWS_TIME Time
    );

VOID
LwRtlSignalEvent(
    IN PLW_RTL_EVENT pEvent
    );

VOID
LwRtlBroadcastEvent(
    IN PLW_RTL_EVENT pEvent
    );
#endif

/////////////////////////////////

#if 0

NTSTATUS
LwRtlThreadsCreateThread(
    OUT pthread_t** ppThread,
    IN PVOID (*ThreadRoutine)(PVOID),
    IN PVOID ThreadContext
    );

NTSTATUS
LwRtlThreadsCreateDetachedThread(
    IN PVOID (*ThreadRoutine)(PVOID),
    IN PVOID ThreadContext
    );

typedef UCHAR LW_RTL_THREADS_MUTEX_TYPE;

#define LW_RTL_THREADS_MUTEX_TYPE_RECURSIVE 1

NTSTATUS
LwRtlThreadsCreateMutex(
    OUT pthread_mutex_t** ppMutex,
    IN LW_RTL_THREADS_MUTEX_TYPE MutexType
    );

VOID
LwRtlThreadsDestroyMutex(
    IN OUT pthread_mutex_t** ppMutex
    );

NTSTATUS
LwRtlThreadsInitializeMutex(
    OUT pthread_mutex_t* pMutex,
    IN LW_RTL_THREADS_MUTEX_TYPE MutexType
    );

VOID
LwRtlThreadsCleanupMutex(
    IN OUT pthread_mutex_t* pMutex
    );

VOID
LwThreadsAcquireMutex(
    IN pthread_mutex_t* pMutex
    );

VOID
LwThreadsReleaseMutex(
    IN pthread_mutex_t* pMutex
    );

NTSTATUS
LwRtlThreadsCreateCond(
    OUT pthread_cond_t** ppCond
    );

VOID
LwRtlThreadsDestroyCond(
    IN OUT pthread_cond_t** ppCond
    );
#endif

#endif /* __LW_BASE_THREADS_H__ */
