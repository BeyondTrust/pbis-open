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

typedef struct
{
    void* (*start) (void* data);
    void* data;
    dcethread* self;
} dcethread_start_args;

static void *
proxy_start(void *arg)
{
    dcethread_start_args *args = (dcethread_start_args*) arg;
    void *result;
    int prev_cancel_state;
    int prev_cancel_type;
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &prev_cancel_state);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &prev_cancel_type);
    dcethread__init_self(args->self);
    
    result = args->start(args->data);

    (void) pthread_setcancelstate(prev_cancel_state, NULL);
    (void) pthread_setcanceltype(prev_cancel_type, NULL);
    
    dcethread__lock(args->self);
    args->self->status = result;
    dcethread__cleanup_self(args->self);   
    dcethread__unlock(args->self);

    free(args);

    return result;
}

int
dcethread_create(dcethread** _thread, dcethread_attr* attr, void *(*start_routine)(void *), void *arg)
{
    dcethread_start_args *start_args;
    dcethread* thread;
    int detachstate;
    
    start_args = (dcethread_start_args *) malloc(sizeof(*start_args));
    if (start_args == NULL) 
    {
	return dcethread__set_errno(ENOMEM);
    }
    
    start_args->start = start_routine;
    start_args->data = arg;
    start_args->self = thread = dcethread__new();
    
    /* Record if this thread was created joinably */
    if (!attr || (pthread_attr_getdetachstate(attr, &detachstate), detachstate == PTHREAD_CREATE_JOINABLE))
    {
	thread->flag.joinable = 1;
    }

    /* If thread is joinable, give it an extra reference */
    if (thread->flag.joinable)
    {
	thread->refs++;
    }

    if (dcethread__set_errno(pthread_create((pthread_t*) &thread->pthread, attr, proxy_start, start_args)))
    {
	dcethread__delete(thread);
	free(start_args);
	return -1;
    }

    DCETHREAD_TRACE("Thread %p: created (pthread %lu)", thread, (unsigned long) thread->pthread);

    dcethread__lock(thread);
    while (thread->state == DCETHREAD_STATE_CREATED)
    {
	dcethread__wait(thread);
    }
    dcethread__unlock(thread);
    
    DCETHREAD_TRACE("Thread %p: started", thread);

    *_thread = thread;
    return dcethread__set_errno(0);
}

int
dcethread_create_throw(dcethread** _thread, dcethread_attr* attr, void *(*start_routine)(void *), void *arg)
{
    DCETHREAD_WRAP_THROW(dcethread_create(_thread, attr, start_routine, arg));
}

#endif /* API */

#ifdef TEST

#include "dcethread-test.h"

static void* volatile basic_result = 0;

static void*
basic(void* data)
{
    basic_result = data;

    return data;
}

MU_TEST(dcethread_create, basic)
{
    dcethread* thread = NULL;

    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, basic, (void*) 0xDEADBEEF) );

    MU_ASSERT(thread != NULL);

    while (basic_result != (void*) 0xDEADBEEF)
    {
	dcethread_yield();
    }
}

#endif /* TEST */
