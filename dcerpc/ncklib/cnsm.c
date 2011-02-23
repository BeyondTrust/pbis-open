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
**      cnsm.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The NCA Connection Protocol State Machine Service
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnsm.h>

/***********************************************************************/


/*
**++
**
**  ROUTINE NAME:       rpc__cn_sm_init
**
**  SCOPE:              PRIVATE - declared in cnsm.h
**
**  DESCRIPTION:
**      
**  The routine will be used to initialize a state machine control
**  block. Depending on the type of state machine it will be called
**  in various places. It basically just fills in the table pointers
**  given, sets the state to closed and initializes the event list.
**
**  INPUTS:             
**
**      state_tbl       The state table this state machine is to use.
**      action_tbl      The action routine table this state machine
**                      is to use.
**	tbl_id		The identifier of the particular action table
**			we are storing in the control block.  
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      sm              The state machine control block which is to
**                      be initialized.
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

PRIVATE void rpc__cn_sm_init 
(
rpc_cn_sm_state_entry_p_t       *state_tbl,
rpc_cn_sm_action_fn_p_t         action_tbl,
rpc_cn_sm_ctlblk_p_t            sm,
unsigned32			tbl_id 
)
{

    /*
     * Put the pointers to the given tables into the state machine
     * control block.
     */
    sm->state_tbl = state_tbl;
    sm->action_tbl = action_tbl;

    /*
     * Set the initial state in the state machine control block to
     * "closed".
     */
    sm->cur_state = RPC_C_SM_CLOSED_STATE;

    /*
     * Store the tbl_id in the controlblock and use it later
     * to selectively bypass calls to the event evaluation
     * routine, going directly to the action routines.
     */
     sm->tbl_id = tbl_id;
    
    /*
     * Initialize the state machine control block event list.
     */
    rpc__cn_sm_init_event_list(sm);
}                       


/*
**++
**
**  ROUTINE NAME:       rpc__cn_sm_init_event_list
**
**  SCOPE:              PRIVATE - declared in cnsm.h
**
**  DESCRIPTION:
**      
** This routine will initialize the event list contained in the
** specified state machine control block. This routine is called as
** part of initializing the state machine control block.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      sm              The state machine control block containing
**                      the event list to be initialized.
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

PRIVATE void rpc__cn_sm_init_event_list 
(
 rpc_cn_sm_ctlblk_t      *sm
)
{
    /*
     * Set up the event list so that it's empty. This means the state
     * must be set to "empty" and the head and tail indices must be
     * zeroed. 
     */
    sm->event_list_state = RPC_C_CN_SM_EVENT_LIST_EMPTY;
    sm->event_list_hindex = 0;
    sm->event_list_tindex = 0;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_sm_eval_event
**
**  SCOPE:              PRIVATE - declared in cnsm.h
**
**  DESCRIPTION:
**      
**  This routine will be used to evaluate an event for the specified
**  state machine. It handles the main work of running a state
**  machine. It will look up either action or state transition for
**  events.  The lookup will return either a state or action.  
**  Distinguish between states and actions by numeric range.   
**
**  INPUTS:             
**
**      event_id        The number of the event to be processed.
**      event_param     The special event related parameter which is
**                      to be passed to the predicate and action
**                      routines for the processing of this event.
**      spc_struct      A special parameter which is to be passed to
**                      the predicate and action routines for the
**                      processing of this event and any subsequent
**                      events which are inserted on the event list. 
**
**  INPUTS/OUTPUTS:     
**
**      sm              The state machine control block to be used
**                      in processing this event.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     The status returned by the last action
**                      routine invoked.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32     rpc__cn_sm_eval_event 
(
  unsigned32              event_id,
  pointer_t               event_parameter,
  pointer_t               spc_struct,
  rpc_cn_sm_ctlblk_t      *sm
)
{
    rpc_cn_sm_event_entry_t     next_event;
    unsigned8                   action_index;
    boolean                     more_events;
    rpc_cn_sm_state_entry_t     *state_entry_p;

    /* 
     * Initialize action status to ok.  This allows state transitions
     * which do not invoke action routines to signal normal completion.
     */
    sm->action_status = rpc_s_ok;

    /*
     * Set up the first event to be evaluated using the input args.
     */
    next_event.event_id = event_id;
    next_event.event_param = event_parameter;

    /*
     * Process the first event and any events which get added.
     */
    more_events = true;
    while (more_events)
    {
        /*
         * Pick up the state table entry to the current state. The
	 * value in the state table is going to be either a next
	 * state or an action.  If it is an action, we take that
	 * action and within the action routine, update sm->cur_state.
	 * In cases where there is no action but there is a next state,
	 * we can just store the next state.  We distinguish between
	 * actions and state by their numeric value.  Next states are
	 * always greater than or equal to rpc_c_cn_statebase. 
   	 * Since states are also used to access an array entry, we need 
	 * to subtract the value we added in order to distinguish it 
	 * from an action routine, before we can access the array. 
         */
        state_entry_p = sm->state_tbl[(sm->cur_state - 
			RPC_C_CN_STATEBASE)];

        /*
         * Look up the index of the action routine using the current
         * state and the id of the event being processed. 
         */
        action_index = state_entry_p[(next_event.event_id - 
			RPC_C_CN_STATEBASE )].action;

        /*
         * If there is no action to take, just transition to the next 
	 * state which is the value returned from  the state table
	 * lookup (state_entry_p).
         */
	if (action_index >= RPC_C_CN_STATEBASE)
	{ 	
            sm->cur_state = action_index;
	} 
	else
	{
            /*
             * Call the action routine.  The spc_struct and event_param 
	     * and sm will be passed to the action routine.  Note
	     * that sm is changed within the action routines.  Each
	     * action routine will update sm->cur_state.  
             */
	    sm->cur_event = next_event.event_id;
	    sm->action_status = 
		(*(sm->action_tbl[action_index]))
		(spc_struct, next_event.event_param, sm); 
         }

        /*
         * Get the next event, if any, off the event list in the
         * state machine control block.  RPC_CN_SM_GET_NEXT_EVENT 
	 * will set more_events to true or false depending on
	 * whether there are more events to process.   
         */
        RPC_CN_SM_GET_NEXT_EVENT (sm, &next_event, more_events);
    }
    return (sm->action_status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_sm_insert_event
**
**  SCOPE:              PRIVATE - declared in cnsm.h
**
**  DESCRIPTION:
**      
**  This routine inserts a new event entry on the state machine
**  control block event list. If the list is full the event can't be
**  inserted and false will be returned.
**
**  INPUTS:             
**
**      event           The event entry being inserted.
**
**  INPUTS/OUTPUTS:     
**
**      sm              The state machine control block containing
**                      the event list.
**
**  OUTPUTS:            
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

PRIVATE void rpc__cn_sm_insert_event 
(
  rpc_cn_sm_event_entry_p_t       event,
  rpc_cn_sm_ctlblk_t              *sm
)
{
#ifdef DEBUG
    /*
     * Check whether the event list is full. This condition occurs
     * when the head and tail indices are equal and the state
     * indicates the list is not empty.
     */
    if ((sm->event_list_hindex == sm->event_list_tindex) &&
        (sm->event_list_state != RPC_C_CN_SM_EVENT_LIST_EMPTY))
    {
	/*
	 * rpc_m_eventlist_full
	 * "(%s) Event list full"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	    rpc_svc_cn_state,
	    svc_c_sev_fatal | svc_c_action_abort,
	    rpc_m_eventlist_full,
	    "rpc__cn_sm_insert_event" ));
    }
#endif

    /*
     * There's room on the event list. Add the new entry to the tail
     * of the list.
     */
    sm->event_list[sm->event_list_tindex].event_id = event->event_id;
    sm->event_list[sm->event_list_tindex].event_param = event->event_param;

    /*
     * Add the event to the event list by incrementing the tail
     * index and checking for wraparound. Also set the state of the
     * event list to non-empty.
     */
    sm->event_list_tindex = (sm->event_list_tindex + 1) &
                            (RPC_C_CN_SM_EVENT_LIST_MAX_ENTRIES - 1);
    sm->event_list_state = ~RPC_C_CN_SM_EVENT_LIST_EMPTY;
}
