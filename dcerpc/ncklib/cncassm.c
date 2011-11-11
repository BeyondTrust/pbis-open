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
**      cncassm.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  NCA Connection (cn) Client (c) Association (as) State Machine (sm).
**
**
 */


#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <ndrglob.h>    /* Network Data Representation syntax global defs */
#include <ndrp.h>       /* Network Data Representation syntax defs */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnid.h>       /* NCA Connection local ID service */
#include <cnnet.h>      /* NCA Connection network service */
#include <cnpkt.h>      /* NCA Connection protocol header */
#include <cncall.h>     /* NCA connection call service */
#include <comcthd.h>    /* Externals for call thread services component */
#include <cnassoc.h>    /* NCA Connection association services */
#include <cnsm.h>       /* NCA Connection state machine declarations */
#include <cnfbuf.h>     /* NCA Connection fragment buffer service */
#include <cnclsm.h>     /* NCA Connection call state machine */
#include <cnassm.h>     /* NCA Connection association state machine */


/******************************************************************************/
/*
 * Global Definitions
 */
GLOBAL char     *rpc_g_cn_assoc_client_events [] =
{
    "REQ",
    "ABORT_REQ",
    "CONN_ACK",
    "CONN_NACK",
    "NO_CONN_IND",
    "ACC_CONF",
    "REJ_CONF",
    "ALT_CONT_REQ",
    "ALT_CONT_CONF",
    "ALLOC_REQ",
    "DEALLOC_REQ",
    "SHUTDOWN_REQ",
    "LOCAL_ERROR",
    "CALLS_DONE",
    "SHUTDOWN_IND"
};

GLOBAL char     *rpc_g_cn_assoc_client_states [] =
{
    "CLOSED",
    "CONNECT_WAIT",
    "INIT_WAIT",
    "OPEN",
    "ACTIVE",
    "CALL_DONE_WAIT"
};


/******************************************************************************/
/*
 * Local defines
 */
/******************************************************************************/
/*
 * The maximum number of associations allowed on an association
 * group.
 */
#define RPC_C_ASSOC_GRP_MAX_ASSOCS              -1


/******************************************************************************/
/*
 * Internal function prototyes
 */
/******************************************************************************/
INTERNAL void send_pdu (
    rpc_cn_assoc_p_t            /*assoc*/,
    unsigned32                  /*pdu_type*/,
    rpc_cn_syntax_p_t           /*pres_context*/,
    boolean                     /*reuse_context*/,
    unsigned32                  /*grp_id*/,
    rpc_cn_sec_context_p_t      /*sec_context*/,
    boolean                     /*old_server*/,
    unsigned32 *               /*st*/);


/***********************************************************************/
/*
 * C L I E N T   A S S O C   P R E D I C A T E   T A B L E
 *
 *  
 * The predicates. All predicates except those noted below are described
 * in the NCA Connection architecture spec.
 *
 * As a performance enhancement,
 * we have revamped many predicate routines as macros and have absorbed
 * the predicates into the actions.  Thus, there is no longer a need
 * for the predicate table.
 *
 * The shutdown_allowed_req_pred is not in
 * the NCA Connection architecture spec. This predicate is a combination of
 * shutdown_requested_pred and shutdown_allowed_pred predicate and
 * active_refs_pred routines. See the routine header of
 * shutdown_allowed_req_pred_rtn for more information. 
 */
/* 
#define SHUTDOWN_REQUESTED_PRED         0
#define SHUTDOWN_ALLOWED_PRED           1
#define ACTIVE_PRED                     2
#define SHUTDOWN_ALLOWED_REQ_PRED       3
#define LASTFRAG_PRED			4
#define VERSION_MISMATCH_PRED           5
*/ 

/*  
 * The predicate routine prototypes.
 */
INTERNAL unsigned8 shutdown_requested_pred_rtn (
    pointer_t /* spc_struct */, 
    pointer_t /* event_param */);

INTERNAL unsigned8 shutdown_allowed_pred_rtn (
    pointer_t /* spc_struct */, 
    pointer_t /* event_param */);

INTERNAL unsigned8 active_pred_rtn (
    pointer_t /* spc_struct */, 
    pointer_t /* event_param */) ATTRIBUTE_UNUSED;

INTERNAL unsigned8 shutdown_allowed_req_pred_rtn (
    pointer_t /* spc_struct */, 
    pointer_t /* event_param */) ATTRIBUTE_UNUSED;

INTERNAL unsigned8 lastfrag_pred_rtn (
    pointer_t /* spc_struct */, 
    pointer_t /* event_param */);

INTERNAL unsigned8 version_mismatch_pred_rtn (
    pointer_t /* spc_struct */, 
    pointer_t /* event_param */);



/***********************************************************************/
/*
 * C L I E N T   A S S O C   A C T I O N   T A B L E
 *
 *
 * The actions. All actions except those noted below are described
 * in the NCA Connection architecture spec.
 *
 * The action mark_syntax_and_sec is not in the NCA architecture spec. It is
 * an action which extracts the negotiated transfer syntax and/or
 * security information from either an rpc_bind_ack or rpc_alter_context_resp PDU and puts
 * it in the association. This NCA architecture spec has the
 * mark_assoc action routine doing this function.
 *
 * The action mark_abort is not in the NCA architecture spec. It is
 * a combination of the mark_assoc and abort_assoc action routines.
 * See the routine header of mark_abort_action_rtn for more
 * information.
 *
 * The action add_mark_set is not in the NCA architecture spec. It is
 * a combination of the add_assoc_to_grp, mark_syntax and
 * set_secondary_addr action routines. See the routine header of
 * add_mark_set_action_rtn for more information.
 *
 * The action rem_mark_abort is not in the NCA architecture spec. It is
 * a combination of the rem_assoc_from_grp, mark_assoc and
 * abort_assoc action routines. See the routine header of
 * rem_mark_abort_action_rtn for more information.
 *
 * The action rem_mark is not in the NCA architecture spec. It is
 * a combination of the rem_assoc_from_grp and mark_assoc
 * routines. See the routine header of rem_mark_action_rtn for more
 * information. 
 *
 * The action rem_mark_discon is not in the NCA architecture spec. It is
 * a combination of the rem_assoc_from_grp, mark_assoc and  discon_calls action
 * routines. See the routine header of rem_mark_discon_action_rtn for more
 * information. 
 *
 * The action decr_rem_mark_abort is not in the NCA architecture spec. It is
 * a combination of the decr_active, rem_assoc_from_grp, mark_assoc
 * and abort_assoc action routines. See the routine header of
 * decr_rem_mark_abort_action_rtn for more information. 
 * 
 * The action shutdown_allowed_action_rtn is not part of the NCA
 * architecture specification.  It is a combination of set_shutdown_
 * request_action_rtn() and rem_mark_abort_action_rtn().  Shutdown_
 * allowed_request_action_rtn() is the entry in the state table.
 * It replaces the previous state table which included explicit
 * predicate routines and multiple action routines per state-event
 * combination.  
 *
 * NOTE: The calls done event is illegal in the active state in the
 * NCA architecture spec. It is a null event here (no state change, no
 * action) so that rpc__cn_assoc_pop_call() will not have to check the
 * state before generating the event.
 */
#define INIT_ASSOC              0
#define REQUEST_CONN            1
#define MARK_ASSOC              2
#define ADD_ASSOC_TO_GRP        3
#define REM_ASSOC_FROM_GRP      4
#define SET_SECONDARY_ADDR      5
#define AUTHENT3                6
#define SEND_ALT_CONTEXT_REQ    7 
#define ABORT_ASSOC             8
#define SET_SHUTDOWN_REQUEST    9
#define DISCON_CALLS            10
#define INCR_ACTIVE             11
#define DECR_ACTIVE             12
#define MARK_SYNTAX_AND_SEC     13
#define MARK_ABORT              14
#define ADD_MARK_SET            15
#define REM_MARK_ABORT          16
#define REM_MARK                17
#define REM_MARK_DISCON         18
#define DECR_REM_MARK_ABORT     19
#define PROTOCOL_ERROR          20
#define PROCESS_FRAG		21
#define RETRY_ASSOC 		22 
#define SHUTDOWN_ALLOWED	23
#define ILLEGAL_EVENT_ABORT     24

/*  
 * The action routine prototypes.
 */
INTERNAL unsigned32     init_assoc_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     request_conn_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     mark_assoc_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     add_assoc_to_grp_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     rem_assoc_from_grp_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     set_secondary_addr_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     authent3_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     send_alt_context_req_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     abort_assoc_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     set_shutdown_request_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     discon_calls_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     incr_active_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     decr_active_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     mark_syntax_and_sec_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     mark_abort_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     add_mark_set_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     rem_mark_abort_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     rem_mark_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     rem_mark_discon_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     decr_rem_mark_abort_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     shutdown_allowed_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     process_frag_action_rtn (
    pointer_t /* spc_struct */,
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     retry_assoc_action_rtn (
    pointer_t /* spc_struct */, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     illegal_event_abort_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

/*  
 * The action table itself.
 */
GLOBAL rpc_cn_sm_action_fn_t  rpc_g_cn_client_assoc_act_tbl [] =
{
    init_assoc_action_rtn,
    request_conn_action_rtn,
    mark_assoc_action_rtn,
    add_assoc_to_grp_action_rtn,
    rem_assoc_from_grp_action_rtn,
    set_secondary_addr_action_rtn,
    authent3_action_rtn,
    send_alt_context_req_action_rtn,
    abort_assoc_action_rtn,
    set_shutdown_request_action_rtn,
    discon_calls_action_rtn,
    incr_active_action_rtn,
    decr_active_action_rtn,
    mark_syntax_and_sec_action_rtn,
    mark_abort_action_rtn,
    add_mark_set_action_rtn,
    rem_mark_abort_action_rtn,
    rem_mark_action_rtn,
    rem_mark_discon_action_rtn,
    decr_rem_mark_abort_action_rtn,
    rpc__cn_assoc_sm_protocol_error,
    process_frag_action_rtn,
    retry_assoc_action_rtn,
    shutdown_allowed_action_rtn,
    illegal_event_abort_action_rtn
};


/***********************************************************************/
/*
 * C L I E N T   A S S O C   S T A T E   T A B L E
 *
 *
 * C L O S E D _ S T A T E
 *
 * state 0 - closed. There is currently no association to the server.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t closed_state =
{
    /* event 0 - req */
	{REQUEST_CONN}, 
    
    /* event 1 - abort_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 2 - request_conn_ack */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 3 - request_conn_nack */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 4 - no_conn_ind */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 5 - accept_conf */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 6 - reject_conf */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 7 - alter_context_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 8 - alter_context_conf */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 9 - allocate_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 10 - deallocate_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 11 - shutdown_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},

    /* event 12 - local_error */
	{RPC_C_CLIENT_ASSOC_CLOSED},

    /* event 13 - calls_done */
	{RPC_C_CLIENT_ASSOC_CLOSED},

    /* event 14 - shutdown_ind */
	{RPC_C_CLIENT_ASSOC_CLOSED}
};


/*
 * C O N N E C T _ W A I T _ S T A T E
 *
 * state 1 - connect_wait. Wait for a transport connection to be
 * created for the association.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t connect_wait_state =
{
    /* event 0 - req */
    ILLEGAL_TRANSITION,
    
    /* event 1 - abort_req */
	 {ABORT_ASSOC},
    
    /* event 2 - request_conn_ack */
	 {INIT_ASSOC},
   
    /* event 3 - request_conn_nack */  
	 {MARK_ASSOC},
    
    /* event 4 - no_conn_ind */
    ILLEGAL_TRANSITION,
    
    /* event 5 - accept_conf */
    ILLEGAL_TRANSITION,
    
    /* event 6 - reject_conf */
    ILLEGAL_TRANSITION,
    
    /* event 7 - alter_context_req */
    ILLEGAL_TRANSITION,
    
    /* event 8 - alter_context_conf */
    ILLEGAL_TRANSITION,
    
    /* event 9 - allocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 10 - deallocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 11 - shutdown_req */
    ILLEGAL_TRANSITION,

    /* event 12 - local_error */
	 {MARK_ASSOC},

    /* event 13 - calls_done */
    ILLEGAL_TRANSITION,

    /* event 14 - shutdown_ind */
	 {RPC_C_CLIENT_ASSOC_CONNECT_WAIT}
};


/*
 * I N I T _ W A I T _ S T A T E
 *
 * state 2 - init_wait. The client is waiting for the association to
 * be accepted by the server.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t init_wait_state =
{
    /* event 0 - req */
	{ILLEGAL_EVENT_ABORT},
    
    /* event 1 - abort_req */
	{MARK_ABORT},
    
    /* event 2 - request_conn_ack */
	ILLEGAL_TRANSITION,
    
    /* event 3 - request_conn_nack */
	ILLEGAL_TRANSITION,
    
    /* event 4 - no_conn_ind */
	{MARK_ASSOC},
    
    /* event 5 - accept_conf */
	{ADD_MARK_SET},
    
    /* event 6 - reject_conf */
	{RETRY_ASSOC},
    
    /* event 7 - alter_context_req */
	ILLEGAL_TRANSITION,
    
    /* event 8 - alter_context_conf */
	ILLEGAL_TRANSITION,
    
    /* event 9 - allocate_req */
	ILLEGAL_TRANSITION,
    
    /* event 10 - deallocate_req */
	ILLEGAL_TRANSITION,
    
    /* event 11 - shutdown_req */
	ILLEGAL_TRANSITION,

    /* event 12 - local_error */
	{MARK_ABORT},

    /* event 13 - calls_done */
	ILLEGAL_TRANSITION,

    /* event 14 - shutdown_ind */
	{RPC_C_CLIENT_ASSOC_INIT_WAIT}
};


/*
 * O P E N _ S T A T E
 *
 * state 3 - open_wait. The association has been successfully
 * established. 
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t open_state =
{
    /* event 0 - req */
    ILLEGAL_TRANSITION,

    /* event 1 - abort_req */
	 {REM_MARK_ABORT},
    
    /* event 2 - request_conn_ack */
    ILLEGAL_TRANSITION,
    
    /* event 3 - request_conn_nack */
    ILLEGAL_TRANSITION,
    
    /* event 4 - no_conn_ind */
	{REM_MARK},
    
    /* event 5 - accept_conf */
    ILLEGAL_TRANSITION,
    
    /* event 6 - reject_conf */
    ILLEGAL_TRANSITION,
    
    /* event 7 - alter_context_req */
	 {SEND_ALT_CONTEXT_REQ},
    
    /* event 8 - alter_context_conf */
	 {RPC_C_CLIENT_ASSOC_OPEN},
    
    /* event 9 - allocate_req */
	 {INCR_ACTIVE},
    
    /* event 10 - deallocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 11 - shutdown_req */
	 {SHUTDOWN_ALLOWED},

    /* event 12 - local_error */
	 {REM_MARK_ABORT},

    /* event 13 - calls_done */
    ILLEGAL_TRANSITION,

    /* event 14 - shutdown_ind */
	 {SHUTDOWN_ALLOWED}
};


/*
 * A C T I V E _ S T A T E
 *
 * state 4 - active. The association is currently in use. Only one
 * call and its related callbacks may use an association at a time.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t active_state =
{
    /* event 0 - req */
	ILLEGAL_TRANSITION,
    
    /* event 1 - abort_req */
	{REM_MARK_ABORT},
    
    /* event 2 - request_conn_ack */
    ILLEGAL_TRANSITION,
    
    /* event 3 - request_conn_nack */
    ILLEGAL_TRANSITION,
    
    /* event 4 - no_conn_ind */
	 {REM_MARK_DISCON},
    
    /* event 5 - accept_conf */
    ILLEGAL_TRANSITION,
    
    /* event 6 - reject_conf */
    ILLEGAL_TRANSITION,
    
    /* event 7 - alter_context_req */
	 {SEND_ALT_CONTEXT_REQ},
    
    /* event 8 - alter_context_conf */
	 {MARK_SYNTAX_AND_SEC},
    
    /* event 9 - allocate_req */
	 {INCR_ACTIVE},
    
    /* event 10 - deallocate_req */
	 {DECR_ACTIVE},
    
    /* event 11 - shutdown_req */
	 {SET_SHUTDOWN_REQUEST},

    /* event 12 - local_error */
	 {REM_MARK_ABORT},

    /* event 13 - calls_done */
	 {RPC_C_CLIENT_ASSOC_ACTIVE},

    /* event 14 - shutdown_ind */
	 {SET_SHUTDOWN_REQUEST}
};


/*
 * C A L L _ D O N E _ W A I T _ S T A T E
 *
 * state 5 - call_done_wait. Wait for outstanding call(s) to
 * complete before recovering the association resources.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t call_done_wait_state =
{
    /* event 0 - req */
    ILLEGAL_TRANSITION,
    
    /* event 1 - abort_req */
    ILLEGAL_TRANSITION,
    
    /* event 2 - request_conn_ack */
    ILLEGAL_TRANSITION,
    
    /* event 3 - request_conn_nack */
    ILLEGAL_TRANSITION,
    
    /* event 4 - no_conn_ind */
    ILLEGAL_TRANSITION,
    
    /* event 5 - accept_conf */
    ILLEGAL_TRANSITION,
    
    /* event 6 - reject_conf */
    ILLEGAL_TRANSITION,
    
    /* event 7 - alter_context_req */
    ILLEGAL_TRANSITION,
    
    /* event 8 - alter_context_conf */
    ILLEGAL_TRANSITION,
    
    /* event 9 - allocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 10 - deallocate_req */
    /* Note that this transition should have an action to decrement the */
    /* active refs and if 0 generate a calls done event. However, */
    /* this implementation will generate the calls done event when */
    /* the last call rep is popped off the association stack. */
	 {RPC_C_CLIENT_ASSOC_CALL_DONE_WAIT},
    
    /* event 11 - shutdown_req */
    ILLEGAL_TRANSITION,

    /* event 12 - local_error */
    ILLEGAL_TRANSITION,

    /* event 13 - calls_done */
	 {RPC_C_CLIENT_ASSOC_CLOSED},

    /* event 14 - shutdown_ind */
	 {RPC_C_CLIENT_ASSOC_CALL_DONE_WAIT}
};


/*
 * The state table containing the action routines.
 */
GLOBAL rpc_cn_sm_state_entry_p_t rpc_g_cn_client_assoc_sm [] =
{
    closed_state,           /* state 0 - closed */
    connect_wait_state,     /* state 1 - connect_wait */
    init_wait_state,        /* state 2 - init_wait */
    open_state,             /* state 3 - open */
    active_state,           /* state 4 - active */
    call_done_wait_state    /* state 5 - call_done_wait */
};


/***********************************************************************/
/*
 *
 * C L I E N T   A S S O C   P R E D I C A T E   R O U T I N E S
 *
 */
/*
**++
**
**  ROUTINE NAME:       shutdown_requested_pred_rtn
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Determines whether the client received one or more
**  rpc_shutdown_request PDUs from the server or a shutdown request
**  event arrived from the user. Its initial value is false. Note that
**  this predicate is persistent. That is, if at the time the predicate
**  becomes true shutdown is not allowed, it may occur later when it
**  becomes legal. However, later could be when the current call
**  finishes executing, or could be when some context handle gets
**  released many moons from now. This will not result in incorrect
**  behavior, but perhaps occasional shutdown at unexpected times when
**  it may no longer be desired.
**
**  INPUTS:
**
**      spc_struct      The association. Note that this is passed in as
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
**  FUNCTION VALUE:     0 if no shutdown PDUs have been received
**                      1 if one or more shutdown PDUs have been received
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 shutdown_requested_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED
)
{
    RPC_CN_DBG_RTN_PRINTF(CLIENT shutdown_requested_pred_rtn);
    
    /*
     * The association flags will indicate whether an
     * rpc_shutdown_request PDU has been received.
     */
    if (((rpc_cn_assoc_t *)spc_struct)->assoc_flags &
        RPC_C_CN_ASSOC_SHUTDOWN_REQUESTED) 
    {
        /*
         * At least one rpc_shutdown_request PDU was received.
         */
        return (1);
    }
    else
    {
        /*
         * No rpc_shutdown_request PDUs have been received.
         */
        return (0);
    }
}


/*
**++
**
**  ROUTINE NAME:       shutdown_allowed_pred_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Determines whether the association is needed to preserve
**  context handles. That is, whether there are no context handles
**  currently active, or there are other associations in the open,
**  active or altered_context_wait states.
**
**  INPUTS:
**
**      spc_struct      The association. Note that this is passed in as
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
**  FUNCTION VALUE:     0 if association needed
**                      1 if association not needed
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 shutdown_allowed_pred_rtn
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;

    RPC_CN_DBG_RTN_PRINTF(CLIENT shutdown_allowed_pred_rtn);
    
    /*
     * The NCA connection architecture spec states: if one or
     * more context handles are active between a client and server at
     * least one connection must be maintained. The reference count on
     * the association group indicates how many context handles are
     * outstanding for all associations in the group. The current
     * number of associations on the group is contained in the group.
     * The association will be allowed to shutdown only if the group
     * reference count is zero or there is more than one association
     * on the group.
     */
    assoc_grp = RPC_CN_ASSOC_GRP (((rpc_cn_assoc_t *)spc_struct)->assoc_grp_id);
    if ((assoc_grp->grp_refcnt == 0) ||
        (assoc_grp->grp_cur_assoc > 1))
    {
        /*
         * There are either no context handles on the group or
         * there is more than one association on the group. The
         * association may be freed.
         */
        return (1);
    }
    else
    {
        /*
         * There are context handles on the group and there is only
         * a single association on the group.
         */
        return (0);
    }
}


/*
**++
**
**  MACRO NAME: 	SHUTDOWN_ALLOWED_PRED        
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**  This is a macro version of shutdown_allowed_pred_rtn predicate routine.
**  We added the macro version to avoid overhead associated with calling
**  the predicate function from within the action routines.
**  Macro determines whether the association is needed to preserve
**  context handles. That is, whether there are no context handles
**  currently active, or there are other associations in the open,
**  active or altered_context_wait states.
**      
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
**  FUNCTION VALUE:     0 if association needed
**                      1 if association not needed
**
**  SIDE EFFECTS:       none
**
**--
**/
#define  SHUTDOWN_ALLOWED_PRED(spc_struct, event_param, status)\
{\
    RPC_CN_DBG_RTN_PRINTF(CLIENT shutdown_allowed_pred_macro);\
    assoc_grp = RPC_CN_ASSOC_GRP (((rpc_cn_assoc_t *)spc_struct)->assoc_grp_id);\
    if ((assoc_grp->grp_refcnt == 0) ||\
        (assoc_grp->grp_cur_assoc > 1))\
    {\
        status = 1;\
    }\
    else\
    {\
        status = 0;\
    }\
}


/*
**++
**
**  ROUTINE NAME:       active_pred_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Determines whether the association is currently in use. Unless
**  the concurrent multiplexing option has been mutually selected, only one
**  call and its related callbacks, or an alter-context request, may use
**  an association at one time. This predicate is manupulated via the
**  incr_active and decr_active action routines which manipulate a
**  reference counter, and is ready by the association group policy
**  mechanisms. A zero reference count implies the predicate value is
**  false, otherwise it is true.
**
**  INPUTS:
**
**      spc_struct      The association. Note that this is passed in as
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
**  FUNCTION VALUE:     0 if association reference counter is 0
**                      1 if association reference counter is non-zero
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 active_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t      *assoc;

    RPC_CN_DBG_RTN_PRINTF(CLIENT active_pred_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    /*
     * A reference counter in the association indicates whether the
     * association is currently active.
     */
    if (assoc->assoc_ref_count == 0)
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
**  ROUTINE NAME:       shutdown_allowed_req_pred_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This predicate routine determines whether an association has
**  been requested to shutdown AND is allowed to shutdown AND
**  whether there are active references to the association.
**
**  INPUTS:
**
**      spc_struct      The association. Note that this is passed in as
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
**  FUNCTION VALUE:     0 if no active refs AND either shutdown not
**                        requested OR shutdown not allowed 
**                      1 if no active refs AND shutdown requested
**                        AND shutdown allowed 
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 shutdown_allowed_req_pred_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param
)
{
    RPC_CN_DBG_RTN_PRINTF(CLIENT shutdown_allowed_req_pred_rtn);
    
    /*
     * Check whether an association shutdown was requested and
     * whether it is allowed.
     */
    if (shutdown_requested_pred_rtn (spc_struct, event_param) &&
        shutdown_allowed_pred_rtn (spc_struct, event_param))
    {
        /*
         * A shutdown was requested and it is allowed.
         */
        return (1);
    }
    else
    {
        /*
         * Either a shutdown was not requested or it is not allowed.
         */
        return (0);
    }
}



/*
**++
**
**  MACRO NAME:  	SHUTDOWN_ALLOWED_REQ_PRED       
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is a macro version of shutdown_allowed_req_pred_rtn predicate routine.
**  We added the macro version to avoid overhead associated with calling
**  the predicate function from within the action routines.
**  This predicate macro determines whether an association has    
**  been requested to shutdown AND is allowed to shutdown AND
**  whether there are active references to the association.
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
**  FUNCTION VALUE:     0 if no active refs AND either shutdown not
**                        requested OR shutdown not allowed 
**                      1 if no active refs AND shutdown requested
**                        AND shutdown allowed 
**
**  SIDE EFFECTS:       none
**
**--
**/
#define  SHUTDOWN_ALLOWED_REQ_PRED(spc_struct, event_param, status)\
{\
    RPC_CN_DBG_RTN_PRINTF(CLIENT shutdown_allowed_req_pred_macro);\
    if (shutdown_requested_pred_rtn (spc_struct, event_param) &&\
        shutdown_allowed_pred_rtn (spc_struct, event_param))\
    {\
        status = 1;\
    }\
    else\
    {\
        status = 0;\
    }\
}


/*
**++
**
**  ROUTINE NAME:       lastfrag_pred_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Determines if the last_frag flag is set in an rpc_bind_ack or
**  alter_context_resp PDU.
**
**  INPUTS:
**
**      spc_struct      The association. Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**                      This input argument is ignored.
**
**      event_param     The fragbuf containing the rpc_bind_ack or
**                      alter_context_resp PDU. 
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if last_frag is not set
**                      1 if last_frag is set.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 lastfrag_pred_rtn (spc_struct, event_param)

pointer_t       spc_struct ATTRIBUTE_UNUSED;
pointer_t       event_param;

{
    rpc_cn_packet_t     *header;

    RPC_CN_DBG_RTN_PRINTF(CLIENT lastfrag_pred_rtn);
    
    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind_ack or alter_context_resp PDU.
     */
    header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;

    /*
     * This is the last packet we are going to get if last_frag flag is set
     * or we are talking to an old client.
     */
    if ((RPC_CN_PKT_FLAGS (header) & RPC_C_CN_FLAGS_LAST_FRAG) ||
        (RPC_CN_PKT_VERS_MINOR (header) == RPC_C_CN_PROTO_VERS_COMPAT))
    {
        return (1);
    }
    else
    {
        return(0);
    }
}


/*
**++
**
**  ROUTINE NAME:       version_mismatch_pre_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Determines if the rpc_bind_nak is due to a procol version mismatch
**  with a version 5.0 server.
**
**  INPUTS:
**
**      spc_struct      The association. Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**                      This input argument is ignored.
**
**      event_param     The fragbuf containing the rpc_bind_nak.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if reject for some other reason
**                      1 if reject due to protocol mismatch error
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 version_mismatch_pred_rtn (spc_struct, event_param)

pointer_t       spc_struct ATTRIBUTE_UNUSED;
pointer_t       event_param;

{
    rpc_cn_packet_t                     *header;
    rpc_cn_versions_supported_p_t       versions;
    unsigned32                          status;
    unsigned32                          i;

    RPC_CN_DBG_RTN_PRINTF(CLIENT version_mismatch_pred_rtn);
    
    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind_nak.
     */
    header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;
    status = rpc__cn_assoc_prej_to_status (RPC_CN_PKT_PROV_REJ_REASON(header));

    if (status == rpc_s_rpc_prot_version_mismatch)
    {

        RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                         ("CN: BIND_NAK - protocol version mismatch:\n"));

        /* Check the versions supported for 5.0 */
        versions = &(RPC_CN_PKT_VERSIONS (header));
        for (i=0; i <= versions->n_protocols; i++)
        {

           RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                          ("\t\tVersion %d.%d\n", 
                          versions->protocols[i].vers_major, 
                          versions->protocols[i].vers_minor));

           if((versions->protocols[i].vers_major == RPC_C_CN_PROTO_VERS) &&
              (versions->protocols[i].vers_minor == RPC_C_CN_PROTO_VERS_COMPAT))
           {
                return(1);
           }
        }   
        return (0);
    }
    else
    {
        return(0);
    }
}

/***********************************************************************/
/*
 * C L I E N T   A S S O C   A C T I O N   R O U T I N E S
 */
/*
**++
**
**  MACRO NAME:       	DECR_REM_MARK_ABORT_ACTION
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This is a macro version of decr_rem_mark_abort_action_rtn(). 
**  We added the macro version to avoid overhead.
**  Macro to decrements the active references, removes an
**  association from an association group, marks and aborts it.
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/
#define  DECR_REM_MARK_ABORT_ACTION(spc_struct, event_param, sm)\
{\
    RPC_CN_DBG_RTN_PRINTF(CLIENT decr_rem_mark_abort_action_macro);\
	assoc = (rpc_cn_assoc_t *) spc_struct;\
	sm_p = (rpc_cn_sm_ctlblk_t *)sm;\
    	DECR_ACTIVE_ACTION(assoc);\
    	if (assoc->assoc_status != rpc_s_ok)\
    	{\
       		sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;\
    		return(assoc->assoc_status);\
	}\
    	rem_mark_abort_action_rtn (spc_struct, event_param, sm);\
       	sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;\
}


/*
**++
**
**  MACRO NAME:       	DECR_ACTIVE_ACTION
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This is a macro version of decr_active_active_rtn() function.
**  We added the macro version to avoid overhead.   We reduced the
**  call parameters to just assoc as the rest are unnecessary.  The
**  macro is called from DECR_REM_MARK_ABORT_ACTION, decr_active_
**  action_rtn, and decr_rem_mark_abort_action_rtn.
**  Action macro  to set the active predicate to false. The client
**  runtime deallocated the association when done with an alter context
**  request, a call, its callbacks, etc.
**
**  INPUTS:
**
**	assoc		Pointer to the association control block. 
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
#define   DECR_ACTIVE_ACTION(assoc)\
{\
        RPC_CN_DBG_RTN_PRINTF(CLIENT decr_active_action_macro);\
    	assoc->assoc_ref_count--;\
}\


/*
**++
**
**  ROUTINE NAME:       init_assoc_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to send an rpc_bind PDU to the server.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The association work structure. This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:    
**
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     init_assoc_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_assoc_sm_work_t      *assoc_sm_work;
    dce_uuid_t                      assoc_uuid;
    boolean                     old_server;
    rpc_protocol_version_t      *protocol_version;
    rpc_cn_sm_ctlblk_t		*sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT init_assoc_action_rtn);
    
    assoc = (rpc_cn_assoc_t *) spc_struct;
    assoc_sm_work = (rpc_cn_assoc_sm_work_t *) event_param;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * Create a UUID for this association, compute its CRC and store
     * it as part of the per-association security information.
     */
    dce_uuid_create (&assoc_uuid, &(assoc->assoc_status));
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));
    assoc->security.assoc_uuid_crc = 
        rpc__cn_pkt_crc_compute ((unsigned8 *)&assoc_uuid, 
                                 sizeof (dce_uuid_t));
    
    /*
     * Check the binding handle for protocol version information
     * If we know we are talking to an old server, this can save us work.
     */
    protocol_version = RPC_CN_ASSOC_CALL(assoc)->binding_rep->protocol_version;
    if ((protocol_version != NULL) &&
        (protocol_version->minor_version == RPC_C_CN_PROTO_VERS_COMPAT))
    {
        old_server = true;
        assoc->assoc_vers_minor = protocol_version->minor_version;
    }
    else if (assoc->assoc_vers_minor == RPC_C_CN_PROTO_VERS_COMPAT)
    {
        old_server = true;
    }
    else
    {
        old_server = false;
    }

    /*
     * Format an rpc_bind PDU and send it to the server.
     */
    send_pdu (assoc,
              RPC_C_CN_PKT_BIND,
              assoc_sm_work->pres_context,
              assoc_sm_work->reuse_context,
              assoc_sm_work->grp_id,
              assoc_sm_work->sec_context,
              old_server,
              &(assoc->assoc_status));
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));

    sm_p->cur_state = RPC_C_CLIENT_ASSOC_INIT_WAIT;
    return (assoc->assoc_status); 
}


/*
**++
**
**  ROUTINE NAME:       request_conn_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to request a new transport/session connection.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     A state machine work structure containing
**                      the interface rep and the association group
**                      id. This is passed in as the special event
**                      related parameter which was passed to the
**                      state machine evaluation routine. 
**
**  INPUTS/OUTPUTS:     
**
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     request_conn_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_sm_event_entry_t     event;
    rpc_cn_sm_ctlblk_t		*sm_p;
 
    RPC_CN_DBG_RTN_PRINTF(CLIENT request_conn_action_rtn);
    
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    if (RPC_DBG2 (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL))
    {
        unsigned_char_t *netaddr, *endpoint;
        unsigned32      dbg_status;

        rpc__naf_addr_inq_netaddr (assoc->cn_ctlblk.rpc_addr,
                                   &netaddr,
                                   &dbg_status);
        rpc__naf_addr_inq_endpoint (assoc->cn_ctlblk.rpc_addr,
                                   &endpoint,
                                   &dbg_status);

        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
            ("CN: call_rep->%x assoc->%x desc->%x connection request initiated to %s[%s]\n",
             assoc->call_rep,
             assoc,
             assoc->cn_ctlblk.cn_sock,
             netaddr,
             endpoint));
        rpc_string_free (&netaddr, &dbg_status);
        rpc_string_free (&endpoint, &dbg_status);
    }

    /*
     * Establish the session/transport connection.
     */
    rpc__cn_network_req_connect (assoc->cn_ctlblk.rpc_addr,
                                 assoc,
                                 &(assoc->assoc_status));

    /*
     * Set up the next event for the state machine based on the
     * results of rpc__cn_network_req_connect().
     */
    switch ((int)assoc->assoc_status)
    {
        case rpc_s_ok:
        {
            /*
             * We have a connection. Send an RPC_C_ASSOC_REQUEST_CONN_ACK
             * event through the association state machine. The event
             * parameter is the interface specification rep contained in
             * the work structure.
             */
            event.event_id = RPC_C_ASSOC_REQUEST_CONN_ACK;
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                            ("CN: call_rep->%x assoc->%x desc->%x connection established\n",
                             assoc->call_rep,
                             assoc,
                             assoc->cn_ctlblk.cn_sock));
            break;
        }

        default:
        {
            if (rpc__cn_network_connect_fail (assoc->assoc_status))
            {
                /*
                 * The connection failed. Send an
                 * RPC_C_ASSOC_REQUEST_CONN_NACK event through the association
                 * state machine. The event parameter is the interface
                 * specification rep contained in the call rep.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: call_rep->%x assoc->%x desc->%x connection request failed st = %x\n",
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock,
                                 assoc->assoc_status));
                
                event.event_id = RPC_C_ASSOC_REQUEST_CONN_NACK;
            }
            else
            {
                /*
                 * Some local error occured which caused us not to get a
                 * connection. Send an RPC_C_ASSOC_LOCAL_ERROR event
                 * through the association state machine.
                 */
                event.event_id = RPC_C_ASSOC_LOCAL_ERROR;
                assoc->assoc_local_status = assoc->assoc_status;
            }
            break;
        }
    }

    /*
     * Now actually send the event through the state machine.
     */
    event.event_param = event_param;
    RPC_CN_ASSOC_INSERT_EVENT (assoc, &event);
    sm_p->cur_state = RPC_C_CLIENT_ASSOC_CONNECT_WAIT; 
    return ( assoc->assoc_status);  
}


/*
**++
**
**  ROUTINE NAME:       mark_assoc_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to mark an association with the appropriate
**  status code. 
**
**  INPUTS:
**
**      spc_struct      The association. Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation
**                      routine.
**                      This argument is ignored.
**
**  INPUTS/OUTPUTS:    
**
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**                      rpc_s_connection_closed
**                      rpc_s_connection_aborted
**                      rpc_s_assoc_shutdown
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     mark_assoc_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t      *assoc;
    rpc_cn_packet_t     *header;
    rpc_cn_syntax_t     *syntax;
    dcethread*          current_thread_id;
    rpc_cn_sm_ctlblk_t   *sm_p;
 
    RPC_CN_DBG_RTN_PRINTF(CLIENT mark_assoc_action_rtn);
    
    /*
     * The special structure passed in is the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    
    /*
     * Mark the association based on the event currently being
     * processed.
     */
    switch (assoc->assoc_state.cur_event)
    {
        case RPC_C_ASSOC_NO_CONN_IND:
        {
            /*
             * The underlying session/transport connection has failed.
             */
            assoc->assoc_status = rpc_s_connection_closed;
            break;
        }
        
        case RPC_C_ASSOC_ABORT_REQ:
        {
            /*
             * The user has aborted the association.
             */
            assoc->assoc_status = rpc_s_connection_aborted;
            break;
        }
        
        case RPC_C_ASSOC_REJECT_CONF:
        {
            /*
             * The association request has been rejected.
             * The event parameter is a pointer to the fragbuf containing
             * the rpc_bind_nack PDU. Find the syntax element
             * corresponding to this negotiation and remove it.
             */
            header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;
            assoc->assoc_status = rpc__cn_assoc_prej_to_status
                (RPC_CN_PKT_PROV_REJ_REASON (header));
            RPC_LIST_FIRST (assoc->syntax_list,
                            syntax, 
                            rpc_cn_syntax_p_t);
            while (syntax != NULL)
            {
                rpc_cn_syntax_t *next_syntax;

                RPC_LIST_NEXT (syntax, next_syntax, rpc_cn_syntax_p_t);
                if (syntax->syntax_call_id == RPC_CN_PKT_CALL_ID (header))
                {
                    RPC_LIST_REMOVE (assoc->syntax_list,
                                     syntax);
                    rpc__cn_assoc_syntax_free (&syntax);
                    break;
                }
                syntax = next_syntax;
            }
            break;
        }
        
        case RPC_C_ASSOC_SHUTDOWN_IND:
        {
            /*
             * A shutdown request has been received from the server and
             * is being processed.
             */
            assoc->assoc_status = rpc_s_assoc_shutdown;
            break;
        }
        
        case RPC_C_ASSOC_REQUEST_CONN_NACK:
        {
            /*
             * The error status has already been marked on the association
             * error status for this event.
             */
            break;
        }

        case RPC_C_ASSOC_LOCAL_ERROR:
        {
            /*
             * The error status is contained in the association local
             * error status for this event.
             */
            assoc->assoc_status = assoc->assoc_local_status;
            break;
        }
    }
    
    /*
     * Wake up any threads blocked waiting for receive data.
     */
    current_thread_id = dcethread_self();
    if (dcethread_equal (current_thread_id,
                       assoc->cn_ctlblk.cn_rcvr_thread_id))
    {
        RPC_CN_ASSOC_WAKEUP (assoc);
    }
    sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;
    return (assoc->assoc_status); 
}


/*
**++
**
**  ROUTINE NAME:       add_assoc_to_grp_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to add an association to an association group.
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
**                      This argument is ignored.
**
**  INPUTS/OUTPUTS:     
**
**	sm             The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  SM is not changed
**			here but is passed in to avoid compile warnings
**			from rpc__cn_sm_eval_event().
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

INTERNAL unsigned32     add_assoc_to_grp_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm  ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t      *assoc;

    RPC_CN_DBG_RTN_PRINTF(CLIENT add_assoc_to_grp_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    rpc__cn_assoc_grp_add_assoc (assoc->assoc_grp_id,
                                 assoc);

    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       rem_assoc_from_grp_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to remove an association from an association group.
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
**	sm             The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  SM is not changed
**			here but is passed in to avoid compile warnings
**			from rpc__cn_sm_eval_event().
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

INTERNAL unsigned32     rem_assoc_from_grp_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t      *assoc;

    RPC_CN_DBG_RTN_PRINTF(CLIENT rem_assoc_from_grp_action_rtn);
    
    /*
     * The special structure passed in is the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    
    /*
     * Remove the association from the group.
     */
    rpc__cn_assoc_grp_rem_assoc (assoc->assoc_grp_id, assoc);

    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       set_secondary_addr_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to set the optional secondary address for the
**  association group from the received data in the rpc_bind_ack accept
**  PDU, if provided.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the rpc_bind_ack PDU.
**                      This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:    
**
**	sm             The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  SM is not changed
**			here but is passed in to avoid compile warnings
**			from rpc__cn_sm_eval_event().
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

INTERNAL unsigned32     set_secondary_addr_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t      *assoc;
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_packet_t     *header;
    rpc_cn_port_any_t   *sec_addr;
    
    RPC_CN_DBG_RTN_PRINTF(CLIENT set_secondary_addr_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    
    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind_ack PDU.
     */
    header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;
    
    /*
     * An rpc_bind_ack PDU has just been received from the server.
     * The association is already part of an association group. If
     * it the only one on the group the secondary address endpoint
     * contained in the rpc_bind_ack PDU still needs to be used to build a
     * secondary RPC address for subsequent associations to the same
     * server. 
     */
    assoc_grp = RPC_CN_ASSOC_GRP (assoc->assoc_grp_id);
    if (assoc_grp->grp_cur_assoc == 1)
    {
        /*
         * If a secondary address endpoint was provided in the
         * rpc_bind_ack PDU use it to create a secondary address and
         * store this on the association group.
         */
        sec_addr = (rpc_cn_port_any_t *)
            ((unsigned8 *)(header) + RPC_CN_PKT_SIZEOF_BIND_ACK_HDR);
        if (sec_addr->length > 1)
        {
            /*
             * Copy the primary group address.
             */
            rpc__naf_addr_copy (assoc_grp->grp_address, 
                                &assoc_grp->grp_secaddr, 
                                &(assoc->assoc_status));
            if (assoc->assoc_status != rpc_s_ok)
            {
                return (assoc->assoc_status);
            }
            
            /*
             * Set the endpoint in the primary group address to that
             * provided in the rpc_bind_ack PDU. The resulting address
             * is the secondary address. The endpoint given in the
             * rpc_bind PDU header is a string representation without
             * a NULL termination.
             */

            rpc__naf_addr_set_endpoint (
                    (unsigned_char_t *) sec_addr->s,
                                        &(assoc_grp->grp_secaddr),
                                        &(assoc->assoc_status));
            if (assoc->assoc_status != rpc_s_ok)
            {
                return (assoc->assoc_status);
            }
            assoc_grp->grp_max_assoc = RPC_C_ASSOC_GRP_MAX_ASSOCS;
        }
        else
        {
            /*
             * The NCA connection architecture states that if a
             * secondary address is not provided only one connection
             * may be made to a server.
             */
            assoc_grp->grp_max_assoc = 1;
        }
    }
    
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       authent3_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to optionally, if required for an authentication
**  mechanism, send an rpc_auth_3 PDU for the third leg of a three_way
**  authentication handshake. This PDU may be concatenated with a
**  subsequent PDU from the actual call, as long as message framing is
**  preserved.
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
**	sm             The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  SM is not changed
**			here but is passed in to avoid compile warnings
**			from rpc__cn_sm_eval_event().
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

INTERNAL unsigned32     authent3_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_assoc_sm_work_t      *assoc_sm_work;
    boolean                     old_server;
    rpc_protocol_version_t      *protocol_version;


    RPC_CN_DBG_RTN_PRINTF(CLIENT authent3_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    
    assoc = (rpc_cn_assoc_t *) spc_struct;
    assoc_sm_work = (rpc_cn_assoc_sm_work_t *) event_param;

    /*
     * Check the binding handle for protocol version information
     * If we know we are talking to an old server, this can save us work.
     */
    protocol_version = RPC_CN_ASSOC_CALL(assoc)->binding_rep->protocol_version;
    if ((protocol_version != NULL) &&
        (protocol_version->minor_version == RPC_C_CN_PROTO_VERS_COMPAT))
    {
        old_server = true;
    }
    else if (assoc->assoc_vers_minor == RPC_C_CN_PROTO_VERS_COMPAT)
    {
        old_server = true;
    }
    else
    {
        old_server = false;
    }

    /*
     * Format an rpc_bind PDU and send it to the server.
     */
    send_pdu (assoc,
              RPC_C_CN_PKT_AUTH3,
              NULL,
              FALSE,
              assoc_sm_work->grp_id,
              assoc_sm_work->sec_context,
              old_server,
              &(assoc->assoc_status));

    /*
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));
    sm_p->cur_state = RPC_C_CLIENT_ASSOC_OPEN;
    */
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       send_alt_context_req_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to send the rpc_alter_context PDU to the server.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The interface spec rep. This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     send_alt_context_req_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_assoc_sm_work_t      *assoc_sm_work;
    boolean			old_server;
    rpc_cn_sm_ctlblk_t		*sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT send_alt_context_req_action_rtn);
    
    assoc = (rpc_cn_assoc_t *) spc_struct;
    assoc_sm_work = (rpc_cn_assoc_sm_work_t *) event_param;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;  

    if (assoc->assoc_vers_minor == RPC_C_CN_PROTO_VERS_COMPAT)
    {
        old_server = true;
    }
    else
    {
        old_server = false;
    }

    /*
     * Remember the call id in case we get a fault on the alter context
     */
    assoc->alter_call_id = rpc_g_cn_call_id;

    /*
     * Format an rpc_alter_context PDU and send it to the server.
     */
    send_pdu (assoc, 
              RPC_C_CN_PKT_ALTER_CONTEXT,
              assoc_sm_work->pres_context,
              assoc_sm_work->reuse_context,
              assoc_sm_work->grp_id,
              assoc_sm_work->sec_context,
              old_server,
              &(assoc->assoc_status));
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));

    sm_p->cur_state = RPC_C_CLIENT_ASSOC_ACTIVE; 
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       abort_assoc_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to abort an association.
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     abort_assoc_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_sm_ctlblk_t		*sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT abort_assoc_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
 
    /*
     * Close the connection on the association.
     */
    rpc__cn_network_close_connect (assoc,
                                   &(assoc->assoc_status));
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));
    sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED; 
    return (assoc->assoc_status); 
}


/*
**++
**
**  ROUTINE NAME:       shutdown_allowed_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This action routine is a combination of SHUTDOWN_ALLOWED_PRED,  
**  which is a macro version of shutdown_allowed_pred_rtn(), 
**  plus the action routines set_shutdown_request_action_rtn()
**  and rem_mark_abort_action_rtn().   shutdown_allowed_action
**  _rtn() was newly created because of the changes to struct 
**  rcp_cn_sm_state_entry_t calling for a flat data structure. 
**  The open_state for events deallocate_req (11) and
**  shutdown_ind (14) require a predicate plus multiple action
**  routines.   Since these action routines are also called individually
**  for different states and events, we combined the particular
**  combination of action routines and renamed it to shutdown_allowed_
**  action_rtn(), applicable only to events 11 and 14, open_state.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the rpc_shutdown_ind PDU. 
**                      This is passed in as the
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
**  FUNCTION VALUE:     completion status returned in
**			assoc->assoc_status.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     shutdown_allowed_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;
    rpc_cn_assoc_grp_t  *assoc_grp;
    unsigned32  status; 

    RPC_CN_DBG_RTN_PRINTF(CLIENT shutdown_allowed_action_rtn);
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * This predicate macro determines whether there are no
     * context handles currently active, or whether there are
     * other associations in the open, active, or altered_
     * context wait states.  Macro writes the functional
     * result into status which is used later to determine
     * the updated state for sm->cur_state.
     */ 
    SHUTDOWN_ALLOWED_PRED(spc_struct, event_param, status);

   /* 
    * The special structure is a pointer to the association.
    */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    /*
     * Status is set in the predicate macro, SHUTDOWN_ALLOWED_PRED.  
     */  
    switch(status)
    {
    case 0:
    	{ 
	set_shutdown_request_action_rtn(spc_struct,event_param, sm);
    	sm_p->cur_state = RPC_C_CLIENT_ASSOC_OPEN; 
	break; 	
        } 
     case 1:
	{
    	rem_mark_abort_action_rtn (spc_struct, event_param, sm);
    	sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;
	break; 	
	}
    }/* end switch */ 
    return (assoc->assoc_status); 
}

/*
**++
**
**  ROUTINE NAME:       set_shutdown_request_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to set the shutdown requested predicate to true.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the rpc_shutdown_ind PDU. 
**                      This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     set_shutdown_request_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT set_shutdown_request_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * Indicate in the association flags that an
     * rpc_shutdown_request PDU has been received.
     */
    assoc->assoc_flags |= RPC_C_CN_ASSOC_SHUTDOWN_REQUESTED;

    sm_p->cur_state = RPC_C_CLIENT_ASSOC_ACTIVE; 
    return (assoc->assoc_status);

}


/*
**++
**
**  ROUTINE NAME:       incr_active_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to set the active predicate to true. The client
**  runtime allocated the association for the new call and its
**  callbacks. Only one call and its related callbacks may allocate an
**  association at a time.
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     incr_active_action_rtn
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT incr_active_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * Increment the association active reference counter.
     */
    RPC_CN_INCR_ACTIVE_CL_ACTION(assoc, sm_p);
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       decr_active_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to set the active predicate to false. The client
**  runtime deallocated the association when done with an alter context
**  request, a call, its callbacks, etc.
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     decr_active_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;
    unsigned32 status; 

    RPC_CN_DBG_RTN_PRINTF(CLIENT decr_active_action_rtn);
    
    /*
     * The predicate macro will fill in a value for status.  That value
     * determines which actions to take here, as well as which states
     * to put next in sm->cur_state.   Note that case 0 is the
     * decr_active_action_rtn(). 
     */  
    SHUTDOWN_ALLOWED_REQ_PRED(spc_struct, event_param, status);
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * The special structure is a pointer to the association. 
     */ 
    assoc = (rpc_cn_assoc_t *)spc_struct;

    /*
     * Status is set inside the predicate macro.  Use the
     * value of status to switch to the action routines.
     * Status is 0 if there are no active refs AND either shutdown 
     * not requested OR shutdown not allowed.  In this case,
     * call DECR_ACTIVE to reset the assoc->assoc_ref_count.
     * Status is 1 if no active refs AND shutdown requested AND 
     * shutdown allowed.  In this case, call DECR_REM_MARK_
     * ABORT which also increments the assoc ref_count and
     * calls rem_mark_abort_action_rtn().
     */  
    switch(status)
    {
    case 0:
	{ 
    	DECR_ACTIVE_ACTION(assoc);
	sm_p->cur_state = RPC_C_CLIENT_ASSOC_OPEN;
	break; 
	}
    case 1:
	{
     	/*
	 * Update state (sm->cur_state) inside the macro itself.  
	 */ 	
	DECR_REM_MARK_ABORT_ACTION(spc_struct, event_param, sm);
	break; 
 	}
     case 2:
	{
	DECR_ACTIVE_ACTION(assoc);
	sm_p->cur_state = RPC_C_CLIENT_ASSOC_ACTIVE;
	break; 
	}
    }  /* end switch */  
    return (assoc->assoc_status); 
}



/*
**++
**
**  ROUTINE NAME:       discon_calls_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to create a no_conn_ind event for each of the
**  calls currently using the association. This allows the call state
**  machinery to properly detect the disconnect and terminate the
**  calls. Note that in this implementation only one call can use an
**  association at a time.
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
**	sm             The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  SM is not changed
**			here but is passed in to avoid compile warnings
**			from rpc__cn_sm_eval_event().
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

INTERNAL unsigned32     discon_calls_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t      *assoc;

    RPC_CN_DBG_RTN_PRINTF(CLIENT discon_calls_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    rpc__cn_call_no_conn_ind (assoc->call_rep);

    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       mark_syntax_and_sec_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to mark the association with negotiated
**  transfer syntax and security.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing an rpc_bind_ack or
**                      rpc_alter_context_resp PDU. 
**                      This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     mark_syntax_and_sec_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t                  *assoc = NULL;
    rpc_cn_packet_t                 *header = NULL;
    unsigned32                      header_size;
    rpc_cn_pres_result_list_t       *pres_result_list = NULL;
    rpc_cn_syntax_t                 *pres_context = NULL;
    rpc_cn_sec_context_t            *sec_context = NULL;
    rpc_cn_auth_tlr_t               *auth_tlr = NULL;
    unsigned32                      i, local_auth_value_len;
    rpc_cn_port_any_t               *sec_addr = NULL;
    rpc_cn_bind_auth_value_priv_p_t priv_auth_value, local_auth_value;
    unsigned32                      st;
    rpc_cn_sm_ctlblk_t		    *sm_p = NULL;
    unsigned32			    status;
    unsigned8                       ptype;

    RPC_CN_DBG_RTN_PRINTF(CLIENT mark_syntax_and_sec_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    
    assoc->alter_call_id = -1;

    status = lastfrag_pred_rtn(spc_struct, event_param);
    if (status == 0)
    {
	process_frag_action_rtn(spc_struct, event_param, sm);
        sm_p->cur_state = RPC_C_CLIENT_ASSOC_ACTIVE;
    	return (assoc->assoc_status);
    }

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the PDU.
     */
    header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;
    header_size = ((rpc_cn_fragbuf_t *)event_param)->data_size;
    ptype = RPC_CN_PKT_PTYPE ( header );

    /*
     * To locate the presentation results in the PDU we'll have to
     * do some math. The secondary address endpoint comes before the
     * presentation results in the packet header and it is variable
     * sized. The secondary address length does not include the null
     * termination byte. The presentation results also has padding in the
     * header before it to ensure it is four byte aligned.
     */
    sec_addr = (rpc_cn_port_any_t *)
        ((unsigned8 *)(header) + RPC_CN_PKT_SIZEOF_BIND_ACK_HDR);
    pres_result_list = (rpc_cn_pres_result_list_t *) 
	RPC_CN_ALIGN_PTR(((unsigned8 *)sec_addr + 2 + sec_addr->length), 4);
    
    /*
     * Lookup the correct presentation context element by matching
     * the call ID in the header if there are results in the list.  If
     * there isn't, simply proceed by checking if there is an auth
     * trailer, since this may be a case where the security context is
     * being negotiated but the presentation context is not.
     */
    if (pres_result_list->n_results > 0)
    {
        rpc__cn_assoc_syntax_lkup_by_cl (assoc,
                                         RPC_CN_PKT_CALL_ID (header),
                                         &pres_context,
                                         &assoc->assoc_status);
        if (assoc->assoc_status == rpc_s_ok)
        {
            /*
             * Since we're only negotiating a transfer syntax for a single
             * abstract syntax we only have to check the first element of the
             * results list.
             */
            if (pres_result_list->pres_results[0].result == 
                RPC_C_CN_PCONT_ACCEPTANCE)
            {       
                pres_context->syntax_valid = true;
        
                /*
                 * We have an accepted transfer syntax. Note that
                 * the transfer syntax UUID is not saved. The stubs
                 * require only the transfer syntax vector index.
                 */
            
                /*
                 * To determine the syntax vector index the syntax vector
                 * will be scanned looking for a transfer syntax which matches
                 * the one just negotiated.
                 */

                for (i = 0; i < pres_context->syntax_vector->count; i++)
                {
                    if (RPC_CN_ASSOC_SYNTAX_EQUAL (&pres_context->syntax_vector->syntax_id[i],
                        &pres_result_list->pres_results[0].transfer_syntax,
                        &local_st))
                    {
                        pres_context->syntax_vector_index = i;
                        break;
                    }
                }
#if DEBUG
                if (i == pres_context->syntax_vector->count)
                {
		    
		    /*
		     * rpc_m_nts_not_found
		     * "(%s) Negotiated transfer syntax not found
		     * in presentation context element"
		     */
		    RPC_DCE_SVC_PRINTF ((
			DCE_SVC(RPC__SVC_HANDLE, "%s"),
			rpc_svc_cn_errors,
			svc_c_sev_fatal | svc_c_action_abort,
			rpc_m_nts_not_found,
			"mark_syntax_and_sec_action_rtn" ));
		}
#endif
            }
            else
            {
                /*
                 * A transfer syntax was not negotiated. Convert the reason code
                 * code in the header into a status code.
                 */
                pres_context->syntax_status = rpc__cn_assoc_pprov_to_status
                (pres_result_list->pres_results[0].reason); 
            }
        }
    }

    /*
     * If an authentication trailer is present on the packet,
     * find the appropriate security context element using the key
     * ID to match. Once found verify the server.
     */
    if (RPC_CN_PKT_AUTH_TLR_PRESENT (header))
    {
        auth_tlr = RPC_CN_PKT_AUTH_TLR (header, header_size);
        rpc__cn_assoc_sec_lkup_by_id (assoc,
                                      auth_tlr->key_id,
                                      &sec_context,
                                      &assoc->assoc_status);

        if (assoc->assoc_status == rpc_s_ok)
        {
            /*
             * A small song and dance to use the reconstructed
             * auth_value if we have one.
             */
            if (assoc->security.auth_buffer_info.auth_buffer != NULL)
            {
                rpc_cn_bind_auth_value_priv_p_t    auth_value;

                /*
                 * Make sure the piece from the last packet gets
                 * in to the reconstruction buffer.
                 */
                process_frag_action_rtn(spc_struct, event_param, sm);

                /*
                 * Use reconstruction buffer for auth_value.
                 */

                local_auth_value = (rpc_cn_bind_auth_value_priv_t *)
                            assoc->security.auth_buffer_info.auth_buffer;
                local_auth_value_len = assoc->security.auth_buffer_info.auth_buffer_len;

                /* Make sure we get these values from the packet we checksum */
                auth_value = (rpc_cn_bind_auth_value_priv_t *)
                                 auth_tlr->auth_value;
                local_auth_value->assoc_uuid_crc = auth_value->assoc_uuid_crc;
                local_auth_value->sub_type = auth_value->sub_type;
                local_auth_value->checksum_length = auth_value->checksum_length;
                
            }
            else
            {
                /*
                 * Only one packet received, use it.
                 */
                local_auth_value = (rpc_cn_bind_auth_value_priv_t *)
                                       auth_tlr->auth_value;
                local_auth_value_len = RPC_CN_PKT_AUTH_LEN(header);
            }
           
            RPC_CN_AUTH_VFY_SRVR_RESP (&assoc->security,
                                       sec_context,
                                       (pointer_t)local_auth_value,
                                       local_auth_value_len,
                                       &sec_context->sec_status);
            if (sec_context->sec_status != rpc_s_ok)
            {
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_SECURITY_ERRORS,
                                ("CN: call_rep->%x assoc->%x desc->%x server verification failed security_context->%x auth_type->%x auth_level->%x auth_len->%x st->%x\n",
                                assoc->call_rep,
                                assoc,
                                assoc->cn_ctlblk.cn_sock,
                                sec_context,
                                auth_tlr->auth_type,
                                auth_tlr->auth_level,
                                RPC_CN_PKT_AUTH_LEN (header),
                                sec_context->sec_status));
            }
            else
            {
                rpc_authn_protocol_id_t authn_protocol;

                /* 
                 * The response is OK. Use the 
                 * authentication level to check the packet.
                 * If a raw packet exists, use that one instead
                 * of the unpacked one.
                 * We want to use the cred_length of the packet we are
                 * doing the checksum on.
                 */
                priv_auth_value =
                    (rpc_cn_bind_auth_value_priv_t *) auth_tlr->auth_value;

                authn_protocol = RPC_CN_AUTH_CVT_ID_WIRE_TO_API (auth_tlr->auth_type, &st);
                if (st != rpc_s_ok)
                {
                    assoc->assoc_status = st;
                    goto DONE;
                }

                if (assoc->raw_packet_p != NULL)
                {
                    RPC_CN_AUTH_RECV_CHECK (authn_protocol,
                        &assoc->security,
                        sec_context,
                        (rpc_cn_common_hdr_p_t) assoc->raw_packet_p->data_p,
                        assoc->raw_packet_p->data_size,
                        priv_auth_value->cred_length,
                        auth_tlr,
                        0, /* dummy for unpack_ints */
                        &sec_context->sec_status);
		}
                else
                {
                    RPC_CN_AUTH_RECV_CHECK (authn_protocol,
                        &assoc->security,
                        sec_context,
                        (rpc_cn_common_hdr_p_t) header,
                        header_size,
                        priv_auth_value->cred_length,
                        auth_tlr,
                        0, /* dummy for unpack_ints */
                        &sec_context->sec_status);
                }
                if (sec_context->sec_status == rpc_s_ok)
                {
                    if (assoc->security.krb_message.length == 0)
                    {
                        sec_context->sec_state = RPC_C_SEC_STATE_COMPLETE;
                    }
                    else
                    {
                        sec_context->sec_state = RPC_C_SEC_STATE_INCOMPLETE;
                    }
                }
            }
        }
   
        /*
         * Free assembly buffer
         */
        if (assoc->security.auth_buffer_info.auth_buffer != NULL)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
                 ("(mark_syntax_and_sec_action_rtn) Free'd auth_buffer: %x\n",
                 assoc->security.auth_buffer_info.auth_buffer));

            RPC_MEM_FREE(assoc->security.auth_buffer_info.auth_buffer, 
                         RPC_C_MEM_CN_PAC_BUF);
            assoc->security.auth_buffer_info.auth_buffer = NULL;
            assoc->security.auth_buffer_info.auth_buffer_len = 0;
            assoc->security.auth_buffer_info.auth_buffer_max = 0;
        }
    }
    else
    {
            if ((assoc->assoc_flags & RPC_C_CN_ASSOC_CLIENT) &&
                ((ptype == RPC_C_CN_PKT_BIND_ACK) ||
                 (ptype == RPC_C_CN_PKT_ALTER_CONTEXT_RESP)) &&
                (assoc->assoc_flags & RPC_C_CN_ASSOC_AUTH_EXPECTED))
            {
	        /*
		 * if there's no auth trailer fallback to get the sec_context
		 * from the matching call
		 */
                rpc__cn_assoc_sec_lkup_by_cl(assoc,
                                             RPC_CN_PKT_CALL_ID (header),
                                             &sec_context,
                                             &assoc->assoc_status);
                if (assoc->assoc_status == rpc_s_ok)
                {
                    sec_context->sec_last_call_id = 0;
                    /* now let the auth plugin decide if a NULL trailer is ok */
                    RPC_CN_AUTH_VFY_SRVR_RESP(&assoc->security,
                                              sec_context,
                                              NULL,
                                              0,
                                              &sec_context->sec_status);
                    if (sec_context->sec_status != rpc_s_ok)
                    {
                        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_SECURITY_ERRORS,
                                ("CN: call_rep->%x assoc->%x desc->%x server verification failed security_context->%x auth_len->%x st->%x\n",
                                assoc->call_rep,
                                assoc,
                                assoc->cn_ctlblk.cn_sock,
                                sec_context,
                                RPC_CN_PKT_AUTH_LEN (header),
                                sec_context->sec_status));
                    }
                    else
		    {
                        if (assoc->security.krb_message.length == 0)
                        {
                            sec_context->sec_state = RPC_C_SEC_STATE_COMPLETE;
                        }
                        else
                        {
                            sec_context->sec_state = RPC_C_SEC_STATE_INCOMPLETE;
                        }
                    }
                }
                else
                {
                        RPC_DBG_PRINTF (rpc_e_dbg_general,
                                        RPC_C_CN_DBG_GENERAL,
                                ("CN: auth_info %x\n", assoc->call_rep->binding_rep->auth_info));
                        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: should not continue further with this PDU\n"));
                        assoc->assoc_status = rpc_s_authn_level_mismatch;
                }
            }
    }

DONE:
    /*
     * Deallocate the raw packet if it exists.
     */
    if (assoc->raw_packet_p != NULL)
    {
        rpc__cn_fragbuf_free (assoc->raw_packet_p);
        assoc->raw_packet_p = NULL;
    }

    /*
     * Queue a dummy fragbuf on the association receive queue to
     * wake up the client caller thread. Do it only if we have received
     * ALTER_CONTEXT because otherwise we may still be in the middle
     * of authentication phase in which case the caller thread has to wait.
     */
    if (ptype == RPC_C_CN_PKT_ALTER_CONTEXT_RESP)
    {
        assoc->assoc_flags &= ~RPC_C_CN_ASSOC_AUTH_EXPECTED;
	RPC_CN_ASSOC_WAKEUP (assoc);
	sm_p->cur_state = RPC_C_CLIENT_ASSOC_ACTIVE;
    }

    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       mark_abort_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to mark and abort an association.
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     mark_abort_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT mark_abort_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    
    /*
     * Note that since we are calling action routines from
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * would update sm->cur_state inappropriately for
     * the calling routine, mark_abort_action_rtn().
     */ 
    abort_assoc_action_rtn (spc_struct, event_param, sm);
    if (assoc->assoc_status != rpc_s_ok)
    {
	sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;
	return (assoc->assoc_status);  
    }

    mark_assoc_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;
    return (assoc->assoc_status);  
}

#ifdef NOT_USED

/***********************************************************************/
/*
 *
 * C L I E N T   A S S O C   P R E D I C A T E   R O U T I N E S
 *
 */
/*
**++
**
**  ROUTINE NAME:       authent3_pred_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Returns 1 if the association request requires an optional 3-leg
**  authentication handshake. Returns 0 if if didn't.
**
**  INPUTS:
**
**      spc_struct      The association. Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragment buffer containing the rpc_bind
**                      PDU. The special event related 
**                      parameter which is passed to the state
**                      machine event evaluation routine.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     0 if no 3-leg authentication handshake is
**                        being done.
**                      1 if a 3-leg authentication handshake is
**                        being done.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned8 authent3_pred_rtn 
(
  pointer_t       spc_struct __attribute__((ATTRIBUTE_UNUSED__)),
  pointer_t       event_param
)
{
    rpc_cn_packet_t     *header;
    rpc_cn_auth_tlr_t   *tlr;
    boolean32           three_way;
    unsigned32          st;
    rpc_authn_protocol_id_t authn_protocol;

    RPC_CN_DBG_RTN_PRINTF(CLIENT authent3_pred_rtn);

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind PDU.
     */
    header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;

    /*
     * The authentication length in the header indicates whether the
     * PDU contains an authentication trailer.
     */
    if (RPC_CN_PKT_AUTH_LEN (header) == 0)
    {
        return (0);
    }
    else
    {
        tlr = RPC_CN_PKT_AUTH_TLR (header, RPC_CN_PKT_FRAG_LEN (header));

        authn_protocol = RPC_CN_AUTH_CVT_ID_WIRE_TO_API (tlr->auth_type, &st);
        if (st != rpc_s_ok)
        {
            return (0);
        }

        RPC_CN_AUTH_THREE_WAY (authn_protocol, three_way);
        if (three_way)
        {
            return (1);
        }
        else
        {
            return (0);
        }
    }
}
#endif


/*
**++
**
**  MACRO NAME:		AUTHENT3_PRED       
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**  This a macro version of the authent3_pred_rtn predicate routine.
**  We added the macro version to avoid overhead associated with calling
**  the predicate function from within the action routines.
**  Macro set status to 1 if the association request requires an optional 3-leg
**  authentication handshake, otherwise, sets status to 0.
**      
**
**  INPUTS:
**
**      spc_struct      The association group. Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragment buffer containing the rpc_bind
**                      PDU. The special event related 
**                      parameter which is passed to the state
**                      machine event evaluation routine.
**
**	status		Instead of returning a value from the macro,
**			write the value calculated in the macro to
**			status.  Status' scope includes the routine
**			calling the macro.  Check status in the calling
**			routine to determine next state and in cases,
**			flow through the action routine. 
** 
**	tlr		Struct rpc_cn_auth_tlr_t.  Declared in the
**			calling routine.  Used in RPC_CN_AUTH_THREE_WAY.
**
**	st		Unsigned32.  Used internally to RPC_CN_AUTH_
**			THREE_WAY.
**
**	three_way	Boolean32.  Used internally to  RPC_CN_AUTH_
**			THREE_WAY.
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
**  FUNCTION VALUE:     0 if no 3-leg authentication handshake is
**                        being done.
**                      1 if a 3-leg authentication handshake is
**                        being done.
**
**  SIDE EFFECTS:       none
**
**--
**/
#define  AUTHENT3_PRED(spc_struct, event_param, status, tlr, st, three_way)\
{\
    RPC_CN_DBG_RTN_PRINTF(CLIENT authent3_pred_macro);\
    header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;\
    if (RPC_CN_PKT_AUTH_LEN (header) == 0)\
    {\
        status = 0;\
    }\
    else\
    {\
        rpc_authn_protocol_id_t authn_protocol; \
        tlr = RPC_CN_PKT_AUTH_TLR (header, RPC_CN_PKT_FRAG_LEN (header));\
        authn_protocol = RPC_CN_AUTH_CVT_ID_WIRE_TO_API (tlr->auth_type, &st); \
        if (st == rpc_s_ok) \
        {\
            RPC_CN_AUTH_THREE_WAY (authn_protocol, three_way);\
            if (three_way)\
            {\
                status = 1;\
            }\
            else\
            {\
                status = 0;\
            }\
        }\
        else\
        {\
            status = 0;\
        }\
    }\
}


/*
**++
**
**  ROUTINE NAME:       add_mark_set_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to add an association to an association group,
**  mark the assciation with the negotiated transfer syntax,
**  optionally perform the third leg of a three-way 
**  authentication handshake, and set the secondary address in the
**  association. This routine will also record the server's maximum
**  transmit and receive fragment sizes in the association
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragment buffer containing the PDU. This
**                      is passed in as the special event related
**                      parameter which was passed to the state
**                      machine evaluation routine. 
**
**  INPUTS/OUTPUTS:     
**
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     add_mark_set_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t      *assoc;
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_packet_t     *header;
    rpc_cn_fragbuf_t    *fragbuf;
    rpc_cn_auth_tlr_t   *tlr = NULL;
    boolean32           three_way = 0;
    unsigned32          local_st;
    rpc_cn_sm_ctlblk_t	*sm_p;
    unsigned32		status;
    rpc_binding_rep_p_t binding_rep = NULL;

    RPC_CN_DBG_RTN_PRINTF(CLIENT add_mark_set_action_rtn);
   
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    
    status = lastfrag_pred_rtn(spc_struct, event_param);
    if (status == 0)
    {
	process_frag_action_rtn(spc_struct, event_param, sm);
	sm_p->cur_state =  RPC_C_CLIENT_ASSOC_INIT_WAIT;
	return (assoc->assoc_status);
    }

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the received PDU.
     */
    fragbuf = (rpc_cn_fragbuf_t *) event_param;

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind_ack PDU.
     */
    header = fragbuf->data_p;

    /*
     * Use the group id passed back from the server to see if we
     * already have an association group.
     */
    assoc->assoc_grp_id = rpc__cn_assoc_grp_lkup_by_remid (RPC_CN_PKT_ASSOC_GROUP_ID (header), 
                                                           RPC_C_CN_ASSOC_GRP_CLIENT,
                                                           assoc->call_rep->binding_rep->rpc_addr,
                                                           &local_st);
    assoc_grp = RPC_CN_ASSOC_GRP (assoc->assoc_grp_id);
    if (assoc_grp == NULL)
    {
        /*
         * There's no existing group. Create a new one.
         */
        assoc->assoc_grp_id = rpc__cn_assoc_grp_alloc (assoc->cn_ctlblk.rpc_addr,
                                                       assoc->transport_info,
                                                       RPC_C_CN_ASSOC_GRP_CLIENT,
                                                       RPC_CN_PKT_ASSOC_GROUP_ID (header),
                                                       &(assoc->assoc_status));
        RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));
    }

    /*
     * Add the new association to the group.
     *
     * Note that since we are calling action routines from
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * called here, would update sm->cur_state inappropriately for
     * add_mark_set_action_rtn().
     */ 
    add_assoc_to_grp_action_rtn (spc_struct, NULL, sm);
    if (assoc->assoc_status != rpc_s_ok)
    {
       sm_p->cur_state =  RPC_C_CLIENT_ASSOC_OPEN;
       return (assoc->assoc_status);
    }

    /*
     * Mark the association with the negotiated syntax(es).
     */
    mark_syntax_and_sec_action_rtn (spc_struct, event_param, sm);
    if (assoc->assoc_status != rpc_s_ok)
    {
       sm_p->cur_state =  RPC_C_CLIENT_ASSOC_OPEN;
       return (assoc->assoc_status);
    }

    /*
     * Check if header signing has been requested _and_ server
     * confirms it is supported. Fail if it does not.
     */
    binding_rep = assoc->call_rep->binding_rep;
    if (binding_rep)
    {
	if (binding_rep->auth_info &&
            (binding_rep->auth_info->authn_flags &
             rpc_c_protect_flags_header_sign))
        {
            if (!(RPC_CN_PKT_FLAGS(header) & RPC_C_CN_FLAGS_SUPPORT_HEADER_SIGN))
            {
                assoc->assoc_status = rpc_s_auth_method;
                RPC_CN_ASSOC_CHECK_ST(assoc, &(assoc->assoc_status));
            }
	}
    }

    /*
     * Record the server's max transmit frag size as our max receive
     * frag size in the association. Record the server's max receive
     * frag size as our max transmit frag size. As per the NCA
     * Connection Architecture zero is a reserved value for the max
     * receive frag implying the default size: rpc_c_assoc_must_recv_frag_size.
     */
    if (RPC_CN_PKT_MAX_RECV_FRAG (header) == 0)
    {
        assoc->assoc_max_xmit_frag = RPC_C_ASSOC_MUST_RECV_FRAG_SIZE;
    }
    else
    {
        assoc->assoc_max_xmit_frag = RPC_CN_PKT_MAX_RECV_FRAG(header);
    }
    assoc->assoc_max_recv_frag = RPC_CN_PKT_MAX_XMIT_FRAG(header);

    /*
     * Record the servers minor version number in the association.
     * This will be used in all subsequent PDU's.
     */
    assoc->assoc_vers_minor = RPC_CN_PKT_VERS_MINOR(header);

    /*
     * Set the secondary address on the group using the secondary
     * address endpoint sent from the server.
     */
    set_secondary_addr_action_rtn (spc_struct, event_param, sm);
    if (assoc->assoc_status != rpc_s_ok)
    {
       sm_p->cur_state =  RPC_C_CLIENT_ASSOC_OPEN;
       return (assoc->assoc_status);
    }

    /*
     * Optionally perform three-way auth.  auth tlr set up in
     * AUTHENT3_PRED, used *again* in 
     */

    AUTHENT3_PRED(spc_struct, event_param, status, tlr, local_st, three_way);

    if (three_way)
    {
	rpc_cn_sec_context_t            *sec_context;
        RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
  		("3-way auth required\n"));
	rpc__cn_assoc_sec_lkup_by_id (assoc,
				      tlr->key_id,
				      &sec_context,
				      &assoc->assoc_status);

	if (assoc->assoc_status == rpc_s_ok)
	{
	    rpc_cn_assoc_sm_work_t assoc_sm_work;
	    assoc_sm_work.grp_id = assoc->assoc_grp_id.all;
	    assoc_sm_work.pres_context = NULL;
	    assoc_sm_work.sec_context = sec_context;

	    /*
	     * After sending AUTH3 we're not expecting more authentication
	     * steps so set the security state to complete. This will
	     * prevent from sending out ALTER_CONTEXT.
	     */
	    sec_context->sec_state = RPC_C_SEC_STATE_COMPLETE;

	    authent3_action_rtn(spc_struct, &assoc_sm_work, sm);

	    assoc->assoc_flags &= ~RPC_C_CN_ASSOC_AUTH_EXPECTED;
	    RPC_CN_ASSOC_WAKEUP (assoc);
	    sm_p->cur_state = RPC_C_CLIENT_ASSOC_ACTIVE;
	    return (assoc->assoc_status);
	}
    }

    /*
     * Queue a dummy fragbuf on the association receive queue to
     * wake up the client caller thread.
     */
    assoc->assoc_flags &= ~RPC_C_CN_ASSOC_AUTH_EXPECTED;
    RPC_CN_ASSOC_WAKEUP (assoc);
    sm_p->cur_state = RPC_C_CLIENT_ASSOC_ACTIVE;
    
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       rem_mark_abort_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to remove an association from an association
**  group, mark and abort it.
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     rem_mark_abort_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT rem_mark_abort_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
 
    rem_assoc_from_grp_action_rtn (spc_struct, event_param, sm);
    if (assoc->assoc_status != rpc_s_ok)
    {
	sm_p->cur_state =  RPC_C_CLIENT_ASSOC_CLOSED ;
	return (assoc->assoc_status); 
    }

    /*
     * Note that since we are calling action routines from
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * called here, would update sm->cur_state inappropriately for
     * rem_mark_abort_action_rtn().
     */ 
    abort_assoc_action_rtn (spc_struct, event_param, sm);
    if (assoc->assoc_status != rpc_s_ok)
    {
	sm_p->cur_state =  RPC_C_CLIENT_ASSOC_CLOSED ;
	return (assoc->assoc_status); 
    }

    mark_assoc_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state =  RPC_C_CLIENT_ASSOC_CLOSED ;
    return (assoc->assoc_status); 
}


/*
**++
**
**  ROUTINE NAME:       rem_mark_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to remove an association from an association
**  group and mark it.
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     rem_mark_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT rem_mark_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
   
    rem_assoc_from_grp_action_rtn (spc_struct, event_param, sm);
    if (assoc->assoc_status != rpc_s_ok)
    {
	sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;   
	return (assoc->assoc_status);
    }

    /*
     * Note that since we are calling action routines from
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * called here, would update sm->cur_state inappropriately for
     * rem_mark_action_rtn().
     */ 
    mark_assoc_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;   
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       rem_mark_discon_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to remove an association from an association
**  group, mark it and create a no connection indication event for
**  all calls currently using the association.
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     rem_mark_discon_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT rem_mark_discon_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    
    /*
     * Note that since we are calling action routines from
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * called here, would update sm->cur_state inappropriately for
     * rem_mark_discon_action_rtn().
     */ 
    rem_assoc_from_grp_action_rtn (spc_struct, event_param, sm);
    mark_assoc_action_rtn (spc_struct, event_param, sm );
    discon_calls_action_rtn (spc_struct, event_param, sm);
    sm_p->cur_state = RPC_C_CLIENT_ASSOC_CALL_DONE_WAIT; 
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       decr_rem_mark_abort_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to decrement the active references, remove an
**  association from an association group, mark and abort it.
**  Note that this action is not called explicitly from the state
**  tables any longer.  It is called from within other actions.
**  Update state not here, but in the routines calling decr_rem
**  mark_abort_action_rtn(). 
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
**	sm             The control block from the event evaluation
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     decr_rem_mark_abort_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p ATTRIBUTE_UNUSED;
   
    RPC_CN_DBG_RTN_PRINTF(CLIENT decr_rem_mark_abort_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    DECR_ACTIVE_ACTION(assoc);
    if (assoc->assoc_status != rpc_s_ok)
    {
       	return (assoc->assoc_status);
    }
    rem_mark_abort_action_rtn (spc_struct, event_param, sm);
    return (assoc->assoc_status);
} 


/*
**++
**
**  ROUTINE NAME:       process_frag_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to process multiple rpc_bind_ack or alter_context_resp
**  PDU packets and reconstruct the security information.
**
**  If we call this routine, this packet is expected to be one of
**  a series of packets containing security information.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the rpc_bind_ack or
**                      alter_context_resp PDU. 
**                      This is passed in as the special event related 
**                      parameter which was passed to the state machine 
**                      evaluation routine.
**
**  INPUTS/OUTPUTS:     none
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

INTERNAL unsigned32     process_frag_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t                      *assoc;
    rpc_cn_packet_t                     *req_header;
    rpc_cn_bind_auth_value_priv_t       *auth_value;
    unsigned32                          auth_value_len;
    rpc_cn_auth_tlr_t                   *auth_tlr;
    unsigned8                           *auth_buffer;
    unsigned32                          auth_buffer_max;
    unsigned32                          auth_buffer_len;


    RPC_CN_DBG_RTN_PRINTF(CLIENT process_frag_action_rtn);

    /*
     * We are not calling this routine directly from the
     * state machine, but indirectly through another routine
     * which is in the state machine.  The 'sm' parameter
     * is not updated internally but we add it to process_
     * frag_action_rtn for consistency purposes.
     */
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind_ack ot alter_context_resp PDU.
     */
    req_header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;

    /*
     * This is a place where we stash the auth information while we
     * reconstruct it.
     */
    auth_buffer = assoc->security.auth_buffer_info.auth_buffer;
    auth_buffer_len = assoc->security.auth_buffer_info.auth_buffer_len;
    auth_buffer_max = assoc->security.auth_buffer_info.auth_buffer_max;

    if (auth_buffer == NULL)
    {
        /*
         * If we get here, odds are we are going to get maybe 1 or 2 more
         * packets, so get some space to save time later.
         */
        auth_buffer_max = RPC_C_CN_LARGE_FRAG_SIZE * 3;

        RPC_MEM_ALLOC(auth_buffer, unsigned8 *, 
                      auth_buffer_max,
                      RPC_C_MEM_CN_PAC_BUF,
                      RPC_C_MEM_WAITOK);

        RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
  ("(process_frag_action_rtn) Alloc'd auth_buffer: %x, auth_buffer_max = %d\n",
                        auth_buffer,
                        auth_buffer_max));
    }

    if ((RPC_CN_PKT_AUTH_LEN (req_header) + auth_buffer_len) > auth_buffer_max)
    {
        auth_buffer_max += RPC_C_CN_LARGE_FRAG_SIZE;

        RPC_MEM_REALLOC(auth_buffer, unsigned8 *,
                        auth_buffer_max,
                        RPC_C_MEM_CN_PAC_BUF,
                        RPC_C_MEM_WAITOK);

        RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
("(process_frag_action_rtn) Realloc'd auth_buffer: %x, auth_buffer_max = %d\n",
                        auth_buffer,
                        auth_buffer_max));
    }

    /*
     * Concatenate this security info on to the buffer.
     * 
     * We have to watch out for the checksum at the end of the
     * auth trailer, we only want to recover the KRB_AP_REQ message.
     */
    auth_tlr = RPC_CN_PKT_AUTH_TLR(req_header,RPC_CN_PKT_FRAG_LEN (req_header));
    auth_value = (rpc_cn_bind_auth_value_priv_t *)auth_tlr->auth_value;
    auth_value_len = RPC_CN_PKT_AUTH_LEN (req_header) - 
                         auth_value->checksum_length;

    /*
     * For the first packet, copy the header info, for the rest
     * we just need the credential fragment.  We also update
     * the cred_length field in the assembly buffer.
     */

    if (auth_buffer_len == 0)
    {
        memcpy(auth_buffer, auth_value, auth_value_len);
    }
    else
    {
        auth_value_len -= RPC_CN_PKT_SIZEOF_BIND_AUTH_VAL;
        assert(auth_value_len == auth_value->cred_length);
        memcpy(auth_buffer+auth_buffer_len, 
               auth_value->credentials, 
               auth_value->cred_length);
        ((rpc_cn_bind_auth_value_priv_t *)auth_buffer)->cred_length += auth_value->cred_length;
    }

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
    ("(process_frag_action_rtn) Copied to auth_buffer: %x, auth_buffer_len=%d, auth_value_len=%d, auth_buffer_max=%d\n", 
    auth_buffer, auth_buffer_len, auth_value_len, auth_buffer_max));

    auth_buffer_len += auth_value_len;

    
    /*
     * Update our per-association data
     */
    assoc->security.auth_buffer_info.auth_buffer = auth_buffer;
    assoc->security.auth_buffer_info.auth_buffer_len = auth_buffer_len;
    assoc->security.auth_buffer_info.auth_buffer_max = auth_buffer_max;

    return (rpc_s_ok);
}


/*
**++
**
**  ROUTINE NAME:       retry_assoc_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to retry an BIND operation with a RPC version 5.0 server.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The rpc_bind_nak packet.
**                      This is passed in as the special event related 
**                      parameter which was passed to the state machine 
**                      evaluation routine.  It is ignored.
**
**  INPUTS/OUTPUTS:     none
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

INTERNAL unsigned32     retry_assoc_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t                      *assoc;
    unsigned32                          status;
    rpc_cn_sm_ctlblk_t 			*sm_p;

    RPC_CN_DBG_RTN_PRINTF(CLIENT retry_assoc_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
   
    status = version_mismatch_pred_rtn(spc_struct, event_param);
    if ( status == 0)
    {
	mark_abort_action_rtn(spc_struct, event_param, sm);
    	sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;
    	return (assoc->assoc_status);  
    }

    /*
     * A bit of defensive code in case a 5.0 server sends
     * multiple BIND_NAKs back instead of closing the connection.
     */
    if (assoc->assoc_vers_minor == RPC_C_CN_PROTO_VERS_COMPAT)
    {
    	sm_p->cur_state = RPC_C_CLIENT_ASSOC_INIT_WAIT;
        return(rpc_s_ok);
    }

    /*
     * Mark the association and the binding handle with version info
     */
    assoc->assoc_vers_minor = RPC_C_CN_PROTO_VERS_COMPAT;

    rpc__binding_prot_version_alloc(
                &(RPC_CN_ASSOC_CALL(assoc)->binding_rep->protocol_version),
                RPC_C_CN_PROTO_VERS,
                RPC_C_CN_PROTO_VERS_COMPAT,
                &status);

    if (status != rpc_s_ok)
    {
    	sm_p->cur_state = RPC_C_CLIENT_ASSOC_INIT_WAIT;
        return(status);
    }

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("CN: assoc->%x Falling back to version 5.0 protocol\n",
                   assoc));

    /*
     * Format an rpc_bind PDU and send it to the server.
     */
    send_pdu (assoc,
              RPC_C_CN_PKT_BIND,
              assoc->assoc_sm_work->pres_context,
              assoc->assoc_sm_work->reuse_context,
              assoc->assoc_sm_work->grp_id,
              assoc->assoc_sm_work->sec_context,
              true,
              &(assoc->assoc_status));
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));

    sm_p->cur_state = RPC_C_CLIENT_ASSOC_INIT_WAIT;
    return (rpc_s_ok);
}


/*
**++
**
**  ROUTINE NAME:       illegal_event_abort_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to ignore an illegal event and abort an association.
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
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     illegal_event_abort_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;
    dcethread* current_thread_id;

    RPC_CN_DBG_RTN_PRINTF(CLIENT illegal_event_abort_action_rtn);
    
    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    
    /*
     * "Illegal state transition detected in CN client association
     * state machine [cur_state: %s, cur_event: %s, assoc: %x]"
     */
    RPC_DCE_SVC_PRINTF ((
	DCE_SVC(RPC__SVC_HANDLE, "%s%s%x"),
	rpc_svc_cn_state,
	svc_c_sev_warning | svc_c_action_none,
        rpc_m_cn_ill_state_trans_ca,
rpc_g_cn_assoc_client_states[assoc->assoc_state.cur_state-RPC_C_CN_STATEBASE],
rpc_g_cn_assoc_client_events[assoc->assoc_state.cur_event-RPC_C_CN_STATEBASE],
        assoc ));

    /*
     * Note that since we are calling action routines from
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * would update sm->cur_state inappropriately for
     * the calling routine, illegal_event_abort_action_rtn().
     */ 
    abort_assoc_action_rtn (spc_struct, event_param, sm);
    if (assoc->assoc_status != rpc_s_ok)
    {
	sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;
	return (assoc->assoc_status);  
    }

    /*
     * Mark the association based on the event currently being
     * processed.
     *
     * For now, we just set it to rpc_s_protocol_error.
     */
    switch (assoc->assoc_state.cur_event)
    {
    case RPC_C_ASSOC_REQ:               /* user         */
    case RPC_C_ASSOC_ABORT_REQ:         /* user         */
    case RPC_C_ASSOC_REQUEST_CONN_ACK:  /* network      */
    case RPC_C_ASSOC_REQUEST_CONN_NACK: /* network      */
    case RPC_C_ASSOC_NO_CONN_IND:       /* network      */
    case RPC_C_ASSOC_ACCEPT_CONF:       /* network      */
    case RPC_C_ASSOC_REJECT_CONF:       /* network      */
    case RPC_C_ASSOC_ALTER_CONTEXT_REQ: /* user         */
    case RPC_C_ASSOC_ALTER_CONTEXT_CONF:/* network      */
    case RPC_C_ASSOC_ALLOCATE_REQ:      /* user         */
    case RPC_C_ASSOC_DEALLOCATE_REQ:    /* user         */
    case RPC_C_ASSOC_SHUTDOWN_REQ:      /* user */
    case RPC_C_ASSOC_LOCAL_ERROR:       /* internal     */
    case RPC_C_ASSOC_CALLS_DONE:        /* user         */
    case RPC_C_ASSOC_SHUTDOWN_IND:      /* network      */
    default:
        assoc->assoc_status = rpc_s_protocol_error;
        break;
    }

    /*
     * Wake up any threads blocked waiting for receive data.
     */
    current_thread_id = dcethread_self();
    if (dcethread_equal (current_thread_id,
                       assoc->cn_ctlblk.cn_rcvr_thread_id))
    {
        RPC_CN_ASSOC_WAKEUP (assoc);
    }
    sm_p->cur_state = RPC_C_CLIENT_ASSOC_CLOSED;
    return (assoc->assoc_status); 
}



/*
**++
**
**  ROUTINE NAME:       send_pdu
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This routine will allocate, format and send either an rpc_bind or
**  rpc_alter_context PDU.
**
**  The PDU may contain security credentials that do not fit in
**  a large fragbuf.  Thus this routine will fragment this info
**  and send it in pieces.
**
**  INPUTS:
**
**      assoc           The association.
**      pdu_type        The PDU type being formatted.
**      pres_context    The presentation context element containing
**                      the client's negotiation information, NULL if no negotiation.
**      grp_id          The association group id to be place in the PDU.
**      sec_context     The security context element containing
**                      the client's negotiation information, NULL if no negotiation.
**      old_server      Indicates if we know we are talking to an RPC 5.0 server
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

INTERNAL void send_pdu 
(
  rpc_cn_assoc_p_t        assoc,
  unsigned32              pdu_type,
  rpc_cn_syntax_p_t       pres_context,
  boolean                 reuse_context,
  unsigned32              grp_id,
  rpc_cn_sec_context_p_t  sec_context,
  boolean                 old_server,
  unsigned32              *st
)
{
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_packet_t             *header;
    rpc_cn_pres_cont_list_t     *pres_cont_list = NULL;
    unsigned32                  header_size;
    rpc_cn_auth_tlr_t           *auth_tlr;
    unsigned32                  auth_len;
    unsigned32                  auth_space;
    unsigned32                  i;
    rpc_cn_assoc_grp_t          *assoc_grp ATTRIBUTE_UNUSED;
    boolean			first_frag, done;
    unsigned8			flags = 0;
    unsigned32                  auth_len_remain;
    pointer_t                   last_auth_pos;
    static unsigned32           rpc_g_next_bind_key_id = 0;
    unsigned8			version;

    RPC_CN_DBG_RTN_PRINTF (CLIENT send_pdu);
    
    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,("CLIENT send_pdu\n"));

    /*
     * Allocate a large fragbuf. We won't expend any more time here
     * trying to optimize to allocate a small fragbuf. 
     */
    RPC_CN_FRAGBUF_ALLOC (fragbuf, rpc_g_cn_large_frag_size, st);
    header = (rpc_cn_packet_t *)fragbuf->data_p;

    if (pdu_type != RPC_C_CN_PKT_AUTH3)
    {
	header_size = RPC_CN_PKT_SIZEOF_BIND_HDR;

	/*
	 * Clear the control fields of the presentation context list.
	 */
	pres_cont_list = (rpc_cn_pres_cont_list_t *)
	    ((unsigned8 *)(header) + header_size);
	memset (pres_cont_list, 0, sizeof (rpc_cn_pres_cont_list_t));
    }
    else
    {
	header_size = RPC_CN_PKT_SIZEOF_AUTH3_HDR;
    }

    /*
     * Format the presentation context list. An assumption here is
     * that the presentation context list, if specified, will contain only
     * a single abstract syntax. If there is no presentation
     * negotiation being performed there will be zero abstract
     * syntaxes in the list.
     */
    if (pres_context != NULL && pres_cont_list != NULL)
    {
        /*
         * A presentation negotiation will be done. Add the
         * size of the presentation context list to the header size.
         */
        header_size += sizeof (rpc_cn_pres_cont_list_t) +
                       (sizeof (rpc_cn_pres_syntax_id_t) *
                       (pres_context->syntax_vector->count - 1)); 
        pres_cont_list->n_context_elem = 1;

        if (!reuse_context)
        {
            pres_context->syntax_pres_id = RPC_CN_ASSOC_CONTEXT_ID (assoc)++;
        }

        pres_cont_list->pres_cont_elem[0].pres_context_id = pres_context->syntax_pres_id;
        pres_cont_list->pres_cont_elem[0].n_transfer_syn = pres_context->syntax_vector->count;
        pres_cont_list->pres_cont_elem[0].abstract_syntax.id =  pres_context->syntax_abstract_id.id;
        pres_cont_list->pres_cont_elem[0].abstract_syntax.version = pres_context->syntax_abstract_id.version;

#ifdef DEBUG
        if (RPC_DBG2 (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL))
        {
            unsigned_char_t     *abstract;
            unsigned32          st;

            dce_uuid_to_string (&pres_context->syntax_abstract_id.id,
                            &abstract, 
                            &st);
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                            ("CN: call_rep->%x assoc->%x desc->%x negotiating for abstract syntax->%s,%x context_id->%x call_id->%x\n",
                             assoc->call_rep,
                             assoc,
                             assoc->cn_ctlblk.cn_sock,
                             abstract,
                             pres_context->syntax_abstract_id.version,
                             RPC_CN_ASSOC_CONTEXT_ID (assoc),
                             rpc_g_cn_call_id));
            rpc_string_free (&abstract, &st);
        }    
#endif

        for (i = 0; i < pres_context->syntax_vector->count; i++)
        {
            pres_cont_list->pres_cont_elem[0].transfer_syntaxes[i].id = 
                pres_context->syntax_vector->syntax_id[i].id;
            pres_cont_list->pres_cont_elem[0].transfer_syntaxes[i].version = 
                pres_context->syntax_vector->syntax_id[i].version;

#ifdef DEBUG
            if (RPC_DBG2 (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL))
            {
                unsigned_char_t     *transfer;
                unsigned32          st;
                
                dce_uuid_to_string (&pres_context->syntax_vector->syntax_id[i].id,
                                &transfer, 
                                &st);
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: call_rep->%x assoc->%x desc->%x transfer_syntax[%x]->%s,%x\n",
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock,
                                 i,
                                 transfer,
                                 pres_context->syntax_vector->syntax_id[i].version));
                rpc_string_free (&transfer, &st);
            }    
#endif

        }

        if (!reuse_context)
        {
            pres_context->syntax_call_id = rpc_g_cn_call_id;
        }
    }
    else if (pdu_type != RPC_C_CN_PKT_AUTH3)
    {
        /*
         * No negotiation is required. Send a presentation context
         * list with no elements. Note that the presentation context
         * list element structure comes with one built in presentation
         * context element in the array. We'll subtract this off.
         */
        header_size += sizeof (rpc_cn_pres_cont_list_t) -
                       sizeof (rpc_cn_pres_cont_elem_t);
    }

    if (pdu_type != RPC_C_CN_PKT_AUTH3)
    {
	/*
	 * Set up common entried in the PDU.
	 */
	RPC_CN_PKT_MAX_XMIT_FRAG (header) = rpc_g_cn_large_frag_size;
	RPC_CN_PKT_MAX_RECV_FRAG (header) = rpc_g_cn_large_frag_size;

	RPC_CN_PKT_ASSOC_GROUP_ID (header) = grp_id;

	/* use negotiated minor version number */
	assoc->bind_packets_sent = 0;
    } else {
	RPC_CN_PKT_AUTH3_MAX_XMIT_FRAG (header) = rpc_g_cn_large_frag_size;
	RPC_CN_PKT_AUTH3_MAX_RECV_FRAG (header) = rpc_g_cn_large_frag_size;
    }

    /* use negotiated minor version number */
    version = assoc->assoc_vers_minor;

    if (sec_context == NULL)
    {
        /*
         * No security, this is the first and last fragment we send
         */

        RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
            ("(send_pdu) sec_context == NULL, no security to deal with.\n"));

        auth_len = 0;
        flags = RPC_C_CN_FLAGS_FIRST_FRAG | RPC_C_CN_FLAGS_LAST_FRAG;

        fragbuf->data_size = header_size;
        rpc__cn_pkt_format_common (header, 
                                   pdu_type, 
                                   flags,
                                   fragbuf->data_size, 
                                   auth_len,
                                   rpc_g_cn_call_id,
                                   version);
        rpc_g_cn_call_id++;
        rpc__cn_assoc_send_fragbuf (assoc, fragbuf, sec_context, true, st);
        assoc->bind_packets_sent++;
        return;
    }

    /*
     * We are expecting a auth_trailer in the bind_ack or
     * alter_context_response PDU.
     */
    assoc->assoc_flags |= RPC_C_CN_ASSOC_AUTH_EXPECTED;

    /*
     * We have security info to deal with.  Calculate the
     * size of the authentication trailer. The header size up to
     * this point should already be a multiple of 4 meaning the
     * auth trailer will fall on a 4-byte boundary. 
     */
    /* header_size = (header_size + 3) & ~0x03; */
    auth_tlr = (rpc_cn_auth_tlr_t *) ((unsigned8 *)(header) + header_size);
    memset(auth_tlr, 0, RPC_CN_PKT_SIZEOF_COM_AUTH_TLR);
    header_size +=  RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;
    auth_len = rpc_g_cn_large_frag_size - header_size;

    /*
     * Fill in common auth header
     */
    auth_tlr->auth_type = RPC_CN_AUTH_CVT_ID_API_TO_WIRE (sec_context->sec_info->authn_protocol, st);
    auth_tlr->auth_level = sec_context->sec_info->authn_level;

    /*
     * We normally take the key_id from the group.
     * In the case of a BIND, we don't have a group
     * yet.  So we use a global variable instead.
     * We already have the CN mutex locked.
     */
    if (pdu_type != RPC_C_CN_PKT_AUTH3 && sec_context->sec_state != RPC_C_SEC_STATE_INCOMPLETE)
    {
        sec_context->sec_key_id = rpc_g_next_bind_key_id++;
    }
    auth_tlr->key_id = sec_context->sec_key_id;

    /*
     * This is the tricky part.  If our sec_context is longer that
     * the space we have remaining, chop it in to pieces.
     */
    first_frag = true;
    done = false;
    last_auth_pos = NULL;
    auth_len_remain = 0;
    if (pdu_type == RPC_C_CN_PKT_BIND || pdu_type == RPC_C_CN_PKT_AUTH3)
    {
        /* enforce the reason for all this code  */
        auth_space = rpc_g_cn_large_frag_size - header_size;
    }
    else
    {
        /* not likely that we will have to break this up */
        auth_space = RPC_CN_ASSOC_MAX_XMIT_FRAG(assoc) - header_size;
    }

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_FRAG_BIND))
    {
        char *x;
        auth_space = atoi(((x = getenv("BIND_FRAG")) == NULL ? "50" : x));
    }
#endif

    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
                   ("(send_pdu) Sending %s request, auth_space=%u\n",
                    rpc__cn_pkt_name(pdu_type), auth_space));

    sec_context->sec_last_call_id = rpc_g_cn_call_id;

    while (!done)
    {
        auth_len = auth_space;
        flags = 0;

        if (first_frag)
        {
            flags |= RPC_C_CN_FLAGS_FIRST_FRAG;
            first_frag = false;
        }

        (void) memset(auth_tlr->auth_value, 0, auth_len);

        RPC_CN_AUTH_FMT_CLIENT_REQ (&assoc->security, 
                                    sec_context,
                                    (pointer_t)auth_tlr->auth_value, 
                                    &auth_len,       
                                    &last_auth_pos,
                                    &auth_len_remain,
                                    old_server,
                                    st);

        if (*st != rpc_s_ok)
        {
            return;
        }

        /* auth_len now has length of auth_value */

        RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
("(send_pdu) After FMT_CLIENT_REQ, header_size=%u, auth_len=%u, auth_len_remain=%u\n", 
            header_size, auth_len, auth_len_remain));

        if (auth_len_remain == 0) {
            flags = flags | RPC_C_CN_FLAGS_LAST_FRAG;
            done = true;
        }

        /*
         * Request header signing support if the corresponding
         * authentication flag is set
         */
        if (sec_context &&
            sec_context->sec_info->authn_flags & rpc_c_protect_flags_header_sign)
        {
            if (pdu_type == RPC_C_CN_PKT_BIND ||
                pdu_type == RPC_C_CN_PKT_ALTER_CONTEXT ||
		pdu_type == RPC_C_CN_PKT_AUTH3)
            {
                flags |= RPC_C_CN_FLAGS_SUPPORT_HEADER_SIGN;
            }
        }

        fragbuf->data_size = header_size + auth_len;
        rpc__cn_pkt_format_common (header, 
                                   pdu_type, 
                                   flags, 
                                   fragbuf->data_size, 
                                   auth_len,
                                   rpc_g_cn_call_id,
                                   version);
        /*
         * Send the buffer and if done, free it
         */
        rpc__cn_assoc_send_fragbuf (assoc, fragbuf, sec_context, done, st);
        assoc->bind_packets_sent++;

        RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
       ("(send_pdu) SENT fragbuf, data_size=%u, first_frag=%s, last_frag=%s st=0x%x\n", 
            fragbuf->data_size, 
            (flags & RPC_C_CN_FLAGS_FIRST_FRAG) ? "true": "false", 
            (flags & RPC_C_CN_FLAGS_LAST_FRAG) ? "true": "false",
            *st));

        if (*st != rpc_s_ok)
        {
            break;
        }

    }  /* while !done */

    rpc_g_cn_call_id++;
}
