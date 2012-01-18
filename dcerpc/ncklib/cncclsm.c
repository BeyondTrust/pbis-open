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
**      cncclsm.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Client call State Machine for the Connection-based RPC runtime.
**
**
*/


#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnfbuf.h>     /* NCA Connection fragment buffer declarations */
#include <cnpkt.h>      /* NCA Connection protocol header */
#include <cnassoc.h>    /* NCA Connection association services */
#include <cnxfer.h>     /* NCA Connection buffered data transfer routines */
#include <cnsm.h>       /* NCA Connection state machine declarations */
#include <cncall.h>     /* NCA connection call service */
#include <cnclsm.h>


/******************************************************************************/
/*
 * Global Definitions
 */
#ifdef DEBUG
GLOBAL char     *rpc_g_cn_call_client_events [] =
{
    "TRANSMIT_REQ     ",
    "CONFIRM          ",
    "FAULT_DNE        ",
    "FAULT            ",
    "LOCAL_ALERT      ",
    "END              ",
    "ASSOC_ALLOC_ACK  ",
    "ASSOC_ALLOC_NAK  ",
    "START            ",
    "LAST_TRANSMIT_REQ",
    "LOCAL_ERROR      ",
    "ALERT_TIMEOUT    "
};

GLOBAL char     *rpc_g_cn_call_client_states [] =
{
    "INIT             ",
    "ASSOC_ALLOC_WAIT ",
    "STUB_WAIT        ",
    "REQUEST          ",
    "RESPONSE         ",
    "CALL_COMPLETED   ",
    "CALL_FAILED_DNE  ",
    "CALL_FAILED      "
};
#endif /* DEBUG */


/***********************************************************************/
/*
** C L I E N T   C A L L   P R E D I C A T E   T A B L E
**/
/*  
 * The predicates.
 * As a performance enhancement,
 * we have revamped many predicate routines as macros and have absorbed
 * the predicates into the actions.  Thus, there is no longer a need
 * for the predicate table;  the predicate declarations too, are
 * modified. 
 */
/* 
#define MAYBE_SEMANTICS_PRED    0
#define LAST_RECV_FRAG_PRED     1
*/ 
/*  
 * The predicate routine prototypes.
 */
INTERNAL unsigned8 maybe_semantics_pred_rtn (
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/
    ) ATTRIBUTE_UNUSED;
INTERNAL unsigned8 last_recv_frag_pred_rtn (
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/
    ) ATTRIBUTE_UNUSED;


/***********************************************************************/
/*
** C L I E N T   C A L L   A C T I O N   T A B L E
**/

/***********************************************************************/

/*  
 * The actions.
 *
 * The QueueAlertTimeout action routine in the NCA CN arch spec
 * is not listed here since cancel timeouts are handled outside the
 * state machine. See the routine header of forward_alert_action_rtn
 * for more details.
 *
 * The VerifySecurity action routine in the NCA CN arch spec is not
 * listed here since this was embedded into existing action routines,
 * where appropriate in this implementation.
 */
#define TRANSMIT_REQ            0
#define HANDLE_RECV_FRAG        1
#define RAISE_FAULT             2
#define FORWARD_ALERT           3
#define ALLOCATE_ASSOC          4

/* abort send = send_orphaned + deallocate assoc + raise fault */
#define ABORT_SEND              5

#define ABORT_RECV              6
#define SEND_LAST_FRAG          7
#define PROTOCOL_ERROR          8

/*  
 * The Action routine prototypes.
 */
INTERNAL unsigned32     transmit_req_action_rtn ( 
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/,
        pointer_t /*sm*/
    );
INTERNAL unsigned32     handle_recv_frag_action_rtn ( 
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/,
        pointer_t /*sm*/
    );
INTERNAL unsigned32     raise_fault_action_rtn (
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/,
        pointer_t /*sm*/
    );
INTERNAL unsigned32     forward_alert_action_rtn (  
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/,
        pointer_t /*sm*/
    );
INTERNAL unsigned32     allocate_assoc_action_rtn ( 
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/,
        pointer_t /*sm*/
    );
INTERNAL unsigned32     abort_send_action_rtn (
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/,
        pointer_t /*sm*/
    );
INTERNAL unsigned32     abort_recv_action_rtn (
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/,
        pointer_t /*sm*/
    );
INTERNAL unsigned32     send_last_frag_action_rtn ( 
        pointer_t /*spc_struct*/, 
        pointer_t /*event_param*/,
        pointer_t /*sm*/
    );

/*  
 * The action table itself.
 */
GLOBAL rpc_cn_sm_action_fn_t  rpc_g_cn_client_call_action_tbl [] =
{
    transmit_req_action_rtn,
    handle_recv_frag_action_rtn,
    raise_fault_action_rtn,
    forward_alert_action_rtn,
    allocate_assoc_action_rtn,
    abort_send_action_rtn,
    abort_recv_action_rtn,
    send_last_frag_action_rtn,
    rpc__cn_call_sm_protocol_error 
};

/***********************************************************************/
/*
** C L I E N T   C A L L   S T A T E   T A B L E
**/

INTERNAL rpc_cn_sm_state_tbl_entry_t init_state =

    /* state 0 - init */
    {
        ILLEGAL_TRANSITION,                 /* event 0 */
        ILLEGAL_TRANSITION,                 /* event 1 */
        ILLEGAL_TRANSITION,                 /* event 2 */
        ILLEGAL_TRANSITION,                 /* event 3 */
        ILLEGAL_TRANSITION,                 /* event 4 */
		  {RPC_C_CLIENT_CALL_CFDNE},	    /* event 5 - call_end */
        ILLEGAL_TRANSITION,                 /* event 6 */
        ILLEGAL_TRANSITION,                 /* event 7 */
		  {ALLOCATE_ASSOC},  		    /* event 8 - start_call */
        ILLEGAL_TRANSITION,                 /* event 9 */
        ILLEGAL_TRANSITION,                 /* event 10 */
        ILLEGAL_TRANSITION                  /* event 11 */
    };

    /* state 1 - assoc_alloc_wait */
INTERNAL rpc_cn_sm_state_tbl_entry_t assoc_alloc_wait_state =
    {
        ILLEGAL_TRANSITION,                 /* event 0 */
        ILLEGAL_TRANSITION,                 /* event 1 */
        ILLEGAL_TRANSITION,                 /* event 2 */
        ILLEGAL_TRANSITION,                 /* event 3 */
        ILLEGAL_TRANSITION,                 /* event 4 */
		  {RPC_C_CLIENT_CALL_CFDNE}, 	    /* event 5 - call_end */
		  {RPC_C_CLIENT_CALL_STUB_WAIT},        /* event 6 - alloc_assoc_ack */
		  {RPC_C_CLIENT_CALL_CFDNE},            /* event 7 - alloc_assoc_nak */
        ILLEGAL_TRANSITION,                 /* event 8 */
        ILLEGAL_TRANSITION,                 /* event 9 */
        ILLEGAL_TRANSITION,                 /* event 10 */
        ILLEGAL_TRANSITION                  /* event 11 */
    };

    /* state 2 - stub_wait */
INTERNAL rpc_cn_sm_state_tbl_entry_t stub_wait_state =
    {
		 {TRANSMIT_REQ},    	            /* event 0 - transmit_req */
        ILLEGAL_TRANSITION,                 /* event 1 */
        ILLEGAL_TRANSITION,                 /* event 2 */
        ILLEGAL_TRANSITION,                 /* event 3 */
        ILLEGAL_TRANSITION,                 /* event 4 */
		  {RPC_C_CLIENT_CALL_CFDNE},            /* event 5 - call_end */
        ILLEGAL_TRANSITION,                 /* event 6 */
        ILLEGAL_TRANSITION,                 /* event 7 */
        ILLEGAL_TRANSITION,                 /* event 8 */
		  {SEND_LAST_FRAG},  	            /* event 9 - last_transmit_req */
		  {RPC_C_CLIENT_CALL_CFDNE},            /* event 10 - local_err */
        ILLEGAL_TRANSITION,                 /* event 10 */
        ILLEGAL_TRANSITION                  /* event 11 */
    };

    /* state 3 - call_request */
INTERNAL rpc_cn_sm_state_tbl_entry_t call_request_state =
    {
		 {TRANSMIT_REQ},  		            /* event 0 - transmit_req */
        ILLEGAL_TRANSITION,                 /* event 1 */
		  {RAISE_FAULT}, 		            /* event 2 - fault_dne */
		  {RAISE_FAULT}, 		            /* event 3 - fault */
		  {FORWARD_ALERT}, 		            /* event 4 - local alert */
		  {ABORT_SEND},  			    /* event 5 - call_end */
        ILLEGAL_TRANSITION,                 /* event 6 */
        ILLEGAL_TRANSITION,                 /* event 7 */
        ILLEGAL_TRANSITION,                 /* event 8 */
		  {SEND_LAST_FRAG},  	            /* event 9 - last_transmit_req */
		  {ABORT_SEND}, 			    /* event 10 - local_err */
		  {ABORT_SEND}  			    /* event 11 - alert timeout */
    };

    /* state 4 - call response */
INTERNAL rpc_cn_sm_state_tbl_entry_t call_response_state =
    {
        ILLEGAL_TRANSITION,                 /* event 0 */
		  {HANDLE_RECV_FRAG}, 	            /* event 1 - rpc_conf */
		  {RAISE_FAULT},  		            /* event 2 - fault_dne */
		  {RAISE_FAULT}, 		            /* event 3 - fault */
		  {FORWARD_ALERT},  	            /* event 4 - local alert */
		  {ABORT_SEND},  		            /* event 5 - call_end */
        ILLEGAL_TRANSITION,                 /* event 6 */
        ILLEGAL_TRANSITION,                 /* event 7 */
        ILLEGAL_TRANSITION,                 /* event 8 */
        ILLEGAL_TRANSITION,                 /* event 9 */
		  {ABORT_SEND},          		    /* event 10 - local_err */
		  {ABORT_SEND}                          /* event 11 - alert timeout */
    };

    /* state 5 - call_completed */
INTERNAL rpc_cn_sm_state_tbl_entry_t call_completed_state =
    {
        ILLEGAL_TRANSITION,                 /* event 0 */
        ILLEGAL_TRANSITION,                 /* event 1 */
        ILLEGAL_TRANSITION,                 /* event 2 */
        ILLEGAL_TRANSITION,                 /* event 3 */
		  {RPC_C_CLIENT_CALL_CALL_COMPLETED},   /* event 4 - local alert */
		  {RPC_C_CLIENT_CALL_CALL_COMPLETED},   /* event 5 - call_end */
        ILLEGAL_TRANSITION,                 /* event 6 */
        ILLEGAL_TRANSITION,                 /* event 7 */
        ILLEGAL_TRANSITION,                 /* event 8 */
        ILLEGAL_TRANSITION,                 /* event 9 */
        ILLEGAL_TRANSITION,                 /* event 10 */
        ILLEGAL_TRANSITION                  /* event 11 */
    };

    /* state 6 - cfdne (call failed, did not execute) */
INTERNAL rpc_cn_sm_state_tbl_entry_t cfdne_state =
    {
		 {RPC_C_CLIENT_CALL_CFDNE},            /* event 0 */
        ILLEGAL_TRANSITION,                 /* event 1 */
        ILLEGAL_TRANSITION,                 /* event 2 */
        ILLEGAL_TRANSITION,                 /* event 3 */
		  {RPC_C_CLIENT_CALL_CFDNE},            /* event 4 - local alert */
		  {RPC_C_CLIENT_CALL_CFDNE},            /* event 5 - call_end */
        ILLEGAL_TRANSITION,                 /* event 6 */
        ILLEGAL_TRANSITION,                 /* event 7 */
        ILLEGAL_TRANSITION,                 /* event 8 */
		  {RPC_C_CLIENT_CALL_CFDNE},            /* event 9 */
        ILLEGAL_TRANSITION,                 /* event 10 */
        ILLEGAL_TRANSITION                  /* event 11 */
    };

    /* state 7 - call_failed */
INTERNAL rpc_cn_sm_state_tbl_entry_t call_failed_state =
    {
		 {RPC_C_CLIENT_CALL_CALL_FAILED},      /* event 0 */
        ILLEGAL_TRANSITION,                 /* event 1 */
        ILLEGAL_TRANSITION,                 /* event 2 */
        ILLEGAL_TRANSITION,                 /* event 3 */
		  {RPC_C_CLIENT_CALL_CALL_FAILED}, 	    /* event 4 - local alert */
		  {RPC_C_CLIENT_CALL_CALL_FAILED},      /* event 5 - call_end */
        ILLEGAL_TRANSITION,                 /* event 6 */
        ILLEGAL_TRANSITION,                 /* event 7 */
        ILLEGAL_TRANSITION,                 /* event 8 */
		{RPC_C_CLIENT_CALL_CALL_FAILED},      /* event 9 */
        ILLEGAL_TRANSITION,                 /* event 10 */
        ILLEGAL_TRANSITION                  /* event 11 */
    };


GLOBAL rpc_cn_sm_state_entry_p_t rpc_g_cn_client_call_sm [] =
{
    init_state,                     /* state 0 - init */
    assoc_alloc_wait_state,         /* state 1 - assoc_alloc_wait */
    stub_wait_state,                /* state 2 - stub_wait */
    call_request_state,             /* state 3 - call_request */
    call_response_state,            /* state 4 - call_response */
    call_completed_state,           /* state 5 - call_completed */
    cfdne_state,                    /* state 6 - call failed, dne */
    call_failed_state               /* state 7 - call_failed */

};


/***********************************************************************/
/*
**
** C L I E N T   C A L L   P R E D I C A T E   R O U T I N E S
**
**/

/***********************************************************************/

/*
**++
**
**  ROUTINE NAME:       maybe_semantics_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Predicate routine invoked from the Call Active State.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The special event related parameter which is
**                      passed to the state machine event evaluation
**                      routine.
**                      This input argument is ignored.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if MaybeSemantics is false
**                      1 if MaybeSemantics is true
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 maybe_semantics_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED
)
{
    rpc_cn_packet_p_t   header_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT maybe_semantics_pred_rtn);

    /*
     *  check the protocol header (cached in the callrep) to see if
     *  PFC_MAYBE is set.
     */

    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (
        (rpc_cn_call_rep_p_t) spc_struct);
    if ((RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_MAYBE) == 0)
    {
        return (0);
    }
    else
    {
        return (1);
    }
}


/*
**++
**
**  MACRO NAME:		MAYBE_SEMANTICS_PRED        
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is a macro version of maybe_semantics_pred_rtn, introduced  for
**  performance reasons.  The macro lets us avoid overhead associated with
**  calling the predicate routine from within the action routine.
**  Predicate macro is invoked from the Call Active State.
**
**  INPUTS:
**
**      spc_struct      The association group. Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The special event related parameter which is
**                      passed to the state machine event evaluation
**                      routine.
**                      This input argument is ignored.
**
**	status		Instead of returning a value from the macro,
**			write the value calculated in the macro to
**			status.  Status' scope includes the routine
**			calling the macro.  Check status in the calling
**			routine to determine next state and in cases,
**			flow through the action routine. 
** 
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**	status		See explanation above.  
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if MaybeSemantics is false
**                      1 if MaybeSemantics is true
**
**  SIDE EFFECTS:       none
**
**--
**/
#define MAYBE_SEMANTICS_PRED(spc_struct, event_param, status)	\
{\
    RPC_CN_DBG_RTN_PRINTF(CLIENT maybe_semantics_pred_macro);\
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR(\
        (rpc_cn_call_rep_p_t) spc_struct);\
    if ((RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_MAYBE) == 0)\
    {\
        status = 0;\
    }\
    else\
    {\
    	status = 1;\
    }\
}



/*
**++
**
**  ROUTINE NAME:       last_recv_frag_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Predicate routine invoked from the Call Response state when an
**  RPCConf event occurs.
**
**  INPUTS:
**
**      spc_struct      The callrep.  This is passed as the 
**                      special structure which is passed to the
**                      state machine event evaluation routine.
**                      This argument is ignored.
**
**      event_param     The received packet contained in a fragment
**                      buffer.  This is passed in as the special 
**                      event related parameter by the state machine 
**                      event evaluation routine.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if LastRecvFrag is false
**                      1 if LastRecvFrag is true
**
**  SIDE EFFECTS:       none
**
**--
**/


INTERNAL unsigned8 last_recv_frag_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param
)
{
    rpc_cn_fragbuf_p_t      fragbuf;
    rpc_cn_packet_p_t       header_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT last_recv_frag_pred_rtn);
    fragbuf = (rpc_cn_fragbuf_p_t) event_param;

    /* 
     * The [unpacked] packet header starts off in the header_overhead 
     * area.
     */
    header_p = (rpc_cn_packet_p_t) fragbuf->data_p;

    if ((RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_LAST_FRAG) == 0)
    {
        return (0);
    }
    else
    {
        return (1);
    }

}

/***********************************************************************/
/*
 * C L I E N T   C A L L   A C T I O N   R O U T I N E S
 */
/***********************************************************************/


/*
**++
**
**  ROUTINE NAME:       allocate_assoc_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Action routine to allocate an association from the current
**  association group.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The if_spec_rep.  This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS: 
**
**	sm              The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     completion status, one of:
**                      rpc_s_ok,
**
**  SIDE EFFECTS:       Either alloc_assoc_ack or alloc_assoc_nak
**                      event would be appended to the state
**                      machine event evaluation list.
**
**--
**/
INTERNAL unsigned32     allocate_assoc_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm
)
{
    rpc_cn_assoc_p_t        assoc_p ATTRIBUTE_UNUSED;
    rpc_cn_call_rep_p_t     call_rep_p;
    rpc_cn_sm_event_entry_t event_entry;
    unsigned32              status;
    rpc_cn_sm_ctlblk_t 	    *sm_p;
 
    RPC_CN_DBG_RTN_PRINTF(CLIENT allocate_assoc_action_rtn);

    call_rep_p = (rpc_cn_call_rep_p_t) spc_struct;

    /* 
     * Allocate the association.  Pass in the binding rep,
     * and interface spec rep and get back an association, it
     * negotiated transfer syntax and its context id.
     */
    if ((call_rep_p->assoc = rpc__cn_assoc_request 
         (call_rep_p,
          (rpc_cn_binding_rep_t *) call_rep_p->binding_rep,
          (rpc_if_rep_t *) event_param, 
          &call_rep_p->transfer_syntax,
          &call_rep_p->context_id, 
          &call_rep_p->sec,
          &status)) != NULL)
    {
        call_rep_p->max_seg_size = RPC_CN_ASSOC_MAX_XMIT_FRAG (call_rep_p->assoc);
        rpc__cn_assoc_push_call (call_rep_p->assoc, call_rep_p, &status);
        event_entry.event_id = RPC_C_CALL_ALLOC_ASSOC_ACK;
        event_entry.event_param = (pointer_t) NULL;
    }
    else
    {
        event_entry.event_id = RPC_C_CALL_ALLOC_ASSOC_NAK;
        event_entry.event_param = (pointer_t) NULL;

        /* 
         * We will return the status returned by assoc_request.
         * This status will be returned by the eval routine since
         * we will invoke no action routine when we transtion to
         * cfdne state.
         */
    }

    /* 
     * Insert the new event on the event queue for our state
     * machine.
     */
    rpc__cn_sm_insert_event (&event_entry, &(call_rep_p->call_state));
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    sm_p->cur_state = RPC_C_CLIENT_CALL_ASSOC_ALLOC_WAIT;
    return (status); 
}


/*
**++
**
**  ROUTINE NAME:       transmit_req_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Action routine to send the call request PDU(s) to the server.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The iovector describing the data to be sent.
**                      This is passed in as the special event related
**                      parameter passed to the state machine event
**                      evaluator.
**
**  INPUTS/OUTPUTS:     
**
**	sm              The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok if the send was completed successfully.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     transmit_req_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm
)
{

    rpc_cn_call_rep_p_t     call_rep;
    rpc_iovector_p_t        stub_data_p;
    rpc_iovector_elt_p_t    iov_elt_p;
    unsigned8               event ATTRIBUTE_UNUSED;
    unsigned32              i;
    unsigned32              status;
#if 0
    boolean                 found_reusable;
#endif
    rpc_cn_sm_ctlblk_t	    *sm_p; 

    RPC_CN_DBG_RTN_PRINTF(CLIENT transmit_req_action_rtn);

    status = rpc_s_ok;

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    stub_data_p = (rpc_iovector_p_t) event_param;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    
    /*
     * We set call_executed to true at this point.
     * This is somewhat conservative; but it is correct.
     * We will reset call_executed if we get back a 
     * fault_dne.
     */
    call_rep->call_executed = true;

    /*
     * A call_transmit must have some stub data.  If this RPC
     * had no input arguments, then the first call should have
     * been a call_transceive with no stub data.
     */
#ifdef DEBUG
    if (stub_data_p->num_elt <= 0)
    {
        status = rpc_s_coding_error;
    }
    else
#endif
    {
        for (i = 0,
#if 0
             found_reusable = false,
#endif
             iov_elt_p = stub_data_p->elt;   /* first iovector element */
             i < stub_data_p->num_elt;
             i++, iov_elt_p++)
        {
            /*
             * If the data_len is 0, just deallocate the iovector
             * element.
             */
            if (iov_elt_p->data_len == 0)
            {
                if (iov_elt_p->buff_dealloc != NULL)
                {
                    (iov_elt_p->buff_dealloc) (iov_elt_p->buff_addr);
                }
            }
            else 
            {
                /*
                 * If the number of bytes < our bcopy_lim,
                 * copy the data and deallocate the buffer.
                 * copy_buffer will automatically transfer the
                 * data if the accumulated byte count reaches
                 * the segment size.
                 */
                if (iov_elt_p->data_len <= RPC_C_CN_BCOPY_LIM)
                {
                    rpc__cn_copy_buffer (call_rep, iov_elt_p, &status);
                    if (iov_elt_p->buff_dealloc != NULL)
                    {
                        (iov_elt_p->buff_dealloc) (iov_elt_p->buff_addr);
                    }
                }
                else 
                {
                    /*
                     * If the buffer must be made immediately reusable, copy
                     * it also.
                     * Note that this can be optimized later so that we won't
                     * copy; just transmit the data; if certain criteria have
                     * been met.
                     */
                    if (iov_elt_p->flags & rpc_c_iovector_elt_reused)
                    {
                        rpc__cn_copy_buffer (call_rep, iov_elt_p, &status);
                        if (status != rpc_s_ok)
                        {
                            goto done;
                        }
                    }
                    else
                    {
#if 0
                        if (iov_elt_p->flags & rpc_c_iovector_elt_reused)
                        {
                            found_reusable = true;
                        }
#endif
                        /*
                         * Don't copy, add this buffer as a new iovector
                         * element.
                         * add_new_vector_elmt will automatically transfer the
                         * data if the accumulated byte count reaches
                         * the segment size.
                         */
                        rpc__cn_add_new_iovector_elmt (call_rep, iov_elt_p, &status);
                        if (status != rpc_s_ok)
                        {
                            goto done;
                        }
                    }
                }
            }
        }
    }

#if 0
    /*
     * Finally, if there is any buffered data on the call rep flush
     * any data that we'd have to copy if possible.
     */
    if (found_reusable)
    {
        rpc__cn_flush_buffers (call_rep, &status);
    }
#endif

done:
;
    sm_p->cur_state = RPC_C_CLIENT_CALL_REQUEST;
    return (status);
}


/*
**++
**
**  ROUTINE NAME:       send_last_frag_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Action routine to send the last call request fragment to the server.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The iovector describing the data to be sent.
**                      This is passed in as the special event related
**                      parameter passed to the state machine event
**                      evaluator.
**                      This parameter can be null for a transceive
**                      with no input arguments.
**
**  INPUTS/OUTPUTS:     
**
**	sm              The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok if the send was completed successfully.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     send_last_frag_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm
)
{

    rpc_cn_call_rep_p_t     call_rep;
    rpc_iovector_p_t        stub_data_p;
    rpc_cn_packet_p_t       header_p;
    unsigned32              status;
    rpc_cn_sm_ctlblk_t	    *sm_p; 
    unsigned8		    n_state; 

    RPC_CN_DBG_RTN_PRINTF(CLIENT send_last_frag_action_rtn);

    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    status = rpc_s_ok;
    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    header_p = (rpc_cn_packet_p_t) (RPC_CN_CREP_SEND_HDR (call_rep));
   
    /* 
     * Status contains the result of the macro.
     */  
    MAYBE_SEMANTICS_PRED(spc_struct, event_param, status);
    if (status == 0)  /* MaybeSemantics is false */  
    {
   	n_state = RPC_C_CLIENT_CALL_RESPONSE;  
    }
    else  /* MaybeSemantics is true */ 
    {
        n_state = RPC_C_CLIENT_CALL_CALL_COMPLETED;
    }

    stub_data_p = (rpc_iovector_p_t) event_param;

    /*
     * If there's stub data, we can process it just like a normal
     * call request.  This might leave data buffered.
     *
     * Note that the absence of stub data is indicated by either
     * a null iovector pointer or an iovector with 0 elements.
     */
    if ((stub_data_p != NULL) && (stub_data_p->num_elt > 0))
    {
   	/* 
	 * Note that since we are calling action routines from
	 * within action routines, we need to update state as
	 * a final step here.  Otherwise, the action routines
	 * would update sm->cur_state inappropriately for
	 * the calling routine.
	 */   
        status = 
		transmit_req_action_rtn (spc_struct, event_param, sm);
        if (status != rpc_s_ok)
        {
		sm_p->cur_state = n_state; 
		return (status);    
        }
    }

    /*
     * Set the last frag flag bit in the cached protocol header
     * and send it along with any buffered data.
     */
    RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_LAST_FRAG;
    if (RPC_CN_CREP_ACC_BYTCNT (call_rep) >= RPC_CN_CREP_SIZEOF_HDR (call_rep))
    {
        rpc__cn_transmit_buffers (call_rep, &status);
        rpc__cn_dealloc_buffered_data (call_rep);

        /*
         * Set the length of the iov to 1.  We don't use the general
         * FREE_ALL_EXCEPT_PROT_HEADER macro since we won't be
         * using the iov again.
         */
        RPC_CN_CREP_IOVLEN (call_rep) = 1;
        if (status != rpc_s_ok)
        {
		sm_p->cur_state = n_state; 
		call_rep->assoc->assoc_status = status;
		return (status);    
        }
    }
    else
    {
        /*
         * If the accumulated bytecount field is less than at
         * least that of the request header, something is really
         * off.
         */
	/*
	 * rpc_m_invalid_accbytcnt
	 * "(%s) Inconsistency in ACC_BYTCNT field"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	    rpc_svc_cn_errors,
	    svc_c_sev_fatal | svc_c_action_abort,
	    rpc_m_invalid_accbytcnt,
	    "send_last_frag_action_rtn" ));
    }

    sm_p->cur_state = n_state; 
    return (rpc_s_ok);    

}


/*
**++
**
**  ROUTINE NAME:       handle_recv_frag_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Action routine to make the (received) fragment data available
**  to the stub for unmarshalling.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragment buffer containing the response
**                      message.  This is passed in as the special 
**                      event related parameter which was passed to
**                      the state machine event evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm              The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok
**
**  SIDE EFFECTS:       Either a fault or fault_dne event may be
**                      appended to the state machine event list.
**
**--
**/
INTERNAL unsigned32     handle_recv_frag_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm
)
{
    unsigned32              status ATTRIBUTE_UNUSED;
    rpc_cn_fragbuf_p_t      fragbuf;
    rpc_cn_packet_p_t       header_p;
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_sm_ctlblk_t	    *sm_p;
    unsigned8		    n_state = 0;  
    
    RPC_CN_DBG_RTN_PRINTF(CLIENT handle_recv_frag_action_rtn);
    
    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    fragbuf = (rpc_cn_fragbuf_p_t) event_param;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    header_p = (rpc_cn_packet_p_t) fragbuf->data_p;

    /*
     * We've got a proper response.  Adjust data_size to describe 
     * the stub data.
     * Note that we do not need to adjust data_p since that will
     * be done by rpc__cn_call_receive. 
     */
    fragbuf->data_size = RPC_CN_PKT_FRAG_LEN (header_p) -
                         RPC_CN_PKT_AUTH_TLR_LEN (header_p) -
                         RPC_CN_PKT_SIZEOF_RESP_HDR;


    /*
     * Determine whether this is the last response fragment.
     */
    if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_LAST_FRAG)
    {
        
        /*
         * The predicate associated with this routine, 
	 * last_recv_frag_pred_rtn, checks the same
	 * flags checked above in the if statement.
	 * If RPC_CN_PKT_FLAGS & rpc_c_cn_flags_lastfrag
         * are 0, then set state to rpc_c_client_call
	 * response, else set state to rpc_c_client_call_
         * call_complete.
         */ 
        n_state = RPC_C_CLIENT_CALL_CALL_COMPLETED;
        /*
         * If there is a timer running stop it since we've heard from
         * the server.
         */
        rpc__cn_call_stop_cancel_timer (call_rep);
        
        /*
         * Record whether the server finished with a pending alert. Note
         * that the alert count in the packet does not include the
         * alert forwarded by setting the PFC_PENDING_ALERT bit in the
         * first fragment of the request.
         */
        if ((call_rep->u.client.cancel.server_count > RPC_CN_PKT_ALERT_COUNT (header_p)) ||
            (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_ALERT_PENDING))
        {
            /*
             * Either the number of alerts forwarded by us is
             * greater than the number of alerts posted to the call
             * executor thread on the server OR the there was still an alert
             * pending in the call executor thread when the server
             * stub returned. In either case set the
             * server_had_pending flag in the call_rep to indicate an
             * alert should be posted to the client caller thread
             * before returning to the client stub.
             */
            call_rep->u.client.cancel.server_had_pending = true;
#ifdef DEBUG
            if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_ALERT_PENDING)
            {
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                                ("(handle_recv_frag_action_rtn) call_rep->%x alert pending flag is set in header\n", call_rep));
            }
            else
            {
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                ("(handle_recv_frag_action_rtn) call_rep->%x number alerts forwarded (%d) > alert count in header (%d)\n", 
                 call_rep,
                 call_rep->u.client.cancel.server_count,
                 RPC_CN_PKT_ALERT_COUNT (header_p)));
            }
#endif
        }
    }
    else
    {
	n_state = RPC_C_CLIENT_CALL_RESPONSE; 
    } 
    /*
     * We are currently executing in the receiver thread.
     *
     * If there is stub data, queue it on the association so that
     * the client call thread can get it.
     * If there is no stub data (e.g., no out arguments), just
     * deallocate the fragment buffer.
     * We make an exception for the first fragment; it is always
     * queued since the client call thread may already be blocked
     * on the condition variable.
     */
    if (fragbuf->data_size ||
        (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_FIRST_FRAG))
    {
        rpc__cn_assoc_queue_frag (call_rep->assoc, fragbuf, true);
    }
    else
    {
        (* fragbuf->fragbuf_dealloc) (fragbuf);
    }

    /*
     * Increment num_pkts in the call rep. This will be used in
     * determining when to check for pending cancels.
     */
    call_rep->num_pkts++;
    
    sm_p->cur_state = n_state;
    return (rpc_s_ok);
}



/*
**++
**
**  ROUTINE NAME:       raise_fault_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Action routine to deallocate the current association and raise
**  fault.  Operationally, this will store the address of the 
**  fragment buffer in the callrep (for later retrieval via the
**  rpc__receive_fault).
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fault packet.  This is passed in as the
**                      event specific structure.
** 
**  INPUTS/OUTPUTS:     
**
**	sm              The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_call_faulted
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     raise_fault_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm
)
{
    unsigned32              status ATTRIBUTE_UNUSED;
    rpc_cn_fragbuf_p_t      fragbuf;
    rpc_cn_packet_p_t       header_p;
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_sm_ctlblk_t	    *sm_p; 

    RPC_CN_DBG_RTN_PRINTF(CLIENT raise_fault_action_rtn);

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    fragbuf = (rpc_cn_fragbuf_p_t) event_param;
    header_p = (rpc_cn_packet_p_t) fragbuf->data_p;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * We've got a proper response.  Adjust data_size to describe 
     * the stub data.
     * Note that we do not need to adjust data_p since that will
     * be done by rpc__cn_call_receive.
     */
    fragbuf->data_size = RPC_CN_PKT_FRAG_LEN (header_p) -
                         RPC_CN_PKT_AUTH_TLR_LEN (header_p) -
                         RPC_CN_PKT_SIZEOF_FAULT_HDR;

    /*
     * If there is a timer running stop it since we've heard from
     * the server.
     */
    rpc__cn_call_stop_cancel_timer (call_rep);

    /*
     * Determine whether this is the last fault fragment.
     */
    if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_LAST_FRAG)
    {
        /*
         * Record whether the server finished with a pending alert. Note
         * that the alert count in the packet does not include the
         * alert forwarded by setting the PFC_PENDING_ALERT bit in the
         * first fragment of the request.
         */
        if ((call_rep->u.client.cancel.server_count > RPC_CN_PKT_ALERT_COUNT (header_p)) ||
            (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_ALERT_PENDING))
        {
            /*
             * Either the number of alerts forwarded by us is
             * greater than the number of alerts posted to the call
             * executor thread on the server OR the there was still an alert
             * pending in the call executor thread when the server
             * stub returned. In either case set the
             * server_had_pending flag in the call_rep to indicate an
             * alert should be posted to the client caller thread
             * before returning to the client stub.
             */
            call_rep->u.client.cancel.server_had_pending = true;
#ifdef DEBUG
            if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_ALERT_PENDING)
            {
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                               ("(raise_fault_action_rtn) call_rep->%x alert pending flag is set in header\n", call_rep));
            }
            else
            {
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                               ("(raise_fault_action_rtn) call_rep->%x number alerts forwarded (%d) > alert count in header (%d)\n",
                                call_rep,
                                call_rep->u.client.cancel.server_count,
                                RPC_CN_PKT_ALERT_COUNT (header_p)));
            }
#endif
        }
    }

    /*
     * We are currently executing in the receiver thread.
     *
     * If there is stub data, queue it on the association so that
     * the client call thread can get it.
     * If there is no stub data (e.g., no out arguments), just
     * deallocate the fragment buffer.
     * We make an exception for the first fragment; it is always
     * queued since the client call thread may already be blocked
     * on the condition variable.
     */
    if (fragbuf->data_size ||
        (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_FIRST_FRAG))
    {
        rpc__cn_assoc_queue_frag (call_rep->assoc, fragbuf, true);
    }
    else
    {
        (* fragbuf->fragbuf_dealloc) (fragbuf);
    }

    /* 
     * There is no predicate associated with this routine but the
     * new value of sm->cur_state is determined by the value of
     * sm->cur_event coming into the routine.  Note that 
     * 2+statebase is event fault_dns;  3+statebase is event
     * fault.
     */  
    if (sm_p->cur_event == (2 + RPC_C_CN_STATEBASE )) 	
    	sm_p->cur_state =  RPC_C_CLIENT_CALL_CFDNE;
    else if (sm_p->cur_event == (3 + RPC_C_CN_STATEBASE ))	
	sm_p->cur_state = RPC_C_CLIENT_CALL_CALL_FAILED; 
    return (rpc_s_ok);
}


/*
**++
**
**  ROUTINE NAME:       forward_alert_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Action routine to forward an alert. The first alert that is
**  forwarded will start the alert timer. This timer will run until
**  either the call is completed or a reponse is received from the
**  server. If the timer expires the call is orphaned. The alert
**  timer setting, clearing and expiration handling is all done outside
**  the state machine action routines, primarily in the
**  rpc__cn_call_forward_cancel, rpc__cn_call_[start,stop]_cancel_timer
**  and rpc__cn_call_cancel_timer. All cancellable operations made in
**  the CN runtime are encompassed in cancel exception handlers.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The error_status to return. This is passed in 
**                      as the special event related parameter which 
**                      was passed to the state machine event 
**                      evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm              The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_call_faulted
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     forward_alert_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm
)
{
    rpc_cn_call_rep_p_t         call_rep;
    rpc_cn_packet_p_t           header_p;
    struct
    {
        rpc_iovector_t          iov;
        rpc_iovector_elt_t      elt_1;
    } pdu;
    unsigned32                  status;
    unsigned8                   prev_ptype;
    rpc_cn_sm_ctlblk_t		*sm_p; 

    RPC_CN_DBG_RTN_PRINTF(CLIENT forward_alert_action_rtn);

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);

    /*
     * The remote alert indication packet uses only the
     * common fields of the header.  We can therefore just
     * use the current header.
     */
    prev_ptype = RPC_CN_PKT_PTYPE (header_p);
    RPC_CN_PKT_PTYPE (header_p) = RPC_C_CN_PKT_REMOTE_ALERT;
    RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_ALERT_PENDING;

    /*
     * If security was requested on this call an authentication
     * trailer will have to be added to the alert PDU.
     */
    if (call_rep->sec == NULL)
    {
        RPC_CN_PKT_FRAG_LEN (header_p) = RPC_CN_PKT_SIZEOF_ALERT_HDR;
        pdu.iov.num_elt = 1;
    }
    else
    {
        RPC_CN_PKT_FRAG_LEN (header_p) = 
            RPC_CN_PKT_SIZEOF_ALERT_HDR + 
            call_rep->prot_tlr->data_size - 
            RPC_CN_CREP_SIZEOF_TLR_PAD (call_rep);
        pdu.iov.num_elt = 2;
        pdu.elt_1.buff_dealloc = NULL;
        pdu.elt_1.data_addr = (byte_p_t) call_rep->prot_tlr->data_p;
        pdu.elt_1.data_len = 
            call_rep->prot_tlr->data_size - 
            RPC_CN_CREP_SIZEOF_TLR_PAD (call_rep);
    }
    
    /* 
     * Send the packet over.
     */
    pdu.iov.elt[0].buff_dealloc = NULL;
    pdu.iov.elt[0].data_addr = (byte_p_t) header_p;
    pdu.iov.elt[0].data_len = RPC_CN_PKT_SIZEOF_ALERT_HDR;
    rpc__cn_assoc_send_frag (call_rep->assoc, &pdu.iov, call_rep->sec, &status);
    
    /*
     * Restore the previous packet type.
     */
    RPC_CN_PKT_PTYPE (header_p) = prev_ptype;

    /*
     * Increment the count of forwarded cancels.
     */
    call_rep->u.client.cancel.server_count++;
    RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                ("(forward_alert_action_rtn) call_rep->%x forwarding cancel total so far = %d\n", 
                 call_rep,
                 call_rep->u.client.cancel.server_count));

    /* 
     * There is no predicate associated with this routine but the
     * new value of sm->cur_state is determined by the value of
     * sm->cur_state  coming into the routine.  In otherwords,
     * this action routine is called from 2 different states
     * and the value of that state determines the new value
     * for sm->cur_state.  Note that 3+statebase is call_request;  
     * 4+statebase is call_response.  rpc_c_cn_statebase is
     * set to 100 to distinguish it from action routine
     * indexes used in the rpc__cn_sm_event_eval() routine. 
     */  
    if (sm_p->cur_state == (3 + RPC_C_CN_STATEBASE))	
	sm_p->cur_state = RPC_C_CLIENT_CALL_REQUEST;
    else if (sm_p->cur_state == (4 + RPC_C_CN_STATEBASE))	
	sm_p->cur_state = RPC_C_CLIENT_CALL_RESPONSE; 
    return (status);
}


/*
**++
**
**  ROUTINE NAME:       abort_send_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Action routine to abort a send.
**  It sends an orphaned message, and then raises a fault by 
**  returning the error status back to the caller.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     This parameter is ignored.  It is passed in 
**                      as the special event related parameter which 
**                      was passed to the state machine event 
**                      evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm              The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
**
**  OUTPUTS:            rpc_s_call_faulted
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     error_status reflecting the fault
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     abort_send_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm
)
{
    rpc_cn_call_rep_p_t         call_rep;
    rpc_cn_packet_p_t           header_p;
    unsigned32                  status;
    rpc_cn_sm_ctlblk_t		*sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT abort_send_action_rtn);

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * The call is going to be orphaned. The stub data bufferred
     * on the call rep will not be sent and can be released.
     */
    rpc__cn_dealloc_buffered_data (call_rep);
    RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);

    /*
     * Now prepare to send an orphaned packet to the server.
     */
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR(call_rep);
    RPC_CN_PKT_PTYPE (header_p) = RPC_C_CN_PKT_ORPHANED;
    RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_LAST_FRAG;
    RPC_DBG_PRINTF (rpc_e_dbg_orphan, RPC_C_CN_DBG_ORPHAN,
                    ("(abort_send_action_rtn) call_rep->%x sending orphan packet ... call id = %x\n", 
                     call_rep,
                     RPC_CN_PKT_CALL_ID (header_p)));

    /*
     * If security was requested on this call an authentication
     * trailer will have to be added to the orphan PDU.
     */
    if (call_rep->sec == NULL)
    {
        RPC_CN_PKT_FRAG_LEN (header_p) = RPC_CN_PKT_SIZEOF_ORPHANED_HDR;
        RPC_CN_CREP_IOVLEN (call_rep) = 1;
    }
    else
    {
        RPC_CN_PKT_FRAG_LEN (header_p) =
            RPC_CN_PKT_SIZEOF_ORPHANED_HDR +
            call_rep->prot_tlr->data_size - 
            RPC_CN_CREP_SIZEOF_TLR_PAD (call_rep);
        RPC_CN_CREP_IOVLEN (call_rep) = 2;
        RPC_CN_CREP_IOV (call_rep)[1].data_addr = (byte_p_t) call_rep->prot_tlr->data_p;
        RPC_CN_CREP_IOV (call_rep)[1].data_len =
            call_rep->prot_tlr->data_size - 
            RPC_CN_CREP_SIZEOF_TLR_PAD (call_rep);
        RPC_CN_CREP_IOV (call_rep)[1].buff_dealloc = NULL;
    }

    /* 
     * Send the packet over.  Note that the returned status is
     * ignored.
     */
    RPC_CN_CREP_IOV (call_rep)[0].data_addr = (byte_p_t) header_p;
    RPC_CN_CREP_IOV (call_rep)[0].data_len = RPC_CN_PKT_SIZEOF_ORPHANED_HDR;
    rpc__cn_assoc_send_frag (call_rep->assoc, 
                             &(call_rep->buffered_output.iov), 
                             call_rep->sec,
                             &status);
    /*
     * Now return to the caller (presumably rpc__cn_call_end) which
     * will deallocate the association on our end and clean up.
     */
    sm_p->cur_state = RPC_C_CLIENT_CALL_CALL_FAILED;
    return (rpc_s_ok);
}

/*
**++
**
**  ROUTINE NAME:       abort_recv_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Action routine to abort a receive.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fault data.
**                      This is passed in as the special event related
**                      parameter which was passed to the state machine 
**                      event evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm              The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     completion status
**                          rpc_s_call_faulted
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     abort_recv_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm
)
{
    unsigned32      status;

    RPC_CN_DBG_RTN_PRINTF(CLIENT abort_recv_action_rtn);
    /* 
     * Note that we are getting state from raise_fault_action_rtn().  Also
     * note that it does not seem that we are actually using abort_recv_
     * action_rtn in the state tables.
     *   
     * Note, we don't need to chase down our receiver thread.
     * We will shortly deallocate the association.  The receiver
     * thread will automatically discard fragments for non-existent
     * associations.
     *
     * We make this a separate action routine (instead of using
     * raise_fault_action_rtn) to leave room for future optimizations.
     */

    /*
     * Abort the association.
     */
    rpc__cn_assoc_abort (((rpc_cn_call_rep_p_t) spc_struct)->assoc, &status);

    /* 
     * Raise the fault.
     */
    return (raise_fault_action_rtn (spc_struct, event_param, sm ));
}

