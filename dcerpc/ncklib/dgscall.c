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
**      dgscall.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  Handle server call (SCALL) handles.
**
**
*/

#include <dg.h>
#include <dgrq.h>
#include <dgxq.h>
#include <dgsct.h>
#include <dgcall.h>
#include <dgccall.h>
#include <dgscall.h>
#include <dgcct.h>
#include <dgpkt.h>
#include <comcthd.h>
#include <comauth.h>


/* ========================================================================= */

/*
 * Server Call Timer constants.
 */

/*
 * SCALL Timer Execution Frequencies 
 * (different call states may have different frequencies)
 */
#ifdef NOTDEF
INTERNAL rpc_clock_t rpc_c_dg_scall_timer_freq_init     = RPC_CLOCK_SEC(1);
INTERNAL rpc_clock_t rpc_c_dg_scall_timer_freq_recv     = RPC_CLOCK_SEC(1);
INTERNAL rpc_clock_t rpc_c_dg_scall_timer_freq_xmit     = RPC_CLOCK_SEC(1);
INTERNAL rpc_clock_t rpc_c_dg_scall_timer_freq_final    = RPC_CLOCK_SEC(1);
INTERNAL rpc_clock_t rpc_c_dg_scall_timer_freq_idle     = RPC_CLOCK_SEC(5);
#endif

/*
 * scall specific com_timeout controllable parameters.  The values which 
 * correspond with the different timeout_knob values are defined below in
 * the routine rpc__dg_scall_timer
 *
 * max_recv_idle_time:
 *      Number of seconds allowed in recv state with out receiving any
 *      data before the client is declared dead and the call is orphaned.
 */
typedef struct {
    unsigned32 max_recv_idle_time;
} com_timeout_params_t;


/* ========================================================================= */

INTERNAL void rpc__dg_scall_timer (
        pointer_t  /*p*/
    );

INTERNAL rpc_dg_scall_p_t scall_init (
        rpc_dg_scall_p_t  /*scall*/,
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL boolean32 scall_uncache (
        rpc_dg_scall_p_t  /*scall*/
    );

INTERNAL void release_scall_from_scte (
        rpc_dg_scall_p_t  /*scall*/
    );

/* ========================================================================= */

/*
 * R E L E A S E _ S C A L L _ F R O M _ S C T E
 *
 * Release an scte's reference to an scall, whether the scall lives in
 * .scall, or on the .maybe_chain.
 */

INTERNAL void release_scall_from_scte
(
    rpc_dg_scall_p_t   scall
)
{
    if (scall->scte->scall == scall)
    {
        RPC_DG_SCALL_RELEASE_NO_UNLOCK(&scall->scte->scall);
        RPC_DBG_PRINTF(rpc_e_dbg_general, 3, (
            "(release_scall_from_scte) released cached scall\n")); 

        return;
    }
    else
    {
        /*
         * Need to check the maybe chain.
         */
        rpc_dg_scall_p_t curr, prev = NULL;

        for (curr = scall->scte->maybe_chain; curr != NULL;
             prev = curr, curr = (rpc_dg_scall_p_t) curr->c.next)
        {
            if (curr == scall)
            {
                /*
                 * First, remove the scall from the maybe chain
                 */
                if (prev == NULL)
                    scall->scte->maybe_chain =
                                (rpc_dg_scall_p_t) curr->c.next;
                else
                    prev->c.next = curr->c.next;

                RPC_DG_SCALL_RELEASE_NO_UNLOCK(&curr);
                RPC_DBG_PRINTF(rpc_e_dbg_general, 3, (
                "(release_scall_from_scte) released maybe scall\n")); 

                return;
            }
        }
    }
    /*
     * Shouldn't ever get here...
     */
    assert(scall->scte->scall == scall);
}

/*
 * S C A L L _ U N C A C H E
 *
 * This code is called for both normal and callback scalls.
 * Perform various cleanup / scall dissocations (reference removals)
 * so that the scall can really be freed.  
 *
 * Return an indication as to whether or not the "handed off" scall
 * reference was uncached or not (the latter case implying that the caller
 * still owns the lock/reference).  The fact that multiple locks are
 * necessary to perform the uncache (and there are some hierarchy problems)
 * may result in the uncache not being performed (in which case the scall
 * remains unmodified).
 */

INTERNAL boolean32 scall_uncache
(
    rpc_dg_scall_p_t scall
)
{
    unsigned32 st;
    boolean b;

    RPC_TRY_LOCK(&b);
    if (! b) 
    {
        RPC_DBG_GPRINTF(("(scall_uncache) couldn't get global lock\n"));
        return false;
    }

    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    assert(scall->c.state == rpc_e_dg_cs_idle || scall->c.state == rpc_e_dg_cs_orphan);

    if (scall->c.is_cbk)
    {
        /*
         * This is a *client side* callback scall; dissociate from our 
         * cbk_ccall if necessary.
         */
        if (scall->cbk_ccall != NULL)
        {
            rpc_dg_ccall_p_t ccall = scall->cbk_ccall;

            assert(ccall->cbk_scall == scall);

            /*
             * Acquire the callback ccall lock.  Note the locking hierarchy
             * for this type of call handle pairing is:  cbk_ccall, is_cbk scall
             * (see dg.h).
             */
            RPC_DG_CALL_TRY_LOCK(&ccall->c, &b);
            if (! b)
            {
                RPC_DBG_GPRINTF(
                    ("(scall_uncache) couldn't get cbk_scall->cbk_ccall lock\n"));
                RPC_UNLOCK(0);
                return false;
            }

            ccall->cbk_start = false;

            RPC_DG_CCALL_RELEASE(&scall->cbk_ccall);

            RPC_DG_SCALL_RELEASE_NO_UNLOCK(&ccall->cbk_scall);
        }
    }
    else
    {
        /*
         * This is a normal (server side) scall.
         */

        /* 
         * If this server side scall has been part of a callback back
         * to the client, free up the cached *server side* callback ccall
         * resources.
         */

        if (scall->cbk_ccall != NULL)
        {
            rpc_dg_ccall_p_t ccall = scall->cbk_ccall;
            
            assert(ccall->cbk_scall == scall);

            /*
             * Acquire the callback ccall lock.  Note the locking hierarchy
             * for this type of call handle pairing is:  scall, is_cbk ccall
             * (see dg.h).
             */
            RPC_DG_CALL_LOCK(&ccall->c);

            rpc__dg_ccall_free_prep(ccall);

            /*
             * Release the reference the CCALL has to its originating SCALL.
             */

            RPC_DG_SCALL_RELEASE_NO_UNLOCK(&ccall->cbk_scall);

            /*
             * Release the reference the SCALL has to the CCALL it used for
             * the callback.  Then call free_handle, which will stop the
             * timer and release the client binding handles reference to
             * the CCALL.
             */

            RPC_DG_CCALL_RELEASE(&scall->cbk_ccall);
            RPC_BINDING_RELEASE((rpc_binding_rep_p_t *) &ccall->h, &st);
        }

        /*
         * Dissociate the scall from its scte if necessary. Presumably,
         * the only time that the scall won't have a scte is if the call
         * had been orphaned, though we don't count on that.
         */

        if (scall->scte != NULL)
        {
            release_scall_from_scte(scall);

            /*
             * Release the SCALL's reference to the SCTE.
             */
        
            RPC_DG_SCT_RELEASE(&scall->scte);
        }
    }

    /*
     * Common scall uncache processing.
     */

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
                ("(scall_uncache) Freeing cached SCALL [%s]\n",
                rpc__dg_act_seq_string(&scall->c.xq.hdr)));

    /*
     * Dissociate the scall from the server binding handle if necessary.
     */

    if (scall->h != NULL)
    {
        RPC_DG_SCALL_RELEASE_NO_UNLOCK(&scall->h->scall);
        RPC_BINDING_RELEASE((rpc_binding_rep_p_t *) &scall->h, &st);
    }

    /*
     * Stop the scall's timer and dissociate it from the scall.
     */

    rpc__timer_clear(&scall->c.timer);
    RPC_DG_SCALL_RELEASE(&scall);

    RPC_UNLOCK(0);
    return true;
}


/*
 * R P C _ _ D G _ S C A L L _ O R P H A N _ C A L L
 *
 * This routine is called in one of three cases:
 *       
 *     1. An orphan quit (NCK 1.5.1 compatable) packet has been received.
 *     2. A call with a higher sequence number has been received for an 
 *        active (non-maybe) call.
 *     3. A server liveness WAY has failed.
 *
 * Things get a little tricky due to the presence of callbacks and
 * the relationship between callbacks and cancels.
 */
  
PRIVATE void rpc__dg_scall_orphan_call
(
    rpc_dg_scall_p_t scall
)
{

    RPC_LOCK_ASSERT(0);
    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    /*
     * If for some reason we're already in the orphan state, just return.
     */
    
    if (scall->c.state == rpc_e_dg_cs_orphan)
    {
        RPC_DBG_GPRINTF(("(rpc__dg_scall_orphan_call) already orphaned\n"));
        return;
    }

    RPC_DBG_GPRINTF(("(rpc__dg_scall_orphan) Orphaning%s scall [%s]\n",
        (scall->c.is_cbk ? " callback" : ""), 
        rpc__dg_act_seq_string(&scall->c.xq.hdr)));

    if (! scall->c.is_cbk)
    {
        /*
         * For a normal scall, Disassociate the selected call from the active
         * call list (SCT).
         * 
         * Note that the reference count of the SCALL is not decremented
         * to zero in this routine.  We do not want to free the SCALL until
         * its associated resources have been freed in either the SCALL timer
         * or upon return from the RPC manager routine.
         */
        assert(scall->scte != NULL);

        release_scall_from_scte(scall);
        RPC_DG_SCT_RELEASE(&scall->scte);
    }
    else
    {
        /*
         * For a callback scall we really can't do any dissociation to
         * attempt to get a new call to run before the current callback
         * completes.  There is only one (client) thread that can run
         * a new callback and it will be busy until the current one
         * completes.  If we did dissociate the cbk_scall from it's ccall,
         * then the listener (do_cbk_request) would try to create and
         * run another callback scall even though we're still running
         * one; ouch!
         *
         * So the best we can do is try to encourage the callback to
         * terminate.  The end result is that a callback scall may exist
         * in the orphaned state for a while AND be locatable by the
         * listener while in this state, so the listener better be prepared
         * to deal with this state.
         */
    }

    /*
     * Common callback and normal scall processing.
     */

    /*
     * Now, set the call's state to "orphaned".  It's resources will
     * be cleaned up by the SCALL timer thread when all outstanding
     * references are released.  There may be up to three references
     * to the SCALL at this point; the timer thread, the state reference
     * (not held in the "idle" state, and possibly an orphan call
     * executor reference.  An orphan call executor reference is freed
     * when the call returns from the server stub in execute_call().
     * All other references are freed by the scall timer. 
     */

    RPC_DG_CALL_SET_STATE(&scall->c, rpc_e_dg_cs_orphan);

    /*
     * Wake the call executor thread to tell it that it is time to go
     * away.  The signal_failure() routine will set the orphaned thread's
     * common call handle (CALL) status to an error condition so that
     * an error will be detected when the executor thread waiting on
     * the transmit or receive queue wakes up.  This will guarantee that
     * the pending recv and xmit operations are not performed and the
     * call executor thread will exit to the stubs for fault processing.
     */
       
    rpc__dg_call_signal_failure(&scall->c, rpc_s_call_orphaned);


    /*
     * Let's lose this call.  Is it queued?
     */

    if (rpc__cthread_dequeue(&scall->c.c))
    {
        /*
         * Call is queued dequeueit and release the logical reference
         * from the call executor (actually from the call executor
         * queue).
         */
        assert(scall->c.refcnt >= 2);
        rpc__dg_pkt_cancel_reservation(&scall->c);
        scall->has_call_executor_ref = false;
        RPC_DG_SCALL_RELEASE_NO_UNLOCK(&scall);
    }
    else
    {
        /*
         * Call not queued--cancel the call execution thread in an attempt
         * to achieve a more timely manager termination.  The posting
         * of this cancel must follow the behavour for normal cancels
         * (i.e. it must not be posted 'too soon' or 'too late' and it
         * must be flushed at the end of the call).  Note, we're using
         * this even for cbk_scall's (i.e. the original client thread)
         * - see rpc__dg_call_local_cancel() for more info; kinda scary,
         * huh?
         */
        rpc__cthread_cancel(&scall->c.c);
    }
}


/*
 * R P C _ _ D G _ S C A L L _ T I M E R
 *
 * Timer routine that runs periodically to do timer-based things on server calls.
 */

INTERNAL void rpc__dg_scall_timer
(
    pointer_t p
)
{
    rpc_dg_scall_p_t scall = (rpc_dg_scall_p_t) p;
    static rpc_clock_t rpc_c_dg_scall_max_idle_time = RPC_CLOCK_SEC(10);
    static com_timeout_params_t scall_com_timeout_params[] = {
        /*  0 min */        {RPC_CLOCK_SEC(2)},
        /*  1 */            {RPC_CLOCK_SEC(4)},
        /*  2 */            {RPC_CLOCK_SEC(8)},
        /*  3 */            {RPC_CLOCK_SEC(15)},
        /*  4 */            {RPC_CLOCK_SEC(30)},
        /*  5 def */        {RPC_CLOCK_SEC(2*30)},
        /*  6 */            {RPC_CLOCK_SEC(3*30)},
        /*  7 */            {RPC_CLOCK_SEC(5*30)},
        /*  8 */            {RPC_CLOCK_SEC(9*30)},
        /*  9 */            {RPC_CLOCK_SEC(17*30)},
        /* 10 infinite */   {RPC_CLOCK_SEC(0)}
    };

    RPC_DG_CALL_LOCK(&scall->c);

    if (scall->c.stop_timer)
    {
        rpc__timer_clear(&scall->c.timer);
        RPC_DG_SCALL_RELEASE(&scall); 
        return;
    }

    switch (scall->c.state) 
    {
        case rpc_e_dg_cs_init:
            /*
             * Nothing to do in this state.
             */
            break;

        case rpc_e_dg_cs_idle:
            /* 
             * If the call has been idle for a long time, stop caching
             * it.  In the case of a callback SCALL, do nothing; the
             * originating CCALL's processing dictates when this cached
             * SCALL finally gets freed.  If for some reason the
             * uncache couldn't complete, we'll try again on the next tick.
             */
            
            if (! scall->c.is_cbk)
            {
                if (rpc__clock_aged(scall->c.state_timestamp, 
                                    rpc_c_dg_scall_max_idle_time))
                {
                    if (scall_uncache(scall))
                        return;
                }
            }
            break;

        case rpc_e_dg_cs_xmit:
            /*
             * Retransmit frags if necessary.
             */
            rpc__dg_call_xmitq_timer(&scall->c);
            break;

        case rpc_e_dg_cs_recv:
            /*
             * Check to see if the client is alive.  If we have not
             * received anything from the client in "max_recv_idle_time" and
             * the receive stream is not complete assume that the client
             * is dead.  In the case of a callback SCALL, do nothing;
             * the originating CCALL's processing dictates when this
             * cached SCALL finally gets freed.
             */
            if (! scall->c.is_cbk)
            {
                if (! scall->c.rq.all_pkts_recvd
                    && rpc__clock_aged
                        (scall->c.last_rcv_timestamp, 
                         scall_com_timeout_params[scall->c.com_timeout_knob]
                                                    .max_recv_idle_time)
                    && scall->c.com_timeout_knob != rpc_c_binding_infinite_timeout)
                {
                    boolean b;

                    /*
                     * We need the global lock because we are about to
                     * modify an SCT entry. We have to violate the locking
                     * hierarchy to get the global lock.  If we can't
                     * get the global lock, just give up.  We'll try
                     * again later.  Otherwise, we will uncache the scall
                     * and stop its timer processing.
                     */
                       
                    RPC_TRY_LOCK(&b);
                    if (b) 
                    {
                        rpc__dg_scall_orphan_call(scall);
                        RPC_DG_CALL_UNLOCK(&scall->c);
                        RPC_UNLOCK(0);
                        return;
                    }
                }
            }
            break;

        case rpc_e_dg_cs_final:
            /*
             * Retransmit response if necessary; eventually give up and change to
             * the idle state.
             */
            rpc__dg_call_xmitq_timer(&scall->c);
            if (scall->c.status != rpc_s_ok
                && ! RPC_DG_HDR_FLAG_IS_SET(&scall->c.xq.hdr, RPC_C_DG_PF_IDEMPOTENT))
            {
                RPC_DG_CALL_SET_STATE(&scall->c, rpc_e_dg_cs_idle);
                if (scall->c.xq.head != NULL) 
                    rpc__dg_xmitq_free(&scall->c.xq, &scall->c);
            }
            break;

        case rpc_e_dg_cs_orphan:
            /*
             * Once the orphaned call has completed, free up the
             * the remaining resources.  As always, callbacks complicates
             * things, yielding a total of three scall scenarios:
             *      a)  a normal (server side) scall that has never
             *          been used in making a callback to a client
             *          (!scall->is_cbk && scall->cbk_ccall == NULL)
             *      b)  a normal (server side) scall that HAS
             *          been used in making a callback to a client
             *          (!scall->is_cbk && scall->cbk_ccall != NULL)
             *      c)  a callback scall (client side) that was the
             *          callback being executed
             *          (scall->is_cbk == true)
             *          (implicitly scall->cbk_ccall != NULL)
             *
             * The appropriate time for freeing up the remaining resources
             * is when the call executor (rpc__dg_execute_call) has
             * completed.  While it is possible to infer this condition
             * by examination of the scall's reference counts, it would
             * make this code fragment intolerably dependent on knowing
             * what/who has references to the scall under the various
             * scenarios.  Therefore we introduce and use the new flag:
             * scall->has_call_executor_ref.
             *
             * If for some reason the uncache couldn't complete, we'll
             * try again on the next tick.
             */

            if (! scall->has_call_executor_ref)
            {
                if (scall_uncache(scall))
                    return;
            }
            break;
    }

    RPC_DG_CALL_UNLOCK(&scall->c);
}


/*
 * R P C _ _ D G _ S C A L L _ R E I N I T
 * 
 * Reinitialize a server call handle for a new call.  The intent is that this
 * is only called internally to this module or by the SCT management
 * operations (rpc__dg_sct_new_call() / rpc__dg_ccall_lsct_new_call()).
 *
 * Prepare an existing SCALL for use in a new call according to the info
 * in the new call's packet header and the source of the new call. The
 * SCALL is remains associated with the same connection (SCTE / activity
 * id).  The call is also declared to not be "setup" (i.e. an call executor
 * is not yet established).
 */

PRIVATE void rpc__dg_scall_reinit
(
    rpc_dg_scall_p_t scall,
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    unsigned32 st;
    boolean  maybe = RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_MAYBE);

    RPC_LOCK_ASSERT(0);
    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    /*
     * Re-initialize the common call handle fields
     */

    RPC_DG_CALL_REINIT(&scall->c);

    scall->c.c.u.server.cancel.accepting    = true;
    scall->c.c.u.server.cancel.queuing      = true;
    scall->c.c.u.server.cancel.had_pending  = false;
    scall->c.c.u.server.cancel.count        = 0;

    /*
     * Typically, subsequent calls on a given actid will be for the same
     * naf and network address and received over the same server socket
     * from the same client socket (netaddr/endpoint), but alas, we can't
     * count on that...
     */ 

    /*
     * Detect naf changes and reinit cached naf-specific info.
     *
     * The max_frag_size is really associated with the
     * more specific "network address / interface" than just the naf
     * (actually they're really dependent on the even lower level of
     * path through the network even if the peer address don't change).
     * However, since the runtime currently manages these as constant
     * for a particular naf (mostly due to to the inability of system
     * APIs and/or network transports to provide this dynamic information),
     * we really only have to reset them if the naf changed (the significance
     * of this is a "different netaddr" check would be more costly).
     */
    if (scall->c.addr == NULL
        || rqe->from.rpc_protseq_id != scall->c.addr->rpc_protseq_id)
    {
        /*
         * Update to the current client address.
         */
        rpc__naf_addr_overcopy((rpc_addr_p_t) &rqe->from, &scall->c.addr, &st);

        /*
         * Initialize the max_frag_size field for the conversation with this
         * client.
         */
        RPC_DG_CALL_SET_MAX_FRAG_SIZE(&scall->c, &st);
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 7,
                       ("(rpc__dg_scall_reinit) Set max fs %lu\n",
                        scall->c.xq.max_frag_size));
    }
    else
    {
        /*
         * Update to the (typically unchanged) current client address.
         * (Only its endpoint may change.)
         */
        rpc__naf_addr_overcopy((rpc_addr_p_t) &rqe->from, &scall->c.addr, &st);
    }

    /*
     * Detect received socket changes and reinit cached socket specific info
     * (the scall may not yet have a cached sock ref or it may be different
     * from the current one).
     */
    if (scall->c.sock_ref != sp)
    {
        if (scall->c.sock_ref != NULL)
            rpc__dg_network_sock_release(&scall->c.sock_ref);

        /*
         * This reference update is a little tricky.  We need to be sure
         * that the socket is not closed before we get a chance to record
         * our reference.  We can do this safely because we are the
         * listener thread, and and we know that the listener thread
         * has a reference to the socket.  If the socket had failed,
         * and we had closed it, we wouldn't be here right now.
         */
        scall->c.sock_ref = sp;
        rpc__dg_network_sock_reference(sp);

        /*
         * Initialize the max_rcv_tsdu and max_snd_tsdu fields
         * for the conversation with this client.
         */
        rpc__naf_inq_max_tsdu(scall->c.addr->rpc_protseq_id,
                              &scall->c.xq.max_rcv_tsdu, &st);
        scall->c.xq.max_snd_tsdu = scall->c.xq.max_rcv_tsdu;
        scall->c.xq.max_rcv_tsdu = MIN(scall->c.xq.max_rcv_tsdu,
                                       scall->c.sock_ref->rcvbuf);
        scall->c.xq.max_snd_tsdu = MIN(scall->c.xq.max_snd_tsdu,
                                       scall->c.sock_ref->sndbuf);

        RPC_DBG_PRINTF(rpc_e_dbg_recv, 7,
                    ("(rpc__dg_scall_reinit) Set rcv tsdu %lu, snd tsdu %lu\n",
                        scall->c.xq.max_rcv_tsdu, scall->c.xq.max_snd_tsdu));

        /*
         * Reinit cached socket-specific information.
         */
        RPC_DG_RBUF_SIZE_TO_WINDOW_SIZE(sp->rcvbuf,
                                        sp->is_private,
                                        scall->c.xq.max_frag_size,
                                        scall->c.rq.window_size);
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 7,
                ("(rpc__dg_scall_reinit) Set ws %lu, rcvbuf %lu, max fs %lu\n",
              scall->c.rq.window_size, sp->rcvbuf, scall->c.xq.max_frag_size));
    }

    if (scall->c.is_cbk && scall->cbk_ccall != NULL)
    {
        /*
         * This is essentially a turnaround. The client, which is waiting
         * for a response, becomes the receiver.
         *
         * We inherit high_rcv_frag_size and snd_frag_size from the original
         * ccall.
         *
         * Note: If this is the initial allocation of the callback scall,
         * is_cbk is still false. rpc__dg_scall_cbk_alloc() will handle that
         * case.
         */
        scall->c.rq.high_rcv_frag_size =
            scall->cbk_ccall->c.rq.high_rcv_frag_size;
        scall->c.xq.snd_frag_size = scall->cbk_ccall->c.xq.snd_frag_size;

        /*
         * Also we inherit the reservation from the original ccall, which
         * gives us enough packets for receiving fragments.
         */
        scall->c.n_resvs = scall->cbk_ccall->c.n_resvs;
    }

    RPC_DBG_PRINTF(rpc_e_dbg_xmit, 6,
                   ("(rpc__dg_scall_reinit) Set snd fs %lu, high rcv fs %lu\n",
                    scall->c.xq.snd_frag_size, scall->c.rq.high_rcv_frag_size));

    /*
     * Re-initialize the fields of the common call handle header that
     * are really part of the prototype packet header.
     */

    scall->c.call_seq           = hdrp->seq;
    scall->c.high_seq           = hdrp->seq;
    scall->c.call_if_id         = hdrp->if_id;
    scall->c.call_if_vers       = hdrp->if_vers;
    scall->c.call_ihint         = hdrp->ihint;
    scall->c.call_opnum         = hdrp->opnum;
    scall->c.call_object        = hdrp->object;

    /*
     * Re-initialize some remaining fields in the prototype packet header.
     * Note: the ptype may not currently be "response" due to the way
     * we handle fault pkts.
     */

    scall->c.xq.base_flags      = 0;
    scall->c.xq.base_flags2     = 0;
    scall->c.xq.hdr.flags       = 0;
    scall->c.xq.hdr.flags2      = 0;

    RPC_DG_HDR_SET_PTYPE(&scall->c.xq.hdr, RPC_C_DG_PT_RESPONSE);

    /*
     * Reset the call state to the initial state.
     */

    RPC_DG_CALL_SET_STATE(&scall->c, rpc_e_dg_cs_init);

    scall->call_is_setup            = false;
    scall->has_call_executor_ref    = false;
    scall->call_is_queued           = false;
    scall->client_needs_sboot       = false;    /* Really "unknown" */

    scall->c.com_timeout_knob       = rpc_mgmt_inq_server_com_timeout();

    /*
     * If the new call uses maybe semantics, and this scall is already
     * associated with an SCTE, then we may need to reposition this scall 
     * within the SCTE.
     */
    if (maybe && scall->scte != NULL && scall->scte->scall == scall)
    {
        rpc_dg_sct_elt_p_t scte = scall->scte;

        RPC_DBG_PRINTF(rpc_e_dbg_general, 3, (
            "(rpc__dg_scall_reinit) using cached scall for maybe call\n")); 

        scall->c.next = (rpc_dg_call_p_t) scte->maybe_chain;
        scte->maybe_chain = scall;
        scte->scall = NULL; 
    }
}


/*
 * S C A L L _ I N I T
 * 
 * Initialize the common portions of normal and callback SCALLs.
 *
 * This routine requres that the global lock be held.
 */

INTERNAL rpc_dg_scall_p_t scall_init
(
    rpc_dg_scall_p_t scall,
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe
)
{
    /*
     * Initialize the common call handle fields
     */

    RPC_LOCK_ASSERT(0);

    rpc__dg_call_init(&scall->c);

    scall->c.c.is_server    = true;

    scall->c.key_info      = NULL;
    scall->c.auth_epv      = NULL;
    
    scall->c.c.u.server.cthread.is_queued = false;


    /*
     * Initialize the fields of the common call handle header that
     * are really part of the prototype packet header.
     */

    scall->c.call_server_boot   = rpc_g_dg_server_boot_time;

    /*
     * Initialize the SCTE reference so that scall_reinit knows
     * that this is a newly created scall.
     */
    scall->scte = NULL;

    /*
     * Initialize the server specific call handle fields
     */

    scall->fwd2_rqe             = NULL;
    scall->h                    = NULL;

    /*
     * use reinit to handle a sizeable chunk of the fields
     */

    RPC_DG_CALL_LOCK(&scall->c);
    rpc__dg_scall_reinit(scall, sp, rqe);
	 return scall;
}
 


/*
 * R P C _ _ D G _ S C A L L _ C B K _ A L L O C
 * 
 * Create a Server Call Handle for a callback request and
 * associate it with it's paired CCALL.  The intent is that this
 * is only called internally to this module or by the SCT management
 * operations (rpc__dg_sct_new_call() / rpc__dg_ccall_lsct_new_call()).
 *
 * The newly returned call handle is returned in a locked state.
 */

PRIVATE rpc_dg_scall_p_t rpc__dg_scall_cbk_alloc
(
    rpc_dg_ccall_p_t ccall,
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_dg_scall_p_t scall;
    static rpc_clock_t rpc_c_dg_scall_timer_freq_init = RPC_CLOCK_SEC(1);

    RPC_LOCK_ASSERT(0);

    RPC_MEM_ALLOC(scall, rpc_dg_scall_p_t, sizeof *scall, 
            RPC_C_MEM_DG_SCALL, RPC_C_MEM_NOWAIT);

    /*
     * Initialize the common SCALL handle fields (and LOCK the SCALL)
     */

    scall_init(scall, sp, rqe);

    /*
     * The rest is specific to callback SCALLs.
     */

    scall->c.actid_hash     = ccall->c.actid_hash;

    /*
     * Setup the CCALL (Logical SCTE) / SCALL cross linkage.
     */

    scall->cbk_ccall      = ccall;
    RPC_DG_CALL_REFERENCE(&ccall->c);

    ccall->cbk_scall        = scall;
    RPC_DG_CALL_REFERENCE(&scall->c);

    /*
     * Initialize the fields of the common call handle header that
     * are really part of the prototype packet header.
     */

    scall->c.call_actid         = ccall->c.call_actid;
    scall->c.call_ahint         = RPC_C_DG_NO_HINT;
    scall->c.is_cbk             = true;

    {
        unsigned32 our_min ATTRIBUTE_UNUSED;

        /*
         * This is essentially a turnaround. The client, which is waiting
         * for a response, becomes the receiver.
         *
         * We inherit high_rcv_frag_size and snd_frag_size from the original
         * scall.
         */
        scall->c.rq.high_rcv_frag_size = ccall->c.rq.high_rcv_frag_size;
        scall->c.xq.snd_frag_size = ccall->c.xq.snd_frag_size;

        /*
         * Also we inherit the reservation from the original ccall, which
         * gives us enough packets for receiving fragments.
         */
        scall->c.n_resvs = ccall->c.n_resvs;
    }

    RPC_DG_CALL_SET_TIMER(&scall->c, rpc__dg_scall_timer, rpc_c_dg_scall_timer_freq_init);

    return(scall);
}


/*
 * R P C _ _ D G _ S C A L L _ A L L O C
 * 
 * Create a Server Call Handle for a non-callback request and
 * associate it with it's SCTE.  The intent is that this
 * is only called internally to this module or by the SCT management
 * operations (rpc__dg_sct_new_call() / rpc__dg_ccall_lsct_new_call()).
 *
 * The newly returned call handle is returned in a locked state.
 */

PRIVATE rpc_dg_scall_p_t rpc__dg_scall_alloc
(
    rpc_dg_sct_elt_p_t scte,
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_dg_scall_p_t scall;
    unsigned32 st ATTRIBUTE_UNUSED;
    static rpc_clock_t rpc_c_dg_scall_timer_freq_init = RPC_CLOCK_SEC(1);
    boolean maybe   = RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_MAYBE);

    RPC_LOCK_ASSERT(0);

    RPC_MEM_ALLOC(scall, rpc_dg_scall_p_t, sizeof *scall, 
            RPC_C_MEM_DG_SCALL, RPC_C_MEM_NOWAIT);

    /*
     * Initialize the common SCALL handle fields (and LOCK the SCALL)
     */

    scall_init(scall, sp, rqe);

    /*
     * The rest is specific to normal (non-callback) SCALLs.
     */

    scall->cbk_ccall        = NULL;
    scall->c.actid_hash     = rpc__dg_uuid_hash(&scte->actid);

    /*
     * Initialize the server specific call handle fields
     */

    /*
     * Setup the SCTE / SCALL cross linkage.
     */

    RPC_DG_SCT_REFERENCE(scte);
    scall->scte = scte;

    RPC_DG_CALL_REFERENCE(&scall->c);
    if (! maybe)
        scte->scall = scall;
    else
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, 3, (
            "(rpc__dg_scall_alloc) putting call on maybe chain\n")); 
        scall->c.next = (rpc_dg_call_p_t) scte->maybe_chain;
        scte->maybe_chain = scall;
    }

    /*
     * Initialize the fields of the common call handle header that
     * are really part of the prototype packet header.
     */

    scall->c.call_actid     = scte->actid;
    scall->c.call_ahint     = scte->ahint;
    scall->c.is_cbk         = false;

    /*
     * Copy over authentication/keying information. 
     */
    scall->c.auth_epv       = scte->auth_epv;   
    scall->c.key_info       = scte->key_info;
    if (scall->c.key_info != NULL)
        RPC_DG_KEY_REFERENCE(scall->c.key_info);

    RPC_DG_CALL_SET_TIMER(&scall->c, rpc__dg_scall_timer, rpc_c_dg_scall_timer_freq_init);
                    

    return(scall);
}


/*
 * R P C _ _ D G _ S C A L L _ F R E E
 *
 * Free the resources associated with a SCALL as well as the SCALL
 * object's storage.
 *
 * This is *the* routine to use to free a SCALL.
 */

PRIVATE void rpc__dg_scall_free
(
    rpc_dg_scall_p_t scall
)
{
    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    assert(scall->c.refcnt == 0);
   
    /*
     * Release all CALL common resources.
     * This must be the last step.
     */
    rpc__dg_call_free(&scall->c);

    RPC_MEM_FREE(scall, RPC_C_MEM_DG_SCALL);
    /* scall may no longer be referenced */
}

