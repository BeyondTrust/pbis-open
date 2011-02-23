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
**      cnsasgsm.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  NCA Connection (cn) Server (s) Association (as) Group (g) State Machine (sm).
**
**
 */


#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnid.h>       /* NCA Connection local ID service */
#include <cnpkt.h>	/* NCA Connection packet encoding */
#include <cnsm.h>       /* NCA Connection state machine declarations */
#include <cnassm.h>     /* NCA Connection association state machine */
#include <cnassoc.h>    /* NCA Connection association services */
#include <cnasgsm.h>    /* NCA Connection association group state machine */


/******************************************************************************/
/*
 * Global Definitions
 */
#ifdef DEBUG
GLOBAL char     *rpc_g_cn_grp_server_events [] =
{
    "NEW              ",
    "ADD_ASSOC        ",
    "REM_ASSOC        ",
    "NO_CALLS_IND     "
};

GLOBAL char     *rpc_g_cn_grp_server_states [] =
{
    "CLOSED           ",
    "OPEN             ",
    "ACTIVE           ",
    "CALL_WAIT        "
};
#endif


/******************************************************************************/
/*
 * Local defines
 */
/******************************************************************************/


/******************************************************************************/
/*
 * Internal function prototyes
 */
/******************************************************************************/


/***********************************************************************/
/*
 * S E R V E R   A S S O C   G R P   P R E D I C A T E   T A B L E
 *
 *  
 * The predicates. All predicates except those noted below are described
 * in the NCA Connection architecture spec.
 * As a performance enhancement,
 * we have revamped many predicate routines as macros and have absorbed
 * the predicates into the actions.  Thus, there is no longer a need
 * for the predicate table;  the predicate declarations too, are
 * modified. 
 */
/* 
#define SERVER_REFS_PRED                0
#define SERVER_ASSOC_COUNT_PRED         1
*/

/*  
 * The predicate routine prototypes.
 */
INTERNAL unsigned8 server_refs_pred_rtn (
    pointer_t spc_struct, 
    pointer_t event_param);

INTERNAL unsigned8 server_assoc_count_pred_rtn (
    pointer_t spc_struct, 
    pointer_t event_param) ATTRIBUTE_UNUSED;



/***********************************************************************/
/*
 * S E R V E R   A S S O C   G R P   A C T I O N   T A B L E
 *
 *
 * The actions. All actions except those noted below are described
 * in the NCA Connection architecture spec.
 */
#define CREATE_GROUP_ID         0
#define INCR_ASSOC_COUNT        1
#define DECR_ASSOC_COUNT        2
#define RUNDOWN_HANDLES         3
#define PROTOCOL_ERROR          4

/*  
 * The action routine prototypes.
 */
INTERNAL unsigned32     create_group_id_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     incr_assoc_count_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     decr_assoc_count_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     rundown_handles_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);


/*  
 * The action table itself.
 */
GLOBAL rpc_cn_sm_action_fn_t  rpc_g_cn_server_grp_action_tbl [] =
{
    create_group_id_action_rtn,
    incr_assoc_count_action_rtn,
    decr_assoc_count_action_rtn,
    rundown_handles_action_rtn,
    rpc__cn_grp_sm_protocol_error
};


/***********************************************************************/
/*
 * S E R V E R   A S S O C   G R P   S T A T E   T A B L E
 *
 *
 * C L O S E D _ S T A T E
 *
 * state 0 - closed. No association group.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t closed_state =
{
    /* event 0 - new */
	{CREATE_GROUP_ID},
    
    /* event 1 - add_assoc */
    ILLEGAL_TRANSITION,
    
    /* event 2 - rem_assoc */
    ILLEGAL_TRANSITION,

    /* event 3 - no_calls_ind */
    ILLEGAL_TRANSITION
};


/*
 * O P E N _ S T A T E
 *
 * state 1 - open. Association group defined.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t open_state =
{
    /* event 0 - new */
    ILLEGAL_TRANSITION,
    
    /* event 1 - add_assoc */
	 {INCR_ASSOC_COUNT},
    
    /* event 2 - rem_assoc */
    ILLEGAL_TRANSITION,

    /* event 3 - no_calls_ind */
    ILLEGAL_TRANSITION
};


/*
 * A C T I V E _ S T A T E
 *
 * state 2 - active. One or more associations are currently members
 * of this group.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t active_state =
{
    /* event 0 - new */
    ILLEGAL_TRANSITION,
    
    /* event 1 - add_assoc */
	 {INCR_ASSOC_COUNT},
    
    /* event 2 - rem_assoc */
	 {DECR_ASSOC_COUNT},

    /* event 3 - no_calls_ind */
    ILLEGAL_TRANSITION,
};


/*
 * C A L L _ W A I T _ S T A T E
 *
 * state 3 - call_wait. Wait for outstanding calls for the now
 * inactive association to complete before running down any
 * outstanding context handles.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t call_wait_state =
{
    /* event 0 - new */
    ILLEGAL_TRANSITION,
    
    /* event 1 - add_assoc */
    ILLEGAL_TRANSITION,
    
    /* event 2 - rem_assoc */
    ILLEGAL_TRANSITION,

    /* event 3 - no_calls_ind */
	 {RUNDOWN_HANDLES}
};


/*
 * The state table containing the action routines.
 */
GLOBAL rpc_cn_sm_state_entry_p_t rpc_g_cn_server_grp_sm [] =
{
    closed_state,               /* state 0 - closed */
    open_state,                 /* state 1 - open */
    active_state,               /* state 2 - active */
    call_wait_state             /* state 3 - call_wait */
};


/***********************************************************************/
/*
 *
 * S E R V E R   A S S O C   G R P   P R E D I C A T E   R O U T I N E S
 *
 */
/*
**++
**
**  ROUTINE NAME:       server_refs_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Determines whether there are context handles currently
**  outstanding for context held at the server
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
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if no context handles are outstanding
**                      1 if one or more context handles are outstanding
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 server_refs_pred_rtn
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED
)
{

    rpc_cn_assoc_grp_t          *assoc_grp;

    RPC_CN_DBG_RTN_PRINTF(SERVER server_refs_pred_rtn);

    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct;

    if (assoc_grp->grp_refcnt == 0)
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
**  MACRO NAME:		SERVER_REFS_PRED        
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  Determines whether there are context handles currently
**  outstanding for context held at the server
**  This is a macro version of the server_refs_pred_rtn predicate.
**  We added the macro version to avoid overhead associated with calling
**  the predicate function from within the action routines. 
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
**  FUNCTION VALUE:     0 if no context handles are outstanding
**                      1 if one or more context handles are outstanding
**
**  SIDE EFFECTS:       none
**
**--
**/

#define SERVER_REFS_PRED(spc_struct, event_param, status)\
{\
    RPC_CN_DBG_RTN_PRINTF(SERVER server_refs_pred_macro);\
    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct;\
    if (assoc_grp->grp_refcnt == 0)\
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
**  ROUTINE NAME:       server_assoc_count_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Determines the association count is greater than 1, is 1 and
**  there are no server references or is 1 and there are server references.
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
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if assoc count is > 1
**                      1 if assoc count = 1 AND there are no server refs
**                      2 if assoc count = 1 AND there are server refs
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 server_assoc_count_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param
)
{
    rpc_cn_assoc_grp_t          *assoc_grp;

    RPC_CN_DBG_RTN_PRINTF(SERVER server_assoc_count_pred_rtn);

    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct;

    if (assoc_grp->grp_cur_assoc == 1)
    {
        if (server_refs_pred_rtn (spc_struct, event_param))
        {
            return (2);  /* assoc_grp->grp_refcnt != 0 */ 
        }
        else
        {
            return (1);  /* assoc_grp->grp_refcnt == 0 */ 
        }
    }
    else
    {
        return (0);
    }
}



/*
**++
**
**  MACRO NAME:		SERVER_ASSOC_COUNT_PRED        
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Determines the association count is greater than 1, is 1 and
**  there are no server references or is 1 and there are server references.
**  This is a macro version of the server_assoc_count_pred_rtn predicate.
**  We added the macro version to avoid overhead associated with calling
**  the predicate function from within the action routines. 
**  Note the manipulation of status below.  SERVER_REFS_PRED manipulates
**  status internally.  If the assoc_grp->grp_refcnt == 0, then SERVER_
**  REFS_PRED sets status=0, otherwise status=1.  SERVER_ASSOC_COUNT
**  PRED will check that status and in turn, reset the status value.
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
**  FUNCTION VALUE:     0 if assoc count is > 1
**                      1 if assoc count = 1 AND there are no server refs
**                      2 if assoc count = 1 AND there are server refs
**
**  SIDE EFFECTS:       none
**
**--
**/

#define SERVER_ASSOC_COUNT_PRED(spc_struct, event_param, status)\
{\
    RPC_CN_DBG_RTN_PRINTF(SERVER server_assoc_count_pred_macro);\
    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct;\
    if (assoc_grp->grp_cur_assoc == 1)\
    {\
        SERVER_REFS_PRED(spc_struct, event_param, status)\
	if (status == 0)\
        {\
            status = 1;\
	}\
       	if (status == 1)\
	{\
		status = 2;\
	}\
    }\
    else\
    {\
        status = 0;\
    }\
}


/***********************************************************************/
/*
 * S E R V E R   A S S O C   G R P   A C T I O N   R O U T I N E S
 */
/*
**++
**
**  ROUTINE NAME:       create_group_id_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to create a new association group id and return
**  it to the server assoc state machine to pass back in the
**  rpc_bind_ack PDU.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**                      This input argument is ignored.
**
**  INPUTS/OUTPUTS:   
**
**	sm		The control block from the event
**			evaluation routine.  Input is the current
**			status and event for the control block.
**			Output is the next state or updated
**			current state, for the control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     completion status, one of:
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     create_group_id_action_rtn
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_assoc_grp_t          *assoc_grp;
    rpc_cn_sm_ctlblk_t		*sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER create_group_id_action_rtn);
    
    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    sm_p->cur_state = RPC_C_ASSOC_GRP_OPEN; 
    return(assoc_grp->grp_status);  
}


/*
**++
**
**  ROUTINE NAME:       incr_assoc_count_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to increment the number of associations counter.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The association to be added. This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm		The control block from the event
**			evaluation routine.  Input is the current
**			status and event for the control block.
**			Output is the next state or updated
**			current state, for the control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     completion status, one of:
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     incr_assoc_count_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_grp_t          *assoc_grp;
    rpc_cn_sm_ctlblk_t		*sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER incr_assoc_count_action_rtn);
    
    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
   
    /*
     * Increment the group association count and add the association
     * to the group's association list.
     */
    assoc_grp->grp_cur_assoc++;
    RPC_LIST_ADD_HEAD (assoc_grp->grp_assoc_list, 
                       (rpc_cn_assoc_t *) event_param, 
                       rpc_cn_assoc_p_t);
    
    sm_p->cur_state = RPC_C_ASSOC_GRP_ACTIVE;
    return( assoc_grp->grp_status);             
}


/*
**++
**
**  ROUTINE NAME:       decr_assoc_count_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to decrement the number of associations counter.
**
**  INPUTS:
**
**      spc_struct      The association group.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The association to be removed. This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:    
**
**	sm		The control block from the event
**			evaluation routine.  Input is the current
**			status and event for the control block.
**			Output is the next state or updated
**			current state, for the control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     completion status, one of:
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     decr_assoc_count_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_grp_t          *assoc_grp;
    rpc_cn_assoc_t              *assoc;
    rpc_cn_sm_ctlblk_t		*sm_p;
    unsigned32			status;

    RPC_CN_DBG_RTN_PRINTF(SERVER decr_assoc_count_action_rtn);

    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct; 
    assoc = (rpc_cn_assoc_t *) event_param;
   
    SERVER_ASSOC_COUNT_PRED( spc_struct, event_param, status);  
    if (status == 2)
    {    
	sm_p->cur_state = RPC_C_SERVER_ASSOC_GRP_CALL_WAIT; 
    }
    else if (status == 1)
    {
       sm_p->cur_state = RPC_C_ASSOC_GRP_CLOSED; 
    }
    else if (status == 0) 
    {
        sm_p->cur_state = RPC_C_ASSOC_GRP_ACTIVE; 
    }
    
    /*
     * Clear the group pointer in the association, decrement the
     * association count in the group and remove the association from
     * the group's association list.
     */
    if ( sm_p->cur_state != RPC_C_SERVER_ASSOC_GRP_CALL_WAIT)
        RPC_CN_LOCAL_ID_CLEAR (assoc->assoc_grp_id);
    
    /* 
     * Assoc_grp pointer receives a value within the SERVER_ASSOC_COUNT_PRED
     * macro.
     */  
    assoc_grp->grp_cur_assoc--;
    RPC_LIST_REMOVE (assoc_grp->grp_assoc_list, assoc);
    return(assoc_grp->grp_status); 
}


/*
**++
**
**  ROUTINE NAME:       rundown_handles_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to inform the server stubs tp rundown any
**  outstanding context handles for server maintained context. All calls
**  for the association group must have completed.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**                      This input argument is ignored.
**
**  INPUTS/OUTPUTS:     
**
**	sm		The control block from the event
**			evaluation routine.  Input is the current
**			status and event for the control block.
**			Output is the next state or updated
**			current state, for the control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     completion status, one of:
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     rundown_handles_action_rtn
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_grp_t          *assoc_grp;
    rpc_cn_sm_ctlblk_t		*sm_p;
    rpc_cn_assoc_t              *assoc;
    unsigned32			status;

    RPC_CN_DBG_RTN_PRINTF(SERVER rundown_handles_action_rtn);
    
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct;   
    assoc = (rpc_cn_assoc_t *) event_param;

    RPC_CN_LOCAL_ID_CLEAR (assoc->assoc_grp_id);
    SERVER_REFS_PRED(spc_struct, event_param, status); 

    if (status == 0)
    {
    	sm_p->cur_state = RPC_C_ASSOC_GRP_CLOSED;
        return(status);
    }
    /*
     * Call the rundown routine registered in the group passing it
     * the group id.
     * 
     * Assoc_grp is assigned within the SERVER_REFS_PRED
     * macro.
     */  
    if (assoc_grp->grp_liveness_mntr != NULL)
    {
        (*assoc_grp->grp_liveness_mntr) ((rpc_client_handle_t) assoc_grp->grp_id.all);
    }

    sm_p->cur_state = RPC_C_ASSOC_GRP_CLOSED;
    return (assoc_grp->grp_status); 
}

