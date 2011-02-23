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
**  NAME
**
**      cncthd.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The NCA Connection Protocol Service's Call Executor Service.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnid.h>       /* NCA Connection local ID service */
#include <cnnet.h>      /* NCA Connection network service */
#include <cnpkt.h>      /* NCA Connection protocol header */
#include <comcthd.h>    /* Externals for call thread services component */
#include <cncall.h>     /* NCA connection call service */
#include <cnassoc.h>    /* NCA Connection association services */
#include <cncthd.h>     /* local header file for prototype */


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_executor
**
**  SCOPE:              PRIVATE - declared in cncthd.h
**
**  DESCRIPTION:
**      
**  INPUTS:
**
**      arg             The call rep for this call.
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

PRIVATE void rpc__cn_call_executor 
(pointer_t arg, boolean32 call_was_queued ATTRIBUTE_UNUSED) 
{
    rpc_binding_rep_t           *binding_r;
    rpc_cn_call_rep_t           *call_r;
    rpc_iovector_t              iovector;
    dce_uuid_t                      type_uuid;
    rpc_mgr_epv_t               manager_epv;
    rpc_v2_server_stub_epv_t    server_stub_epv;
    rpc_if_rep_p_t              if_spec_rep;
    unsigned32                  flags;
    unsigned32                  max_calls;
    unsigned32                  max_rpc_size;
    rpc_if_callback_fn_t        if_callback;
    unsigned32                  status;

    RPC_LOG_CN_CTHD_NTR;

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("CN: call_rep->%x call executor running ... %s queued\n", arg, (call_was_queued ? "WAS" : "WAS NOT")));

    /*
     * The arg passed in is really a call rep.
     */
    call_r = (rpc_cn_call_rep_t *) arg;

    /*
     * Release the call rep lock which was acquired for us in the
     * common code.
     */
    RPC_CALL_UNLOCK ((rpc_call_rep_t *) call_r);

    /*
     * If there is an object uuid, see if there's a type uuid
     * associated with it.
     */
    rpc_object_inq_type (&call_r->binding_rep->obj, &type_uuid, &status);
    if ((status != rpc_s_object_not_found) && (status != rpc_s_ok))
    {
        RPC_CALL_LOCK ((rpc_call_rep_t *) call_r);
        rpc__cthread_cancel_caf ((rpc_call_rep_t *) call_r);
        RPC_CALL_UNLOCK ((rpc_call_rep_t *) call_r);
        RPC_CN_LOCK ();
        rpc__cn_call_reject ((rpc_call_rep_p_t) call_r, status);
        RPC_CN_UNLOCK ();
        goto CLEANUP;
    }
    
    /*
     * Get the if rep and the server stub and manager EPV.
     */
    rpc__if_lookup2 (call_r->u.server.if_id,
                     call_r->u.server.if_vers,
                     &type_uuid, 
                     &call_r->u.server.ihint, 
                     &if_spec_rep,
                     &server_stub_epv, 
                     &manager_epv, 
                     &flags, 
                     &max_calls, 
                     &max_rpc_size, 
                     &if_callback, 
                     &status);
    if (status != rpc_s_ok)
    {
        RPC_CALL_LOCK ((rpc_call_rep_t *) call_r);
        rpc__cthread_cancel_caf ((rpc_call_rep_t *) call_r);
        RPC_CALL_UNLOCK ((rpc_call_rep_t *) call_r);
        RPC_CN_LOCK ();
        rpc__cn_call_reject ((rpc_call_rep_p_t) call_r, status);
        RPC_CN_UNLOCK ();
        goto CLEANUP;
    }
    
    /*
     * If the operation number is out of range, indicate a fault to
     * the protocol service, otherwise process the incoming packet(s).
     */
    if (call_r->opnum >= if_spec_rep->opcnt)
    {
        RPC_CALL_LOCK ((rpc_call_rep_t *) call_r);
        rpc__cthread_cancel_caf ((rpc_call_rep_t *) call_r);
        RPC_CALL_UNLOCK ((rpc_call_rep_t *) call_r);
        RPC_CN_LOCK ();
        rpc__cn_call_reject ((rpc_call_rep_p_t) call_r, rpc_s_op_rng_error);
        RPC_CN_UNLOCK ();
        goto CLEANUP;
    }

    /*
     * Receive the first packet.
     */
    rpc__cn_call_receive ((rpc_call_rep_t *) call_r,
                          &iovector.elt[0], 
                          &status);
    if (status != rpc_s_ok)
    {
        RPC_CALL_LOCK ((rpc_call_rep_t *) call_r);
        rpc__cthread_cancel_caf ((rpc_call_rep_t *) call_r);
        RPC_CALL_UNLOCK ((rpc_call_rep_t *) call_r);
        RPC_CN_LOCK ();
        rpc__cn_call_reject ((rpc_call_rep_p_t) call_r, rpc_s_op_rng_error);
        RPC_CN_UNLOCK ();
        goto CLEANUP;
    }
    
    /*
     * Mark the call as having executed.
     */
    call_r->call_executed = true;
    
    /*
     * Enable posting of cancels to this call executor thread.
     * This will also post any queued cancels.
     */
    RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                   ("(rpc__cn_call_executor) call_rep->%x enabling posting of cancels and posting any queued cancels\n", call_r));
    RPC_CALL_LOCK ((rpc_call_rep_t *) call_r);
    rpc__cthread_cancel_enable_post ((rpc_call_rep_p_t) call_r);
    RPC_CALL_UNLOCK ((rpc_call_rep_t *) call_r);
   
    /*
     * Dispatch appropriately depending on the stub version.
     */
    switch (if_spec_rep->stub_rtl_if_vers)
    {
        /*
         * If this is an old v0 or v1 stub runtime interface
         * then do the dirty work out of line.
         */
        case 0:
        case 1:
	/*
	 * rpc_m_pre_v2_ifspec
	 * "(%s) Pre-v2 interface spec"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	    rpc_svc_server_call,
	    svc_c_sev_fatal | svc_c_action_abort,
	    rpc_m_pre_v2_ifspec,
	    "rpc__cn_call_executor" ));
	break;

        /*
         * This is the v2 (new) stub runtime interface.
         */
        case 2:
        
        RPC_LOG_SERVER_STUB_PRE;
        ((*server_stub_epv[call_r->opnum])) 
        ((handle_t) call_r->binding_rep,
         (rpc_call_handle_t) call_r,
         &iovector.elt[0],
         &(RPC_CN_ASSOC_NDR_FORMAT (call_r->assoc)),
         &call_r->transfer_syntax,
         manager_epv, 
         &status);
        RPC_LOG_SERVER_STUB_POST;
        break;
        
        /*
         * Unknown version
         */
        default:
	/*
	 * rpc_m_unk_ifspec
	 * "(%s) Unknown interface spec version"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	    rpc_svc_server_call,
	    svc_c_sev_fatal | svc_c_action_abort,
	    rpc_m_pre_v2_ifspec,
	    "rpc__cn_call_executor" ));
	break;
    }

    /*
     * Check for an error while in the server stub but before the
     * manager routine was entered.
     */
    if (status != rpc_s_ok)
    {
        RPC_CALL_LOCK ((rpc_call_rep_t *) call_r);
        rpc__cthread_cancel_caf ((rpc_call_rep_t *) call_r);
        RPC_CALL_UNLOCK ((rpc_call_rep_t *) call_r);
        RPC_CN_LOCK ();
        rpc__cn_call_reject ((rpc_call_rep_p_t) call_r, status);
        RPC_CN_UNLOCK ();
        goto CLEANUP;
    }

    /*
     * If the stub returned successfully, end the call and free the binding handle.
     */
    binding_r = (rpc_binding_rep_t *) call_r->binding_rep;
    RPC_CALL_LOCK ((rpc_call_rep_t *) call_r);
    rpc__cthread_cancel_caf ((rpc_call_rep_t *) call_r);
    RPC_CALL_UNLOCK ((rpc_call_rep_t *) call_r);

CLEANUP:
    binding_r = (rpc_binding_rep_t *) call_r->binding_rep;
    rpc__cn_call_end ((rpc_call_rep_p_t *) &call_r, &status);
    RPC_LOCK (0);
    RPC_BINDING_RELEASE (&binding_r,
                         &status);
    RPC_UNLOCK (0);
    RPC_LOG_CN_CTHD_XIT;
}
