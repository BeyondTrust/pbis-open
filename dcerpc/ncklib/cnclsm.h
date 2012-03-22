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
#ifndef _CNCALLSM_H
#define _CNCALLSM_H 1
/*
**
**  NAME
**
**      cnclsm.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types/constants internal to the NCA Connection
**  RPC Protocol Service client call state machine.
**
**
*/


/***********************************************************************/
/*
 * C L I E N T   C A L L   S T A T E S
 */
/*
 * State values are incremented by 100 to distinguish them from
 * action routine indexes which are all < 100.  This was done as
 * an efficiency measure to the engine, rpc__cn_sm_eval_event().
 */ 
#define RPC_C_CLIENT_CALL_INIT              100
#define RPC_C_CLIENT_CALL_ASSOC_ALLOC_WAIT  101 
#define RPC_C_CLIENT_CALL_STUB_WAIT         102
#define RPC_C_CLIENT_CALL_REQUEST           103
#define RPC_C_CLIENT_CALL_RESPONSE          104
#define RPC_C_CLIENT_CALL_CALL_COMPLETED    105
#define RPC_C_CLIENT_CALL_CFDNE             106
#define RPC_C_CLIENT_CALL_CALL_FAILED       107
#define RPC_C_CLIENT_CALL_STATES            108


/***********************************************************************/
/*
 * C L I E N T / S E R V E R   C A L L   E V E N T S
 */

/*
 * Events common to both client and server state machines
 */
#define RPC_C_CALL_SEND                     100 
#define RPC_C_CALL_TRANSMIT_REQ             100    /* client */
#define RPC_C_CALL_RPC_RESP                 100    /* server */
#define RPC_C_CALL_RECV                     101  
#define RPC_C_CALL_RPC_CONF                 101   /* client */
#define RPC_C_CALL_RPC_IND                  101   /* server */
#define RPC_C_CALL_FAULT_DNE                102  
#define RPC_C_CALL_FAULT                    103  
#define RPC_C_CALL_LOCAL_ALERT              104  
#define RPC_C_CALL_END                      105 

/*
 * Events only applicable to client state machine
 */
#define RPC_C_CALL_ALLOC_ASSOC_ACK          106
#define RPC_C_CALL_ALLOC_ASSOC_NAK          107
#define RPC_C_CALL_START_CALL               108
#define RPC_C_CALL_LAST_TRANSMIT_REQ        109
#define RPC_C_CALL_LOCAL_ERR                110
#define RPC_C_CALL_ALERT_TIMEOUT            111 
#define RPC_C_CALL_CLIENT_EVENTS            112 

/*
 * Events only applicable to server state machine
 */
#define RPC_C_CALL_REMOTE_ALERT_IND         106
#define RPC_C_CALL_ORPHANED                 107 
#define RPC_C_CALL_SERVER_EVENTS            108


/***********************************************************************/
/*
 * C L I E N T   C A L L   T A B L E S
 */
EXTERNAL rpc_cn_sm_state_entry_p_t rpc_g_cn_client_call_sm [];
EXTERNAL rpc_cn_sm_action_fn_t rpc_g_cn_client_call_action_tbl [];

#ifdef DEBUG
EXTERNAL char   *rpc_g_cn_call_client_events [];
EXTERNAL char   *rpc_g_cn_call_client_states [];
#endif

/***********************************************************************/
/*
 * S E R V E R   C A L L   S T A T E S
 */
#define RPC_C_SERVER_CALL_INIT              100 
#define RPC_C_SERVER_CALL_CALL_REQUEST      101 
#define RPC_C_SERVER_CALL_CALL_RESPONSE     102  
#define RPC_C_SERVER_CALL_CALL_COMPLETED    103  


/***********************************************************************/
/*
 * S E R V E R   C A L L   T A B L E S
 */
EXTERNAL rpc_cn_sm_state_entry_p_t rpc_g_cn_server_call_sm [];
EXTERNAL rpc_cn_sm_action_fn_t     rpc_g_cn_server_call_action_tbl [];

#ifdef DEBUG
EXTERNAL char   *rpc_g_cn_call_server_events [];
EXTERNAL char   *rpc_g_cn_call_server_states [];
#endif

/*
 * Action routine to invoke in case of a protocol error detected
 * during an illegal state transition.
 */
PRIVATE unsigned32     rpc__cn_call_sm_protocol_error (
        pointer_t /* sc_struct */,
        pointer_t /* event_param */,
	pointer_t /* sm		 */
    );


/***********************************************************************/
/*
 * R P C _ C N _ C A L L _ S M _ T R C
 */
#ifdef DEBUG
#define RPC_CN_CALL_SM_TRC(crep, event_id, id)\
{\
    if (RPC_CALL_IS_CLIENT((rpc_call_rep_t *)(crep)))\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_CALL_SM_TRACE, \
                        ("STATE CLIENT CALL:   %x state->%s event->%s\n",\
                         id, \
                         rpc_g_cn_call_client_states[(crep)->call_state.cur_state-RPC_C_CN_STATEBASE],\
                         rpc_g_cn_call_client_events[event_id-RPC_C_CN_STATEBASE]));\
    }\
    else\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_CALL_SM_TRACE, \
                        ("STATE SERVER CALL:   %x state->%s event->%s\n",\
                         id, \
                         rpc_g_cn_call_server_states[(crep)->call_state.cur_state-RPC_C_CN_STATEBASE],\
                         rpc_g_cn_call_server_events[event_id-RPC_C_CN_STATEBASE]));\
    }\
}
#else
#define RPC_CN_CALL_SM_TRC(crep, event_id, id)
#endif


/***********************************************************************/
/*
 * R P C _ C N _ C A L L _ S M _ T R C _ S T A T E
 */
#ifdef DEBUG
#define RPC_CN_CALL_SM_TRC_STATE(crep, id)\
{\
    if (RPC_CALL_IS_CLIENT((rpc_call_rep_t *)(crep)))\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_CALL_SM_TRACE, \
                        ("STATE CLIENT CALL:   %x new state->%s\n",\
                         id, \
                         rpc_g_cn_call_client_states[(crep)->call_state.cur_state-RPC_C_CN_STATEBASE])); \
    }\
    else\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_CALL_SM_TRACE, \
                        ("STATE SERVER CALL:   %x new state->%s\n",\
                         id, \
                         rpc_g_cn_call_server_states[(crep)->call_state.cur_state-RPC_C_CN_STATEBASE])); \
    }\
}
#else
#define RPC_CN_CALL_SM_TRC_STATE(crep, id)
#endif

/***********************************************************************/
/*
 * R P C _ C N _ P O S T _ C A L L _ S M _ E V E N T
 *
 * Posts an event to either a server or client call state machine.
 *
 * Sample usage:
 *
 *
 *    rpc_cn_assoc_p_t          assoc;
 *    unsigned8                 event;
 *    rpc_cn_fragbuf_p_t        fragbuf;
 *    unsigned32                st;
 *
 * RPC_CN_POST_CALL_SM_EVENT (assoc, event, fragbuf, st);
 */

#define RPC_CN_POST_CALL_SM_EVENT(assoc, event_id, fragbuf, st) \
{ \
    rpc_cn_call_rep_p_t crep; \
\
    crep = RPC_CN_ASSOC_CALL (assoc); \
    if (crep != NULL) \
    { \
        if (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep)) \
            == \
            RPC_CN_PKT_CALL_ID (RPC_CN_FRAGBUF_PKT_HDR (fragbuf))) \
        { \
            RPC_CN_CALL_SM_TRC (crep, event_id, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));\
            st = rpc__cn_sm_eval_event (event_id, (pointer_t) fragbuf, \
                 (pointer_t) crep, &(crep->call_state)); \
            RPC_CN_CALL_SM_TRC_STATE (crep, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep)))); \
        } \
        else \
        { \
            (*fragbuf->fragbuf_dealloc)(fragbuf); \
        } \
    } \
    else \
    { \
        (*fragbuf->fragbuf_dealloc)(fragbuf); \
    } \
}

/***********************************************************************/
/*
 * R P C _ C N _ P O S T _ F I R S T _ C A L L _ S M _ E V E N T
 *
 * Posts the first [server] event to the call state machine.
 * This differs from the normal post call sm event because the
 * callid field has not been initialized upon the first server
 * event.
 *
 * Sample usage:
 *
 *    rpc_cn_call_rep_p_t       crep;
 *    rpc_cn_assoc_p_t          assoc;
 *    unsigned8                 event;
 *    rpc_cn_fragbuf_p_t        fragbuf;
 *    unsigned32                st;
 *
 * RPC_CN_POST_FIRST_CALL_SM_EVENT (crep, assoc, event, fragbuf, st);
 */

#define RPC_CN_POST_FIRST_CALL_SM_EVENT(crep, assoc, event_id, fragbuf, st) \
{ \
    rpc__cn_assoc_alloc ((assoc), (unsigned32*)&(st));\
    if (st == rpc_s_ok)\
    {\
        RPC_CN_CALL_SM_TRC (crep, event_id, (RPC_CN_PKT_CALL_ID (RPC_CN_FRAGBUF_PKT_HDR (fragbuf))));\
        st = rpc__cn_sm_eval_event (event_id, (pointer_t)fragbuf, \
             (pointer_t)(crep), &((crep)->call_state)); \
        RPC_CN_CALL_SM_TRC_STATE (crep, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));\
    }\
} 


/***********************************************************************/
/*
 * R P C _ C N _ C A L L _ E V A L _ E V E N T
 *
 * Posts an event from either a client caller or server call
 * executor thread to the call state machine.
 *
 * Sample usage:
 *
 *    rpc_cn_call_rep_p_t       crep;
 *    unsigned8                 event;
 *    pointer_t                 spc_struct;
 *    unsigned32                st;
 *
 * RPC_CN_CALL_EVAL_EVENT (event_id, spc_struct, crep, st);
 */

#define RPC_CN_CALL_EVAL_EVENT(event_id, spc_struct, crep, st)\
{ \
    RPC_CN_CALL_SM_TRC (crep, event_id, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));\
    st = rpc__cn_sm_eval_event ((event_id), (pointer_t)(spc_struct), \
                                (pointer_t)(crep), \
                                &(crep)->call_state); \
    RPC_CN_CALL_SM_TRC_STATE (crep, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep)))); \
} 


/***********************************************************************/
/*
 * R P C _ C N _ C A L L _ I N S E R T _ E V E N T
 *
 * This macro will be called when an event is generated inside an
 * action routine of the call state machine.
 */
#define RPC_CN_CALL_INSERT_EVENT(crep, event)\
{\
    RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_CALL_SM_TRACE, \
                    ("STATE INSERT EVENT ")); \
    RPC_CN_CALL_SM_TRC ((crep), (event)->event_id, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));\
    rpc__cn_sm_insert_event ((event),\
                             &((crep)->assoc_state));\
}

#endif /* _CNCLSM_H */
