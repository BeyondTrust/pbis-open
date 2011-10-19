/*
 * Copyright (c) 2008, Likewise Software, Inc.
 * All rights reserved.
 */

/*
 * Copyright (c) 2007, Novell, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Novell, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>
#include <errno.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"

#ifdef API

int
interrupt_join(dcethread* thread, void* data)
{
    dcethread* other = (dcethread*) data;
    if (!pthread_mutex_trylock((pthread_mutex_t*) &other->lock))
    {
        pthread_cond_broadcast((pthread_cond_t*) &other->state_change);
        pthread_mutex_unlock((pthread_mutex_t*) &other->lock);
        return 1;
    }
    else
    {
        return 0;
    }
}

int
dcethread_join(dcethread* thread, void **status)
{
    int (*old_interrupt)(dcethread*, void*);
    void *old_data;

    if (thread == dcethread__self())
    {
        return dcethread__set_errno(EDEADLK);
    }

    if (!thread->flag.joinable)
    {
        DCETHREAD_WARNING("Joining implicit dcethread %p is ill-advised", thread);
    }

    /* Begin our blocking wait on the other thread */
    if (dcethread__begin_block(dcethread__self(), interrupt_join, (void*) thread, &old_interrupt, &old_data))
    {
        dcethread__dispatchinterrupt(dcethread__self());
        return dcethread__set_errno(EINTR);
    }
    /* Lock the other thread in preparation for waiting on its state condition */
    dcethread__lock(thread);
    /* While the thread is still alive */
    while (thread->state != DCETHREAD_STATE_DEAD)
    {
        /* Wait for state change */
        dcethread__wait(thread);
        /* We need to check if we got interrupted, which involves locking dcethread__self().
           To avoid holding two locks simultaneously (which could result in deadlock),
           unlock the other thread for now */
        dcethread__unlock(thread);
        /* Check if we've been interrupted and end block if we have */
        if (dcethread__poll_end_block(dcethread__self(), old_interrupt, old_data))
        {
            /* Process interrupt */
            dcethread__dispatchinterrupt(dcethread__self());
            return dcethread__set_errno(EINTR);
        }
        /* Re-lock thread to resume state change wait */
        dcethread__lock(thread);
    }

    /* Capture thread result */
    if (status)
        *status = thread->status;
    /* Remove reference */
    dcethread__release(thread);
    /* We're done */
    dcethread__unlock(thread);

    return dcethread__set_errno(0);
}

int
dcethread_join_throw(dcethread* thread, void **status)
{
    DCETHREAD_WRAP_THROW(dcethread_join(thread, status));
}

#endif /* API */

#ifdef TEST

#include "dcethread-test.h"

static void*
basic_thread(void* data)
{
    return data;
}

MU_TEST(dcethread_join, basic)
{
    dcethread* thread;
    void* result;

    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, basic_thread, (void*) 0xDEADBEEF) );
    MU_TRY_DCETHREAD( dcethread_join(thread, &result) );

    MU_ASSERT_EQUAL(MU_TYPE_POINTER, result, (void*) 0xDEADBEEF);
}

static void*
infinite_thread(void* data)
{
    while (1)
    {
        dcethread_pause();
    }

    return NULL;
}

static void*
join_thread(void* data)
{
    dcethread* infinite;
    volatile int interrupted = 0;

    MU_TRY_DCETHREAD( dcethread_create(&infinite, NULL, infinite_thread, NULL) );

    DCETHREAD_TRY
    {
        /* Join will never get anywhere */
        MU_TRY_DCETHREAD( dcethread_join(infinite, NULL) );
    }
    DCETHREAD_CATCH(dcethread_interrupt_e)
    {
        /* Note that we got interrupted */
        interrupted = 1;
        /* Detach and kill thread instead */
        MU_TRY_DCETHREAD( dcethread_interrupt(infinite) );
        MU_TRY_DCETHREAD( dcethread_detach(infinite) );
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(interrupted);

    return NULL;
}

MU_TEST(dcethread_join, interrupt_pre)
{
    dcethread* thread;

    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, join_thread, NULL) );
    MU_TRY_DCETHREAD( dcethread_interrupt(thread) );
    MU_TRY_DCETHREAD( dcethread_join(thread, NULL) );
}

MU_TEST(dcethread_join, interrupt_post)
{
    dcethread* thread;
    struct timespec ts;

    ts.tv_nsec = 100000000;
    ts.tv_sec = 0;
  
    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, join_thread, NULL) );
    MU_TRY_DCETHREAD( dcethread_delay(&ts) );
    MU_TRY_DCETHREAD( dcethread_interrupt(thread) );
    MU_TRY_DCETHREAD( dcethread_join(thread, NULL) );
}

#endif /* TEST */
