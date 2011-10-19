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
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
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

#include <dce/dcethread.h>
#include <signal.h>
#include <errno.h>
#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"

static void *async_signal_handler (void *dummy);

/* FIXME: do we need to retain thread? */
static dcethread* thread_to_interrupt;
static sigset_t async_sigset;
static pthread_t helper_thread = (pthread_t)0;

/* -------------------------------------------------------------------- */

/* 
 * A S Y N C _ S I G N A L _ H A N D L E R
 * 
 * This async signal handler is already running on the correct thread
 * stack.   ALL async signals map to a "cancel".  A cancel unwind happens
 * only at well defined points, so we can't RAISE the exception; just
 * post the cancel.
 */
static void *async_signal_handler(void *dummy)
{
    /*
     * Wait for and handle asynchronous signals.
     */
    while (1)
    {
        int sig;

        sigwait(&async_sigset, &sig);
	dcethread__lock(thread_to_interrupt);
	dcethread__interrupt(thread_to_interrupt);
	dcethread__unlock(thread_to_interrupt);
    }

    return NULL;
}


/*
 * P T H R E A D _ S I G N A L _ T O _ C A N C E L _ N P
 *
 * Async signal handling consists of creating an exception package helper
 * thread.  This helper thread will sigwait() on all async signals of
 * interest and will convert specific asynchronous signals (defined in the
 * async signal handler) to exceptions.  The helper thread then posts the
 * cancel to the thread that initialized the exception package for 
 * 'handling'.
 */
void
dcethread_signal_to_interrupt(sigset_t *sigset, dcethread* thread)
{
    /* 
     * The helper thread will need the thread id of the first thread
     * to initialize the exception package.  The thread that initialized
     * the exception package will receive a cancel when an asynchronous
     * signal is received.
     */
    thread_to_interrupt = thread;
    async_sigset = *sigset;

    dcethread_lock_global();

    if (helper_thread != (pthread_t)0)
    {
        pthread_cancel(helper_thread);
        pthread_detach(helper_thread);
    }

    /* 
     * Create a 'helper thread' to catch aynchronous signals.
     */
    pthread_create(&helper_thread, NULL, async_signal_handler, 0);

    /* 
     * The 'helper thread' will never be joined so toss any pthread package
     * internal record of the thread away to conserve resources.
     */
    pthread_detach(helper_thread);

    dcethread_unlock_global();
}

