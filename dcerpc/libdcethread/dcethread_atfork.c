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

/* Work around AIX WEXITSTATUS macro */
#if defined(_AIX) && defined(_ALL_SOURCE)
#undef _ALL_SOURCE
#endif

/* Work around HP-UX WEXITSTATUS macro */
#if defined(__hpux__) && defined(_BSD)
#undef _BSD
#endif

#include <errno.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"

#ifdef API

#define ATFORK_MAX_HANDLERS 256

/* pthread fork handling wrapper
 *
 * Using fork() in a process with multiple threads can be tricky because only
 * the calling thread is duplicated into the child process, leaving things in
 * a potentially inconsistent state. pthread_atfork() allows callbacks to be run
 * before and after a fork() in order to perform cleanup, but the version in
 * the final POSIX spec does not provide a user data pointer to the callbacks
 * (which the draft spec presumably did).  This wrapper provides this capability
 * by multiplexing multiple handlers through a single handler with the underlying
 * pthreads implementation.
 *
 * All related functions hold an exclusion lock for the duration of their execution.
 * This addresses two potential issues:
 *     - Concurrent modification of the atfork_handlers array
 *     - Handlers (such as in comfork.c) which are not tolerant of concurrent fork()
 *       calls
 * Given this, performing a fork() or registering a fork handler from a fork handler
 * would be a Bad Idea.
 */


/* !!! HACK !!!
 *
 * Certain versions of SPARC Solaris 10 have a regression in the behavior of pthread_atfork()
 * that can cause freezes and other bizarre behavior.  Therefore, we do not use it
 * on that platform.
 *
 * The bug in question is Solaris bug 6570016.  It is fixed by patch 127111-03 or Solaris 10 u5.
 *
 * This hack means that DCE/RPC applications must always call exec() soon after fork(),
 * or use dcethread_fork() instead.
 */
#if defined (sun) && defined(sparc)
#  define AVOID_PTHREAD_ATFORK
#endif

typedef struct
{
    void *user_state;
    void (*pre_fork)(void *);
    void (*parent_fork)(void *);
    void (*child_fork)(void *);
} dcethread_atfork_handler;

/* Initilization once control */
static pthread_once_t atfork_once = DCETHREAD_ONCE_INIT;
/* Exclusion lock -- prevents modification of handler list while fork()s are active */
static pthread_rwlock_t atfork_lock;
/* Array of handlers */
static volatile dcethread_atfork_handler atfork_handlers[ATFORK_MAX_HANDLERS];
/* Current size of the array */
static volatile ssize_t atfork_handlers_len = 0;

/* Proxy callbacks which multiplex calls to all registered handlers */
static void
__dcethread_pre_fork(void)
{
    unsigned int i;
    
    pthread_rwlock_rdlock(&atfork_lock);
    
    for (i = 0; i < atfork_handlers_len; i++)
    {
	if (atfork_handlers[i].pre_fork)
	    atfork_handlers[i].pre_fork(atfork_handlers[i].user_state);
    }
    
    pthread_rwlock_unlock(&atfork_lock);
}

static void
__dcethread_parent_fork(void)
{
    unsigned int i;

    pthread_rwlock_rdlock(&atfork_lock);
    
    for (i = 0; i < atfork_handlers_len; i++)
    {
	if (atfork_handlers[i].parent_fork)
	    atfork_handlers[i].parent_fork(atfork_handlers[i].user_state);
    }
    
    pthread_rwlock_unlock(&atfork_lock);
}

static void
__dcethread_child_fork(void)
{
    unsigned int i;

    pthread_rwlock_rdlock(&atfork_lock);
    
    for (i = 0; i < atfork_handlers_len; i++)
    {
	if (atfork_handlers[i].child_fork)
	    atfork_handlers[i].child_fork(atfork_handlers[i].user_state);
    }
    
    pthread_rwlock_unlock(&atfork_lock);
}

/* Registration function to add a new handler */

static
void
__dcethread_atfork_init(void)
{
    if (pthread_rwlock_init(&atfork_lock, NULL) != 0)
    {
        abort();
    }
}

static
void
dcethread_atfork_init()
{
    pthread_once(&atfork_once, __dcethread_atfork_init);
}

int
dcethread_atfork(void *user_state, void (*pre_fork)(void *), void (*parent_fork)(void *), void (*child_fork)(void *))
{
    dcethread_atfork_handler handler;
    
    dcethread_atfork_init();

    pthread_rwlock_wrlock(&atfork_lock);
    
    if (atfork_handlers_len >= ATFORK_MAX_HANDLERS)
    {
	pthread_rwlock_unlock(&atfork_lock);
	return dcethread__set_errno(ENOMEM);
    }
    
    /* Fill in struct */
    handler.user_state = user_state;
    handler.pre_fork = pre_fork;
    handler.child_fork = child_fork;
    handler.parent_fork = parent_fork;
    
#ifndef AVOID_PTHREAD_ATFORK
    /* If no handlers have been registered yet, register our proxy functions exactly once with the
       real pthread_atfork */
    if (atfork_handlers_len == 0)
    {
	if (dcethread__set_errno(pthread_atfork(__dcethread_pre_fork, __dcethread_parent_fork, __dcethread_child_fork)))
	{
	    pthread_rwlock_unlock(&atfork_lock);
	    return -1;
	}
    }
#endif    

    /* Add handler to array */
    atfork_handlers[atfork_handlers_len++] = handler;
	
    pthread_rwlock_unlock(&atfork_lock);
    
    return dcethread__set_errno(0);
}

int
dcethread_atfork_throw(void *user_state, void (*pre_fork)(void *), void (*parent_fork)(void *), void (*child_fork)(void *))
{
    DCETHREAD_WRAP_THROW(dcethread_atfork(user_state, pre_fork, parent_fork, child_fork));
}

pid_t
dcethread_fork(void)
{
    pid_t pid = -1;

    dcethread_atfork_init();

#ifdef AVOID_PTHREAD_ATFORK
    __dcethread_pre_fork();
#endif

    pid = fork();

#ifdef AVOID_PTHREAD_ATFORK
    if (pid == 0)
    {
        __dcethread_child_fork();
    }
    else if (pid != -1)
    {
        __dcethread_parent_fork();
    }
#endif

    return pid;
}

#endif /* API */

#ifdef TEST

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "dcethread-test.h"

struct called_s
{
    int pre, parent, child;
};

static void
pre_handler(void *_data)
{
    MU_TRACE("Fork pre handler active in thread %p", dcethread_self());
    ((struct called_s*) _data)->pre = 1;
} 

static void
parent_handler(void *_data)
{
    MU_TRACE("Fork parent handler active in thread %p", dcethread_self());
    ((struct called_s*) _data)->parent = 1;
}

static void
child_handler(void *_data)
{
    ((struct called_s*) _data)->child = 1;
}  


MU_TEST(dcethread_atfork, basic)
{
    struct called_s called = {0,0,0};
    uid_t child;

    MU_TRY_DCETHREAD( dcethread_atfork(&called, pre_handler, parent_handler, child_handler) );

    if ((child = dcethread_fork()))
    {
	if (child == -1)
	{
	    MU_FAILURE("fork() failed: %s", strerror(errno));
	}

	if (waitpid(child, &called.child, 0) != child)
	{
	    MU_FAILURE("waitpid() failed: %s", strerror(errno));
	}

        called.child = WEXITSTATUS(called.child);

	MU_ASSERT(called.pre);
	MU_ASSERT(called.parent);
	MU_ASSERT(called.child);
    }
    else
    {
	exit(called.child);
    }
}

#endif /* TEST */
