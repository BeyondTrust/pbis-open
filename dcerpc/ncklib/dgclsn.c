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
**      dgclsn.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  Client-oriented routines that run in the 
**  listener thread.
**
**
*/

#include <dg.h>
#include <dgxq.h>
#include <dgclsn.h>
#include <dgcall.h>


INTERNAL boolean32 chk_sboot (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

INTERNAL void do_quack_body (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

/* ========================================================================= */

/*
 * C H K _ S B O O T
 * 
 * Server boot time processing / verification. Checks that a received
 * packet is from the same instance of the server as previously received
 * packets for this call.  This function is used in all server->client
 * packet type managers.
 */

INTERNAL boolean32 chk_sboot
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    if (ccall->c.call_server_boot != 0 &&
        rqe->hdrp->server_boot != ccall->c.call_server_boot)
    {
        RPC_DBG_GPRINTF(("s->c Server boot time mismatch [%s]\n",
            rpc__dg_act_seq_string(rqe->hdrp)));
        rpc__dg_call_signal_failure(&ccall->c, rpc_s_wrong_boot_time);
        return (false);
    }
    else
    {
        return (true);
    }
}


/*
 * R P C _ _ D G _ D O _ C O M M O N _ R E S P O N S E
 *
 * Provide processing common for receipt of response, reject, fault, and 
 * quack packets.
 *
 * Returns:   false - if the packet should be discarded (old or a duplicate).
 *            true  - otherwise
 *
 */

PRIVATE boolean rpc__dg_do_common_response
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    rpc_dg_ptype_t ptype = RPC_DG_HDR_INQ_PTYPE(hdrp);
    unsigned32 st;

    /*
     * Make sure the packet is for the current call.  Note: the sequence
     * number may be greater than our current one due to proxy calls.
     *
     * Response packet considerations: 
     * If we're in a call and this response is for an old call, send
     * an "ack" just in case the activity ID has been reused on a call
     * to a different server.  If the response is to a previous call
     * then assume that an "ack" was lost and resend it.  (Note that
     * we will only resend an ACK on receipt of the last, or only, fragment
     * of a response;  we do this because when an ACK is lost, the server
     * will retransmit the last blast's worth of data and we don't want
     * to send ACKs for each one.)  Duplicate "ack"s are not a problem
     * because servers will simply ignore them. Note that the seq number
     * can actually be greater than the current call's sequence due to
     * proxy calls.
     *
     * Fault packet considerations: 
     * If the fault is to a previous call it may be a duplicate, however,
     * it may also be a valid fault packet that has not been "ack"ed.
     * Valid fault packets may result from activity ID's being rapidly
     * reused after an "ack" has been lost.  Also, the packet may be
     * for this call and we may already be in an error state which may
     * have been because we've already received a fault but for some
     * reason we haven't completed the call yet.  The proper action in
     * all three of the above cases is to send an "ack" to stop the server
     * from resending the fault.  Any extra "ack"s will be ignored by
     * the server.  Note that the seq number can actually be greater
     * than the current call's sequence due to proxy calls.
     */

    if (ccall == NULL 
        || RPC_DG_SEQ_IS_LT(hdrp->seq, ccall->c.call_seq)
        || (ptype == RPC_C_DG_PT_FAULT && ccall->fault_rqe != NULL)
        || (ptype == RPC_C_DG_PT_WORKING && ccall->c.state != rpc_e_dg_cs_recv))
    {   
        if (ptype == RPC_C_DG_PT_FAULT ||
            (ptype == RPC_C_DG_PT_RESPONSE &&
             (! RPC_DG_HDR_FLAG_IS_SET(hdrp, RPC_C_DG_PF_FRAG) ||
              RPC_DG_HDR_FLAG_IS_SET(hdrp, RPC_C_DG_PF_LAST_FRAG))))
        {
            rpc__dg_xmit_hdr_only_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, 
                rqe->hdrp, RPC_C_DG_PT_ACK);
        }
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
                ("(rpc__dg_do_common_response) Dropped %s [%s]\n", 
                rpc__dg_pkt_name(ptype), rpc__dg_act_seq_string(rqe->hdrp)));
        return (false);
    }

    RPC_DG_CALL_LOCK_ASSERT(&ccall->c);

    /*
     * If the call has been orphaned, our interest in various pkt types
     * is lessened.
     */

    if (ccall->c.state == rpc_e_dg_cs_orphan 
            && (ptype == RPC_C_DG_PT_RESPONSE || ptype == RPC_C_DG_PT_FACK))
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, 4, 
        ("(rpc__dg_do_common_response) Call failed or orphaned, Dropped %s [%s]\n",
            rpc__dg_pkt_name(ptype), rpc__dg_act_seq_string(rqe->hdrp)));
        return (false);
    } 

    /*
     * If all the packets has been received for this call, we don't have to
     * do any more work here (in particular, we don't want to check the boot
     * time since in the case of broadcast, we don't want to get upset by
     * responses from other servers which will of course have different
     * boot times).
     */

    if (ccall->c.rq.all_pkts_recvd)
        return (false);

    if (! chk_sboot(rqe, ccall))
    {
        return (false);
    }

    /*
     * There are several interesting and / or necessary pieces of
     * information that we need to snag from a response packet header.
     * Pick up the hints and the response sequence number (to properly
     * manage our sequence number space in the presence of proxy calls).
     * 
     * Now that we've successfully "(re)connected" to the server we also
     * reset the call's com_timeout_knob to be no lower than the default
     * timeout level.
     */

    if (! ccall->response_info_updated)
    {
        ccall->response_info_updated = true;
        ccall->c.call_ahint = hdrp->ahint;
        ccall->c.call_ihint = hdrp->ihint;

        ccall->c.com_timeout_knob = MAX(ccall->c.com_timeout_knob, 
                                        rpc_c_binding_default_timeout);
    }

    /*
     * We need to pick update the high_seq (highest proxy call seq) info
     * from the response stream.  Logically, we only need to get this
     * info for the last pkt of the response stream, however, it's
     * easier to always pick it up and just make certain that we
     * don't allow it to go backwards (without this check it can
     * go backwards, trust me).
     */

    if (RPC_DG_SEQ_IS_LT(ccall->c.high_seq,hdrp->seq))
        ccall->c.high_seq = hdrp->seq;

    /*
     * Update our server address binding if necessary.  Also,
     * if we're are updating the remote address being used for
     * this conversation, we must also call the NAF routine to
     * to determine the max_path_tpdu for this path.
     */

    if (! ccall->server_bound)
    {
        ccall->server_bound = true;
        ccall->c.call_server_boot = hdrp->server_boot;
        rpc__naf_addr_overcopy((rpc_addr_p_t) &rqe->from, &ccall->c.addr, &st);
      
        if ((ccall->c.xq.base_flags & RPC_C_DG_PF_BROADCAST) != 0)
        {
            RPC_DG_CALL_SET_MAX_FRAG_SIZE(&ccall->c, &st);

            RPC_DG_RBUF_SIZE_TO_WINDOW_SIZE(sp->rcvbuf,
                                            sp->is_private,
                                            ccall->c.xq.max_frag_size,
                                            ccall->c.rq.window_size);
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 7,
          ("(rpc__dg_do_common_response) Set ws %lu, rcvbuf %lu, max fs %lu\n",
              ccall->c.rq.window_size, sp->rcvbuf, ccall->c.xq.max_frag_size));
        }
    }                

    /*
     * Update the call's had_pending state with the information in the response.
     * This is really only necesssary on the last pkt of a response, but it's
     * probably cheaper to always do it.  Ensure that once a "had_pending"
     * state is detected, it stays that way.
     */

    if (RPC_DG_HDR_FLAG2_IS_SET(hdrp, RPC_C_DG_PF2_CANCEL_PENDING))
    {
        if (!ccall->cancel.server_had_pending)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10, 
                ("(rpc__dg_do_common_response) setting cancel_pending\n"));
        }
        ccall->cancel.server_had_pending = true;
    }

    return (true);
}    


/*
 * R P C _ _ D G _ D O _ R E J E C T
 *
 * Handle a call reject packet
 *
 * Signal a call failure with the appropriate status.
 */

PRIVATE boolean rpc__dg_do_reject
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    unsigned32 mst;

    /*
     * Perform common response packet processing.
     */

    if (! rpc__dg_do_common_response(sp, rqe, ccall))
        return (RPC_C_DG_RDF_FREE_RQE);

    RPC_DBG_GPRINTF(("(rpc__dg_do_reject) Got a live one [%s]\n", 
        rpc__dg_act_seq_string(rqe->hdrp)));

    RPC_DG_CALL_LOCK_ASSERT(&ccall->c);

    /*
     * When in the orphan state, a reject is just as good as a quack.
     */

    if (ccall->c.state == rpc_e_dg_cs_orphan)
    {
        ccall->quit.quack_rcvd = true;
        rpc__dg_call_signal(&ccall->c);
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    /* 
     * Free any packets we have on the transmit queue still since the
     * fault is (implicitly) ack'ing them.
     */

    if (ccall->c.xq.head != NULL)
        rpc__dg_xmitq_free(&ccall->c.xq, &ccall->c);

    /* 
     * A reject response terminates the OUTs data stream.
     */ 

    ccall->c.rq.all_pkts_recvd = true;

    /*
     * Get the NCA status from the reject packet body and map it to a local
     * status code.  Blow off the call with this mapped status.
     */

    rpc__dg_get_epkt_body_st(rqe, &ccall->reject_status);

    switch ((int)ccall->reject_status)
    {
        case nca_s_comm_failure:        mst = rpc_s_comm_failure;        break;
        case nca_s_unk_if:              mst = rpc_s_unknown_if;          break;
        case nca_s_unsupported_type:    mst = rpc_s_unsupported_type;    break;
        case nca_s_manager_not_entered: mst = rpc_s_manager_not_entered; break;
        case nca_s_op_rng_error:        mst = rpc_s_op_rng_error;        break;
        case nca_s_who_are_you_failed:  mst = rpc_s_who_are_you_failed;  break;
        case nca_s_proto_error:         mst = rpc_s_protocol_error;      break;
        case nca_s_wrong_boot_time:     mst = rpc_s_wrong_boot_time;     break;
        default:                        mst = rpc_s_unknown_reject;      break;
    }

    rpc__dg_call_signal_failure(&ccall->c, mst);

    return (RPC_C_DG_RDF_FREE_RQE);
}


/*
 * R P C _ _ D G _ D O _ F A U L T
 *
 * Handle a call fault packet.
 *
 * Stash the fault pkt away for eventual retrieval by the client
 * stub and signal a call "call_faulted" failure.
 */

PRIVATE boolean rpc__dg_do_fault
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    
    /*
     * Perform common response packet processing.
     */

    if (! rpc__dg_do_common_response(sp, rqe, ccall))
        return (RPC_C_DG_RDF_FREE_RQE);

    RPC_DBG_GPRINTF(("(rpc__dg_do_fault) Got a live one [%s]\n",
        rpc__dg_act_seq_string(rqe->hdrp)));

    RPC_DG_CALL_LOCK_ASSERT(&ccall->c);

    /*
     * When in the orphan state, a fault is just as good as a quack.
     */

    if (ccall->c.state == rpc_e_dg_cs_orphan)
    {
        ccall->quit.quack_rcvd = true;
        rpc__dg_call_signal(&ccall->c);
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    /* 
     * A fault response terminates the OUTs data stream.
     */ 

    ccall->c.rq.all_pkts_recvd = true;

    ccall->fault_rqe = rqe;
    rpc__dg_call_signal_failure(&ccall->c, rpc_s_call_faulted);

    return (0 /* DON'T free the rqe */);
}


/*
 * R P C _ _ D G _ D O _ R E S P O N S E
 *
 * Handle a response packet.
 */

PRIVATE boolean rpc__dg_do_response
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    boolean rqe_wake_thread;
    rpc_dg_xmitq_p_t xq;
    boolean32 drop;

    /*
     * Perform common response packet processing.
     */

    if (! rpc__dg_do_common_response(sp, rqe, ccall))
        return (RPC_C_DG_RDF_FREE_RQE);

    RPC_DG_CALL_LOCK_ASSERT(&ccall->c);

    xq = &ccall->c.xq;

    /* 
     * Free any packets we have on the transmit queue still since the
     * response is (implicitly) ack'ing them.
     */

    if (xq->head != NULL)
        rpc__dg_xmitq_free(xq, &ccall->c);

    /*
     * If we were pinging the server, we can stop now.
     */
                           
    RPC_DG_PING_INFO_INIT(&ccall->ping);

    /*
     * Add the packet to the receive queue, noting whether or not it
     * actually gets queued.
     */

    drop = ! rpc__dg_call_recvq_insert(&ccall->c, rqe, &rqe_wake_thread);

    /*
     * If "recvq_insert" told us that there are a sufficient number of
     * rqe's to warrant wakeing up the client then do so.
     */

    if (rqe_wake_thread)
    {
        rpc__dg_call_signal(&ccall->c);
    }

    /*
     * Decide on a return status.  If the packet was dropped, tell the
     * caller to free the packet.  Otherwise, if we signalled the call
     * that the packet was queued to, tell the listener thread to yield.
     * Otherwise, return 0, meaning, don't free the packet, or yield.
     */

    if (drop)
        return (RPC_C_DG_RDF_FREE_RQE);

    if (rqe_wake_thread) 
        return (RPC_C_DG_RDF_YIELD /* and DON'T free the rqe */);

    return (0 /* DON'T free the rqe */);
}


/*
 * P I N G _ L A T E R
 *
 * Schedule a ping to happen later for the specified ccall.  This routine is
 * shared by the "working" and "nocall/fack" packet processing routines.
 */

INTERNAL void ping_later (
        rpc_dg_ccall_p_t /*ccall*/,
        rpc_dg_recvq_elt_p_t /*rqe*/
    );

INTERNAL void ping_later
(
    rpc_dg_ccall_p_t ccall,
    rpc_dg_recvq_elt_p_t rqe ATTRIBUTE_UNUSED
)
{
    struct rpc_dg_ping_info_t *ping;
    rpc_clock_t now, duration, interval;

    ping = &ccall->ping;
    now = rpc__clock_stamp();

    /*
     * Stop any ongoing pinging.
     */

    ping->pinging = false;

    /*
     * Compute the time to start pinging again as a function of how long
     * the call has been running.  (The longer the call's been running,
     * the longer we wait until starting to ping again.)  
     *
     * Note that there's an interaction between the ping rate and the
     * server's "max receive state idle time":  the server, while in
     * the receive state, may declare the client dead if it doesn't hear
     * from it from it in a while.
     */

    duration = now - ccall->c.start_time;

    if (duration < RPC_CLOCK_SEC(10))
        interval = RPC_CLOCK_SEC(3);
    else if (duration < RPC_CLOCK_SEC(30))
        interval = RPC_CLOCK_SEC(10);
    else 
        interval = RPC_CLOCK_SEC(30);

    ping->next_time = now + interval;

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
        ("(stop_pinging) Next ping at: now = %lu, interval = %lu [%s]\n", 
        now, interval, rpc__dg_act_seq_string(rqe->hdrp)));
}


/*
 * R P C _ _ D G _ D O _ W O R K I N G
 *
 * Handle a working packet.  Working's are received in response to pings
 * and mean that the server has received all the in's.  Note that the
 * server may still fail the call (e.g., because it doesn't handle the
 * requested interface).
 */

PRIVATE boolean rpc__dg_do_working
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    rpc_dg_xmitq_p_t xq;

    RPC_LOCK_ASSERT(0);

    /*
     * Perform common response packet processing.
     */

    if (! rpc__dg_do_common_response(sp, rqe, ccall))
        return (RPC_C_DG_RDF_FREE_RQE);

    RPC_DG_CALL_LOCK_ASSERT(&ccall->c);

    xq = &ccall->c.xq;

    /* 
     * If our call is non-idempotent, free any packets we have on the
     * transmit queue still since the working response is (implicitly)
     * ack'ing the receipt of them.  Note well that we MUST NOT free
     * the queue for idempotent calls since the server is allowed to
     * drop the response in which case a subsequent ping from us will
     * induce a nocall response and oblige us to retransmit the request.
     */

    if ((xq->base_flags & RPC_C_DG_PF_IDEMPOTENT) == 0 &&
        xq->head != NULL)
    {
        rpc__dg_xmitq_free(xq, &ccall->c);
    }

    ping_later(ccall, rqe);

    return (RPC_C_DG_RDF_FREE_RQE);
}


/*
 * R P C _ _ D G _ D O _ N O C A L L
 *
 * Handle a no-call packet.  No-call's are received in response to pings.
 * Note that in the case of large in's, some of which have already been
 * fack'd, no-call simply means that not all the in's have arrived, NOT
 * that the server knows nothing about the call.  Confusing, eh?
 */

PRIVATE boolean rpc__dg_do_nocall
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    rpc_dg_xmitq_p_t xq;

    RPC_LOCK_ASSERT(0);

    /*
     * Make sure the pkt is for the current call, that we are in the
     * receive state, and that the call has not already failed.  Note:
     * the sequence number may be greater than our current one due to
     * proxy calls.
     */
       
    if (ccall == NULL
        || RPC_DG_SEQ_IS_LT(rqe->hdrp->seq, ccall->c.call_seq)
        || ccall->c.state != rpc_e_dg_cs_recv
        || ccall->c.status != rpc_s_ok)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, 2, 
            ("(rpc__dg_do_nocall) Dropped [%s]\n", 
            rpc__dg_act_seq_string(rqe->hdrp)));
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    RPC_DG_CALL_LOCK_ASSERT(&ccall->c);

    if (! chk_sboot(rqe, ccall))
    {
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    xq = &ccall->c.xq;

    /*
     * When in the orphan state, a nocall is just as good as a quack.
     */

    if (ccall->c.state == rpc_e_dg_cs_orphan)
    {
        ccall->quit.quack_rcvd = true;
        rpc__dg_call_signal(&ccall->c);
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    if (xq->head == NULL) {
        RPC_DBG_PRINTF(rpc_e_dbg_general, 2, 
            ("(rpc__dg_do_nocall) Transmit queue is empty [%s]\n",
            rpc__dg_act_seq_string(rqe->hdrp)));
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    /*
     * No-call means we should stop pinging.
     */

    RPC_DG_PING_INFO_INIT(&ccall->ping);   

    /*
     * If there's a body in this no-call, it must be a fack body.  
     */

    if (rqe->hdrp->len > 0)
    {
        boolean sent_data;

        /*
         * Process the nocall just like a fack.  
         */

        RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
            ("(rpc__dg_do_nocall) Has fack body [%s]\n", 
            rpc__dg_act_seq_string(rqe->hdrp)));

        rpc__dg_fack_common(sp, rqe, &ccall->c, &sent_data);

        /*
         * Conditionally schedule the next ping for later (i.e., backed
         * off, just as if we'd gotten a "working".  The "condition"
         * is that the fack processing has NOT just queued up some data
         * to send.  In this case, we must ping soon to verify that the
         * frag was received.  (The problem case comes when the call
         * has been queued for long enough that the ping interval is
         * longer than the comm timeout setting.)
         */

        if (! sent_data)
        {
            ping_later(ccall, rqe);
        }

        return (RPC_C_DG_RDF_FREE_RQE);
    }

    /*
     * =========== Process a no-call that lacks a fack body ===========
     */

    /*
     * If this is an idempotent call (which must mean a single pkt
     * request, though we won't count on that) and we're retransmitting
     * the beginning of a call, then it may have been the case that the
     * call actually ran, but the response was lost.  To ensure that
     * we don't loose a cancel, reset some of our cancel state (which may
     * result in re-forwarding a cancel) so that call_end() can make an
     * accurate cancel_pending decision.
     */
    
    if (xq->head->fragnum == 0 
        && (ccall->c.xq.base_flags & RPC_C_DG_PF_IDEMPOTENT))
    {
        ccall->cancel.server_count = 0;
        ccall->cancel.server_is_accepting = true;
        ccall->cancel.server_had_pending = false;
        /* Retain *current* ccall->cancel.local_count */
    }
    
    /*
     * Since we had begun pinging, we know that everything on the queue
     * has been sent atleast once.  However, the receipt of a nocall
     * is an indication from the server that it's missing something.
     * Unfortunately, we have no way of telling what it's missing, so
     * all we can do is begin sending everything on the queue again.
     */
    
    rpc__dg_xmitq_restart(&ccall->c);
    
    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
        ("(rpc__dg_do_nocall) Retransmitting %lu frags [%s]\n", 
        xq->blast_size, rpc__dg_act_seq_string(rqe->hdrp)));
    
    return (RPC_C_DG_RDF_FREE_RQE);
}


/*
 * D O _ Q U A C K _ B O D Y
 *
 * Process the contents of a quack body.
 * A quack with a body is interpreted as a cancel-request ack.
 */

INTERNAL void do_quack_body
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    unsigned32 cancel_id;
    boolean server_is_accepting;

#ifndef MISPACKED_HDR

    rpc_dg_quackpkt_body_p_t bodyp = (rpc_dg_quackpkt_body_p_t) &rqe->pkt->body;

    /*
     * Make sure we understand the quack packet body version.
     */

    if (bodyp->vers != RPC_C_DG_QUACKPKT_BODY_VERS)
    {
        RPC_DBG_GPRINTF(("(do_quack_body) Unknown version; Dropped [%s]\n", 
                rpc__dg_act_seq_string(rqe->hdrp)));
        return;
    }

    /*
     * Extract the cancel_id of the cancel being ack'ed and the sender's
     * server_is_accepting (cancels) flag.
     */

    cancel_id           = bodyp->cancel_id; 
    server_is_accepting = bodyp->server_is_accepting; 

    if (NDR_DREP_INT_REP(rqe->hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_32(cancel_id);
    }                                    
#else

#error "No code for mispacked_hdr here!"

#endif /* MISPACKED_HDR */

    /*
     * Update the call handle information.  Since pkts may be duplicated
     * / rcvd out of order... ignore "old" cancel quacks.
     * 
     * While we currently don't think that a server will ever tell us
     * to re-enable acceptance once it declares that it is no longer
     * accepting cancels there's no good reason to enforce this (and
     * there's good reason to leave the flexibility in).  Of course,
     * we don't want "old" acks to affect the state.
     * 
     * Define the architectural requirement that servers may adjust their
     * cancel acceptance state as they see fit; however, the clients
     * will not adapt to the new state unless the state is conveyed to
     * the client in a cancel quack specifying a cancel_id that
     * at least as high as the highest cancel previously acked by the
     * server.
     */
    if (cancel_id < ccall->cancel.server_count)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
                ("(do_quack_body) Old quack; Dropped [%s]\n", 
                rpc__dg_act_seq_string(rqe->hdrp)));
        return;
    }

    ccall->cancel.server_count          = cancel_id;
    ccall->cancel.server_is_accepting   = server_is_accepting;
}


/*
 * R P C _ _ D G _ D O _ Q U A C K
 *
 * Handle a call quit acknowlegement packet.
 * Quacks have two forms:
 *  a)  orphan-request ack (2.0 and pre 2.0)
 *  b)  cancel-request ack (2.0 only)
 *
 * Note: A 2.0 client sending a cancel-request will get a orphan-request-ack
 * when talking to a 1.5.1 server (since cancel and orphan requests are
 * identical from the 1.5.1 server's perspective).
 */

PRIVATE boolean rpc__dg_do_quack
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_ccall_p_t ccall
)
{
    /*
     * Perform common response packet processing.
     */

    if (! rpc__dg_do_common_response(sp, rqe, ccall))
        return (RPC_C_DG_RDF_FREE_RQE);

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3,
        ("(rpc__dg_do_quack) Rcvd %s quack [%s]\n", 
        rqe->hdrp->len != 0 ? "cancel" : "orphan",
        rpc__dg_act_seq_string(rqe->hdrp)));

    RPC_DG_CALL_LOCK_ASSERT(&ccall->c);
 
    /*
     * An orphan-quack is identified by a quack without a body
     * (a cancel-quack has a body).
     */
    if (rqe->hdrp->len != 0)
    {
	do_quack_body(rqe, ccall);
    }
    else
    {
        /*
         * If we're not in the orphan state (this orphan-quack IS for our call)
         * we must be talking to a 1.5.1 server (which has now orphaned
         * the call, so there's no need to continue).
         */

        if (ccall->c.state != rpc_e_dg_cs_orphan)
        {
            RPC_DBG_GPRINTF(
                ("(rpc__dg_do_quack) Rcvd orphan quack in response to cancel %s [%s]\n", 
                rpc__dg_call_state_name(ccall->c.state),
                rpc__dg_act_seq_string(rqe->hdrp)));
            rpc__dg_call_signal_failure(&ccall->c, rpc_s_call_cancelled);
            return (RPC_C_DG_RDF_FREE_RQE);
        }

        /*
         * A "quack" packet has been received, signal the thread waiting in 
         * rpc__dg_call_end() to clean up the resources held by the call and
         * transition to the idle state.
         */

        ccall->quit.quack_rcvd = true;
        rpc__dg_call_signal(&ccall->c);
    }
    
    return (RPC_C_DG_RDF_FREE_RQE);
}

