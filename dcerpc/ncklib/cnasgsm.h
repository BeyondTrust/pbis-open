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
**      cnasgsm.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types/constants internal to the 
**  NCA Connection (cn) Association (as) Group (g) State Machine (sm).
**
**
*/


#ifndef _CNASGSM_H
#define _CNASGSM_H  1
/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ G R P _ S M _ T R C
 */
#ifdef DEBUG
#define RPC_CN_ASSOC_GRP_SM_TRC(assoc_grp, event_id)\
{\
    if ((assoc_grp)->grp_flags & RPC_C_CN_ASSOC_GRP_CLIENT)\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_GRP_SM_TRACE, \
                        ("STATE CLIENT GRP:    %x state->%s event->%s\n",\
                         assoc_grp,\
                         rpc_g_cn_grp_client_states[(assoc_grp)->grp_state.cur_state-RPC_C_CN_STATEBASE],\
                         rpc_g_cn_grp_client_events[event_id-RPC_C_CN_STATEBASE]));\
    }\
    else\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_GRP_SM_TRACE, \
                        ("STATE SERVER GRP:    %x state->%s event->%s\n",\
                         assoc_grp,\
                         rpc_g_cn_grp_server_states[(assoc_grp)->grp_state.cur_state-RPC_C_CN_STATEBASE],\
                         rpc_g_cn_grp_server_events[event_id-RPC_C_CN_STATEBASE]));\
    }\
}
#else
#define RPC_CN_ASSOC_GRP_SM_TRC(assoc_grp, event_id)
#endif


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ G R P _ S M _ T R C _ S T A T E
 */
#ifdef DEBUG
#define RPC_CN_ASSOC_GRP_SM_TRC_STATE(assoc_grp)\
{\
    if ((assoc_grp)->grp_flags & RPC_C_CN_ASSOC_GRP_CLIENT)\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_GRP_SM_TRACE, \
                        ("STATE CLIENT GRP:    %x new state->%s\n",\
                         assoc_grp->grp_remid.all,\
                         rpc_g_cn_grp_client_states[(assoc_grp)->grp_state.cur_state-RPC_C_CN_STATEBASE])); \
    }\
    else\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_GRP_SM_TRACE, \
                        ("STATE SERVER GRP:    %x new state->%s\n",\
                         assoc_grp->grp_id.all,\
                         rpc_g_cn_grp_server_states[(assoc_grp)->grp_state.cur_state-RPC_C_CN_STATEBASE])); \
    }\
}
#else
#define RPC_CN_ASSOC_GRP_SM_TRC_STATE(assoc_grp)
#endif


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ G R P _ E V A L _ E V E N T
 *
 * This macro will be called when association group events are detected.
 */
#define RPC_CN_ASSOC_GRP_EVAL_EVENT(assoc_grp, event_id, event_param, st)\
{\
    RPC_CN_ASSOC_GRP_SM_TRC (assoc_grp, event_id);\
    st = rpc__cn_sm_eval_event ((event_id),\
                                (pointer_t) (event_param),\
                                (pointer_t) (assoc_grp),\
                                &((assoc_grp)->grp_state));\
    if ((assoc_grp)->grp_state.cur_state == RPC_C_ASSOC_GRP_CLOSED)\
    {\
        rpc__cn_assoc_grp_dealloc (assoc_grp->grp_id);\
    }\
    RPC_CN_ASSOC_GRP_SM_TRC_STATE (assoc_grp); \
}


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ G R P _ I N S E R T _ E V E N T
 *
 * This macro will be called when an event is generated inside an
 * action routine of the association group state machine.
 */
#define RPC_CN_ASSOC_GRP_INSERT_EVENT(assoc_grp, event)\
{\
    RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_GRP_SM_TRACE, \
                    ("STATE INSERT EVENT ")); \
    RPC_CN_ASSOC_SM_TRC ((assoc_grp), (event)->event_id);\
    rpc__cn_sm_insert_event ((event),\
                             &((assoc_grp)->grp_state));\
}

/***********************************************************************/
/*
 * A S S O C   G R P   E V E N T S
 */

/*
 * Events common to both client and server state machines
 *
 * State values are incremented by 100 to distinguish them from
 * action routine indexes which are all < 100.  This was done as
 * an efficiency measure to the engine, rpc__cn_sm_eval_event().
 */ 
#define RPC_C_ASSOC_GRP_NEW                     100
#define RPC_C_ASSOC_GRP_ADD_ASSOC               101 
#define RPC_C_ASSOC_GRP_REM_ASSOC               102 

/*
 * Events only applicable to server state machine
 */
#define RPC_C_ASSOC_GRP_NO_CALLS_IND            103


/***********************************************************************/
/*
 * A S S O C   G R P   S T A T E S
 */

/*
 * States common to both client and server state machines
 */
#define RPC_C_ASSOC_GRP_CLOSED                  100 
#define RPC_C_ASSOC_GRP_OPEN                    101 
#define RPC_C_ASSOC_GRP_ACTIVE                  102 

/***********************************************************************/
/*
 * C L I E N T   A S S O C   G R P   S T A T E S
 */

#define RPC_C_CLIENT_ASSOC_GRP_STATES	        103 

/***********************************************************************/
/*
 * C L I E N T   A S S O C   G R P   T A B L E S
 */
EXTERNAL rpc_cn_sm_state_entry_p_t rpc_g_cn_client_grp_sm [];
EXTERNAL rpc_cn_sm_action_fn_t     rpc_g_cn_client_grp_action_tbl [];

#if DEBUG
EXTERNAL char   *rpc_g_cn_grp_client_events [];
EXTERNAL char   *rpc_g_cn_grp_client_states [];
#endif

/***********************************************************************/
/*
 * S E R V E R   A S S O C   G R P   S T A T E S
 */

#define RPC_C_SERVER_ASSOC_GRP_CALL_WAIT        103  
#define RPC_C_SERVER_ASSOC_GRP_STATES           104 

/***********************************************************************/
/*
 * S E R V E R    A S S O C   G R P   T A B L E S
 */
EXTERNAL rpc_cn_sm_state_entry_p_t rpc_g_cn_server_grp_sm [];
EXTERNAL rpc_cn_sm_action_fn_t     rpc_g_cn_server_grp_action_tbl [];

#if DEBUG
EXTERNAL char   *rpc_g_cn_grp_server_events [];
EXTERNAL char   *rpc_g_cn_grp_server_states [];
#endif
#endif /* _CNASGSM_H */
