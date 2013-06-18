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
**      dgcall.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  Handles call (CALL) handles.
**
**       
*/

#include <dg.h>
#include <dgrq.h>
#include <dgxq.h>
#include <dgpkt.h>
#include <dgccall.h>
#include <dgcall.h>
#include <dgexec.h>
#include <comauth.h>


/*
 * R P C _ _ D G _ C A L L _ S T A T E _ N A M E
 *
 * Return the text name of an activity state.  This can't simply be a variable
 * because of the vagaries of global libraries.
 */

#ifdef DEBUG

PRIVATE char *rpc__dg_call_state_name
(
    rpc_dg_call_state_t state
)
{
    static char *names[] = {
        "init",
        "xmit",
        "recv",
        "final",
        "idle",
        "orphan"
    };

    if ((int)state > (int)rpc_e_dg_cs_orphan)
        return("unknown");

    return(names[(int) state]);
}

#endif /* DEBUG */


/*
 * R P C _ _ D G _ C A L L _ X M I T _ F A C K
 *
 * Transmit a "fack" appropriate to the state of the passed "call".
 *
 * Note that it's possible that a call might want to send a fack
 * but not currently be holding onto an RQE (if the call just got
 * dequeued, for example).  In such a case, it can send NULL for the
 * rqe parameter.
 */

PRIVATE void rpc__dg_call_xmit_fack
(
    rpc_dg_call_p_t call,
    rpc_dg_recvq_elt_p_t rqe,
    boolean32 is_nocall
)
{
    unsigned32 *maskp = NULL, num_elements = 0, i, shift;
    rpc_socket_iovec_t iov[3];
    rpc_dg_pkt_hdr_t hdr;
#ifndef MISPACKED_HDR
    rpc_dg_fackpkt_body_t body;
#else
    rpc_dg_raw_fackpkt_body_t body;
#endif
    boolean b;
    rpc_dg_recvq_p_t rq = &call->rq;
    rpc_dg_xmitq_p_t xq = &call->xq;
    rpc_dg_recvq_elt_p_t rqe_p;
    unsigned32 is_rationing;
    unsigned32 low_on_pkts;

    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * Create a pkt header initialized with the prototype's contents.
     */

    hdr = xq->hdr;

    RPC_DG_HDR_SET_PTYPE(&hdr, is_nocall ? RPC_C_DG_PT_NOCALL : RPC_C_DG_PT_FACK);
    
    hdr.flags       = 0;
    hdr.len         = RPC_C_DG_RAW_FACKPKT_BODY_SIZE;
    hdr.fragnum     = rq->next_fragnum - 1;

    /*
     * Create the fack packet's body.
     */

#ifndef MISPACKED_HDR
    body.vers           = RPC_C_DG_FACKPKT_BODY_VERS;
    body.pad1           = 0;
    body.max_tsdu       = xq->max_rcv_tsdu;

    /*
     * Before advertising our max_frag_size, make sure that we have
     * enough packets reserved. We advertise what we can actually
     * receive.
     */
    if (call->n_resvs >= call->max_resvs)
    {
        /*
         * We already have the maximum reservation.
         */
        body.max_frag_size  = xq->max_frag_size;
    }
    else if (RPC_DG_CALL_IS_SERVER(call) &&
             (call->n_resvs == 0 || ((rpc_dg_scall_p_t) call)->call_is_queued))
    {
        /*
         * If the scall doesn't have the reservation, it hasn't got to the
         * execution phase yet. We will advertise the minimum fragment
         * size.
         * Also, a queued scall shouldn't increase the reservation.
         *
         * If this is a callback scall with the private socket, should we
         * try the reservation? Probably not.
         */
        body.max_frag_size = RPC_C_DG_MUST_RECV_FRAG_SIZE;
    }
    else
    {
        /*
         * The ccall with the private socket may not have the reservation
         * yet. We will try the reservation here because 1) we are in the
         * receive state and 2) if we don't advertise larger fragment size,
         * the large OUTs call can not take advantage of the multi-buffer
         * fragment.
         */

        if (rpc__dg_pkt_adjust_reservation(call, call->max_resvs, false))
        {
            body.max_frag_size  = xq->max_frag_size;
        }
        else if (call->n_resvs == 0)
        {
            /*
             * The ccall with the private socket has no reservation.
             */
            body.max_frag_size = RPC_C_DG_MUST_RECV_FRAG_SIZE;
        }
        else
        {
            /*
             * We can receive whatever fits in the reserved packets.
             */
            RPC_DG_NUM_PKTS_TO_FRAG_SIZE(call->n_resvs, body.max_frag_size);
        }
    }

    if (rqe != NULL)
    {
        if (rqe->hdrp == NULL)
            body.serial_num = call->rq.head_serial_num;
        else
            body.serial_num =
                (rqe->hdrp->serial_hi << 8) | rqe->hdrp->serial_lo;
    }
    else
        body.serial_num     = call->rq.high_serial_num;

    body.selack_len     = 0; 
    body.selack[0]      = 0; 

    /*
     * Determine what to advertise as the window size.  Ignoring other
     * considerations, the window size is the amount of buffering we
     * expect the underlying system to provide if we get behind in
     * processing incoming data.
     *
     * However, there are certain situations in which we might want to
     * advertise a smaller window size--
     *
     *     Queued calls are not allowed to add data to their receive
     *     queues.  In this case we would advertise a window size of
     *     0, to tell the sender to stop sending data.
     *
     *     Server call threads that have not yet acquired a packet
     *     reservation.  This situation is treated the same as for queued
     *     calls.  Client threads without reservations are treated
     *     differently; the only way they could have proceeded to this
     *     stage (ie. receiving a response) is if they are using a private
     *     socket with private packets.
     *   
     *     When the system is rationing, advertise a window of 1, since 
     *     this is all that a call is allowed to queue.  
     *     
     *     If the packet's 'low_on_pkts' flag is set, advertise a window
     *     of 2.  The low_on_pkts flag indicates that the number of free
     *     packets is less that two times the number of reservations.
     *     In such a case, it makes no sense to advertise a full window.
     *
     *     Calls whose recvq length is approaching its maximum allowable
     *     value.  In this case we don't want the sender to send any
     *     more than the amount of space available on the receive queue.
     *
     * We need to adjust the window size from the number of fragments
     * to kilobytes as per AES.
     *
     * When talking to the older runtime, which expects the window size
     * in the number of packets, the fack receiver may induce more data
     * in the network. We don't think that this is a problem because the
     * window size is an advisory information.
     *
     * If the max_frag_size is not a multiple of 1024, we advertise less
     * than what we think, which is problematic in rationing and
     * low_on_pkts cases because we may stop(!) the sender. Thus in
     * these two cases we don't adjust the window size.
     */
    is_rationing = rpc__dg_pkt_is_rationing(&low_on_pkts);

    if (call->n_resvs == 0 && RPC_DG_CALL_IS_SERVER(call))
    {
        body.window_size = 0;
    }                          
    else if (is_rationing)
    {
        body.window_size = 1;
    }                         
    else if (low_on_pkts)
    {
        body.window_size = 2;
    }                         
    else                 
    {
        /*
         * If high_rcv_frag_size > RPC_C_DG_MUST_RECV_FRAG_SIZE, that means
         * we have advertised MBF and the sender can cope with it. Thus,
         * each slots on the recvq takes xq->max_frag_size.
         *
         * Otherwise, we make a conservative (or wild?) guess that the
         * sender will send a single buffer fragment.
         */
        if (rq->high_rcv_frag_size > RPC_C_DG_MUST_RECV_FRAG_SIZE)
        {
            body.window_size = ((rq->max_queue_len - rq->queue_len)
                                * xq->max_frag_size) >> 10;
        }
        else
        {
            body.window_size = ((rq->max_queue_len - rq->queue_len)
                                * RPC_C_DG_MUST_RECV_FRAG_SIZE) >> 10;
        }
        body.window_size = MIN(rq->window_size, body.window_size);
    }

    /*
     * See if we need to send selective ack information. 
     */

    if (RPC_DG_FRAGNUM_IS_LT(rq->next_fragnum, rq->high_fragnum))
    { 
        /*
         * Determine the number of elements in the selective ack mask
         * array, worrying about wrap-around.  If we need only one, just
         * use the fack packet.  Otherwise, we need to allocate an array
         * to hold the entire mask.  Either way, initialize the mask
         * to 0.
         */

        num_elements = (((unsigned16)(rq->high_fragnum - rq->next_fragnum)) 
                       / 32) + 1;
        
        if (num_elements == 1)
            maskp = &body.selack[0];
        else
        {
            RPC_MEM_ALLOC(maskp, unsigned32 *, num_elements * sizeof *maskp,
                RPC_C_MEM_DG_SELACK_MASK, RPC_C_MEM_NOWAIT); 
        }                                                   
        for (i = 0; i < num_elements; i++)
            *(maskp + i) = 0;
          
        body.selack_len = num_elements;

        /* 
         * Loop through the list of received packets which have fragnums
         * greater than that which is explicitly mentioned in this fack,
         * and add them to the selective ack bit mask.  If there are
         * in-order frags, they would have been explicitly acked, so
         * start with the next frag.  If there are no in-order frags,
         * start at the head of the queue.
         */

        rqe_p = (rq->last_inorder != NULL) ? rq->last_inorder->next : rq->head;

        while (rqe_p != NULL )
        {         
            /*
             * Calculate the number of bits we would have to shift the
             * current fragnum into a bitmask of unlimited size, worrying
             * about wrap- around.  Munge the shift value to get the
             * bit into the correct mask element.  Currently, shift will
             * almost always be less than 32, so the assignment is
             * effectively       
             *                *maskp &= 1 << shift;
             */

            shift = rqe_p->hdrp->fragnum - (unsigned16) rq->next_fragnum;
            *(maskp + (shift/32)) |= 1 << (shift%32);
            rqe_p = rqe_p->next;
        }
    }                                  
#else
    !!! 
#endif

    /*
     * Setup the iov and send the packet.
     */

    iov[0].iov_base = (byte_p_t) &hdr;
    iov[0].iov_len  = RPC_C_DG_RAW_PKT_HDR_SIZE;
    iov[1].iov_base = (byte_p_t) &body;
    iov[1].iov_len  = hdr.len;

    if (num_elements > 1)
    {
        /*
         * If we need to send multiple selective ack masks, subtract
         * the length of the one built into the fack packet from the
         * second iov element, and setup a third iov element to hold
         * the extended mask.  Note that in setting the header length,
         * we need to remember that the fack body size includes the size
         * of one mask element, thus the subtraction.
         */

        iov[1].iov_len -= sizeof *maskp;
        hdr.len = RPC_C_DG_RAW_FACKPKT_BODY_SIZE + 
                  ((num_elements - 1) * sizeof *maskp);
        iov[2].iov_base = (byte_p_t) maskp;
        iov[2].iov_len  = num_elements  * sizeof *maskp;
    }

    rpc__dg_xmit_pkt(call->sock_ref->sock, call->addr, iov, num_elements > 1 ? 3 : 2, &b);

    RPC_DBG_PRINTF(rpc_e_dbg_xmit, 5, 
        ("(rpc__dg_call_xmit_fack) %lu.%u ws %lu, tsdu %lu, fs %lu, mask [0x%x]\n",
        hdr.fragnum, body.serial_num, body.window_size, 
        body.max_tsdu, body.max_frag_size,
        num_elements > 1 ? *maskp : body.selack[0]));

    /*
     * Free any memory we may have allocated.
     */

    if (num_elements > 1)
    {
        RPC_MEM_FREE(maskp, RPC_C_MEM_DG_SELACK_MASK);
    }
}


/*
 * R P C _ _ D G _  C A L L  _ X M I T
 * 
 * Transmit one blast of packets.
 *
 * This routine expects that a blast's worth of packets have been queued
 * and that the fack request frequency has been set.  It is the callers
 * responsibility to put the queue into a sensible state, and to make 
 * sure that the last packet is tagged appropriately. 
 */

PRIVATE void rpc__dg_call_xmit
(
    rpc_dg_call_p_t call,                     
    boolean32 block
)
{              
    unsigned32 num_sent, extra_fack = 0;
    unsigned8 freqs_total;
    boolean rexmiting = true;
    rpc_dg_xmitq_p_t xq = &call->xq;
    unsigned8 blast_size = xq->blast_size;
    rpc_dg_xmitq_elt_p_t xqe = xq->rexmitq;
    /*
     * Below is a table for mapping window sizes to the number of fack
     * requests per window that would be appropriate for that window.
     * Inducing an appropriate number of facks per RTT serves two purposes:
     *
     *   1) it increases the granularity of the ack clock we use to do
     *   transmissions, allowing us to decrease the blast size needed to
     *   keep the congestion window full, and to achieve a better temporal
     *   spacing of the packets within the RTT.
     
     *   2) it provides redundancy, increasing the likelihood that we will
     *   detect packet loss without having to timeout.
     *
     * The trade-off is the increase in backward traffic.  The table below
     * specifies what we think are the best compromises for a given window
     * size.  (The macro that follows just helps to hide the details.)
     * Whenever the advertised window size stored in the xmitq is updated
     * (do_fack_body), this table is consulted for an an appropriate
     * 'outstanding fack' count.  Note that the table is actually declared
     * in the routine do_fack_body, where it is used.
     */
     static unsigned8 window_size_to_freqs_out[] = 
                    {   0, 1, 2, 2, 2,
                           3, 3, 3, 3,
                           3, 4, 4, 4,
                           4, 4, 4, 4 };



    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * Determine an appropriate number of outstanding fack requests
     * per RTT.
     */
    freqs_total = (call)->xq.window_size > 16 ?
                               (call)->xq.window_size / 4 : 
                               window_size_to_freqs_out[(call)->xq.window_size];

    /*
     * If are more than 1 away from our target, set extra_fack to
     * the number of the packet within this blast which should request
     * a fack (The last packet always asks for a fack).  We don't set
     * this value above 2 because in the case where the sender is slower
     * than the receiver, it will never be able to fully open the
     * congestion window and will end up requesting facks on every packet.
     */

    if (freqs_total - xq->freqs_out > 1)
    {
        extra_fack = blast_size / 2;
    }

    /*
     * Send up to a blast size worth of retransmits/new-transmits.
     */
              
    for (num_sent = 1; num_sent <= blast_size; num_sent++)
    {
        /*
         * If we have gotten to the end of the retransmit queue, switch
         * over to the regular queue.
         */

        if (xqe == NULL && rexmiting)
        {
            xqe = xq->first_unsent;
            rexmiting = false;
        }                     

        /*
         * If we have gotten to the end of the regular queue, we're done
         * early.
         */

        if (xqe == NULL)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_xmit, 5, 
                           ("(rpc__dg_call_xmit) Premature end of queue\n")); 
            break;
        }

        if (RPC_DG_FLAG_IS_SET(xq->base_flags, RPC_C_DG_PF_FRAG))    
        {
            if (num_sent == extra_fack || num_sent == blast_size)
            {
                xqe->flags &= ~ RPC_C_DG_PF_NO_FACK;
                /*
                 * Make note of newly outstanding fack request.
                 */
                xq->freqs_out++;
            }
            else
                xqe->flags |= RPC_C_DG_PF_NO_FACK;
        }
        else
        {
            xq->freqs_out++;
        }
    
        /*
         * Mark xqe as part of the current congestion window.
         */

        xqe->in_cwindow = true;

        rpc__dg_xmitq_elt_xmit(xqe, call, block);
                   

        if (rexmiting)
        {
            xqe = xqe->next_rexmit;
            RPC_DG_STATS_INCR(dups_sent);
        }
        else
        {
            xqe = xqe->next;
            xq->first_unsent = xqe;
        }
    }

    /*
     * Bump congestion window count, and adjust high-water mark if
     * appropriate.
     */

    xq->cwindow_size += num_sent - 1;
    if (xq->cwindow_size > xq->high_cwindow)
       xq->high_cwindow = xq->cwindow_size;

    xq->timestamp = rpc__clock_stamp();   
    xq->rexmitq = NULL;
    xq->blast_size = 0;
}



/*
 * R P C _ _ D G _ C A L L _ X M I T Q _ T I M E R
 *
 * Do any time-based retransmissions for a call's transmit queue.
 */

PRIVATE void rpc__dg_call_xmitq_timer
(
    rpc_dg_call_p_t call
)
{
    rpc_dg_xmitq_p_t xq = &call->xq;
    rpc_dg_xmitq_elt_p_t xqe;

    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * If the call is in an error state there's no need to keep
     * transmitting things off the xmitq...  with the exception of the
     * call faulted error where the fault pkt is on the xmitq kinda
     * masquerading as a response pkt.  The fault response may (depends
     * if the call is idempotent or not) be required to be "robust" and
     * receive an ack.
     */
    if (call->status != rpc_s_ok 
        && call->status != rpc_s_call_faulted)
    {
        return;
    }

    /*
     * If there's nothing on the xmitq or we haven't reached the time
     * to do retransmits for this xmitq, just return now.
     */

    xqe = xq->head;

    if (xqe == NULL || ! rpc__clock_aged(xq->timestamp, xq->rexmit_timeout))
    {
        return;
    }

    /*
     * See if we've been waiting too long for an acknowledgement,
     * and if we have, blow off the call.
     */

    if (rpc__dg_xmitq_awaiting_ack_tmo(xq, call->com_timeout_knob))
    {
        rpc__dg_call_signal_failure(call, rpc_s_comm_failure);
        return;
    }

    /*
     * Transmit the first pkt on the xmitq.
     *
     * Note how we blithely ignore window constraints.  If the receiver
     * has closed the window on us, we want to occasionally elicit acks
     * so that we'll know when window space opens up.
     *
     * Don't "rexmit" a pkt that has not yet been sent.  If the pkt is
     * the first unsent, then just setting the blast size to 1 (with
     * an empty rexmitq) will cause START_XMIT to send this pkt and
     * properly manage the xq pointers.
     */

    if (xqe != xq->first_unsent)
    {
        xq->rexmitq = xqe;
        xqe->next_rexmit = NULL;
    }

    /*
     * Adjust the re-xmit age for the next time (i.e., be a good network
     * citizen and backoff the next re-xmit).
     */

    xq->rexmit_timeout = MIN(RPC_C_DG_MAX_REXMIT_TIMEOUT, (xq->rexmit_timeout << 1));

    /*                                                 
     * If this packet is considered part of the current congestion
     * window, remove it.  This may also require decrementing the 
     * count of outstanding facks.
     */
    
    if (xqe->in_cwindow)
    {    
        xq->cwindow_size--;
        xqe->in_cwindow = false;

        if (! RPC_DG_FLAG_IS_SET(xqe->flags, RPC_C_DG_PF_NO_FACK) ||
            RPC_DG_FLAG_IS_SET(xqe->flags, RPC_C_DG_PF_LAST_FRAG))
        {
            xq->freqs_out--;
        }
    }

    xq->blast_size = 1;
    
    RPC_DBG_PRINTF(rpc_e_dbg_xmit, 4, (
        "(rpc__dg_call_xmitq_timer) re-xmit'ing %lu.%u [%s]\n", 
        xq->hdr.seq, xqe->fragnum, rpc__dg_act_seq_string(&xq->hdr)));

    RPC_DG_START_XMIT(call);
}


/*
 * R P C _ _ D G _ C A L L _ I N I T
 *
 * Initialize the parts of call handles (rpc_dg_call_t) that are
 * initialized in the same fashion for both client and server call handles.
 *
 * No lock requirements. The call is private at this point.
 */

PRIVATE void rpc__dg_call_init
(
    rpc_dg_call_p_t call
)
{
    rpc_clock_t now = rpc__clock_stamp();

    RPC_DG_CALL_LOCK_INIT(call);

    call->c.protocol_id = RPC_C_PROTOCOL_ID_NCADG;

    call->next                  = NULL;
    call->state                 = rpc_e_dg_cs_init;
    call->status                = rpc_s_ok;
    call->state_timestamp       = now;

    RPC_DG_CALL_COND_INIT(call);

    rpc__dg_xmitq_init(&call->xq);
    rpc__dg_recvq_init(&call->rq);

    call->sock_ref              = NULL;
    call->actid_hash            = 0;
    call->key_info              = NULL;
    call->auth_epv              = NULL;
    call->addr                  = NULL;
    memset(&call->timer, 0, sizeof(call->timer));
    call->last_rcv_timestamp    = 0;
    call->start_time            = now;
    call->high_seq              = 0;
    call->pkt_chain             = NULL;
    call->com_timeout_knob      = 0;
    call->refcnt                = 0;
    call->n_resvs               = 0;
    call->n_resvs_wait          = 0;
    call->max_resvs             = 0;

    call->blocked_in_receive    = false;
    call->priv_cond_signal      = false;
    call->stop_timer            = false;
    call->is_cbk                = false;
    call->is_in_pkt_chain       = false;

    memset(&call->thread_id, 0, sizeof(call->thread_id));
}


/*
 * R P C _ _ D G _ C A L L _ F R E E
 *
 * Free the resources referenced by the common call handle header
 * (rpc_dg_call_t).
 * 
 * This is an low level routine that should be invoked by the higher
 * level call handle type specific (i.e. ccall, scall) free routines.
 * The higher level routines are responsible for freeing resources
 * associated with the non-common portion of the call handle as well
 * as freeing the call handle object storage.
 * 
 * This routine has the side effect of releasing the call's lock (which
 * is only natural since the call structure, including it's lock are
 * destroyed).
 */

PRIVATE void rpc__dg_call_free
(
    rpc_dg_call_p_t call
)
{
    unsigned32 st;

    RPC_DG_CALL_LOCK_ASSERT(call); 

    rpc__naf_addr_free(&call->addr, &st);

    rpc__dg_xmitq_free(&call->xq, call);
    rpc__dg_recvq_free(&call->rq);
                         
    /*
     * In the event that this call was using a private socket, it is
     * important that the socket is released from the call handle only
     * *after* the xmit/recv queues have been freed.  Every private socket
     * is allocated a recv/xmit packet pair, which is available for use
     * by the call.  Since at any given time these packets may be queued
     * on the call handle, we want to make sure that they are reassociated
     * with the socket before we lose our reference to it.  Freeing the
     * queues before the socket ensures that this will happen.
     */
    rpc__dg_network_sock_release(&call->sock_ref);

    if (call->key_info)
        RPC_DG_KEY_RELEASE(call->key_info);
    CLOBBER_PTR(call->key_info);
    
    RPC_DG_CALL_UNLOCK(call);
    RPC_DG_CALL_LOCK_DELETE(call);
    RPC_DG_CALL_COND_DELETE(call);

    CLOBBER_PTR(call->next);
    /* common call handle header may no longer be referenced */
}


/*
 * R P C _ _ D G _ C A L L _ W A I T 
 * 
 * Wait for a change in the call's state using the call's condition var
 * Note, the caller is responsible for determing if the purpose for
 * waiting was satisfied - don't assume anything.
 *
 * The 'event' argument indicates if the caller is waiting for a local
 * event to occur.  Currently there are two such events on which a call 
 * might need to block: 1) waiting for a packet pool reservation to 
 * become available, or 2) waiting for a datagram packet to become 
 * available.
 *   
 * The local/remote distinction is only meaningful for calls that are
 * using private sockets.  Such calls wait for local events using the
 * call's condition variable, and wait for remote events by blocking
 * in recvfrom.  Calls using a shared socket always wait on a condition
 * variable.
 *
 * Detect pending cancels if we're called with general cancelability
 * enabled (i.e. user cancels or orphans) and arrange for proper cancel
 * handling.  This is the central place where others synchronously detect
 * that a call now has an error associated with it (see
 * dg_call_signal_failure()).
 * 
 * This routine *must* be called with the call's mutex held.  This routine
 * returns with the lock held.
 */    

PRIVATE void rpc__dg_call_wait
(
    rpc_dg_call_p_t call,
    rpc_dg_wait_event_t event,
    unsigned32 *st
)
{
    boolean is_server = RPC_DG_CALL_IS_SERVER(call);
    rpc_dg_ccall_p_t ccall = (rpc_dg_ccall_p_t) call;
        
    /* !!! RPC_UNLOCK_ASSERT(); */
    RPC_DG_CALL_LOCK_ASSERT(call); 

    *st = call->status;
    if (*st != rpc_s_ok)
            return;

    /*
     * Anytime the call handle is unlocked, it is possible for the listener
     * (or timer) thread to modify its xmitq, and signal it to do a send.
     * If the signal occurs during some window in which the call handle
     * is unlocked, we'll miss it.  Therefore, check here, before
     * waiting(), to see if there is any data to transmit.  (See bottom
     * of routine for more information about transmissions.)
     *
     * If we do transmit data, return immediately.  In the normal case
     * the caller is waiting for queue space to open up on the xmitq,
     * which will have happened by calling call_xmit().
     */

    if (RPC_DG_CALL_READY_TO_SEND(call))
    {
        rpc__dg_call_xmit(call, true);
        return;
    }

    /*
     * If this is a client call handle make sure that we haven't been
     * asked to run a callback prior to waiting.  If we detect a local
     * cancel, process it.
     */

    if ((! is_server && ! ccall->cbk_start) || is_server) 
    {
        /*
         * If the call is using a private socket, and is waiting
         * for a network event, let it wait in recvfrom. 
         */ 
        if (call->sock_ref->is_private && 
            event == rpc_e_dg_wait_on_network_event)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 5, (
                    "(rpc__dg_call_wait) waiting on network event\n"));

            rpc__dg_network_select_dispatch(ccall->c.sock_ref->sock, 
                (pointer_t) ccall->c.sock_ref, (boolean32) true, st);
        }
        else
        {

#ifndef _PTHREAD_NO_CANCEL_SUPPORT
            DCETHREAD_TRY {
                RPC_DG_CALL_COND_WAIT(call);
            } DCETHREAD_CATCH(dcethread_interrupt_e) {
                rpc__dg_call_local_cancel(call);
            } DCETHREAD_CATCH_ALL(THIS_CATCH) {
                rpc__dg_call_signal_failure(call, 
                           (unsigned32) -1 /* !!! rpc_s_unknown_exception */);
            } DCETHREAD_ENDTRY
#else
            /*
             * We certainly can't try to detect a non-existant cancel; 
             * just wait (and avoid the TRY/CATCH overhead).
             */
            RPC_DG_CALL_COND_WAIT(call);
#endif
        }

        *st = call->status;
        if (*st != rpc_s_ok)
            return;
    }

    /*
     * If this is a client call handle and we've been asked to do a
     * callback, now's the time to do it.  While the callback is running,
     * we must unlock the original CCALL.  Additionally, we lock and bump
     * the cbk_scall's reference count (mimicing the normal processing
     * of handing off a scall reference to the call executor).
     *
     * To provide the same server call executor thread environment
     * as a 'real' server call executor, disable / restore cancelability
     * while in the server call processing (see rpc__dg_call_local_cancel()).
     */

    if (! is_server) 
    {
        if (ccall->cbk_start) 
        {
            int oc;

            assert(ccall->cbk_scall != NULL);
            ccall->cbk_start = false;

            RPC_DG_CALL_LOCK(&ccall->cbk_scall->c);
            RPC_DG_CALL_REFERENCE(&ccall->cbk_scall->c);
            ccall->cbk_scall->has_call_executor_ref = true;

            RPC_DG_CALL_UNLOCK(&ccall->c);

            oc = dcethread_enableinterrupt_throw(0);
            rpc__dg_execute_call((pointer_t) &ccall->cbk_scall->c, false);
            dcethread_enableinterrupt_throw(oc);

            RPC_DG_CALL_LOCK(&ccall->c);
        }
    }

    /*
     * If there is data to transmit (or retransmit), send it now.  Transmissions
     * have been moved into this routine based on the following observations.
     * 
     *      - The ability to transmit (open congestion window space)
     *        occurs asynchronously to the execution of the thread.  A
     *        transmitting thread is only interested in queueing data,
     *        it doesn't care/know about the actual (re)transmission.
     *        threads do not wait on the ability to transmit directly.
     *
     *      - When cwindow space opens up, the thread is signalled to
     *        do the send.  The thread will have been waiting on a
     *        different condition (probably space on the xmitq) but will
     *        need to check first if there is data that needs to be sent.
     *
     *      - Each time a thread needs to wait for any condition (e.g.
     *        a packet to become available) it must also check that it
     *        wasn't woken to do a send.
     *
     *      - Whenever a thread is awoken to do a send, it *always* wants
     *        to do the send immediately, before checking on whatever
     *        other condition it was waiting on.
     *
     *      - It is error prone to require each caller of call_wait to
     *        perform its own check for transmissable data, as well as
     *        whatever condition it was really interested in.
     */  

    if (RPC_DG_CALL_READY_TO_SEND(call))
        rpc__dg_call_xmit(call, true);
}


/*
 * R P C _ _ D G _ C A L L _ S I G N A L
 * 
 * Signal a change in the call's state.
 */

PRIVATE void rpc__dg_call_signal
(
    rpc_dg_call_p_t call
)
{
    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * If the call is using a private socket, and is currently blocked in
     * a call to recvfrom, we'll need to post a cancel against its thread
     * in order to wake it.  Otherwise, just signal the call's condition
     * variable.
     */
    if (call->sock_ref->is_private && call->blocked_in_receive)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 5, (
                    "(rpc__dg_call_signal) cancelling private socket thread\n"));

        call->priv_cond_signal = true;
        dcethread_interrupt_throw(call->thread_id);
    }
    else
    {
        /* 
         * normally (always?) at most one waiter will be present 
         */ 
        RPC_DG_CALL_COND_SIGNAL(call);  
    }
}


/*
 * R P C _ _ D G _ C A L L _ S I G N A L _ F A I L U R E
 * 
 * Signal a change in the call's state due to a failure.
 *
 * The idea is that is that dg_call_signal_failure() and dg_call_wait()
 * are the only routines that reference the the call's error status
 * field.  Additionally, this field's value is what will be returned to
 * the stubs.  Retain the 1st failure status.
 */

PRIVATE void rpc__dg_call_signal_failure
(
    rpc_dg_call_p_t call,
    unsigned32 stcode
)
{
    RPC_DG_CALL_LOCK_ASSERT(call);

    RPC_DBG_GPRINTF(
        ("(rpc__dg_call_signal_failure) %s st = 0x%x (orig st = 0x%x) [%s]\n", 
        RPC_DG_CALL_IS_SERVER(call) ? "SCALL" : "CCALL", 
        stcode, call->status, rpc__dg_act_seq_string(&call->xq.hdr)));

    if (call->status == rpc_s_ok)
        call->status = stcode;

    rpc__dg_call_signal(call);
}


/*
 * R P C _ _ D G _ C A L L _ X M I T Q _ P U S H
 *
 * Push out any queue transmit data.
 */

PRIVATE void rpc__dg_call_xmitq_push
(
    rpc_dg_call_p_t call,
    unsigned32 *st
)
{
    rpc_dg_xmitq_p_t xq = &call->xq;

    RPC_DG_CALL_LOCK_ASSERT(call);

    *st = rpc_s_ok;
                                   
    /* 
     * Normally, we want to loop in here while the (re)transmission logic
     * sends whatever data remains on the xmitq.  In the case where the
     * queue is quiescent, this requires a little bootstrapping.  This
     * will be the case if either 1) no data has been sent from this
     * xmitq yet, or 2) everyting on the queue was already facked and
     * the queue is now empty.  In either case, we can't wait for a fack
     * or a timeout before doing the next transmit.  Both cases are 
     * identified by a NULL xq->head, and are handled by setting the
     * blast size to 1 so that the partial packet get sent immediately.
     */

    if (xq->head == NULL)
        xq->blast_size = 1;
    /*
     * If the queue is not empty and has no outstanding fack request,
     * send two fragments.
     *
     * call_transmit_int() does not start the window until the two
     * fragments are filled. Thus, we could have one full and one
     * partial fragments here.
     */
    else if (xq->freqs_out == 0)
        xq->blast_size = 2;


    /*
     * Turn around the connection and force (and wait for) all queued
     * up packets to be transmitted; the listener handles retransmits.
     * If the call has an error condition we're done.
     */

    xq->push = true;  

    /*
     * Ensure that a final partial packet gets added to the queue.
     * Note that this call must follow the setting of the push flag 
     * so that append_pp knows whether to set the frag bit in the 
     * packet header.
     */

    rpc__dg_xmitq_append_pp(call, st);
    if (*st != rpc_s_ok)
        return;

    while (true) 
    {
        if (RPC_DG_CALL_READY_TO_SEND(call))
            rpc__dg_call_xmit(call, true);
        if (xq->first_unsent == NULL)
            break;
        rpc__dg_call_wait(call, rpc_e_dg_wait_on_network_event, st);
        if (*st != rpc_s_ok)
            return;
    }
}


/*
 * R P C _ _ D G _ C A L L _ R E C V Q _ I N S E R T
 *
 * Insert a receive queue element (packet) into a receive queue in the
 * right place.  Drop duplicate packets and generate a "fack" if one
 * is requested (even for duplicates).  Return "true" iff we actually
 * add the packet to the receive queue.  The "rqe_is_head_inorder" output
 * param is set to "true" iff the passed receive queue element became
 * head of the queue and is in-order.
 */

PRIVATE boolean rpc__dg_call_recvq_insert
(
    rpc_dg_call_p_t call,
    rpc_dg_recvq_elt_p_t rqe,
    boolean *wake_thread
)
{
    rpc_dg_recvq_p_t rq = &call->rq;
    rpc_dg_recvq_elt_p_t scan_rqe, prev_scan_rqe;
    unsigned16 fragnum = rqe->hdrp->fragnum;
    unsigned16 next_fragnum = rq->next_fragnum;
    unsigned16 curr_serial = (rqe->hdrp->serial_hi << 8) | rqe->hdrp->serial_lo;
    boolean added_to_queue;

    RPC_DG_CALL_LOCK_ASSERT(call);

    added_to_queue = true;
    *wake_thread = false;  

    /*
     * We could rework things to lessen the processing for the single
     * pkt stream case, but at this point this "additional" overhead's
     * performance impact has to be a second order effect...
     */

    /*
     * Remember if we're recving frags.  To insulate us from potentially
     * bogus transmitters, once we're recving frags we stay that way.
     * It's cheaper and more effective to do this than to "assert" it.
     */
    rq->recving_frags |= RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_FRAG);

    /*
     * The receiver needs to keep track of the highest serial number it
     * has seen so far, just in case it ever needs to send a fack but
     * isn't holding onto an rqe at the time.
     */
    if (RPC_DG_SERIAL_IS_LT(rq->high_serial_num, curr_serial))
        rq->high_serial_num = curr_serial;

    /*
     * The receiver needs to keep track of the largest fragment size it
     * has seen so far, so that when it becomes the sender it can start
     * sending that much data.
     */

    if (rqe->frag_len > rq->high_rcv_frag_size) {
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 7,
                ("(rpc__dg_call_recvq_insert) Set high_rcv_frag %lu was %lu\n",
                        rqe->frag_len, rq->high_rcv_frag_size));
        rq->high_rcv_frag_size = rqe->frag_len;

        /*
         * If high_rcv_frag_size > RPC_C_DG_MUST_RECV_FRAG_SIZE, that means
         * we have advertised MBF and the sender can cope with it. We need
         * to adjust the max_queue_len, if possible.
         *
         * Note: MBF (Multi-Buffer Fragments) implies that the call has the
         * more than 1 packet reservations. Also if rq->max_queue_len !=
         * RPC_C_DG_MAX_RECVQ_LEN, then the max_queue_len is already
         * adjusted and no need for re-adjustment (because the packet
         * reservation gets adjusted only once).
         *
         * LBF (Large-Buffer Fragment) and the fragment size kept between
         * calls means that the first request pdu could be greater than
         * RPC_C_DG_MUST_RECV_FRAG_SIZE.
         */
        if (call->n_resvs > 0
            && rq->high_rcv_frag_size > RPC_C_DG_MUST_RECV_FRAG_SIZE
            && rq->max_queue_len == RPC_C_DG_MAX_RECVQ_LEN)
        {
            unsigned32 max_queue_len =
                RPC_C_DG_MAX_RECVQ_LEN / MIN(call->n_resvs, call->max_resvs);

            /*
             * Update rq->max_queue_len.
             *
             * If the current queue_len already exceeds the new
             * max_queue_len, we can't adjust it here.
             *
             * Also, we must have at least one inorder fragment. Otherwise
             * nothing gets dequeued.
             */
            if (rq->queue_len <= max_queue_len
                && rq->inorder_len > 0)
            {
                rq->max_queue_len = max_queue_len;
                RPC_DBG_PRINTF(rpc_e_dbg_recv, 7,
                       ("(rpc__dg_call_recvq_insert) Set max_queue_len %lu\n",
                                rq->max_queue_len));
            }
        }
    }

    /*
     * Scalls without packet pool reservations are not allowed to queue
     * packets.  The two cases in which we encounter this condition are
     * for queued calls, and scalls that were started when no reservations
     * were available.
     *
     * The callback scall should be allowed to queue data without the
     * reservation if using the private socket. However, since the
     * callback is not officially supported (and the scall doesn't use
     * the private socket), we make no distinction. (Trying a little
     * performance recovery...)
     *
     * Note that it is ok for a ccall to queue data before it has made
     * a packet reservation, since ccalls may have private packets
     * associated with their socket refs.
     */  

    if (RPC_DG_CALL_IS_SERVER(call) && call->n_resvs == 0)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 6, (
            "(rpc__dg_call_recvq_insert) dropping queued/un-setup call's pkt\n")); 

        added_to_queue = false;
    }
    else if (fragnum == next_fragnum)
    {
        /*
         * This is the fast path.  First make sure that the recvq does not
         * already contain the max allowable packets; if so, we can't accept
         * this one.  
         */

        if (rq->queue_len == rq->max_queue_len)
        {
            added_to_queue = false;
        }   
        
        /*
         * If we are rationing packets, we can only accept a packet if
         * it is the next immediately usable packet for this call.  This
         * will be the case if the current packet will become the new
         * head of this call's recvq.
         *
         * Another check needs to be made to make sure that the call
         * executor call isn't holding onto a packet (in dg_execute_call)
         * before the call's WAY has been done.  The call thread will set
         * a flag in the recvq to indicate that it's done with its WAY
         * processing.
         */
        else if (rpc__dg_pkt_is_rationing(NULL) &&
                 (rq->last_inorder != NULL || 
                                       (! rq->is_way_validated && fragnum != 0)))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 1, 
                ("(rpc__dg_call_recvq_insert) rationing, not 'next' pkt, fc %lu pkt %lu [%s]\n",
                rpc_g_dg_pkt_pool.free_count, rpc_g_dg_pkt_pool.pkt_count, 
                rpc__dg_act_seq_string(&call->xq.hdr)));

            added_to_queue = false;
        }

        /*
         * Otherwise, it's the next in-order packet and we add it to
         * the queue.  If there are currently no in-order pkts, we know
         * that this pkt will be inserted at the head of the queue and
         * make the head in-order.  Otherwise, there are already in-order
         * fragments on the queue (hence the head is already in-order).
         * We insert this frag after the "last_inorder" frag and scan
         * to locate the new "last_inorder" frag.  In both cases, the
         * "last_inorder" and "next_fragnum" are updated appropriatly.
         */

        else
        { 
            if (rq->last_inorder == NULL)
            {
                rqe->next = rq->head;
                rq->head = rqe;
                rq->head_fragnum = fragnum;
            }
            else
            {
                rqe->next = rq->last_inorder->next;
                rq->last_inorder->next = rqe;
            }
    
            prev_scan_rqe = rqe;
            scan_rqe = rqe->next;
            next_fragnum++;
            rq->inorder_len++;
    
            while (scan_rqe != NULL && scan_rqe->hdrp->fragnum == next_fragnum)
            {
                prev_scan_rqe = scan_rqe;
                scan_rqe = scan_rqe->next;
                next_fragnum++;
                rq->inorder_len++;
            }
    
            rq->last_inorder = prev_scan_rqe;
            rq->next_fragnum = next_fragnum;
        }
    }
    else  if (RPC_DG_FRAGNUM_IS_LT(fragnum, next_fragnum))
    {
        /*
         * This is an old fragment and we don't add it to the queue.
         */

        added_to_queue = false;
        RPC_DG_STATS_INCR(dups_rcvd);
    }                                      
    else
    {      
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 6, (
            "(rpc__dg_call_recvq_insert) recv out of order %lu.%u, next_fragnum = .%u\n", 
            rqe->hdrp->seq, fragnum, next_fragnum));

        RPC_DG_STATS_INCR(oo_rcvd);

        /*
         * This is not an in-order fragment (i.e. the head can not now
         * become in-order nor can the "last_inorder" pointer require
         * updating); it may also be a duplicate. 
         *
         * If we're currently rationing packets, then we must drop this
         * one.
         */

        if (rpc__dg_pkt_is_rationing(NULL))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 1, 
                ("(rpc__dg_call_recvq_insert) rationing, oo pkt, fc %lu pkt %lu [%s]\n",
                rpc_g_dg_pkt_pool.free_count, rpc_g_dg_pkt_pool.pkt_count, 
                rpc__dg_act_seq_string(&call->xq.hdr)));

            added_to_queue = false;
        }


        /*
         * Don't accept the packet if it is greater than max_recvq_len
         * away from the first packet (to be) queued.  This check avoids the
         * possibility of having a queue of max length (which can't be
         * added to), but which doesn't contain the next awaited packet.
         */  

        else if ((rq->head != NULL &&
                  RPC_DG_FRAGNUM_IS_LT(rq->head_fragnum, next_fragnum) &&
                  RPC_DG_FRAGNUM_IS_LTE(rq->head_fragnum +
                                        rq->max_queue_len, fragnum))
                 || RPC_DG_FRAGNUM_IS_LTE(next_fragnum +
                                          rq->max_queue_len, fragnum))
        {
            added_to_queue = false;
        }

        /*
         * If the queue is empty, the frag becomes the head.  Otherwise,
         * we must scan the queue for the proper insertion point and
         * ensure this isn't a duplicate.  If there are no in-order
         * fragments, the scan starts at the head of the queue.  If there
         * ARE in-order frags, the scan starts at the first frag after
         * the last in-order frag.
         */

        else
        {
            if (rq->head == NULL)
            {
                rq->head = rqe;
                rq->head_fragnum = fragnum;
            }
            else 
            {
                if (rq->last_inorder == NULL) 
                {
                    scan_rqe = rq->head;
                    prev_scan_rqe = NULL;
                }
                else 
                {
                    scan_rqe = rq->last_inorder->next;
                    prev_scan_rqe = rq->last_inorder;
                }
            
                while (scan_rqe != NULL && 
                       RPC_DG_FRAGNUM_IS_LT(scan_rqe->hdrp->fragnum, fragnum))
                {
                    prev_scan_rqe = scan_rqe;
                    scan_rqe = scan_rqe->next;
                }
    
                if (scan_rqe != NULL && fragnum == scan_rqe->hdrp->fragnum)
                {
                    added_to_queue = false;
                }
                else
                {
                    rqe->next = scan_rqe;
    
                    /* 
                     * Before setting the pointer in the up-stream packet,
                     * check to see if we just pointed this packet at the
                     * previous head of the list.  If so, this packet becomes
                     * the new head.
                     */
    
                    if (scan_rqe == rq->head)
                    {
                        rq->head = rqe;
                        rq->head_fragnum = fragnum;
                    }
                    else
                        prev_scan_rqe->next = rqe;
                }
            }
        }
    }

    /*
     * We *always* fall through to this common processing.
     */

    /*
     * if we're accepting the packet, update some queue state.
     */
    if (added_to_queue)
    {
        rq->queue_len++;
        
        /*
         * So that the selective ack code can easily determine the length
         * of the mask needed for a fack body, keep track of the highest
         * numbered fragment we have on the queue.
         */
        if (RPC_DG_FRAGNUM_IS_LT(rq->high_fragnum, fragnum))
        {
            rq->high_fragnum = fragnum;
        }

        /*
         * The data stream is complete when our queue has inorder data
         * and it's not a fragmented stream, or if the last inorder
         * fragment has the last_frag bit set.
         *
         * If rq->last_inorder->hdrp == NULL (can happen iff last_inorder ==
         * head), we must have added the out of order fragment, that means
         * the data stream is incomplete.
         */
        if (rq->last_inorder != NULL &&
            (! rq->recving_frags ||
             (rq->last_inorder->hdrp != NULL &&
              RPC_DG_HDR_FLAG_IS_SET(rq->last_inorder->hdrp,
                                     RPC_C_DG_PF_LAST_FRAG))))
        {
            rq->all_pkts_recvd = true;
        }
    }

    RPC_DBG_PRINTF(rpc_e_dbg_recv, 7, 
        ("(rpc__dg_call_recvq_insert) recv %s %lu.%u.%u len=%lu %s\n",
        rpc__dg_pkt_name(RPC_DG_HDR_INQ_PTYPE(rqe->hdrp)),
        rqe->hdrp->seq, 
        fragnum, (rqe->hdrp->serial_hi << 8) | rqe->hdrp->serial_lo,
        rqe->hdrp->len,
        (rq->recving_frags && 
            ! RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_NO_FACK)) ? 
                "frq" : ""));
    RPC_DBG_PRINTF(rpc_e_dbg_recv, 7,
        ("(rpc__dg_call_recvq_insert) recv frag_len %lu\n", rqe->frag_len));

    /*
     * If we've got a fragmented receive stream consider sending a fack.
     * Send a fack if the receive queue is full or if the sender asked
     * for a fack.  To avoid confusing pre-v2 code, don't send a fack
     * when the stream is complete.
     */
    if (rq->recving_frags 
        && ! rq->all_pkts_recvd
        && (rq->queue_len == rq->max_queue_len ||
            ! RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_NO_FACK)))
    {       
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 6, (
            "(rpc__dg_call_recvq_insert) recv data %lu.%u frq, fack --> .%u\n", 
            rqe->hdrp->seq, fragnum, rq->next_fragnum - 1));
        rpc__dg_call_xmit_fack(call, rqe, false);
    }

    /*
     * If the call has a private socket, then it was the call thread
     * itself that called us.  No need to signal it.
     */  
    if (call->sock_ref->is_private)
        return (added_to_queue);

    /*
     * It is appropriate to awaken a receiver when:
     *      Their receive stream is complete
     *              -OR-
     *      The recvq is full (the head should be inorder)
     *              -OR-
     *      There are a sufficient number of inorder RQE's
     *              -OR-
     *      We're rationing, and the thread has a useable packet.
     *
     * Note: We estimate # of inorder bytes based on high_rcv_frag_size.
     */
    if (rq->all_pkts_recvd || 
        rq->queue_len == rq->max_queue_len ||
        rq->high_rcv_frag_size * rq->inorder_len >= rq->wake_thread_qsize ||
        (rpc__dg_pkt_is_rationing(NULL) && rq->inorder_len > 0))
        *wake_thread = true;

    return(added_to_queue);
}


/*
 * R P C _ _ D G _ C A L L _ L O C A L _ C A N C E L
 *
 * A local cancel has been detected (i.e. a cancelable operation 
 * caused an unwind).  What needs to be done depends on the type
 * of call handle.  Callbacks complicate matters - as always :-)
 * The actual code is pretty simple; it's the case analysis that's
 * a bear; presumably, this 'documentation' will be useful.
 */

PRIVATE void rpc__dg_call_local_cancel
(
    rpc_dg_call_p_t call
)
{
    RPC_DG_CALL_LOCK_ASSERT(call);

    if (RPC_DG_CALL_IS_CLIENT(call))
    {
        rpc_dg_ccall_p_t ccall = (rpc_dg_ccall_p_t) call;

        /*
         * All local detected cancels while dealing with a ccall are
         * forwarded to the associated server.  This is because any such
         * cancel *was* generated locally.  The original client will
         * only accept a forwarded cancel request (resulting in a remotely
         * generated cancel) while a callback is in progress.  The callback
         * would flush any pending cancels when it completes.
         *
         * If the call has yet to establish a cancel timeout, do so now.
         */
        ccall->cancel.local_count++;
        rpc__dg_ccall_setup_cancel_tmo(ccall);

        RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10,
                ("(rpc__dg_call_local_cancel) ccall fwd [%s]\n",
                rpc__dg_act_seq_string(&ccall->c.xq.hdr)));

        rpc__dg_ccall_xmit_cancel_quit(ccall, ccall->cancel.local_count);
    }
    else
    {
        rpc_dg_scall_p_t scall = (rpc_dg_scall_p_t) call;

        /*
         * If we're handling a non-callback scall (i.e. a normal scall)
         * we don't (typically) expect to see local cancels since the
         * scall execution thread (typically) executes with cancel delivery
         * disabled; the sstub enables delivery only while in the manager.
         * 
         * In the event that we have detected the local cancel, then
         * someone enabled cancel delivery while calling back into the
         * runtime.  This could happen (a) because the sstub was trying
         * to implement a cancelable-prologue mode (though NIDL doesn't
         * support this at the time this was written); (b) a manager
         * called a pipe handler without disabling cancel delivery
         * (presumably because it wanted a cancel to be able to terminate
         * the call); (c) runtime or sstub bug :-)
         *
         * Such a local cancel was presumably generated by the reception
         * of a cancel-request.  However, some local manager could have
         * (illegally?) posted a cancel to this call execution thread.
         * Lastly, the local cancel may have originated due to local
         * orphan call processing.
         * 
         * In any case, the effect is the same.  Local cancel detection while
         * processing a normal scall results in a call failure.
         */
        if (!scall->c.is_cbk)
        {
            RPC_DBG_GPRINTF(
                    ("(rpc__dg_call_local_cancel) scall failure [%s]\n",
                    rpc__dg_act_seq_string(&scall->c.xq.hdr)));

            rpc__dg_call_signal_failure(&scall->c, rpc_s_call_cancelled);
        }
        else

        /*
         * We're handling a callback scall and we've detected a local cancel.
         *
         * Callbacks and cancels are a very poor / ugly story.  Basically
         * (given the current client / server RPC cancel design) callbacks
         * and cancels can only work properly when clients (both the original
         * and the callback originating manager) are executing with cancel-on.
         * Other scenarios result in very inproper handling of a RPC cancel.
         * 
         * If the original client RPC was being made with cancel delivery
         * disabled, then the callback's sstub forceable enabling cancel
         * delivery while in the callback manager has already broken
         * the cancel model.  A cancel against the original client thread
         * was NOT suppose to be seen.
         * 
         * Another case where callbacks and cancels don't work is if
         * the client RPC is being performed cancel-on and the manager
         * decides to perform a callback with cancel-off.  Again, a cancel
         * against the original thread can result in terminating the
         * callback manager (which wasn't suppose to see a cancel).
         * 
         * Since the only callback cancel case that can work correctly is
         * when all components of the original call are being made with
         * cancel delivery enabled, that's the only case we'll try to make
         * work.  Here's what we do:
         *      If we detected a local cancel for a cbk_scall
         *          AND the cbk_scall had received a forwarded 
         *          cancel-request
         *      then we treat this just like the normal scall and
         *          fail the cbk_scall
         *      otherwise, the cancel must have been a result
         *          of a dcethread_interrupt_throw() against the original
         *          client and the cancel should treated just like
         *          the normal ccall; forward the cancel to the server
         */
        {
            if (scall->c.c.u.server.cancel.count > 0)
            {
                /*
                 * The callback accepted a cancel-request.
                 */
                RPC_DBG_GPRINTF(
                    ("(rpc__dg_call_local_cancel) cbk_scall failure [%s]\n",
                    rpc__dg_act_seq_string(&scall->c.xq.hdr)));

                rpc__dg_call_signal_failure(&scall->c, rpc_s_call_cancelled);
            }
            else
            {
                /*
                 * Locally generated cancel - forward it to the originator
                 * (the server) of this callback (as above).
                 *
                 * The callback's cbk_ccall is the one where the info
                 * must be updated.  Unfortunately there's a locking
                 * hierarchy violation that we have to deal with.  In
                 * the case that we can't get the lock, just repost the
                 * cancel to the thread so we don't loose it (we will
                 * try to handle it again the next time the thread calls
                 * a cancelable operation).
                 */

                rpc_dg_ccall_p_t ccall = scall->cbk_ccall;
                boolean b;

                RPC_DG_CALL_TRY_LOCK(&ccall->c, &b);
                if (!b)
                {
                    RPC_DBG_PRINTF(rpc_e_dbg_cancel, 3,
                        ("(rpc__dg_call_local_cancel) cbk_scall can't get ccall lock [%s]\n",
                        rpc__dg_act_seq_string(&scall->c.xq.hdr)));

                    dcethread_interrupt_throw(dcethread_self());
                }
                else
                {
                    ccall->cancel.local_count++;
                    rpc__dg_ccall_setup_cancel_tmo(ccall);

                    RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10,
                        ("(rpc__dg_call_local_cancel) cbk_scall ccall fwd [%s]\n",
                        rpc__dg_act_seq_string(&ccall->c.xq.hdr)));

                    rpc__dg_ccall_xmit_cancel_quit(ccall, 
                            ccall->cancel.local_count);
                    RPC_DG_CALL_UNLOCK(&ccall->c);
                }
            }
        }
    }
}

