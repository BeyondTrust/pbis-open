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
/*
 */
/*
**
**  NAME
**
**      cncall.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  The NCA Connection Protocol Service's Call Service.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <ndrglob.h>    /* NDR representation global declarations */
#include <ndrp.h>       /* NDR system dependent definitions */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnbind.h>     /* NCA connection binding service */
#include <cnsm.h>       /* NCA Connection generic state machine operations */
#include <cnclsm.h>     /* NCA Connection client call state machine */
#include <cnpkt.h>      /* NCA Connection protocol header */
#include <cnassoc.h>    /* NCA Connection association services */
#include <cnfbuf.h>     /* NCA Connection fragment buffer routines */
#include <cnxfer.h>     /* NCA Connection data transfer routines */
#include <comtwr.h>     /* Externals for Towers sub-component */
#include <comtwrref.h>  /* tower defs for other RPC components */
#include <dce/ep.h>     /* rpcd endpoint (ep) interface definitions */
#include <cncall.h>

/*
 * Macro to test for a cancel every so often for a cancel. The
 * num_pkts field of the call rep is cleared when the first frag is
 * sent. It is incremented on every subsequent transmitted or received
 * fragment.
 */
#define RPC_CN_CANCEL_CHECK_FREQ        8
#define RPC_CN_CHECK_FOR_CANCEL(call_r) \
{ \
    if ((call_r)->num_pkts & RPC_CN_CANCEL_CHECK_FREQ) \
    { \
        rpc__cn_call_check_for_cancel (call_r);\
    } \
}

/*
 * Macro to forward, if the first frag has been sent, and queued
 * cancels. 
 */
#define RPC_CN_FORWARD_QUEUED_CANCELS(call_r, st) \
{ \
    if ((call_r)->u.client.cancel.local_count) \
    { \
        rpc__cn_call_forward_cancel (call_r, st); \
    } \
}    

/*
 * Macro to check for a pending cancel and forward, if the first frag
 * has been sent, that cancel and any queued cancels.
 */
#define RPC_CN_CHK_AND_FWD_CANCELS(call_r, st) \
{ \
    RPC_CN_CHECK_FOR_CANCEL (call_r); \
    RPC_CN_FORWARD_QUEUED_CANCELS (call_r, st); \
}
    
INTERNAL unsigned32 rpc__cn_call_cvt_from_nca_st (
        unsigned32      /*a_st*/
    );

INTERNAL unsigned32 rpc__cn_call_cvt_to_nca_st (
        unsigned32      /*l_st*/
    );

INTERNAL void rpc__cn_call_check_for_cancel (
        rpc_cn_call_rep_p_t     /*call_rep*/
    );

INTERNAL void rpc__cn_call_forward_cancel (
        rpc_cn_call_rep_p_t     /*call_rep*/,
        unsigned32              * /*status*/
    );

INTERNAL void rpc__cn_call_binding_serialize (
        rpc_binding_rep_p_t     /*binding_r*/,
        rpc_clock_t             /*cancel_timeout*/,
        unsigned32              * /*cancel_cnt*/,
        unsigned32              * /*st*/
    );

INTERNAL boolean rpc__cn_call_cancel_timer (
        rpc_cn_call_rep_p_t     /*call_r*/
    );

/***********************************************************************/

/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_start
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This is the NCA connection oriented protocol specific routine
**  which begins a remote procedure call.  It returns the information
**  needed to marshal input arguments.  This routine is intended 
**  for use by the client stub only.
**
**  INPUTS:
**
**      binding_r       The binding rep containing the location of
**                      the server.
**      call_options    The options pertaining to this RPC.
**      ifspec_r        The interface specification rep data structure
**                      describing the interface to which the RPC belongs.
**      opnum           The operation number of the RPC in the
**                      interface.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      transfer_syntax The negotiated transfer syntax for this RPC.
**
**      st              The return status of this routine.
**          rpc_s_ok                    The call was successful.
**          rpc_s_invalid_call_opt      An invalid call option was specified.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      call_rep        The call rep.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_call_rep_t *rpc__cn_call_start 
(
  rpc_binding_rep_p_t     binding_r,
  unsigned32              call_options,
  rpc_if_rep_p_t          ifspec_r,
  unsigned32              opnum,
  rpc_transfer_syntax_t   *transfer_syntax,
  unsigned32              *st
)
{
    rpc_cn_call_rep_t       *call_rep;
    rpc_iovector_elt_p_t    iov_p;
    rpc_cn_packet_p_t       header_p;
    unsigned32              cancel_count = 0;
    rpc_clock_t             cancel_timeout = 0;
    unsigned32              temp_st;
    dcethread*              current_thread_id;
    boolean                 resolving_thread;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_start);
    RPC_LOG_CN_CALL_START_NTR;
    CODING_ERROR (st);

    /*
     * If this call requires security, verify that the caller's
     * credentials are valid right now. If they aren't, try to refresh
     * them. If they can't be refreshed return an error. We do this
     * now while there are no locks taken out. The CRED_REFRESH
     * operation may make an RPC to the security server which will
     * require the RPC locks to be taken out. This would result in a
     * deadlock if we had any RPC locks taken out now.
     *
     * It is the responsibility of the authentication protocol to
     * serialize access to the auth info structure. For example the
     * Kerberos auth protocol uses a lock per auth_info structure to
     * protect it (the krb_info_lock).
     *
     * XXX is there a deadlock here? See Transarc Defect 17575
     * http://vcs.es.net/DCCC/DCEWG/DCE-1.1_Patch16.txt
     */
    if (RPC_CN_AUTH_REQUIRED (binding_r->auth_info))
    {
        /*
         * The binding has an auth info structure hanging off it,
         * which indicates this call is to be authenticated.
         */
        RPC_CN_AUTH_CRED_REFRESH (binding_r->auth_info,
                                  st);
        if (*st != rpc_s_ok)
        {
            return (NULL);
        }
    }

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    /*
     * The broadcast call option and callbacks are not supported by the
     * connection oriented protocol services.
     * We detect callbacks by checking the binding to see if it is
     * that of a server.
     */
    if (call_options & rpc_c_call_brdcst)
    {
        *st = rpc_s_invalid_call_opt;
        RPC_CN_UNLOCK ();
        return (NULL);
    }

    if (binding_r->is_server)
    {
        *st = rpc_s_wrong_kind_of_binding;
        RPC_CN_UNLOCK ();
        return (NULL);
    }

    /*
     * Get the thread's cancel timeout value (so we don't block forever).
     */
    RPC_GET_CANCEL_TIMEOUT (cancel_timeout, st);
    if (*st != rpc_s_ok)
    {
        RPC_CN_UNLOCK ();
        return (NULL);
    }

    /*
     * Check to see if we're currently resolving a binding.
     */
    if (((rpc_cn_binding_rep_t *)binding_r)->being_resolved)
    {
        current_thread_id = dcethread_self ();
        resolving_thread = dcethread_equal 
            (((rpc_cn_binding_rep_t *)binding_r)->resolving_thread_id,
             current_thread_id);
        if (!resolving_thread)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->none waiting on binding resolution\n"));
            /*
             * Serialize calls on this binding handle until the handle 
             * has an endpoint.
             */
            RPC_BINDING_CALL_START ((rpc_binding_rep_t *) binding_r);

            rpc__cn_call_binding_serialize (binding_r,
                                            cancel_timeout,
                                            &cancel_count,
                                            st);
            RPC_BINDING_CALL_END (binding_r);
            if (*st != rpc_s_ok)
            {
                /*
                * The cancel timeout probably expired.
                */
                RPC_CN_UNLOCK ();
                return (NULL);
            }
        }
    }
    else
    {
        /*
         * Check to see if we have a fully bound handle, i.e.,
         * the endpoint information is present.
         * If not, call rpc_ep_resolve_binding.
         */
        if (!binding_r->addr_has_endpoint)
        {
            /*
             * The binding handle is partially bound. Prevent other
             * threads from using it in a call until it is fully bound.
             */
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->none partially bound binding handle\n"));

            RPC_BINDING_CALL_START ((rpc_binding_rep_t *) binding_r);
            ((rpc_cn_binding_rep_t *)binding_r)->being_resolved = true;
            ((rpc_cn_binding_rep_t *)binding_r)->resolving_thread_id = dcethread_self ();

            RPC_CN_UNLOCK ();
            rpc_ep_resolve_binding ((rpc_binding_handle_t) binding_r, 
                                    (rpc_if_handle_t) ifspec_r, st);
            RPC_CN_LOCK ();
            ((rpc_cn_binding_rep_t *)binding_r)->being_resolved = false;

            /*
             * Let other threads use the binding handle now.
             */
            RPC_BINDING_CALL_END (binding_r);
            if (binding_r->calls_in_progress)
            {
                RPC_BINDING_COND_BROADCAST (0);
            }

            /*
             * If the attempt to resolve the binding handle was 
             * unsuccessful return.
             */
            if (*st != rpc_s_ok) 
            {
                RPC_CN_UNLOCK ();
                return (NULL);
            }
        }
    }

    /*
     * At this point, we have a fully-bound binding.
     * We can now allocate a call_rep.
     */
    call_rep = (rpc_cn_call_rep_p_t) 
        rpc__list_element_alloc (&rpc_g_cn_call_lookaside_list, true);
    if (call_rep == NULL)
    {
        RPC_CN_UNLOCK ();
        *st = rpc_s_no_memory;
        return (NULL);
    }

    /*
     * Mark it as a client call rep.
     */
    call_rep->common.is_server = false;
    call_rep->prot_tlr = NULL;

    /*
     * Clear out the io vectors
     */
    {
        int i;
        for( i=1; i<RPC_C_MAX_IOVEC_LEN; i++ ) {
            call_rep->buffered_output.iov.elt[i].buff_addr = NULL;
            call_rep->buffered_output.iov.elt[i].buff_dealloc = NULL;
        }
    }

    /*
     * Start off in the initial state.
     */
    rpc__cn_sm_init (rpc_g_cn_client_call_sm,
                     rpc_g_cn_client_call_action_tbl,
                     &(call_rep->call_state),
		     rpc_c_cn_cl_call);
    call_rep->call_state.cur_state = RPC_C_CLIENT_CALL_INIT;

    /*
     * Init security info
     */
    call_rep->sec = NULL;

    /*
     * Init the association in case allocating it later in this function
     * fails.
     */
    call_rep->assoc = NULL;

    /*
     * Init some booleans.
     */
    call_rep->cn_call_status = rpc_s_ok;
    call_rep->last_frag_received = false;
    call_rep->call_executed = false;

    /*
     * Initialize some cancel state information. 
     */
    call_rep->u.client.cancel.server_is_accepting = false;
    call_rep->u.client.cancel.server_had_pending = false;
    call_rep->u.client.cancel.server_count = 0;

    /*
     * Initialize the fault fragbuf pointer to NULL.
     */
    call_rep->u.client.fault_data = NULL;

    /*
     * Include any cancels detected while serializing access to the
     * binding handle. These will be forwarded when the first
     * request fragment is sent. The absolute cancel timeout time
     * left for this thread was gotten by RPC_GET_CANCEL_TIMEOUT.
     */
    call_rep->u.client.cancel.local_count = cancel_count;
    call_rep->u.client.cancel.timeout_time = cancel_timeout;
    call_rep->u.client.cancel.timer_running = false;
    if (cancel_count)
    {
        /*
         * Start a cancel timer. We won't bother checking the status
         * here because there is no timer running yet and no
         * association set up yet.
         */
        rpc__cn_call_start_cancel_timer (call_rep, st);
    }

    /*
     * Save the binding rep in the call rep for use in an action routine.
     */
    call_rep->binding_rep = binding_r;

    /*
     * Store the address of the fragment buffer in the
     * call rep.
     */
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);

    RPC_CN_PKT_PTYPE (header_p) = RPC_C_CN_PKT_REQUEST;
    RPC_CN_PKT_FLAGS (header_p) = RPC_C_CN_FLAGS_FIRST_FRAG;
    if (call_options & rpc_c_call_maybe)
    {
        RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_MAYBE;
    }
    RPC_CN_PKT_ALERT_COUNT(header_p) = 0;
    RPC_CN_PKT_FRAG_LEN (header_p) = 0;
    RPC_CN_PKT_CALL_ID (header_p) = rpc_g_cn_call_id++;
    RPC_CN_PKT_OPNUM (header_p) = opnum;

    /*
     * Initialize the iovector in the call_rep to contain only
     * one initial element, pointing to the protocol header.
     * Also, update pointers to show that we can copy data into
     * the stub data area.
     */
    RPC_CN_CREP_IOVLEN (call_rep) = 1;
    RPC_CN_CREP_CUR_IOV_INDX (call_rep) = 0;
    iov_p = &(RPC_CN_CREP_IOV (call_rep)[0]);

    /*
     * The start of stub data will depend upon whether the object 
     * UUID field is present.
     */
    if (memcmp (&binding_r->obj, &uuid_g_nil_uuid, sizeof (dce_uuid_t)) != 0)
    {
        /*
         * A non-nil object UUID is present.
         */
        RPC_CN_PKT_OBJECT(header_p) = binding_r->obj;
        RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_OBJECT_UUID;
        RPC_CN_CREP_SIZEOF_HDR (call_rep) = RPC_CN_PKT_SIZEOF_RQST_HDR_W_OBJ;
        RPC_CN_CREP_FREE_BYTES (call_rep) = 
            RPC_C_CN_SMALL_FRAG_SIZE - RPC_CN_PKT_SIZEOF_RQST_HDR_W_OBJ;
        RPC_CN_CREP_ACC_BYTCNT (call_rep) = RPC_CN_PKT_SIZEOF_RQST_HDR_W_OBJ;
        RPC_CN_CREP_FREE_BYTE_PTR(call_rep) = 
        RPC_CN_PKT_RQST_STUB_DATA_W_OBJ (header_p);
        iov_p->data_len = RPC_CN_PKT_SIZEOF_RQST_HDR_W_OBJ;
    }
    else
    {
        /*
         * There is no object UUID (it is nil) in the binding rep.
         */
        RPC_CN_CREP_SIZEOF_HDR (call_rep) = RPC_CN_PKT_SIZEOF_RQST_HDR_NO_OBJ;
        RPC_CN_CREP_FREE_BYTES (call_rep) = 
            RPC_C_CN_SMALL_FRAG_SIZE - RPC_CN_PKT_SIZEOF_RQST_HDR_NO_OBJ;
        RPC_CN_CREP_ACC_BYTCNT (call_rep) = RPC_CN_PKT_SIZEOF_RQST_HDR_NO_OBJ;
        RPC_CN_CREP_FREE_BYTE_PTR(call_rep) =
        RPC_CN_PKT_RQST_STUB_DATA_NO_OBJ (header_p);
        iov_p->data_len = RPC_CN_PKT_SIZEOF_RQST_HDR_NO_OBJ;
    }

    /*
     * Evaluate the RPC_REQUEST event.
     * If we return successfully, an association should be allocated 
     * and we should be in the call_pending state.
     */

    RPC_CN_CALL_EVAL_EVENT (RPC_C_CALL_START_CALL, 
                            ifspec_r,
                            call_rep, 
                            *st);

    if (*st == rpc_s_ok)
    {
        /*
         * Use the presentation context id in the call rep. This was
         * placed there by the rpc__cn_assoc_alloc routine.
         */
        RPC_CN_PKT_PRES_CONT_ID (header_p) = call_rep->context_id;
        
        /*
         * Use the negotiated minor version number from the association.
         */
        RPC_CN_PKT_VERS_MINOR (header_p) = call_rep->assoc->assoc_vers_minor;

        /*
         * If security is requested for this call an auth trailer will
         * be formatted here and included with every packet sent.
         */
        if (call_rep->sec != NULL)
        {
            rpc_cn_auth_tlr_t       *auth_tlr;
            unsigned32              auth_value_len;
            
            /*
             * Allocate a small fragbuf to contain the authentication trailer.
             */
            RPC_CN_FRAGBUF_ALLOC (call_rep->prot_tlr, RPC_C_CN_SMALL_FRAG_SIZE, st);
            call_rep->prot_tlr->fragbuf_dealloc = NULL;
            auth_value_len = RPC_C_CN_SMALL_FRAG_SIZE;
            auth_tlr = (rpc_cn_auth_tlr_t *)call_rep->prot_tlr->data_p;
            auth_tlr->auth_type = RPC_CN_AUTH_CVT_ID_API_TO_WIRE (call_rep->sec->sec_info->authn_protocol, st);
            auth_tlr->auth_level = call_rep->sec->sec_info->authn_level;
            auth_tlr->key_id = call_rep->sec->sec_key_id;
            auth_tlr->stub_pad_length = 0;
            auth_tlr->reserved = 0;
            RPC_CN_AUTH_PRE_CALL (RPC_CN_ASSOC_SECURITY (call_rep->assoc),
                                  call_rep->sec,
                                  (pointer_t) auth_tlr->auth_value,
                                  &auth_value_len,
                                  st);
            RPC_CN_CREP_ADJ_IOV_FOR_TLR (call_rep, header_p, auth_value_len);
        }
        else
        {
            RPC_CN_PKT_AUTH_LEN (header_p) = 0;
        }
        
        /*
         * Return the negotiated transfer syntax placed in the call
         * rep by rpc__cn_assoc_alloc.
         */
        *transfer_syntax = call_rep->transfer_syntax;
    }
    else
    {
        /*
         * We had a problem. Clean-up using call-end.
         */
        RPC_CN_UNLOCK ();
        rpc__cn_call_end ((rpc_call_rep_p_t *) &call_rep, &temp_st);
        RPC_CN_LOCK ();
    }

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();
    RPC_LOG_CN_CALL_START_XIT;
    return ((rpc_call_rep_p_t) call_rep);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_transmit
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This is the NCA connection oriented protocol specific routine
**  to transmit a vector or marshalled arguments to the remote
**  thread.  It uses the call rep as the identifier of the RPC
**  being performed.  This routine is intended for use by the
**  client or server stub only.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**      call_args       A vector of buffers containing marshaled RPC
**                      arguments.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**          rpc_s_ok        The call was successful.
**          rpc_s_cancel_timeout
**                          After forwarding a local alert, the
**                          client timed out waiting for the
**                          call to complete.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_transmit 
(
  rpc_call_rep_p_t        call_r,
  rpc_iovector_p_t        call_args,
  unsigned32              *st
)
{
    rpc_cn_call_rep_t       *call_rep;
    rpc_iovector_elt_p_t    iov_elt_p;
    unsigned32              i;
    rpc_cn_fragbuf_p_t      frag_buf;
    rpc_cn_packet_p_t       header_p;
    unsigned32              fault_code;
    boolean                 valid_fragbuf;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_transmit);
    RPC_LOG_CN_CALL_TRANSMIT_NTR;
    CODING_ERROR (st);

    call_rep = (rpc_cn_call_rep_p_t) call_r;

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call transmit...\n",
                     call_rep));

    if (RPC_DBG2 (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT))
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT,
                        ("PACKET: call transmit args.num_elt->%d\n", call_args->num_elt));
        for (i = 0; i < call_args->num_elt; i++)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT,
                            ("        elt[%d]: elt.flags->%x args.buff_len->%d args.data_len->%d\n", 
                             i, 
                             call_args->elt[i].flags,
                             call_args->elt[i].buff_len, 
                             call_args->elt[i].data_len));
        }
    }

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    /*
     * The event call_send corresponds to transmit_req on
     * the client side and rpc_resp on the server side.
     *
     * If the call has been orphaned, return rpc_s_call_orphaned
     * and deallocate any input data.
     */
    if (call_rep->cn_call_status == rpc_s_call_orphaned)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: call_rep->%x call orphaned...\n",
                         call_rep));
        
        iov_elt_p = call_args->elt;
        for (i = 1; i <= call_args->num_elt; i++, iov_elt_p++)
        {
            if (iov_elt_p->buff_dealloc != NULL)
            {
                (iov_elt_p->buff_dealloc) (iov_elt_p->buff_addr);
            }
        }
        *st = rpc_s_call_orphaned;
    }
    else
    {
        RPC_CN_CALL_EVAL_EVENT (RPC_C_CALL_SEND, 
                                call_args,
                                call_rep, 
                                *st);
        if (((call_rep->call_state.cur_state == RPC_C_CLIENT_CALL_CALL_FAILED)
	    || (call_rep->call_state.cur_state == RPC_C_CLIENT_CALL_CFDNE))
	    && ((call_rep->call_state.cur_event == RPC_C_CALL_SEND)
		|| (call_rep->call_state.cur_event == RPC_C_CALL_TRANSMIT_REQ)
		|| (call_rep->call_state.cur_event == RPC_C_CALL_LAST_TRANSMIT_REQ)))
	{
	    valid_fragbuf = false;
	    while (!valid_fragbuf)
	    {
		rpc__cn_assoc_receive_frag(call_rep->assoc,
					   &frag_buf,
					   st);

		if (*st != rpc_s_ok)
		{
		    RPC_CN_UNLOCK ();
		    return;
		}
		if (frag_buf->data_p != NULL)
		{
		    valid_fragbuf = true;
		}
	    }
	    /*
	     * Now adjust data_p to point to just the stub data.
	     * Note that data_size has already been adjusted by
	     * raise_fault_action_rtn
	     */
	    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);
	    if (RPC_CN_PKT_PTYPE(header_p) == RPC_C_CN_PKT_FAULT)
	    {

		frag_buf->data_p = (pointer_t)
				   (RPC_CN_PKT_FAULT_STUB_DATA(header_p));

	 	/* 
		 * We had conservatively assumed that the call has executed
		 * as soon as the call started. We can now update that
		 * information.
		 */

		if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_DID_NOT_EXECUTE)
		{
		    call_rep->call_executed = false;
		}

		/*
		 * This can be either a call_reject or a call_fault.
		 */

		fault_code = RPC_CN_PKT_STATUS(header_p);
		RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
				("CN: call_rep->%x fault packet received st=%x\n",
				  call_rep, fault_code));
		if (fault_code)
		{
		    /*
		     * Non-zero fault code implies a call_reject.
		     */

		    /*
		     * Deallocate the fragment buffer since we won't be returning
		     * it to the client stub.
		     */

		    (* frag_buf->fragbuf_dealloc) (frag_buf);
		    /*
		     * We translate the on-wire fault code into a local status
		     * and return it.
		     */
		    *st = rpc__cn_call_cvt_from_nca_st(fault_code);

		    /*
		     * Release the CN global mutex to allow other threads to run
		     */

		    RPC_CN_UNLOCK ();
		    return;
		}
		else
		{
		    /*
		     * On a fault, we store the address of the fragment buffer
		     * in the call_rep for later retrieval via
		     * rpc__receive_fault.
		     */

		    call_rep->u.client.fault_data = frag_buf;
		    *st = rpc_s_call_faulted;

		    /*
		     * Release the CN global mutex to allow other threads to
		     * run.
		     */

		    RPC_CN_UNLOCK ();
		    return;
		}
	    }
	}

        /*
         * Check for any pending cancels just in case we're in a
         * long marshalling loop and not calling any cancellable
         * operations. Also, forward any cancels which may have been
         * queued already before we were ready to forward them.
         */
        if (RPC_CALL_IS_CLIENT (call_r))
        {
            RPC_CN_CHK_AND_FWD_CANCELS (call_rep, st);
        }
    }

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();
    RPC_LOG_CN_CALL_TRANSMIT_XIT;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_transceive
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This is the NCA connection oriented protocol specific routine
**  to transmit a vector of marshalled arguments to the remote
**  thread.  It uses the call rep as the identifier of the RPC
**  being performed.  Block until the first buffer of marshalled
**  output arguments has been received.  This routine is intended 
**  for use by the client stub only.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**      in_call_args    A vector of buffers containing marshaled RPC
**                      input arguments.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      out_call_args   A buffer containing marshaled RPC output
**                      arguments.
**      remote_ndr_format The Network Data Representation of the
**                      server machine.
**      st              The return status of this routine.
**          rpc_s_ok        The call was successful.
**          rpc_s_call_faulted
**                          A fault was received.
**          rpc_s_cancel_timeout
**                          After forwarding a local alert, the
**                          client timed out waiting for the
**                          call to complete.
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_transceive 
(
  rpc_call_rep_p_t        call_r,
  rpc_iovector_p_t        in_call_args,
  rpc_iovector_elt_t      *out_call_args,
  ndr_format_t            *remote_ndr_format,
  unsigned32              *st
)
{
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_fragbuf_p_t      frag_buf;
    rpc_cn_packet_p_t       header_p;
    rpc_iovector_elt_p_t    iov_elt_p;
    unsigned32              fault_code;
    unsigned32              i;
    boolean                 valid_fragbuf;
    unsigned32              temp_status;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_transceive);
    RPC_LOG_CN_CALL_TRANSCEIVE_NTR;
    CODING_ERROR (st);

    call_rep = (rpc_cn_call_rep_p_t) call_r;

    out_call_args->buff_dealloc = NULL;
    out_call_args->data_addr = (byte_p_t) NULL;
    out_call_args->data_len = 0;

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call transceive...\n",
                     call_rep));

    if (RPC_DBG2 (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT))
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT,
                        ("PACKET: call transceive in_args.num_elt->%d\n", in_call_args->num_elt));
        for (i = 0; i < in_call_args->num_elt; i++)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT,
                            ("        elt[%d]: elt.flags->%x in_args.buff_len->%d in_args.data_len->%d\n", 
                             i, 
                             in_call_args->elt[i].flags,
                             in_call_args->elt[i].buff_len, 
                             in_call_args->elt[i].data_len));
        }
    }

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    rpc_g_cn_mgmt.calls_sent++;

    /*
     * If the call has been orphaned, return rpc_s_call_orphaned
     * and deallocate any input data.
     */
    if (call_rep->cn_call_status == rpc_s_call_orphaned)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: call_rep->%x call orphaned...\n",
                         call_rep));
        
        out_call_args->buff_dealloc = NULL;
        out_call_args->data_addr = (byte_p_t) NULL;
        out_call_args->data_len = 0;

        /*
         * Free the input args.
         */
        iov_elt_p = in_call_args->elt;
        for (i = 1; i <= in_call_args->num_elt; i++, iov_elt_p++)
        {
            if (iov_elt_p->buff_dealloc != NULL)
            {
                (iov_elt_p->buff_dealloc) (iov_elt_p->buff_addr);
            }
        }
        *st = rpc_s_call_orphaned;

        /*
         * Release the CN global mutex to allow other threads to
         * run.
         */
        RPC_CN_UNLOCK ();
        return;
    }

    /*
     * Transmit arguments to remote thread.
     * This is the last send fragment.
     * Note that we might get NULL data.
     */
    RPC_CN_CALL_EVAL_EVENT (RPC_C_CALL_LAST_TRANSMIT_REQ,
                           in_call_args, 
                           call_rep, 
                           temp_status);
    if (temp_status != rpc_s_ok)
    {
	call_rep->cn_call_status = temp_status;
    }

    if (call_rep->cn_call_status == rpc_s_ok)
    {
	/*
	 * Check for any pending cancels just in case we're in a
	 * long marshalling loop and not calling any cancellable
	 * operations. Also, forward any cancels which may have been
	 * queued already before we were ready to forward them.
	 */
	RPC_CN_CHK_AND_FWD_CANCELS (call_rep, &call_rep->cn_call_status);

        /*
         * Do a receive if the call does not have maybe semantics.
         */
        header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);
        if ((RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_MAYBE) == 0)
        {
            /*
             * If we have already received the last fragment,
             * return a null iovector element.
             */
            if (call_rep->last_frag_received)
            {
                out_call_args->buff_dealloc = NULL;
                out_call_args->data_addr = (byte_p_t) NULL;
                out_call_args->data_len = 0;
            }
            else
            {
                /*
                 * Receive a packet from the association.
                 * (The receive routine allocates the fragment buffer.)
                 * Make sure the fragbuf is not left over from an
                 * orphaned presentation negotiation.
                 */
                valid_fragbuf = false;
                while (!valid_fragbuf)
                {
                    rpc__cn_assoc_receive_frag (call_rep->assoc, 
                                                &frag_buf, 
                                                st);
                    
                    if (*st != rpc_s_ok)
                    {
                        out_call_args->buff_dealloc = NULL;
                        out_call_args->data_addr = (byte_p_t) NULL;
                        out_call_args->data_len = 0;
                        RPC_CN_UNLOCK ();
                        return;
                    }
                    if (frag_buf->data_p != NULL)
                    {
                        valid_fragbuf = true;
                    }
                }

                /*
                 * Set call_rep->last_frag_received if this is the last fragment.
                 */
                header_p = (rpc_cn_packet_p_t) frag_buf->data_p;
                if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_LAST_FRAG)
                {
                    call_rep->last_frag_received = true;
                }

                /*
                 * Copy the drep field from the association
                 * (which is in the 4 byte wire format)
                 * to the output remote_ndr_format arg.
                 */
                *remote_ndr_format = RPC_CN_ASSOC_NDR_FORMAT (call_rep->assoc);

                /*
                 * Now adjust data_p to point to just the stub data.
                 * Note that data_size has already been adjusted.
                 */
                if (RPC_CN_PKT_PTYPE(header_p) == RPC_C_CN_PKT_RESPONSE)
                {
                    frag_buf->data_p = (pointer_t)
                                      (RPC_CN_PKT_RESP_STUB_DATA(header_p));
                }
                else if (RPC_CN_PKT_PTYPE(header_p) == RPC_C_CN_PKT_FAULT)
                {
                    frag_buf->data_p = (pointer_t)
                        RPC_CN_PKT_FAULT_STUB_DATA (header_p);
                    /*
                     * We had conservatively assumed that the call
                     * has executed as soon as the call started.
                     * We can now update that information.
                     */
                    if (RPC_CN_PKT_FLAGS (header_p) & 
                        RPC_C_CN_FLAGS_DID_NOT_EXECUTE)
                    {
                        call_rep->call_executed = false;
                    }

                    /*
                     * This can be either a call_reject or a call_fault.
                     */
                    fault_code = RPC_CN_PKT_STATUS(header_p);
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                    ("CN: call_rep->%x fault packet received st = %x\n",
                                     call_rep,
                                     fault_code));
                    if (fault_code)
                    {
                        /*
                         * Non-zero fault code implies a call_reject.
                         */

                        /*
                         * Deallocate the fragment buffer since we
                         * won't be returning it to the client stub.
                         */
                        (* frag_buf->fragbuf_dealloc) (frag_buf);
                        
                        /*
                         * We translate the on-wire fault code into
                         * a local status code & return it.
                         */
                        *st = rpc__cn_call_cvt_from_nca_st (fault_code);

                        /*
                         * Release the CN global mutex to allow other threads to
                         * run.
                         */
                        RPC_CN_UNLOCK ();
                        return;
                    }
                    else
                    {
                        /* 
                         * On a fault, we store the address of the fragment 
                         * buffer in the call_rep for later retrieval via 
                         * rpc__receive_fault.
                         */
                        call_rep->u.client.fault_data = frag_buf;
                        *st = rpc_s_call_faulted;

                        /*
                         * Release the CN global mutex to allow other threads to
                         * run.
                         */
                        RPC_CN_UNLOCK ();
                        return;
                    }
                }
                else
                {
                    /*
                     * This is a protocol error, just return the whole
                     * packet.
                     */
                    call_rep->cn_call_status = rpc_s_protocol_error;
                }


                if (frag_buf->data_size > 0)
                {
                    /*
                     * Set up the iovector element to point to the received data.
                     */
                    out_call_args->buff_dealloc =
                        (rpc_buff_dealloc_fn_t)frag_buf->fragbuf_dealloc; 
                    out_call_args->buff_addr = (byte_p_t) frag_buf;
                    out_call_args->buff_len = frag_buf->max_data_size;
                    out_call_args->data_addr = (byte_p_t) frag_buf->data_p;
                    out_call_args->data_len = frag_buf->data_size;

                }
                else
                {
                    /*
                     * No stub data.
                     */
                    out_call_args->data_addr = (byte_p_t) NULL;
                    out_call_args->data_len = 0;

                    /*
                     * We also deallocate the fragbuf now.
                     */
                    (* frag_buf->fragbuf_dealloc) (frag_buf);
                }
            }
        }
    }
    *st = call_rep->cn_call_status;

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();

    RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT,
        ("PACKET: call transceive out_elt.flags->%x out_elt.buff_len->%d out_elt.data_len->%d\n", 
        out_call_args->flags,
        out_call_args->buff_len, 
        out_call_args->data_len));
    RPC_LOG_CN_CALL_TRANSCEIVE_XIT;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_receive
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  Return a buffer of marshalled arguments from the remote
**  thread.  This routine is intended for use by the client
**  or server stub only.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      call_args       A vector of buffers containing marshaled RPC
**                      arguments.
**      st              The return status of this routine.
**          rpc_s_ok        The call was successful.
**          rpc_s_call_orphaned
**          rpc_s_call_faulted
**          rpc_s_protocol_error
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_receive 
(
  rpc_call_rep_p_t        call_r,
  rpc_iovector_elt_p_t    call_args,
  unsigned32              *st
)
{
    rpc_cn_call_rep_t       *call_rep;
    rpc_cn_packet_p_t       header_p;
    rpc_cn_fragbuf_p_t      frag_buf;
    unsigned32              fault_code;
    boolean                 valid_fragbuf;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_receive);
    RPC_LOG_CN_CALL_RECEIVE_NTR;
    CODING_ERROR (st);

    call_rep = (rpc_cn_call_rep_p_t) call_r;

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call receive...\n",
                     call_rep));

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    /*
     * If the call has been orphaned, return rpc_s_call_orphaned.
     */
    if (call_rep->cn_call_status == rpc_s_call_orphaned)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: call_rep->%x call orphaned...\n",
                         call_rep));
        
        call_args->buff_dealloc = NULL;
        call_args->data_addr = (byte_p_t) NULL;
        call_args->data_len = 0;
        *st = rpc_s_call_orphaned;

        /*
         * Release the CN global mutex to allow other threads to
         * run.
         */
        RPC_CN_UNLOCK ();
        return;
    }

    /*
     * If we have already received the last fragment,
     * return a null iovector element.
     */
    if (call_rep->last_frag_received)
    {
        call_args->buff_dealloc = NULL;
        call_args->data_addr = (byte_p_t) NULL;
        call_args->data_len = 0;
        *st = rpc_s_ok;

        /*
         * Release the CN global mutex to allow other threads to
         * run.
         */
        RPC_CN_UNLOCK ();
        RPC_LOG_CN_CALL_RECEIVE_XIT;
        return;
    }

    /*
     * Receive a packet from the association.
     * (The receive routine allocates the fragment buffer.)
     * Make sure the fragbuf is not left over from an
     * orphaned presentation negotiation.
     */
    valid_fragbuf = false;
    while (!valid_fragbuf)
    {
        rpc__cn_assoc_receive_frag (call_rep->assoc, 
                                    &frag_buf, 
                                    st);
        if (*st != rpc_s_ok)
        {
            call_args->buff_dealloc = NULL;
            call_args->data_addr = (byte_p_t) NULL;
            call_args->data_len = 0;
            RPC_CN_UNLOCK ();
            return;
        }
        if (frag_buf->data_p != NULL)
        {
            valid_fragbuf = true;
        }
    }

    /*
     * Set call_rep->last_frag_received if this is the last fragment.
     */
    header_p = (rpc_cn_packet_p_t) frag_buf->data_p;
    if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_LAST_FRAG)
    {
        call_rep->last_frag_received = true;
    }

    /*
     * Now adjust data_p to point to just the stub data.
     * Note that data_size has already been adjusted.
     * To do this properly, we have to check the packet
     * type.
     */
    if (RPC_CN_PKT_PTYPE(header_p) == RPC_C_CN_PKT_RESPONSE)
    {
        frag_buf->data_p = (pointer_t)
                      (RPC_CN_PKT_RESP_STUB_DATA(header_p));
    }
    else if (RPC_CN_PKT_PTYPE(header_p) == RPC_C_CN_PKT_REQUEST)
    {
        if (RPC_CN_PKT_OBJ_UUID_PRESENT (header_p))
        {
                frag_buf->data_p = (pointer_t) 
                    RPC_CN_PKT_RQST_STUB_DATA_W_OBJ (header_p);
        }
        else
        {
            frag_buf->data_p = (pointer_t)
                RPC_CN_PKT_RQST_STUB_DATA_NO_OBJ (header_p);
        }
    }
    else if (RPC_CN_PKT_PTYPE(header_p) == RPC_C_CN_PKT_FAULT)
    {
        frag_buf->data_p = (pointer_t) RPC_CN_PKT_FAULT_STUB_DATA (header_p);
        /*
         * We had conservatively assumed that the call
         * has executed as soon as the call started.
         * We can now update that information.
         */
        if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_DID_NOT_EXECUTE)
        {
            call_rep->call_executed = false;
        }

        /*
         * This can be either a call_reject or a call_fault.
         */
        fault_code = RPC_CN_PKT_STATUS(header_p);
        if (fault_code)
        {
            /*
             * Deallocate the fragment buffer since we
             * won't be returning it to the client stub.
             */
            (* frag_buf->fragbuf_dealloc) (frag_buf);

            /*
             * Non-zero fault code implies a call_reject.
             *
             * We translate the on-wire fault code into
             * a local status code & return it.
             */
            *st = rpc__cn_call_cvt_from_nca_st (fault_code);

            /*
             * Release the CN global mutex to allow other threads to
             * run.
             */
            RPC_CN_UNLOCK ();
            return;
        }
        else
        {
            /* 
             * On a fault, we store the address of the fragment 
             * buffer in the call_rep for later retrieval via 
             * rpc__receive_fault.
             */
            call_rep->u.client.fault_data = frag_buf;
            *st = rpc_s_call_faulted;

            /*
             * Release the CN global mutex to allow other threads to
             * run.
             */
            RPC_CN_UNLOCK ();
            return;
        }
    }
    else
    {
        /*
         * This is a protocol error, just return the whole
         * packet.
         */
        call_rep->cn_call_status = rpc_s_protocol_error;
    }


    if (frag_buf->data_size > 0)
    {
        /*
         * Set up the iovector element to point to the received data.
         */
        call_args->buff_addr = (byte_p_t) frag_buf;
        call_args->buff_len = frag_buf->max_data_size;
        call_args->data_addr = (byte_p_t) frag_buf->data_p;
        call_args->data_len = frag_buf->data_size;
        call_args->buff_dealloc =
                (rpc_buff_dealloc_fn_t)frag_buf->fragbuf_dealloc; 
    }
    else
    {
        /*
         * No stub data.
         */
        call_args->data_addr = (byte_p_t) NULL;
        call_args->data_len = 0;

        /*
         * We also deallocate the fragbuf now.
         */
        (* frag_buf->fragbuf_dealloc) (frag_buf);
    }

    /*
     * Check for any pending cancels just in case we're in a
     * long unmarshalling loop and not calling any cancellable
     * operations. Also, forward any cancels which may have been
     * queued already before we were ready to forward them.
     */
    if (RPC_CALL_IS_CLIENT (call_r))
    {
        RPC_CN_CHK_AND_FWD_CANCELS (call_rep, st);
    }
    
    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    *st = call_rep->cn_call_status;
    RPC_CN_UNLOCK ();

    RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT,
        ("PACKET: call receive args.flags->%x args.buff_len->%d args.data_len->%d\n", 
        call_args->flags,
        call_args->buff_len, 
        call_args->data_len));
    RPC_LOG_CN_CALL_RECEIVE_XIT;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_block_until_free
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This routine will block until all buffers given to the RPC
**  runtime by the stub containing marshaled RPC output arguments
**  are free.  It is provided for use by the server stub when
**  the marshaled arguments are contained in buffers which are
**  on stack.  The danger is that the server stub would return
**  to the RPC runtime thereby invalidating its stack and buffer
**  contents.  This routine is intended for use by the server
**  stub only.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**          rpc_s_ok        The call was successful.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_block_until_free 
(
  rpc_call_rep_p_t        call_r,
  unsigned32              *st
)
{
    rpc_cn_call_rep_t       *call_rep;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_block_until_free);
    CODING_ERROR (st);

    call_rep = (rpc_cn_call_rep_p_t) call_r;

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call block until free...\n",
                     call_rep));

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    if (RPC_CN_CREP_ACC_BYTCNT (call_rep) >= RPC_CN_CREP_SIZEOF_HDR (call_rep))
    {
        rpc__cn_transmit_buffers (call_rep, st);
        rpc__cn_dealloc_buffered_data (call_rep);
        RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);
    }

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_alert
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This routine forwards a local alert to the remote RPC thread
**  identified by the call rep provided.  This routine is intended
**  for use by the client stub only.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**          rpc_s_ok        The call was successful.
**          rpc_s_alert_failed
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_alert 
(
  rpc_call_rep_p_t        call_r,
  unsigned32              *st
)
{
    rpc_cn_call_rep_t   *call_rep;
    volatile boolean32  retry_op;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_alert);
    CODING_ERROR (st);

    call_rep = (rpc_cn_call_rep_p_t) call_r;

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call cancel...\n",
                     call_rep));

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    /*
     * The stub has detected a local cancel. Do the right thing
     * based on whether we're a client or server.
     */
    rpc__cn_call_local_cancel (call_rep, &retry_op, st);

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_end
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  Ends a remote procedure call.  This is the last in the sequence
**  of calls by the client or server stub.  This call is intended
**  for use by the client and server stubs only.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC. A NULL will be
**                      returned if this call is successful.
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**          rpc_s_ok        The call was successful.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_end 
(
  rpc_call_rep_p_t        *call_r,
  unsigned32              *st
)
{
    rpc_cn_call_rep_t       *call_rep;
    unsigned32              cur_iov_index;
    rpc_cn_fragbuf_p_t      fault_fbp;
    rpc_cn_assoc_t          *assoc;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_end);
    RPC_LOG_CN_CALL_END_NTR;
    CODING_ERROR (st);

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    call_rep = (rpc_cn_call_rep_p_t) *call_r;

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call end\n",
                     call_rep));

    RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT,
                    ("PACKET: call end\n"));

    if (call_rep != NULL)
    {
        /* 
         * Evaluate the state machine with the call_end event
         */
        RPC_CN_CALL_EVAL_EVENT (RPC_C_CALL_END,
                                NULL, 
                                call_rep, 
                                *st);

        /*
         * Need to hold on to the assoc in a local variable since
         * rpc__cn_assoc_pop_call is going to NULL call_rep->assoc.
         * We'll pass the local variable to rpc__cn_assoc_dealloc.
         */
        assoc = call_rep->assoc;

        /*
         * Pop the call rep off the association.
         */
        rpc__cn_assoc_pop_call (call_rep->assoc, call_rep);

        /*
         * Deallocate the association.
         */
        rpc__cn_assoc_dealloc (assoc, call_rep, st);

        if (RPC_CALL_IS_CLIENT ((rpc_call_rep_p_t) call_rep))
        {
            /*
             * Free remaining fault data fragbufs.
             */
            if (call_rep->u.client.fault_data != NULL)
            {
                fault_fbp = (rpc_cn_fragbuf_p_t) (call_rep->u.client.fault_data);
                if (fault_fbp->fragbuf_dealloc != NULL)
                {
                    (*fault_fbp->fragbuf_dealloc) (fault_fbp);
                }
            }
            
            /*
             * Cancel this thread if:
             *
             * 1) the call completed on the server with a cancel pending
             *    or the number of forwarded cancels is greater than the
             *    number of cancels received by the server. (Note that
             *    handle_recv_frag_action_rtn sets the server_had_pending
             *    flag if one of these conditions exist.)
             *
             * 2) we have caught cancels in this thread but not
             *    forwarded them yet.
             */
            if ((call_rep->u.client.cancel.server_had_pending) ||
                (call_rep->u.client.cancel.local_count > 0))
            {
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                               ("(rpc__cn_call_end) call_rep->%x reposting cancel\n", call_rep));
                dcethread_interrupt_throw (dcethread_self());
            }
            
            /*
             * Stop the cancel timer if one was running.
             */
            rpc__cn_call_stop_cancel_timer (call_rep);
        }

        /*
         * If there are iovector elements in the call_rep,
         * free them all.  There would not be any in the case
         * of a maybe call.
         */
        if (RPC_CN_CREP_IOVLEN (call_rep) > 0)
        {
            for (cur_iov_index = 0; 
                 cur_iov_index < RPC_CN_CREP_IOVLEN (call_rep);
                 cur_iov_index++)
            {
                if (RPC_CN_CREP_IOV (call_rep) [cur_iov_index].buff_dealloc !=
                    NULL)
                {
                    (RPC_CN_CREP_IOV (call_rep) [cur_iov_index].buff_dealloc)
                    (RPC_CN_CREP_IOV (call_rep) [cur_iov_index].buff_addr);
                }
                RPC_CN_CREP_IOV (call_rep) [cur_iov_index].buff_addr = NULL;
            }
        }

        /*
         * If security was used on this call free the fragbuf used
         * for the authentication trailer.  We check to make sure
         * that the fragbuf actually exists because we do not allocate
         * one on the server side for maybe calls.
         */
        if ((call_rep->sec) && (call_rep->prot_tlr != NULL))
        {
            rpc__cn_smfragbuf_free (call_rep->prot_tlr);
        }

        /* 
         * Free the call_rep itself.
         */
        rpc__list_element_free (&rpc_g_cn_call_lookaside_list,
                                (pointer_t) call_rep);
        *call_r = NULL;
    }
    else
    {
        *st = rpc_s_ok;
    }

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();

    RPC_LOG_CN_CALL_END_XIT;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_transmit_fault
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This routine forwards an exception to the remote RPC thread
**  identified by the call rep.  This routine is intended for
**  server stub use only.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**      call_fault_info Marshaled fault information to be returned
**                      to the client.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**          rpc_s_ok        The call was successful.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_transmit_fault 
(
  rpc_call_rep_p_t        call_r,
  rpc_iovector_p_t        call_fault_info,
  unsigned32              *st
)
{
    rpc_cn_call_rep_p_t     call_rep;
    rpc_iovector_elt_p_t    iov_elt_p;
    unsigned32              i;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_transmit_fault);
    CODING_ERROR (st);

    call_rep = (rpc_cn_call_rep_p_t) call_r;

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call transmit fault\n",
                     call_rep));

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    /*
     * If the call has been orphaned, return rpc_s_call_orphaned
     * and deallocate any input data.
     */
    if (call_rep->cn_call_status == rpc_s_call_orphaned)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: call_rep->%x call orphaned...\n",
                         call_rep));
        
        iov_elt_p = call_fault_info->elt;
        for (i = 1; i <= call_fault_info->num_elt; i++, iov_elt_p++)
        {
            if (iov_elt_p->buff_dealloc != NULL)
            {
                (iov_elt_p->buff_dealloc) (iov_elt_p->buff_addr);
            }
        }
        *st = rpc_s_call_orphaned;
    }
    else
    {
        /* 
         * Evaluate the state machine with the call_fault event.
         */
        RPC_CN_CALL_EVAL_EVENT (RPC_C_CALL_FAULT,
                                call_fault_info, 
                                call_rep, 
                                *st);
    }

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_reject
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  Map a local status code to an architected status code.
**  Send a call reject packet.  This is invoked after an
**  error has been discovered in the call executor or receiver
**  threads.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**      l_st            The local status code which will be mapped
**                      to an architected status code to be sent back
**                      to the client.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    This routine assumes that the prototype
**                      send header in the call_rep has been
**                      set up. 
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_reject 
(
  rpc_call_rep_p_t        call_r,
  unsigned32              l_st
)
{
    rpc_cn_call_rep_p_t         call_rep;
    unsigned32                  st ATTRIBUTE_UNUSED;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_reject);

    call_rep = (rpc_cn_call_rep_p_t) call_r;

    RPC_CN_LOCK_ASSERT ();

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call rejected - reason = %x\n",
                     call_rep, l_st));


    /*
     * Map the local status code to an appropriate architected
     * status code.
     */
    call_rep->cn_call_status = rpc__cn_call_cvt_to_nca_st (l_st);

    /*
     * For a fault with an architected status, we pass the
     * architected status code in the call rep and pass a null
     * fault_data.
     */
    RPC_CN_CALL_EVAL_EVENT (RPC_C_CALL_FAULT_DNE, 
                            NULL,
                            call_rep, 
                            st);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_receive_fault
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This routine will unmarshal a fault message from the remote
**  (server) thread.  It is intended for use by the client
**  stub only.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      remote_ndr_format  The Network Data Representation format of the remote
**                      machine. This is used by the stub to unmarshal
**                      arguments encoded using NDR as the trasnfer syntax.
**
**      call_fault_info A vector of buffers containing marshaled RPC
**                      fault information.
**
**      st              The return status of this routine.
**            rpc_s_ok          The call completed normally.
**            rpc_s_no_fault    No fault information available.
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_receive_fault 
(
  rpc_call_rep_p_t        call_r,
  rpc_iovector_elt_p_t    call_fault_info,
  ndr_format_t            *remote_ndr_format,
  unsigned32              *st

)
{
    rpc_cn_call_rep_t       *call_rep;
    rpc_cn_fragbuf_p_t      fault_fragp;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_receive_fault);
    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call receive fault\n",
                     call_r));

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    /*
     * When a fault is received, the appropriate action routine
     * would unmarshal it (into a large fragment buffer) and store
     * its address in the callrep.
     */
    call_rep = (rpc_cn_call_rep_p_t) call_r;
    fault_fragp = (rpc_cn_fragbuf_p_t) (call_rep->u.client.fault_data);
    call_rep->u.client.fault_data = NULL;

    if (fault_fragp != NULL)
    {
        /*
         * Copy the remote NDR format into the output arguments.
         */
        *remote_ndr_format = RPC_CN_ASSOC_NDR_FORMAT (call_rep->assoc);

        /*
         * We now copy the descriptors into the iovector.
         * The stub will deallocate the fragment buffer.
         */
        call_fault_info->buff_dealloc = 
            (rpc_buff_dealloc_fn_t)fault_fragp->fragbuf_dealloc;
        call_fault_info->buff_addr = (byte_p_t) fault_fragp;
        call_fault_info->buff_len = fault_fragp->max_data_size;
        call_fault_info->data_addr = (byte_p_t) fault_fragp->data_p;
        call_fault_info->data_len = fault_fragp->data_size;

        *st = rpc_s_ok;
    }
    else
    {
        call_fault_info->buff_dealloc = NULL;
        call_fault_info->buff_addr = NULL;
        call_fault_info->buff_len = 0;
        call_fault_info->data_addr = NULL;
        call_fault_info->data_len = 0;

        *st = rpc_s_no_fault;
    }

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();
}

/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_did_mgr_execute
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This routine will return whether or not a the manager routine
**  for the call identified by the call handle has begun executing or not.
**  It is intended for use by the client stub only.
**
**  INPUTS:
**
**      call_r          The call rep containing information
**                      pertaining to this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**            rpc_s_ok          The call completed normally.
**            rpc_s_no_fault    No fault information available.
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      boolean32       true if the manager routine has begun, false otherwise.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean32 rpc__cn_call_did_mgr_execute 
(
  rpc_call_rep_p_t        call_r,
  unsigned32              *st ATTRIBUTE_UNUSED
)
{
    rpc_cn_call_rep_t       *call_rep;
    boolean                 call_executed;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_did_mgr_execute);
    CODING_ERROR (st);

    call_rep = (rpc_cn_call_rep_p_t) call_r;

    /*
     * Acquire the CN global mutex to prevent other threads from
     * running.
     */
    RPC_CN_LOCK ();

    call_executed = call_rep->call_executed;

    /*
     * Release the CN global mutex to allow other threads to
     * run.
     */
    RPC_CN_UNLOCK ();
    return (call_executed);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_cvt_from_nca_st
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  This routine will translate an on-wire status code (nca_s..)
**  into the corresponding runtime status code (rpc_s..).
**
**  INPUTS:
**
**      a_st            The NCA status code as rets it appears on the wire.
**                      This is the status as returned by, for example,
**                      a call_fault packet.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      unsigned32      The runtime status code corresponding to
**                      the nca status code.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32 rpc__cn_call_cvt_from_nca_st 
(
  unsigned32 a_st
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_cvt_from_nca_st);
    switch ((int)a_st)
    {
        case nca_s_op_rng_error:                return (rpc_s_op_rng_error);
        case nca_s_unk_if:                      return (rpc_s_unknown_if);
        case nca_s_proto_error:                 return (rpc_s_protocol_error);
        case nca_s_server_too_busy:             return (rpc_s_server_too_busy);
        case nca_s_invalid_pres_context_id:     return (rpc_s_context_id_not_found);
        case nca_s_unsupported_type:            return (rpc_s_unsupported_type);
        case nca_s_manager_not_entered:         return (rpc_s_manager_not_entered);
        case nca_s_out_args_too_big:            return (rpc_s_credentials_too_large);
        case nca_s_unsupported_authn_level:     return (rpc_s_unsupported_authn_level);
        case nca_s_invalid_checksum:            return (rpc_s_invalid_checksum);
        case nca_s_invalid_crc:                 return (rpc_s_invalid_crc);
        default:                                return (rpc_s_unknown_reject);
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_cvt_to_nca_st
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  This routine will translate a local status to to an architected status code (nca_s..).
**
**  INPUTS:
**
**      l_st            The local (rpc_s...) status code.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      unsigned32      The architected (nca_s...) status code.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32 rpc__cn_call_cvt_to_nca_st 
(
  unsigned32 l_st
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_cvt_to_nca_st);
    switch ((int)l_st)
    {
        case rpc_s_op_rng_error:                return (nca_s_op_rng_error);
        case rpc_s_unknown_if:                  return (nca_s_unk_if);
        case rpc_s_protocol_error:              return (nca_s_proto_error);
        case rpc_s_cthread_not_found:           return (nca_s_server_too_busy);
        case rpc_s_server_too_busy:             return (nca_s_server_too_busy);
        case rpc_s_unsupported_type:            return (nca_s_unsupported_type);
        case rpc_s_unknown_mgr_type:            return (nca_s_unsupported_type);
        case rpc_s_manager_not_entered:         return (nca_s_manager_not_entered);
        case rpc_s_context_id_not_found:        return (nca_s_invalid_pres_context_id);
        case rpc_s_call_orphaned:               return (nca_s_unspec_reject);
        case rpc_s_unknown_reject:              return (nca_s_unspec_reject);
        case rpc_s_credentials_too_large:       return (nca_s_out_args_too_big);
        case rpc_s_unsupported_authn_level:     return (nca_s_unsupported_authn_level);
        case rpc_s_invalid_checksum:            return (nca_s_invalid_checksum);
        case rpc_s_invalid_crc:                 return (nca_s_invalid_crc);
        default:
            RPC_DBG_GPRINTF(("(rpc__cn_call_cvt_to_nca_st) unknown status; st=%08lx\n", l_st));
            return (nca_s_unspec_reject);
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_no_conn_ind
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This routine will be called from a client receiver thread when
**  it detects the connection a call is in progress on has been lost.
**
**  INPUTS:
**
**      call_rep        The call rep data structure containing the
**                      state of the call in progress.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_no_conn_ind 
(
  rpc_cn_call_rep_p_t call_rep ATTRIBUTE_UNUSED
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_no_conn_ind);

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: call_rep->%x call no connection indication\n",
                     call_rep));

    /*
     * For now this routine does nothing.
     */
}

/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_ccb_create
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This routine will initialize an call control block
**  which is allocated from heap by rpc__list_element_alloc. It
**  will allocate a fragbuf for the protocol header and initialize
**  some fields of the header.
**
**  INPUTS:
**
**      ccb             The call control block to be initialized.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_ccb_create 
(
  rpc_cn_call_rep_p_t     ccb
)
{
    rpc_cn_fragbuf_p_t      fragbuf_p;
    rpc_cn_packet_p_t       header_p;
    rpc_iovector_elt_p_t    iov_p;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_ccb_create);

    RPC_LIST_INIT (ccb->common.link);
    ccb->common.protocol_id = RPC_C_PROTOCOL_ID_NCACN;
    fragbuf_p = rpc__cn_fragbuf_alloc (false);
    fragbuf_p->fragbuf_dealloc = NULL;
    ccb->prot_header = fragbuf_p;
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (ccb);

    /*
     * Copy the common header fields.
     */
    memcpy (header_p, &rpc_g_cn_common_hdr, sizeof (rpc_g_cn_common_hdr));

    iov_p = &(RPC_CN_CREP_IOV (ccb)[0]);
    iov_p->buff_dealloc = NULL;
    iov_p->buff_addr = (byte_p_t) fragbuf_p;
    iov_p->buff_len = fragbuf_p->max_data_size;
    iov_p->data_addr = (byte_p_t) fragbuf_p->data_p;

    /*
     * Init the common call rep mutex.
     */
    RPC_CALL_LOCK_INIT ((rpc_call_rep_p_t) ccb);
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_ccb_free
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  This routine will free the fragment buffer contained in an call
**  control block before the rpc__list_element_free routine returns
**  it to heap storage. 
**
**  INPUTS:
**
**      ccb             The call control block to be initialized.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_ccb_free 
(
  rpc_cn_call_rep_p_t     ccb
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_ccb_free);

    /*
     * Free the small fragbuf used for the protocol header.
     */
    if (ccb->prot_header != NULL)
    {
        rpc__cn_smfragbuf_free (ccb->prot_header);
    }

    /*
     * Delete the common call rep mutex.
     */
    RPC_CALL_LOCK_DELETE ((rpc_call_rep_p_t) ccb);
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_local_cancel
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  A local cancel has been detected (i.e., a cancellable operation
**  caused an unwound).  This routine can be invoked from either
**  the client or the server.
**  If it occurs on the client, a cancel is forwarded to the server.
**  If it occurs on the server, rpc_s_call_cancelled is returned.
**
**  Note that this routine assumes that the global CN lock is held.
**
**  INPUTS:
**
**      call_rep        The call rep.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_local_cancel 
(
  rpc_cn_call_rep_p_t     call_rep,
  volatile boolean32      *retry_op,
  unsigned32              *status
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_local_cancel);
    CODING_ERROR (status);

    RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                   ("(rpc__cn_call_local_cancel) call_rep->%x local cancel caught\n", 
                    call_rep));
    /*
     * If the call rep is NULL, this is the server side of a
     * call and it has been orphaned. We were in a cancellable pipe 
     * routine. Return the rpc_s_call_cancelled status code.
     */
    if (call_rep == NULL)
    {
        *retry_op = false;
        *status = rpc_s_call_cancelled;
        return;
    }

    if (RPC_CALL_IS_CLIENT (((rpc_call_rep_t *) call_rep)))
    {
        /*
         * Record the cancel that was just detected.
         */
        call_rep->u.client.cancel.local_count++;
        rpc__cn_call_start_cancel_timer (call_rep, status);
        if (*status == rpc_s_ok)
        {
            /*
             * Send a cancel event through the state machine for all
             * cancels detected so far but not forwarded yet.
             */
            rpc__cn_call_forward_cancel (call_rep, status);
            *retry_op = true;
        }
        else
        {
            *retry_op = false;
        }
    }
    else
    {
        *retry_op = false;
        *status = rpc_s_call_cancelled;
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_check_for_cancel
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Check for a pending cancel. If one is caught increment the local
**  count field of the call rep to indicate it. Of course, if
**  general cancel delivery is disabled, then we aren't required
**  (and in fact don't want) to detect one.
**
**  INPUTS:
**
**      call_rep        The call rep.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__cn_call_check_for_cancel 
(
  rpc_cn_call_rep_p_t     call_rep
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_check_for_cancel);

    /*
     * If a cancel timeout has been established there's no need to
     * check. A cancel has been forwarded and no response has been
     * received yet.
     */
    if (call_rep->u.client.cancel.timeout_time == 0)
    {
        DCETHREAD_TRY 
        { 
            dcethread_checkinterrupt (); 
        } 
        DCETHREAD_CATCH (dcethread_interrupt_e) 
        { 
            RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                            ("(rpc__cn_call_check_for_cancel) call_rep->%x local cancel detected\n", call_rep));
            (call_rep)->u.client.cancel.local_count++; 
        } 
        DCETHREAD_ENDTRY
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_forward_cancel
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Forward all cancels which have been detected to this point to
**  the server.
**
**  INPUTS:
**
**      call_rep        The call rep.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__cn_call_forward_cancel 
(
  rpc_cn_call_rep_p_t     call_rep,
  unsigned32              *status
)
{
    unsigned32          temp_status ATTRIBUTE_UNUSED;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_forward_cancel);
    CODING_ERROR (status);

    /*
     * Foward all queued cancels if the first request fragment has been sent
     * to the server already.
     */
    if (call_rep->u.client.cancel.server_is_accepting)
    {
        for (; call_rep->u.client.cancel.local_count > 0;
             call_rep->u.client.cancel.local_count--)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                           ("(rpc__cn_call_forward_cancel) call_rep-> %x forwarding cancel\n", call_rep));
            RPC_CN_CALL_EVAL_EVENT (RPC_C_CALL_LOCAL_ALERT,
                                    NULL, 
                                    call_rep, 
                                    temp_status);
        }
    }
    else
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                       ("(rpc__cn_call_forward_cancel) call_rep->%x haven't sent first frag yet\n", call_rep));
    }
    *status = call_rep->cn_call_status;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_binding_serialize
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Serialize calls on this binding handle until the handle has a bound
**  server instance.  This is necessary to maintain the promise: "All
**  calls to using a specific binding handle will go to the same server
**  instance".
**
**  INPUTS:
**
**      binding_r       The binding rep access to which is being serialized
**      cancel_timeout  The thread's cancel timeout value
**
**  INPUTS/OUTPUTS:     
**
**      cancel_cnt      The number of cancels detected while waiting
**                      for the binding rep to become free. 
**
**  OUTPUTS:            
**
**      st              The RPC runtime status code 
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__cn_call_binding_serialize 
(
  rpc_binding_rep_p_t     binding_r,
  rpc_clock_t             cancel_timeout,
  unsigned32              *cancel_cnt,
  unsigned32              *st
)
{
    volatile boolean    is_awaiting_timeout = false;
    volatile boolean    has_timed_out = false;
    volatile struct timespec     zero_delta, delta, abstime, curtime;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_binding_serialize);
    RPC_LOCK_ASSERT(0);        
    CODING_ERROR (st);

    *st = rpc_s_ok;

    zero_delta.tv_sec = 0;
    zero_delta.tv_nsec = 0;
    delta.tv_sec = 0;
    delta.tv_nsec = 0;

    delta.tv_sec = cancel_timeout;

    /*
     * Wait while the binding is being resolved by someone else 
     * (this routine is called only by threads not doing the 
     * resolving) and we haven't timed out.
     * 
     * If the user attempts to cancel the call,
     * don't wait longer than the cancel timeout time.
     */
    while ((((rpc_cn_binding_rep_t *)binding_r)->being_resolved) &&
           (!has_timed_out))
    {
        DCETHREAD_TRY
        {
            if (!is_awaiting_timeout)
            {
                RPC_BINDING_COND_WAIT (0);
            }
            else
            {
                RPC_BINDING_COND_TIMED_WAIT ((struct timespec *) &abstime);
                dcethread_get_expiration ((struct timespec *) &zero_delta, 
                                          (struct timespec *) &curtime);
                if (curtime.tv_sec >= abstime.tv_sec)
                {
                    has_timed_out = true;
                }
            }
        }
        DCETHREAD_CATCH (dcethread_interrupt_e)
        {
            /*
             * Track the cancels and setup a cancel timeout value 
             * (if appropriate).
             */
            RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                           ("(rpc__cn_call_binding_serialize) binding_rep->%x cancel detected\n", binding_r));
            if (delta.tv_sec == 0)
            {
                has_timed_out = true;
            }
            else
            {
                if (delta.tv_sec == rpc_c_cancel_infinite_timeout)
                {
                    ;   /* we never timeout */
                }
                else
                {
                    /*
                     * Compute the max timeout time for the wait.
                     * Generate a cancel time stamp for use by the caller
                     * in subsequently setting up the call's cancel state.
                     */
                    if (is_awaiting_timeout == false)
                    {
                        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                                       ("(rpc__cn_call_binding_serialize) binding_rep->%x %d sec cancel timeout setup\n",
                                        binding_r, delta.tv_sec));
                        
                        dcethread_get_expiration ((struct timespec *) (&delta), 
                                                  (struct timespec *) (&abstime));
                    }
                    is_awaiting_timeout = true;
                }
            }
            *cancel_cnt += 1;

        /* 
         * Any other type of exception is something serious; just let it 
         * propagate and we die in our usual unclean fashion.
         */
        }
        DCETHREAD_ENDTRY
    }

    if (has_timed_out)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                       ("(rpc__cn_call_binding_serialize) binding_rep->%x cancel timeout\n", binding_r));
        *st = rpc_s_cancel_timeout;
    }
    else if (binding_r->addr_has_endpoint == false)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                       ("(rpc__cn_call_binding_serialize) binding_rep->%x endpoint not found\n", binding_r));
        *st = rpc_s_endpoint_not_found;
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_start_cancel_timer
**
**  SCOPE:              PRIVATE - delcared in cncall.h
**
**  DESCRIPTION:
**
**  Start a timer to time out cancels.
**
**  INPUTS:
**
**      call_r          The call rep representing the RPC being made.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The RPC runtime status code 
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_start_cancel_timer 
(
  rpc_cn_call_rep_p_t     call_r,
  unsigned32              *st
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_start_cancel_timer);
    CODING_ERROR (st);

    RPC_CN_LOCK_ASSERT ();

    /*
     * The timer may have already expired.
     */
    if ((*st = call_r->cn_call_status) == rpc_s_ok)
    {
        /*
         * Start a cancel timer if one is not already running and the
         * cancel timeout for this thread is not infinite.
         */
        if ((!call_r->u.client.cancel.timer_running) &&
            (call_r->u.client.cancel.timeout_time != (typeof(call_r->u.client.cancel.timeout_time))(rpc_c_cancel_infinite_timeout)))
        {
            RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                           ("(rpc__cn_call_start_cancel_timer) call_rep->%x starting cancel timer - %d seconds\n", 
                            call_r, call_r->u.client.cancel.timeout_time));
            call_r->u.client.cancel.timer_running = true;
            call_r->u.client.cancel.thread_h = dcethread_self ();
            rpc__timer_set (&call_r->u.client.cancel.timer,
                            (rpc_timer_proc_p_t) rpc__cn_call_cancel_timer,
                            (pointer_t) call_r,
                            (rpc_clock_t) RPC_CLOCK_SEC (call_r->u.client.cancel.timeout_time));
        }
    }
    else
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                       ("(rpc__cn_call_start_cancel_timer) call_rep->%x timer expired ... returning rpc_s_cancel_timeout\n", 
                        call_r));
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_stop_cancel_timer
**
**  SCOPE:              PRIVATE - declared in cncall.h
**
**  DESCRIPTION:
**
**  Clear the timer to time out cancels.
**
**  INPUTS:
**
**      call_r          The call rep representing the RPC being made.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The RPC runtime status code 
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_call_stop_cancel_timer 
(
  rpc_cn_call_rep_p_t     call_r
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_stop_cancel_timer);

    if (call_r->u.client.cancel.timer_running)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                       ("(rpc__cn_call_stop_cancel_timer) call_rep->%x cancel timer stopped\n", call_r));
        rpc__timer_clear (&call_r->u.client.cancel.timer);
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_cancel_timer
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Timer routine to time out cancels
**
**  INPUTS:
**
**      call_r          The call rep representing the RPC being made.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The RPC runtime status code 
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      true            the routine should be rescheduled
**      false           the routine should not be rescheduled
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean rpc__cn_call_cancel_timer 
(
  rpc_cn_call_rep_p_t     call_r
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_cancel_timer);

    RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                   ("(rpc__cn_call_cancel_timer) call_rep->%x cancel timer expired\n", call_r));
    RPC_CN_LOCK ();
    call_r->cn_call_status = rpc_s_cancel_timeout;
    dcethread_interrupt_throw (call_r->u.client.cancel.thread_h);
    call_r->u.client.cancel.timer_running = false;

    /*
     * Set the server had pending flag so that the cancel which
     * started this timer will be reposted in call_end.
     */
    call_r->u.client.cancel.server_had_pending = true;
    rpc__timer_clear (&call_r->u.client.cancel.timer);
    RPC_CN_UNLOCK ();
    return (false);
}
