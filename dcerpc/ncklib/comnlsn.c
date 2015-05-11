/*
 * 
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
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
/*
 */
/*
**
**  NAME:
**
**      comnlsn.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Network Listener Service internal operations.
**      This file provides the "less portable" portion of the PRIVATE
**      Network Listener internal interface (see comnet.c) and should
**      ONLY be called by the PRIVATE Network Listener Operations.
**
**      Different implementations may choose to provide an alternate
**      implementation of the listener thread processing by providing
**      a reimplementation of the operations in this file.  The portable
**      Network Listener Services operations (for the public and private
**      API) is in comnet.c
**
**      This particular implementation is "tuned" for:
**          a) the use of a dcethread for listener processing
**          b) a rpc_socket_t is a UNIX File Descriptor
**          c) BSD UNIX select()
**
**
*/

#include "config.h"

#ifdef HAVE_SYS_FD_SET_H
#include <sys/fd_set.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <commonp.h>
#include <com.h>
#include <comp.h>
#include <comnetp.h>

/*
*****************************************************************************
*
* local data structures
*
*****************************************************************************
*/


/*
 * Some defaults related to select() fd_sets.
 */

#ifndef RPC_C_SELECT_NFDBITS
#  define RPC_C_SELECT_NFDBITS      NFDBITS
#endif

#ifndef RPC_SELECT_FD_MASK_T
#  define RPC_SELECT_FD_MASK_T      fd_mask
#endif

#ifndef RPC_SELECT_FD_SET_T
#  define RPC_SELECT_FD_SET_T       fd_set
#endif

#ifndef RPC_SELECT_FD_COPY
#  define RPC_SELECT_FDSET_COPY(src_fd_set,dst_fd_set,nfd) { \
    int i; \
    RPC_SELECT_FD_MASK_T *s = (RPC_SELECT_FD_MASK_T *) &src_fd_set; \
    RPC_SELECT_FD_MASK_T *d = (RPC_SELECT_FD_MASK_T *) &dst_fd_set; \
    for (i = 0; i < (nfd); i += RPC_C_SELECT_NFDBITS) \
        *d++ = *s++; \
   }
#endif


/*
 * Miscellaneous Data Declarations
 */

INTERNAL dcethread*                  listener_thread;
INTERNAL volatile boolean                    listener_thread_running = false;
INTERNAL volatile boolean                    listener_thread_was_running = false;
INTERNAL volatile boolean           listener_should_handle_cancels = false;

INTERNAL rpc_listener_state_t       listener_state_copy;

INTERNAL RPC_SELECT_FD_SET_T        listener_readfds;
INTERNAL int                        listener_nfds = 0;

/*
 * This should really be heap allocated (see g_e_t_dtablesize(2)) to
 * deal with systems that have an extremely large fd_set.  For now,
 * at least keep this off the stack.
 */
INTERNAL RPC_SELECT_FD_SET_T        readfds_copy;



INTERNAL void copy_listener_state (
        rpc_listener_state_p_t   /*lstate*/
    );

INTERNAL void lthread (
        rpc_listener_state_p_t   /*lstate*/
    );

INTERNAL void lthread_loop (void);



/*
 * R P C _ _ N L S N _ A C T I V A T E _ D E S C
 *
 * Mark that the specified descriptor is now "live" -- i.e., that events
 * on it should be processed.  This routine is also responsible for
 * starting up a listener thread if once doesn't already exist.  
 *
 * Note that once a socket has been activated, it is should never be
 * removed and not closed from the set of sockets that are
 * select(2)'d on, because we always must always "drain" the socket
 * of events (via recv or accept) so it doesn't get "clogged up"
 * with stuff (that would consume system resources). 
 */

PRIVATE void rpc__nlsn_activate_desc 
(
    rpc_listener_state_p_t  lstate,
    unsigned32              idx,
    unsigned32              *status
)
{
    RPC_MUTEX_LOCK_ASSERT (lstate->mutex);

    lstate->socks[idx].is_active = true;

    /*
     * If the listener thread is running, then just cancel it and let
     * it do the work.  Note that that thread must do the work since
     * this module's state is not covered by a mutex (for efficiency).
     * Otherwise, start the listen thread now.
     */

    if (listener_thread_running)
    {
        dcethread_interrupt_throw (listener_thread);
    }
    else
    {
        listener_thread_running = true;
        listener_should_handle_cancels = true;
        dcethread_create_throw (
            &listener_thread,                   /* new thread    */
            &rpc_g_default_dcethread_attr,         /* attributes    */
            (dcethread_startroutine)lthread,   /* start routine */
            lstate);           /* arguments     */
    }

    *status = rpc_s_ok;
}

/*
 * R P C _ _ N L S N _ D E A C T I V A T E _ D E S C
 *
 * Mark that the specified descriptor is no longer "live" -- i.e., that
 * events on it should NOT be processed.
 */

PRIVATE void rpc__nlsn_deactivate_desc 
(
    rpc_listener_state_p_t  lstate,
    unsigned32              idx,
    unsigned32              *status
)
{
    dcethread*   current_thread;

    RPC_MUTEX_LOCK_ASSERT (lstate->mutex);

    *status = rpc_s_ok;

    lstate->socks[idx].is_active = false;

    /*
     * If the listener thread is not running, there's nothing more to do.
     */

    if (! listener_thread_running)
    {
        return;
    }

    /*
     * If we're the listener thread, then just update the listener state
     * ourselves.  If we're not, then cancel the listener thread so it
     * will pick up the new state.  Note that in the latter case, we
     * synchronize with the listener thread.  The point of this exercise
     * is that when we return from this function, we want to make SURE
     * that the listener thread will NOT try to select() on the FD we're
     * trying to deactivate.  (Some callers of this function close the FD
     * and some other [perhaps not RPC runtime] thread might call open()
     * or socket() and get that FD.  We need to make sure that the listener
     * thread never uses this reincarnated FD.)
     */

    current_thread = dcethread_self();
    if (dcethread_equal (current_thread, listener_thread))
    {
        copy_listener_state(lstate);
    }
    else 
    {
        lstate->reload_pending = true;
        dcethread_interrupt_throw (listener_thread);

        while (lstate->reload_pending)
        {
            RPC_COND_WAIT (lstate->cond, lstate->mutex);
        }
    }
} 



/*
 * C O P Y _ L I S T E N E R _ S T A T E
 *
 * Copy the listener state pointed to by the input parameter into our
 * copy of the listener state.  N.B. This routine must be called only
 * from the listener thread.
 */

INTERNAL void copy_listener_state 
(
    rpc_listener_state_p_t  lstate
)
{
    unsigned16              nd;

    RPC_MUTEX_LOCK_ASSERT (lstate->mutex);

    /*
     * Make a copy of the active entries in the network information
     * table.  We'll pass the copy to the listen loop.  Using a copy
     * means that the listen loop can run without taking and releasing
     * locks.  Descriptors are presumably added/deleted infrequently
     * enough that this strategy is a net win.  We also compute the
     * "nfds" and "readfds" arguments to select(2), which we also pass
     * down to the listen loop.
     */

    FD_ZERO (&listener_readfds);
    listener_nfds = 0;

    for (nd = 0, listener_state_copy.num_desc = 0; nd < lstate->high_water; nd++)
    {
        rpc_listener_sock_p_t lsock = &lstate->socks[nd];
        int desc = rpc__socket_get_select_desc(lsock->desc);

        if (lsock->busy)
        {
            listener_state_copy.socks[listener_state_copy.num_desc++] = *lsock;
            FD_SET (desc, &listener_readfds);
            if (desc + 1 > listener_nfds)
            {
                listener_nfds = desc + 1;
            }
        }
    }

    listener_state_copy.high_water = listener_state_copy.num_desc;

    /*
     * Let everyone who's waiting on our completion of the state update
     * know we're done.
     */
    lstate->reload_pending = false;
    RPC_COND_BROADCAST (lstate->cond, lstate->mutex);
}


/*
 * L T H R E A D
 *
 * Startup routine for the listen thread.
 */

INTERNAL void lthread 
(
    rpc_listener_state_p_t  lstate
)
{
    /*
     * Loop, calling the real listen loop on each pass.  Each time a
     * socket (descriptor) is activated or deactivated this thread is
     * cancelled (so that we can know about the change and call "select"
     * with the right masks).  When the thread is supposed to really
     * go away, as on a fork, the handle_cancel boolean will be set
     * appropriately before the cancel is posted.
     */
              
    while (listener_should_handle_cancels)
    {
        /*
         * Invoke the real listen loop, protecting against (presumably)
         * cancel-induced unwinds.
         */
	dcethread_enableinterrupt_throw(0);
        RPC_MUTEX_LOCK (lstate->mutex);
        copy_listener_state (lstate);
        RPC_MUTEX_UNLOCK (lstate->mutex);
	dcethread_enableinterrupt_throw(1);

        DCETHREAD_TRY
        {
            lthread_loop ();
        }     
        DCETHREAD_CATCH(dcethread_interrupt_e)
        {
#ifdef NON_CANCELLABLE_IO_SELECT
            dcethread_enableasync_throw(0);
#endif
            RPC_DBG_PRINTF (rpc_e_dbg_general, 2, ("(lthread) Unwound\n"));
//		dce_pthread_clear_exit_np();
        }
        DCETHREAD_ENDTRY
    }
}



/*
 * L T H R E A D _ L O O P
 *
 * Server listen thread loop.
 */

INTERNAL void lthread_loop (void)
{
    unsigned32          status;
    int                 nd;
    int                 n_found;

    /*
     * Loop waiting for incoming packets.
     */

    while (true)
    {
        /*
         * Wait for packets.
         */

        do
        { 
            RPC_SELECT_FDSET_COPY(listener_readfds, readfds_copy, listener_nfds);

            /*
             * Block waiting for packets.  We ocassionally need to see
             * changes in the readfds and listener state even if we aren't
             * recving pkts.  At such times, we cancel this thread to
             * get it to see the updates.  If 'select' isn't cancellable
             * on a particular implementation, enabling async
             * cancellability should do the trick.
             *
             * By posix definition dcethread_enableasync_throw is not a "cancel
             * point" because it must return an error status and an errno.
             * dcethread_enableasync_throw(1) will not deliver
             * a pending cancel nor will the cancel be delivered asynchronously,
             * thus the need for dcethread_checkinterrupt.
             * 
             */
#ifdef NON_CANCELLABLE_IO_SELECT
            dcethread_enableasync_throw(1);
            dcethread_checkinterrupt();
#endif /* NON_CANCELLABLE_IO_SELECT */
            RPC_LOG_SELECT_PRE;
            n_found = dcethread_select (
			      listener_nfds, &readfds_copy, NULL, NULL, NULL);
            RPC_LOG_SELECT_POST;

#ifdef NON_CANCELLABLE_IO_SELECT
            dcethread_enableasync_throw(0);
#endif /* NON_CANCELLABLE_IO_SELECT */
            if (n_found < 0)
            {
                if (errno != EINTR)
                {
                    RPC_DBG_GPRINTF 
                        (("(lthread_loop) select failed: %d, errno=%d\n",
                        n_found, errno));
                
                    /*
                     * Check for pending cancels.  Select might return
                     * EIO (and not check for a pending cancel) because
                     * of a broken socket that another thread is trying
                     * to deactivate.  The testcancel will induce us
                     * to take the cancel that the thread trying to
                     * deactivate the socket sent us.
                     */
                    dcethread_checkinterrupt();
                }
                continue;
            }
        }
        while (n_found <= 0);

        /*
         * Process any descriptors that were active.
         */

        for (nd = 0; n_found && (nd < listener_state_copy.num_desc); nd++)
        {
            rpc_listener_sock_p_t lsock = &listener_state_copy.socks[nd];
            int desc = rpc__socket_get_select_desc(lsock->desc);

            if (lsock->busy && FD_ISSET (desc, &readfds_copy))
            {
                n_found--;

                (*lsock->network_epv->network_select_disp)
                    (lsock->desc, lsock->priv_info, lsock->is_active, &status);
                if (status != rpc_s_ok)
                {
                    RPC_DBG_GPRINTF
                    (("(lthread) select dispatch failed: desc=%d *status=%d\n",
                        lsock->desc, status));

                    /*
                     * Check for pending cancels.  Select might have
                     * returned "success" (and not checked for a pending
                     * cancel) even though there is a broken socket that
                     * another thread is trying to deactivate.  The
                     * testcancel will induce us to take the cancel that
                     * the thread trying to deactivate the socket sent
                     * us.
                     */
                    dcethread_checkinterrupt();
                }
            }
        }
    }
}

#ifdef ATFORK_SUPPORTED
/*
 * R P C _ _ N L S N _ F O R K _ H A N D L E R
 *
 * Handle fork related issues for this module.
 */

PRIVATE void rpc__nlsn_fork_handler
(
  rpc_listener_state_p_t  lstate,
  rpc_fork_stage_id_t stage
)
{ 
    unsigned32 st;

    switch ((int)stage)
    {
        case RPC_C_PREFORK:
            RPC_MUTEX_LOCK (lstate->mutex);
            listener_thread_was_running = false;
            if (listener_thread_running)
            {
                listener_should_handle_cancels = false;
                dcethread_interrupt_throw(listener_thread);
                RPC_MUTEX_UNLOCK (lstate->mutex);
                dcethread_join_throw(listener_thread, (void **)&st);
                RPC_MUTEX_LOCK (lstate->mutex);
                /* THIS DOESN'T MAKE ANY SENSE
                DCETHREAD_TRY {
		    dcethread_detach_throw(listener_thread);
		} DCETHREAD_CATCH(dcethread_use_error_e) {
                } DCETHREAD_ENDTRY */
                listener_thread_was_running = true;
                listener_thread_running = false;
            }
            break;

        case RPC_C_POSTFORK_PARENT:
            if (listener_thread_was_running)
            {
                listener_thread_was_running = false;
                listener_thread_running = true;
                listener_should_handle_cancels = true;
                dcethread_create_throw (
                    &listener_thread,                   /* new thread    */
                    &rpc_g_default_dcethread_attr,         /* attributes    */
                    (dcethread_startroutine)lthread,   /* start routine */
                    (dcethread_addr)lstate);           /* arguments*/
            }
            RPC_MUTEX_UNLOCK (lstate->mutex);
            break;

        case RPC_C_POSTFORK_CHILD:  
            /*
             * Unset the flag that says the listern thread has been started
             */
            listener_thread_was_running = false;
            listener_thread_running = false;
            /*
             * The mutex has already been destroyed.
             * RPC_MUTEX_UNLOCK (lstate->mutex);
             */
            break;
    }
}
#endif
