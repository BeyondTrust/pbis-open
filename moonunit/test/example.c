/*
 * Copyright (c) 2007, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file example.c
 * @brief MoonUnit example tests
 */

/** \cond SKIP */

#include <moonunit/interface.h>

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

MU_LIBRARY_NAME("ExampleCTests");

static int constructed = 0;

MU_LIBRARY_CONSTRUCT()
{
    constructed = 42;
}

static int x = 0;
static int y = 0;

/* 
 * The following tests demonstrate basic testing
 * using mathematical expressions.
 */

/* Set up two variables for all subsequent arithmetic tests */
MU_FIXTURE_SETUP(Arithmetic)
{
	x = 2;
	y = 3;
}

MU_TEST(Arithmetic, equal)
{
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, constructed, 42);
}

MU_TEST(Arithmetic, add)
{
	MU_ASSERT_EQUAL(MU_TYPE_INTEGER, x + y, 5);
}

MU_TEST(Arithmetic, add2)
{
    MU_ASSERT_NOT_EQUAL(MU_TYPE_INTEGER, x + y, 6);
}

MU_TEST(Arithmetic, subtract)
{
	MU_ASSERT_EQUAL(MU_TYPE_INTEGER, x - y, -1);
}

MU_TEST(Arithmetic, multiply)
{
	MU_ASSERT_EQUAL(MU_TYPE_INTEGER, x * y, 6);
}

MU_TEST(Arithmetic, divide)
{
	MU_ASSERT_EQUAL(MU_TYPE_INTEGER, y / x, 1);
}

MU_TEST(Arithmetic, skip)
{
    MU_SKIP("This test is skipped");
}

MU_TEST(Arithmetic, bad)
{
    MU_EXPECT(MU_STATUS_ASSERTION);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, x + y, 4);	
}

MU_TEST(Arithmetic, bad2)
{
    MU_EXPECT(MU_STATUS_ASSERTION);

    MU_ASSERT(x > y);
}

MU_TEST(Arithmetic, crash)
{
    MU_EXPECT(MU_STATUS_CRASH);
    
    MU_ASSERT(x / (y - 3) == x);
}

/*
 * The following tests demonstrate various ways
 * to crash or otherwise fail
 */

/* 
 * To show that failures can occur in setup/teardown
 * routines as well, this teardown routine crashes
 * on purpose during one of the tests.
 */
MU_FIXTURE_TEARDOWN(Crash)
{
	if (!strcmp(Mu_Test_Name(MU_CURRENT_TEST), "segfault_teardown"))
		*(int*)0 = 42;
}

MU_TEST(Crash, segfault)
{
    MU_EXPECT(MU_STATUS_CRASH);
    
    *(int*)0 = 42;
}

MU_TEST(Crash, segfault_teardown)
{
    MU_EXPECT(MU_STATUS_CRASH);
    /* The teardown function will crash during this test (see above) */
}

MU_TEST(Crash, pipe)
{
    int fd[2];
    
    MU_EXPECT(MU_STATUS_CRASH);
    
    if (pipe(fd))
        MU_FAILURE("pipe(): %s", strerror(errno));
    close(fd[0]);
    if (write(fd[1], fd, sizeof(int)*2) < 0)
        MU_FAILURE("write(): %s", strerror(errno));
}

MU_TEST(Crash, abort)
{
    MU_EXPECT(MU_STATUS_CRASH);
    abort();
}

MU_TEST(Crash, timeout)
{
    MU_EXPECT(MU_STATUS_TIMEOUT);
    /* The harness will assume this test failed after 100 ms */
    MU_TIMEOUT(100);

    /* Go into an infinite wait */
    while (1)
        pause();
}

MU_TEST(Crash, not_reached)
{
    MU_EXPECT(MU_STATUS_ASSERTION);
    
    MU_ASSERT_NOT_REACHED();
}

/* 
 * The following tests demonstrate the available logging levels
 */

MU_TEST(Log, warning)
{
    MU_WARNING("This is a warning");
}

MU_TEST(Log, info)
{
    MU_INFO("This is informational");
}

MU_TEST(Log, verbose)
{
    MU_VERBOSE("This is verbose output");
}

MU_TEST(Log, debug)
{
    MU_DEBUG("This is debug output");
}

MU_TEST(Log, trace)
{
    MU_TRACE("This is trace output");
}

MU_TEST(Log, resource)
{
    MU_INFO("%s", MU_RESOURCE("info message"));
}

/*
 * This test will show you how the logger plugin arranges
 * events in relation to test results.  The default "console"
 * plugin shows the test result last to maintain chronological 
 * order.
 */
MU_TEST(Log, fail)
{
    MU_EXPECT(MU_STATUS_FAILURE);

    MU_INFO("This test fails");

    MU_FAILURE("I told you so");
}

/*
 * Some utility code to implement a thread barrier for an
 * upcoming test
 */

typedef struct
{
    int needed, waiting;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} barrier_t;

static void
barrier_init(barrier_t* barrier, int needed)
{
    barrier->needed = needed;
    barrier->waiting = 0;
    pthread_mutex_init(&barrier->lock, NULL);
    pthread_cond_init(&barrier->cond, NULL);
}

static int
barrier_wait(barrier_t* barrier)
{
    pthread_mutex_lock(&barrier->lock);

    barrier->waiting++;

    if (barrier->waiting == barrier->needed)
    {
        barrier->waiting = 0;
        pthread_cond_broadcast(&barrier->cond);
        pthread_mutex_unlock(&barrier->lock);
        return 1;
    }
    else
    {
        pthread_cond_wait(&barrier->cond, &barrier->lock);
        pthread_mutex_unlock(&barrier->lock);
        return 0;
    }
}

static barrier_t barrier;


static void*
racer(void* number)
{
    /* All threads begin at the starting line */
    barrier_wait(&barrier);
    /* Test functions/macros are safe to use from multiple threads. */
    MU_INFO("Racer #%lu at the finish line", (unsigned long) number);
    /* Whichever thread reaches MU_SUCCESS first ends the test */
    MU_SUCCESS();
    return NULL;
}

/* 
 * This test demonstrates multithreading and using MU_ITERATE to
 * repeat tests with non-deterministic behavior.
 */
MU_TEST(Thread, race)
{
    pthread_t racer1, racer2;

    /* This test should be run 10 times */
    MU_ITERATE(10);

    /* Set up the thread barrier */
    barrier_init(&barrier, 3);
    
    /* Create two racers or fail */
    if (pthread_create(&racer1, NULL, racer, (void*) 0x1))
        MU_FAILURE("Failed to create thread: %s\n", strerror(errno));
    if (pthread_create(&racer2, NULL, racer, (void*) 0x2))
        MU_FAILURE("Failed to create thread: %s\n", strerror(errno));

    /* Wait at the barrier - when all threads reach it,
     * the racers will be released
     */
       
    barrier_wait(&barrier);

    /* Wait for the racers to finish */
    pthread_join(racer1, NULL);
    pthread_join(racer2, NULL);

    /* We will never get here as one of the two racers will end the test */
    MU_ASSERT_NOT_REACHED();
}

/** \endcond */
