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

#include <config.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>
#include <stdio.h>
#ifdef HAVE_EXECINFO_H
#    include <execinfo.h>
#endif

#include "dcethread-exception.h"
#include "dcethread-debug.h"
#include "dcethread-private.h"

static pthread_key_t frame_key;
static void (*uncaught_handler) (dcethread_exc* exc, const char* file, unsigned int line, void* data);
static void* uncaught_handler_data;

dcethread_exc dcethread_uninitexc_e;
dcethread_exc dcethread_exquota_e;
dcethread_exc dcethread_insfmem_e;
dcethread_exc dcethread_nopriv_e;
dcethread_exc dcethread_illaddr_e;
dcethread_exc dcethread_illinstr_e;
dcethread_exc dcethread_resaddr_e;
dcethread_exc dcethread_privinst_e;
dcethread_exc dcethread_resoper_e;
dcethread_exc dcethread_aritherr_e;
dcethread_exc dcethread_intovf_e;
dcethread_exc dcethread_intdiv_e;
dcethread_exc dcethread_fltovf_e;
dcethread_exc dcethread_fltdiv_e;
dcethread_exc dcethread_fltund_e;
dcethread_exc dcethread_decovf_e;
dcethread_exc dcethread_subrng_e;
dcethread_exc dcethread_excpu_e;
dcethread_exc dcethread_exfilsiz_e;
dcethread_exc dcethread_SIGTRAP_e;
dcethread_exc dcethread_SIGIOT_e;
dcethread_exc dcethread_SIGEMT_e;
dcethread_exc dcethread_SIGSYS_e;
dcethread_exc dcethread_SIGPIPE_e;
dcethread_exc dcethread_unksyncsig_e;
dcethread_exc dcethread_interrupt_e;
dcethread_exc dcethread_badparam_e;           /* Bad parameter */
dcethread_exc dcethread_existence_e;          /* Object does not exist */
dcethread_exc dcethread_in_use_e;             /* Object is in use */
dcethread_exc dcethread_use_error_e;          /* Object inappropriate for operation */
dcethread_exc dcethread_nostackmem_e;         /* No memory to allocate stack */
dcethread_exc dcethread_exit_thread_e;        /* Used to terminate a thread */

static void
default_uncaught_handler(dcethread_exc* exc, const char* file, unsigned int line, void* data)
{
    if (!dcethread__exc_matches(exc, &dcethread_interrupt_e) &&
        !dcethread__exc_matches(exc, &dcethread_exit_thread_e))
    {
        const char* name = dcethread__exc_getname(exc);
        if (name)
        {
            fprintf(stderr, "%s:%i: uncaught exception %s in thread %p\n", file, line, name, dcethread__self());
        }
        else
        {
            fprintf(stderr, "%s:%i: uncaught exception %p (%i) in thread %p\n",
                    file, line, exc, dcethread__exc_getstatus(exc), dcethread__self());
        }

#ifdef HAVE_BACKTRACE_SYMBOLS_FD
        void* buffer[256];
        int size;
        
        size = backtrace(buffer, 256);

        fprintf(stderr, "Backtrace:\n");
        backtrace_symbols_fd(buffer, size, fileno(stderr));
#endif
        abort();        
    }

    pthread_exit(0);
}

void
dcethread__init_exceptions(void)
{
    pthread_key_create(&frame_key, NULL);
    uncaught_handler = default_uncaught_handler;

    DCETHREAD_EXC_INIT(dcethread_uninitexc_e);
    DCETHREAD_EXC_INIT(dcethread_exquota_e);
    DCETHREAD_EXC_INIT(dcethread_insfmem_e);
    DCETHREAD_EXC_INIT(dcethread_nopriv_e);
    DCETHREAD_EXC_INIT(dcethread_illaddr_e);
    DCETHREAD_EXC_INIT(dcethread_illinstr_e);
    DCETHREAD_EXC_INIT(dcethread_resaddr_e);
    DCETHREAD_EXC_INIT(dcethread_privinst_e);
    DCETHREAD_EXC_INIT(dcethread_resoper_e);
    DCETHREAD_EXC_INIT(dcethread_aritherr_e);
    DCETHREAD_EXC_INIT(dcethread_intovf_e);
    DCETHREAD_EXC_INIT(dcethread_intdiv_e);
    DCETHREAD_EXC_INIT(dcethread_fltovf_e);
    DCETHREAD_EXC_INIT(dcethread_fltdiv_e);
    DCETHREAD_EXC_INIT(dcethread_fltund_e);
    DCETHREAD_EXC_INIT(dcethread_decovf_e);
    DCETHREAD_EXC_INIT(dcethread_subrng_e);
    DCETHREAD_EXC_INIT(dcethread_excpu_e);
    DCETHREAD_EXC_INIT(dcethread_exfilsiz_e);
    DCETHREAD_EXC_INIT(dcethread_SIGTRAP_e);
    DCETHREAD_EXC_INIT(dcethread_SIGIOT_e);
    DCETHREAD_EXC_INIT(dcethread_SIGEMT_e);
    DCETHREAD_EXC_INIT(dcethread_SIGSYS_e);
    DCETHREAD_EXC_INIT(dcethread_SIGPIPE_e);
    DCETHREAD_EXC_INIT(dcethread_unksyncsig_e);
    DCETHREAD_EXC_INIT(dcethread_interrupt_e);
    DCETHREAD_EXC_INIT(dcethread_badparam_e);
    DCETHREAD_EXC_INIT(dcethread_existence_e);
    DCETHREAD_EXC_INIT(dcethread_in_use_e);
    DCETHREAD_EXC_INIT(dcethread_use_error_e);
    DCETHREAD_EXC_INIT(dcethread_nostackmem_e);
    DCETHREAD_EXC_INIT(dcethread_exit_thread_e);
}

void
dcethread__frame_push(dcethread_frame* frame)
{
    dcethread_frame* cur = pthread_getspecific(frame_key);
    void *pframe = (void*)(struct _dcethread_frame*) frame;
    
    memset(pframe, 0, sizeof(*frame));

    frame->parent = cur;
    
    pthread_setspecific(frame_key, (void*) frame);
}

void
dcethread__frame_pop(dcethread_frame* frame)
{
    dcethread_frame* cur = pthread_getspecific(frame_key);

    if (cur == frame)
    {
	pthread_setspecific(frame_key, (void*) frame->parent);
    }
    else
    {
	DCETHREAD_ERROR("Attempted to pop exception frame in incorrect order");
    }
}

void
dcethread__exc_init(dcethread_exc* exc, const char* name)
{
    exc->kind = DCETHREAD_EXC_KIND_ADDRESS;
    exc->match.address = exc;
    exc->name = name;
}

void
dcethread__exc_setstatus(dcethread_exc* exc, int value)
{
    exc->kind = DCETHREAD_EXC_KIND_STATUS;
    exc->match.value = value;
}

int
dcethread__exc_getstatus(dcethread_exc* exc)
{
    if (exc->kind == DCETHREAD_EXC_KIND_STATUS)
	return exc->match.value;
    else
	return -1;
}

const char*
dcethread__exc_getname(dcethread_exc* exc)
{
    if (exc->kind == DCETHREAD_EXC_KIND_STATUS)
    {
        return exc->name;
    }
    else
    {
        return ((dcethread_exc*) exc->match.address)->name;
    }
}

int
dcethread__exc_matches(dcethread_exc* exc, dcethread_exc* pattern)
{
    return (exc->kind == pattern->kind &&
	    (exc->kind == DCETHREAD_EXC_KIND_STATUS ?
	     exc->match.value == pattern->match.value :
	     exc->match.address == pattern->match.address));
}

void
dcethread__exc_raise(dcethread_exc* exc, const char* file, unsigned int line)
{
    dcethread_frame* cur = pthread_getspecific(frame_key);

    if (cur)
    {
	cur->exc = *exc;
        cur->file = file;
        cur->line = line;
	siglongjmp(((struct _dcethread_frame*) cur)->jmpbuf, 1);
    }
    else
    {
        uncaught_handler(exc, file, line, uncaught_handler_data);
        abort();
    }
}

void
dcethread__exc_handle_interrupt(dcethread* thread, void* data)
{
    dcethread__exc_raise((dcethread_exc*) data, NULL, 0);
}

dcethread_exc*
dcethread__exc_from_errno(int err)
{
    switch (err)
    {
    case EINVAL:    return &dcethread_badparam_e;
    case ERANGE:    return &dcethread_badparam_e;
    case EDEADLK:   return &dcethread_in_use_e;
    case EBUSY:     return &dcethread_in_use_e;
    case EAGAIN:    return &dcethread_in_use_e;
    case ENOMEM:    return &dcethread_insfmem_e;
    case EPERM:     return &dcethread_nopriv_e;
    case -1:        return &dcethread_interrupt_e; /* XXX */
    default:        return &dcethread_use_error_e;
    }
}

void
dcethread__exc_set_uncaught_handler(void (*handler) (dcethread_exc*, const char*, unsigned int, void*), void* data)
{
    uncaught_handler = handler;
    uncaught_handler_data = data;
}
