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

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"

#ifdef API

int
dcethread_cond_wait(dcethread_cond *cond, dcethread_mutex *mutex)
{
    int ret = 0;
    int (*interrupt_old)(dcethread*, void*) = NULL;
    void *data_old = NULL;
    condwait_info info;

    info.cond = cond;
    info.mutex = mutex;

    if (dcethread__begin_block(dcethread__self(), dcethread__interrupt_condwait, &info, &interrupt_old, &data_old))
    {
        dcethread__dispatchinterrupt(dcethread__self());
        return dcethread__set_errno(EINTR);
    }
    mutex->owner = (pthread_t) -1;
    ret = dcethread__set_errno(pthread_cond_wait(cond, (pthread_mutex_t*) &mutex->mutex));
    mutex->owner = pthread_self();
    if (dcethread__end_block(dcethread__self(), interrupt_old, data_old))
    {
        dcethread__dispatchinterrupt(dcethread__self());
        return dcethread__set_errno(EINTR);
    }

    return dcethread__set_errno(ret);
}

int
dcethread_cond_wait_throw(dcethread_cond *cond, dcethread_mutex *mutex)
{
    DCETHREAD_WRAP_THROW(dcethread_cond_wait(cond, mutex));
}

#endif /* API */

#ifdef TEST

#include "dcethread-test.h"

static void*
basic_thread(void* data)
{
    volatile int interrupt_caught = 0;
    dcethread_cond cond;
    dcethread_mutex mutex;

    MU_TRY_DCETHREAD( dcethread_mutex_init(&mutex, NULL) );
    MU_TRY_DCETHREAD( dcethread_cond_init(&cond, NULL) );

    DCETHREAD_TRY
    {
	MU_ASSERT(!interrupt_caught);
        MU_TRY_DCETHREAD( dcethread_mutex_lock (&mutex) );

	while (1)
	{
            MU_TRY_DCETHREAD( dcethread_cond_wait (&cond, &mutex) );
	}
    }
    DCETHREAD_CATCH(dcethread_interrupt_e)
    {
	MU_ASSERT(!interrupt_caught);
	interrupt_caught = 1;
    }
    DCETHREAD_FINALLY
    {
        dcethread_mutex_unlock (&mutex);
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(interrupt_caught);

    return NULL;
}

MU_TEST(dcethread_cond_wait, interrupt_pre)
{
    dcethread* thread;

    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, basic_thread, NULL) );
    MU_TRY_DCETHREAD( dcethread_interrupt(thread) );
    MU_TRY_DCETHREAD( dcethread_join(thread, NULL) );
}

MU_TEST(dcethread_cond_wait, interrupt_post)
{
    dcethread* thread;
    struct timespec ts;

    ts.tv_nsec = 100000000;
    ts.tv_sec = 0;
    
    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, basic_thread, NULL) );
    MU_TRY_DCETHREAD( dcethread_delay(&ts) );
    MU_TRY_DCETHREAD( dcethread_interrupt(thread) );
    MU_TRY_DCETHREAD( dcethread_join(thread, NULL) );
}

dcethread_mutex global_mutex = DCETHREAD_MUTEX_INITIALIZER;

static void*
global_lock_thread(void* data)
{
    volatile int interrupt_caught = 0;
    dcethread_cond cond;

    MU_TRY_DCETHREAD( dcethread_cond_init(&cond, NULL) );

    DCETHREAD_TRY
    {
	MU_ASSERT(!interrupt_caught);
        MU_TRY_DCETHREAD( dcethread_mutex_lock (&global_mutex) );

	while (1)
	{
            MU_TRY_DCETHREAD( dcethread_cond_wait (&cond, &global_mutex) );
	}
    }
    DCETHREAD_CATCH(dcethread_interrupt_e)
    {
	MU_ASSERT(!interrupt_caught);
	interrupt_caught = 1;
    }
    DCETHREAD_FINALLY
    {
        dcethread_mutex_unlock (&global_mutex);
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(interrupt_caught);

    return NULL;    
}

MU_TEST(dcethread_cond_wait, interrupt_global)
{
    dcethread* thread;
    struct timespec ts;

    ts.tv_nsec = 100000000;
    ts.tv_sec = 0;

    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, global_lock_thread, NULL) );
    MU_TRY_DCETHREAD( dcethread_delay(&ts) );
    MU_TRY_DCETHREAD( dcethread_mutex_lock(&global_mutex) );
    MU_TRY_DCETHREAD( dcethread_interrupt(thread) );
    MU_TRY_DCETHREAD( dcethread_mutex_unlock(&global_mutex) );
    MU_TRY_DCETHREAD( dcethread_join(thread, NULL) );    
}

#endif /* TEST */
