/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include <moonunit/moonunit.h>
#include <lw/base.h>
#include <lw/rtlgoto.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>

#include "benchmark.h"

#define MU_ASSERT_STATUS_SUCCESS(status) \
    MU_ASSERT(STATUS_SUCCESS == (status))

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

PLW_THREAD_POOL gpPool = NULL;
pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gEvent = PTHREAD_COND_INITIALIZER;

MU_FIXTURE_SETUP(Task)
{
    MU_ASSERT_STATUS_SUCCESS(LwRtlBlockSignals());
    MU_ASSERT_STATUS_SUCCESS(LwRtlCreateThreadPool(&gpPool, NULL));   
}

MU_FIXTURE_TEARDOWN(Task)
{
    LwRtlFreeThreadPool(&gpPool);
}

static
VOID
BasicWorkItem(
    PLW_WORK_ITEM pItem,
    PVOID pContext
    )
{
    BOOLEAN volatile *pbValue = pContext;

    pthread_mutex_lock(&gLock);
    *pbValue = TRUE;
    pthread_cond_signal(&gEvent);
    pthread_mutex_unlock(&gLock);

    LwRtlFreeWorkItem(&pItem);
}

MU_TEST(Task, BasicWorkItem)
{
    BOOLEAN volatile bValue = FALSE;
    PLW_WORK_ITEM pItem = NULL;

    MU_ASSERT_STATUS_SUCCESS(LwRtlCreateWorkItem(
        gpPool,
        &pItem,
        BasicWorkItem,
        (PVOID) &bValue));

    LwRtlScheduleWorkItem(pItem, 0);

    pthread_mutex_lock(&gLock);
    while (!bValue)
    {
        pthread_cond_wait(&gEvent, &gLock);
    }
    pthread_mutex_unlock(&gLock);
}

static
VOID
CompatWorkItem(
    PVOID pContext
    )
{
    BOOLEAN volatile *pbValue = pContext;

    pthread_mutex_lock(&gLock);
    *pbValue = TRUE;
    pthread_cond_signal(&gEvent);
    pthread_mutex_unlock(&gLock);
}

MU_TEST(Task, CompatWorkItem)
{
    BOOLEAN volatile bValue = FALSE;

    MU_ASSERT_STATUS_SUCCESS(LwRtlQueueWorkItem(
                                 gpPool,
                                 CompatWorkItem,
                                 (PVOID) &bValue,
                                 0));

    pthread_mutex_lock(&gLock);
    while (!bValue)
    {
        pthread_cond_wait(&gEvent, &gLock);
    }
    pthread_mutex_unlock(&gLock);
}

static const int gTarget = 5;

static
VOID
Timer(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    )
{
    int volatile *pValue = pContext;

    if (*pValue < gTarget)
    {
        (*pValue)++;
        *pWaitMask = LW_TASK_EVENT_TIME;
        *pllTime = 10000000ll; /* 10 ms */
    }
    else
    {
        *pWaitMask = 0;
    }
}

MU_TEST(Task, Timer)
{
    int value = 0;
    PLW_TASK pTask = NULL;

    MU_ASSERT_STATUS_SUCCESS(LwRtlCreateTask(
                                 gpPool,
                                 &pTask,
                                 NULL,
                                 Timer, 
                                 (PVOID) &value));

    LwRtlWakeTask(pTask);
    LwRtlWaitTask(pTask);
    LwRtlReleaseTask(&pTask);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, value, gTarget);
}

static
VOID
Yield(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pllTime
    )
{
    int volatile *pValue = pContext;

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        MU_ASSERT(*pValue == 0);
    }

    if (*pValue < gTarget)
    {
        (*pValue)++;
        *pWaitMask = LW_TASK_EVENT_YIELD;
    }
    else
    {
        *pWaitMask = 0;
    }
}

MU_TEST(Task, Yield)
{
    int value1 = 0;
    int value2 = 0;
    PLW_TASK pTask1 = NULL;
    PLW_TASK pTask2 = NULL;
    PLW_TASK_GROUP pGroup = NULL;

    MU_ASSERT_STATUS_SUCCESS(LwRtlCreateTaskGroup(
                                 gpPool,
                                 &pGroup));

    MU_ASSERT_STATUS_SUCCESS(LwRtlCreateTask(
                                 gpPool,
                                 &pTask1,
                                 pGroup,
                                 Yield,
                                 (PVOID) &value1));

    MU_ASSERT_STATUS_SUCCESS(LwRtlCreateTask(
                                 gpPool,
                                 &pTask1,
                                 pGroup,
                                 Yield,
                                 (PVOID) &value2));

    LwRtlWakeTaskGroup(pGroup);
    LwRtlReleaseTask(&pTask1);
    LwRtlReleaseTask(&pTask2);
    LwRtlWaitTaskGroup(pGroup);
    LwRtlFreeTaskGroup(&pGroup);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, value1, gTarget);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, value2, gTarget);
}

static
VOID
OnOff(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pllTime
    )
{
    int* fds = pContext;
    char c = 0;

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        /* Wait for readability of pipe.
         * Write a byte so we wake up immediately.
         */
        MU_ASSERT_STATUS_SUCCESS(LwRtlSetTaskFd(pTask, fds[0], LW_TASK_EVENT_FD_READABLE));
        MU_ASSERT(write(fds[1], &c, sizeof(c)) == sizeof(c));
        *pWaitMask = LW_TASK_EVENT_FD_READABLE;
    }
    else if (WakeMask & LW_TASK_EVENT_FD_READABLE)
    {
        /* Read the byte we wrote */
        MU_ASSERT(read(fds[0], &c, sizeof(c)) == sizeof(c));
        /* Write another byte so the read end is readable again */
        MU_ASSERT(write(fds[1], &c, sizeof(c)) == sizeof(c));
        /* Wait for a 1-nanosecond timeout instead.
         * Internally, the threadpoll will need to disable any readability
         * event it configured.
         */
        *pllTime = 1;
        *pWaitMask = LW_TASK_EVENT_TIME;
    }
    else if (WakeMask & LW_TASK_EVENT_TIME)
    {
        /* Ensure that we did not receive a readability event */
        MU_ASSERT(!(WakeMask & LW_TASK_EVENT_FD_READABLE));
        /* Test complete */
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }
}

MU_TEST(Task, EventOnEventOff)
{
    PLW_TASK pTask = NULL;
    int fds[2] = {-1, -1};

    MU_ASSERT(pipe(fds) == 0);

    MU_ASSERT_STATUS_SUCCESS(LwRtlCreateTask(
                                 gpPool,
                                 &pTask,
                                 NULL,
                                 OnOff,
                                 (PVOID) &fds));

    LwRtlWakeTask(pTask);
    LwRtlWaitTask(pTask);
    LwRtlReleaseTask(&pTask);

    MU_ASSERT(close(fds[0]) == 0);
    MU_ASSERT(close(fds[1]) == 0);
}

#define BUFFER_SIZE (64 * 1024)
#define SEND_SEGMENTS 1
#define NUM_ITERATIONS 32
#define NUM_PAIRS 250

MU_TEST(Task, Transceive)
{
    static BENCHMARK_SETTINGS settings =
    {
        .ulBufferSize = BUFFER_SIZE,
        .usSendSegments = SEND_SEGMENTS,
        .ulIterations = NUM_ITERATIONS,
        .ulPairs = NUM_PAIRS
    };
    ULONG64 ullTotal = 0;
    ULONG64 ullTime = 0;

    BenchmarkThreadPool(
        gpPool,
        &settings,
        &ullTime,
        &ullTotal);

    MU_INFO("Transferred %llu bytes in %.2f seconds, %.2f mbit/s",
            (unsigned long long) ullTotal,
            ullTime / 1000000000.0,
            (ullTotal / 131072.0) / (ullTime / 1000000000.0));
}

static
VOID
WaitSigTerm(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pllTime
    )
{
    siginfo_t info;

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        MU_ASSERT_STATUS_SUCCESS(LwRtlSetTaskUnixSignal(pTask, SIGTERM, TRUE));
        *pWaitMask = LW_TASK_EVENT_UNIX_SIGNAL;
        kill(getpid(), SIGTERM);
    }
    else if (WakeMask & LW_TASK_EVENT_UNIX_SIGNAL)
    {
        while (LwRtlNextTaskUnixSignal(pTask, &info))
        {
            switch(info.si_signo)
            {
            case SIGTERM:
                LwRtlExitMain(STATUS_SUCCESS);
                *pWaitMask = LW_TASK_EVENT_COMPLETE;
                return;
                break;
            default:
                break;
            }
        }

        *pWaitMask = LW_TASK_EVENT_UNIX_SIGNAL;
    }
}

MU_TEST(Task, ExitOnSignal)
{
    PLW_TASK pTask = NULL;

    MU_ASSERT_STATUS_SUCCESS(LwRtlCreateTask(
                                 gpPool,
                                 &pTask,
                                 NULL,
                                 WaitSigTerm, 
                                 NULL));

    LwRtlWakeTask(pTask);

    MU_ASSERT_STATUS_SUCCESS(LwRtlMain());
}
