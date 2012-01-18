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
**      dg.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  Most entries points from common services are
**  in this module.
**
**
*/

#include <dg.h>
#include <dgrq.h>
#include <dgxq.h>
#include <dgpkt.h>
#include <dgcall.h>
#include <dgccall.h>
#include <dgccallt.h>
#include <dgcct.h>

/* =============================================================================== */

/*
 * Define a fragment number based function that determines if it is time
 * to check for (and fwd) a pending cancel for a call.
 *
 * We don't have to check too frequently.  However we also don't want
 * to do it too infrequently since the cancel timeout timer won't get
 * started until one is detected.
 */
#ifndef RPC_DG_CANCEL_CHECK_FREQ
#  define RPC_DG_CANCEL_CHECK_FREQ    64
#endif

#ifndef _PTHREAD_NO_CANCEL_SUPPORT
#    define CANCEL_CHECK(fnum, call) \
    if ((fnum) >= RPC_DG_CANCEL_CHECK_FREQ && \
        (fnum) % RPC_DG_CANCEL_CHECK_FREQ == 0) \
    { \
        cancel_check(call); \
    }

    INTERNAL void cancel_check ( rpc_dg_call_p_t );

#else
#    define CANCEL_CHECK(fnum, call)  {}
#endif


/* =================================================================== */

/*
 * C A N C E L _ C H E C K
 *
 * Check for a posted cancel; if there was one, handle it.  This routine
 * must flush the posted cancel.  Of course, if general cancel delivery
 * is disabled, then we aren't required (and in fact don't want) to detect
 * one.
 * 
 * For ccalls, Once a cancel timeout has been established there's really
 * no need for us to continue to check;  a cancel has been forwarded
 * and either the call will complete before the timeout occurs or not.
 * Scalls also need to check for cancels (see rpc__dg_call_local_cancel).
 */

#ifndef _PTHREAD_NO_CANCEL_SUPPORT
INTERNAL void cancel_check(call)
rpc_dg_call_p_t call;
{
    RPC_DG_CALL_LOCK_ASSERT(call);

    if (RPC_DG_CALL_IS_CLIENT(call))
    {
        rpc_dg_ccall_p_t ccall = (rpc_dg_ccall_p_t) call;

        if (ccall->cancel.timeout_time > 0)
        {
            return;
        }
    }

    DCETHREAD_TRY
    {
        dcethread_checkinterrupt();
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10,
            ("(check_and_fwd) local cancel detected\n"));
        rpc__dg_call_local_cancel(call);
    }
    DCETHREAD_ENDTRY
}
#endif


/*
 * R P C _ _ D G _ C A L L _ T R A N S M I T _ I N T
 *
 * Send part of RPC call request/response and return.
 *
 * Note: a 0 length transmit request is legitimate and typically causes an 
 * empty request fragment to get created and queued.  This is how RPCs with 
 * 0 ins get a request fragment created.  We don't special case this 
 * processing, it happens naturally given the way this code is written 
 * (which also means that we don't bother ensuring that this is the only 
 * situation where a 0 length request fragment can be generated - that's 
 * the stub's responsibility).
 *
 * Since a sender may have the good fortune to virtually never block,
 * we must periodically forceable check for a pending cancel.  We don't have
 * to do this too frequently.  However we also don't want to do it too
 * infrequently since the cancel timeout timer won't get started until one
 * is detected.  Once a cancel timeout has been established there's really no
 * need for us to check.
 */

PRIVATE void rpc__dg_call_transmit_int(call, data, st)
rpc_dg_call_p_t call;
rpc_iovector_p_t data;
unsigned32 *st;
{
    unsigned16 room_left;               /* # bytes left in partial packet body */
    unsigned16 frag_length = 0;         /* # bytes in the entire frag */
    boolean    alloc_pkt;

    unsigned16 elt_num;                 /* Current data elt being processed */
    unsigned32 data_used;               /* # bytes of current data elt consumed */
    unsigned32 body_size = 0;
    rpc_dg_xmitq_p_t xq = &call->xq;
    rpc_dg_xmitq_elt_p_t xqe = NULL;

    boolean end_of_data;
    rpc_dg_auth_epv_p_t auth_epv = call->auth_epv;

    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * Check for call failure conditions...
     */
    *st = call->status;
    if (*st != rpc_s_ok)
        return;

    if (call->state == rpc_e_dg_cs_orphan)
    {
        if (xq->head != NULL) 
            rpc__dg_xmitq_free(xq, call);
        *st = rpc_s_call_orphaned;
        return;
    }

    if (RPC_DG_CALL_IS_SERVER(call) && call->state == rpc_e_dg_cs_recv)
    {
        /*
         * This is a turnaround. The server, which has been the
         * receiver, becomes the sender.
         *
         * Instead of sending a small fragment, we start with the largest
         * fragment size seen so far. We assume that the client can receive
         * the fragment as large as what it has been sending; since the
         * client's xq.snd_frag_size is bounded by its xq.max_frag_size.
         *
         * Thanks for the reservation we have made so far, we have enough
         * reservations to send this large fragment!
         */

        if (call->rq.high_rcv_frag_size > xq->snd_frag_size)
        {
            xq->snd_frag_size = MIN(call->rq.high_rcv_frag_size,
                                    xq->max_snd_tsdu);
            /*
             * Fragment sizes must be 0 MOD 8.  Make it so.
             */
            xq->snd_frag_size &= ~ 0x7;

            /*
             * If we are going to use high_rcv_frag_size, then it implies that
             * we don't need to wait for the first fack.
             */
            xq->first_fack_seen = true;
        }

        /*
         * For KRPC, should we reduce the packet reservation here?
         */
    }

    elt_num = 0;
    data_used = 0;
    end_of_data = data->num_elt == 0 ? true : false;

    /*
     * Since we don't retain the computed body size (maybe we should?)
     * we need to recompute it if we've already got a partial pkt.
     */
    if (xq->part_xqe != NULL)
    {
        body_size = MIN(xq->snd_frag_size, RPC_C_DG_MAX_PKT_BODY_SIZE);
        
        xqe = xq->part_xqe;
        frag_length = xqe->body_len + RPC_C_DG_RAW_PKT_HDR_SIZE;
        while (xqe->more_data)
        {
            xqe = xqe->more_data;
            frag_length += xqe->body_len;
        }            

        if (auth_epv != NULL) 
            frag_length += (auth_epv->overhead);
    }
    
    do
    {
        /*
         * Broadcasts are not allowed to send fragments.  If the broadcast
         * attribute is set, check to see if we already have a full packet
         * of data, and are beginning to fill a second.
         */

        if (((xq->base_flags & RPC_C_DG_PF_BROADCAST) != 0) &&
            (xq->next_fragnum > 0))
        {
            *st = rpc_s_in_args_too_big;
            return;
        }

        /*
         * Make sure we have a partial packet to copy into.  If we don't
         * have one, first wait until there's some window space open
         * (if necessary) and then allocate a partial packet.  Make
         * sure each pkt gets filled to the max allowable capacity.
         * If the call has an error associated with it, we're done.
         */

        if (xq->part_xqe == NULL) 
        {
            frag_length = RPC_C_DG_RAW_PKT_HDR_SIZE;
            alloc_pkt = true;
            if (auth_epv != NULL) 
                frag_length += (auth_epv->overhead);
        }
        else
        {                      
            alloc_pkt = (xqe->body_len == body_size) && 
                        (frag_length < xq->snd_frag_size);
        }

        if (alloc_pkt)
        {                      
            rpc_dg_xmitq_elt_p_t tmp;
            /*
             * Sleep until 
             *    1) a fack comes in and
             *          a) opens window space
             *          b) packets need to be rexmitted
             *    2) a timeout occurs and packets need to be rexmitted.
             *    3) the first fack comes in to tell the receiver's idea
             *       of the right fragment size, iff non-callback.
             *
             * Either way, call call_xmit to handle the new queue state.
             */
            while ((xq->first_unsent != NULL &&
                    RPC_DG_FRAGNUM_IS_LTE(xq->first_unsent->fragnum + 
                        xq->max_blast_size, xq->next_fragnum))
                   || (xq->next_fragnum == 1 &&
                       xq->first_fack_seen == false &&
                       !call->is_cbk))
            {
                rpc__dg_call_wait(call, rpc_e_dg_wait_on_network_event, st);
                if (*st != rpc_s_ok)
                    return;
            }

            tmp = rpc__dg_pkt_alloc_xqe(call, st);
            if (*st != rpc_s_ok)
                return;

            if (xq->part_xqe == NULL)
                xqe = xq->part_xqe = tmp;
            else                     
            {
                xqe->more_data = tmp;
                xqe = tmp;
            }

            body_size = MIN(xq->snd_frag_size, RPC_C_DG_MAX_PKT_BODY_SIZE);
        }

        /*
         * Copy as much data into the partial packet as will fit.
         */

        room_left = MIN(body_size - xqe->body_len,
                        xq->snd_frag_size - frag_length);

        if (elt_num < data->num_elt) {
            rpc_iovector_elt_t *iove = &data->elt[elt_num];

            while (true) {
                unsigned32 data_left = iove->data_len - data_used;

                if (room_left <= data_left) {
                    memcpy(((char *)xqe->body)+xqe->body_len, &iove->data_addr[data_used], 
                          room_left);
                    data_used += room_left;
                    xqe->body_len += room_left;
                    frag_length += room_left;
                    room_left = 0;                      

                    /*
                     * If we copied the full data buffer, and it's the last one,
                     * set end of data flag, and free the buffer.
                     */

                    if (data_used == iove->data_len && (elt_num + 1 == data->num_elt))
                    {
                        end_of_data = true;

                        if (iove->buff_dealloc != NULL)
                            RPC_FREE_IOVE_BUFFER(iove);
                    }
                    break;
                }
                else {
                    memcpy(((char *)xqe->body)+xqe->body_len, &iove->data_addr[data_used], 
                          data_left);
                    room_left -= data_left;
                    xqe->body_len += data_left;
                    frag_length += data_left;
                    if (iove->buff_dealloc != NULL)
                        RPC_FREE_IOVE_BUFFER(iove);
                    
                    /*
                     * We copied the full data buffer;  if it's the last one,
                     * set end of data flag.
                     */

                    if (++elt_num == data->num_elt)
                    {
                        end_of_data = true;
                        break;             
                    }
                    iove = &data->elt[elt_num];
                    data_used = 0;
                }
            }
        }

        RPC_DG_CALL_SET_STATE(call, rpc_e_dg_cs_xmit);
                                               
        /*
         * The transmit/windowing routines assume that anything on the
         * queue is ready to be sent, so we need to worry that we don't
         * put the last packet on the queue here (or the last_frag bit
         * won't get set).  If we have run out of new data, leave the
         * partial packet as it is (possibly full), and return. 
         */
           
        if (end_of_data)
            return;
                          
        /*
         * Add newly-filled pkt to xmit queue.
         */

        if (frag_length == xq->snd_frag_size)
        {
            rpc__dg_xmitq_append_pp(call, st);
            if (*st != rpc_s_ok)
                return;

            /*
             * When (re)starting a window, we can't wait for an ack/timeout
             * before sending data. In this case (only) for the xmit by setting
             * the blast size, and calling call_xmit() directly.
             *
             * Instead of starting the window with the one fragment queued,
             * send two fragments since we have the max pkt reservation.
             */

            if (xq->freqs_out == 0)
            {       
                if (call->n_resvs < 2)
                {
                    xq->blast_size = 1;
                    rpc__dg_call_xmit(call, true);
                }
                else if (xq->head != xq->tail)
                {
                    xq->blast_size = 2;
                    rpc__dg_call_xmit(call, true);
                }
            }                 

            /*
             * periodically check for a local posted cancel...
             */
            CANCEL_CHECK(xq->next_fragnum, call);
        }
    }
    while (elt_num < data->num_elt);
}


/*
 * R P C _ _ D G _ C A L L _ R E C E I V E _ I N T
 *
 * Return the next, and possibly last part of the RPC call
 * request/response.
 * 
 * Note well that this routine differs from "rpc__dg_call_receive" in
 * that this routine might return an I/O vector element whose data length
 * is zero but whose dealloc routine pointer is non-NULL.  Callers of
 * this routine must call the dealloc routine even in the case of
 * zero-length data.
 *
 * Since a receiver may have the good fortune to virtually never block,
 * we must periodically forceable check for a pending cancel.
 */

PRIVATE void rpc__dg_call_receive_int(call, data, st)
rpc_dg_call_p_t call;
rpc_iovector_elt_t *data;
unsigned32 *st;
{
    rpc_dg_recvq_p_t rq = &call->rq;
    rpc_dg_recvq_elt_p_t rqe;
    rpc_key_info_p_t key_info = call->key_info;
    

    RPC_DG_CALL_LOCK_ASSERT(call);

    data->data_len = 0;
    data->data_addr = NULL;
    data->buff_dealloc = NULL;
    data->buff_len = 0;
    data->buff_addr = NULL;

    /*
     * Check for call failure conditions...
     */
    *st = call->status;
    if (*st != rpc_s_ok)
        return;

    if (call->state == rpc_e_dg_cs_orphan)
    {                     
        if (rq->head != NULL) 
            rpc__dg_recvq_free(rq);
        *st = rpc_s_call_orphaned;
        return;
    }

    /*
     * Check to see if there's any more receive data to return.
     * The above call->status check ensures that we will return
     * EOD in non-call-failure conditions.
     */
    if (rq->all_pkts_recvd && rq->head == NULL)
        return;

    /*
     * Wait till the next in-order fragment is available.
     * If the call has an error associated with it, we're done.
     */

    while (rq->last_inorder == NULL)
    {                     
        rpc__dg_call_wait(call, rpc_e_dg_wait_on_network_event, st);

        if (*st != rpc_s_ok)
            return;
    }

    rqe = rq->head;
    
    /*
     * Check authentication iff the head of the fragment.
     */
    if (key_info != NULL && rqe->hdrp != NULL)
    {
        rpc_dg_auth_epv_p_t auth_epv = call->auth_epv;
        
        unsigned32 blocksize = auth_epv->blocksize;
        char *cksum;
        unsigned long raw_bodysize;
        unsigned32 pkt_bodysize;
        rpc_dg_recvq_elt_p_t last_rqe = rqe;
        char auth_trailer[32];  /* XXX: should be malloc'ed */
        byte_p_t real_auth_trailer = NULL;
        unsigned32 saved_pkt_len = 0;

        /*
         * It's not really necessary to round up the packet body
         * length here because the sender includes the length of
         * padding before the auth trailer in the packet body length.
         * However, I think, that's a wrong behavior and we shouldn't
         * rely on it.
         */
        raw_bodysize = ((rqe->hdrp->len + blocksize - 1)
                        / blocksize) * blocksize;

        /*
         * Verify that cksum is entirely contained inside the packet,
         * and the auth_type is what we expected.
         *
         * This "shouldn't fail" unless someone's playing games with
         * us.
         */

        if (((RPC_C_DG_RAW_PKT_HDR_SIZE + raw_bodysize + auth_epv->overhead)
            > rqe->frag_len) ||
            (rqe->hdrp->auth_proto != auth_epv->auth_proto))
        {
            *st = nca_s_proto_error;
        }
        else
        {
            pkt_bodysize = MIN(raw_bodysize,
                               last_rqe->pkt_len - RPC_C_DG_RAW_PKT_HDR_SIZE);
            raw_bodysize -= pkt_bodysize;

            while (last_rqe->more_data != NULL && raw_bodysize > 0)
            {
                pkt_bodysize = MIN(raw_bodysize,
                                   last_rqe->more_data->pkt_len);
                raw_bodysize -= pkt_bodysize;
                last_rqe = last_rqe->more_data;
            }
            cksum = last_rqe->pkt->body.args + pkt_bodysize;

            if (pkt_bodysize + auth_epv->overhead >
                ((rqe == last_rqe) ?
                 (last_rqe->pkt_len - RPC_C_DG_RAW_PKT_HDR_SIZE) :
                 (last_rqe->pkt_len)))
            {
                /*
                 * Heck, auth trailer is not continuous!
                 * or the *real* last rqe has auth trailer only.
                 */
                unsigned32 len;

                assert(last_rqe->more_data != NULL);

                if (rqe == last_rqe)
                    len = last_rqe->pkt_len -
                        (pkt_bodysize + RPC_C_DG_RAW_PKT_HDR_SIZE);
                else
                    len = last_rqe->pkt_len - pkt_bodysize;

                /*
                 * We should always allocate auth_trailer.
                 * But, for now...
                 */
                if (auth_epv->overhead <= 24)
                {
                    cksum = (char *)RPC_DG_ALIGN_8(auth_trailer);
                }
                else
                {
                    RPC_MEM_ALLOC (real_auth_trailer,
                                   byte_p_t,
                                   auth_epv->overhead + 7,
                                   RPC_C_MEM_UTIL,
                                   RPC_C_MEM_WAITOK);
                    if (real_auth_trailer == NULL)
                    {
                        *st = rpc_s_no_memory;
                        goto after_recv_ck;
                    }
                    cksum = (char *)RPC_DG_ALIGN_8(real_auth_trailer);
                }
                memcpy(cksum, last_rqe->pkt->body.args + pkt_bodysize, len);
                memcpy(cksum + len, last_rqe->more_data->pkt->body.args,
                       auth_epv->overhead - len);
            }

            /*
             * Adjust the last packet buffer's pkt_len,
             * i.e., excluding the auth trailer.
             */
            if (rqe == last_rqe)
            {
                saved_pkt_len = last_rqe->pkt_len;
                last_rqe->pkt_len = pkt_bodysize + RPC_C_DG_RAW_PKT_HDR_SIZE;
            }
            else
            {
                saved_pkt_len = last_rqe->pkt_len;
                last_rqe->pkt_len = pkt_bodysize;
            }
            (*auth_epv->recv_ck) (key_info, rqe, cksum, st);

            if (real_auth_trailer != NULL)
                RPC_MEM_FREE (real_auth_trailer, RPC_C_MEM_UTIL);
        }
    after_recv_ck:
        if (*st == rpc_s_dg_need_way_auth)
        {
            /*
             * Restore the last packet buffer's pkt_len.
             */
            if (saved_pkt_len != 0)
                last_rqe->pkt_len = saved_pkt_len;

            /*
             * Setup the return iovector element.
             *
             * Note: The call executor does not own this fragment and should
             * never free it. It's supposed to call me again.
             */
            RPC_DG_RECVQ_IOVECTOR_SETUP(data, rqe);
            return;
        }
        else if (*st != rpc_s_ok)
            return;

        /*
         * The *real* last rqe is no longer necessary.
         */
        if (last_rqe->more_data != NULL)
        {
            rpc__dg_pkt_free_rqe(last_rqe->more_data, call);
            last_rqe->more_data = NULL;
        }
    }

    /*
     * Dequeue the fragment off the head of the list.  Adjust the last
     * in-order pointers if necessary.
     */

    if (rqe->hdrp != NULL)
    {
        rq->head_serial_num =
            (rqe->hdrp->serial_hi << 8) | rqe->hdrp->serial_lo;
    }
    if (rqe->more_data != NULL)
    {
        rq->head = rqe->more_data;
        rq->head->next = rqe->next;
        if (rq->last_inorder == rqe)
            rq->last_inorder = rq->head;
        rqe->more_data = NULL;
    }
    else
    {
        rq->head = rqe->next;
        if (rq->head != NULL)
            rq->head_fragnum = rq->head->hdrp->fragnum;
        if (rq->last_inorder == rqe)
            rq->last_inorder = NULL;
        rq->inorder_len--;
        rq->queue_len--;
    }
        
    /*
     * If the queue was completely full, the sender will have been sent
     * a "0 window size" fack to stop it from sending.  Send a fack now
     * to let it know it can begin sending again.
     */
    if (rq->queue_len == rq->max_queue_len - 1)
    {  
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
            ("(rpc__dg_call_receive_int) sending fack to prod peer\n"));
        rpc__dg_call_xmit_fack(call, rqe, false);
    }

    /*
     * periodically check for a local posted cancel...
     */
    if (rqe->hdrp != NULL)
    CANCEL_CHECK(rqe->hdrp->fragnum, call);

    *st = rpc_s_ok;

    /*
     * Setup the return iovector element.  Note, the stubs are obligated
     * to pass us a non-null data even if they "know" there are no outs
     * (which means that we must initialize it).  Since someone has to
     * free the rqe (and the stub always has to check), we keep the
     * code path simpler and just let them always do it.
     */

    RPC_DG_RECVQ_IOVECTOR_SETUP(data, rqe);
}


/*
 * R P C _ _ D G _ C A L L _ T R A N S M I T
 *
 * Shell over internal "comm_transmit" routine that acquires the call lock
 * first.
 */

PRIVATE void rpc__dg_call_transmit(call_, data, st)
rpc_call_rep_p_t call_;
rpc_iovector_p_t data;
unsigned32 *st;
{
    rpc_dg_call_p_t call = (rpc_dg_call_p_t) call_;

    if (RPC_DG_CALL_IS_CLIENT(call)) {
        assert(call->state == rpc_e_dg_cs_init || call->state == rpc_e_dg_cs_xmit);
    }
    else 
    {
        assert(call->state == rpc_e_dg_cs_recv || call->state == rpc_e_dg_cs_xmit || call->state == rpc_e_dg_cs_orphan);
    }

    RPC_DG_CALL_LOCK(call);
    rpc__dg_call_transmit_int(call, data, st);
    RPC_DG_CALL_UNLOCK(call);
}


/*
 * R P C _ _ D G _ C A L L _ R E C E I V E
 *
 * Shell over internal "comm_receive" routine that acquires the call lock
 * first.
 */

PRIVATE void rpc__dg_call_receive(call_, data, st)
rpc_call_rep_p_t call_;
rpc_iovector_elt_t *data;
unsigned32 *st;
{
    rpc_dg_call_p_t call = (rpc_dg_call_p_t) call_;

    assert(call->state == rpc_e_dg_cs_recv || call->state == rpc_e_dg_cs_orphan);

    RPC_DG_CALL_LOCK(call);

    rpc__dg_call_receive_int(call, data, st);

    if (*st == rpc_s_ok)
    {
        if (data->data_len == 0 && data->buff_dealloc != NULL) 
            RPC_FREE_IOVE_BUFFER(data);
    }

    RPC_DG_CALL_UNLOCK(call);
}


/*
 * R P C _ _ D G _ C A L L _ T R A N S C E I V E
 *
 * Send last of RPC call request, wait for RPC call response, and return
 * at least the first part of it along with its associated drep.
 */

PRIVATE void rpc__dg_call_transceive(call_, xmit_data, recv_data, ndr_format, st)
rpc_call_rep_p_t call_;
rpc_iovector_p_t xmit_data;
rpc_iovector_elt_t *recv_data;
ndr_format_t *ndr_format;
unsigned32 *st;
{
    rpc_dg_ccall_p_t ccall;
    rpc_dg_binding_client_p_t h;

    ccall = (rpc_dg_ccall_p_t) call_;
    assert(RPC_DG_CALL_IS_CLIENT(&ccall->c));

    assert(ccall->c.state == rpc_e_dg_cs_init || ccall->c.state == rpc_e_dg_cs_xmit);

    RPC_DG_CALL_LOCK(&ccall->c);

    /*
     * Be sure to handle the no-IN's case (generate at least one request
     * packet even if there are no IN's for this remote call).  Also,
     * if there is input data to this call, push that out.  (We're trying
     * to avoid the case where data has been sent previously but then
     * transceive is called with no transmit data.)
     */

    if (ccall->c.state == rpc_e_dg_cs_init || xmit_data->num_elt > 0)
    {
        rpc__dg_call_transmit_int(&ccall->c, xmit_data, st);

        if (*st != rpc_s_ok)
        {
            RPC_DG_CALL_UNLOCK(&ccall->c);
            return;
        }
    }

    rpc__dg_call_xmitq_push(&ccall->c, st);
    if (*st != rpc_s_ok)
    {
        RPC_DG_CALL_UNLOCK(&ccall->c);
        return;
    }

    /*
     * If this is a "maybe" call, there's no response and we're done.
     * Otherwise, wait for and return the 1st response fragment.
     */

    if (ccall->c.xq.base_flags & RPC_C_DG_PF_MAYBE)
    {
        recv_data->data_len = 0;
        recv_data->buff_dealloc = NULL;
        recv_data->buff_addr = NULL;
        ccall->c.rq.all_pkts_recvd = true;
    }
    else
    {
        RPC_DG_PING_INFO_INIT(&ccall->ping);
        RPC_DG_CALL_SET_STATE(&ccall->c, rpc_e_dg_cs_recv);
        
        rpc__dg_call_receive_int(&ccall->c, recv_data, st);

        if (*st == rpc_s_ok)
        {
            rpc_dg_recvq_elt_p_t rqe =
                RPC_DG_RECVQ_ELT_FROM_IOVECTOR_ELT(recv_data);
            /*
             * The first rqe must have a valid hdrp.
             */
            assert(rqe->hdrp != NULL);
            RPC_DG_HDR_INQ_DREP(ndr_format, rqe->hdrp);

            if (recv_data->data_len == 0 && recv_data->buff_dealloc != NULL) 
                RPC_FREE_IOVE_BUFFER(recv_data);
        }

        /*
         * Update the binding handle with bound server instance information.
         * Only do this if the call succeded in binding to a server (which
         * may not always be the case; e.g. consider "maybe" calls).  Signal
         * other threads that that may have been blocked for binding
         * serialization.  Watch out for the locking hierarchy violation.
         */

        h = (rpc_dg_binding_client_p_t) ccall->h;
        if (! h->c.c.bound_server_instance && ccall->server_bound)
        {
            boolean gotit;
            unsigned32 tst;

            RPC_TRY_LOCK(&gotit);
            if (! gotit)
            {
                RPC_DG_CALL_UNLOCK(&ccall->c);
                RPC_LOCK(0);
                RPC_DG_CALL_LOCK(&ccall->c);
            }

            h->server_boot = ccall->c.call_server_boot;
            rpc__naf_addr_overcopy(ccall->c.addr, &h->c.c.rpc_addr, &tst);
            h->c.c.addr_has_endpoint = true;
            h->c.c.bound_server_instance = true;
            RPC_DBG_PRINTF(rpc_e_dbg_general, 5, 
                ("(rpc__dg_call_transceive) unblocking serialized waiters...\n"));
            RPC_BINDING_COND_BROADCAST(0);

            RPC_UNLOCK(0);
        }
    }

    RPC_DG_CALL_UNLOCK(&ccall->c);
}


/*
 * R P C _ _ D G _ C A L L _ B L O C K _ U N T I L _ F R E E
 *
 * Response completely sent. Block until memory associated with reponse
 * is free.
 */
PRIVATE void rpc__dg_call_block_until_free(call_, st)
rpc_call_rep_p_t call_ ATTRIBUTE_UNUSED;
unsigned32 *st;
{
    assert(RPC_DG_CALL_IS_SERVER((rpc_dg_call_p_t) call_));

    *st = rpc_s_ok;

    /*
     * This is currently a NO-OP because we are always copying the
     * data provided to us by the stub.
     */
}


/*
 * R P C _ _ D G _ C A L L _ A L E R T
 *
 * Alert current RPC call.
 */
PRIVATE void rpc__dg_call_alert(call_, st)
rpc_call_rep_p_t call_ ATTRIBUTE_UNUSED;
unsigned32 *st;
{
    *st = rpc_s_ok;

    /*
     * rpc_m_unimp_call
     * "(%s) Call not implemented"
     */
    RPC_DCE_SVC_PRINTF ((
	DCE_SVC(RPC__SVC_HANDLE, "%s"),
	rpc_svc_general,
	svc_c_sev_fatal | svc_c_action_abort,
	rpc_m_unimp_call,
	"rpc__dg_call_alert" ));
}


/*
 * R P C _ _ D G _ C A L L _ R E C E I V E _ F A U L T
 *
 */

PRIVATE void rpc__dg_call_receive_fault(call_, data, ndr_format, st)
rpc_call_rep_p_t call_;
rpc_iovector_elt_t *data;
ndr_format_t *ndr_format;
unsigned32 *st;
{
    rpc_dg_ccall_p_t ccall;
    rpc_key_info_p_t key_info;
    rpc_dg_recvq_elt_p_t rqe;

    ccall = (rpc_dg_ccall_p_t) call_;
    assert(RPC_DG_CALL_IS_CLIENT(&ccall->c));
    if (ccall->fault_rqe == NULL)
    {
        *st = rpc_s_no_fault;
        return;
    }
    rqe = ccall->fault_rqe;

    *st = rpc_s_ok;

    RPC_DG_CALL_LOCK(&ccall->c);

    data->data_len = 0;
    data->data_addr = NULL;
    data->buff_dealloc = NULL;
    data->buff_len = 0;
    data->buff_addr = NULL;

    /*
     * Check authentication.
     */
    key_info = ccall->c.key_info;

    /*
     * Check authentication iff the head of the fragment.
     */
    if (key_info != NULL && rqe->hdrp != NULL)
    {
        rpc_dg_auth_epv_p_t auth_epv = ccall->c.auth_epv;

        unsigned32 blocksize = auth_epv->blocksize;
        char *cksum;
        unsigned long raw_bodysize;
        unsigned32 pkt_bodysize;
        rpc_dg_recvq_elt_p_t last_rqe = rqe;
        char auth_trailer[32];  /* XXX: should be malloc'ed */
        byte_p_t real_auth_trailer = NULL;

        /*
         * It's not really necessary to round up the packet body
         * length here because the sender includes the length of
         * padding before the auth trailer in the packet body length.
         * However, I think, that's a wrong behavior and we shouldn't
         * rely on it.
         */
        raw_bodysize = ((rqe->hdrp->len + blocksize - 1)
                        / blocksize) * blocksize;

        /*
         * Verify that cksum is entirely contained inside the packet,
         * and the auth_type is what we expected.
         *
         * This "shouldn't fail" unless someone's playing games with
         * us.
         */

        if (((RPC_C_DG_RAW_PKT_HDR_SIZE + raw_bodysize + auth_epv->overhead)
            > rqe->frag_len) ||
            (rqe->hdrp->auth_proto != auth_epv->auth_proto))
        {
            *st = nca_s_proto_error;
        }
        else
        {
            pkt_bodysize = MIN(raw_bodysize,
                               last_rqe->pkt_len - RPC_C_DG_RAW_PKT_HDR_SIZE);
            raw_bodysize -= pkt_bodysize;

            while (last_rqe->more_data != NULL && raw_bodysize > 0)
            {
                pkt_bodysize = MIN(raw_bodysize,
                                   last_rqe->more_data->pkt_len);
                raw_bodysize -= pkt_bodysize;
                last_rqe = last_rqe->more_data;
            }
            cksum = last_rqe->pkt->body.args + pkt_bodysize;

            if (pkt_bodysize + auth_epv->overhead >
                ((rqe == last_rqe) ?
                 (last_rqe->pkt_len - RPC_C_DG_RAW_PKT_HDR_SIZE) :
                 (last_rqe->pkt_len)))
            {
                /*
                 * Heck, auth trailer is not continuous!
                 * or the *real* last rqe has auth trailer only.
                 */
                unsigned32 len;

                assert(last_rqe->more_data != NULL);

                if (rqe == last_rqe)
                    len = last_rqe->pkt_len -
                        (pkt_bodysize + RPC_C_DG_RAW_PKT_HDR_SIZE);
                else
                    len = last_rqe->pkt_len - pkt_bodysize;

                /*
                 * We should allocate auth_trailer.
                 * But, for now...
                 */
                if (auth_epv->overhead <= 24)
                {
                    cksum = (char *)RPC_DG_ALIGN_8(auth_trailer);
                }
                else
                {
                    RPC_MEM_ALLOC (real_auth_trailer,
                                   byte_p_t,
                                   auth_epv->overhead + 7,
                                   RPC_C_MEM_UTIL,
                                   RPC_C_MEM_WAITOK);
                    if (real_auth_trailer == NULL)
                    {
                        *st = rpc_s_no_memory;
                        goto after_recv_check;
                    }
                    cksum = (char *)RPC_DG_ALIGN_8(real_auth_trailer);
                }
                memcpy(cksum, last_rqe->pkt->body.args + pkt_bodysize, len);
                memcpy(cksum + len, last_rqe->more_data->pkt->body.args,
                       auth_epv->overhead - len);

                /*
                 * The *real* last rqe is no longer necessary.
                 */
                rpc__dg_pkt_free_rqe(last_rqe->more_data, &ccall->c);
                last_rqe->more_data = NULL;
            }

            /*
             * Adjust the last packet buffer's pkt_len,
             * i.e., excluding the auth trailer.
             */
            if (rqe == last_rqe)
                last_rqe->pkt_len = pkt_bodysize + RPC_C_DG_RAW_PKT_HDR_SIZE;
            else
                last_rqe->pkt_len = pkt_bodysize;
            (*auth_epv->recv_ck) (key_info, rqe, cksum, st);

            if (real_auth_trailer != NULL)
                RPC_MEM_FREE (real_auth_trailer, RPC_C_MEM_UTIL);
        }
    after_recv_check:
        if (*st != rpc_s_ok)
        {
            rpc__dg_pkt_free_rqe(rqe, &ccall->c);
            ccall->fault_rqe = NULL;    /* we no longer own the rqe */
            RPC_DG_CALL_UNLOCK(&ccall->c);
            return;
        }
    }

     /*
      * The multi-buffer fault fragment is not officially supported and
      * the IDL generated stubs won't use it because the fault pdu
      * contains only unsigned32 status. Well, maybe someone can use
      * it...
      */
    if (rqe->more_data != NULL)
    {
        ccall->fault_rqe = rqe->more_data;
        ccall->fault_rqe->next = rqe->next; /* should be NULL */
        rqe->more_data = NULL;
    }
    else
        ccall->fault_rqe = NULL;

    RPC_DG_RECVQ_IOVECTOR_SETUP(data, rqe);
    if (rqe->hdrp != NULL)
        RPC_DG_HDR_INQ_DREP(ndr_format, rqe->hdrp);

    RPC_DG_CALL_UNLOCK(&ccall->c);
}


/*
 * R P C _ _ D G _ C A L L _ F A U L T
 *
 * Declare the current call as "faulted", generating a fault packet with
 * the provided body.
 *
 * Server ONLY.
 */
PRIVATE void rpc__dg_call_fault(call_, fault_info, st)
rpc_call_rep_p_t call_;
rpc_iovector_p_t fault_info;
unsigned32 *st;
{
    rpc_dg_scall_p_t scall = (rpc_dg_scall_p_t) call_;
    unsigned32 tst;

    assert(RPC_DG_CALL_IS_SERVER(&scall->c));

    *st = rpc_s_ok;

    RPC_DG_CALL_LOCK(&scall->c);

    /*
     * Purge the recvq since it won't be used after this.  The recvq
     * may currently have lots of rqes on it and freeing it now will
     * help pkt quotas.
     */

    rpc__dg_recvq_free(&scall->c.rq);

    /*
     * Toss any pending xmitq pkts and add the fault_info to the xmit
     * queue just as if it were a response (but whack the proto pkt header
     * to the fault pkt type).  The call will now be in the xmit state if it
     * wasn't already there.  Defer the sending of the fault until
     * the "end of the call" (execute_call).  This prevents the client
     * from receiving the complete response, completing the call and
     * generating a new one while the server still thinks the call is
     * not complete (thinking it must have dropped an ack,...).  The
     * fault is really just a special response pkt.
     * 
     * This routine is called by the sstub (the thread executing the
     * call) so there's no need to signal the call.  We don't actually
     * want the call's status to be set to a error value; the server
     * runtime wants to still complete processing the call which involves
     * sending the fault response to the client (instead of any further
     * data response).
     * 
     * Subsequent fault response retransmissions will occur just as if
     * this were a "normal" call response as well as in reply to a ping.
     * Of course, faults for idempotent calls don't get remembered or
     * retransmitted.
     */

    RPC_DBG_GPRINTF(("(rpc__dg_call_fault) call has faulted [%s]\n",
        rpc__dg_act_seq_string(&scall->c.xq.hdr)));

    RPC_DG_XMITQ_REINIT(&scall->c.xq, &scall->c);
    RPC_DG_HDR_SET_PTYPE(&scall->c.xq.hdr, RPC_C_DG_PT_FAULT);

    rpc__dg_call_transmit_int(&scall->c, fault_info, &tst);

    RPC_DG_CALL_UNLOCK(&scall->c);
}


/*
 * R P C _ _ D G _ C A L L _ D I D _ M G R _ E X E C U T E
 *
 * Return whether or not the manager routine for the call identified
 * by the call handle has executed.
 */

PRIVATE boolean32 rpc__dg_call_did_mgr_execute(call_, st)
rpc_call_rep_p_t call_;
unsigned32 *st;
{
    rpc_dg_ccall_p_t ccall = (rpc_dg_ccall_p_t) call_;
    rpc_dg_xmitq_p_t xq = &ccall->c.xq;
    boolean r;

    *st = rpc_s_ok;

    RPC_DG_CALL_LOCK(&ccall->c);

    /*
     * If we don't know the boot time of the server and the call was not
     * idempotent, then the server CAN'T have executed the call.
     */
    if (ccall->c.call_server_boot == 0 && 
        ! RPC_DG_FLAG_IS_SET(xq->base_flags, RPC_C_DG_PF_IDEMPOTENT))
    {
        r = false;
        goto DONE;
    }

    /*
     * We decide whether the mgr executed as a function of any reject status
     * (i.e., from a "reject" packet).
     */
    switch ((int)ccall->reject_status)
    {
        /*
         * If ".reject_status" is zero, we didn't get a reject packet.  We
         * might have executed the manager.
         */
        case 0:
            r = true;
            goto DONE;

        /*
         * Any of these rejects means we KNOW we didn't execute the manager
         */
        case nca_s_unk_if:
        case nca_s_unsupported_type:
        case nca_s_manager_not_entered:
        case nca_s_op_rng_error:    
        case nca_s_who_are_you_failed:
        case nca_s_wrong_boot_time: 
            r = false;
            goto DONE;

        /*
         * Unknown reject status.  Assume the worst.
         */
        default:        
            r = true;
            goto DONE;
    }

DONE:

    RPC_DG_CALL_UNLOCK(&ccall->c);
    return (r);
}


/*
 * R P C _ _ D G _ N E T W O R K _ I N Q _ P R O T _ V E R S
 *
 */

PRIVATE void rpc__dg_network_inq_prot_vers(prot_id, vers_major, vers_minor, st)
unsigned8 *prot_id;
unsigned32 *vers_major;
unsigned32 *vers_minor;
unsigned32 *st;
{
    *prot_id = RPC_C_DG_PROTO_ID;
    *vers_major = RPC_C_DG_PROTO_VERS;
    *vers_minor = 0;
    *st = rpc_s_ok;
}
