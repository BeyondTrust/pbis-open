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
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"

int
dcethread_cond_timedwait(dcethread_cond *cond, dcethread_mutex *mutex, struct timespec *abstime)
{
    int ret = 0;
    int (*interrupt_old)(dcethread*, void*) = NULL;
    void *data_old = NULL;
    condwait_info info;

    info.cond = cond;
    info.mutex = mutex;

    do
    {
        if (dcethread__begin_block(dcethread__self(), dcethread__interrupt_condwait, &info, &interrupt_old, &data_old))
        {
            dcethread__dispatchinterrupt(dcethread__self());
            return dcethread__set_errno(EINTR);
        }
        mutex->owner = (pthread_t) -1;
	ret = pthread_cond_timedwait(cond, (pthread_mutex_t*) &mutex->mutex, abstime);
        mutex->owner = pthread_self();
        if (dcethread__end_block(dcethread__self(), interrupt_old, data_old))
        {
            dcethread__dispatchinterrupt(dcethread__self());
            return dcethread__set_errno(EINTR);
        }
    } while (ret == EINTR);


    return dcethread__set_errno(ret);
}

int
dcethread_cond_timedwait_throw(dcethread_cond *cond, dcethread_mutex *mutex, struct timespec *abstime)
{
    int ret = dcethread_cond_timedwait(cond, mutex, abstime);

    if (ret < 0 && errno == ETIMEDOUT)
	return -1;
    else if (ret < 0)
    {
        dcethread__exc_raise(dcethread__exc_from_errno(errno), __FILE__, __LINE__);
    }
    else
    {
        return ret;
    }
}
