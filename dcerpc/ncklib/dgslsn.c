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
**      dgslsn.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  Server-oriented routines that run in the 
**  listener thread.
**
**
*/

#include <dg.h>
#include <dgsct.h>
#include <dgpkt.h>
#include <dgcall.h>
#include <dgscall.h>
#include <dgccall.h>
#include <dgccallt.h>
#include <dgexec.h>
#include <dgslsn.h>
#include <dgutl.h>
#include <dgrq.h>
#include <dgxq.h>

#include <dce/conv.h>
#include <dce/convc.h>

#include <comprot.h>
#include <comcthd.h>

INTERNAL void scall_xmit_cancel_quack (
        rpc_dg_scall_p_t  /*scall*/,
        unsigned32  /*cancel_id*/,
        unsigned32  /*is_accepting*/
    );

INTERNAL void do_quit_body (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_scall_p_t  /*scall*/
    );

/*
 * Orphan quit packet check macro. 
 */

#define RPC_DG_IS_ORPHAN_QUIT(rqe)   ((rqe)->hdrp->len == 0)

/* ========================================================================= */

/*
 * R P C _ _ D G _ S E R V E R _ C H K _ A N D _ S E T _ S B O O T
 * 
 * Server boot time processing / verification.  Initially, the client
 * does not know the server instantiation and sets the server boot time
 * to 0.  After the first call the server must set the server boot time
 * to indicate which server instantiation the incoming call is related
 * to.
 *
 * If this is the first call to a server, we will set the server boot
 * time to it's own server boot time associating the call with the current
 * instantiation of the server.  If the server boot time has been set
 * in previous calls to the server, we verify that the incoming packet
 * is meant for this server.  Should the packet be for a previous
 * instantiation of the server, we send a "reject" packet using the old
 * server boot time found in the packet header.  The old boot time must
 * be used so that the client side processing does not drop the packet
 * - if the server boot time received by the client does not match its
 * the ccall information the client server boot time processing will
 * drop the packet assuming it does not know about the rejected call.
 */

PRIVATE boolean32 rpc__dg_svr_chk_and_set_sboot
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_sock_pool_elt_p_t sp
)
{
    if (rqe->hdrp->server_boot == 0)
    {
        rqe->hdrp->server_boot = rpc_g_dg_server_boot_time;
        return (true);
    }
    else
    {
        if (rqe->hdrp->server_boot == rpc_g_dg_server_boot_time)
        {
            return (true);
        }
        {
            RPC_DBG_GPRINTF(("c->s Server boot time mismatch [%s]\n",
                rpc__dg_act_seq_string(rqe->hdrp)));
            rpc__dg_xmit_error_body_pkt((sp)->sock, (rpc_addr_p_t) &rqe->from,
                rqe->hdrp, RPC_C_DG_PT_REJECT, nca_s_wrong_boot_time);
            return (false);
        }
    }
}   


/*
 * S C A L L _ X M I T _ C A N C E L _ Q U A C K
 *
 * Whip up a cancel quack and send it on it's way.
 * A cancel-ack manifests itself as an embellished 'quack' pkt.
 * Pre 2.0 receivers of a cancel-ack will treat this as a
 * plain quack (orphan-ack); which is the best they can do.
 */

INTERNAL void scall_xmit_cancel_quack
(
    rpc_dg_scall_p_t scall,
    unsigned32 cancel_id,
    unsigned32 is_accepting
)
{
    rpc_socket_iovec_t iov[2];
    rpc_dg_pkt_hdr_t hdr;
#ifndef MISPACKED_HDR
    rpc_dg_quackpkt_body_t body;
#else
    rpc_dg_raw_quackpkt_body_t body;
#endif
    boolean b;

    /*
     * Create a pkt header initialized with the prototype hdr.
     */

    hdr = scall->c.xq.hdr;

    RPC_DG_HDR_SET_PTYPE(&hdr,RPC_C_DG_PT_QUACK);
    
    hdr.flags       = 0;
    hdr.len         = RPC_C_DG_RAW_QUACKPKT_BODY_SIZE;

    RPC_DG_HDR_SET_DREP(&hdr);

    /*
     * Create the cancel-ack packet's body.
     */

#ifndef MISPACKED_HDR
    body.vers              = RPC_C_DG_QUACKPKT_BODY_VERS;
    body.cancel_id         = cancel_id;
    body.server_is_accepting = is_accepting;
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

    rpc__dg_xmit_pkt(scall->c.sock_ref->sock, scall->c.addr, iov, 2, &b);
}


/*
 * D O _ Q U I T _ B O D Y
 *
 * Process the contents of a quit body.
 * A quit with a body is interpreted as a cancel-request.
 */

INTERNAL void do_quit_body
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_scall_p_t scall
)
{
    unsigned32 cancel_id;

#ifndef MISPACKED_HDR

    rpc_dg_quitpkt_body_p_t bodyp = (rpc_dg_quitpkt_body_p_t) &rqe->pkt->body;

    /*
     * Make sure we understand the quit packet body version.
     */

    if (bodyp->vers != RPC_C_DG_QUITPKT_BODY_VERS)
    {
        RPC_DBG_GPRINTF(("(do_quit_body) Unknown version; Dropped [%s]\n", 
                rpc__dg_act_seq_string(rqe->hdrp)));
        return;
    }

    /*
     * Extract the cancel_id of the cancel.
     */

    cancel_id           = bodyp->cancel_id; 

    if (NDR_DREP_INT_REP(rqe->hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_32(cancel_id);
    }                                    
#else

    !!!

#endif

    /*
     * At this point we know the cancel is for the current or
     * a new call...
     */

    /*
     * If the cancel is for a newer call, we must have never received
     * the original request...  We could respond with a "nocall" but
     * why bother, the sender will eventually ping us and find that
     * out.
     */

    if (rqe->hdrp->seq > scall->c.call_seq)
        return;

    /*
     * The cancel is for a currently running / previous call.
     * If it is an "old" (duplicate) cancel request just ignore it.
     */
    if (cancel_id <= scall->c.c.u.server.cancel.count)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10, 
                ("(do_quit_body) Old cancel; Dropped [%s]\n", 
                rpc__dg_act_seq_string(rqe->hdrp)));
        return;
    }

    switch (scall->c.state)
    {
        /*
         * If the call is "in progress" give the new cancel
         * a chance to take affect.
         * If we're not accepting cancels, rpc__cthread_cancel will
         * "do the right thing".  
         *
         * While there's no current requirement to actually post multiple
         * cancel events (i.e. pthreads currently compresses things into
         * a single pending bit), why enforce that restriction.
         * 
         * Besides, we need to properly maintain the server_count because
         * we use it as our cancel_id (in cancel-acks).  The client will
         * keep transmitting cancel requests until we ack 'this' cancel.
         *
         * Notice that we post a cancel (using rpc__cthread_cancel()) 
         * even if this is a cbk_scall (i.e. the original client thread); 
         * at this point, this seems to be the right thing to do (see
         * rpc__dg_call_local_cancel()) - pretty scary huh?
         */
        case rpc_e_dg_cs_init:
        case rpc_e_dg_cs_xmit:
        case rpc_e_dg_cs_recv:
            {
                int i;

                RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10,
                        ("(do_quit_body) posting cancels [%s]\n",
                        rpc__dg_act_seq_string(rqe->hdrp)));
                for (i = (cancel_id - scall->c.c.u.server.cancel.count); i; i--)
                    rpc__cthread_cancel(&scall->c.c);
            }
            break;

        /*
         * If the call is 'idle' the call has already completed.
         * If the call is 'orphaned' we've already done all we can.
         * If the call is 'final' the cancel won't affect the call.
         * In either case, we're done.
         */
        case rpc_e_dg_cs_idle:
        case rpc_e_dg_cs_orphan:
        case rpc_e_dg_cs_final:
        default:
            break;
    }

    /*
     * Ack the cancel-request with the highest cancel-request's id
     * that we have accepted and our "server_is_accepting" state.
     *
     * Note: we need to do this *following* any rpc__cthread_cancel()
     * operations that are done.
     */
           
    RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10, 
                ("(do_quit_body) Sending cancel quack [%s]\n",
                rpc__dg_act_seq_string(rqe->hdrp)));

    scall_xmit_cancel_quack(
            scall, 
            scall->c.c.u.server.cancel.count, 
            scall->c.c.u.server.cancel.accepting);
}

/*
 * R P C _ _ D G _ D O _ Q U I T
 * 
 * Handle a call quit packet.  
 * 
 * There are two types of quit packets; orphans and cancels.  Orphan
 * quits implement NCK 1.5.1 compatable quit semantics to non-transparently
 * disassociate a server connection and call executor from a client call.
 * Orphan calls are non-transparent because they specifically alter the
 * runtime state of the call to be orphaned so that no further processing
 * (receives, transmits, etc) will be performed for the orphaned call.
 * Cancel quits, a new feature at NCK 2.0, are distinguished by an
 * 'embellishment' to the original quit packet contents.  Cancels
 * transparently deliver a notification to an executor thread without
 * altering the runtime state of the call. Runtime processing of the
 * cancelled call continues uninterrupted.
 */
   
PRIVATE boolean rpc__dg_do_quit
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_scall_p_t scall
)
{
    if (! rpc__dg_svr_chk_and_set_sboot(rqe, sp))    
    {
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    /*
     * Weed out old and duplicate "quit" packets.  For duplicate packets,
     * resend a "quack" under the assumption that the last "quack" we
     * sent was lost.
     */

    if (scall == NULL || RPC_DG_SEQ_IS_LT(rqe->hdrp->seq, scall->c.call_seq))
    {
        RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10, 
            ("(rpc__dg_do_quit) Old quit rcvd [%s]\n", 
            rpc__dg_act_seq_string(rqe->hdrp)));
        return (RPC_C_DG_RDF_FREE_RQE);
    } 

    /*
     * If it is a cancel-quit handle it.
     */
    if (! RPC_DG_IS_ORPHAN_QUIT(rqe))
    {
        do_quit_body(rqe, scall);
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    /*
     * This is an  Orphan Quit for the current / new call.
     * Always generate a quack.
     */

    RPC_DBG_PRINTF(rpc_e_dbg_cancel, 10, 
            ("(rpc__dg_do_quit) Sending orphan quack [%s]\n",
            rpc__dg_act_seq_string(rqe->hdrp)));
    rpc__dg_xmit_hdr_only_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, rqe->hdrp, 
            RPC_C_DG_PT_QUACK);

    /*
     * If the quit is for a newer call, we must have never received
     * the original request...  No problem, effectively we've already
     * finished the orphan processing; just generate a quack and we're done.
     */

    if (rqe->hdrp->seq > scall->c.call_seq)
        return (RPC_C_DG_RDF_FREE_RQE);

    /*
     * The orphan quit is for a currently running / previous  call.
     * A orphan quit terminates the INs data stream.
     */

    scall->c.rq.all_pkts_recvd = true;

    switch (scall->c.state)
    {
        /*
         * If the call is "in progress", initiate orphan processing.
         */
        case rpc_e_dg_cs_init:
        case rpc_e_dg_cs_xmit:
        case rpc_e_dg_cs_recv:
        case rpc_e_dg_cs_final:

            RPC_DBG_GPRINTF(("(rpc__dg_do_quit) Orphaning call [%s]\n",
                rpc__dg_act_seq_string(rqe->hdrp)));

            rpc__dg_scall_orphan_call(scall);
            break;

        /*
         * If the call is 'idle' the call has already completed.
         * If the call is 'orphaned' we've already done all we can.
         * In either case, we're done.
         */
        case rpc_e_dg_cs_idle:
        case rpc_e_dg_cs_orphan:
        default:
            break;
    }

    return (RPC_C_DG_RDF_FREE_RQE);
}


/*
 * R P C _ _ D G _ D O _ A C K
 *
 * Handle "ack" packets from a client.
 */

PRIVATE boolean rpc__dg_do_ack
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_scall_p_t scall
)
{
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    unsigned32 seq          = hdrp->seq;

    if (! rpc__dg_svr_chk_and_set_sboot(rqe, sp))    
    {
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    RPC_LOCK_ASSERT(0);

    /*
     * If there is no call handle, or if the ack's sequence number is
     * less than the one stored in the handle, just ignore the packet
     * (A packet must have been lost.)
     */

    if (scall == NULL || RPC_DG_SEQ_IS_LT(seq, scall->c.call_seq))
    {
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, ("(rpc__dg_do_ack) Old ACK [%s]\n",
            rpc__dg_act_seq_string(hdrp)));
    }

    /*
     * If the sequence numbers match, and we're in the final state, 
     * then free up the resources of this call.
     */

    else if (seq == scall->c.call_seq)
    {                 
        RPC_DG_CALL_LOCK_ASSERT(&scall->c);

        if (scall->c.state == rpc_e_dg_cs_final)
        {
            if (scall->c.xq.head != NULL) 
                rpc__dg_xmitq_free(&scall->c.xq, &scall->c);
            RPC_DG_CALL_SET_STATE(&scall->c, rpc_e_dg_cs_idle);
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 5, ("got ack\n"));
        }
        else
        {
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, ("(rpc__dg_do_ack) Wrong state (%s) [%s]\n",
                rpc__dg_call_state_name(scall->c.state), rpc__dg_act_seq_string(hdrp)));
        }
    }

    /*
     * Otherwise, someone screwed up.
     */

    else
    {
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, ("(rpc__dg_do_ack) Bad sequence number [%s]\n",
            rpc__dg_act_seq_string(hdrp)));
    }

    return (RPC_C_DG_RDF_FREE_RQE);
}


/*
 * P I N G _ C O M M O N
 *
 * Routine common to request and ping processing, in case we got a packet
 * whose sequence is current.
 */
INTERNAL void ping_common  (
        rpc_dg_sock_pool_elt_p_t  /*sp*/, 
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_scall_p_t  /*scall*/ 
    );

INTERNAL void ping_common
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_scall_p_t scall
)
{
    rpc_dg_xmitq_p_t xq = &scall->c.xq;
#ifdef DEBUG
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
#endif
      
    RPC_LOCK_ASSERT(0);
    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    switch (scall->c.state)
    {
    case rpc_e_dg_cs_recv:

        /*
         * If the call is in the receive state, and we have all the ins,
         * return a "working" packet.
         */

        if (scall->c.rq.all_pkts_recvd)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
                ("(ping_common) Working [%s]\n",
                rpc__dg_act_seq_string(hdrp)));

            rpc__dg_xmit_hdr_only_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, rqe->hdrp, 
                RPC_C_DG_PT_WORKING);
        }                                            

        /*
         * Return a "nocall" if we don't have all the ins yet.  The
         * "nocall" will carry a fack packet which will help clients
         * understand that "nocall" really just means that all the ins
         * weren't received and not that we really don't know about
         * the client's request.
         */

        else
        {
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
                ("(ping_common) No call (state=%s, rq->next_fragnum=%u) [%s]\n",
                rpc__dg_call_state_name(scall->c.state), 
                scall->c.rq.next_fragnum,
                rpc__dg_act_seq_string(hdrp)));

            rpc__dg_call_xmit_fack(&scall->c, rqe, true);
        }

        break;

    case rpc_e_dg_cs_xmit:
    case rpc_e_dg_cs_final:

        /*
         * If the call is in the transmit or final states, retransmit
         * the head of the transmit queue to get things started again.
         * First see if the client/call thread has managed to queue up
         * a packet.
         */
   
        if (xq->head == NULL)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
                ("(ping_common) No xmit queue [%s]\n",
                rpc__dg_act_seq_string(hdrp)));
        }
        else
        {     
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
                ("(ping_common) Retransmitting response (xq->head->fragnum=%u) [%s]\n",
                xq->head->fragnum,
                rpc__dg_act_seq_string(hdrp)));

            rpc__dg_xmitq_restart(&scall->c);
        }
        break;                         

    case rpc_e_dg_cs_orphan:

        /*
         * This should only happen for callback scalls that have yet
         * to complete their orphan processing (normal scalls should
         * never be attached to the SCT if they are orphaned).
         * 
         * For all practical purposes we're still "working".
         */

        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
            ("(ping_common) Orphan Working [%s]\n",
            rpc__dg_act_seq_string(hdrp)));

        rpc__dg_xmit_hdr_only_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, rqe->hdrp, 
                RPC_C_DG_PT_WORKING);
        break;                         

    case rpc_e_dg_cs_init:
    case rpc_e_dg_cs_idle:

        /*
         * Otherwise, return a "nocall."
         */

        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
            ("(ping_common) No call (state=%s, rq->next_fragnum=%u) [%s]\n",
            rpc__dg_call_state_name(scall->c.state), 
            scall->c.rq.next_fragnum,
            rpc__dg_act_seq_string(hdrp)));

        rpc__dg_xmit_hdr_only_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, rqe->hdrp, 
            RPC_C_DG_PT_NOCALL);
        break;

    default:
	/*
	 * rpc_m_unhandled_callstate
	 * "(%s) Unhandled call state: %s"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s%s"),
	    rpc_svc_recv,
	    svc_c_sev_fatal | svc_c_action_abort,
	    rpc_m_unhandled_callstate,
	    "ping_common",
            rpc__dg_call_state_name(scall->c.state) ));
	break;
    }
} 


/*
 * R P C _ _ D G _ D O _ P I N G
 *
 * Handle "ping" packets from a client.
 */

PRIVATE boolean rpc__dg_do_ping
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_scall_p_t scall
)
{
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    unsigned32 seq          = hdrp->seq;
    unsigned32 scall_seq;

    if (! rpc__dg_svr_chk_and_set_sboot(rqe, sp))    
    {
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    RPC_LOCK_ASSERT(0);

    /*
     * If there is no call handle, we're definitely not in a call.
     * If the call is in the "orphan" state the SCALL will be NULL
     * at this point.
     */

    if (scall == NULL)
    {
        rpc__dg_xmit_hdr_only_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, rqe->hdrp, 
            RPC_C_DG_PT_NOCALL);
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
            ("(rpc__dg_do_ping) No call (no call handle) [%s]\n",
            rpc__dg_act_seq_string(hdrp)));
        return (RPC_C_DG_RDF_FREE_RQE);
    }                            

    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    /*
     * If the ping's sequence number is greater than the one stored in
     * the handle, then say "no call".  (A packet must have been lost.)
     */

    scall_seq = scall->c.call_seq;

    /*
     * If the ping's sequence number is greater than the one stored in
     * the handle, then say "no call".  (A packet must have been lost.)
     */

    if (RPC_DG_SEQ_IS_LT(scall_seq, seq))
    {
        rpc__dg_xmit_hdr_only_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, rqe->hdrp, 
            RPC_C_DG_PT_NOCALL);
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, 
            ("(rpc__dg_do_ping) No call (higher numbered ping), previous=%lu [%s]\n",
            scall_seq, rpc__dg_act_seq_string(hdrp)));
    }                            

    /*
     * If the sequence number matches the one stored in the call handle,
     * call the common ping/retransmission routine.
     */

    else if (seq == scall_seq) 
        ping_common(sp, rqe, scall);

    /*
     * Otherwise, assume the ping is a duplicate and drop it.
     */

    else 
        RPC_DBG_PRINTF(rpc_e_dbg_recv, 1, ("(rpc__dg_do_ping) Duplicate ping [%s]\n", 
            rpc__dg_act_seq_string(hdrp)));

    return (RPC_C_DG_RDF_FREE_RQE);  
}


/*
 * D O _ R E Q U E S T _ C O M M O N
 *
 * Common request packet filtering for both non-callback and callback
 * existing SCALLs.  Returns the disposition of the pkt / call.
 *
 * Note that the sct handles maybe calls differently from other calls.
 * If the current request is for a maybe call, and an scall already
 * exists for that call, then the seq and state info sent to this routine
 * will be taken from that scall.  If an scall doesn't exist for this 
 * maybe call, then the seq and state info will represent the current
 * state of the scte (or of a currently executing non-maybe scall).
 */

typedef enum {
    do_req_e_old, 
    do_req_e_cur_call,
    do_req_e_orphan_cur_call,
    do_req_e_new_call,
    do_req_e_ping,
    do_req_e_non_idem_rerun
} do_req_enum_t;

INTERNAL do_req_enum_t do_request_common (
        unsigned32  /*cur_call_seq*/,
        rpc_dg_call_state_t  /*cur_call_state*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL do_req_enum_t do_request_common
(
    unsigned32 cur_call_seq,
    rpc_dg_call_state_t cur_call_state,
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    unsigned32 rqe_seq      = hdrp->seq;
    boolean idem            = RPC_DG_HDR_FLAG_IS_SET(hdrp, RPC_C_DG_PF_IDEMPOTENT);
    boolean maybe           = RPC_DG_HDR_FLAG_IS_SET(hdrp, RPC_C_DG_PF_MAYBE);

    /*
     * Process the rqe_seq == cur_call_seq pkts.
     */

    if (rqe_seq == cur_call_seq)
    {
        switch (cur_call_state) 
        {
        case rpc_e_dg_cs_idle:
            /*
             * This is an attempted rerun.  This is perfectly legitimate
             * for idempotent calls (reinit the call handle and accept the
             * packet) and illegal for non-idempotent calls.
             *
             * If this isn't the first pkt in a non-idem call, just ignore
             * it rather than sending a protocol error reject and possibly
             * confusing the sender.  If the sender really is trying to rerun
             * the call, they will eventually resend frag 0 and then we really
             * must reject it.
             */
            if (! idem)
            {
                if (hdrp->fragnum != 0)
                    return (do_req_e_old);

                RPC_DBG_GPRINTF(("(do_request_common) Duplicate non-idempotent call [%s]\n",
                    rpc__dg_act_seq_string(hdrp)));

                return (do_req_e_non_idem_rerun);
            }

            return (do_req_e_new_call);

        case rpc_e_dg_cs_final:
            /*
             * Treat the (probably duplicate) pkt as a ping and retransmit
             * the reply.
             */

            return (do_req_e_ping);

        case rpc_e_dg_cs_init:
        case rpc_e_dg_cs_recv:
            /*
             * The pkt is for the current in progress call and
             * the call is receiving pkts (presumably this is
             * a fragment of a call with large [ins]).  This
             * is the fast path; just add the pkt to the recvq.
             */

            return (do_req_e_cur_call);

        case rpc_e_dg_cs_xmit:
            /*
             * Since we're already transmitting, just ignore this
             * (probably duplicate) pkt.
             */
            RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
                ("(do_request_common) Duplicate pkt [%s]\n",
                rpc__dg_act_seq_string(hdrp)));

            return (do_req_e_old);

        case rpc_e_dg_cs_orphan:
            /*
             * If the existing call is in the orphan state, we really
             * just want to ignore the request since the call isn't
             * going to ever use it.
             * 
             * This should only happen for callback scalls that have
             * yet to complete their orphan processing (normal scalls
             * should never be attached to the SCT if they are orphaned).
             */

            RPC_DBG_GPRINTF(("(do_request_common) Call already orphaned [%s]\n",
                    rpc__dg_act_seq_string(hdrp)));

            return (do_req_e_old);

        default:
	    /*
	     * rpc_m_unhandled_callstate
	     * "(%s) Unhandled call state: %s"
	     */
	    RPC_DCE_SVC_PRINTF ((
		DCE_SVC(RPC__SVC_HANDLE, "%s%s"),
		rpc_svc_recv,
		svc_c_sev_fatal | svc_c_action_abort,
		rpc_m_unhandled_callstate,
		"do_request_common",
                rpc__dg_call_state_name(cur_call_state) ));
	    break;
        }
    }

    /*
     * Now process "new" calls (rqe_seq > cur_call_seq).
     */

    if (RPC_DG_SEQ_IS_LT(cur_call_seq, rqe_seq))
    {
        switch (cur_call_state) 
        {
        case rpc_e_dg_cs_idle:
            /*
             * This is a new call for a connection with a previously
             * completed a call.  In the case of idempotent calls, this
             * is the fast path (since idempotent calls currently don't
             * transition through the "final" state).  For non-idempotent
             * calls, this slow path stuff.  Either way, just reinit
             * and go.
             */

            return (do_req_e_new_call);

        case rpc_e_dg_cs_final:
            /*
             * The pkt is for new call, implicitly acking the previous
             * call (fast path).  Handle any implied ACK processing before
             * processing the new request.  This involves reinitializing
             * the SCALL (which frees any saved reply).
             */

            return (do_req_e_new_call);

        case rpc_e_dg_cs_init:
        case rpc_e_dg_cs_recv:
        case rpc_e_dg_cs_xmit:
            /*
             * A call is in progress and this is for a new call so we
             * must have missed a quit.  Kill off the existing call and
             * drop the pkt, immediately free up the SCTE to accept the
             * new call (when it is retried); the client will surely
             * retransmit the request.
             */

            RPC_DBG_GPRINTF(
                ("(do_request_common) Higher seq pkt killing call, current=%lu state=%s [%s]\n",
                cur_call_seq, rpc__dg_call_state_name(cur_call_state), 
                rpc__dg_act_seq_string(hdrp)));

            return (do_req_e_orphan_cur_call);

        case rpc_e_dg_cs_orphan:
            /*
             * This should only happen for callback scalls that have
             * yet to complete their orphan processing (normal scalls
             * should never be attached to the SCT if they are orphaned).
             *
             * But if the scall *is* still in the sct, and this new call is
             * a maybe call, we can let it through (since it will be attached
             * to the sct on the "maybe chain.")
             */

            RPC_DBG_GPRINTF(("(do_request_common) Awaiting Orphan Completion [%s]\n",
                    rpc__dg_act_seq_string(hdrp)));

            if (maybe)
                return (do_req_e_new_call);
            else
                return (do_req_e_old);

        default:
	    /*
	     * rpc_m_unhandled_callstate
	     * "(%s) Unhandled call state: %s"
	     */
	    RPC_DCE_SVC_PRINTF ((
		DCE_SVC(RPC__SVC_HANDLE, "%s%s"),
		rpc_svc_recv,
		svc_c_sev_fatal | svc_c_action_abort,
		rpc_m_unhandled_callstate,
		"do_request_common",
                rpc__dg_call_state_name(cur_call_state) ));
	    break;
        }
    }

    /*
     * If it isn't for the current call and it isn't for a new call, 
     * it must be old.  For a maybe call, that's okay, for all others,
     * drop the call.
     */

    if (maybe)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, 3,
            ("(do_request_common) Running old maybe, previous=%lu [%s]\n",
            cur_call_seq, rpc__dg_act_seq_string(hdrp)));
 
        return (do_req_e_new_call);
    }
    else
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, 3,
            ("(do_request_common) Old sequence, previous=%lu [%s]\n",
            cur_call_seq, rpc__dg_act_seq_string(hdrp)));

        return (do_req_e_old);
    }
}


/*
 * D O _ C B K _ R E Q U E S T
 *
 * Filter the callback request packet.
 * Return true and locked SCALL (create one if necessary) if 
 * the request is to be accepted, otherwise false and no locked SCALL.
 *
 * Handle callbacks.  The goal is to come up with an SCALL that we
 * can check using the common SCALL processing.
 */

INTERNAL boolean do_cbk_request (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_scall_p_t * /*scallp*/
    );

INTERNAL boolean do_cbk_request
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_scall_p_t *scallp
)
{
    rpc_dg_scall_p_t scall;
    rpc_dg_ccall_p_t ccall;
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;

    boolean drop;

    if (RPC_DG_HDR_FLAG_IS_SET(hdrp, RPC_C_DG_PF_FORWARDED))
    {
        RPC_DBG_GPRINTF(("(do_cbk_request) Bogus forwarding on callback [%s]\n",
                        rpc__dg_act_seq_string(hdrp)));
        return (false);
    }
    
    /*
     * Lookup the request packet's activity ID in the CCALL table.  If
     * there's no CCALL, the callback is bogus (except for WAY case [see
     * above]).  If the CCALL has a callback SCALL, use the SCALL as
     * the SCALL for this request.  If there is no SCALL yet, create
     * one and stash it in the CCALL.  (The CCALL is prodded to get the
     * original client to unblock and run the callback -- the client
     * gets the SCALL by looking in its CCALL.)
     */

    ccall = rpc__dg_ccallt_lookup(&hdrp->actuid, hdrp->ahint);

    if (ccall == NULL)
    {
        RPC_DBG_GPRINTF(("(do_cbk_request) Unknown callback activity [%s]\n",
            rpc__dg_act_seq_string(hdrp)));
    
        return (false);
    }

    /*
     * A CCALL with the appropriate activity is present.
     */

    ccall->c.last_rcv_timestamp = rpc__clock_stamp();

    /* 
     * get the connection's current scall (if one exists).
     */
    rpc__dg_ccall_lsct_inq_scall(ccall, &scall);

    /*
     * Determine the disposition of the pkt / call and 
     * take the appropriate action.
     */
    drop = false;
    switch (do_request_common(ccall->c.high_seq,
                scall != NULL ? scall->c.state : rpc_e_dg_cs_idle,
                rqe))
    {
        case do_req_e_orphan_cur_call:
            rpc__dg_scall_orphan_call(scall);
            /* fall through to drop the pkt - see orphan_call comments */
        case do_req_e_old:
            drop = true;
            break;
        case do_req_e_cur_call:
            /* nothing special to do */
            break;
        case do_req_e_new_call:
            rpc__dg_ccall_lsct_new_call(ccall, sp, rqe, &scall);
            if (scall == NULL)
            {
                RPC_DBG_GPRINTF(("(do_cbk_request) Alloc scall failed\n"));
                drop = true;
            }
            break;
        case do_req_e_ping:
            ping_common(sp, rqe, scall);
            drop = true;
            break;
        case do_req_e_non_idem_rerun:
            rpc__dg_xmit_error_body_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, 
                hdrp, RPC_C_DG_PT_REJECT, nca_s_proto_error); 
            drop = true;
            break;
        default:
	    /*
	     * rpc_m_call_failed
	     * "%s failed"
	     */
	    RPC_DCE_SVC_PRINTF ((
		DCE_SVC(RPC__SVC_HANDLE, "%s"),
		rpc_svc_recv,
		svc_c_sev_fatal | svc_c_action_abort,
		rpc_m_call_failed_no_status,
		"do_cbk_request/do_request_common" ));
	    break;
    }

    *scallp = scall;
    RPC_DG_CCALL_RELEASE(&ccall);

    if (drop)
        return (false);

    return (true);
}


/*
 * R P C _ _ D G _ D O _ R E Q U E S T
 *
 * Handle a "request" and "idem_request" packets.
 * (client->server packet type)
 *
 * The ultimate goal is to validate the pkt, establish a connection and
 * call handle for this call, queue the pkt on the call's receive queue
 * and get a call executor task (or in the case of a callback request,
 * the original client call task) to process the pkt, eventually
 * dispatching to the server stub.
 * 
 * The model is that call executors are only passed pkts for a single
 * (current) call.  This means we do a lot of filtering, to handle the
 * various situations that can arise in a unreliable datagram based
 * environment.
 * 
 * We take an optimistic approach for the "fast path" at the expense
 * of slightly less efficiency in the "slow path".  The "fast path" is
 * defined as non-callback processing for (a) a request pkt for a new
 * call and (b) request pkts for in progress calls (i.e. large [ins]).
 * All other cases are handled out of line.
 */

PRIVATE boolean rpc__dg_do_request
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_dg_sct_elt_p_t scte;
    boolean rqe_wake_thread;
    unsigned32 st;
    rpc_dg_scall_p_t scall  = NULL;
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    boolean is_cbk          = RPC_DG_SOCKET_IS_CLIENT(sp);
    boolean maybe           = RPC_DG_HDR_FLAG_IS_SET(hdrp, RPC_C_DG_PF_MAYBE);
    boolean drop;
    boolean client_needs_sboot;

    client_needs_sboot = (rqe->hdrp->server_boot == 0);

    if (! rpc__dg_svr_chk_and_set_sboot(rqe, sp))    
    {
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    RPC_LOCK_ASSERT(0);

    /*
     * Distinguish between calls and callbacks.  The point is to have
     * a usable server call handle after this mongo "if" statement.
     */

    if (is_cbk)
    {
        drop = (do_cbk_request(sp, rqe, &scall) == false);
    } 
    else 
    {
        /*
         * If the call is bound for the convc manager interface, hand
         * it off to a handler which will handle it.  Because the convc
         * call has the "maybe" attribute, we can just pretend we never
         * saw this packet.  See comments above "rpc__dg_handle_convc"
         * for more details on INDY processing.
         */
           
        if (UUID_EQ(hdrp->if_id, ((rpc_if_rep_p_t) convc_v1_0_c_ifspec)->id, &st))
        {
            rpc__dg_handle_convc(rqe);
            return (RPC_C_DG_RDF_FREE_RQE);
        }
    
        /*
         * This is a normal call.  Lookup the connection in the server
         * connection table (create one if necessary).  Filter the packet
         * based on our present knowledge of the connection's highest sequence 
         * number / call state information.
         */

        scte = rpc__dg_sct_get(&hdrp->actuid, hdrp->ahint, hdrp->seq);

        /* 
         * get the connection's current scall (if one exists).
         */
        rpc__dg_sct_inq_scall(scte, &scall, rqe);

        /*
         * Determine the disposition of the pkt / call and 
         * take the appropriate action.
         */
        drop = false;
        switch (do_request_common(scte->high_seq, 
                    scall != NULL ? scall->c.state : rpc_e_dg_cs_idle,
                    rqe))
        {
            case do_req_e_old:
                drop = true;
                break;
            case do_req_e_cur_call:
                /* nothing special to do */
                break;
            case do_req_e_orphan_cur_call:
                rpc__dg_scall_orphan_call(scall);
                if (! maybe)
                {
                    drop = true;
                    break;
                }
                /*
                 * else, drop thru...
                 */
            case do_req_e_new_call:
                rpc__dg_sct_new_call(scte, sp, rqe, &scall);
                if (scall == NULL)
                {
                    RPC_DBG_GPRINTF(("(do_request) Alloc scall failed\n"));
                    drop = true;
                }
                break;
            case do_req_e_ping:
                ping_common(sp, rqe, scall);
                drop = true;
                break;
            case do_req_e_non_idem_rerun:
                rpc__dg_xmit_error_body_pkt(sp->sock, (rpc_addr_p_t) &rqe->from, 
                    hdrp, RPC_C_DG_PT_REJECT, nca_s_proto_error); 
                drop = true;
                break;
            default:
	        /*
	         * rpc_m_call_failed
	         * "%s failed"
	         */
	        RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%s"),
		    rpc_svc_recv,
		    svc_c_sev_fatal | svc_c_action_abort,
		    rpc_m_call_failed_no_status,
		    "do_request/do_request_common" ));
	        break;
        }

        /*
         * Release the scte. Either we're droping the pkt or we're
         * continuing and we've got a scall that's referencing it.
         */
        RPC_DG_SCT_RELEASE(&scte);
    }

    if (drop)
    {
        if (scall != NULL) 
            RPC_DG_CALL_UNLOCK(&scall->c);
        return (RPC_C_DG_RDF_FREE_RQE);
    }

    /*
     * If we get here, we've got the correct SCALL and we're gonna try
     * to add the pkt to its receive queue and get someone to process
     * it.  A callback may be required before we know if we're really
     * going to accept the call, but that's the call executor's job to
     * figure that out and clean up if it ends up rejecting it (it also
     * performs other defered pkt header verification checks).
     */

    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    scall->c.last_rcv_timestamp = rpc__clock_stamp();
    scall->client_needs_sboot = client_needs_sboot;

    /*
     * Check to see if the pkt is the second half of a two-packet forward.
     * To be so, (1) we must have the first half, (2) the "forwarded"
     * flag must NOT be set in this pkt (go figure!), and (3) the activity
     * ID and sequence and fragment numbers of this pkt must match the
     * ones in the first half.  If all that's so, then jam the source
     * address and drep from the first rqe into the current one, free
     * the first rqe, and proceed as if none of this nightmare had
     * happened.
     */

    if (scall->fwd2_rqe != NULL  
        && ! RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_FORWARDED)
        && UUID_EQ(rqe->hdrp->actuid, scall->fwd2_rqe->hdrp->actuid, &st)
        && rqe->hdrp->seq == scall->fwd2_rqe->hdrp->seq
        && rqe->hdrp->fragnum == scall->fwd2_rqe->hdrp->fragnum)
    {
        rpc_dg_fpkt_p_t fpkt = (rpc_dg_fpkt_p_t) scall->fwd2_rqe->pkt;
        struct sockaddr *sp = (struct sockaddr *) &rqe->from.sa;

        rqe->from.len = fpkt->fhdr.len;
        *sp = fpkt->fhdr.addr;
        rpc__naf_addr_overcopy((rpc_addr_p_t) &rqe->from, &scall->c.addr, &st);

        rqe->hdrp->drep[0] = fpkt->fhdr.drep[0];
        rqe->hdrp->drep[1] = fpkt->fhdr.drep[1];
        rqe->hdrp->drep[2] = fpkt->fhdr.drep[2];

        rpc__dg_pkt_free_rqe(scall->fwd2_rqe, &scall->c);
        scall->fwd2_rqe = NULL;
    }

    /*
     * If this is the first half of a two-packet forward, then just stash
     * this rqe away for now and return.  (Note that single packet forwards
     * are handled by "recv_pkt" in "dglsn.c".)
     */

    if (RPC_DG_HDR_FLAG2_IS_SET(hdrp, RPC_C_DG_PF2_FORWARDED_2))
    {   
        if (scall->fwd2_rqe != NULL)
            rpc__dg_pkt_free_rqe(scall->fwd2_rqe, &scall->c);
        scall->fwd2_rqe = rqe;
        RPC_DG_CALL_UNLOCK(&scall->c);
        return (0 /* DON'T free the rqe */);
    }
               
    /*
     * We are going to accept this packet, subject to resource constraints
     * which will be considered below.  If a call executor has not yet
     * been set up for the call, do so now.  For callbacks, we prod the
     * application thread to run the callback.  Otherwise, we allocate
     * a server call executor thread to run the call.  Note that, because
     * of rationing, we may setup a call executor, but then drop the
     * packet.  This is okay, a fack will be returned and the client will
     * retransmit.
     */

    if (! scall->call_is_setup)
    {
        /*
         * Change to the recv state (we're logically "working" on
         * the call now) and assign a call executor.
         */

        RPC_DG_CALL_SET_STATE(&scall->c, rpc_e_dg_cs_recv);

        scall->call_is_setup = true;

        if (is_cbk)
        {
            rpc_dg_ccall_p_t ccall;

            /*
             * Note: call_wait() sets up the callback call executor ref
             * stuff for rpc__dg_execute_call().
             */

            ccall = scall->cbk_ccall;

            /*
             * This is essentially a turnaround. The client, which is waiting
             * for a response, becomes the receiver.
             * (See rpc__dg_scall_reinit()::dgscall.c.)
             *
             * The server is assuming that we have enough packets reserved for
             * what we have been sending (that's true!).
             * (See ccall_common_init()::dgccall.c.)
             * Since we have inherited the origincal ccall's reservation, no
             * need to make the reservation.
             *
             */
    
            RPC_DG_CALL_UNLOCK(&scall->c);
            RPC_DG_CALL_LOCK(&ccall->c);

            /* 
             * In compliance with the packet rationing rules, free any
             * packets on the originating ccall's transmit queue since
             * the callback is (implicitly) ack'ing them (in the same
             * way as a response does).  Making sure that the ccall is
             * not holding any packets allows us to transfer its packet
             * reservation onto the newly created scall.
             */
        
            if (ccall->c.xq.head != NULL)
                rpc__dg_xmitq_free(&ccall->c.xq, &ccall->c);

            ccall->cbk_start = true;
            rpc__dg_call_signal(&ccall->c);

            RPC_DG_CALL_UNLOCK(&ccall->c);
            RPC_DG_CALL_LOCK(&scall->c);
        }
        else
        {
            rpc_dg_scall_p_t    ce_scall_ref;

            /*
             * Create a reference for the call executor.
             */
            RPC_DG_CALL_REFERENCE(&scall->c);
            ce_scall_ref = scall;
            scall->has_call_executor_ref = true;

            RPC_DG_STATS_INCR(calls_rcvd);

            /*
             * Setup a call executor.
             */

            rpc__cthread_invoke_null(
                (rpc_call_rep_p_t) scall, &hdrp->object,
                &hdrp->if_id, hdrp->if_vers, hdrp->opnum,
                rpc__dg_execute_call, (pointer_t) scall, 
                &st);

            switch ((int)st)
            {
                case rpc_s_ok:
                    /*
                     * Try to make a reservation with the packet pool.
                     * Since we are calling from the listener thread,
                     * specify that the call not block.  It's okay if a
                     * reservation can't be granted at this time;  the
                     * packet will get dropped in recvq_insert, and the
                     * call will try again to make a reservation (blocking
                     * this time) in execute_call.
                     */
    
                    if (scall->c.n_resvs < scall->c.max_resvs)
                    {
                        rpc__dg_pkt_adjust_reservation(&scall->c,
                                                       scall->c.max_resvs,
                                                       false);
                    }

                    break;

                case rpc_s_call_queued:
                    scall->call_is_queued = true;
                    if (scall->c.n_resvs < scall->c.max_resvs)
                    {
                        rpc__dg_pkt_adjust_reservation(&scall->c,
                                                       scall->c.max_resvs,
                                                       false);
                    }
                    RPC_DBG_GPRINTF((
              "(rpc__dg_do_request) Call was queued with%s reservation [%s]\n",
                                     ((scall->c.n_resvs == 0)?"out":""),
                                     rpc__dg_act_seq_string(hdrp)));
                    break;

                default:
                    /*
                     * We couldn't give the call to an executor and the
                     * call couldn't be queued waiting for one (i.e. there
                     * is no longer a call executor reference).  Reset
                     * the SCALL back to the idle state to make it reclaimable.
                     */
                   
                    RPC_DBG_GPRINTF((
                        "(rpc__dg_do_request) No call executors [%s]\n",
                        rpc__dg_act_seq_string(hdrp)));
    
                    RPC_DG_CALL_SET_STATE(&scall->c, rpc_e_dg_cs_idle);
                    rpc__dg_recvq_free(&scall->c.rq);

                    /*
                     * Arrange for a retransmit not not appear as a rerun...
                     * The client will eventually ping and retransmit
                     * and we'll hopefully acquire an executor before
                     * it times out.
                     */
                    rpc__dg_sct_backout_new_call(scall->scte, hdrp->seq);

                    scall->has_call_executor_ref = false;
                    RPC_DG_SCALL_RELEASE(&ce_scall_ref);
                                                    
                    return (RPC_C_DG_RDF_FREE_RQE);
            }
        }
    }
    
    /*
     * Add the pkt to the receive queue, subject to queue resource
     * utilization constraints.  Once the pkt has been added to the queue,
     * the call executor (or client task in the event of a callback)
     * is responsible for freeing the pkt.
     */

    drop =  ! rpc__dg_call_recvq_insert(&scall->c, rqe, &rqe_wake_thread);

    /*
     * As an optimization, under normal conditions, we only wake the
     * thread if it has several packets available to read; this cuts
     * down on the overhead of a task switch for every packet.  (Under
     * rationing, we would want to wake the thread for one packet, since
     * that's all we'll allow it to queue.)  The determination of whether
     * to wake the thread is done is recvq_insert.  Note: this general
     * processing strategy will delay a server's generation of a required
     * WAY callback until it has queued up the number of inorder packets
     * specified in the call handle's rq.
     */

    if (rqe_wake_thread) 
        rpc__dg_call_signal(&scall->c);

    RPC_DG_CALL_UNLOCK(&scall->c);
  
    /*
     * Decide on a return status.  If the packet was dropped, tell the
     * caller to free the packet.  Otherwise, if we signalled the call
     * that the packet was  queued to, tell the listener thread to yield.
     * Otherwise, return 0, meaning, don't free the packet, or yield.
     */

    if (drop)
        return (RPC_C_DG_RDF_FREE_RQE);

    if (rqe_wake_thread) 
        return (RPC_C_DG_RDF_YIELD /* and DON'T free the rqe */);

    return (0);
}       
