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
**      cnsclsm.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Server call State Machine for the Connection-based RPC runtime.
**
**
*/


#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <ndrglob.h>    /* NDR representation global declarations */
#include <ndrp.h>       /* System dependent NDR decls */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnfbuf.h>     /* NCA Connection fragment buffer declarations */
#include <cnpkt.h>      /* NCA Connection protocol header */
#include <cnsm.h>       /* NCA Connection state machine declarations */
#include <cnxfer.h>     /* NCA Connection buffered data transfer */
#include <cnassoc.h>    /* NCA Connection association services */
#include <comcthd.h>    /* Externals for Call Thread sub-component  */
#include <cncall.h>     /* NCA connection call service */
#include <cnclsm.h>


/******************************************************************************/
/*
 * Global Definitions
 */
#ifdef DEBUG
GLOBAL char     *rpc_g_cn_call_server_events [] =
{
    "RESPONSE         ",
    "INDICATION       ",
    "FAULT_DNE        ",
    "FAULT            ",
    "LOCAL_ALERT      ",
    "END              ",
    "REMOTE_ALERT_IND ",
    "ORPHANED         "
};

GLOBAL char     *rpc_g_cn_call_server_states [] =
{
    "INIT             ",
    "REQUEST          ",
    "RESPONSE         ",
    "CALL_COMPLETED   "
};
#endif


/***********************************************************************/
/*
 * S E R V E R   C A L L   P R E D I C A T E   T A B L E
 */

/*
 * The predicates.  
 * As a performance enhancement, we have changed some predicate routines 
 * and made them macros and we have absorbed the predicates into the 
 * actions.  Thus, there is no longer a need for the predicate table.  
 * We invoke the predicate functions as MACROS or as function calls 
 * from within the action routines.
 */
/*
 * The predicate routine prototypes.
 */
INTERNAL unsigned8 disconnected_maybe_pred_rtn 
    (
	pointer_t spc_struct, 
        pointer_t event_param
    ) ATTRIBUTE_UNUSED;
INTERNAL unsigned8 request_fault_pred_rtn 
    (
        pointer_t spc_struct, 
        pointer_t event_param
    ) ATTRIBUTE_UNUSED;
INTERNAL unsigned8 response_fault_pred_rtn 
    (
        pointer_t spc_struct, 
        pointer_t event_param
    ) ATTRIBUTE_UNUSED;
INTERNAL unsigned8 last_recv_frag_pred_rtn 
    (
        pointer_t spc_struct, 
        pointer_t event_param
    ) ATTRIBUTE_UNUSED;
INTERNAL unsigned8 first_frag_pred_rtn
    (
        pointer_t spc_struct, 
        pointer_t event_param
    ) ATTRIBUTE_UNUSED;
INTERNAL unsigned8 disc_last_send_pred_rtn
    (
        pointer_t spc_struct,
        pointer_t event_param
    ) ATTRIBUTE_UNUSED;



/***********************************************************************/
/*
 * S E R V E R   C A L L   A C T I O N   T A B L E
 */

/***********************************************************************/

/*
 * The actions.
 */
#define HANDLE_FIRST_FRAG         0
#define HANDLE_FRAG               1
#define SEND_CALL_RESP            2
#define SEND_CALL_FAULT           3
#define PROCESS_ALERT_MSG         4
#define ABORT_RESP                5
#define ABORT_RESP_SEND_FAULT     6
#define STOP_ORPHAN               7
#define DISCARD_FRAGMENT          8
#define CALL_END                  9
#define PROTOCOL_ERROR           10

/*
 * We do not currently execute actions when we abort a receive.
 */
#define ABORT_RECEIVE             RPC_C_SM_NO_ACTION
#define ABORT_RECEIVE_SEND_FAULT  SEND_CALL_FAULT


/*
 * The action routine prototypes.
 */
INTERNAL unsigned32     handle_first_frag_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );
INTERNAL unsigned32     handle_frag_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );
INTERNAL unsigned32     send_call_resp_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );
INTERNAL unsigned32     send_call_fault_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );
INTERNAL unsigned32     process_alert_msg_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );

INTERNAL unsigned32     abort_resp_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );

INTERNAL unsigned32     abort_resp_send_fault_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );
INTERNAL unsigned32     stop_orphan_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );
INTERNAL unsigned32     discard_fragment_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );
INTERNAL unsigned32     call_end_action_rtn (
        pointer_t  /*spc_struct*/, 
        pointer_t /*event_param*/,
	pointer_t /*sm*/
    );

/*
 * The action table itself.
 */
GLOBAL rpc_cn_sm_action_fn_t  rpc_g_cn_server_call_action_tbl [] =
{
    handle_first_frag_action_rtn,
    handle_frag_action_rtn,
    send_call_resp_action_rtn,
    send_call_fault_action_rtn,
    process_alert_msg_action_rtn,
    abort_resp_action_rtn,
    abort_resp_send_fault_action_rtn,
    stop_orphan_action_rtn,
    discard_fragment_action_rtn,
    call_end_action_rtn,
    rpc__cn_call_sm_protocol_error 
};

/***********************************************************************/
/*
 * S E R V E R   C A L L   S T A T E   T A B L E
 */

/*
 * A state table entry exists for every state in the state machine.
 * In the case of the server call state machine, for example, a
 * state table entry exists for init, call_request, call_response,
 * etc.
 *
 * Each state table entry describes the actions and state transitions
 * which can occur in response to each event while in that state.
 * So for example, the state table entry for the init state must
 * describe the outcomes for every event.
 *
 * The outcome for an event is a new state and an action routine to
 * invoke.  
 *
 * Sometimes, how a given event is handled depends upon certain
 * conditions.  For example, a fault_dne event occurring in the 
 * call_response state is handled differently depending upon whether
 * the connection has been disconnected and the semantics of the
 * call.  This is translated into a number of different outcomes
 * for each event.
 *
 * As an optimization, the structure rpc_cn_sm_state_tbl_entry_t
 * was modified to contain one unsigned8 value.  That value can
 * be either a state to transition to, or the action index.
 * We distinguish between states and action indexes by value;
 * state values are >=100 and action indexes are in the range
 * of 1 - 14. If the state and event combination in the state table
 * requires an action, then the rpc_cn_sm_state_tbl_entry_t structure
 * contains an index into the action tbl for that set of 
 * circumstances.  The action routine itself will contain a
 * predicate function, usually inline, and the action routine will
 * update the control block's cur_state which in effect, is the
 * next state.   The predicate function within the action routine
 * will guide the logic through the action so that the appropriate
 * action is taken depending on the outcome of the predicate 
 * function.  If the state and event combination requires no action but does
 * require a state update, then we look to rpc_cn_sm_state_tbl_entry_t
 * to contain that state update value. 
 * 
 */

INTERNAL rpc_cn_sm_state_tbl_entry_t init_state =

    {
        ILLEGAL_TRANSITION,                 /* event 0 - rpc_resp */
		  {HANDLE_FIRST_FRAG},		    /* event 1 - rpc_ind */  
        ILLEGAL_TRANSITION,                 /* event 2 - fault_dne */
        ILLEGAL_TRANSITION,                 /* event 3 - fault */
        ILLEGAL_TRANSITION,                 /* event 4 - local_alert */
		  {RPC_C_SERVER_CALL_CALL_COMPLETED},   /* event 5 - call_end state */
        ILLEGAL_TRANSITION,                 /* event 6 - remote_alert_ind */
        ILLEGAL_TRANSITION                  /* event 7 - orphaned */
    };

INTERNAL rpc_cn_sm_state_tbl_entry_t call_request_state =
    {
        ILLEGAL_TRANSITION,                 /* event 0 - rpc_resp */
		  {HANDLE_FRAG},	 		    /* event 1 - rpc_ind */
		  {ABORT_RECEIVE_SEND_FAULT}, 	    /* event 2 - fault_dne */
		  {ABORT_RECEIVE_SEND_FAULT},           /* event 3 - fault */
        ILLEGAL_TRANSITION,                 /* event 4 - local_alert */
		  {CALL_END},                           /* event 5 - call_end */
		  {PROCESS_ALERT_MSG},  		    /* event 6 - remote_alert_ind */
        /*
         * This transition should not be going to the call
         * completed state when the call is still executing.
         * However, this implementation will return an error to any
         * attempts to send a reponse on a call which is in the
         * call_completed state before entering the state machine.
         */
		  {STOP_ORPHAN}  	                    /* event 7 - orphaned */
    };

INTERNAL rpc_cn_sm_state_tbl_entry_t call_response_state =
    {
		 {SEND_CALL_RESP},                    /* event 0 - rpc_resp */
        ILLEGAL_TRANSITION,                /* event 1 - rpc_ind */
		  {ABORT_RESP_SEND_FAULT},             /* event 2 - fault_dne */
		  {ABORT_RESP_SEND_FAULT},             /* event 3 - fault */
        ILLEGAL_TRANSITION,                /* event 4 - local_alert */
		  {CALL_END}, 		           /* event 5 - call_end */
		  {PROCESS_ALERT_MSG},  	           /* event 6 - remote_alert_ind */
		  {STOP_ORPHAN} 		           /* event 7 - orphaned */
    };


INTERNAL rpc_cn_sm_state_tbl_entry_t call_completed_state =
    {
        ILLEGAL_TRANSITION,                 /* event 0 */
        ILLEGAL_TRANSITION,                 /* event 1 */
        ILLEGAL_TRANSITION,                 /* event 2 */
        ILLEGAL_TRANSITION,                 /* event 3 */
        ILLEGAL_TRANSITION,                 /* event 4 */
		  {RPC_C_SERVER_CALL_CALL_COMPLETED},   /* event 5 - call_end */
        ILLEGAL_TRANSITION,                 /* event 6 */
		  {RPC_C_SERVER_CALL_CALL_COMPLETED}    /* event 7 - orphaned */
    };


GLOBAL rpc_cn_sm_state_entry_p_t rpc_g_cn_server_call_sm [] =
{
    init_state,                     /* state 0 - init */
    call_request_state,             /* state 1 - call_request */
    call_response_state,            /* state 2 - call_response */
    call_completed_state            /* state 3 - call_completed */
};


/***********************************************************************/
/*
 *
 * S E R V E R   C A L L   P R E D I C A T E   R O U T I N E S
 *
 */

/***********************************************************************/


/*
**++
**
**  ROUTINE NAME:       disconnected_maybe_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Predicate routine to evaluate whether the communications link
**  has been disconnected or if the call has maybe semantics.
**  This routine is invoked from the Call Request and Call Response
**  states when an RPCInd or RPCResp event occurred.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     Fragment buffer containing the received data.
**                      Note that it is passed in as the special event 
**                      related parameter which is passed to the state 
**                      machine event evaluation routine.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    the association status has been set to non-OK if the 
**                      connection has been disconnected.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if Disconnected and MaybeSemantics are both false
**                      1 if either Disconnected or MaybeSemantics is true
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 disconnected_maybe_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param
)
{
    RPC_CN_DBG_RTN_PRINTF(SERVER disconnected_maybe_pred_rtn);

    if ( (((rpc_cn_call_rep_p_t) spc_struct)->assoc->assoc_status != 
          rpc_s_ok) ||
         (RPC_CN_PKT_FLAGS (RPC_CN_FRAGBUF_PKT_HDR (event_param)) & 
          RPC_C_CN_FLAGS_MAYBE) )
    {
        return (1);
    }
    else
    {
        return (0);
    }
}


/*
**++
**
**  ROUTINE NAME:       disc_last_send_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Predicate routine which evaluates whether the communications link
**  has been disconnected and whether this the the last fragment
**  of the response.  It is invoked from the Call Response state when 
**  an RPCResp event occurred.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     Iovector element containing the stub data for
**                      the response.
**                      Note that it is passed in as the special event 
**                      related parameter which is passed to the state 
**                      machine event evaluation routine.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    the association status has been set to non-OK if the 
**                      connection has been disconnected.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if Disconnected is true
**                      1 if Disconnected is false and this is the
**                        last fragment of the response.
**                        (NOTE that this value cannot in fact be
**                         returned since we can only detect
**                         the last fragment when the CALL_END
**                         is issued.)
**                      2 if Disconnected is false and this is not
**                        the last fragment of the response.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 disc_last_send_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED
)
{
    RPC_CN_DBG_RTN_PRINTF(SERVER disc_last_send_pred_rtn);

    if  (((rpc_cn_call_rep_p_t) spc_struct)->assoc->assoc_status != 
          rpc_s_ok)
    {
        return (0);
    }
    else
    {
        return (2);
    }

    /*
     * Note, our current implementation can never detect the
     * last fragment of the response until the CALL_END is
     * issued.  Consequently, 1 cannot be returned.
     */
}


/*
**++
**
**  MACRO NAME:		DISC_LAST_SEND_PRED        
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is a macro version of the disc_last_send_pred_rtn() predicate.
**  We added the macro version to avoid overhead when calling the
**  predicate function from within the action routines.
**  Predicate macro which evaluates whether the communications link
**  has been disconnected and whether this the the last fragment
**  of the response.  It is invoked from the Call Response state when 
**  an RPCResp event occurred.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     Iovector element containing the stub data for
**                      the response.
**                      Note that it is passed in as the special event 
**                      related parameter which is passed to the state 
**                      machine event evaluation routine.
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
**  FUNCTION VALUE:     0 if Disconnected is true
**                      1 if Disconnected is false and this is the
**                        last fragment of the response.
**                        (NOTE that this value cannot in fact be
**                         returned since we can only detect
**                         the last fragment when the call_end
**                         is issued.)
**                      2 if Disconnected is false and this is not
**                        the last fragment of the response.
**
**  SIDE EFFECTS:      none  
**
**--
**/
#define  DISC_LAST_SEND_PRED(spc_struct, event_param, status) 	\
{									\
    RPC_CN_DBG_RTN_PRINTF(SERVER disc_last_send_pred_macro); 	\
    if (((rpc_cn_call_rep_p_t) spc_struct)->assoc->assoc_status !=  rpc_s_ok)	\
    { 									\
   	status = 0; 							\
    } 									\
    else 								\
    { 									\
      	status = 2; 							\
    } 									\
}


/*
**++
**
**  ROUTINE NAME:       request_fault_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Routine evaluating the combination of the predicates: MaybeSemantics, 
**  Disconnected, and LastSendFrag for the fault.  It is invoked from 
**  the Call Request state when either a fault or fault_dne event occurs.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     Iovector element containing the fault data.
**                      Note that it is passed in as the special event 
**                      related parameter which is passed to the state 
**                      machine event evaluation routine.
**                      This argument is ignored in the current
**                      implementation.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    the association status has been set to non-OK if the 
**                      connection has been disconnected.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if MaybeSemantics or Disconnected is true
**                      1 if LastSendFrag for the fault is true and
**                        and both MaybeSemantics and Disconnected
**                        are false
**                      2 if LastSendFrag for the fault is false,
**                        and both MaybeSemantics and Disconnected
**                        are false.
**                        Note that in our current implementation,
**                        this outcome cannot occur.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 request_fault_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED
)
{
    rpc_cn_call_rep_p_t call_rep;

    RPC_CN_DBG_RTN_PRINTF(SERVER request_fault_pred_rtn);

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;

    if ( ( call_rep->assoc->assoc_status !=
           rpc_s_ok ) ||
         ( (RPC_CN_PKT_FLAGS ( (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep)))
           & RPC_C_CN_FLAGS_MAYBE ) )
    {
        return (0);
    }
    else
    {
        /*
         * Note: Although the architecture allows the fault data
         * to be sent in several chunks, the current implementation
         * assumes that it is passed to the runtime in a single
         * operation.
         */
        return (1);
    }
}


/*
**++
**
**  MACRO NAME:		REQUEST_FAULT_PRED        
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is a macro version of the request_fault_pred_rtn() predicate.
**  We added the macro version to avoid overhead when calling the
**  predicate function from within the action routines.
**  Macro evaluating the combination of the predicates: MaybeSemantics, 
**  Disconnected, and LastSendFrag for the fault.  It is invoked from 
**  the Call Request state when either a fault or fault_dne event occurs.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     Iovector element containing the fault data.
**                      Note that it is passed in as the special event 
**                      related parameter which is passed to the state 
**                      machine event evaluation routine.
**                      This argument is ignored in the current
**                      implementation.
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
**  FUNCTION VALUE:     0 if MaybeSemantics or Disconnected is true
**                      1 if LastSendFrag for the fault is true and
**                        and both MaybeSemantics and Disconnected
**                        are false
**                      2 if LastSendFrag for the fault is false,
**                        and both MaybeSemantics and Disconnected
**                        are false.
**                        Note that in our current implementation,
**                        this outcome cannot occur.
**
**  SIDE EFFECTS:       none
**
**--
**/
#define REQUEST_FAULT_PRED(call_rep, event_param, status)		\
{									\
    RPC_CN_DBG_RTN_PRINTF(SERVER request_fault_pred_macro);		\
    if ( ( call_rep->assoc->assoc_status !=				\
           rpc_s_ok ) ||						\
         ( (RPC_CN_PKT_FLAGS ( (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep)))	\
           & RPC_C_CN_FLAGS_MAYBE ) )					\
    {									\
        status = 0;							\
    }									\
    else								\
    {									\
        status = 1;							\
    }									\
}



/*
**++
**
**  ROUTINE NAME:       response_fault_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Evaluate the MaybeSemantics and Disconnected predicates from
**  the Call Response state when either a fault or fault_dne 
**  event occurs.
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
**  IMPLICIT INPUTS:    the association status has been set to non-OK if the 
**                      connection has been disconnected.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if MaybeSemantics true,
**                      1 if Disconnected is true,
**                      2 otherwise.
**
**       NOTE that the architecture allows 4 possible return values
**            so that fault data can be transmitted in several 
**            operations.  The current implementation assumes that
**            any fault data is transmitted in a single operation.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 response_fault_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED
)
{
    rpc_cn_packet_p_t   header_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER response_fault_pred_rtn);

    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (
        (rpc_cn_call_rep_p_t) spc_struct);

    if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_MAYBE)
    {
        return (0);
    }
    else
    {
        if (((rpc_cn_call_rep_p_t) spc_struct)->assoc->assoc_status 
            != rpc_s_ok)
        {
            return (1);
        }
        else
        {
            return (2);
        }
    }
}


/*
**++
**
**  MACRO NAME:		RESPONSE_FAULT_PRED        
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is a macro version of the response_fault_pred_rtn() predicate.
**  We added the macro version to avoid overhead when calling the
**  predicate function from within the action routines.
**  Evaluate the MaybeSemantics and Disconnected predicates from
**  the Call Response state when either a fault or fault_dne 
**  event occurs.
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
**  FUNCTION VALUE:     0 if MaybeSemantics true,
**                      1 if Disconnected is true,
**                      2 otherwise.
**
**       NOTE that the architecture allows 4 possible return values
**            so that fault data can be transmitted in several 
**            operations.  The current implementation assumes that
**            any fault data is transmitted in a single operation.
**
**  SIDE EFFECTS:       none
**
**--
**/
#define RESPONSE_FAULT_PRED(spc_struct, event_param, status)	\
{								\
    RPC_CN_DBG_RTN_PRINTF(SERVER response_fault_pred_macro);	\
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (	\
        (rpc_cn_call_rep_p_t) spc_struct);			\
    if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_MAYBE)	\
    {								\
        status = 0;						\
    }								\
    else							\
    {								\
        if (((rpc_cn_call_rep_p_t) spc_struct)->assoc->assoc_status != rpc_s_ok) \
        {							\
            status = 1; 					\
        }							\
        else							\
        {							\
            status = 2; 					\
        }							\
    }								\
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
**  Evaluate whether this is the last received request fragment
**  when an RPCInd event occurs.
**
**  INPUTS:
**
**      spc_struct      This argument is the special structure which
**                      is passed to the state machine event evaluation 
**                      routine.
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
  pointer_t       spc_struct ATTRIBUTE_UNUSED,
  pointer_t       event_param
)
{

    RPC_CN_DBG_RTN_PRINTF(SERVER last_recv_frag_pred_rtn);

    if (RPC_CN_PKT_FLAGS (RPC_CN_FRAGBUF_PKT_HDR (event_param)) & 
        RPC_C_CN_FLAGS_LAST_FRAG)
    {
        return (1);
    }
    else
    {
        return (0);
    }
}

 
/*
**++
**
**  MACRO NAME:		LAST_RECV_FRAG_PRED
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is a macro of the last_recv_frag_pred_rtn routine
**  introduced to avoid the overhead of a call-to-subroutine. 
**  Evaluate whether this is the last received request fragment
**  when an RPCInd event occurs.
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
**  FUNCTION VALUE:     0 if LastRecvFrag is false
**                      1 if LastRecvFrag is true
**
**  SIDE EFFECTS:       none
**
**--
**/
#define LAST_RECV_FRAG_PRED(spc_struct, event_param, status)	\
{									\
   RPC_CN_DBG_RTN_PRINTF(SERVER last_recv_frag_pred_macro );	\
   if (RPC_CN_PKT_FLAGS (RPC_CN_FRAGBUF_PKT_HDR (event_param)) &	\
        RPC_C_CN_FLAGS_LAST_FRAG)					\
    {									\
        status = 1;							\
    }									\
    else								\
    {									\
        status = 0;							\
    }									\
} 


 

/*
**++
**
**  ROUTINE NAME:       first_frag_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Evaluates whether a received packet has the first_frag bit
**  set.
**
**  INPUTS:
**
**      spc_struct      This argument is the special structure which
**                      is passed to the state machine event evaluation 
**                      routine.
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
**  FUNCTION VALUE:     0 if FirstFrag is false
**                      1 if FirstFrag is true
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 first_frag_pred_rtn
(
  pointer_t       spc_struct ATTRIBUTE_UNUSED,
  pointer_t       event_param
)

{
    RPC_CN_DBG_RTN_PRINTF(SERVER first_frag_pred_rtn);

    if (RPC_CN_PKT_FLAGS (RPC_CN_FRAGBUF_PKT_HDR (event_param)) & 
        RPC_C_CN_FLAGS_FIRST_FRAG)
    {
        return (1);
    }
    else
    {
        return (2);
    } 

}

 
/*
**++
**
**  MACRO NAME:		ALERTED_PRED
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is a macro of the alerted_pred_rtn routine
**  introduced to avoid the overhead of a call-to-subroutine. 
**  Evaluates whether the current call has been alerted.
**
**  INPUTS:
**
**      spc_struct      The call rep. Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
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
**  FUNCTION VALUE:     0 if call was not alerted.
**                      1 if call was alerted.
**
**  SIDE EFFECTS:       none
**
**--
**/
#define ALERTED_PRED(spc_struct, status)\
{\
    RPC_CN_DBG_RTN_PRINTF(SERVER alerted_pred_macro);\
    if ( ((rpc_cn_call_rep_p_t) spc_struct)->cn_call_status ==\
        rpc_s_call_cancelled )\
    {\
        status = 1;\
    }\
    else\
    {\
        status = 0;\
    }\
}




/***********************************************************************/
/*
 * S E R V E R   C A L L   A C T I O N   R O U T I N E S
 */
/***********************************************************************/
/*
**++
**
**  MACRO NAME:         RPC_CN_ASSOC_QUEUE_FRAG
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:	Used as an optimization.  The macro function is
**			called from several places and was included 
**			made into a macro for efficiency reasons. 
**			Called from handle_first_frag_action_rtn()
**			and from handle_frag_action_rtn(). 
**
**  INPUTS:
**
**      event_param     The special event related parameter which is
**                      passed to the state machine event evaluation
**                      routine.
**                      This input argument is ignored.
**
**      spc_struct      The association group. Note that this is passed in as
**                      the special structure which is passed to the
**			state machine event evaluation routine.
** 
**	fragbuf 	(rpc_cn_fragbuf_p_t)event_param.  Used in
**			the buffer manipulation routines called from
**			within the macro.
** 
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:		none             
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
#define RPC_CN_ASSOC_QUEUE_FRAG(event_param, spc_struct, fragbuf) \
{\
    if ((RPC_CN_PKT_FLAGS (RPC_CN_FRAGBUF_PKT_HDR (event_param)) & \
         RPC_C_CN_FLAGS_FIRST_FRAG)) \
    { \
        rpc__cn_assoc_queue_frag (((rpc_cn_call_rep_p_t) spc_struct)->assoc, \
                                  fragbuf, \
                                  false); \
    } \
    else \
    { \
        rpc__cn_assoc_queue_frag (((rpc_cn_call_rep_p_t) spc_struct)->assoc, \
                                  fragbuf, \
                                  true); \
    } \
}
 
/*
**++
**
**  MACRO NAME:       	RPC_CN_FRAGBUF_DATA_SIZE
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:	Used as an optimization.  The macro function is
**			called from several places and was included 
**			made into a macro for efficiency reasons. 
**			Called from handle_first_frag_action_rtn()
**			and from handle_frag_action_rtn(). 
**
**  INPUTS:
**
**	header_p	 RPC_CN_FRAGBUF_PKT_HDR (event_param)
** 
**	fragbuf 	(rpc_cn_fragbuf_p_t)event_param.  Used in
**			the buffer manipulation routines called from
**			within the macro.
** 
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:		none             
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
#define RPC_CN_FRAGBUF_DATA_SIZE(header_p, fragbuf)\
{\
    if (RPC_CN_PKT_OBJ_UUID_PRESENT (header_p)) \
    { \
        fragbuf->data_size = RPC_CN_PKT_FRAG_LEN (header_p) - \
                             RPC_CN_PKT_AUTH_TLR_LEN (header_p) - \
                             RPC_CN_PKT_SIZEOF_RQST_HDR_W_OBJ; \
    } \
    else \
    { \
        fragbuf->data_size = RPC_CN_PKT_FRAG_LEN (header_p) - \
                             RPC_CN_PKT_AUTH_TLR_LEN (header_p) - \
                             RPC_CN_PKT_SIZEOF_RQST_HDR_NO_OBJ; \
    } \
} 

 
/*
**++
**
**  ROUTINE NAME:       handle_first_frag_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Make the received fragment data available to the stub for
**  unmarshalling.  The fragment is assumed to be the first
**  fragment of the request.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**  INPUTS/OUTPUTS:
**
**      event_param     The fragment buffer containing the received
**                      data.  On output, the data_p and data_size
**                      fields will describe the stub data.
**
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
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
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     handle_first_frag_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    unsigned32              status;
    rpc_cn_fragbuf_p_t      request_fragbuf;
    rpc_cn_packet_p_t       request_header_p;
    rpc_cn_packet_p_t       response_header_p;
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_sm_ctlblk_t 	    *sm_p;
    
    RPC_CN_DBG_RTN_PRINTF(SERVER handle_first_frag_action_rtn);
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    request_fragbuf = (rpc_cn_fragbuf_p_t) event_param;
    request_header_p = RPC_CN_FRAGBUF_PKT_HDR (event_param);

    /* 
     * As part of the performance changes, we run a macro version
     * of the predicate function, last_recv_frag_pred_rtn() here.  
     * Update status inside the macro.    
     */ 
    LAST_RECV_FRAG_PRED (spc_struct, event_param, status); 
    if (status == 1)  /* LastRecvFrag is true */ 
    {
    	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_RESPONSE;
    }
    else  /* LastRecvFrag is false */ 
    {
    	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_REQUEST;
    }
    status = rpc_s_ok;
 
    /*
     * Get the max_seg_size from the association.
     */
    call_rep->max_seg_size = 
    RPC_CN_ASSOC_MAX_XMIT_FRAG (call_rep->assoc);
    
    /*
     * Copy the opnum field into the local call rep.
     */
    call_rep->opnum = RPC_CN_PKT_OPNUM (request_header_p);
    if (!(RPC_CN_PKT_FLAGS (request_header_p) & RPC_C_CN_FLAGS_MAYBE))
    {
        
 	/*
         * Fill in the fields of the response header if this is not
         * a maybe call.
         */
        RPC_CN_CREP_SIZEOF_HDR (call_rep) = RPC_CN_PKT_SIZEOF_RESP_HDR;
        response_header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);

        RPC_CN_PKT_PTYPE (response_header_p) = RPC_C_CN_PKT_RESPONSE;
        RPC_CN_PKT_FLAGS (response_header_p) = RPC_C_CN_FLAGS_FIRST_FRAG;
        RPC_CN_PKT_FRAG_LEN (response_header_p) = 0;
        RPC_CN_PKT_VERS_MINOR (response_header_p) = 
            call_rep->assoc->assoc_vers_minor;
        RPC_CN_PKT_CALL_ID (response_header_p) = 
            RPC_CN_PKT_CALL_ID (request_header_p); 

        RPC_CN_PKT_ALLOC_HINT (response_header_p) = 0;
        RPC_CN_PKT_PRES_CONT_ID (response_header_p) =
            RPC_CN_PKT_PRES_CONT_ID (request_header_p);

        RPC_CN_PKT_RESP_RSVD (response_header_p) = 0;

        /*
         * Initialize the iovector in the call_rep to contain only
         * one initial element, pointing to the protocol header.
         * Also, update pointers to show that we can copy data into
         * the stub data area.
         */
        RPC_CN_CREP_IOVLEN (call_rep) = 1;
        RPC_CN_CREP_CUR_IOV_INDX (call_rep) = 0;
        RPC_CN_CREP_FREE_BYTES (call_rep) = 
            RPC_C_CN_SMALL_FRAG_SIZE - RPC_CN_PKT_SIZEOF_RESP_HDR;
        RPC_CN_CREP_ACC_BYTCNT (call_rep) = RPC_CN_PKT_SIZEOF_RESP_HDR;
        RPC_CN_CREP_FREE_BYTE_PTR(call_rep) = 
            RPC_CN_PKT_RESP_STUB_DATA (response_header_p);
        (RPC_CN_CREP_IOV (call_rep)[0]).data_len = RPC_CN_PKT_SIZEOF_RESP_HDR;

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
            RPC_CN_FRAGBUF_ALLOC (call_rep->prot_tlr, RPC_C_CN_SMALL_FRAG_SIZE, &status);
            call_rep->prot_tlr->fragbuf_dealloc = NULL;
            auth_value_len = RPC_C_CN_SMALL_FRAG_SIZE;
            auth_tlr = (rpc_cn_auth_tlr_t *)call_rep->prot_tlr->data_p;
            auth_tlr->auth_type = RPC_CN_AUTH_CVT_ID_API_TO_WIRE (call_rep->sec->sec_info->authn_protocol, &status);
            auth_tlr->auth_level = call_rep->sec->sec_info->authn_level;
            auth_tlr->key_id = call_rep->sec->sec_key_id;
            auth_tlr->stub_pad_length = 0;
            auth_tlr->reserved = 0;
            RPC_CN_AUTH_PRE_CALL (RPC_CN_ASSOC_SECURITY (call_rep->assoc),
                                  call_rep->sec,
                                  (pointer_t) auth_tlr->auth_value,
                                  &auth_value_len,
                                  &status);
            if (status != rpc_s_ok)
            {
                dce_error_string_t error_text;
                int temp_status;

                dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);

		/*
		 * "%s on server failed: %s"
		 */
		RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
		    rpc_svc_auth,
		    svc_c_sev_error,
		    rpc_m_call_failed_s,
		    "RPC_CN_AUTH_PRE_CALL",
		    error_text ));
		
		/*
                 * A fault packet containing an appropriate status code
                 * will be sent to the client in the receiver
                 * thread.
                 */
	         return (status);
            }
            RPC_CN_CREP_ADJ_IOV_FOR_TLR (call_rep, response_header_p, auth_value_len);
        }
        else
        {
            RPC_CN_PKT_AUTH_LEN (response_header_p) = 0;
        }
    }
    else
    {
        /*
         * There is no response packet for a maybe call.
         */
        (RPC_CN_CREP_IOV (call_rep)[0]).data_len = 0;

        /*
         * Even though there is no response for a maybe call,
         * we still set the flag bit because the server call state 
         * machine predicates check it.
         */
        RPC_CN_CREP_SIZEOF_HDR (call_rep) = RPC_CN_PKT_SIZEOF_RESP_HDR;
        response_header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);
 
        RPC_CN_PKT_FLAGS (response_header_p) = RPC_C_CN_FLAGS_MAYBE;
    }

    /* 
     * Run the functional components of handle_frag_action_routine minus
     * its internal predicate component.
     */
    /*
     * Adjust fragment buffer so that data_size describes the stub data.
     * Note that we do not adjust data_p; that will be done by
     * rpc__cn_call_receive.
     */
    RPC_CN_FRAGBUF_DATA_SIZE(request_header_p, request_fragbuf);
    
    /*
     * Queue the stub data on the association so that the call
     * executor thread can get it. 
     *
     * As an optimization for the first fragment the association
     * receive queue condition variable will not be signalled since
     * the call executor thread is not started until after we return
     * from this state transition back to the receiver thread main loop.
     */
    RPC_CN_ASSOC_QUEUE_FRAG(event_param, spc_struct, 
			(rpc_cn_fragbuf_p_t) event_param); 
    
    /*
     * Check the header for a pending cancel. If we had a pending
     * cancel at the client side re-generate it here.
     */
    if (RPC_CN_PKT_FLAGS (request_header_p) & RPC_C_CN_FLAGS_ALERT_PENDING)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                       ("(handle_first_frag_action_rtn) call_rep->%x alert pending bit set in header calling rpc__cthread_cancel()\n", call_rep));
        RPC_CALL_LOCK (((rpc_call_rep_t *) call_rep));
        rpc__cthread_cancel ((rpc_call_rep_t *) call_rep);
        RPC_CALL_UNLOCK (((rpc_call_rep_t *) call_rep));
    }
    return (status);
}


/*
**++
**
**  ROUTINE NAME:       handle_frag_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Make the received fragment data available to the stub for
**  unmarshalling.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**  INPUTS/OUTPUTS:
**
**      event_param     The fragment buffer containing the received
**                      data.  On output, the data_p and data_size
**                      fields will describe the stub data.
**
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     handle_frag_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    unsigned32              status;
    rpc_cn_fragbuf_p_t      fragbuf;
    rpc_cn_packet_p_t       header_p;
    rpc_cn_sm_ctlblk_t 	    *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER handle_frag_action_rtn);
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    fragbuf = (rpc_cn_fragbuf_p_t) event_param;
    header_p = RPC_CN_FRAGBUF_PKT_HDR (event_param);
    
    /* 
     * As part of the performance changes, we run a macro version
     * of the predicate function, last_recv_frag_pred_rtn() here.  Update 
     * status inside the macro.    
     */ 
    LAST_RECV_FRAG_PRED (spc_struct, event_param, status); 
    if (status == 1)  /* LastRecvFrag is true */ 
    {
    	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_RESPONSE;
    }
    else  /* LastRecvFrag is false */ 
    {
        sm_p->cur_state = RPC_C_SERVER_CALL_CALL_REQUEST;
    }
  
    status = rpc_s_ok;  
    
    /*
     * Adjust fragment buffer so that data_size describes the stub data.
     * Note that we do not adjust data_p; that will be done by
     *   rpc__cn_call_receive.
     */
    RPC_CN_FRAGBUF_DATA_SIZE(header_p, fragbuf);

    /*
     * Queue the stub data on the association so that the call
     * executor thread can get it. 
     *
     * As an optimization for the first fragment the association
     * receive queue condition variable will not be signalled since
     * the call executor thread is not started until after we return
     * from this state transition back to the receiver thread main loop.
     */
    RPC_CN_ASSOC_QUEUE_FRAG(event_param, spc_struct, fragbuf); 

    return(status);
}


/*
**++
**
**  ROUTINE NAME:       send_call_resp_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Start sending the call response PDU(s) to the client.
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
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
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

INTERNAL unsigned32     send_call_resp_action_rtn 
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
    rpc_cn_sm_ctlblk_t 	    *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER send_call_resp_action_rtn);
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    stub_data_p = (rpc_iovector_p_t) event_param;
    
    /* 
     * As part of the performance changes, we run a macro version
     * of the predicate function, disc_last_send_pred_rtn() here.  Update 
     * status inside the macro.   Note that DISC_LAST_SEND_PRED
     * can only return 0 or 2, not 1. 
     */ 
    DISC_LAST_SEND_PRED(spc_struct, event_param, status );
    sm_p->cur_state = RPC_C_SERVER_CALL_CALL_RESPONSE; 
    if  (status == 0)
    {
   	abort_resp_action_rtn( spc_struct, event_param, sm); 
	return(rpc_s_ok);  
    }
    status = rpc_s_ok;  
    
    /*
     * A call_response must have some stub data.
     *
     * This raises a question:
     *    If the call had no output arguments, then I guess
     *    the CALL_END on the client would send back just
     *    the protocol header.  The runtime would have no
     *    way of knowing until the CALL_END.  Right?
     *    Does this mean that we need to track whether any
     *    response was generated and if not, issue just a
     *    response header during a call end.
     */
#ifdef DEBUG
    if ((stub_data_p == NULL) || (stub_data_p->num_elt <= 0))
    {
	/*
	 * rpc_m_no_stub_data
	 * "(%s) No stub data to send"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	    rpc_svc_xmit,
	    svc_c_sev_fatal | svc_c_action_abort,
	    rpc_m_no_stub_data,
	    "send_call_resp_action_rtn" ));
    }
    else
    {
#endif
        status = rpc_s_ok;
        for (i = 0, iov_elt_p = stub_data_p->elt; 
             i < stub_data_p->num_elt; 
             i++, iov_elt_p++)
        {
            /*
             * If there is no data, just deallocate the iovector
             * element.
             */
            if (iov_elt_p->data_len <= 0)
            {
                if (iov_elt_p->buff_dealloc != NULL)
                {
                    (iov_elt_p->buff_dealloc) (iov_elt_p->buff_addr);
                }
            }
            /*
             * If the number of bytes < our bcopy_lim, copy the data
               and deallocate the buffer.  rpc__cn_copy_buffer will
               automatically transfer the data if the accumulated byte
               count reaches the segment size.
             */
            else if (iov_elt_p->data_len <= RPC_C_CN_BCOPY_LIM)
            {
                rpc__cn_copy_buffer (call_rep, iov_elt_p, &status);
                if (iov_elt_p->buff_dealloc != NULL)
                {
                    (iov_elt_p->buff_dealloc) (iov_elt_p->buff_addr);
                }
            }
            /*
             * If the buffer must be made immediately reusable, copy
             * it also.
             * Note that this can be optimized later so that we won't
             * copy; just transmit the data; if certain criteria have
             * been met.
             */
            else if (iov_elt_p->flags & rpc_c_iovector_elt_reused)
            {
                rpc__cn_copy_buffer (call_rep, iov_elt_p, &status);
                if (status != rpc_s_ok)
                {
		      return (status); 
                }
            }
            else
            {
                /*
                 * Don't copy, add this buffer as a new iovector
                 * element.  rpc__cnadd_new_vector_elmt will 
                 * automatically transfer the data if the 
                 * accumulated byte count reaches the segment size.
                 */
                rpc__cn_add_new_iovector_elmt (call_rep, iov_elt_p, &status);

                if (status != rpc_s_ok)
                {
		      return (status); 
                }
            }
        }
#ifdef DEBUG
    }
#endif
    return (status); 
}


/*
**++
**
**  ROUTINE NAME:       send_call_fault_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Start sending the call fault PDU(s) to the server.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The iovector describing the fault data 
**                      to be sent.  This is passed in as the special 
**                      event related parameter passed to the state 
**                      machine event evaluator.
**                      This parameter can be null.
**
**  INPUTS/OUTPUTS:     
**
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    call_rep->cn_call_status is assumed to
**                      indicate the cause of the fault.  If it is
**                      0, then it is assumed that there is 
**                      fault data to be sent.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok if the send was completed successfully.
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     send_call_fault_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_packet_p_t       header_p;
    rpc_iovector_p_t        stub_data_p;
    rpc_iovector_elt_p_t    iov_elt_p;
    unsigned32              i;
    unsigned32              status;
    unsigned32              pdu_size;
    rpc_cn_sm_ctlblk_t	    *sm_p;
 
    RPC_CN_DBG_RTN_PRINTF(SERVER send_call_fault_action_rtn);

    status = rpc_s_ok;

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    stub_data_p = (rpc_iovector_p_t) event_param;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 

    /* 
     * As part of the performance changes, we run a macro version
     * of the predicate function, request_fault_pred_rtn() here.  Update 
     * status inside the macro.    
     */ 
    REQUEST_FAULT_PRED (call_rep, event_param, status); 
    if (status == 0) 
    { 
	/* 
	 * abort_receive () is defined as RPC_C_SM_NO_ACTION
	 */ 
   	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_COMPLETED;  
   	return(status);  
    }
    else if (status == 1)  
    {
   	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_COMPLETED; 
    }
    else if (status == 2)  
    {
   	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_RESPONSE; 
    } 
    status = rpc_s_ok;  

    /*
     * Use the packet header bufferred on the call rep to format the
     * fault packet header.
     */
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);

    /*
     * The call is going to be terminated. The stub data bufferred
     * on the call rep will not be sent and can be released.
     */
    rpc__cn_dealloc_buffered_data (call_rep);
    RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);

    /*
     * Adjust the bufferred header to be the size of a fault packet
     * header. 
     */
    if (call_rep->sec == NULL)
    {
        pdu_size = RPC_CN_PKT_SIZEOF_FAULT_HDR;
    }
    else
    {
        pdu_size = 
            RPC_CN_PKT_SIZEOF_FAULT_HDR +
            call_rep->prot_tlr->data_size;
        RPC_CN_CREP_IOVLEN (call_rep) = 2;
    }
    RPC_CN_CREP_SIZEOF_HDR (call_rep) = pdu_size;
    RPC_CN_CREP_IOV(call_rep)[0].data_len = pdu_size;
    RPC_CN_CREP_ACC_BYTCNT (call_rep) = pdu_size;

    /*
     * Now set up the fault packet header.
     */
    RPC_CN_PKT_PTYPE (header_p) = RPC_C_CN_PKT_FAULT;
    if (call_rep->call_executed)
    {
        RPC_CN_PKT_FLAGS (header_p) = RPC_C_CN_FLAGS_FIRST_FRAG;
    }
    else
    {
        RPC_CN_PKT_FLAGS (header_p) = RPC_C_CN_FLAGS_DID_NOT_EXECUTE |
                                   RPC_C_CN_FLAGS_FIRST_FRAG;
    }
    RPC_CN_PKT_ALLOC_HINT (header_p) = 0;
    RPC_CN_PKT_PRES_CONT_ID (header_p) = call_rep->context_id;
    RPC_CN_PKT_RESP_RSVD (header_p) = 0;
    RPC_CN_PKT_RESP_RSVD2 (header_p) = 0;

    /*
     * The fault status can be either an architected non-zero
     * value indicating a runtime error, such as an interface
     * version mismatch, or zero, indicating a stub defined
     * exception specified with the stub data.
     */
    if (call_rep->cn_call_status != rpc_s_ok)
    {
        RPC_CN_PKT_STATUS (header_p) = call_rep->cn_call_status;
    }
    else
    {
        RPC_CN_PKT_STATUS (header_p) = 0;

#ifdef DEBUG        
        /*
         * There should be stub data in this case.
         */
        if ((stub_data_p == NULL) || (stub_data_p->num_elt <= 0))
        {
	    /*
	     * rpc_m_no_stub_data
	     * "(%s) No stub data to send"
	     */
	    RPC_DCE_SVC_PRINTF ((
	        DCE_SVC(RPC__SVC_HANDLE, "%s"),
	        rpc_svc_xmit,
	        svc_c_sev_fatal | svc_c_action_abort,
	        rpc_m_no_stub_data,
	        "send_call_fault_action_rtn" ));
	}
#endif

        /*
         * Chain the fault data onto the buffered output.
         */
        for (i = 0, iov_elt_p = stub_data_p->elt;
             i < stub_data_p->num_elt;
             i++, iov_elt_p++)
        {
            rpc__cn_add_new_iovector_elmt (call_rep, 
                                           iov_elt_p, 
                                           &status);
            if (status != rpc_s_ok)
            {
                rpc__cn_dealloc_buffered_data (call_rep);
		return (status);  
	    }
        }
    }

    /*
     * We can only handle one fragment of fault data.
     */
    RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_LAST_FRAG;
    
    /*
     * Update the alert_pending state and stop accepting
     * forwarded cancels.
     */
    RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                    ("(send_call_fault_action_rtn) call_rep->%x setting alert count (%d) in packet header\n", 
                     call_rep,
                     call_rep->u.server.cancel.local_count));
    RPC_CN_PKT_ALERT_COUNT (header_p) = call_rep->u.server.cancel.local_count;
    RPC_CALL_LOCK (((rpc_call_rep_t *) call_rep));
    if (call_rep->common.u.server.cancel.had_pending)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                        ("(send_call_fault_action_rtn) call_rep->%x setting alert pending bit in packet header\n", call_rep));
        RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_ALERT_PENDING;
    }
    RPC_CALL_UNLOCK (((rpc_call_rep_t *) call_rep));
    rpc__cn_transmit_buffers (call_rep, &status);
    rpc__cn_dealloc_buffered_data (call_rep);
    RPC_CN_CREP_IOVLEN (call_rep) = 1;
    return (status);  
}


/*
**++
**
**  ROUTINE NAME:       process_alert_msg_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Process a received remote alert message.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragment buffer containing the remote
**                      alert message.  This fragment buffer is
**                      deallocated.
**
**  INPUTS/OUTPUTS:     
**
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok.
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     process_alert_msg_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_fragbuf_p_t      fragbuf;
    unsigned32              status ATTRIBUTE_UNUSED;
    rpc_cn_sm_ctlblk_t 	    *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER process_alert_msg_action_rtn);

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    /*
     * Increment the number of remote cancels received.
     */
    call_rep->u.server.cancel.local_count++;

    RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                   ("(process_alert_msg_action_rtn) call_rep->%x received remote alert packet total = %d\n", 
                    call_rep,
                    call_rep->u.server.cancel.local_count));

    /*
     * We are currently executing in the context of the
     * receiver thread so call cthread_cancel. This routine will
     * cancel the call executor thread if the server is accepting and
     * not queueing cancels.
     */
    RPC_CALL_LOCK (((rpc_call_rep_t *) call_rep));
    rpc__cthread_cancel ((rpc_call_rep_t *) call_rep);
    RPC_CALL_UNLOCK (((rpc_call_rep_t *) call_rep));

    /*
     * We can just deallocate the fragment buffer.
     */
    fragbuf = (rpc_cn_fragbuf_p_t) event_param;
    (* fragbuf->fragbuf_dealloc) (fragbuf);

    /* 
     * In this case, the updated value of sm->cur_state is
     * determined by the current value of sm->cur_state.  
     * Note that 2+rpc_c_cn_statebase is call_response_state;
     * 1+rpc_c_cn_statebase is call_request_state.  rpc_c_
     * cn_statbase is used to distinguish state values from
     * action routine indexes in the state tables.  
     */ 
    if (sm_p->cur_state == (2 + RPC_C_CN_STATEBASE ))
	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_RESPONSE; 
    else if (sm_p->cur_state == ( 1 + RPC_C_CN_STATEBASE )) 
	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_REQUEST; 
    return( rpc_s_ok);
}


/*
**++
**
**  ROUTINE NAME:       abort_resp_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Discontinue any further transmission of data for the current
**  call, to the best extent possible.  Some error condition has
**  terminated the current call.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     If not null, this is the address of an 
**                      iovector.
**
**  INPUTS/OUTPUTS: 	    
**
**	sm             The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  SM is not changed
**			here.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok.
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     abort_resp_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_packet_p_t       header_p ATTRIBUTE_UNUSED;

    RPC_CN_DBG_RTN_PRINTF(SERVER abort_resp_action_rtn);

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
 
    /*
     * If there are buffered iovector elements, deallocate them.
     *
     * Note that the comparison below is not strictly correct.
     * Even if there is only a single iovector element, there
     * may be copied stub data after the header.  It's ok to
     * ignore them because the only way in which the first
     * iovector element would be reused is if we use it to send
     * a fault.  That operation would adjust the pointers
     * anyway to point to only the fault data.
     */
   if (RPC_CN_CREP_IOVLEN (call_rep) > 1)
    {
        rpc__cn_dealloc_buffered_data (call_rep);

        /*
         * This will keep the call_end_action_rtn from attempting to
         * send the remaining iov which at this point is only an auth_tlr
         * if this was an authenticated call.  We are calling this an
         * orphaned call which technically, it is, ie the connection is
         * gone and along with it the association.
         */
        call_rep->cn_call_status = rpc_s_call_orphaned ;

        /* 
         * Set the length of the iovector to be 1 (just the
         * protocol header); everything else has been
         * deallocated.
         *
         * Note that we don't use the FREE_ALL_EXCEPT_PROT_HEADER
         * macro from cnxfer.c since we know that we won't be
         * queueing more data.
         */
        RPC_CN_CREP_IOVLEN (call_rep) = 1;
    }

    return (rpc_s_ok);
}


/*
**++
**
**  ROUTINE NAME:       abort_resp_send_fault_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Discontinue any further transmission of data for the current
**  call, to the best extent possible; then send a fault.  
**  Some error condition has terminated the current call.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     If not null, this is the address of an 
**                      iovector element containing fault data.
**
**  INPUTS/OUTPUTS:     
**
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok.
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     abort_resp_send_fault_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    unsigned32              status;
    rpc_cn_sm_ctlblk_t 	    *sm_p;
    unsigned8		    n_state ATTRIBUTE_UNUSED;
    rpc_cn_packet_p_t       header_p;
    rpc_cn_call_rep_p_t	    call_rep;
    rpc_iovector_elt_p_t    iov_elt_p;
    rpc_iovector_p_t        stub_data_p;
    unsigned32		    i;
    unsigned32	  	    pdu_size;    
 
    RPC_CN_DBG_RTN_PRINTF(SERVER abort_resp_send_fault_action_rtn);
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * RESPONSE_FAULT_PRED sets status which is used here
     * to determine the updated state in the control block,
     * and to determine which action routines (if any)
     * to call next.
     */  
    RESPONSE_FAULT_PRED(spc_struct, event_param, status);
    if (status == 0)  		/* MaybeSemantics true */
    {
        sm_p->cur_state = RPC_C_SERVER_CALL_CALL_COMPLETED;
        return (rpc_s_ok);
    }
    else if (status == 1)  	/* Disconnected is true */  
    {
   	abort_resp_action_rtn (spc_struct, event_param, sm);
        sm_p->cur_state = RPC_C_SERVER_CALL_CALL_COMPLETED;
        return (rpc_s_ok);
    }
    else if (status == 2)   	/* Otherwise.. */ 
    {   
            sm_p->cur_state = RPC_C_SERVER_CALL_CALL_COMPLETED;
    } 

    /*
     * REMOVED: this should be called acording to the spec,
     * but it doesn't do anything that isn't done already,
     * and it now sets the call status to orphaned.
     *
     * Abort the send; we don't care about its returned status.
     * status = abort_resp_action_rtn (spc_struct, event_param);
     */

    /*
     * Send the fault.
     *
     * send_call_fault_action_rtn (spc_struct, event_param );  
     * We changed send_call_fault_action_rtn to include a predicate 
     * for performance reasons so we cannot just call the routine
     * here because the predicate function would cause additional
     * and incorrect state transitions.  
     */  
    RPC_CN_DBG_RTN_PRINTF(SERVER send_call_fault_action_rtn);
 
    call_rep = (rpc_cn_call_rep_p_t)spc_struct;
    stub_data_p = (rpc_iovector_p_t) event_param; 

    /*
     * Use the packet header bufferred on the call rep to format the
     * fault packet header.
     */
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);

    /*
     * The call is going to be terminated. The stub data bufferred
     * on the call rep will not be sent and can be released.
     */
    rpc__cn_dealloc_buffered_data (call_rep);
    RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);

    /*
     * Adjust the bufferred header to be the size of a fault packet
     * header. 
     */
    if (call_rep->sec == NULL)
    {
        pdu_size = RPC_CN_PKT_SIZEOF_FAULT_HDR;
    }
    else
    {
        pdu_size = 
            RPC_CN_PKT_SIZEOF_FAULT_HDR +
            call_rep->prot_tlr->data_size;
        RPC_CN_CREP_IOVLEN (call_rep) = 2;
    }
    RPC_CN_CREP_SIZEOF_HDR (call_rep) = pdu_size;
    RPC_CN_CREP_IOV(call_rep)[0].data_len = pdu_size;
    RPC_CN_CREP_ACC_BYTCNT (call_rep) = pdu_size;

    /*
     * Now set up the fault packet header.
     */
    RPC_CN_PKT_PTYPE (header_p) = RPC_C_CN_PKT_FAULT;
    if (call_rep->call_executed)
    {
        RPC_CN_PKT_FLAGS (header_p) = RPC_C_CN_FLAGS_FIRST_FRAG;
    }
    else
    {
        RPC_CN_PKT_FLAGS (header_p) = RPC_C_CN_FLAGS_DID_NOT_EXECUTE |
                                   RPC_C_CN_FLAGS_FIRST_FRAG;
    }
    RPC_CN_PKT_ALLOC_HINT (header_p) = 0;
    RPC_CN_PKT_PRES_CONT_ID (header_p) = call_rep->context_id;
    RPC_CN_PKT_RESP_RSVD (header_p) = 0;
    RPC_CN_PKT_RESP_RSVD2 (header_p) = 0;

    /*
     * The fault status can be either an architected non-zero
     * value indicating a runtime error, such as an interface
     * version mismatch, or zero, indicating a stub defined
     * exception specified with the stub data.
     */
    if (call_rep->cn_call_status != rpc_s_ok)
    {
        RPC_CN_PKT_STATUS (header_p) = call_rep->cn_call_status;
    }
    else
    {
        RPC_CN_PKT_STATUS (header_p) = 0;

#ifdef DEBUG        
        /*
         * There should be stub data in this case.
         */
        if ((stub_data_p == NULL) || (stub_data_p->num_elt <= 0))
        {
	    /*
	     * rpc_m_no_stub_data
	     * "(%s) No stub data to send"
	     */
	    RPC_DCE_SVC_PRINTF ((
	        DCE_SVC(RPC__SVC_HANDLE, "%s"),
	        rpc_svc_xmit,
	        svc_c_sev_fatal | svc_c_action_abort,
	        rpc_m_no_stub_data,
	        "send_call_fault_action_rtn" ));
	}
#endif

        /*
         * Chain the fault data onto the buffered output.
         */
        for (i = 0, iov_elt_p = stub_data_p->elt;
             i < stub_data_p->num_elt;
             i++, iov_elt_p++)
        {
            rpc__cn_add_new_iovector_elmt (call_rep, 
                                           iov_elt_p, 
                                           &status);
            if (status != rpc_s_ok)
            {
                rpc__cn_dealloc_buffered_data (call_rep);
		return(status);  
	    }
        }
    }

    /*
     * We can only handle one fragment of fault data.
     */
    RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_LAST_FRAG;
    
    /*
     * Update the alert_pending state and stop accepting
     * forwarded cancels.
     */
    RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                    ("(send_call_fault_action_rtn) call_rep->%x setting alert count (%d) in packet header\n", 
                     call_rep,
                     call_rep->u.server.cancel.local_count));
    RPC_CN_PKT_ALERT_COUNT (header_p) = call_rep->u.server.cancel.local_count;
    RPC_CALL_LOCK (((rpc_call_rep_t *) call_rep));
    if (call_rep->common.u.server.cancel.had_pending)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                        ("(send_call_fault_action_rtn) call_rep->%x setting alert pending bit in packet header\n", call_rep));
        RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_ALERT_PENDING;
    }
    RPC_CALL_UNLOCK (((rpc_call_rep_t *) call_rep));
    rpc__cn_transmit_buffers (call_rep, &status);
    rpc__cn_dealloc_buffered_data (call_rep);
    RPC_CN_CREP_IOVLEN (call_rep) = 1;
    return (status);
}


/*
**++
**
**  ROUTINE NAME:       stop_orphan_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Tell the stub to orphan the previous call.  If it was executing,
**  i.e., was still receiving for a pipe, alert it.  Otherwise,
**  discard the input data.  If possible (not required), insure
**  that no response or fault is returned for the orphaned call.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragment buffer containing the orphaned
**                      packet.  This fragment buffer is deallocated.
**
**  INPUTS/OUTPUTS:     
**
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     stop_orphan_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_fragbuf_p_t      fragbuf;
    unsigned32              status;
    rpc_binding_rep_t       *binding_r;
    rpc_cn_assoc_p_t        assoc;
    rpc_cn_sm_ctlblk_t 	    *sm_p; 

    RPC_CN_DBG_RTN_PRINTF(SERVER stop_orphan_action_rtn);

    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * We are currently executing in the context of the
     * receiver thread. The call has been orphaned by the client.
     * Free all stub data buffered for sending up to this point.
     */
     rpc__cn_dealloc_buffered_data (call_rep);
     RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);

    /*
     * If the call is queued dequeue it otherwise cancel and wakeup the thread
     * it is executing in.
     */
    status = rpc_s_ok;
    call_rep->cn_call_status = rpc_s_call_orphaned;
    if (rpc__cthread_dequeue((rpc_call_rep_t *) call_rep))
    {
        RPC_DBG_PRINTF(rpc_e_dbg_orphan, RPC_C_CN_DBG_ORPHAN,
                       ("(stop_orphan_action_rtn) call_rep->%x queued call ... dequeued call id = %x\n", 
                        call_rep,
                        RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR(call_rep))));
        binding_r = (rpc_binding_rep_t *) call_rep->binding_rep;
        RPC_CN_UNLOCK ();
        rpc__cn_call_end ((rpc_call_rep_p_t *) &call_rep, &status);
        RPC_CN_LOCK ();
        RPC_BINDING_RELEASE (&binding_r, &status);
    }
    else
    {
        /*
         * We need to cancel and wake up the call executor
         * thread. If the call executor thread is in the manager
         * routine the cancel may get through. If it is in the
         * runtime blocked on a call_receive cancels are disabled
         * and needs to be woken up.
         */
        RPC_DBG_PRINTF(rpc_e_dbg_orphan, RPC_C_CN_DBG_ORPHAN,
                       ("(stop_orphan_action_rtn) call_rep->%x running call ... cancelling and waking up call id = %x\n", 
                        call_rep,
                        RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR(call_rep))));

        /*
         * Need to hold on to the assoc in a local variable since
         * rpc__cn_assoc_pop_call is going to NULL call_rep->assoc.
         * We'll pass the local variable to rpc__cn_assoc_dealloc and
         * use it for RPC_COND_SIGNAL.
         */
        assoc = call_rep->assoc;

        /*
         * Pop the call rep off the association.
         */
        rpc__cn_assoc_pop_call (call_rep->assoc, call_rep);
 
        /*
         * Deallocate the association.
         */
        rpc__cn_assoc_dealloc (assoc, call_rep, &status);

        /*
         * If the stub is waiting for more data, signal it (really
         * rpc__cn_assoc_receive_frag()) to wake up (and find out
         * it's been orphaned).
         */
        if (assoc->assoc_msg_waiters > 0)
        {
            RPC_COND_SIGNAL (assoc->assoc_msg_cond, rpc_g_global_mutex);
        }

        RPC_CALL_LOCK (((rpc_call_rep_t *) call_rep));
        rpc__cthread_cancel ((rpc_call_rep_t *) call_rep);
        RPC_CALL_UNLOCK (((rpc_call_rep_t *) call_rep));
    }

    /*
     * Deallocate the fragment buffer containing the orphaned packet.
     */
    fragbuf = (rpc_cn_fragbuf_p_t) event_param;
    (*fragbuf->fragbuf_dealloc) (fragbuf);

    /*
     * We do not need to call CALL_END since the call_executor
     * thread will do so when it processes the cancel.
     */
    sm_p->cur_state = RPC_C_SERVER_CALL_CALL_COMPLETED;
    return(status);  
}


/*
**++
**
**  ROUTINE NAME:       discard_fragment_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Discard the received packet.  This routine is invoked when we
**  get an unexpected (but benign) PDU.
**
**  INPUTS:
**
**      spc_struct      This is the special structure which is passed 
**                      to the state machine event evaluation routine.
**                      This parameer is ignored.
**
**      event_param     The fragment buffer containing the received
**                      packet.  This is passed in as the event
**                      specific parameter to the state machine
**                      event evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm             The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  SM is not changed
**			here.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok.
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     discard_fragment_action_rtn 
(
  pointer_t       spc_struct ATTRIBUTE_UNUSED,
  pointer_t       event_param,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_fragbuf_p_t  fragbuf_p;
    unsigned32          status;

    RPC_CN_DBG_RTN_PRINTF(SERVER discard_fragment_action_rtn);

    fragbuf_p = (rpc_cn_fragbuf_p_t) event_param;
    (* fragbuf_p->fragbuf_dealloc) (fragbuf_p);

    status = rpc_s_ok;
    return (status);
}


/*
**++
**
**  ROUTINE NAME:       call_end_action_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Handle the call end event.
**
**  INPUTS:
**
**      spc_struct      The call rep.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     This parameter is ignored.
**
**  INPUTS/OUTPUTS:     
**
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/
INTERNAL unsigned32     call_end_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_call_rep_p_t     call_rep;
    rpc_cn_packet_p_t       header_p;
    unsigned32              status;
    rpc_cn_sm_ctlblk_t 	    *sm_p;
    unsigned8		    n_state ATTRIBUTE_UNUSED;

    RPC_CN_DBG_RTN_PRINTF(SERVER call_end_action_rtn);
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
   
    /* 
     * If state is call_request_state (1+rpc_c_cn_statebase), we
     * need to use the alerted_pred_rtn function. 
     * If the state is call_response 
     * (2+rpc_c_cn_statebase), then we use no predicate routine.  
     * The value of the state is augmented with rpc_c_cn_statebase in
     * order to quickly distinguish it from action routine indexes
     * in the rpc_cn_sm_event_eval() routine. 
     */ 
    if (sm_p->cur_state == ( 1 + RPC_C_CN_STATEBASE ))
    {
      ALERTED_PRED(spc_struct, status);
      if ( status == 1)
      { 
 		sm_p->cur_state = RPC_C_SERVER_CALL_CALL_COMPLETED;
      }
    	else
      {
		/* No action required here. */ 
        	sm_p->cur_state = RPC_C_SM_NO_NSTATE;	
		return (status);
      } 
    }  /* end of call_request_state */ 
    
    else if(sm_p->cur_state == ( 2 + RPC_C_CN_STATEBASE ))
	sm_p->cur_state = RPC_C_SERVER_CALL_CALL_COMPLETED;

    status = rpc_s_ok;
    
    call_rep = (rpc_cn_call_rep_p_t) spc_struct;
  
    /*
     * If this is not a maybe call; i.e., there is a cached
     * protocol header, and we have not call call_end to
     * clean up after some error condition;
     * then send the last fragment.
     * Note that if there is no stub data, we will still
     * send a header with the last_frag_bit set.
     */
    if (((RPC_CN_CREP_IOV (call_rep)[0]).data_len) != 0)
    {
        header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);
        RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_LAST_FRAG;
 
        /*
         * Update the alert_pending state and stop accepting
         * forwarded cancels.
         */
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                        ("(call_end_action_rtn) call_rep->%x setting alert count (%d) in packet header\n", 
                         call_rep,
                         call_rep->u.server.cancel.local_count));
        RPC_CN_PKT_ALERT_COUNT (header_p) = call_rep->u.server.cancel.local_count;
        RPC_CALL_LOCK (((rpc_call_rep_t *) call_rep));
        if (call_rep->common.u.server.cancel.had_pending)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                           ("(call_end_action_rtn) call_rep->%x setting alert pending bit in packet header\n", call_rep));
            RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_ALERT_PENDING;
        }

        RPC_CALL_UNLOCK (((rpc_call_rep_t *) call_rep));

        /*
         * Don't transmit if orphaned.
         */
        if (call_rep->cn_call_status != rpc_s_call_orphaned)
        {
            rpc__cn_transmit_buffers (call_rep, &status);
        }

        rpc__cn_dealloc_buffered_data (call_rep);

        /*
         * Set the length of the iovector to be 1 (just the
         * protocol header); everything else has been
         * deallocated.
         *
         * Note that we don't use the FREE_ALL_EXCEPT_PROT_HEADER
         * macro from cnxfer.c since we know that we won't be
         * queueing more data.
         */
        RPC_CN_CREP_IOVLEN (call_rep) = 1;
    }
    return (status);  
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_sm_protocol_error
**
**  SCOPE:              PRIVATE
**
**  DESCRIPTION:
**
**  Action routine invoked when an illegal transition is detected.
**  This routine writes an error message to stdout and DIEs.
**
**  INPUTS:
**
**      spc_struct      The special structure which is passed to the
**                      state machine event evaluation routine.
**                      This is assumed to be the call rep.
**
**      event_param     The event specific argument.
**
**  INPUTS/OUTPUTS:     none
**
**	sm		The control block from the event evaluation
**			routine.  Input is the current state and
**			event for the control block.  Output is the
**			next state or updated current state, for the 
**			control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     unsigned32
**
**  SIDE EFFECTS:       output is printed on stdout.
**
**--
**/
PRIVATE unsigned32     rpc__cn_call_sm_protocol_error 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    unsigned32          status ATTRIBUTE_UNUSED;
    rpc_cn_sm_ctlblk_t 	    *sm_p;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_call_sm_protocol_error);
    
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    sm_p->cur_state = RPC_C_SM_NO_NSTATE;
 
    /*
     * "Illegal state transition detected in CN {client|server} call state
     * machine [cur_state: %d, cur_event: %d, call_rep: %x]"
     */
    RPC_DCE_SVC_PRINTF ((
	DCE_SVC(RPC__SVC_HANDLE, "%d%d%x"),
	rpc_svc_cn_state,
	svc_c_sev_fatal | svc_c_action_abort,
        RPC_CALL_IS_SERVER( (rpc_call_rep_t *)call_rep ) ?
            rpc_m_cn_ill_state_trans_sr : rpc_m_cn_ill_state_trans_cr,
        call_rep->call_state.cur_state,
        call_rep->call_state.cur_event,
        call_rep ));
	 /* FIXME: is this correct? */
	 return rpc_s_ok;
}


