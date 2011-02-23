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
**      dgexec.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  
**
**
*/                

/* ========================================================================= */

#include <commonp.h>
#include <dg.h>
#include <dgpkt.h>
#include <dgrq.h>
#include <dgxq.h>
#include <dgsct.h>
#include <dgexec.h>
#include <dgscall.h>
#include <dgcall.h>
#include <dghnd.h>
#include <comcthd.h>
#include <comauth.h>

/* ========================================================================= */

INTERNAL void queue_mapped_reject ( 
        rpc_dg_scall_p_t  /*scall*/,
        unsigned32        /*st*/
    );

/* ========================================================================= */
/*
 * Declare a global pointer to a routine that can handle calls to 
 * pre-v2 server stubs.  If the compatibility library is used to
 * register pre-v2 interfaces, then it will initialize this 
 * pointer to the appropriate routine.
 */

GLOBAL  rpc__dg_pre_v2_server_fn_t rpc_g_dg_pre_v2_server_call_p = NULL;

/* ========================================================================= */

/*
 * Q U E U E _ M A P P E D _ R E J E C T
 *
 * This routine is used by rpc__dg_execute_call when it receives an error
 * status back from a stub.  First, we translate from the rpc_s_* error
 * code to an equivalent nca_s_* code, and then we reinitialize the xmitq,
 * (just like rpc__dg_call_fault()) and queue a reject packet.  Normal
 * output processing will then be used to send this reject packet to
 * the client.  Note that a "orphan induced" reject will never make
 * it to the client (hence we don't really care what the stcode mapping is).
 * We specify the mapping just so that we don't get spurious messages.
 */

INTERNAL void queue_mapped_reject
(
    rpc_dg_scall_p_t scall,
    unsigned32 st
)
{     
    rpc_iovector_t iovec;
    unsigned32 tst, mst;

    switch ((int)st)
    {
        case rpc_s_who_are_you_failed:
                                        mst = nca_s_who_are_you_failed;  break;
        case rpc_s_comm_failure:        mst = nca_s_comm_failure;        break;
        case rpc_s_unknown_if:          mst = nca_s_unk_if;              break;
        case rpc_s_protocol_error:      mst = nca_s_proto_error;         break;
        case rpc_s_unsupported_type:    mst = nca_s_unsupported_type;    break;
        case rpc_s_manager_not_entered: mst = nca_s_manager_not_entered; break;
        case rpc_s_op_rng_error:        mst = nca_s_op_rng_error;        break;
        case rpc_s_call_orphaned:       mst = nca_s_unspec_reject;       break;
        case rpc_s_unknown_reject:      mst = nca_s_unspec_reject;       break;
        case rpc_s_unknown_mgr_type:    mst = nca_s_unsupported_type;    break;
        default:                        
            RPC_DBG_GPRINTF(("(queue_mapped_reject) unknown status; st=0x%x\n", st));
            mst = nca_s_unspec_reject;       
            break;
    }

    /*
     * Build the iovector for calling transmit_int
     */

    iovec.num_elt = 1;
    iovec.elt[0].buff_dealloc = NULL;
    iovec.elt[0].flags = rpc_c_iovector_elt_reused;
    iovec.elt[0].data_addr = (byte_p_t) &mst;
    iovec.elt[0].data_len = sizeof(st);

    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    /*
     * Purge the recvq since it won't be used after this.  The recvq
     * may currently have lots of rqes on it and freeing it now will
     * help pkt rationing.  It's likely that the recvq is already empty
     * however, this is the slow path so do it (again) just to be sure.
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

    RPC_DBG_GPRINTF(("(queue_mapped_reject) st=0x%x => 0x%x [%s]\n", 
        st, mst, rpc__dg_act_seq_string(&scall->c.xq.hdr)));

    RPC_DG_XMITQ_REINIT(&scall->c.xq, &scall->c);
    RPC_DG_HDR_SET_PTYPE(&scall->c.xq.hdr, RPC_C_DG_PT_REJECT);
    
    rpc__dg_call_transmit_int(&scall->c, &iovec, &tst);
    /*
     * The transmit may fail because the call is already orphaned.
     * It may fail for some other reason as well.  In either case,
     * we're not gonna get a response to the client.  Just keep
     * falling through (other calls may fail as well) and clean up.
     */
}


/*
 * R P C _ _ D G _ E X E C U T E _ C A L L 
 * 
 * Perform final validation of the request (including a callback if
 * necessary).  If everything checks out, do what's necessary to dispatch
 * to the server stub.
 */

PRIVATE void rpc__dg_execute_call
(
    pointer_t scall_,
    boolean32 call_was_queued ATTRIBUTE_UNUSED
)
{
    ndr_format_t drep;
    unsigned32 st, reject_st;
    boolean broadcast;
    boolean idem = false;
    boolean maybe;
    boolean sent_response;
    boolean called_stub;
    rpc_dg_scall_p_t scall = (rpc_dg_scall_p_t) scall_;
    rpc_dg_pkt_hdr_p_t hdrp;
    rpc_iovector_elt_t iove;
    rpc_dg_recvq_elt_p_t rqe;
    unsigned16 ihint;
    rpc_dg_binding_server_p_t h;
    rpc_v2_server_stub_epv_t ss_epv;
    rpc_mgr_epv_t mgr_epv;
    rpc_if_rep_p_t ifspec;
    dce_uuid_t type;
    int force_way_auth;
    rpc_key_info_p_t key_info;
    rpc_dg_auth_epv_p_t auth_epv;
    unsigned16 opnum;
    unsigned32 flags;
    unsigned32 max_calls;
    unsigned32 max_rpc_size;
    rpc_if_callback_fn_t if_callback;
    int prev_cancel_state;

    /*
     * All of this code (99% of which is never executed) is in the fast path.
     *
     * NOTE: This routine is responsible for sending back a correct
     * cancel pending status to the client under all conditions
     * (to ensure that cancels don't get lost - i.e. forwarded to the
     * server, accepted, not delivered and then not reported as
     * a cancel pending).
     *
     * Any "reject response" to the client must be robust (at least for
     * Non-Idempotent calls).  This is necessary because the client may 
     * have already received a fack causing it to free some pkts that it 
     * would need to "rerun" the call (assuming the stub was never entered) 
     * in the event that a reject was lost.
     *
     * Client's recover from lost responses to idempotent calls (including
     * proper cancel pending resetting) so we don't have to worry about
     * being robust in this situation.
     */

    /*
     * The caller of this routine is responsible for handing off a
     * call *reference* to us.  We will release our reference when
     * we're done.
     */

    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    /*
     * We are now executing.
     */
    scall->call_is_queued = false;

    /*
     * Initialize the iove, since in any failure case (i.e. orphan),
     * it may not be updated correctly; subsequent logic depends on freeing
     * things based on the proper state of the iove.
     */

    iove.buff_dealloc = NULL; 

    /*
     * Initialize the "called_stub" flag to false.  If a call gets
     * rejected, and never enters the stub routine, it's up to us to
     * free the request RQE.
     */
    called_stub = false;

    /*
     * Initialize the "sent call response" flag to indicate a failure.
     * This is necessary so that failures resulting in END_OF_CALL
     * end up transitioning to the proper call state when we wrap-up
     * call processing (at the end of this routine).
     */
    sent_response = false;

    /*
     * Before continuing, it's likely that the call has been "opened
     * up" (due to a unlock/lock around call executor handoff) and we
     * need to check if it is safe to continue...
     */
    if (scall->c.state != rpc_e_dg_cs_recv)
        goto END_OF_CALL;

    /*
     * If this call does not yet have a reservation, make one now.  Any
     * call that was queued will not have a reservation; also, even if
     * a executor thread was initially available for the call, there
     * might not have been any reservations available at that time.  
     * (Note that the call to make the reservation may block until a
     * reservation becomes available.)
     *
     * The make_reservation routine requires that the global lock be
     * held.  To respect the locking heirarchy, we need to juggle the
     * locks around a little, checking that the state of the call doesn't
     * change during the times when it's unlocked.
     */

    if (scall->c.n_resvs < scall->c.max_resvs)
    {                                  
        RPC_DG_CALL_UNLOCK(&scall->c);
        RPC_LOCK(0);
        RPC_DG_CALL_LOCK(&scall->c);
        if (scall->c.state != rpc_e_dg_cs_recv)
        {
            RPC_UNLOCK(0);
            goto END_OF_CALL;
        }             

        /*
         * We always start with the maximum reservation because we no longer
         * reset high_rcv_frag_size and snd_frag_size between the calls.
         * (The previous call may have used/advertised the larger fragment
         * size.)
         *
         * This is fine in the user space since the packet rationing will
         * never happen. (We assume that there are always enough packet
         * buffers available.)
         *
         * This may accelerate the packet rationing in the kernel, though
         * (iff MBF is turned on). Unfortunately, we can't start with the
         * minimum reservation in the kernel because the other end may be a
         * user space.
         */
        rpc__dg_pkt_adjust_reservation(&scall->c, scall->c.max_resvs, true);

        RPC_UNLOCK(0); 
         
        /*
         * Since the call's been opened up, we need to check its status.
         */ 
        if (scall->c.state != rpc_e_dg_cs_recv)
        {
            RPC_DBG_GPRINTF((
                "(rpc__dg_execute_call) Cancelled while awaiting pkt reservation\n"));
            goto END_OF_CALL;
        }

        /*
         * Since this call did not have a reservation, any data received for
         * it was dropped, and the client was told not to send any more.
         * Since the call can now receive data, prod the client into
         * retransmitting.
         */
        rpc__dg_call_xmit_fack(&scall->c, NULL, ! scall->c.rq.recving_frags);
    }

    /*
     * Now's as good a time as any to enable direct cancel posting to
     * the thread (while we've got the call lock held).  It might have
     * been nice to defer this to just before the sstub dispatch, but
     * then we'd have to re-acquire the call lock.
     *
     * NOTE: This routine MUST call rpc_cthread_cancel_caf() before
     * returning (regardless of the return path)!  This requirement
     * exists because cancels may be (become) pending at any time and
     * must be flushed (otherwise subsequent calls using this thread
     * will inherit this call's cancel).
     */

    rpc__cthread_cancel_enable_post(&scall->c.c);

    /*
     * Create a server binding handle, if we don't already have one hanging
     * off the scall.  If we have a cached one, reinit it.
     */

    if (scall->h != NULL)
    {
        h = scall->h;
        RPC_DG_BINDING_SERVER_REINIT(h);
    }
    else
    {                   
        rpc_addr_p_t addr;

        rpc__naf_addr_copy(scall->c.addr, &addr, &st);
        h = (rpc_dg_binding_server_p_t) rpc__binding_alloc
            (true, &scall->c.call_object, RPC_C_PROTOCOL_ID_NCADG, addr, &st);
        if (st != rpc_s_ok)
        {
            RPC_DBG_GPRINTF((
                "(rpc__dg_execute_call) Can't allocate binding, st = 0x%x\n", st));
            goto END_OF_CALL;
        }

        RPC_DG_CALL_REFERENCE(&scall->c);
        h->scall = scall;  

        if (!scall->c.is_cbk)
        {
            key_info = scall->scte->key_info;
            if (key_info != NULL)
            {
                rpc_auth_info_p_t auth_info = key_info->auth_info;
                h->c.c.auth_info = auth_info;
                RPC_DG_AUTH_REFERENCE(auth_info); /* for the handle */
            }
        }       

        scall->h = h;
    }

    assert(RPC_DG_CALL_IS_SERVER(&scall->c));

    /*
     * Dequeue the first pkt off of the receive queue (including it's hdr).
     *
     * WARNING: we MUST use comm_receive_int() because comm_receive(),
     * while it would do the locking for us, doesn't return a useable iove
     * for 0 length data.
     *
     * We're supposed to be in the init state until we know we're accepting
     * the call (that means after a WAY callback if one is necessary).
     * Make certain this is the case following the receive.
     * 
     * WARNING 2: Note that this call verifies the authenticity of the 
     * packet it reads *except* in two cases:
     * 
     *  - When the packet is from a call on an activity the server doesn't
     * currently know about (in which case we notice later on that the
     * authn_proto field in the header is non-zero).
     * 
     *  - When the authentication check fails with a status code of
     * "rpc_s_dg_need_way_auth".  Note that in this event, the
     * "receive_int" is still viewed as having succeeded, albeit with
     * a non-zero status code.
     * 
     * In either of these cases, a way_auth callback is made, and, 
     * if it is successful, the authenticity check is retried 
     * (further down in this function).  
     */

    rpc__dg_call_receive_int(&scall->c, &iove, &st);
    force_way_auth = false;
    if (st == rpc_s_dg_need_way_auth) {
        RPC_DBG_PRINTF(rpc_e_dbg_general, 4,
            ("(rpc__dg_execute_call) will force way callback\n"));
        st = rpc_s_ok;
        /*
         * We don't own the rqe. It's still on recvq.
         */
        force_way_auth = true;
    }
    else if (st != rpc_s_ok)
    {
        RPC_DBG_GPRINTF((
            "(rpc__dg_execute_call) Receive failed st = 0x%x\n", st));
        goto END_OF_CALL;
    }

    rqe = RPC_DG_RECVQ_ELT_FROM_IOVECTOR_ELT(&iove);
    assert(rqe != NULL && rqe->hdrp != NULL);
    hdrp = rqe->hdrp;
    idem  = ((hdrp->flags & RPC_C_DG_PF_IDEMPOTENT) != 0);
    broadcast = ((hdrp->flags & RPC_C_DG_PF_BROADCAST) != 0);
    maybe  = ((hdrp->flags & RPC_C_DG_PF_MAYBE) != 0);

    if (scall->c.is_cbk)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
            ("(rpc__dg_execute_call) Callback [%s]\n", 
                rpc__dg_act_seq_string(hdrp)));
    }

    /*
     * Perform some of the request pkt verification that was defered.
     * This includes interface id and operation number.
     */

    if (!scall->c.is_cbk)
        key_info = scall->scte->key_info;
    else
        key_info = NULL;

    /*
     * Does the request specify authentication, do we not have auth info
     * yet, is the call not "maybe", and is this not a callback (!!!
     * for the callback case)?  If so, then get the auth info now.
     */
    if (hdrp->auth_proto != 0 &&
        key_info == NULL &&
        ! maybe &&
        ! scall->c.is_cbk)
    {
        rpc_authn_protocol_id_t authn_protocol;
        rpc_auth_info_p_t auth_info;
        
        assert(scall->c.key_info == NULL);

        /*
         * Get the appropiate DG auth EPV.  We need to convert the wire
         * auth protocol ID into the corresponding API value and then
         * get the EPV using that latter value.
         */
        authn_protocol = rpc__auth_cvt_id_wire_to_api(hdrp->auth_proto, &st);
        if (st != rpc_s_ok)
        {
            reject_st = rpc_s_unknown_reject;
            goto AFTER_CALL_TO_STUB;
        }
        auth_epv = (rpc_dg_auth_epv_p_t) 
                        rpc__auth_rpc_prot_epv
                            (authn_protocol, RPC_C_PROTOCOL_ID_NCADG);
        if (auth_epv == NULL)
        {
            reject_st = rpc_s_unknown_reject;
            goto AFTER_CALL_TO_STUB;
        }

        /* 
         * Call into auth service to create an auth info.
         * 
         * This generates an auth_info and a key_info.  The auth_info
         * gets attached to the handle, while the key_info gets
         * attached to the scte and scall.
         */
        key_info = (*auth_epv->create) (&st);
        if (st != rpc_s_ok) 
        {
            reject_st = rpc_s_unknown_reject;
            goto AFTER_CALL_TO_STUB;
        }
        scall->c.key_info = key_info;
        scall->c.auth_epv = auth_epv;
        /* we have one reference to the key_info already. */
        scall->scte->key_info = key_info;
        scall->scte->auth_epv = auth_epv;
        RPC_DG_KEY_REFERENCE(key_info); /* for the scte */

        /* fill in the auth_info in the handle */
        auth_info = key_info->auth_info;
        h->c.c.auth_info = auth_info;
        RPC_DG_AUTH_REFERENCE(auth_info); /* for the handle */
    }
    auth_epv = scall->c.auth_epv;

    /*
     * If the interface isn't valid, send a rejection.
     */
    rpc_object_inq_type(&scall->c.call_object, &type, &st);
    if (! (st == rpc_s_ok || st == rpc_s_object_not_found))
    {
        RPC_DBG_GPRINTF((
            "(rpc__dg_execute_call) rpc_object_inq_type failed, st=0x%x [%s]\n", 
            st, rpc__dg_act_seq_string(hdrp)));
        reject_st = st;
        goto AFTER_CALL_TO_STUB;
    }

    ihint = hdrp->ihint;
    rpc__if_lookup2 (&hdrp->if_id, hdrp->if_vers, &type,
                     &ihint, &ifspec, &ss_epv, &mgr_epv,
                     &flags, &max_calls, &max_rpc_size,
                     &if_callback, &st);

    if (st != rpc_s_ok)
    {
        RPC_DBG_GPRINTF((
            "(rpc__dg_execute_call) rpc__if_lookup failed, st=0x%x [%s]\n", 
            st, rpc__dg_act_seq_string(hdrp)));
        reject_st = st;
        goto AFTER_CALL_TO_STUB;
    }

    /*
     * The interface is valid, update the call ihint so we tell the client.
     */

    scall->c.call_ihint = ihint;

    /*
     * Extract a copy of the opnum from the packet header, and check to see that 
     * it's appropriate for this interface.
     */

    opnum = hdrp->opnum;
    if (opnum >= ifspec->opcnt)
    {
        RPC_DBG_GPRINTF((
            "(rpc__dg_execute_call) Opnum (%u) out of range [%s]\n", 
            opnum, rpc__dg_act_seq_string(hdrp)));
        reject_st = rpc_s_op_rng_error;
        goto AFTER_CALL_TO_STUB;
    }

    /*
     * To guarantee at-most-once semantics for non-idempotent RPCs, we
     * must ensure that the call is filtered based on a WAY validated
     * sequence number.  If we don't have such a sequence number, then
     * call back to client to get one (the returned WAY validated seq 
     * must match this RPC's seq - i.e. it must be the RPC that the client
     * is currently performing).  Note that we may do a way_auth
     * callback even when we wouldn't otherwise do it because the
     * underlying authentication layers decided one was needed.
     * 
     * The analogous processing for non-idempotent callbacks (from a
     * server manager to the client originating the call, who needs to
     * validate the callback's seq) was previously taken care of in the
     * do_request() processing (a WAY validated logical scte high_seq
     * was already known).
     *
     * Note also that maybe calls with large-INs are tagged as
     * non-idempotent but do not need to be protected against re-runs.
     * (The architecture specifies that maybe calls can *not* have
     * at-most-once semantics, but the implementation finds it more 
     * convenient to use the non-idempotent code paths for handling
     * calls with large-INs.)  For this reason, avoid doing a WAY for
     * maybe calls (the client may not even be still running!).
     *
     * Release and reacquire the call lock while performing this
     * (slow path / lengthy) WAY and Auth processing.
     *
     * We perform the WAY RPC with general cancel delivery disabled.
     * The RPC prologue is suppose to be transparent and clients can
     * orphan the call if they get tired of waiting around.
     */
    
    if (! maybe &&
         (force_way_auth || key_info != NULL ||
         (! idem && ! scall->c.is_cbk))) 
    {
        if (!force_way_auth && RPC_DG_SCT_IS_WAY_VALIDATED(scall->scte))
        {
            /*
             * We want to make this check because it's better to be safe
             * than sorry regarding at-most-once semantics.  It's
             * conceivable that the connection became WAY validated *after*
             * this call had passed it's initial filtering (if nothing
             * else, it should protect us from other potential coding
             * errors :-)
             */
            if (scall->c.call_seq != scall->scte->high_seq)
            {
                RPC_DBG_PRINTF(rpc_e_dbg_general, 2, 
                    ("(execute_call) Old sequence, previous=%lu [%s]\n",
                    scall->scte->high_seq, rpc__dg_act_seq_string(hdrp)));
                goto END_OF_CALL;
            }
        }
        else
        {
            boolean high_seq_was_way_validated =
                (boolean)(scall->scte->high_seq_is_way_validated);

            /*
             * WAY validate the connection and ensure that this call
             * is the current call.  Unlock the scall while performing the
             * WAY validation.
             */
            rpc_dg_sct_elt_p_t  scte;

            RPC_DG_CALL_UNLOCK(&scall->c);

            /*
             * The WAY validation routine must be called with the connection
             * unlocked.  Due to locking hierarchy and the fact that we
             * unlocked the scall, we've opened up a window... check if
             * it's safe to continue.
             */
            RPC_LOCK(0);
            RPC_DG_CALL_LOCK(&scall->c);
            if (scall->c.state != rpc_e_dg_cs_recv)
            {
                RPC_UNLOCK(0);
                goto END_OF_CALL;
            }
            scte = scall->scte;
            RPC_DG_CALL_UNLOCK(&scall->c);

            rpc__dg_sct_way_validate(scte, force_way_auth, &st);

            RPC_UNLOCK(0);

            RPC_DG_CALL_LOCK(&scall->c);

            /*
             * Before continuing, we've "opened up" the call (due to
             * the unlock/lock) and we need to check if it is safe to
             * continue...
             */
            if (scall->c.state != rpc_e_dg_cs_recv)
                goto END_OF_CALL;

            if (st != rpc_s_ok)
            {
                reject_st = rpc_s_who_are_you_failed;
                goto AFTER_CALL_TO_STUB;
            }
            else
            {
                if (scall->c.call_seq != scall->scte->high_seq)
                {
                    RPC_DBG_PRINTF(rpc_e_dbg_general, 2, 
                        ("(rpc__dg_execute_call) Old sequence, previous=%lu [%s]\n",
                        scall->scte->high_seq, rpc__dg_act_seq_string(hdrp)));
                    goto END_OF_CALL;
                }
            }

            /*
             * If high_seq_was_way_validated, rpc__dg_call_receive_int()
             * has already verified the packet by calling
             * (*auth_epv->recv_ck)().
             * It's ok to call it again here except when using
             * pkt_privacy where the packet body is already decrypted.
             * For consistency, we don't verify the packet if it's
             * already done.
             */
            if (key_info != NULL && !force_way_auth
                && !high_seq_was_way_validated)
            {
                unsigned32 blocksize = auth_epv->blocksize;
                char *cksum;
                int raw_bodysize;

                /*
                 * This must be a single buffer fragment.
                 * The very first fragment!
                 */
                if (rqe->hdrp == NULL || rqe->frag_len != rqe->pkt_len)
                {
                    reject_st = rpc_s_who_are_you_failed;
                    goto AFTER_CALL_TO_STUB;
                }

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
                 * Now that we have obtained authentication
                 * credentials, go back and verify that cksum is
                 * entirely contained inside the packet, and the
                 * auth_type is what we expected.  This "shouldn't
                 * fail" unless someone's playing games with us.
                 */

                if (((RPC_C_DG_RAW_PKT_HDR_SIZE + raw_bodysize +
                    auth_epv->overhead) > rqe->frag_len) ||
                    (rqe->hdrp->auth_proto != auth_epv->auth_proto))
                {
                    st = nca_s_proto_error;
                }
                else 
                {
                    /*
                     * Adjust the packet buffer's pkt_len,
                     * i.e., excluding the auth trailer.
                     * Also adjust data_len in the iovector.
                     */
                    rqe->pkt_len = raw_bodysize + RPC_C_DG_RAW_PKT_HDR_SIZE;
                    iove.data_len = raw_bodysize;

                    cksum = rqe->pkt->body.args + raw_bodysize;
                    RPC_DBG_PRINTF(rpc_e_dbg_general, 4,
                        ("(rpc__dg_execute_call) calling recv_ck now\n"));
                    (*auth_epv->recv_ck) (key_info, rqe, cksum, &st);
                }
                if (st != rpc_s_ok) 
                {
                    RPC_DBG_PRINTF(rpc_e_dbg_general, 2,
                        ("(rpc__dg_execute_call) pkt didn't verify -- %lx\n", st));
                    reject_st = rpc_s_who_are_you_failed;
                    goto AFTER_CALL_TO_STUB;
                }
            }
            else if (key_info != NULL && force_way_auth)
            {
                /*
                 * Call rpc__dg_call_receive_int() again. This time,
                 * (*auth_epv->recv_ck)() is supposed to succeed.
                 */
                rpc__dg_call_receive_int(&scall->c, &iove, &st);
                force_way_auth = false;
                if (st == rpc_s_dg_need_way_auth) {
                    /*
                     * We still don't own the rqe...
                     */
                    force_way_auth = true;
                }
                if (st != rpc_s_ok)
                {
                    RPC_DBG_GPRINTF((
"(rpc__dg_execute_call) Receive failed st = 0x%x after forced WAY auth callback\n", st));
                    reject_st = rpc_s_who_are_you_failed;
                    goto AFTER_CALL_TO_STUB;
                }
                assert(rqe == RPC_DG_RECVQ_ELT_FROM_IOVECTOR_ELT(&iove));
            }
        }
    }

    assert(force_way_auth == false);

    /*
     * If we get here, we're accepting the call and we're gonna dispatch
     * to the server stub!  Setup the required args for the dispatch
     * (the iove was done above) and run call the server stub.
     */

    RPC_DG_HDR_INQ_DREP(&drep, hdrp);
 
    /*
     * The packet rationing code needs to know that we no longer need
     * to worry about  doing WAYs.
     */
    scall->c.rq.is_way_validated = true;

    /*
     * Unlock the call lock while in the stub.
     */
    RPC_DG_CALL_UNLOCK(&scall->c);

    /*
     * Note: the stubs are absolutely, positively required to free the
     * provided iove described buffer (assuming the len > 0), even if
     * the stub detects and returns an error condition.   Set the
     * "called_stub" flag to true so that we know we don't have to worry
     * about freeing the RQE ourselves.
     */
    called_stub = true;

    /*
     * As required by the packet rationing rules, if the I/O vector element
     * has no data, free it up now because the server stub doesn't bother
     * to free such elements.  Note that we needed the element until
     * now for the info that was in its packet header.
     */

    if (iove.data_len == 0 && iove.buff_dealloc != NULL)
        RPC_FREE_IOVE_BUFFER(&iove);
    
    switch (ifspec->stub_rtl_if_vers)
    {
        /*
         * If this is an old v0 or v1 stub runtime interface.  Do the
         * dirty work out of line.
         */
        case RPC_C_STUB_RTL_IF_VERS_NCS_1_0:
        case RPC_C_STUB_RTL_IF_VERS_NCS_1_5:
            if (rpc_g_dg_pre_v2_server_call_p == NULL) {
		/*
		 * rpc_m_pre_v2_ss
		 * "(%s) Can't handle pre-v2 server stubs"
		 */
		RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%s"),
		    rpc_svc_server_call,
		    svc_c_sev_fatal | svc_c_action_abort,
		    rpc_m_pre_v2_ss,
		    "rpc__dg_execute_call" ));
	    }

            prev_cancel_state = dcethread_enableinterrupt_throw(0);
            (*rpc_g_dg_pre_v2_server_call_p)(
                ifspec, 
                opnum,
                (handle_t) h, 
                (rpc_call_handle_t) scall, 
                &iove, 
                drep,        
                ss_epv,
                mgr_epv,
                &reject_st);
            dcethread_enableinterrupt_throw(prev_cancel_state);
            break;       

        /*
         * This is the v2 (new) stub runtime interface.
         */
        case RPC_C_STUB_RTL_IF_VERS_DCE_1_0:
            prev_cancel_state = dcethread_enableinterrupt_throw(0);
            (*(ss_epv[opnum]))(
                    (handle_t) h, 
                    (rpc_call_handle_t) scall, 
                    &iove, 
                    &drep,
                    &ndr_g_transfer_syntax,
                    mgr_epv, 
                    &reject_st);
            dcethread_enableinterrupt_throw(prev_cancel_state);
            break;

        /*
         * Unknown version
         */

        default:
            RPC_DBG_GPRINTF((
                "(rpc__dg_execute_call) Unknown rtl/if version. 0x%x\n", 
               ifspec->stub_rtl_if_vers));
            RPC_DG_CALL_LOCK(&scall->c);

            if (iove.buff_dealloc != NULL)
                RPC_FREE_IOVE_BUFFER(&iove);

            goto END_OF_CALL;
    }

    /*
     * While the stub may have returned due to call orphaning, this will
     * not typically be the case.  Even if it completed succesfully
     * we could become orphaned further down in this processing (e.g.
     * in xmitq_push).  Defer orphan checking and cleanup till we only
     * have to do it once; the extra work done if we are orphaned won't
     * kill us.
     */

    /*
     * Acquire the call lock since we need it for several pieces of
     * processing from here on in.
     *
     * Before continuing, we've "opened up" the call (due to the
     * unlock/lock) and we need to check if it is safe to continue...
     */

    RPC_DG_CALL_LOCK(&scall->c);
    if (scall->c.state != rpc_e_dg_cs_recv 
        && scall->c.state != rpc_e_dg_cs_xmit)
    {
        goto END_OF_CALL;
    }

    /*
     * Error cases detected before we get to calling the stub and that want
     * to send a "reject" re-enter here.
     */
AFTER_CALL_TO_STUB:

    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    /*
     * If this was a broadcast request and we're either rejecting the call
     * or the call faulted, just skip to the end.
     */

    if (broadcast && 
        (reject_st != rpc_s_ok || 
         RPC_DG_HDR_INQ_PTYPE(&scall->c.xq.hdr) == RPC_C_DG_PT_FAULT))
    {
        goto END_OF_CALL;
    }

    /*
     * The stub was obligated to call the iove's dealloc routine,
     * so we don't have to free that.  We don't need the recvq anymore.
     * In normal cases, the list will already be empty, so having this
     * in the fast path doesn't hurt and (in the error cases) it frees 
     * up resources while we potentially wait in xmitq_push() (or
     * awaiting a xqe for a reject or no [outs] response).
     */

    if (scall->c.rq.head != NULL) 
        rpc__dg_recvq_free(&scall->c.rq);

    /*
     * If a reject condition exists, prepare the reject response.
     * Otherwise, handle the case where the stub has no [outs] and it's
     * not a maybe call; we still need to generate a response pkt.
     *
     * We depend on both of these response queuing operations
     * to only queue the response and not send it since we've yet
     * setup the return cancel_pending status for the client.
     */

    if (reject_st != rpc_s_ok)
    {
        /*
         * If the reject path caused us to jump over the call into the
         * stub, we need to free the request RQE here.
         *
         * If we are forced to do WAY auth and havn't done it so, don't free
         * it because we don't own the rqe.
         */
    
        if (! called_stub && !force_way_auth && iove.buff_dealloc != NULL)
            RPC_FREE_IOVE_BUFFER(&iove);
    
        queue_mapped_reject(scall, reject_st);
    }
    else
    {
        if (scall->c.state == rpc_e_dg_cs_recv && !maybe) 
        {
            rpc_iovector_t  xmit_data;

            xmit_data.num_elt = 0;
            rpc__dg_call_transmit_int(&scall->c, &xmit_data, &st);
            /*
             * The transmit may fail because the call is already orphaned.
             * It may fail for some other reason as well.  In either case,
             * we're not gonna get a response to the client.  Just keep
             * falling through (other calls may fail as well) and clean up.
             */
        }
    }

    /*
     * At this point, we can stop accepting forwarded cancels.  Determine
     * the cancel pending disposition of the call and set the call's
     * xq cancel_pending flag accordingly so that the response (or at
     * least the last pkt of the response) gets sent with the proper
     * state.  This is the single point where the "send response"
     * path ensures that it has flushed any pending cancels from the
     * call executor thread; this includes cancels generated by
     * a received cancel-request or a cancel induced by orphan call 
     * processing.
     * 
     * We could have stopped accepting cancels as soon as the stub
     * returned, but we really wanted to wait till here before setting
     * up the return cancel_pending status.  After this, we shouldn't
     * screw around anymore with the xq (i.e. re-initing it).  There
     * should be a reject, fault or normal response queued up and
     * it should go out with the correct cancel_pending flag.
     * That is of course, unless that call has been orphaned, in which
     * case no further response of any kind will be sent to the client
     * (setting the cancel_pending flag will not affect the client;
     * which is a *requirement* under this condition).
     */

    if (rpc__cthread_cancel_caf(&scall->c.c))
    {
        RPC_DBG_PRINTF(rpc_e_dbg_cancel, 5, 
            ("(rpc__dg_execute_call) setting cancel_pending\n"));
        scall->c.xq.base_flags2 |= RPC_C_DG_PF2_CANCEL_PENDING;
    }
    
    /*
     * Assuming that the call isn't already orphaned, finally push
     * out the remainder of the response.  The push may fail
     * because orphaning occurs during the push or for some
     * other reason; just continue to cleanup processing. Indicate
     * whether or not the response was sent so we can determine
     * the appropriate call state when we're done.
     */

    if (scall->c.state != rpc_e_dg_cs_orphan)
    {
        rpc__dg_call_xmitq_push(&scall->c, &st);
        if (st == rpc_s_ok) 
            sent_response = true;
        else
            RPC_DBG_GPRINTF((
                "(rpc__dg_execute_call) xmitq_push returns 0x%x\n", st));
    }

    /*
     * Error cases that want to skip the reply-sending machinery re-enter here.
     */
END_OF_CALL:

    RPC_DG_CALL_LOCK_ASSERT(&scall->c);

    /*
     * End of the fast path.
     *
     * Any response has been sent (or at least all the pkts have been
     * sent once).  Perform final call wrap-up processing / state
     * transitioning.  In the event that we didn't take the send
     * response path, we still need to flush any pending cancels.
     * In the event that we took the send response path but the response
     * wasn't succesfully sent, we'll call the following twice but
     * that's ok.
     */

    if (! sent_response)
        (void) rpc__cthread_cancel_caf(&scall->c.c);

    /*
     * If the call is not "idempotent" we must defer complete end of
     * call processing until the client's ack is received.  (Note: "maybe"
     * and "broadcast" are tagged as "idempotent".)  For idempotent calls
     * with small outs, we can clean up right now (if the client never
     * gets the response, it can rerun the call).
     *
     * Idempotent calls with large outs are treated similarly to
     * non-idempotent calls.  We retain the outs until "acknowledged"
     * by the client or the retransmit logic gives up.  This is required
     * to prevent the undesireable situation of the client receiving
     * a "nocall" in response to a "ping" after the client has already
     * received some of the outs.
     *
     * If we didn't (seemingly) successfully send a response, skip the
     * final state (this covers orphan processing as well).  Furthermore,
     * if the call has been orphaned stay in that state.
     *
     * An orphaned call has already been disassociated from its SCTE
     * (ccall in the case of a cbk_scall) and there should be a maximum
     * of two references to the orphaned SCALL; the call executor's and
     * the timer thread.  The only actions required are to release any
     * remaining resources held by the call and release one reference
     * to the SCALL (the timer thread will eventually complete to job
     * of destroying the scall).
     */

    if ((! idem || RPC_DG_FLAG_IS_SET(scall->c.xq.base_flags, RPC_C_DG_PF_FRAG))
        && sent_response) 
    {
        RPC_DG_CALL_SET_STATE(&scall->c, rpc_e_dg_cs_final);
    }
    else
    {
        /*
         * It's really the end of the call, so we can free the xmitq.
         */
    
        if (scall->c.xq.head != NULL) 
            rpc__dg_xmitq_free(&scall->c.xq, &scall->c);
    
        /*
         * Typically, the call goes back to the idle state, ready to
         * handle the next call.  First, If this was a callback, update
         * the callback sequence number in the associated client callback
         * handle.
         * 
         * If the call was orphaned, we can't to do either of the above
         * (we just want to let the scall's timer complete the job of
         * destroying the scall).
         */

        if (scall->c.state != rpc_e_dg_cs_orphan)
        {
            if (scall->c.is_cbk)
            {
                scall->cbk_ccall->c.high_seq = scall->c.call_seq;
            }

            RPC_DG_CALL_SET_STATE(&scall->c, rpc_e_dg_cs_idle);
        }
    }

    /*
     * Give up the packet reservation for this call.
     */

    rpc__dg_pkt_cancel_reservation(&scall->c);

    if (scall->c.is_cbk && scall->cbk_ccall != NULL)
    {
        /*
         * Update the original ccall's high_rcv_frag_size and snd_frag_size.
         */

        scall->cbk_ccall->c.rq.high_rcv_frag_size =
            scall->c.rq.high_rcv_frag_size;
        scall->cbk_ccall->c.xq.snd_frag_size = scall->c.xq.snd_frag_size;
    }
    /*
     * We're now done with our scall lock/reference.
     */

    scall->has_call_executor_ref = false;
    RPC_DG_SCALL_RELEASE(&scall);
}
