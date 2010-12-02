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
/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 * 
 */

#ifndef __DCETHREAD_H__
#define __DCETHREAD_H__

#include <dce/ndrtypes.h>
/* Unfortunately, pthreads uses a lot of macros
   and static initializers which can't easily be
   abstracted away */
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
/* FIXME: this is kind of dirty */
#ifdef __FreeBSD__
#include <sys/select.h>
#endif

#if defined(__GNUC__) && (__GNUC__ >= 3)
#    define __DCETHREAD_UNUSED__ __attribute__((unused))
#    define __DCETHREAD_NORETURN__ __attribute__((noreturn))
#else
#    define __DCETHREAD_UNUSED__
#    define __DCETHREAD_NORETURN__
#endif

/* Opaque thread type */
typedef volatile struct _dcethread dcethread;

/* We need to be able to track the
   owner of a mutex */
typedef volatile struct
{
    pthread_mutex_t mutex;
    pthread_t owner;
} dcethread_mutex;

/* Alias pthread types */
typedef pthread_cond_t dcethread_cond;
typedef pthread_attr_t dcethread_attr;
typedef pthread_mutexattr_t dcethread_mutexattr;
typedef pthread_condattr_t dcethread_condattr;
typedef pthread_once_t dcethread_oncectl;
typedef pthread_key_t dcethread_key;
typedef void* dcethread_addr;
typedef void (*dcethread_initroutine)(void);
typedef void* (*dcethread_startroutine)(void*);

/* Solaris and AIX have a broken PTHREAD_ONCE_INIT macro, so wrap it
   appropriately depending on platform */

#if __LW_BROKEN_ONCE_INIT
#  define DCETHREAD_ONCE_INIT {PTHREAD_ONCE_INIT}
#else
#  define DCETHREAD_ONCE_INIT PTHREAD_ONCE_INIT
#endif
#define DCETHREAD_MUTEX_INITIALIZER {PTHREAD_MUTEX_INITIALIZER, (pthread_t) -1}
#if __LW_BROKEN_ONCE_INIT &&  !defined(_AIX)
#  define DCETHREAD_COND_INITIALIZER {{{0}, 0, 0}, 0}
#else
#  define DCETHREAD_COND_INITIALIZER PTHREAD_COND_INITIALIZER
#endif

/* Entry points */
int dcethread_get_expiration(struct timespec* delta, struct timespec* abstime);

int dcethread_delay(struct timespec const* interval);

void dcethread_lock_global(void);

void dcethread_unlock_global(void);

int dcethread_ismultithreaded(void);

int dcethread_mutexattr_getkind(dcethread_mutexattr *attr);

int dcethread_mutexattr_setkind(dcethread_mutexattr *attr, int kind);

void dcethread_signal_to_interrupt(sigset_t *sigset, dcethread* thread);

int dcethread_attr_create(dcethread_attr *attr);
int dcethread_attr_create_throw(dcethread_attr *attr);

int dcethread_attr_delete(dcethread_attr *attr);
int dcethread_attr_delete_throw(dcethread_attr *attr);

int dcethread_attr_setprio(dcethread_attr *attr, int priority);
int  dcethread_attr_setprio_throw(dcethread_attr *attr, int priority);

int dcethread_attr_getprio(dcethread_attr *attr);
int dcethread_attr_getprio_throw(dcethread_attr *attr);

int dcethread_attr_setsched(dcethread_attr *attr, int sched);
int dcethread_attr_setsched_throw(dcethread_attr *attr, int sched);

int dcethread_attr_getsched(dcethread_attr *attr);
int dcethread_attr_getsched_throw(dcethread_attr* attr);

int dcethread_attr_setinheritsched(dcethread_attr *attr, int inherit);
int dcethread_attr_setinheritsched_throw(dcethread_attr *attr, int inherit);

int dcethread_attr_getinheritsched(dcethread_attr *attr);
int dcethread_attr_getinheritsched_throw(dcethread_attr *attr);

int dcethread_attr_setstacksize(dcethread_attr *attr, long stacksize);
int dcethread_attr_setstacksize_throw(dcethread_attr *attr, long stacksize);

long dcethread_attr_getstacksize(dcethread_attr *attr);
long dcethread_attr_getstacksize_throw(dcethread_attr* attr);

int dcethread_create(dcethread** _thread, dcethread_attr* attr, void *(*start_routine)(void *), void *arg);
int dcethread_create_throw(dcethread** _thread, dcethread_attr* attr, void *(*start_routine)(void *), void *arg);

int dcethread_detach(dcethread *thread);
int dcethread_detach_throw(dcethread *thread);

int dcethread_join(dcethread* thread, void **status);
int dcethread_join_throw(dcethread* thread, void **status);

int dcethread_setprio(dcethread* thread, int priority);
int dcethread_setprio_throw(dcethread* thread, int priority);

int dcethread_getprio(dcethread* thread);
int dcethread_getprio_throw(dcethread* thread);

int dcethread_mutexattr_create(dcethread_mutexattr *attr);
int dcethread_mutexattr_create_throw(dcethread_mutexattr *attr);

int dcethread_mutexattr_delete(dcethread_mutexattr *attr);
int dcethread_mutexattr_delete_throw(dcethread_mutexattr *attr);

int dcethread_mutex_init(dcethread_mutex *mutex, dcethread_mutexattr* attr);
int dcethread_mutex_init_throw(dcethread_mutex *mutex, dcethread_mutexattr* attr);

int dcethread_mutex_destroy(dcethread_mutex *mutex);
int dcethread_mutex_destroy_throw(dcethread_mutex *mutex);

int dcethread_mutex_lock(dcethread_mutex *mutex);
int dcethread_mutex_lock_throw(dcethread_mutex *mutex);

int dcethread_mutex_unlock(dcethread_mutex *mutex);
int dcethread_mutex_unlock_throw(dcethread_mutex *mutex);

int dcethread_mutex_trylock(dcethread_mutex *mutex);
int dcethread_mutex_trylock_throw(dcethread_mutex *mutex);

int dcethread_condattr_create(dcethread_condattr *attr);
int dcethread_condattr_create_throw(dcethread_condattr *attr);

int dcethread_condattr_delete(dcethread_condattr *attr);
int dcethread_condattr_delete_throw(dcethread_condattr *attr);

int dcethread_cond_init(dcethread_cond *cond, dcethread_condattr* attr);
int dcethread_cond_init_throw(dcethread_cond *cond, dcethread_condattr* attr);

int dcethread_cond_destroy(dcethread_cond *cond);
int dcethread_cond_destroy_throw(dcethread_cond *cond);

int dcethread_cond_broadcast(dcethread_cond *cond);
int dcethread_cond_broadcast_throw(dcethread_cond *cond);

int dcethread_cond_signal(dcethread_cond *cond);
int dcethread_cond_signal_throw(dcethread_cond *cond);

int dcethread_cond_wait(dcethread_cond *cond, dcethread_mutex *mutex);
int dcethread_cond_wait_throw(dcethread_cond *cond, dcethread_mutex *mutex);

int dcethread_cond_timedwait(dcethread_cond *cond, dcethread_mutex *mutex, struct timespec *abstime);
int dcethread_cond_timedwait_throw(dcethread_cond *cond, dcethread_mutex *mutex, struct timespec *abstime);

int dcethread_once(dcethread_oncectl *once_block, void (*init_routine)(void));
int dcethread_once_throw(dcethread_oncectl *once_block, void (*init_routine)(void));

int dcethread_keycreate(dcethread_key *key, void (*destructor)(void *value));
int dcethread_keycreate_throw(dcethread_key *key, void (*destructor)(void *value));

int dcethread_setspecific(dcethread_key key, void *value);
int dcethread_setspecific_throw(dcethread_key key, void *value);

int dcethread_getspecific(dcethread_key key, void **value);
int dcethread_getspecific_throw(dcethread_key key, void **value);

int dcethread_interrupt(dcethread* thread);
int dcethread_interrupt_throw(dcethread* thread);

int dcethread_enableasync(int on);
int dcethread_enableasync_throw(int on);

int dcethread_enableinterrupt(int on);
int dcethread_enableinterrupt_throw(int on);

int dcethread_kill(dcethread* thread, int sig);
int dcethread_kill_throw(dcethread* thread, int sig);

void dcethread_yield(void);

void dcethread_checkinterrupt(void);

int dcethread_equal(dcethread* t1, dcethread* t2);

dcethread* dcethread_self(void);

__DCETHREAD_NORETURN__ void dcethread_exit(void* status);

int dcethread_atfork(void *user_state, void (*pre_fork)(void *), void (*parent_fork)(void *), void (*child_fork)(void *));
int dcethread_atfork_throw(void *user_state, void (*pre_fork)(void *), void (*parent_fork)(void *), void (*child_fork)(void *));

/* Exceptions */

typedef struct _dcethread_exc
{
    enum
    {
	DCETHREAD_EXC_KIND_ADDRESS = 0x02130455,
	DCETHREAD_EXC_KIND_STATUS = 0x02130456
    } kind;
    union
    {
        int value;
        struct _dcethread_exc *address;       
    } match;
    const char* name;
} dcethread_exc;

void dcethread_exc_init(dcethread_exc* exc, const char* name);
void dcethread_exc_setstatus(dcethread_exc* exc, int value);
int dcethread_exc_getstatus(dcethread_exc* exc);
int dcethread_exc_matches(dcethread_exc* exc, dcethread_exc* pattern);
__DCETHREAD_NORETURN__ void dcethread_exc_raise(dcethread_exc* exc, const char* file, unsigned int line);

typedef volatile struct _dcethread_frame
{
    dcethread_exc exc;
    const char* file;
    unsigned int line;
    sigjmp_buf jmpbuf;
    volatile struct _dcethread_frame *parent;
} dcethread_frame;

void dcethread_frame_push(dcethread_frame* frame);
void dcethread_frame_pop(dcethread_frame* frame);

#define DCETHREAD_TRY							\
    do									\
    {									\
        /* handler frame */						\
        __DCETHREAD_UNUSED__ dcethread_frame __dcethread_frame;         \
	/* has a catch clause been run this frame? */			\
	__DCETHREAD_UNUSED__ volatile char __dcethread_handled = 0;     \
	/* has a finally clause been run this frame? */			\
	__DCETHREAD_UNUSED__ volatile char __dcethread_finally = 0;     \
	/* is a live (unhandled) exception in play? */			\
	__DCETHREAD_UNUSED__ volatile int __dcethread_live;             \
									\
	dcethread_frame_push(&__dcethread_frame);			\
	__dcethread_live = sigsetjmp(((struct _dcethread_frame*) &__dcethread_frame)->jmpbuf, 1); \
									\
									\
	if (!__dcethread_live)						\
	{								\
	    /* Try block code goes here */				

#define DCETHREAD_CATCH(_exc)						 \
        }								 \
	else if (!__dcethread_handled &&				 \
	         !__dcethread_finally &&				\
		 dcethread_exc_matches((dcethread_exc*) &__dcethread_frame.exc, &(_exc))) \
	{								\
	    __dcethread_handled = 1;					\
	    __dcethread_live = 0;					\
	    /* exception code here */

#define DCETHREAD_CATCH_EXPR(expr)			\
        }						\
	else if (!__dcethread_handled &&		\
		 !__dcethread_finally && (expr))	\
	{						\
	    __dcethread_handled = 1;			\
	    __dcethread_live = 0;			\
	    /* exception code here */

#define DCETHREAD_CATCH_ALL(_exc)			       \
        }						       \
	else if (!__dcethread_handled &&		       \
		 !__dcethread_finally)			       \
	{						       \
            __DCETHREAD_UNUSED__ dcethread_exc* _exc = (dcethread_exc*) &__dcethread_frame.exc; \
	    __dcethread_handled = 1;			       \
	    __dcethread_live = 0;			       \
        /* exception code here */

#define DCETHREAD_FINALLY			\
        }					\
	if (!__dcethread_finally)		\
        {					\
	     __dcethread_finally = 1;		\
	     /* user finally code here */

#define DCETHREAD_ENDTRY						\
        }								\
        dcethread_frame_pop(&__dcethread_frame);			\
	if (__dcethread_live)						\
	{								\
	    DCETHREAD_RERAISE;                                          \
	}								\
    } while (0);							\

#define DCETHREAD_RAISE(e) (dcethread_exc_raise(&(e), __FILE__, __LINE__))

#define DCETHREAD_RERAISE (dcethread_exc_raise((dcethread_exc*) &__dcethread_frame.exc, \
                                               (const char*) __dcethread_frame.file, \
                                               (unsigned int) __dcethread_frame.line))

#define DCETHREAD_EXC_CURRENT ((dcethread_exc*) &__dcethread_frame.exc)

#define DCETHREAD_EXC_INIT(e) (dcethread_exc_init(&(e), #e))

/* Standard exceptions */

extern dcethread_exc dcethread_uninitexc_e;       /* Uninitialized exception */

/*
 * The following exceptions are common error conditions which may be raised
 * by the thread library or any other facility following this exception
 * specification.
 */

extern dcethread_exc dcethread_exquota_e;         /* Exceeded quota */
extern dcethread_exc dcethread_insfmem_e;         /* Insufficient memory */
extern dcethread_exc dcethread_nopriv_e;          /* No privilege */

/*
 * The following exceptions describe hardware or operating system error
 * conditions that are appropriate for most hardware and operating system
 * platforms. These are raised by the exception facility to report operating
 * system and hardware error conditions.
 */

extern dcethread_exc dcethread_illaddr_e;         /* Illegal address */
extern dcethread_exc dcethread_illinstr_e;        /* Illegal instruction */
extern dcethread_exc dcethread_resaddr_e;         /* Reserved addressing mode */
extern dcethread_exc dcethread_privinst_e;        /* Privileged instruction */
extern dcethread_exc dcethread_resoper_e;         /* Reserved operand */
extern dcethread_exc dcethread_aritherr_e;        /* Arithmetic error */
extern dcethread_exc dcethread_intovf_e;          /* Integer overflow */
extern dcethread_exc dcethread_intdiv_e;          /* Integer divide by zero */
extern dcethread_exc dcethread_fltovf_e;          /* Floating overflow */
extern dcethread_exc dcethread_fltdiv_e;          /* Floating divide by zero */
extern dcethread_exc dcethread_fltund_e;          /* Floating underflow */
extern dcethread_exc dcethread_decovf_e;          /* Decimal overflow */
extern dcethread_exc dcethread_subrng_e;          /* Subrange */
extern dcethread_exc dcethread_excpu_e;           /* Exceeded CPU quota */
extern dcethread_exc dcethread_exfilsiz_e;        /* Exceeded file size */

/*
 * The following exceptions correspond directly to UNIX synchronous
 * terminating signals.  This is distinct from the prior list in that those
 * are generic and likely to have equivalents on most any operating system,
 * whereas these are highly specific to UNIX platforms.
 */

extern dcethread_exc dcethread_SIGTRAP_e;         /* SIGTRAP received */
extern dcethread_exc dcethread_SIGIOT_e;          /* SIGIOT received */
extern dcethread_exc dcethread_SIGEMT_e;          /* SIGEMT received */
extern dcethread_exc dcethread_SIGSYS_e;          /* SIGSYS received */
extern dcethread_exc dcethread_SIGPIPE_e;         /* SIGPIPE received */
extern dcethread_exc dcethread_unksyncsig_e;      /* Unknown synchronous signal */

/*
 * The following exception is raised in the target of an interruption
 */

extern dcethread_exc dcethread_interrupt_e;
extern dcethread_exc dcethread_badparam_e;           /* Bad parameter */
extern dcethread_exc dcethread_existence_e;          /* Object does not exist */
extern dcethread_exc dcethread_in_use_e;             /* Object is in use */
extern dcethread_exc dcethread_use_error_e;          /* Object inappropriate for operation */
extern dcethread_exc dcethread_nostackmem_e;         /* No memory to allocate stack */
extern dcethread_exc dcethread_exit_thread_e;        /* Used to terminate a thread */

/*
 * Interruptible system calls, etc
 */

int dcethread_pause();
ssize_t dcethread_read(int fd, void *buf, size_t count);
ssize_t dcethread_write(int fd, void *buf, size_t count);
ssize_t dcethread_send(int s, const void *buf, size_t len, int flags);
ssize_t dcethread_sendto(int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t dcethread_sendmsg(int s, const struct msghdr *msg, int flags);
ssize_t dcethread_recv(int s, void *buf, size_t len, int flags);
ssize_t dcethread_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
ssize_t dcethread_recvmsg(int s, struct msghdr *msg, int flags);
int dcethread_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

pid_t dcethread_fork(void);

#ifdef DCETHREAD_ENFORCE_API
#ifdef DCETHREAD_USE_THROW
#define dcethread_attr_create ($ ERROR $)
#define dcethread_attr_delete ($ ERROR $)
#define dcethread_attr_setprio ($ ERROR $)
#define dcethread_attr_getprio ($ ERROR $)
#define dcethread_attr_setsched ($ ERROR $)
#define dcethread_attr_getsched ($ ERROR $)
#define dcethread_attr_setinheritsched ($ ERROR $)
#define dcethread_attr_getinheritsched ($ ERROR $)
#define dcethread_attr_setstacksize ($ ERROR $)
#define dcethread_attr_getstacksize ($ ERROR $)
#define dcethread_create ($ ERROR $)
#define dcethread_detach ($ ERROR $)
#define dcethread_join ($ ERROR $)
#define dcethread_setprio ($ ERROR $)
#define dcethread_getprio ($ ERROR $)
#define dcethread_mutexattr_create ($ ERROR $)
#define dcethread_mutexattr_delete ($ ERROR $)
#define dcethread_mutex_init ($ ERROR $)
#define dcethread_mutex_destroy ($ ERROR $)
#define dcethread_mutex_lock ($ ERROR $)
#define dcethread_mutex_unlock ($ ERROR $)
#define dcethread_mutex_trylock ($ ERROR $)
#define dcethread_condattr_create ($ ERROR $)
#define dcethread_condattr_delete ($ ERROR $)
#define dcethread_cond_init ($ ERROR $)
#define dcethread_cond_destroy ($ ERROR $)
#define dcethread_cond_broadcast ($ ERROR $)
#define dcethread_cond_signal ($ ERROR $)
#define dcethread_cond_wait ($ ERROR $)
#define dcethread_cond_timedwait ($ ERROR $)
#define dcethread_once ($ ERROR $)
#define dcethread_keycreate ($ ERROR $)
#define dcethread_setspecific ($ ERROR $)
#define dcethread_getspecific ($ ERROR $)
#define dcethread_interrupt ($ ERROR $)
#define dcethread_enableasync ($ ERROR $)
#define dcethread_enableinterrupt ($ ERROR $)
#else
#define dcethread_attr_create_throw ($ ERROR $)
#define dcethread_attr_delete_throw ($ ERROR $)
#define dcethread_attr_setprio_throw ($ ERROR $)
#define dcethread_attr_getprio_throw ($ ERROR $)
#define dcethread_attr_setsched_throw ($ ERROR $)
#define dcethread_attr_getsched_throw ($ ERROR $)
#define dcethread_attr_setinheritsched_throw ($ ERROR $)
#define dcethread_attr_getinheritsched_throw ($ ERROR $)
#define dcethread_attr_setstacksize_throw ($ ERROR $)
#define dcethread_attr_getstacksize_throw ($ ERROR $)
#define dcethread_create_throw ($ ERROR $)
#define dcethread_detach_throw ($ ERROR $)
#define dcethread_join_throw ($ ERROR $)
#define dcethread_setprio_throw ($ ERROR $)
#define dcethread_getprio_throw ($ ERROR $)
#define dcethread_mutexattr_create_throw ($ ERROR $)
#define dcethread_mutexattr_delete_throw ($ ERROR $)
#define dcethread_mutex_init_throw ($ ERROR $)
#define dcethread_mutex_destroy_throw ($ ERROR $)
#define dcethread_mutex_lock_throw ($ ERROR $)
#define dcethread_mutex_unlock_throw ($ ERROR $)
#define dcethread_mutex_trylock_throw ($ ERROR $)
#define dcethread_condattr_create_throw ($ ERROR $)
#define dcethread_condattr_delete_throw ($ ERROR $)
#define dcethread_cond_init_throw ($ ERROR $)
#define dcethread_cond_destroy_throw ($ ERROR $)
#define dcethread_cond_broadcast_throw ($ ERROR $)
#define dcethread_cond_signal_throw ($ ERROR $)
#define dcethread_cond_wait_throw ($ ERROR $)
#define dcethread_cond_timedwait_throw ($ ERROR $)
#define dcethread_once_throw ($ ERROR $)
#define dcethread_keycreate_throw ($ ERROR $)
#define dcethread_setspecific_throw ($ ERROR $)
#define dcethread_getspecific_throw ($ ERROR $)
#define dcethread_interrupt_throw ($ ERROR $)
#define dcethread_enableasync_throw ($ ERROR $)
#define dcethread_enableinterrupt_throw ($ ERROR $)
#endif
#define pthread_attr_create ($ ERROR $)
#define pthread_attr_delete ($ ERROR $)
#define pthread_attr_setprio ($ ERROR $)
#define pthread_attr_getprio ($ ERROR $)
#define pthread_attr_setsched ($ ERROR $)
#define pthread_attr_getsched ($ ERROR $)
#define pthread_attr_setinheritsched ($ ERROR $)
#define pthread_attr_getinheritsched ($ ERROR $)
#define pthread_attr_setstacksize ($ ERROR $)
#define pthread_attr_getstacksize ($ ERROR $)
#define pthread_create ($ ERROR $)
#define pthread_detach ($ ERROR $)
#define pthread_join ($ ERROR $)
#define pthread_setprio ($ ERROR $)
#define pthread_getprio ($ ERROR $)
#define pthread_mutexattr_create ($ ERROR $)
#define pthread_mutexattr_delete ($ ERROR $)

#ifdef pthread_mutex_init
#undef     pthread_mutex_init
#endif
#define pthread_mutex_init ($ ERROR $)

#ifdef pthread_mutex_destroy
#undef     pthread_mutex_destroy
#endif
#define pthread_mutex_destroy ($ ERROR $)

#ifdef pthread_mutex_lock
#undef     pthread_mutex_lock
#endif
#define pthread_mutex_lock ($ ERROR $)

#ifdef pthread_mutex_unlock
#undef     pthread_mutex_unlock
#endif

#ifdef pthread_mutex_unlock
#undef     pthread_mutex_unlock
#endif
#define pthread_mutex_unlock ($ ERROR $)

#ifdef pthread_mutex_trylock
#undef     pthread_mutex_trylock
#endif
#define pthread_mutex_trylock ($ ERROR $)
#define pthread_condattr_create ($ ERROR $)
#define pthread_condattr_delete ($ ERROR $)

#ifdef pthread_cond_init
#undef     pthread_cond_init
#endif
#define pthread_cond_init ($ ERROR $)

#ifdef pthread_cond_destroy
#undef    pthread_cond_destroy
#endif
#define pthread_cond_destroy ($ ERROR $)

#ifdef pthread_cond_broadcast
#undef    pthread_cond_broadcast
#endif
#define pthread_cond_broadcast ($ ERROR $)

#ifdef pthread_cond_signal
#undef     pthread_cond_signal
#endif
#define pthread_cond_signal ($ ERROR $)

#ifdef pthread_cond_wait
#undef     pthread_cond_wait
#endif
#define pthread_cond_wait ($ ERROR $)

#ifdef pthread_cond_timedwait
#undef    pthread_cond_timedwait
#endif
#define pthread_cond_timedwait ($ ERROR $)

#ifdef pthread_once
#undef     pthread_once
#endif
#define pthread_once ($ ERROR $)
#define pthread_keycreate ($ ERROR $)

#ifdef pthread_setspecific
#undef     pthread_setspecific
#endif
#define pthread_setspecific ($ ERROR $)

#ifdef pthread_getspecific
#undef     pthread_getspecific
#endif
#define pthread_getspecific ($ ERROR $)
#define pthread_cancel ($ ERROR $)
#define pthread_setasynccancel ($ ERROR $)
#define pthread_setcancel ($ ERROR $)
#define pthread_kill ($ ERROR $)

#ifdef pthread_exit
#undef     pthread_exit
#endif
#define pthread_exit ($ ERROR $)
#define DO_NOT_CLOBBER ($ ERROR $)
#define EXCEPTION ($ ERROR $)
#define EXCEPTION_INIT ($ ERROR $)
#define TRY ($ ERROR $)
#define ENDTRY ($ ERROR $)
#define CATCH ($ ERROR $)
#define CATCH_ALL ($ ERROR $)
#define CATCH_EXPR ($ ERROR $)
#define RAISE ($ ERROR $)
#define RERAISE ($ ERROR $)
#define FINALLY ($ ERROR $)
#define pthread_mutex_t ($ ERROR $)
#define pthread_cond_t ($ ERROR $)
#define pthread_attr_t ($ ERROR $)
#define pthread_mutexattr_t ($ ERROR $)
#define pthread_condattr_t ($ ERROR $)
#define pthread_once_t ($ ERROR $)
#define pthread_key_t ($ ERROR $)
#define pthread_addr_t ($ ERROR $)
#define pthread_initroutine_t ($ ERROR $)
#define pthread_startroutine_t ($ ERROR $)
#define pause ($ ERROR $)

#ifdef read
#    undef read
#endif
#define read ($ ERROR $)
#define write ($ ERROR $)
#define send ($ ERROR $)
#ifdef sendto
#    undef sendto
#endif
#define sendto ($ ERROR $)
#ifdef sendmsg
#    undef sendmsg
#endif
#define sendmsg ($ ERROR $)
#define recv ($ ERROR $)
#ifdef recvfrom
#    undef recvfrom
#endif
#define recvfrom ($ ERROR $)
#ifdef recvmsg
#    undef recvmsg
#endif
#define recvmsg ($ ERROR $)
#define select ($ ERROR $)
#endif

#endif
