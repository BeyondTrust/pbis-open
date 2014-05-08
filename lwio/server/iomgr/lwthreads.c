/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        threads.c
 *
 * Abstract:
 *
 *        Likewise Base
 *                    
 *        Thread Utilities
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "config.h"
#include "lwthreads.h"
#include <lw/rtlgoto.h>
#include <lw/rtlmemory.h>
#include <lw/errno.h>
#include <lwioutils.h>
#include <sys/time.h>
#include <time.h>

#define _PTHREAD_FUNCTION_CANNOT_FAIL(Function, ...) \
    do { \
        int error = Function(__VA_ARGS__); \
        LWIO_ASSERT_FORMAT(!error, #Function "() failed (error = %d)", error); \
    } while (0)

#define _PTHREAD_FUNCTION_CAN_FAIL(pIsOk, FailValue, Function, ...) \
    do { \
        int error = Function(__VA_ARGS__); \
        LWIO_ASSERT_FORMAT(!error || ((FailValue) == error), #Function "() failed (error = %d)", error); \
        *(pIsOk) = error ? FALSE : TRUE; \
    } while (0)

#define PTHREAD_MUTEXATTTR_DESTROY(pMutexAttr) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_mutexattr_destroy, pMutexAttr)

#define PTHREAD_MUTEX_TRYLOCK(pIsOk, pMutex) \
    _PTHREAD_FUNCTION_CAN_FAIL(pIsOk, EBUSY, pthread_mutex_trylock, pMutex)
    
#define PTHREAD_MUTEX_LOCK(pMutex) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_mutex_lock, pMutex)

#define PTHREAD_MUTEX_UNLOCK(pMutex) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_mutex_unlock, pMutex)

#define PTHREAD_MUTEX_DESTROY(pMutex) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_mutex_destroy, pMutex)

#define PTHREAD_COND_TIMEDWAIT(pIsOk, pCondition, pMutex, pAbsoluteTimespec) \
    _PTHREAD_FUNCTION_CAN_FAIL(pIsOk, ETIMEDOUT, pthread_cond_timedwait, pCondition, pMutex, pAbsoluteTimespec)

#define PTHREAD_COND_WAIT(pCondition, pMutex) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_cond_wait, pCondition, pMutex)

#define PTHREAD_COND_SIGNAL(pCondition) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_cond_signal, pCondition)

#define PTHREAD_COND_BROADCAST(pCondition) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_cond_broadcast, pCondition)

#define PTHREAD_COND_DESTROY(pCondition) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_cond_destroy, pCondition)

#define PTHREAD_JOIN(pThread, pResult) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_join, pThread, pResult)

#define PTHREAD_DETACH(pThread) \
    _PTHREAD_FUNCTION_CANNOT_FAIL(pthread_detach, pThread)

#define _LW_RTL_UNIX_TO_WINDOWS_EPOCH_OFFSET_SECONDS 11644473600LL

static
NTSTATUS
LwRtlpUnixTimeFromWindowsTime(
    OUT PLW_RTL_UNIX_TIME_SECONDS UnixTimeSeconds,
    OUT OPTIONAL PLONG UnixTimeMicroseconds,
    OUT OPTIONAL PLONG UnixTimeNanoseconds,
    IN LW_RTL_WINDOWS_TIME WindowsTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LW_RTL_UNIX_TIME_SECONDS unixTimeSeconds =  0;
    LONG microseconds = 0;
    LONG nanoseconds = 0;
    LONG64 windowsTimeSeconds = 0;

    if (UnixTimeMicroseconds && UnixTimeNanoseconds)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (WindowsTime < 0)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    windowsTimeSeconds = WindowsTime / LW_RTL_WINDOWS_TIMESPAN_SECOND;

    unixTimeSeconds = windowsTimeSeconds - _LW_RTL_UNIX_TO_WINDOWS_EPOCH_OFFSET_SECONDS;
    LWIO_ASSERT(unixTimeSeconds <= MAXLONG);
    LWIO_ASSERT(unixTimeSeconds >= MINLONG);

    if (UnixTimeMicroseconds)
    {
        microseconds = WindowsTime / LW_RTL_WINDOWS_TIMESPAN_MICROSECOND - (windowsTimeSeconds * 1000 * 1000);
        LWIO_ASSERT(microseconds < (1000 * 1000));
    }

    if (UnixTimeNanoseconds)
    {
        nanoseconds = (WindowsTime - (windowsTimeSeconds * LW_RTL_WINDOWS_TIMESPAN_SECOND)) * 100;
        LWIO_ASSERT(nanoseconds < (1000 * 1000 * 1000));
    }

cleanup:
    if (status)
    {
        unixTimeSeconds =  0;
        microseconds = 0;
        nanoseconds = 0;
    }

    *UnixTimeSeconds = unixTimeSeconds;
    if (UnixTimeMicroseconds)
    {
        *UnixTimeMicroseconds = microseconds;
    }
    if (UnixTimeNanoseconds)
    {
        *UnixTimeNanoseconds = nanoseconds;
    }

    return status;
}

static
NTSTATUS
LwRtlpWindowsTimeFromUnixTime(
    OUT PLW_RTL_WINDOWS_TIME WindowsTime,
    IN LW_RTL_UNIX_TIME_SECONDS UnixTimeSeconds,
    IN OPTIONAL PLONG UnixTimeMicroseconds,
    IN OPTIONAL PLONG UnixTimeNanoseconds
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LW_RTL_WINDOWS_TIME windowsTime = 0;

    if (UnixTimeMicroseconds && UnixTimeNanoseconds)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if ((UnixTimeSeconds < 0) &&
        ((- _LW_RTL_UNIX_TO_WINDOWS_EPOCH_OFFSET_SECONDS) > UnixTimeSeconds))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    windowsTime = (UnixTimeSeconds + _LW_RTL_UNIX_TO_WINDOWS_EPOCH_OFFSET_SECONDS) * LW_RTL_WINDOWS_TIMESPAN_SECOND;

    if (UnixTimeMicroseconds)
    {
        LONG microseconds = *UnixTimeMicroseconds;
        if (microseconds < 0)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }
        windowsTime += microseconds * LW_RTL_WINDOWS_TIMESPAN_MICROSECOND;
    }

    if (UnixTimeNanoseconds)
    {
        LONG nanoseconds = *UnixTimeNanoseconds;
        if (nanoseconds < 0)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }
        windowsTime += nanoseconds / 100;
    }

    if (windowsTime < UnixTimeSeconds)
    {
        status = STATUS_INTEGER_OVERFLOW;
        GOTO_CLEANUP();
    }

cleanup:
    if (status)
    {
        windowsTime = 0;
    }

    *WindowsTime = windowsTime;

    return status;
}

NTSTATUS
LwRtlpGetCurrentUnixTime(
    OUT PLW_RTL_UNIX_TIME_SECONDS UnixTimeSeconds,
    OUT OPTIONAL PLONG UnixTimeMicroseconds,
    OUT OPTIONAL PLONG UnixTimeNanoseconds
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LW_RTL_UNIX_TIME_SECONDS seconds = 0;
    LONG microseconds = 0;
    LONG nanoseconds = 0;

    if (UnixTimeMicroseconds && UnixTimeNanoseconds)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!UnixTimeMicroseconds && !UnixTimeNanoseconds)
    {
        time_t now = time(NULL);
        if (-1 == now)
        {
            // POSIX does not indicate that errno is used.
            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP();
        }

        seconds = now;
    }
    else
    {
        struct timeval now = { 0 };

        if (gettimeofday(&now, NULL) < 0)
        {
            int error = errno;
            status = LwErrnoToNtStatus(error);
            LWIO_ASSERT(status);
            GOTO_CLEANUP_ON_STATUS(status);
        }

        seconds = now.tv_sec;
        microseconds = now.tv_usec;
        nanoseconds = microseconds * 1000;
    }

cleanup:
    *UnixTimeSeconds = seconds;
    if (UnixTimeMicroseconds)
    {
        *UnixTimeMicroseconds = microseconds;
    }
    if (UnixTimeNanoseconds)
    {
        *UnixTimeNanoseconds = nanoseconds;
    }
    return status;
}

NTSTATUS
LwRtlGetCurrentWindowsTime(
    OUT PLW_RTL_WINDOWS_TIME WindowsTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LW_RTL_WINDOWS_TIME windowsTime = 0;
    LW_RTL_UNIX_TIME_SECONDS unixSeconds = 0;
    LONG unixNanoseconds = 0;

    status = LwRtlpGetCurrentUnixTime(&unixSeconds, NULL, &unixNanoseconds);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlpWindowsTimeFromUnixTime(&windowsTime, unixSeconds, NULL, &unixNanoseconds);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    *WindowsTime = windowsTime;
    return status;
}

//
// time_t is a signed integral type at least 32-bits wide.
//
// struct timespec has:
//
// - time_t tv_sec (seconds)
// - long tv_nsec (nanoseconds)
//
// struct timeval has:
//
// - time_t tv_sec (seconds)
// - suseconds_t tv_usec (microseconds)
//
// suseconds_t is signed integral ranging at least [-1, 1,000,000]
//

static
NTSTATUS
LwRtlpUnixTimespecFromWindowsTime(
    OUT struct timespec* UnixTimespec,
    IN LW_RTL_WINDOWS_TIME WindowsTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct timespec result = { 0 };
    LW_RTL_UNIX_TIME_SECONDS unixTimeSeconds = 0;
    LONG unixTimeNanoseconds = 0;
    LW_RTL_WINDOWS_TIME currentTime = 0;
    LW_RTL_WINDOWS_TIME useTime = 0;

    if (WindowsTime < 0)
    {
        status = LwRtlGetCurrentWindowsTime(&currentTime);
        GOTO_CLEANUP_ON_STATUS(status);

        useTime = currentTime + - WindowsTime;
        if (useTime < currentTime)
        {
            status = STATUS_INTEGER_OVERFLOW;
            GOTO_CLEANUP();
        }
    }
    else
    {
        useTime = WindowsTime;
    }

    status = LwRtlpUnixTimeFromWindowsTime(
                    &unixTimeSeconds,
                    NULL,
                    &unixTimeNanoseconds,
                    useTime);
    GOTO_CLEANUP_ON_STATUS(status);

    result.tv_sec = unixTimeSeconds;
    result.tv_nsec = unixTimeNanoseconds;

cleanup:
    *UnixTimespec = result;

    return status;
}

// Internal flags
#define _LW_RTL_EVENT_FLAG_MUTEX_INITIALIZED    0x1
#define _LW_RTL_EVENT_FLAG_COND_INITIALIZED     0x2
#define _LW_RTL_EVENT_FLAG_SET                  0x4

static
BOOLEAN
LwRtlpIsInitializedEvent(
    IN PLW_RTL_EVENT pEvent
    )
{
    return (IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_MUTEX_INITIALIZED) &&
            IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_COND_INITIALIZED));
}

// TODO: Add IsManualReset
NTSTATUS
LwRtlInitializeEvent(
    OUT PLW_RTL_EVENT pEvent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int error = 0;

    pEvent->Private.Flags = 0;

    error = pthread_mutex_init(&pEvent->Private.Mutex, NULL);
    status = LwErrnoToNtStatus(error);
    GOTO_CLEANUP_ON_STATUS(status);

    SetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_MUTEX_INITIALIZED);

    error = pthread_cond_init(&pEvent->Private.Condition, NULL);
    status = LwErrnoToNtStatus(error);
    GOTO_CLEANUP_ON_STATUS(status);

    SetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_COND_INITIALIZED);

cleanup:
    if (status)
    {
        LwRtlCleanupEvent(pEvent);
    }

    return status;
}

VOID
LwRtlCleanupEvent(
    IN OUT PLW_RTL_EVENT pEvent
    )
{
    if (pEvent)
    {
        if (IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_COND_INITIALIZED))
        {
            PTHREAD_COND_DESTROY(&pEvent->Private.Condition);
        }
        if (IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_MUTEX_INITIALIZED))
        {
            PTHREAD_MUTEX_DESTROY(&pEvent->Private.Mutex);
        }
        pEvent->Private.Flags = 0;
    }
}

#if 0
static
LW_BOOLEAN
LwRtlpTimedWaitEvent(
    IN PLW_RTL_EVENT pEvent,
    IN OPTIONAL PLW_RTL_WINDOWS_TIME Timeout
    )
{
    BOOLEAN gotLock = FALSE;
    struct timespec absoluteTimespec = { 0 };
    NTSTATUS status = 0;

    LWIO_ASSERT(LwRtlpIsInitializedEvent(pEvent));

    status = LwRtlpUnixTimespecFromWindowsTime(&absoluteTimespec, *Timeout);
    LWIO_ASSERT(NT_SUCCESS(status));
    if (status)
    {
        GOTO_CLEANUP();
    }

    PTHREAD_MUTEX_LOCK(&pEvent->Private.Mutex);

    // Protect against spurious or stolen wakes.  Note thta
    // this should never happen for pthread condition variables.
    while (!IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET))
    {
        PTHREAD_COND_TIMEDWAIT(
                &gotLock,
                &pEvent->Private.Condition,
                &pEvent->Private.Mutex,
                &absoluteTimespec);
        if (!gotLock)
        {
            break;
        }
    }

    if (gotLock)
    {
        LWIO_ASSERT(IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET));
        PTHREAD_MUTEX_UNLOCK(&pEvent->Private.Mutex);
    }

cleanup:
    return gotLock;
}

static
LW_VOID
LwRtlpSimpleWaitEvent(
    IN PLW_RTL_EVENT pEvent
    )
{
    LWIO_ASSERT(LwRtlpIsInitializedEvent(pEvent));

    PTHREAD_MUTEX_LOCK(&pEvent->Private.Mutex);

    // Protect against spurious or stolen wakes.  Note thta
    // this should never happen for pthread condition variables.
    while (!IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET))
    {
        PTHREAD_COND_WAIT(&pEvent->Private.Condition, &pEvent->Private.Mutex);
        // TODO-Perhaps remove this assert
        LWIO_ASSERT(IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET));
    }

    PTHREAD_MUTEX_UNLOCK(&pEvent->Private.Mutex);
}
#endif

LW_BOOLEAN
LwRtlWaitEvent(
    IN PLW_RTL_EVENT pEvent,
    IN OPTIONAL PLW_RTL_WINDOWS_TIME Timeout
    )
{
    BOOLEAN isSignalled = FALSE;
    struct timespec absoluteTimespec = { 0 };

    LWIO_ASSERT(LwRtlpIsInitializedEvent(pEvent));

    if (Timeout)
    {
        NTSTATUS status = 0;

        status = LwRtlpUnixTimespecFromWindowsTime(&absoluteTimespec, *Timeout);
        LWIO_ASSERT(NT_SUCCESS(status));
        if (status)
        {
            GOTO_CLEANUP();
        }
    }

    PTHREAD_MUTEX_LOCK(&pEvent->Private.Mutex);

    if (Timeout)
    {
        // Protect against spurious or stolen wakes.  Note thta
        // this should never happen for pthread condition variables.
        while (!IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET))
        {
            PTHREAD_COND_TIMEDWAIT(
                    &isSignalled,
                    &pEvent->Private.Condition,
                    &pEvent->Private.Mutex,
                    &absoluteTimespec);
            if (!isSignalled)
            {
                break;
            }
        }
    }
    else
    {
        // Protect against spurious or stolen wakes.  Note thta
        // this should never happen for pthread condition variables.
        while (!IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET))
        {
            PTHREAD_COND_WAIT(&pEvent->Private.Condition, &pEvent->Private.Mutex);
            // TODO-Perhaps remove this assert
            LWIO_ASSERT(IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET));
        }
        isSignalled = TRUE;
    }

    if (isSignalled)
    {
        LWIO_ASSERT(IsSetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET));
    }

    PTHREAD_MUTEX_UNLOCK(&pEvent->Private.Mutex);

cleanup:
    return isSignalled;
}

VOID
LwRtlSetEvent(
    IN PLW_RTL_EVENT pEvent
    )
{
    LWIO_ASSERT(LwRtlpIsInitializedEvent(pEvent));

    PTHREAD_MUTEX_LOCK(&pEvent->Private.Mutex);

    SetFlag(pEvent->Private.Flags, _LW_RTL_EVENT_FLAG_SET);
    PTHREAD_COND_BROADCAST(&pEvent->Private.Condition);

    PTHREAD_MUTEX_UNLOCK(&pEvent->Private.Mutex);
}

// Internal flags
#define _LW_RTL_MUTEX_FLAG_INITIALIZED          0x1

static
BOOLEAN
LwRtlpIsInitializedMutex(
    IN PLW_RTL_MUTEX pMutex
    )
{
    return IsSetFlag(pMutex->Private.Flags, _LW_RTL_MUTEX_FLAG_INITIALIZED);
}

NTSTATUS
LwRtlInitializeMutex(
    OUT PLW_RTL_MUTEX pMutex,
    IN BOOLEAN IsRecursive
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int error = 0;
    BOOLEAN isMutexAttrInitialized = FALSE;
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_t* useMutexAttr = NULL;

    pMutex->Private.Flags = 0;

    if (IsRecursive)
    {
        error = pthread_mutexattr_init(&mutexAttr);
        status = LwErrnoToNtStatus(error);
        GOTO_CLEANUP_ON_STATUS(status);

        isMutexAttrInitialized = TRUE;

        error = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
        status = LwErrnoToNtStatus(error);
        GOTO_CLEANUP_ON_STATUS(status);

        useMutexAttr = &mutexAttr;
    }
#if 1
    else
    {
        error = pthread_mutexattr_init(&mutexAttr);
        status = LwErrnoToNtStatus(error);
        GOTO_CLEANUP_ON_STATUS(status);

        isMutexAttrInitialized = TRUE;

        error = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_ERRORCHECK);
        status = LwErrnoToNtStatus(error);
        GOTO_CLEANUP_ON_STATUS(status);

        useMutexAttr = &mutexAttr;
    }
#endif

    error = pthread_mutex_init(&pMutex->Private.Mutex, useMutexAttr);
    status = LwErrnoToNtStatus(error);
    GOTO_CLEANUP_ON_STATUS(status);

    SetFlag(pMutex->Private.Flags, _LW_RTL_MUTEX_FLAG_INITIALIZED);

cleanup:
    if (status)
    {
        LwRtlCleanupMutex(pMutex);
    }

    if (isMutexAttrInitialized)
    {
        PTHREAD_MUTEXATTTR_DESTROY(&mutexAttr);
    }

    return status;
}

VOID
LwRtlCleanupMutex(
    IN OUT PLW_RTL_MUTEX pMutex
    )
{
    if (pMutex)
    {
        if (IsSetFlag(pMutex->Private.Flags, _LW_RTL_MUTEX_FLAG_INITIALIZED))
        {
            PTHREAD_MUTEX_DESTROY(&pMutex->Private.Mutex);
        }
        pMutex->Private.Flags = 0;
    }
}

BOOLEAN
LwRtlTryLockMutex(
    IN PLW_RTL_MUTEX pMutex
    )
{
    BOOLEAN gotLock = FALSE;

    LWIO_ASSERT(LwRtlpIsInitializedMutex(pMutex));

    PTHREAD_MUTEX_TRYLOCK(&gotLock, &pMutex->Private.Mutex);

    return gotLock;
}

VOID
LwRtlLockMutex(
    IN PLW_RTL_MUTEX pMutex
    )
{
    LWIO_ASSERT(LwRtlpIsInitializedMutex(pMutex));

    PTHREAD_MUTEX_LOCK(&pMutex->Private.Mutex);
}

VOID
LwRtlUnlockMutex(
    IN PLW_RTL_MUTEX pMutex
    )
{
    LWIO_ASSERT(LwRtlpIsInitializedMutex(pMutex));

    PTHREAD_MUTEX_UNLOCK(&pMutex->Private.Mutex);
}

// Internal flags
#define _LW_RTL_CONDITION_VARIABLE_FLAG_INITIALIZED     0x1

static
BOOLEAN
LwRtlpIsInitializedConditionVariable(
    IN PLW_RTL_CONDITION_VARIABLE pConditionVariable
    )
{
    return IsSetFlag(pConditionVariable->Private.Flags, _LW_RTL_CONDITION_VARIABLE_FLAG_INITIALIZED);
}

LW_NTSTATUS
LwRtlInitializeConditionVariable(
    OUT PLW_RTL_CONDITION_VARIABLE pConditionVariable
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int error = 0;

    pConditionVariable->Private.Flags = 0;

    error = pthread_cond_init(&pConditionVariable->Private.Condition, NULL);
    status = LwErrnoToNtStatus(error);
    GOTO_CLEANUP_ON_STATUS(status);

    SetFlag(pConditionVariable->Private.Flags, _LW_RTL_CONDITION_VARIABLE_FLAG_INITIALIZED);

cleanup:
    if (status)
    {
        LwRtlCleanupConditionVariable(pConditionVariable);
    }

    return status;
}

LW_VOID
LwRtlCleanupConditionVariable(
    IN OUT PLW_RTL_CONDITION_VARIABLE pConditionVariable
    )
{
    if (pConditionVariable)
    {
        if (IsSetFlag(pConditionVariable->Private.Flags, _LW_RTL_CONDITION_VARIABLE_FLAG_INITIALIZED))
        {
            PTHREAD_COND_DESTROY(&pConditionVariable->Private.Condition);
        }
        pConditionVariable->Private.Flags = 0;
    }
}

LW_BOOLEAN
LwRtlWaitConditionVariable(
    IN PLW_RTL_CONDITION_VARIABLE pConditionVariable,
    IN PLW_RTL_MUTEX pMutex,
    IN OPTIONAL PLW_RTL_WINDOWS_TIME Timeout
    )
{
    BOOLEAN isSignalled = FALSE;

    LWIO_ASSERT(LwRtlpIsInitializedConditionVariable(pConditionVariable));
    LWIO_ASSERT(LwRtlpIsInitializedMutex(pMutex));

    if (Timeout)
    {
        NTSTATUS status = 0;
        struct timespec absoluteTimespec = { 0 };

        status = LwRtlpUnixTimespecFromWindowsTime(&absoluteTimespec, *Timeout);
        LWIO_ASSERT(NT_SUCCESS(status));
        if (!status)
        {
            PTHREAD_COND_TIMEDWAIT(
                    &isSignalled,
                    &pConditionVariable->Private.Condition,
                    &pMutex->Private.Mutex,
                    &absoluteTimespec);
        }
    }
    else
    {
        PTHREAD_COND_WAIT(&pConditionVariable->Private.Condition, &pMutex->Private.Mutex);
        isSignalled = TRUE;
    }

    return isSignalled;
}

LW_VOID
LwRtlSignalConditionVariable(
    IN PLW_RTL_CONDITION_VARIABLE pConditionVariable
    )
{
    LWIO_ASSERT(LwRtlpIsInitializedConditionVariable(pConditionVariable));

    PTHREAD_COND_SIGNAL(&pConditionVariable->Private.Condition);
}

LW_VOID
LwRtlBroadcastConditionVariable(
    IN PLW_RTL_CONDITION_VARIABLE pConditionVariable
    )
{
    LWIO_ASSERT(LwRtlpIsInitializedConditionVariable(pConditionVariable));

    PTHREAD_COND_BROADCAST(&pConditionVariable->Private.Condition);
}

typedef struct _LW_RTL_THREAD {
    struct {
        pthread_t Thread;
    } Private;
} LW_RTL_THREAD;

LW_NTSTATUS
LwRtlCreateThread(
    OUT PLW_RTL_THREAD* ppThread,
    IN LW_THREAD_PROC ThreadRoutine,
    IN LW_PVOID ThreadContext
    )
{
    NTSTATUS status = 0;
    int error = 0;
    PLW_RTL_THREAD pThread = NULL;

    status = LW_RTL_ALLOCATE(&pThread, LW_RTL_THREAD, sizeof(*pThread));
    GOTO_CLEANUP_ON_STATUS(status);

    error = pthread_create(&pThread->Private.Thread, NULL, ThreadRoutine, ThreadContext);
    status = LwErrnoToNtStatus(error);

    // cannot fail after here

cleanup:
    if (status)
    {
        RTL_FREE(&pThread);
    }

    *ppThread = pThread;

    return status;
}

LW_PVOID
LwRtlJoinThread(
    IN OUT PLW_RTL_THREAD pThread
    )
{
    LW_PVOID result = NULL;
    PTHREAD_JOIN(pThread->Private.Thread, &result);
    RTL_FREE(&pThread);
    return result;
}

LW_VOID
LwRtlDetachThread(    
    IN OUT PLW_RTL_THREAD pThread
    )
{
    PTHREAD_DETACH(pThread->Private.Thread);
    RTL_FREE(&pThread);
}

////////

#if 0
static
NTSTATUS
LwRtlThreadsCreateThreadEx(
    OUT pthread_t** ppThread,
    IN BOOLEAN bStartDetached,
    IN PVOID (*ThreadRoutine)(PVOID),
    IN PVOID ThreadContext
    )
{
    NTSTATUS status = 0;
    int error = 0;
    pthread_t* pThread = NULL;
    pthread_attr_t* pThreadAttr = NULL;
    pthread_attr_t threadAttr;

    status = LW_RTL_ALLOCATE(&pThread, pthread_t, sizeof(*pThread));
    GOTO_CLEANUP_ON_STATUS(status);

    if (bStartDetached)
    {
        error = pthread_attr_init(&threadAttr);
        // ISSUE-2008/10/02-dalmeida -- Map error code?
        status = error;
        GOTO_CLEANUP_ON_STATUS(status);

        error = pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
        status = LwErrnoToNtStatus(error);
        GOTO_CLEANUP_ON_STATUS(status);

        pThreadAttr = &threadAttr;
    }

    error = pthread_create(pThread, pThreadAttr, ThreadRoutine, ThreadContext);
    status = LwErrnoToNtStatus(error);
    GOTO_CLEANUP_ON_STATUS(status);

    if (bStartDetached)
    {
        RTL_FREE(&pThread);
    }

cleanup:
    if (status)
    {
        RTL_FREE(&pThread);
    }

    *ppThread = pThread;

    return status;
}

NTSTATUS
LwRtlThreadsCreateThread(
    OUT pthread_t** ppThread,
    IN PVOID (*ThreadRoutine)(PVOID),
    IN PVOID ThreadContext
    )
{
    return LwRtlThreadsCreateThreadEx(ppThread, FALSE, ThreadRoutine, ThreadContext);
}

NTSTATUS
LwRtlThreadsCreateDetachedThread(
    IN PVOID (*ThreadRoutine)(PVOID),
    IN PVOID ThreadContext
    )
{
    pthread_t* pThread = NULL;
    return LwRtlThreadsCreateThreadEx(&pThread, TRUE, ThreadRoutine, ThreadContext);
}

static
NTSTATUS
LwRtlThreadsCreateOrInitializeMutex(
    OUT OPTIONAL pthread_mutex_t** NewMutex,
    OUT OPTIONAL pthread_mutex_t* ExistingMutex,
    IN LW_RTL_THREADS_MUTEX_TYPE MutexType
    )
{
    NTSTATUS status = 0;
    int error = 0;
    pthread_mutexattr_t mutexAttrStorage;
    pthread_mutexattr_t* mutexAttr = NULL;
    pthread_mutex_t* newMutex = NULL;
    pthread_mutex_t* useMutex = NULL;

    if (IS_BOTH_OR_NEITHER(NewMutex, ExistingMutex))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (MutexType)
    {
        error = pthread_mutexattr_init(&mutexAttrStorage);
        if (error)
        {
            status = LwErrnoToNtStatus(error);
            GOTO_CLEANUP_ON_STATUS(status);
        }

        mutexAttr = &mutexAttrStorage;

        switch (MutexType)
        {
            case LW_RTL_THREADS_MUTEX_TYPE_RECURSIVE:
                error = pthread_mutexattr_settype(mutexAttr, PTHREAD_MUTEX_RECURSIVE);
                if (error)
                {
                    status = LwErrnoToNtStatus(error);
                    GOTO_CLEANUP_ON_STATUS(status);
                }
                GOTO_CLEANUP_ON_STATUS(status);
                break;
            default:
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP();
        }
    }

    useMutex = ExistingMutex;
    if (!useMutex)
    {
        status = LW_RTL_ALLOCATE(&newMutex, pthread_mutex_t, sizeof(*newMutex));
        GOTO_CLEANUP_ON_STATUS(status);

        useMutex = newMutex;
    }

    error = pthread_mutex_init(useMutex, mutexAttr);
    if (error)
    {
        status = LwErrnoToNtStatus(error);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    newMutex = NULL;

cleanup:
    LwRtlThreadsDestroyMutex(&newMutex);

    if (mutexAttr)
    {
        pthread_mutexattr_destroy(mutexAttr);
    }

    if (NewMutex)
    {
        *NewMutex = useMutex;
    }

    return status;
}

NTSTATUS
LwRtlThreadsCreateMutex(
    OUT pthread_mutex_t** ppMutex,
    IN LW_RTL_THREADS_MUTEX_TYPE MutexType
    )
{
    return LwRtlThreadsCreateOrInitializeMutex(ppMutex, NULL, MutexType);
}

VOID
LwRtlThreadsDestroyMutex(
    IN OUT pthread_mutex_t** ppMutex
    )
{
    if (*ppMutex)
    {
        pthread_mutex_destroy(*ppMutex);
        LwRtlMemoryFree(*ppMutex);
        *ppMutex = NULL;
    }
}

NTSTATUS
LwRtlThreadsInitializeMutex(
    OUT pthread_mutex_t* pMutex,
    IN LW_RTL_THREADS_MUTEX_TYPE MutexType
    )
{
    return LwRtlThreadsCreateOrInitializeMutex(NULL, pMutex, MutexType);
}

VOID
LwRtlThreadsCleanupMutex(
    IN OUT pthread_mutex_t* pMutex
    )
{
    if (pMutex)
    {
        pthread_mutex_destroy(pMutex);
    }
}

#define _LW_RTL_THREADS_LOG_ERROR(Format, ...)

#define _LW_RTL_THREADS_MUTEX_OP(Function, pMutex) \
    do { \
        int mutexOpError = (Function)(pMutex); \
        if (mutexOpError) \
        { \
            _LW_RTL_THREADS_LOG_ERROR(#Function "() failed (error = %d)", mutexOpError); \
        } \
    } while (0)

VOID
LwThreadsAcquireMutex(
    IN pthread_mutex_t* pMutex
    )
{
    _LW_RTL_THREADS_MUTEX_OP(pthread_mutex_lock, pMutex);
}

VOID
LwThreadsReleaseMutex(
    IN pthread_mutex_t* pMutex
    )
{
    _LW_RTL_THREADS_MUTEX_OP(pthread_mutex_unlock, pMutex);
}

NTSTATUS
LwRtlThreadsCreateCond(
    OUT pthread_cond_t** ppCond
    )
{
    NTSTATUS status = 0;
    int error = 0;
    pthread_cond_t* pCond = NULL;

    status = LW_RTL_ALLOCATE(&pCond, pthread_cond_t, sizeof(*pCond));
    GOTO_CLEANUP_ON_STATUS(status);

    error = pthread_cond_init(pCond, NULL);
    status = LwErrnoToNtStatus(error);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (status)
    {
        RTL_FREE(&pCond);
    }

    *ppCond = pCond;
    return status;
}

VOID
LwRtlThreadsDestroyCond(
    IN OUT pthread_cond_t** ppCond
    )
{
    if (*ppCond)
    {
        pthread_cond_destroy(*ppCond);
        LwRtlMemoryFree(*ppCond);
        *ppCond = NULL;
    }
}
#endif
