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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Test Program for running performance tests against lsassd and winbind
 *
 * Authors: Kyle Stemen <kstemen@likewisesoftware.com>
 *
 */

#include "types.h"
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include <errno.h>
#include "tests.h"

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#ifndef HAVE_CLOCK_GETTIME

/* replacement implementation for systems that lack clock_gettime
   support (e.g. Mac OS X) */

typedef int clockid_t;

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    int ret;
    struct timeval tv;

    if (tp == NULL)
    {
	return EFAULT;
    }

    if (clk_id != CLOCK_REALTIME)
    {
	return EINVAL;
    }

    ret = gettimeofday(&tv, NULL);
    if (ret)
    {
	return ret;
    }

    tp->tv_sec  = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

#endif /* HAVE_CLOCK_GETTIME */


int64_t
GetTimeDiff(
        const struct timespec *end,
        const struct timespec *start
        )
{
    int64_t endNanoSecs = (int64_t)end->tv_sec * 1000000000 +
        end->tv_nsec;
    int64_t startNanoSecs = (int64_t)start->tv_sec * 1000000000 +
        start->tv_nsec;

    return endNanoSecs - startNanoSecs;
}

void RunTests(
        PerfTest* tests,
        size_t testCount
        )
{
    size_t testIndex;
    BOOL passed;
    PVOID runArg;
    struct timespec startTime = {0};
    struct timespec endTime = {0};
    int result;
    int runNum;

    for (testIndex = 0; testIndex < testCount; testIndex++)
    {
        printf("Running: %s\n", tests[testIndex].description);

        runArg = NULL;
        if (tests[testIndex].setup != NULL)
        {
            passed = tests[testIndex].setup(
                    tests[testIndex].setupArg,
                    &runArg);
            if (!passed)
            {
                printf("Failed\n");
                continue;
            }
        }

        if (tests[testIndex].run == NULL)
        {
            printf("Error: test function is null\n");
            continue;
        }

        result = clock_gettime(CLOCK_REALTIME, &startTime);
        if (result < 0)
        {
            perror("clock_gettime");
            return;
        }

        switch (tests[testIndex].type)
        {
            case TEST_TYPE_RUNS_PER_SEC:
                runNum = 0;
                do
                {
                    do
                    {
                        passed = tests[testIndex].run(
                                runArg);
                        runNum++;
                    } while (passed && runNum % 10 != 0);

                    result = clock_gettime(CLOCK_REALTIME, &endTime);
                    if (result < 0)
                    {
                        perror("clock_gettime");
                        return;
                    }
                } while (passed &&
                        GetTimeDiff(&endTime, &startTime) < 10 * (int64_t)1000000000);

                if (passed)
                {
                    printf("Result: %f calls per second\n", runNum /
                            (GetTimeDiff(&endTime, &startTime) / 1000000000.0));
                }
                else
                {
                    printf("Failed\n");
                }
                break;

            case TEST_TYPE_SINGLE_RUN:
                passed = tests[testIndex].run(
                        runArg);
                result = clock_gettime(CLOCK_REALTIME, &endTime);
                if (result < 0)
                {
                    perror("clock_gettime");
                    return;
                }

                if (passed)
                {
                    printf("Result: %f seconds\n",
                            (GetTimeDiff(&endTime, &startTime) / 1000000000.0));
                }
                else
                {
                    printf("Failed\n");
                }
                break;

            default:
                printf("Unknown test type %d\n", tests[testIndex].type);
                return;
        }
        if (tests[testIndex].cleanup != NULL)
        {
            tests[testIndex].cleanup(runArg);
        }
    }
}

void RunLsassTests(
        void *lsassTestLib,
        const char *user0001
        )
{
    PerfTest testList[] =
    {
        {
            "Lsass server connect and disconnects per second",
            TEST_TYPE_RUNS_PER_SEC,
            NULL,
            dlsym(lsassTestLib, "RunConnectDisconnect"),
            NULL,
            NULL
        },
        {
            "Cached LsaFindUserById's per second for user0001 with shared connection",
            TEST_TYPE_RUNS_PER_SEC,
            dlsym(lsassTestLib, "SetupFindUserById"),
            dlsym(lsassTestLib, "RunFindUserById"),
            dlsym(lsassTestLib, "CleanupFindUserById"),
            (PVOID)user0001
        },
        {
            "LsaGetLogInfo's per second with shared connection (tests marshalling latency)",
            TEST_TYPE_RUNS_PER_SEC,
            dlsym(lsassTestLib, "SetupConnectLsass"),
            dlsym(lsassTestLib, "RunGetLogLevel"),
            dlsym(lsassTestLib, "CleanupConnectLsass"),
            (PVOID)user0001
        },
    };

    RunTests(
            testList,
            sizeof(testList)/sizeof(testList[0]));
}

int main(int argc, const char *argv[])
{
    char user0001[256];
    char groupsize1[256];
    char groupsize1000[256];
    char user[256];
    char usergroup[256];
    void *libperflsass = NULL;

    PerfTest testList[] =
    {
        {
            "Cached getpwuid's per second for user0001",
            TEST_TYPE_RUNS_PER_SEC,
            SetupGrabUid,
            RunGrabUid,
            NULL,
            user0001,
        },
        {
            "Cached getgrgid's per second for groupsize1",
            TEST_TYPE_RUNS_PER_SEC,
            SetupGrabGid,
            RunGrabGid,
            NULL,
            groupsize1
        },
        {
            "Cached getgrgid's per second for groupsize1000",
            TEST_TYPE_RUNS_PER_SEC,
            SetupGrabGid,
            RunGrabGid,
            NULL,
            groupsize1000
        },
        {
            "Uncached 500 user lookup for user0001-user0500",
            TEST_TYPE_SINGLE_RUN,
            SetupClearCache,
            RunGrabUsers1_500,
            NULL,
            user
        },
        {
            "Uncached 500 group lookup for usergroup0001-usergroup0500",
            TEST_TYPE_SINGLE_RUN,
            SetupClearCache,
            RunGrabGroups1_500,
            NULL,
            usergroup
        },
    };

    if (argc != 2 || argv[1][0] == '-')
    {
        printf("Usage: %s <domain>\n"
                "\n"
                "This program runs a series of nsswitch performance tests against winbind or \n"
                "lsassd.\n"
                "\n"
                "The following users should be available from the domain:\n"
                "<domain>\\user0001 through <domain>\\user0500\n"
                "\n"
                "The following groups should be available from the domain:\n"
                "<domain>\\groupsize1\n"
                "<domain>\\groupsize1000\n"
                "<domain>\\usergroup0001 through <domain>\\usergroup0500\n",
                argv[0]);
        exit(1);
    }

    snprintf(user0001, sizeof(user0001), "%s\\user0001", argv[1]);
    snprintf(groupsize1, sizeof(groupsize1), "%s\\groupsize1", argv[1]);
    snprintf(groupsize1000, sizeof(groupsize1000), "%s\\groupsize1000", argv[1]);
    snprintf(user, sizeof(user), "%s\\user", argv[1]);
    snprintf(usergroup, sizeof(groupsize1), "%s\\usergroup", argv[1]);

    RunTests(
            testList,
            sizeof(testList)/sizeof(testList[0]));

    libperflsass = dlopen("./libperflsass.so", RTLD_NOW | RTLD_LOCAL);

    if (libperflsass != NULL)
    {
        printf("Running lsass specific tests:\n");
        RunLsassTests(libperflsass, user0001);
        dlclose(libperflsass);
    }
    else
    {
        printf("Unable to load libperflsass.so, so skipping lsass specific tests\n");
    }

    return 0;
}
