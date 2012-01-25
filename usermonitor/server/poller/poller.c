/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        poller.c
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 *
 *        Poller thread
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#include "includes.h"

#define NANOSECS_PER_SECOND 1000000000

VOID
UmnSrvTimevalToTimespec(
    OUT struct timespec *pDest,
    IN struct timeval *pSrc
    )
{
    pDest->tv_sec = pSrc->tv_sec;
    pDest->tv_nsec = pSrc->tv_usec * 1000;
}

VOID
UmnSrvTimespecAdd(
    OUT struct timespec *pDest,
    IN struct timespec *pA,
    IN struct timespec *pB
    )
{
    long lTotalSecs = pA->tv_sec + pB->tv_sec;
    long lTotalNSecs = 0;

    // Just incase the source operands were in an overflowed format, normalize
    // them now.
    lTotalSecs += pA->tv_nsec / NANOSECS_PER_SECOND;
    lTotalSecs += pB->tv_nsec / NANOSECS_PER_SECOND;

    // The largest valid value for tv_nsec is (10^9 - 1). Two of these values
    // can be added without worrying about overflow because:
    //      (10^9 - 1) * 2 < (2^31 - 1)
    lTotalNSecs = pA->tv_nsec % NANOSECS_PER_SECOND +
                    pB->tv_nsec % NANOSECS_PER_SECOND;

    // Carry from nsecs to secs
    lTotalSecs += lTotalNSecs / NANOSECS_PER_SECOND;
    lTotalNSecs %= NANOSECS_PER_SECOND;

    pDest->tv_sec = lTotalSecs;
    pDest->tv_nsec = lTotalNSecs;
}

VOID
UmnSrvTimespecSubtract(
    OUT struct timespec *pDest,
    IN struct timespec *pA,
    IN struct timespec *pB
    )
{
    long lTotalSecs = pA->tv_sec - pB->tv_sec;
    long lTotalNSecs = 0;

    // Just incase the source operands were in an overflowed format, normalize
    // them now.
    lTotalSecs += pA->tv_nsec / NANOSECS_PER_SECOND;
    lTotalSecs -= pB->tv_nsec / NANOSECS_PER_SECOND;

    // The result of this computation is in the range:
    // -(10^9 - 1) < lTotalNSecs < 10^9 - 1
    // That range is representable in long (no overflow)
    lTotalNSecs = pA->tv_nsec % NANOSECS_PER_SECOND -
                    pB->tv_nsec % NANOSECS_PER_SECOND;

    // Carry from nsecs to secs
    if (lTotalNSecs < 0)
    {
        lTotalSecs--;
        lTotalNSecs += NANOSECS_PER_SECOND;
    }

    pDest->tv_sec = lTotalSecs;
    pDest->tv_nsec = lTotalNSecs;
}

DWORD
UmnSrvPollerRefresh(
    VOID
    )
{
    DWORD dwError = 0;

    if (gbPollerThreadRunning)
    {
        gbPollerRefresh = TRUE;
        dwError = pthread_cond_signal(
                        &gSignalPoller);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else
    {
        dwError = ESRCH;
        BAIL_ON_UMN_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

PVOID
UmnSrvPollerThreadRoutine(
    IN PVOID pUnused
    )
{
    DWORD dwError = 0;
    struct timeval now;
    struct timespec periodStart, periodUsed, nowSpec, nextWake, pushWait = {0};
    BOOLEAN bMutexLocked = FALSE;
    DWORD dwPeriodSecs = 60;

    UMN_LOG_INFO("User poller thread started");

    dwError = pthread_mutex_lock(&gSignalPollerMutex);
    BAIL_ON_UMN_ERROR(dwError);
    bMutexLocked = TRUE;

    while (!gbPollerThreadShouldExit)
    {
        dwError = gettimeofday(&now, NULL);
        BAIL_ON_UMN_ERROR(dwError);

        UmnSrvTimevalToTimespec(
            &periodStart,
            &now);

        dwError = gettimeofday(&now, NULL);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvUpdateAccountInfo(now.tv_sec);
        BAIL_ON_UMN_ERROR(dwError);

        // periodUsed = now - periodStart
        UmnSrvTimevalToTimespec(
            &periodUsed,
            &now);

        UmnSrvTimespecSubtract(
            &periodUsed,
            &periodUsed,
            &periodStart);

        pushWait.tv_sec = dwPeriodSecs;

        UmnSrvTimespecAdd(
            &nextWake,
            &periodStart,
            &pushWait);

        UMN_LOG_INFO("User poller sleeping for %f seconds",
                pushWait.tv_sec + pushWait.tv_nsec /
                (double)NANOSECS_PER_SECOND);

        while (!gbPollerThreadShouldExit && !gbPollerRefresh)
        {
            dwError = pthread_cond_timedwait(
                        &gSignalPoller,
                        &gSignalPollerMutex,
                        &nextWake);
            if (dwError == ETIMEDOUT)
            {
                dwError = 0;
                break;
            }
            else if (dwError == EINTR)
            {
                // Try again
                continue;
            }
            else if (dwError == 0)
            {
                // Check that this wasn't a spurious wakeup
                dwError = gettimeofday(&now, NULL);
                BAIL_ON_UMN_ERROR(dwError);

                UmnSrvTimevalToTimespec(
                    &nowSpec,
                    &now);

                if (nowSpec.tv_sec > nextWake.tv_sec ||
                        (nowSpec.tv_sec == nextWake.tv_sec &&
                            nowSpec.tv_nsec >= nextWake.tv_nsec))
                {
                    break;
                }
            }
            else
            {
                BAIL_ON_UMN_ERROR(dwError);
            }
        }
        gbPollerRefresh = FALSE;
    }

    UMN_LOG_INFO("User poller thread stopped");

cleanup:
    if (bMutexLocked)
    {
        pthread_mutex_unlock(&gSignalPollerMutex);
    }

    if (dwError != 0)
    {
        UMN_LOG_ERROR(
                "User monitor polling thread exiting with code %d",
                dwError);
        kill(getpid(), SIGTERM);
    }

    return NULL;

error:
    goto cleanup;
}

DWORD
UmnSrvStopPollerThread(
    VOID
    )
{
    DWORD dwError = 0;

    gbPollerThreadShouldExit = TRUE;
    if (gbPollerThreadRunning)
    {
        dwError = pthread_cond_signal(
                        &gSignalPoller);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = pthread_join(
                        gPollerThread,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);

        gbPollerThreadRunning = FALSE;
    }

    if (gbSignalPollerCreated)
    {
        dwError = pthread_cond_destroy(&gSignalPoller);
        BAIL_ON_UMN_ERROR(dwError);
        gbSignalPollerCreated = FALSE;
    }
                    
    if (gbSignalPollerMutexCreated)
    {
        dwError = pthread_mutex_destroy(&gSignalPollerMutex);
        BAIL_ON_UMN_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
UmnSrvStartPollerThread(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bDestroyOnError = FALSE;

    if (gbPollerThreadRunning)
    {
        dwError = EEXIST;
        BAIL_ON_UMN_ERROR(dwError);
    }

    bDestroyOnError = TRUE;

    dwError = pthread_cond_init(
                    &gSignalPoller,
                    NULL);
    BAIL_ON_UMN_ERROR(dwError);
    gbSignalPollerCreated = TRUE;
                    
    dwError = pthread_mutex_init(
                    &gSignalPollerMutex,
                    NULL);
    BAIL_ON_UMN_ERROR(dwError);
    gbSignalPollerMutexCreated = TRUE;

    gbPollerThreadShouldExit = FALSE;

    dwError = pthread_create(
                    &gPollerThread,
                    NULL,
                    UmnSrvPollerThreadRoutine,
                    NULL);
    BAIL_ON_UMN_ERROR(dwError);
    gbPollerThreadRunning = TRUE;

cleanup:
    return dwError;

error:
    if (bDestroyOnError)
    {
        UmnSrvStopPollerThread();
    }
    goto cleanup;
}
