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

void
dcethread_checkinterrupt(void)
{
    dcethread* thread = dcethread__self();
    int interruptible;
    int state;

    dcethread__lock(thread);
    state = thread->state;
    interruptible = thread->flag.interruptible;

    if (state == DCETHREAD_STATE_INTERRUPT && interruptible)
    {
        dcethread__change_state(thread, DCETHREAD_STATE_ACTIVE);
    }

    dcethread__unlock(thread);

    if (state == DCETHREAD_STATE_INTERRUPT && interruptible)
    {
        dcethread__dispatchinterrupt(thread);
    }

    return;
}

#endif

#ifdef TEST

#include "dcethread-test.h"

/* Test for regression of bug 6935,
   where dcethread_checkinterrupt() does not
   properly clear the interrupted state of the thread */
MU_TEST(dcethread_checkinterrupt, bug_6935)
{
    int interrupted_once = 0;
    int interrupted_twice = 0;

    dcethread_interrupt(dcethread_self());

    DCETHREAD_TRY
    {
        dcethread_checkinterrupt();
    }
    DCETHREAD_CATCH(dcethread_interrupt_e)
    {
        interrupted_once = 1;
    }
    DCETHREAD_ENDTRY;

    DCETHREAD_TRY
    {
        dcethread_checkinterrupt();
    }
    DCETHREAD_CATCH(dcethread_interrupt_e)
    {
        interrupted_twice = 1;
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(interrupted_once && !interrupted_twice);
}

#endif
