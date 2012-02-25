/*
 * Copyright (c) 2010 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Portions of this software have been released under the following terms:
 *
 * (c) Copyright 1989-1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989-1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989-1993 DIGITAL EQUIPMENT CORPORATION
 *
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 * permission to use, copy, modify, and distribute this file for any
 * purpose is hereby granted without fee, provided that the above
 * copyright notices and this notice appears in all source code copies,
 * and that none of the names of Open Software Foundation, Inc., Hewlett-
 * Packard Company or Digital Equipment Corporation be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Neither Open Software
 * Foundation, Inc., Hewlett-Packard Company nor Digital
 * Equipment Corporation makes any representations about the suitability
 * of this software for any purpose.
 *
 * Copyright (c) 2007, Novell, Inc. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Novell Inc. nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
/*
**
**  NAME
**
**      cnsassm.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  NCA Connection (cn) Server (s) Association (as) State Machine (sm).
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
#include <cnassoc.h>    /* NCA Connection association services */
#include <comcthd.h>    /* Externals for Call Thread sub-component  */
#include <cncall.h>     /* NCA connection call service */
#include <cnsm.h>       /* NCA Connection state machine declarations */
#include <cnfbuf.h>     /* NCA Connection fragment buffer service */
#include <cnassm.h>     /* NCA Connection association state machine */



/******************************************************************************/
/*
 * Global Definitions
 */
GLOBAL char     *rpc_g_cn_assoc_server_events [] =
{
    "INDICATION",
    "ABORT_REQ",
    "REJ_RESP",
    "ALT_CONT_IND",
    "NO_CONN_IND",
    "ALT_CONT_RESP",
    "AUTH3_IND",
    "AUTH3_ACK",
    "AUTH3_NACK",
    "ALLOC_REQ",
    "DEALLOC_REQ",
    "SHUTDOWN_REQ",
    "LOCAL_ERROR",
    "ACC_RESP",
    "ASSOC_COMPLETE"
};

GLOBAL char     *rpc_g_cn_assoc_server_states [] =
{
    "CLOSED",
    "REQUESTED",
    "AUTH3_WAIT",
    "AUTH3",
    "OPEN",
    "ASSOC_WAIT"
};


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

INTERNAL void rpc__cn_assoc_process_auth_tlr (
    rpc_cn_assoc_p_t         /*assoc*/,
    rpc_cn_packet_p_t        /*req_header*/,
    unsigned32               /*req_header_size*/,
    rpc_cn_packet_p_t        /*resp_header*/,
    unsigned32              * /*header_size*/,
    unsigned32              * /*auth_len*/,
    rpc_cn_sec_context_p_t  * /*sec_context*/,
    boolean		      old_client,	
    unsigned32              * /*st*/);

INTERNAL void send_frag_resp_pdu (
    rpc_cn_assoc_p_t        /*assoc*/,
    rpc_cn_fragbuf_p_t      /*fragbuf*/,
    rpc_cn_packet_p_t       /*req_header*/);

INTERNAL void save_sec_fragment (
    rpc_cn_assoc_p_t        /*assoc*/,
    rpc_cn_packet_p_t	    /*header*/);


/***********************************************************************/
/*
 * S E R V E R   A S S O C   P R E D I C A T E   T A B L E
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
/* #define AUTHENT3_PRED                   0
 * #define ACTIVE_PRED                     1
 * #define LASTBIND_PRED		   2 
 */ 
/*  
 * The predicate routine prototypes.
 */
INTERNAL unsigned8 authent3_pred_rtn (
    pointer_t /*spc_struct*/, 
    pointer_t /*event_param*/) ATTRIBUTE_UNUSED;

INTERNAL unsigned8 active_pred_rtn (
    pointer_t /*spc_struct*/, 
    pointer_t /*event_param*/) ATTRIBUTE_UNUSED;

INTERNAL unsigned8 lastbindfrag_pred_rtn (
    pointer_t /*spc_struct*/, 
    pointer_t /*event_param*/);


/***********************************************************************/
/*
 * S E R V E R   A S S O C   A C T I O N   T A B L E
 *
 *
 * The actions. All actions except those noted below are described
 * in the NCA Connection architecture spec.
 */
#define ACCEPT_ASSOC            0
#define REJECT_ASSOC            1
#define ADD_ASSOC_TO_GRP        2
#define REM_ASSOC_FROM_GRP      3
#define DO_ALTER_CONT_REQ       4
#define SEND_ALTER_CONT_RESP    5
#define DO_AUTHENT3             6
#define DO_ASSOC_REQ            7
#define SEND_SHUTDOWN_REQ       8
#define INCR_ACTIVE             9
#define DECR_ACTIVE             10
#define ABORT_ASSOC             11
#define MARK_ASSOC              12
#define CANCEL_CALLS            13
#define ACCEPT_ADD              14
#define REM_MARK_ABORT          15
#define REM_MARK_CANCEL         16
#define INCR_DO_ALTER           17
#define SEND_DECR               18
#define MARK_ABORT              19
#define REM_MARK_ABORT_CAN      20
#define PROTOCOL_ERROR          21
#define DO_ASSOC_WAIT		22 
#define DO_ASSOC		23 

/*  
 * The action routine prototypes.
 */
INTERNAL unsigned32     accept_assoc_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     reject_assoc_action_rtn (
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

INTERNAL unsigned32     do_alter_cont_req_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     send_alter_cont_resp_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     do_authent3_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     do_assoc_req_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     send_shutdown_req_action_rtn (
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

INTERNAL unsigned32     abort_assoc_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     mark_assoc_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     cancel_calls_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     accept_add_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     rem_mark_abort_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     rem_mark_cancel_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     incr_do_alter_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     send_decr_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     mark_abort_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     rem_mark_abort_can_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     do_assoc_wait_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

INTERNAL unsigned32     do_assoc_action_rtn (
    pointer_t  /*spc_struct*/, 
    pointer_t  /*event_param*/,
    pointer_t  /*sm*/);

/*  
 * The action table itself.
 */
GLOBAL rpc_cn_sm_action_fn_t  rpc_g_cn_server_assoc_act_tbl [] =
{
    accept_assoc_action_rtn,
    reject_assoc_action_rtn,
    add_assoc_to_grp_action_rtn,
    rem_assoc_from_grp_action_rtn,
    do_alter_cont_req_action_rtn,
    send_alter_cont_resp_action_rtn,
    do_authent3_action_rtn,
    do_assoc_req_action_rtn,
    send_shutdown_req_action_rtn,
    incr_active_action_rtn,
    decr_active_action_rtn,
    abort_assoc_action_rtn,
    mark_assoc_action_rtn,
    cancel_calls_action_rtn,
    accept_add_action_rtn,
    rem_mark_abort_action_rtn,
    rem_mark_cancel_action_rtn,
    incr_do_alter_action_rtn,
    send_decr_action_rtn,
    mark_abort_action_rtn,
    rem_mark_abort_can_action_rtn,
    rpc__cn_assoc_sm_protocol_error,
    do_assoc_wait_action_rtn,
    do_assoc_action_rtn
};


/***********************************************************************/
/*
 * S E R V E R   A S S O C   S T A T E   T A B L E
 *
 *
 * C L O S E D _ S T A T E
 *
 * state 0 - closed. The association is unknown to the server.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t closed_state =
{
    /* event 0 - ind */
	{DO_ASSOC},
    
    /* event 1 - abort_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 2 - reject_resp */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 3 - alter_context_ind */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 4 - no_conn_ind */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 5 - alter_context_resp */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 6 - auth3_ind */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 7 - auth3_ack */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 8 - auth3_nack */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 9 - allocate_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 10 - deallocate_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},
    
    /* event 11 - shutdown_req */
	{RPC_C_CLIENT_ASSOC_CLOSED},

    /* event 12 - local_error */
	{RPC_C_CLIENT_ASSOC_CLOSED},

    /* event 13 - accept_resp */
	{RPC_C_CLIENT_ASSOC_CLOSED},

    /* event 14 - assoc_complete_resp */
    ILLEGAL_TRANSITION 
};


/*
 * R E Q U E S T E D _ S T A T E
 *
 * state 1 - requested_wait. The server has received a complete rpc_bind PDU
 * and is processing it.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t requested_state =
{
    /* event 0 - ind */
    ILLEGAL_TRANSITION,
    
    /* event 1 - abort_req */
	 {ABORT_ASSOC},
    
    /* event 2 - reject_resp */
	 {REJECT_ASSOC},
    
    /* event 3 - alter_context_ind */
    ILLEGAL_TRANSITION,
    
    /* event 4 - no_conn_ind */
	 {MARK_ASSOC},
    
    /* event 5 - alter_context_resp */
    ILLEGAL_TRANSITION,
    
    /* event 6 - auth3_ind */
    ILLEGAL_TRANSITION,
    
    /* event 7 - auth3_ack */
    ILLEGAL_TRANSITION,
    
    /* event 8 - auth3_nack */
    ILLEGAL_TRANSITION,
    
    /* event 9 - allocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 10 - deallocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 11 - shutdown_req */
	 {RPC_C_SERVER_ASSOC_REQUESTED},

    /* event 12 - local_error */
	 {MARK_ABORT},

    /* event 13 - accept_resp */
	 {ACCEPT_ADD},

    /* event 14 - assoc_complete_resp */
    ILLEGAL_TRANSITION 
};


/*
 * A U T H 3 _ W A I T _ S T A T E
 *
 * state 2 - auth3_wait. Wait for the 3rd leg of the optional 3-way
 * authentication handshake.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t auth3_wait_state =
{
    /* event 0 - ind */
    ILLEGAL_TRANSITION,
    
    /* event 1 - abort_req */
	 {ABORT_ASSOC},
    
    /* event 2 - reject_resp */
    ILLEGAL_TRANSITION,
    
    /* event 3 - alter_context_ind */
	 {DO_ALTER_CONT_REQ},
    
    /* event 4 - no_conn_ind */
	 {MARK_ASSOC},
//	 {REM_MARK_CANCEL},
    
    /* event 5 - alter_context_resp */
    ILLEGAL_TRANSITION,
    
    /* event 6 - auth3_ind */
	 {DO_AUTHENT3},
    
    /* event 7 - auth3_ack */
    ILLEGAL_TRANSITION,
    
    /* event 8 - auth3_nack */
    ILLEGAL_TRANSITION,
    
    /* event 9 - allocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 10 - deallocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 11 - shutdown_req */
	 {SEND_SHUTDOWN_REQ},

    /* event 12 - local_error */
	 {MARK_ABORT},

    /* event 13 - accept_resp */
    ILLEGAL_TRANSITION,

    /* event 14 - assoc_complete_resp */
    ILLEGAL_TRANSITION 
};


/*
 * A U T H 3 _ S T A T E
 *
 * state 3 - auth3_wait. Process the 3rd leg of the optional 3-way
 * authentication handshake.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t auth3_state =
{
    /* event 0 - ind */
    ILLEGAL_TRANSITION,
    
    /* event 1 - abort_req */
	 {ABORT_ASSOC},
    
    /* event 2 - reject_resp */
    ILLEGAL_TRANSITION,
    
    /* event 3 - alter_context_ind */
    ILLEGAL_TRANSITION,
    
    /* event 4 - no_conn_ind */
	 {MARK_ASSOC},
//	 {REM_MARK_CANCEL},
    
    /* event 5 - alter_context_resp */
         {SEND_ALTER_CONT_RESP},
    
    /* event 6 - auth3_ind */
    ILLEGAL_TRANSITION,
    
    /* event 7 - auth3_ack */
	 {ADD_ASSOC_TO_GRP},
    
    /* event 8 - auth3_nack */
	 {ABORT_ASSOC},
   
    /* event 9 - allocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 10 - deallocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 11 - shutdown_req */
	 {SEND_SHUTDOWN_REQ},

    /* event 12 - local_error */
	 {MARK_ABORT},

    /* event 13 - accept_resp */
    ILLEGAL_TRANSITION,

    /* event 14 - assoc_complete_resp */
    ILLEGAL_TRANSITION 
};


/*
 * O P E N _ S T A T E
 *
 * state 4 - open. A successul association has been established. One
 * or more presentation syntaxes have successfully been negotiated.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t open_state =
{
    /* event 0 - ind */
    ILLEGAL_TRANSITION,
    
    /* event 1 - abort_req */
	 {REM_MARK_ABORT},
    
    /* event 2 - reject_resp */
    ILLEGAL_TRANSITION,
    
    /* event 3 - alter_context_ind */
	 {INCR_DO_ALTER},
    
    /* event 4 - no_conn_ind */
	 {REM_MARK_CANCEL},
    
    /* event 5 - alter_context_resp */
	 {SEND_DECR},
    
    /* event 6 - auth3_ind */
    ILLEGAL_TRANSITION,
    
    /* event 7 - auth3_ack */
    ILLEGAL_TRANSITION,
    
    /* event 8 - auth3_nack */
    ILLEGAL_TRANSITION,
    
    /* event 9 - allocate_req */
	 {INCR_ACTIVE},
    
    /* event 10 - deallocate_req */
	 {DECR_ACTIVE},
    
    /* event 11 - shutdown_req */
	 {SEND_SHUTDOWN_REQ},

    /* event 12 - local_error */
	 {REM_MARK_ABORT_CAN},

    /* event 13 - accept_resp */
    ILLEGAL_TRANSITION,
    
    /* event 14 - assoc_complete_resp */
    ILLEGAL_TRANSITION 
};


/*
 * A S S O C _ W A I T
 *
 * state 5 - assoc_wait. The server has received at least one rpc_bind PDU
 * packet already and is processing another.
 */
INTERNAL rpc_cn_sm_state_tbl_entry_t assoc_wait_state =
{
    /* event 0 - ind */
	{DO_ASSOC_WAIT},
    
    /* event 1 - abort_req */
	{ABORT_ASSOC},

    /* event 2 - reject_resp */
	{REJECT_ASSOC},
    
    /* event 3 - alter_context_ind */
    ILLEGAL_TRANSITION,
    
    /* event 4 - no_conn_ind */
	 {MARK_ASSOC},

    /* event 5 - alter_context_resp */
    ILLEGAL_TRANSITION,
    
    /* event 6 - auth3_ind */
    ILLEGAL_TRANSITION,
    
    /* event 7 - auth3_ack */
    ILLEGAL_TRANSITION,
    
    /* event 8 - auth3_nack */
    ILLEGAL_TRANSITION,
    
    /* event 9 - allocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 10 - deallocate_req */
    ILLEGAL_TRANSITION,
    
    /* event 11 - shutdown_req */
	 {RPC_C_SERVER_ASSOC_REQUESTED},

    /* event 12 - local_error */
	 {MARK_ABORT},

    /* event 13 - accept_resp */
	 {ACCEPT_ADD},

    /* event 14 - assoc_complete_resp */
	 {DO_ASSOC_REQ} 
};



/*
 * The state table containing the action routines.
 */
GLOBAL rpc_cn_sm_state_entry_p_t rpc_g_cn_server_assoc_sm [] =
{
    closed_state,               /* state 0 - closed */
    requested_state,            /* state 1 - requested */
    auth3_wait_state,           /* state 2 - auth3_wait */
    auth3_state,                /* state 3 - auth3 */
    open_state,                 /* state 4 - open */
    assoc_wait_state            /* state 5 - assoc_wait */
};


/***********************************************************************/
/*
 *
 * S E R V E R   A S S O C   P R E D I C A T E   R O U T I N E S
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
  pointer_t       spc_struct ATTRIBUTE_UNUSED,
  pointer_t       event_param
)
{
    rpc_cn_packet_t     *header;
    rpc_cn_auth_tlr_t   *tlr;
    boolean32           three_way;
    unsigned32          st;
    unsigned32          authn_protocol;

    RPC_CN_DBG_RTN_PRINTF(SERVER authent3_pred_rtn);

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
        if (st == rpc_s_ok)
        {
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
        else
        {
            return (0);
        }
    }
}


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
    RPC_CN_DBG_RTN_PRINTF(SERVER authent3_pred_macro);\
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

    RPC_CN_DBG_RTN_PRINTF(SERVER active_pred_rtn);

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
**  ROUTINE NAME:       lastbindfrag_pred_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Returns 1 if the association request has is last_frag flag bit set
**  Returns 0 if if doesn't.
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
**  FUNCTION VALUE:     0 If the rpc_bind PDU last_frag flag it is NOT set
**                      1 If the rpc_bind PDU last_frag flag bit is set
**
**  SIDE EFFECTS:       none
**
**--
**/


INTERNAL unsigned8 lastbindfrag_pred_rtn (spc_struct, event_param)

pointer_t       spc_struct ATTRIBUTE_UNUSED;
pointer_t       event_param;

{
    rpc_cn_packet_t     *header;

    RPC_CN_DBG_RTN_PRINTF(SERVER lastbindfrag_pred_rtn);

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind PDU.
     */
    header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;

    /*
     * This is the last packet we are going to get if last_frag flag is set
     * or we are talking to an old client.
     */
    if ((RPC_CN_PKT_FLAGS (header) & RPC_C_CN_FLAGS_LAST_FRAG) ||
        (RPC_CN_PKT_VERS_MINOR (header) < RPC_C_CN_PROTO_VERS_MINOR))
    {
        return (1);
    }
    else
    {
        return(0);
    }
}


/***********************************************************************/
/*
 * S E R V E R   A S S O C   A C T I O N   R O U T I N E S
 */
/*
**++
**
**  ROUTINE NAME:       accept_assoc_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to send an rpc_bind_ack association accept PDU
**  to the client, including the secondary address, association
**  group id and, optionally, authentication information.
**
**  If we have authentication information, we may send more than one
**  PDU to the client containing the pieces of the auth info.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragment buffer containing the rpc_bind_ack PDU. 
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
**  FUNCTION VALUE:     completion status returned in
**			assoc->assoc_status.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     accept_assoc_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_packet_t             *req_header;
    rpc_cn_fragbuf_t            *fragbuf;

    RPC_CN_DBG_RTN_PRINTF(SERVER accept_assoc_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    /*
     * The event parameter is the fragbuf containing
     * the rpc_bind_ack PDU and the security context element.
     */
    fragbuf    = (rpc_cn_fragbuf_t *) event_param;
    req_header = (rpc_cn_packet_t *) fragbuf->data_p;

    /*
     * If we have security, we may have to break it apart.
     */
    if (RPC_CN_PKT_AUTH_TLR_PRESENT (req_header))
    {
        send_frag_resp_pdu(assoc, fragbuf, req_header);
    }
    else
    {
        /*
         * Just send the PDU and free the fragbuf.
         */
        rpc__cn_assoc_send_fragbuf (assoc,
                                    fragbuf,
                                    assoc->security.assoc_current_sec_context,
                                    true,
                                    &(assoc->assoc_status));
    }
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       reject_assoc_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to send an rpc_bind_nack association reject PDU
**  to the client. The client will abort the connection after
**  receiving the association reject.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragment buffer containing the rpc_bind_nack PDU.
**                      This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     reject_assoc_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_sm_ctlblk_t 		*sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER reject_assoc_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind_nack PDU.
     */
    fragbuf = (rpc_cn_fragbuf_t *) event_param;

    /*
     * Now actually send the PDU and free the fragbuf.
     */
    rpc__cn_assoc_send_fragbuf (assoc, 
                                fragbuf, 
                                NULL, 
                                true,
                                &(assoc->assoc_status));
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));
    
    sm_p->cur_state = RPC_C_SERVER_ASSOC_CLOSED; 
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
**  Action routine add the current association to the association
**  group. 
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
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     add_assoc_to_grp_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t      *assoc;
    rpc_cn_sm_ctlblk_t  *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER add_assoc_to_grp_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * Add the association to the group.
     */
    rpc__cn_assoc_grp_add_assoc (assoc->assoc_grp_id,
                                 assoc);

    sm_p->cur_state = RPC_C_SERVER_ASSOC_OPEN; 
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
**  Action routine to remove the current association from the
**  association group.
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
    rpc_cn_assoc_t *assoc;

    RPC_CN_DBG_RTN_PRINTF(SERVER rem_assoc_from_grp_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    /*
     * Remove the association from the group.
     */
    rpc__cn_assoc_grp_rem_assoc (assoc->assoc_grp_id,
                                 assoc);

    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       do_alter_cont_req_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to process the alter context negotiation request.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the alter_context PDU.
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

INTERNAL unsigned32     do_alter_cont_req_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_sm_ctlblk_t          *sm_p;
    rpc_cn_packet_t             *req_header;
    rpc_cn_packet_t             *resp_header;
    rpc_cn_pres_cont_list_t     *pres_cont_list;
    rpc_cn_pres_result_list_t   *pres_result_list;
    unsigned32                  result_list_len;
    unsigned32                  header_size;
    unsigned32                  auth_len;
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_sm_event_entry_t     event;
    rpc_cn_port_any_t           *sec_addr;
    boolean 			old_client;
    rpc_cn_sec_context_p_t      sec_context = NULL;
    unsigned8                   resp_flags = 0;
    
    RPC_CN_DBG_RTN_PRINTF(SERVER do_alter_cont_req_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;
    old_client = (assoc->assoc_vers_minor <= RPC_C_CN_PROTO_VERS_COMPAT);

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_alter_context PDU.
     */
    req_header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;

    if (!(RPC_CN_PKT_FLAGS(req_header) & RPC_C_CN_FLAGS_LAST_FRAG) &&
        !(RPC_CN_PKT_VERS_MINOR (req_header) < RPC_C_CN_PROTO_VERS_MINOR))
    {
        /*
         * Alter context is not complete (and we are not talking to
         * an old client who doesn't set the last_frag flag).
         * Save the security fragment, and wait for the next one.
         */
        save_sec_fragment(assoc, req_header);
		  /* FIXME: is this value correct ? */
        return rpc_s_ok;
    }
    /*
     * Make sure if we have processed previous alter_context PDU's
     * that we append the piece from the last packet in the sequence.
     */
    if (assoc->security.auth_buffer_info.auth_buffer != NULL)
    {
        save_sec_fragment(assoc, req_header);
    }

    /*
     * Mark the current security context as NULL. This will be
     * filled in when we have successfully established a security
     * context. It will be used by the action routine which actually
     * sends the rpc_bind_ack PDU.
     */
    assoc->security.assoc_current_sec_context = NULL;

    /*
     * Allocate a large fragbuf for the response PDU. We won't expend any
     * more time here trying to optimize to allocate a small fragbuf.
     */
    RPC_CN_FRAGBUF_ALLOC (fragbuf, rpc_g_cn_large_frag_size, &(assoc->assoc_status));
    resp_header = (rpc_cn_packet_t *) (fragbuf->data_p);
    header_size = RPC_CN_PKT_SIZEOF_ALT_CTX_R_HDR;

    sec_addr = (rpc_cn_port_any_t *)
        ((unsigned8 *)(resp_header) + header_size);

    /* changed from 1 to 0 - lukeh May 2003 */
    sec_addr->length = 0;
    sec_addr->s[0] = '\0';

    header_size = (2 + header_size + 3) & ~0x03;

    pres_cont_list = (rpc_cn_pres_cont_list_t *) 
        ((unsigned8 *) req_header + RPC_CN_PKT_SIZEOF_ALT_CTX_HDR);

    pres_result_list = (rpc_cn_pres_result_list_t *)
        ((unsigned8 *)(resp_header) + header_size);

    result_list_len = rpc_g_cn_large_frag_size - header_size;

    rpc__cn_assoc_syntax_negotiate (assoc,
                                    pres_cont_list,
                                    &result_list_len,
                                    pres_result_list,
                                    &assoc->assoc_status);

    header_size += result_list_len;
    if (assoc->assoc_status == rpc_s_ok)
    {
        auth_len = rpc_g_cn_large_frag_size - header_size;
        rpc__cn_assoc_process_auth_tlr (assoc,
                                        req_header, 
                                        ((rpc_cn_fragbuf_t *)event_param)->data_size,
                                        resp_header,
                                        &header_size, 
                                        &auth_len,
                                        &assoc->security.assoc_current_sec_context,
					old_client,
                                        &assoc->assoc_status);

    }
    
    /*
     * Prepare header flags especially taking rpc_c_protect_flags_header_sign
     * into account if it has been requested and accepted
     */
    sec_context = assoc->security.assoc_current_sec_context;

    resp_flags = RPC_C_CN_FLAGS_FIRST_FRAG | RPC_C_CN_FLAGS_LAST_FRAG;
    if (sec_context &&
	(sec_context->sec_info->authn_flags & rpc_c_protect_flags_header_sign))
    {
        resp_flags |= RPC_C_CN_FLAGS_SUPPORT_HEADER_SIGN;
    }

    /*
     * An rpc_alter_context_response PDU will be sent if the association status is
     * OK.
     */
    if (assoc->assoc_status == rpc_s_ok)
    {
        fragbuf->data_size = header_size;
        rpc__cn_pkt_format_common (resp_header, 
                                   RPC_C_CN_PKT_ALTER_CONTEXT_RESP,
                                   resp_flags,
                                   fragbuf->data_size,
                                   auth_len,
                                   RPC_CN_PKT_CALL_ID (req_header),
                                   assoc->assoc_vers_minor);

        /*
         * Send an alter context response event through the association state
         * machine. Note the event parameter is the fragment buffer
         * containing the response PDU.
         */
        sm_p->cur_state = RPC_C_SERVER_ASSOC_AUTH3;
        event.event_id = RPC_C_ASSOC_ALTER_CONTEXT_RESP;
        event.event_param = (pointer_t) fragbuf;
        RPC_CN_ASSOC_INSERT_EVENT (assoc, &event);
    }
    else
    {
        /*
         * The architecture doesn't specify what to do here. We'll
         * at least spit out what happened.
         */
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                        ("CN: call_rep->%x assoc->%x desc->%x error %x while processing alter context PDU \n",
                         assoc->call_rep,
                         assoc,
                         assoc->cn_ctlblk.cn_sock,
                         assoc->assoc_status));
    }

    return (assoc->assoc_status);
}

/*
**++
**
**  ROUTINE NAME:       send_alter_cont_resp_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to send an alter_context_resp PDU
**  to the client.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the alter_context_resp PDU.
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

INTERNAL unsigned32     send_alter_cont_resp_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_packet_t             *req_header;
    rpc_cn_sm_ctlblk_t 		*sm_p;
    unsigned8			n_state, o_state;
    rpc_cn_sec_context_p_t	sec;

    RPC_CN_DBG_RTN_PRINTF(SERVER send_alter_cont_resp_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    o_state = sm_p->cur_state;

    sec = assoc->security.assoc_current_sec_context;
    if (sec == NULL || sec->sec_state == RPC_C_SEC_STATE_COMPLETE)
    {
	n_state = RPC_C_SERVER_ASSOC_OPEN;
    }
    else
    {
	n_state = RPC_C_SERVER_ASSOC_AUTH3_WAIT;
    }

    if (o_state != RPC_C_SERVER_ASSOC_OPEN &&
        n_state == RPC_C_SERVER_ASSOC_OPEN)
    {
	add_assoc_to_grp_action_rtn (spc_struct, (pointer_t) assoc, sm);
    }

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the alter_context_resp PDU.
     */
    fragbuf = (rpc_cn_fragbuf_t *) event_param;
    req_header = (rpc_cn_packet_t *) fragbuf->data_p;

    /*
     * If we have security, we may have to break it apart.
     */
    if (RPC_CN_PKT_AUTH_TLR_PRESENT (req_header))
    {
        send_frag_resp_pdu(assoc, fragbuf, req_header);
    }
    else
    {
        /*
         * Just send the PDU and free the fragbuf.
         */
        rpc__cn_assoc_send_fragbuf (assoc,
                                    fragbuf,
                                    assoc->security.assoc_current_sec_context,
                                    true,
                                    &(assoc->assoc_status));
    }
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));

    sm_p->cur_state = n_state;
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       do_authent3_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to process the 3rd leg of an optional 3-way
**  authentication handshake.
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
**
**  INPUTS/OUTPUTS:     
**
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     do_authent3_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;
    rpc_cn_packet_t *req_header;
    rpc_cn_packet_t *tmp_resp_header;
    unsigned32 auth_len;
    rpc_cn_fragbuf_t *fragbuf;
    unsigned32 header_size;
    rpc_cn_port_any_t *sec_addr;
    boolean old_client;
    unsigned8 tmp[rpc_g_cn_large_frag_size];
    unsigned8 new_state;
    rpc_cn_sec_context_p_t sec;

    RPC_CN_DBG_RTN_PRINTF(SERVER do_authent3_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    old_client = (assoc->assoc_vers_minor <= RPC_C_CN_PROTO_VERS_COMPAT);

    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    sm_p->cur_state = RPC_C_SERVER_ASSOC_AUTH3; 

    fragbuf = (rpc_cn_fragbuf_t *)event_param;
    req_header = (rpc_cn_packet_t *)fragbuf->data_p; 

    if (!(RPC_CN_PKT_FLAGS (req_header)) &&
        !(RPC_CN_PKT_VERS_MINOR (req_header) < RPC_C_CN_PROTO_VERS_MINOR))
    {
        save_sec_fragment (assoc, req_header);
                   /* FIXME: is this value correct? */
        return rpc_s_ok;
    }
    if (assoc->security.auth_buffer_info.auth_buffer != NULL)
    {
        save_sec_fragment (assoc, req_header);
    }
    assoc->security.assoc_current_sec_context = NULL;

    /*
     * Allocate a temporary "response" buffer so we can
     * call rpc__cn_assoc_process_auth_tlr()
     */
    tmp_resp_header = (rpc_cn_packet_t *) (tmp);
    header_size = RPC_CN_PKT_SIZEOF_BIND_ACK_HDR; /* not really, but. */

    sec_addr = (rpc_cn_port_any_t *)((unsigned8 *)(tmp_resp_header) + header_size);
    /* changed from 1 to 0 - lukeh May 2003 */
    sec_addr->length = 0;
    sec_addr->s[0] = '\0';

    header_size = (2 + header_size + 3) & ~0x03;

    /* Process the trailer */

    rpc__cn_assoc_process_auth_tlr (assoc,
                                    req_header,
                                    fragbuf->data_size,
                                    tmp_resp_header,
                                    &header_size,
                                    &auth_len,
                                    &assoc->security.assoc_current_sec_context,
                                    old_client,
                                    &assoc->assoc_status);
    if (assoc->assoc_status == rpc_s_ok && auth_len != 0)
    {
        /* Make sure the authentication subsystem didn't return a response */
        assoc->assoc_status = rpc_s_proto_unsupp_by_auth;
    }

    sec = assoc->security.assoc_current_sec_context;
    if (sec == NULL || sec->sec_state == RPC_C_SEC_STATE_COMPLETE)
    {
        new_state = (assoc->assoc_status == rpc_s_ok) ? RPC_C_ASSOC_AUTH3_ACK : RPC_C_ASSOC_AUTH3_NACK;

        assoc->assoc_status = rpc__cn_sm_eval_event (new_state, event_param, assoc, &assoc->assoc_state);
        assoc->assoc_flags &= ~RPC_C_CN_ASSOC_SCANNED;
    }
    else
    {
        sm_p->cur_state = RPC_C_SERVER_ASSOC_AUTH3_WAIT;
    }

    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       do_assoc_req_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to process the association request and
**  presentation and security negotiations.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the rpc_bind PDU. 
**                      This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**   	sm 		The control block from the event evaluation
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
**                      rpc_s_assoc_grp_max_exceeded
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     do_assoc_req_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t              *assoc;
    rpc_cn_assoc_grp_t          *assoc_grp;
    rpc_cn_packet_t             *req_header;
    rpc_cn_packet_t             *resp_header;
    rpc_cn_pres_cont_list_t     *pres_cont_list;
    rpc_cn_pres_result_list_t   *pres_result_list;
    unsigned32                  header_size;
    unsigned32                  result_list_len;
    unsigned32                  auth_len;
    rpc_cn_local_id_t           grp_id;
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_sm_event_entry_t     event;
    rpc_cn_port_any_t           *sec_addr;
    boolean			old_client; 
    rpc_cn_sm_ctlblk_t 		*sm_p; 
    rpc_cn_sec_context_p_t      sec_context = NULL;
    unsigned8                   resp_flags = 0;

    RPC_CN_DBG_RTN_PRINTF (SERVER do_assoc_req_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    
    /*
     * Since this is a new association we don't yet have an
     * association UUID CRC (and will not unless a client on this
     * association uses security).
     */
    assoc->security.assoc_have_uuid_crc = false;

    /*
     * Mark the current security context as NULL. This will be
     * filled in when we have successfully established a security
     * context. It will be used by the action routine which actually
     * sends the rpc_bind_ack PDU.
     */
    assoc->security.assoc_current_sec_context = NULL;

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind PDU.
     */
    req_header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;

    /*
     * Allocate a large fragbuf for the response PDU. It will either
     * be an rpc_bind_ack or rpc_bind_nack. We won't expend any
     * more time here trying to optimize to allocate a small fragbuf.
     */
    RPC_CN_FRAGBUF_ALLOC (fragbuf, rpc_g_cn_large_frag_size, &(assoc->assoc_status));
    resp_header = (rpc_cn_packet_t *) (fragbuf->data_p);

    /*
     * Assume that everything is going to go OK and begin formatting
     * an rpc_bind_ack PDU. If something bad happens we'll format an
     * rpc_bind_nack PDU below.
     */
    header_size = RPC_CN_PKT_SIZEOF_BIND_ACK_HDR;

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_PROT_VERS_MISMATCH))
    {
        RPC_CN_PKT_VERS_MINOR (req_header) =
            RPC_C_CN_PROTO_VERS_MINOR + 1;
    }
#endif

    if ((RPC_CN_PKT_VERS (req_header) != RPC_C_CN_PROTO_VERS) 
        ||
        (RPC_CN_PKT_VERS_MINOR (req_header) > RPC_C_CN_PROTO_VERS_MINOR))
    {
        assoc->assoc_status = rpc_s_rpc_prot_version_mismatch;
    }
    else
    {
        /*
         * As per the NCA Connection Architecture return the
         * client's minor protocol version number and restrict the use
         * of the protocol to the client's minor version.
         */
        assoc->assoc_vers_minor = RPC_CN_PKT_VERS_MINOR (req_header);
	old_client = (assoc->assoc_vers_minor <= RPC_C_CN_PROTO_VERS_COMPAT);
        RPC_CN_PKT_VERS_MINOR (resp_header) = RPC_CN_PKT_VERS_MINOR(req_header);

        /*
         * Return the server's operational max size fragment it will
         * send and max size fragment it is capable of receiving. Make
         * sure these values are not greater than what the client gave.
         */
        if (RPC_CN_PKT_MAX_RECV_FRAG (req_header) == 0)
        {
            /*
             * This is a reserved value as specified in the NCA
             * Connection Architecture spec. It implies the default
             * rpc_c_assoc_must_recv_frag_size. 
             */
            RPC_CN_PKT_MAX_RECV_FRAG (req_header) = RPC_C_ASSOC_MUST_RECV_FRAG_SIZE;
        }
        if (RPC_CN_PKT_MAX_XMIT_FRAG (req_header) == 0)
        {
            /*
             * This is a reserved value as specified in the NCA
             * Connection Architecture spec. It implies the default
             * rpc_c_assoc_must_recv_frag_size. 
             */
            RPC_CN_PKT_MAX_XMIT_FRAG (req_header) = RPC_C_ASSOC_MUST_RECV_FRAG_SIZE;
        }
        RPC_CN_PKT_MAX_XMIT_FRAG (resp_header) =             
        MIN (rpc_g_cn_large_frag_size, RPC_CN_PKT_MAX_RECV_FRAG (req_header));
        RPC_CN_PKT_MAX_RECV_FRAG (resp_header) = 
        MIN (rpc_g_cn_large_frag_size, RPC_CN_PKT_MAX_XMIT_FRAG (req_header));
        assoc->assoc_max_xmit_frag = RPC_CN_PKT_MAX_XMIT_FRAG (resp_header);
        assoc->assoc_max_recv_frag = RPC_CN_PKT_MAX_RECV_FRAG (resp_header);
        
        /*
         * Determine whether the rpc_bind PDU contains an association
         * group id.
         */
        if (RPC_CN_PKT_ASSOC_GROUP_ID (req_header) != 0)
        {
            /*
             * The rpc_bind PDU does contain a group id. Use it to look
             * up an association group.
             */
            grp_id.all = RPC_CN_PKT_ASSOC_GROUP_ID (req_header);
            assoc->assoc_grp_id = 
            rpc__cn_assoc_grp_lkup_by_id (grp_id,
                                          RPC_C_CN_ASSOC_GRP_SERVER,
                                          assoc->transport_info,
                                          &(assoc->assoc_status)); 
            assoc_grp = RPC_CN_ASSOC_GRP (assoc->assoc_grp_id);
            if (assoc->assoc_status == rpc_s_ok)
            {
#ifdef DEBUG
                if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                                  RPC_C_CN_DBG_GRP_MAX_EXCEEDED))
                {
                    assoc_grp->grp_cur_assoc = assoc_grp->grp_max_assoc;
                }
#endif
                /*
                 * The association group was found. Determine whether it can
                 * support another association.
                 */
                if (assoc_grp->grp_cur_assoc == assoc_grp->grp_max_assoc)
                {
                    /*
                     * The group can't support another association. Reject this
                     * association request by sending an rpc_bind_nack PDU.
                     */
                    assoc->assoc_status = rpc_s_assoc_grp_max_exceeded;
                }
            }
        } /* end if (RPC_CN_PKT_ASSOC_GROUP_ID (req_header) != 0) */
        else /* (RPC_CN_PKT_ASSOC_GROUP_ID (req_header) == 0) */
        {
            /*
             * A new association group needs to be created.
             */
            assoc->assoc_grp_id = rpc__cn_assoc_grp_alloc (assoc->cn_ctlblk.rpc_addr,
                                                           assoc->transport_info,
                                                           RPC_C_CN_ASSOC_GRP_SERVER,
                                                           0,
                                                           &(assoc->assoc_status));
            assoc_grp = RPC_CN_ASSOC_GRP (assoc->assoc_grp_id);
        } /* end else (RPC_CN_PKT_ASSOC_GROUP_ID (req_header) == 0) */
        
        if (assoc->assoc_status == rpc_s_ok)
        {
            /*
             * Return the appropriate group ID for the client to use on
             * the next association request.
             */
            RPC_CN_PKT_ASSOC_GROUP_ID (resp_header) = assoc_grp->grp_id.all;

            /*
             * Determine the length of the secondary address endpoint including the '\0'
             * termination and store this in the PDU. Also string copy the
             * actual endpoint into the PDU. If the endpoint won't fit fall
             * through and send an rpc_bind_nak.
             */
            sec_addr = (rpc_cn_port_any_t *) 
                ((unsigned8 *) (resp_header) + header_size);
            sec_addr->length = 
                strlen ((char *)assoc->cn_ctlblk.cn_listening_endpoint) + 1;
            if (sec_addr->length <
                (rpc_g_cn_large_frag_size - header_size + 2))
            {
                strcpy ((char *)(sec_addr->s),
                        (char *)assoc->cn_ctlblk.cn_listening_endpoint);
                
                /*
                 * To format the balance of the rpc_bind_ack PDU fields we're
                 * going to have to do some pointer arithmetic because the
                 * secondary address endpoint is variable length. Also note 
		 * that the because the presentation result list must start 
		 * on a 4-byte boundary  rounding up will be necessary.
                 */

                header_size = (header_size + 2 + sec_addr->length + 3) & ~0x3;

                pres_result_list = (rpc_cn_pres_result_list_t *)
                    ((unsigned8 *) resp_header + header_size);
               
                /*
                 * Get a local pointer to the presentation context list in the
                 * PDU header. Use this along with the location of the
                 * presentation result list in the response hdr to pass to the
                 * syntax negotiation routine. Upon successful return from this
                 * routine the presentation result list will be formatted.
                 * If unsuccessful the presentation context result list would
                 * not fit in the balance of the fragbuf. Fall through and
                 * send an rpc_bind_nak.
                 */
                pres_cont_list = (rpc_cn_pres_cont_list_t *) 
                    ((unsigned8 *) req_header + RPC_CN_PKT_SIZEOF_BIND_HDR);
                
                result_list_len = rpc_g_cn_large_frag_size - header_size;

                rpc__cn_assoc_syntax_negotiate (assoc,
                                                pres_cont_list,
                                                &result_list_len,
                                                pres_result_list,
                                                &assoc->assoc_status);

		header_size += result_list_len;
                if (assoc->assoc_status == rpc_s_ok)
                {
                    auth_len = rpc_g_cn_large_frag_size - header_size;
                    rpc__cn_assoc_process_auth_tlr (assoc,
                                                    req_header, 
                                                    ((rpc_cn_fragbuf_t *)event_param)->data_size,
                                                    resp_header,
                                                    &header_size, 
                                                    &auth_len,
                                                    &assoc->security.assoc_current_sec_context,
						    old_client,
                                                    &assoc->assoc_status);
		}
            }
            else
            {
                assoc->assoc_status = RPC_S_HEADER_FULL;
            }
        }
    }

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_HEADER_FULL))
    {
        assoc->assoc_status = RPC_S_HEADER_FULL;
    }
#endif

    /*
     * Prepare header flags especially taking rpc_c_protect_flags_header_sign
     * into account if it has been requested and accepted
     */
    sec_context = assoc->security.assoc_current_sec_context;

    resp_flags = RPC_C_CN_FLAGS_FIRST_FRAG | RPC_C_CN_FLAGS_LAST_FRAG;
    if (sec_context &&
	(sec_context->sec_info->authn_flags & rpc_c_protect_flags_header_sign))
    {
        resp_flags |= RPC_C_CN_FLAGS_SUPPORT_HEADER_SIGN;
    }

    /*
     * An rpc_bind_nak PDU will be sent if the association status is
     * not OK. Assume it will be the only fragment sent.
     */
    if (assoc->assoc_status == rpc_s_ok)
    {
        fragbuf->data_size = header_size;
        rpc__cn_pkt_format_common (resp_header, 
                                   RPC_C_CN_PKT_BIND_ACK,
                                   resp_flags,
                                   fragbuf->data_size,
                                   auth_len,
                                   RPC_CN_PKT_CALL_ID (req_header),
                                   assoc->assoc_vers_minor);
        
	/*
         * Send an accept response event through the association state
         * machine.
         */
        event.event_id = RPC_C_ASSOC_ACCEPT_RESP;
    }
    else
    {
	int i = 0;

        /*
         * Some failure happened. Reject this
         * association request by sending an rpc_bind_nak PDU
         * with the list of supported protocol versions.
         */
        fragbuf->data_size = RPC_CN_PKT_SIZEOF_BIND_NACK_HDR +
            (RPC_C_CN_PROTO_VERS_MINOR * sizeof(rpc_cn_version_t));
        rpc__cn_pkt_format_common (resp_header, 
                                   RPC_C_CN_PKT_BIND_NAK,
                                   resp_flags,
                                   fragbuf->data_size,
                                   0,
                                   RPC_CN_PKT_CALL_ID (req_header),
                                   assoc->assoc_vers_minor);
        (RPC_CN_PKT_VERSIONS (resp_header)).n_protocols
            = RPC_C_CN_PROTO_VERS_MINOR + 1;
        for (i=0;i <= RPC_C_CN_PROTO_VERS_MINOR; i++)
        {
            (RPC_CN_PKT_VERSIONS (resp_header)).protocols[i].vers_major
                = RPC_C_CN_PROTO_VERS; 
            (RPC_CN_PKT_VERSIONS (resp_header)).protocols[i].vers_minor = i;
        }
        RPC_CN_PKT_PROV_REJ_REASON(resp_header) =
            rpc__cn_assoc_status_to_prej (assoc->assoc_status); 

        /*
         * Set up the event parameter as the fragment buffer containg
         * the PDU and send the event through the association state
         * machine.
         */
        event.event_id = RPC_C_ASSOC_REJECT_RESP;
    }

    event.event_param = (pointer_t) fragbuf;
    RPC_CN_ASSOC_INSERT_EVENT (assoc, &event); 
    sm_p->cur_state = RPC_C_SERVER_ASSOC_REQUESTED; 
    return (assoc->assoc_status);
}



/*
**++
**
**  ROUTINE NAME:       send_shutdown_req_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to send an rpc_shutdown orderly shutdown PDU
**  to the client.
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
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     send_shutdown_req_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t              *assoc;
    unsigned32                  pdu_size;
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_packet_t             *header;
    rpc_cn_sm_ctlblk_t 		*sm_p;
    unsigned8			n_state;

    RPC_CN_DBG_RTN_PRINTF(SERVER send_shutdown_req_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /* 
     * There is no predicate associated  with this routine but the
     * new value of sm->cur_state is determined by the value of
     * sm->cur_state coming into the routine.  Note that
     * 2+rpc_c_cn_statebase is auth3_wait_state; 3+rpc_c_cn_statebase is
     * auth3_wait; 4+rpc_c_cn_statebase is open_state.  These values
     * are static and rpc_c_cn_statebase is the value added to
     * the states in order to distinguish them from the action
     * routine indexes. 
     */
    switch (sm_p->cur_state) {
    case (2 + RPC_C_CN_STATEBASE):
	n_state = RPC_C_SERVER_ASSOC_AUTH3_WAIT;
        break;
    case (3 + RPC_C_CN_STATEBASE):
	n_state = RPC_C_SERVER_ASSOC_AUTH3;
        break;
    case (4 + RPC_C_CN_STATEBASE):
	n_state = RPC_C_SERVER_ASSOC_OPEN;
        break;
    default:
        fprintf(stderr, "%s no value for n_state\n", __PRETTY_FUNCTION__);
        abort();
        break;
    }

    /*
     * Allocate a fragbuf capable of holding a shutdown PDU.
     */
    pdu_size = RPC_CN_PKT_SIZEOF_SHUTDOWN_HDR;

    /*
     * Allocate a fragbuf of the appropriate size.
     */
    RPC_CN_FRAGBUF_ALLOC (fragbuf, pdu_size, &(assoc->assoc_status));
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));

    /*
     * Now begin construction of the PDU.
     */
    header = (rpc_cn_packet_t *) fragbuf->data_p;
    rpc__cn_pkt_format_common (header, 
                               RPC_C_CN_PKT_SHUTDOWN,
                               0,
                               pdu_size,
                               0,
                               0,
                               assoc->assoc_vers_minor);

    /*
     * Now actually send the PDU and free the fragbuf.
     */
    rpc__cn_assoc_send_fragbuf (assoc, 
                                fragbuf, 
                                NULL, 
                                true,
                                &(assoc->assoc_status));
    RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));
    sm_p->cur_state = n_state;  
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
**  Action routine to set the active predicate to true. The server
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
**   	sm 		The control block from the event evaluation
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

    RPC_CN_DBG_RTN_PRINTF(SERVER incr_active_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 

    /*
     * Inccrement the association active reference counter.
     */
    RPC_CN_INCR_ACTIVE_SVR_ACTION(assoc, sm_p);
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
**  Action routine to set the active predicate to false. The server
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
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    
**
**   	sm 		The control block from the event evaluation
**                      routine.  Input is the current state and
**                      event for the control block.  Output is the
**                      next state or updated current state, for the
**                      control block.
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
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER decr_active_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 

    /*
     * Decrement the association active reference counter.
     */
    RPC_CN_DECR_ACTIVE_SVR_ACTION(assoc, sm_p);
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
**  Action routine to abort the current association.
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
**   	sm 		The control block from the event evaluation
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
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER abort_assoc_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * Close the connection on the association.
     */
    if (assoc->cn_ctlblk.cn_state == RPC_C_CN_OPEN)
    {
        rpc__cn_network_close_connect (assoc,
                                       &(assoc->assoc_status));
        RPC_CN_ASSOC_CHECK_ST (assoc, &(assoc->assoc_status));
    }
    sm_p->cur_state =  RPC_C_SERVER_ASSOC_CLOSED; 
    return (assoc->assoc_status);
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
**  Action routine to mark the current association with a status code.
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
**   	sm 		The control block from the event evaluation
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
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL unsigned32     mark_assoc_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t      *assoc;
    dcethread*           current_thread_id;
    rpc_cn_sm_ctlblk_t  *sm_p; 
    
    RPC_CN_DBG_RTN_PRINTF(SERVER mark_assoc_action_rtn);

    /*
     * The special structure is a pointer to the association.
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
        
        case RPC_C_ASSOC_LOCAL_ERROR:
        {
            /*
             * The error status is contained in the association local
             * error status for this event.
             */
            assoc->assoc_status = assoc->assoc_local_status;
            break;
        }

        default:
        {
            if (assoc->assoc_status == rpc_s_ok)
                assoc->assoc_status = rpc_s_connection_aborted;
            break;
        }
    }
    
    /*
     * Queue a dummy fragbuf on the association receive queue. This
     * will wake up any threads blocked waiting for receive data.
     */
    current_thread_id = dcethread_self();
    if (dcethread_equal (current_thread_id,
                       assoc->cn_ctlblk.cn_rcvr_thread_id))
    {
        RPC_CN_ASSOC_WAKEUP (assoc);
    }

    sm_p->cur_state = RPC_C_SERVER_ASSOC_CLOSED; 
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       cancel_calls_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to cancel all the calls currently using the
**  association. This allows the call state machinery to properly try to
**  terminate calls when the connection has terminated.
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

INTERNAL unsigned32     cancel_calls_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t      *assoc;
    error_status_t      st;
    rpc_binding_rep_t   *binding_r;

    RPC_CN_DBG_RTN_PRINTF(SERVER cancel_calls_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;

    /*
     * The connection is gone.
     */
    if (assoc->call_rep != NULL)
    {
        /*
         * If the call is queued dequeue it otherwise cancel the
         * thread it is executing in and signal the association message
         * queue condition variable if a thread is waiting on it.
         */
        if (rpc__cthread_dequeue((rpc_call_rep_t *) assoc->call_rep))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_orphan, RPC_C_CN_DBG_ORPHAN,
                           ("(cancel_calls_action_rtn) call_rep->%x queued call ... dequeued call id = %x\n",
                            assoc->call_rep,
                            RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR(assoc->call_rep))));
            binding_r = (rpc_binding_rep_t *) assoc->call_rep->binding_rep;
            RPC_CN_UNLOCK ();
            rpc__cn_call_end ((rpc_call_rep_p_t *) &assoc->call_rep, &st);
            RPC_CN_LOCK ();
            RPC_BINDING_RELEASE (&binding_r, &st);
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
                           ("(cancel_calls_action_rtn) call_rep->%x running call ... cancelling\n",
                            assoc->call_rep));
            RPC_CN_ASSOC_CANCEL_AND_WAKEUP (assoc);
        }
    }
    else
    {
        RPC_DBG_PRINTF(rpc_e_dbg_orphan, RPC_C_CN_DBG_ORPHAN,
                       ("(cancel_calls_action_rtn) call_rep->%x assoc->%x no call ... do nothing\n",
                        assoc->call_rep,
                        assoc));
    }
          
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       accept_add_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to accept the current association and add it to
**  an association group.
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
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     accept_add_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t      *assoc;
    rpc_cn_sm_ctlblk_t  *sm_p;
    unsigned8		n_state; 
    rpc_cn_sec_context_p_t sec;

    RPC_CN_DBG_RTN_PRINTF(SERVER accept_add_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    sec = assoc->security.assoc_current_sec_context;
    if (sec == NULL || sec->sec_state == RPC_C_SEC_STATE_COMPLETE)
    {
	n_state = RPC_C_SERVER_ASSOC_OPEN;
    }
    else
    {
	n_state = RPC_C_SERVER_ASSOC_AUTH3_WAIT;
    }

    /*
     * Add the association to the group and send the rpc_bind_ack PDU 
     * to the client. 
     *
     * Note: the order in which this is done is important. The assoc
     * must be added to the group before sending back the rpc_bind_ack PDU. 
     * The process of sending a PDU results in the CN mutex being released. 
     * This opens a window where its possible the association group to which 
     * we're going to add the assoc could get deallocated which would result
     * in a state transition error when we tried to add to it.
     *
     * Note that since we are calling action routines from 
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * we call now, would update sm_p->cur_state inappropriately 
     * for accept_add_action_rtn().
     */
    if (n_state != RPC_C_SERVER_ASSOC_AUTH3_WAIT)
    {
        add_assoc_to_grp_action_rtn (spc_struct, (pointer_t) assoc, sm);
    }
    accept_assoc_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state = n_state;
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
**  Action routine to remove the current association from an
**  association group, mark the association appropriately and abort
**  the association.
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
**   	sm 		The control block from the event evaluation
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

    RPC_CN_DBG_RTN_PRINTF(SERVER rem_mark_abort_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
  
    /*
     * Remove the association from the group, mark it with the
     * appropriate status and abort the underlying session/transport
     * connection. 
     *
     * Note that since we are calling action routines from 
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * we call now, would update sm_p->cur_state inappropriately 
     * for rem_mark_abort_action_rtn().
     */  
    rem_assoc_from_grp_action_rtn (spc_struct, event_param, sm);
    abort_assoc_action_rtn (spc_struct, event_param, sm);
    mark_assoc_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state = RPC_C_SERVER_ASSOC_CLOSED; 
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       rem_mark_cancel_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to remove the current association from an
**  association group, mark the association appropriately and
**  cancel all calls currently using the association.
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
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     rem_mark_cancel_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER rem_mark_cancel_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    
    /*
     * Remove the association from the group and mark it with the
     * appropriate status and cancels all calls currently using it.
     *
     * Note that since we are calling action routines from 
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * we call now, would update sm_p->cur_state inappropriately 
     * for rem_mark_cancel_action_rtn().
     */  
    rem_assoc_from_grp_action_rtn (spc_struct, event_param, sm);
    mark_assoc_action_rtn (spc_struct, event_param, sm);
    cancel_calls_action_rtn (spc_struct, event_param, sm);
    
    sm_p->cur_state = RPC_C_SERVER_ASSOC_CLOSED; 
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       incr_do_alter_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to increment the association reference count and process
**  an rpc_alter_context_req PDU
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the alter_context_req PDU.
**                      This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     incr_do_alter_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER incr_do_alter_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
    
    /*
     * Increment the active reference counter in the association and
     * process the alter context request.
     *
     * Note that since we are calling action routines from 
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * we call now, would update sm_p->cur_state inappropriately 
     * for incr_do_alter_action_rtn().
     */  
    incr_active_action_rtn (spc_struct, event_param, sm);
    do_alter_cont_req_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state = RPC_C_SERVER_ASSOC_OPEN; 
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       send_decr_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to send and an rpc_alter_context_response PDU and
**  decrement the association reference count.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the
**                      alter_context_resp PDU.
**                      This is passed in as the
**                      special event related parameter which was
**                      passed to the state machine evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     send_decr_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;
 
    RPC_CN_DBG_RTN_PRINTF(SERVER send_decr_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
  
    /*
     * Send the alter_context_resp PDU to the client and decrement the
     * active reference counter in the association.
     *
     * Note that since we are calling action routines from 
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * we call now would update sm_p->cur_state inappropriately
     * for send_decr_action_rtn().
     */  
    send_alter_cont_resp_action_rtn (spc_struct, event_param, sm);
    decr_active_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state = RPC_C_SERVER_ASSOC_OPEN; 
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
**  Action routine to mark the association with an error and abort
**  the session/transport connection.
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
**   	sm 		The control block from the event evaluation
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

    RPC_CN_DBG_RTN_PRINTF(SERVER mark_abort_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm; 
  
    /*
     * Mark the association with the appropriate status and abort
     * the underlying session/transport connection. 
     *
     * Note that since we are calling action routines from 
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * we call now, would update sm_p->cur_state inappropriately 
     * for mark_abort_action_rtn().
     */  
    abort_assoc_action_rtn (spc_struct, event_param, sm);
    mark_assoc_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state = RPC_C_SERVER_ASSOC_CLOSED; 
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       rem_mark_abort_can_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to remove the current association from an
**  association group, mark the association appropriately, abort
**  the underlying session/transport connection and cancels all calls
**  currently using the association.
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
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     rem_mark_abort_can_action_rtn 
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t *assoc;
    rpc_cn_sm_ctlblk_t *sm_p;

    RPC_CN_DBG_RTN_PRINTF(SERVER rem_mark_abort_can_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * Remove the association from the group, mark it with the
     * appropriate status, abort the underlying session/transport
     * connection and cancel the call executor thread.
     *
     * Note that since we are calling action routines from 
     * within action routines, we need to update state as
     * a final step here.  Otherwise, the action routines
     * we call now, would update sm_p->cur_state inappropriately
     * for rem_mark_abort_action_rtn().
     */  
    rem_assoc_from_grp_action_rtn (spc_struct, event_param, sm);
    abort_assoc_action_rtn (spc_struct, event_param, sm);
    mark_assoc_action_rtn (spc_struct, event_param, sm);
    cancel_calls_action_rtn (spc_struct, event_param, sm);

    sm_p->cur_state = RPC_C_SERVER_ASSOC_CLOSED;
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       do_assoc_wait_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine to process multiple rpc_bind PDU packets
**  and reconstruct the security information.
**
**  If we call this routine, this packet is expected to be one of
**  a series of rpc_bind packets contaiining security information.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the rpc_bind PDU. 
**                      This is passed in as the special event related 
**                      parameter which was passed to the state machine 
**                      evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     do_assoc_wait_action_rtn
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t                      *assoc;
    rpc_cn_packet_t                     *req_header;
    rpc_cn_sm_event_entry_t             event;
    rpc_cn_sm_ctlblk_t 			*sm_p;


    RPC_CN_DBG_RTN_PRINTF(SERVER do_assoc_wait_action_rtn);

    /*
     * The special structure is a pointer to the association.
     */
    assoc = (rpc_cn_assoc_t *) spc_struct;
    sm_p = (rpc_cn_sm_ctlblk_t *)sm;

    /*
     * The event parameter is a pointer to the fragbuf containing
     * the rpc_bind PDU.
     */
    req_header = (rpc_cn_packet_t *) ((rpc_cn_fragbuf_t *)event_param)->data_p;

    /*
     * Save the security frament in the association reconstruction buffer
     */
    save_sec_fragment(assoc, req_header);

    /*
     * Now we check to see if this is the last packet for this rpc_bind PDU
     * If it is, we send a assoc_complete event through the state
     * machine, and run this PDU through the 'old' association request code.
     */
    if (RPC_CN_PKT_FLAGS (req_header) & RPC_C_CN_FLAGS_LAST_FRAG)
    {
        event.event_id = RPC_C_ASSOC_ASSOC_COMPLETE_RESP;
        event.event_param = event_param;
        RPC_CN_ASSOC_INSERT_EVENT (assoc, &event);
    }

    sm_p->cur_state = RPC_C_SERVER_ASSOC_WAIT;
    return (rpc_s_ok);
}




/*
**++
**
**  ROUTINE NAME:       do_assoc_action_rtn
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Action routine vectors to either do_assoc_wait_action_rtn
**  or to do_assoc_req_action_rtn, depending on predicate value.
**
**  INPUTS:
**
**      spc_struct      The association.  Note that this is passed in as
**                      the special structure which is passed to the
**                      state machine event evaluation routine.
**
**      event_param     The fragbuf containing the rpc_bind PDU. 
**                      This is passed in as the special event related 
**                      parameter which was passed to the state machine 
**                      evaluation routine.
**
**  INPUTS/OUTPUTS:     
**
**   	sm 		The control block from the event evaluation
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

INTERNAL unsigned32     do_assoc_action_rtn
(
  pointer_t       spc_struct,
  pointer_t       event_param,
  pointer_t       sm 
)
{
    rpc_cn_assoc_t                      *assoc;
    rpc_cn_packet_t                     *req_header ATTRIBUTE_UNUSED;
    rpc_cn_sm_event_entry_t             event ATTRIBUTE_UNUSED;
    rpc_cn_sm_ctlblk_t 			*sm_p ATTRIBUTE_UNUSED;
    unsigned32				status;

    RPC_CN_DBG_RTN_PRINTF(SERVER do_assoc_action_rtn);
    assoc = (rpc_cn_assoc_t *) spc_struct;

    status = lastbindfrag_pred_rtn (spc_struct, event_param);
    if (status == 0)
    {
	do_assoc_wait_action_rtn (spc_struct, event_param, sm);
        return (rpc_s_ok);
    }

    do_assoc_req_action_rtn (spc_struct, event_param, sm);
    return (assoc->assoc_status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_process_auth_tlr
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Support routine to decode an auth trailer on an bind or alter
**  context PDU and format the auth trailer on a bind_ack or
**  alter_context_response PDU
**
**  If the client had to break part the security information
**  we have passed through the assoc_wait state and we must
**  use the re-constuction buffer, not the info in the PDU.
**  
**  In addition, since we delayed checksumming the packets
**  which contained the pieces of the sec info, we have to 
**  do this too.
**
**  INPUTS:
**
**      assoc           The association.
**      req_header      bind or alter_context PDU.
**      req_header_size The size of the bind or alter_context PDU.
**      resp_header     partially formatted bind_ack or alter_context_response
**                      PDU.
**
**  INPUTS/OUTPUTS:     
**
**      header_size     On input, the size used in the
**                      resp_header already. On output,
**                      the size of the resp_header including the
**                      trailer. 
**
**  OUTPUTS:            none
**
**      auth_len        The size of the auth_value field in the auth trailer.
**      sec_context     The security context element used.
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

INTERNAL void rpc__cn_assoc_process_auth_tlr 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_cn_packet_p_t       req_header,
  unsigned32              req_header_size,
  rpc_cn_packet_p_t       resp_header,
  unsigned32              *header_size,
  unsigned32              *auth_len,
  rpc_cn_sec_context_p_t  *sec_context,
  boolean		  old_client,
  unsigned32              *st
)
{
    rpc_cn_auth_tlr_t               *req_auth_tlr;
    rpc_cn_auth_tlr_t               *resp_auth_tlr;
    rpc_auth_info_t                 *info;
    rpc_cn_sec_context_t            *sec;
    rpc_cn_bind_auth_value_priv_p_t local_auth_value, priv_auth_value;
    unsigned32                      local_auth_value_len;
    rpc_authn_protocol_id_t         authn_protocol;
    rpc_authn_flags_t               req_authn_flags = 0;

    RPC_CN_DBG_RTN_PRINTF(SERVER rpc__cn_assoc_process_auth_tlr);
    CODING_ERROR (st);

    /*
     * If security was requested in the request PDU a
     * security response is required in the response PDU.
     * All security related errors will be reported within the
     * auth_value field of the auth trailer of the
     * response PDU.
     */
    if (!RPC_CN_PKT_AUTH_TLR_PRESENT (req_header))
    {
        *auth_len = 0;
        *st = rpc_s_ok;
        goto DONE;
    }
    RPC_DBG_PRINTF (rpc_e_dbg_auth, 20,
                 ("(rpc__cn_assoc_process_auth_tlr) auth trailer present\n"));

    /*
     * First make sure the authentication protocol requested is
     * supported here. If not, send a bind_nak PDU back to the
     * client.
     */
    req_auth_tlr = RPC_CN_PKT_AUTH_TLR (req_header, 
                                        req_header_size);
    authn_protocol = RPC_CN_AUTH_CVT_ID_WIRE_TO_API ( req_auth_tlr->auth_type, st);
    if (*st != rpc_s_ok)
    {
        goto DONE;
    }
    if ( ! RPC_CN_AUTH_INQ_SUPPORTED ( authn_protocol))
    {
        *st = rpc_s_unknown_auth_protocol;
        goto DONE;
    }

    /*
     * Check if header signing extension has been requested
     */
    if (RPC_CN_PKT_FLAGS(req_header) & RPC_C_CN_FLAGS_SUPPORT_HEADER_SIGN)
    {
        req_authn_flags |= rpc_c_protect_flags_header_sign;
    }

    /*
     * Determine whether a security context with the given
     * key ID already exists. If it does this is an attempt at
     * renewing said context.
     */
    sec = *sec_context = NULL;
    rpc__cn_assoc_sec_lkup_by_id (assoc,
                                  req_auth_tlr->key_id,
                                  &sec,
                                  st);
    *st = rpc_s_ok;
    if (sec == NULL)
    {
        RPC_CN_AUTH_CREATE_INFO (
                   authn_protocol,
                   req_auth_tlr->auth_level,
                   req_authn_flags,
                   &info,
                   st);
    }
    if (*st != rpc_s_ok)
    {
        dce_error_string_t error_text;
        int temp_status;

        dce_error_inq_text(*st, (unsigned char*) error_text, &temp_status);

	/*
         * "%s failed: %s"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
	    rpc_svc_auth,
	    svc_c_sev_error,
	    rpc_m_call_failed,
	    "RPC_CN_AUTH_CREATE_INFO",
	    error_text ));
        goto DONE;
    }
    if (sec == NULL)
    {
        sec = rpc__cn_assoc_sec_alloc (info,
                                       st);
    }
    else
    {
        info = NULL;
    }
    if (*st != rpc_s_ok)
    {
        goto DONE;
    }

    sec->sec_key_id = req_auth_tlr->key_id;
    /*
     * Get the assoc_uuid_crc and put it in the association.
     * Get it from the last packet recieved.
     */
    RPC_CN_AUTH_TLR_UUID_CRC (
                   authn_protocol,
                   (pointer_t)req_auth_tlr->auth_value,
                   RPC_CN_PKT_AUTH_LEN (req_header),
                   &assoc->security.assoc_uuid_crc);

    /*
     * Check to see if we have pieced together the
     * auth_value and should use that buffer
     */
    if (assoc->security.auth_buffer_info.auth_buffer != NULL)
    {
        local_auth_value = (rpc_cn_bind_auth_value_priv_t *)
                            assoc->security.auth_buffer_info.auth_buffer;
        local_auth_value_len = assoc->security.auth_buffer_info.auth_buffer_len;
        /*
         * Better safe than sorry: make sure we get sub_type from the
         * packet we are going to checksum.
         */
        local_auth_value->sub_type = ((rpc_cn_bind_auth_value_priv_t *)
                                       req_auth_tlr->auth_value)->sub_type;
    }
    else
    {
        local_auth_value = (rpc_cn_bind_auth_value_priv_t *)
                            req_auth_tlr->auth_value;
        local_auth_value_len = RPC_CN_PKT_AUTH_LEN(req_header);
    }

    RPC_CN_AUTH_VFY_CLIENT_REQ (&assoc->security, 
                                sec,
                                (pointer_t)local_auth_value,
                                local_auth_value_len,
				old_client,
                                &sec->sec_status);

    if (sec->sec_status == rpc_s_ok)
    {
        /*
         * We only checksum one packet, since we ignored everything
         * in the others.  We want to pass the cred_length of the packet
         * we are doing the checksum on.
         */
        priv_auth_value = (rpc_cn_bind_auth_value_priv_t *)
                               req_auth_tlr->auth_value;
        if (assoc->raw_packet_p != NULL)
        {
            /*
             * Use the raw packet if it exists.
             */
            RPC_CN_AUTH_RECV_CHECK (authn_protocol,
                       &assoc->security,
                       sec,
                       (rpc_cn_common_hdr_t *)assoc->raw_packet_p->data_p,
                       assoc->raw_packet_p->data_size,
                       priv_auth_value->cred_length,
                       req_auth_tlr,
                       0, /* dummy unpack_ints */
                       &sec->sec_status);
        }
        else
        {
            /*
             * Raw packet doesn't exist; use unpacked one.
             */
            RPC_CN_AUTH_RECV_CHECK (authn_protocol,
                                &assoc->security,
                                sec,
                                (rpc_cn_common_hdr_t *)req_header,
                                req_header_size,
                                priv_auth_value->cred_length,
                                req_auth_tlr,
                                0, /* dummy unpack_ints */
                                &sec->sec_status);
        }
    } /* sec->sec_status == rpc_s_ok */

    /*
     * Free assembly buffer
     */
    if (assoc->security.auth_buffer_info.auth_buffer)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
                 ("(rpc__cn_assoc_process_auth_tlr) Free'd auth_buffer: %x\n",
                 assoc->security.auth_buffer_info.auth_buffer));

        RPC_MEM_FREE(assoc->security.auth_buffer_info.auth_buffer, 
                     RPC_C_MEM_CN_PAC_BUF);
        assoc->security.auth_buffer_info.auth_buffer = NULL;
        assoc->security.auth_buffer_info.auth_buffer_len = 0;
        assoc->security.auth_buffer_info.auth_buffer_max = 0;
    }

    /*
     * Start filling in the response packet
     */
    *header_size = ((*header_size + 3) & ~0x3);
    resp_auth_tlr = (rpc_cn_auth_tlr_t *)
                    ((unsigned8 *)(resp_header) + *header_size);
    (void) memset(resp_auth_tlr, 0, (rpc_g_cn_large_frag_size - *header_size));
    resp_auth_tlr->auth_type = req_auth_tlr->auth_type;
    resp_auth_tlr->auth_level = req_auth_tlr->auth_level;
    resp_auth_tlr->stub_pad_length = 0;
    resp_auth_tlr->reserved = 0;
    resp_auth_tlr->key_id = req_auth_tlr->key_id;
    *header_size += RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;
    *auth_len = rpc_g_cn_large_frag_size - *header_size;

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_FRAG_BIND_ACK))
    {
        char *x;
        *auth_len = atoi(((x = getenv("BIND_ACK_FRAG")) == NULL ? "50" : x));
    }
#endif

    /*
     * We need to handle large security info if its not going to 
     * fit inside one fragbuf.  So this routine will store a 
     * long KRB message in assoc->security->krb_message.
     * When the PDU is sent, the rest of the security will be sent.
     */
    RPC_CN_AUTH_FMT_SRVR_RESP (sec->sec_status,
                               &assoc->security, 
                               sec,
                               (pointer_t)req_auth_tlr->auth_value, 
                               RPC_CN_PKT_AUTH_LEN (req_header), 
                               (pointer_t)resp_auth_tlr->auth_value, 
                               auth_len);

    /* auth_len now has length of auth_value */
    *header_size += *auth_len;

    if (sec->sec_status != rpc_s_ok)
    {
        dce_error_string_t error_text;
        int temp_status;

        dce_error_inq_text(sec->sec_status, (unsigned char*) error_text, &temp_status);

        /*
	 * "%s on server failed: %s"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
	    rpc_svc_auth,
	    svc_c_sev_error,
	    rpc_m_call_failed_s,
	    "RPC_CN_AUTH_VFY_CLIENT_REQ",
	    error_text ));

        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_SECURITY_ERRORS,
                        ("CN: call_rep->%x assoc->%x desc->%x client verification failed security_context->%x auth_type->%x auth_level->%x auth_len->%x stub_pad_length->%x st->%x\n",
                         assoc->call_rep,
                         assoc,
                         assoc->cn_ctlblk.cn_sock,
                         sec,
                         req_auth_tlr->auth_type,
                         req_auth_tlr->auth_level,
                         RPC_CN_PKT_AUTH_LEN (req_header),
                         req_auth_tlr->stub_pad_length,
                         sec->sec_status));
        sec->sec_state = RPC_C_SEC_STATE_INCOMPLETE;
    } else {
        sec->sec_state = RPC_C_SEC_STATE_COMPLETE;
    }
    RPC_LIST_ADD_TAIL (assoc->security.context_list, 
                       sec, 
                       rpc_cn_sec_context_p_t);
    assoc->security.assoc_have_uuid_crc = true;
    *sec_context = sec;

    if (info != NULL)
    {
        RPC_CN_AUTH_RELEASE_REFERENCE(&info);
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
}



/*
**++
**
**  ROUTINE NAME:       send_frag_resp_pdu
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Support routine to send a bind_ack or alter_context_response PDU
**  with fragmented security.
**
**  If the security information did not all fit in to the first
**  response fragbuf, we will send it and as many more as it
**  takes to completely send the credential information.
**  
**  INPUTS:
**
**      assoc           The association.
**      fragbuf         The response fragbuf.
**      header      The header of the rpc_bind_ack or alter_context_resp
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    
**
**      assoc->security.krb_message    Contains the credentials
**                                     we might have to send in pieces.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/


INTERNAL void send_frag_resp_pdu(assoc, 
                                 fragbuf,
                                 header)

rpc_cn_assoc_p_t        assoc;
rpc_cn_fragbuf_p_t	fragbuf;
rpc_cn_packet_p_t       header;


{
    sec_krb_message                         *krb_message_ptr;
    rpc_cn_auth_tlr_t                       *auth_tlr;
    rpc_cn_bind_auth_value_priv_t         *auth_value;
    unsigned32                              cred_remain;
    unsigned32                              flags;
    unsigned32                              cred_len;
    unsigned32                              msg_length;
    unsigned32                              offset      = 0;
    boolean                                 first_frag  = true;
    boolean                                 free_buf    = false;
    boolean                                 free_message=false;
    rpc_cn_sec_context_p_t                  sec_context = NULL;

    /*
     * We stashed the KRB_AP_REP message here when we built it.
     */
    krb_message_ptr = &(assoc->security.krb_message);
    cred_remain = krb_message_ptr->length;

    auth_tlr = RPC_CN_PKT_AUTH_TLR (header, RPC_CN_PKT_FRAG_LEN (header));
    auth_value = (rpc_cn_bind_auth_value_priv_t *)auth_tlr->auth_value;

    /*
     * We want to send the first PDU as is, since its all set
     * to go, but we may need to send the same fragbuf several 
     * times to get all the security info across.
     * 
     * Slightly different than the client side as we are filling
     * in only the credentials field, not the whole auth_value_t.
     */
    do
    {
        flags = 0;
	sec_context = assoc->security.assoc_current_sec_context;

        if (first_frag)
        {
            flags = RPC_C_CN_FLAGS_FIRST_FRAG;
            first_frag = false;
        }

        cred_len = auth_value->cred_length;
        if (cred_remain > 0)
        {
            cred_remain -= cred_len;
        }

        if (cred_remain == 0)
        {
            flags |= RPC_C_CN_FLAGS_LAST_FRAG;
            free_buf = true;
        }

	if (sec_context &&
	    (sec_context->sec_info->authn_flags &
	     rpc_c_protect_flags_header_sign))
	{
		flags |= RPC_C_CN_FLAGS_SUPPORT_HEADER_SIGN;
	}

        RPC_CN_PKT_FLAGS (header) = flags;

        /*
         * Now actually send the PDU.  Free it if free_buf set.
         */
        rpc__cn_assoc_send_fragbuf (assoc, 
                                    fragbuf,
                                    sec_context,
                                    free_buf,
                                    &(assoc->assoc_status));

        RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
("(send_frag_resp_pdu) SENT %s PDU: data_size=%u, first_frag=%s, last_frag=%s, cred_len=%u, cred_remain=%u\n",
              ((RPC_CN_PKT_PTYPE(header) == RPC_C_CN_PKT_BIND_ACK) ? 
                  "rpc_bind_ack" : "alter_context_resp"),
              fragbuf->data_size,
              (flags & RPC_C_CN_FLAGS_FIRST_FRAG) ? "true": "false",
              (flags & RPC_C_CN_FLAGS_LAST_FRAG) ? "true": "false",
              cred_len,
              cred_remain));

        if (cred_remain > 0)
        {
            /* We didn't free the krb_message in fmt_srvr_resp */
            free_message = true;

            /*
             * Zero out old cred info and put in next segment
             */
            (void) memset(auth_value->credentials, 0, cred_len);

            /*
             * If the rest of the creditials will all fit, put it in,
             * otherwise use the same number of bytes as before.
             */
            if (cred_len > cred_remain)
            {
                unsigned32	shorten = (cred_len - cred_remain);

                msg_length = cred_remain;
                auth_value->cred_length = cred_remain;
                fragbuf->data_size -= shorten;
                RPC_CN_PKT_AUTH_LEN (header) -= shorten;
                RPC_CN_PKT_FRAG_LEN (header) = fragbuf->data_size;
            }
            else
            {
                msg_length = cred_len;
            }

            offset += cred_len;
            assert((offset + msg_length) <= (unsigned32)krb_message_ptr->length);
            memcpy(auth_value->credentials, 
                   krb_message_ptr->data + offset, 
                   msg_length);
        }
    } while (cred_remain > 0);

    /*
     * Free the krb_message if we need to.
     */
    if (free_message)
    {
#ifdef AUTH_KRB
        sec_krb_message_free (krb_message_ptr);
#endif

        RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
             ("(send_frag_resp_pdu) Freeing KRB message: 0x%x\n",
              krb_message_ptr));
    }
}



/*
**++
**
**  ROUTINE NAME:       save_sec_fragment
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Support routine which copies the bytes in the credential
**  field of an auth_tlr into a reconstruction buffer.
**
**  Used to process rpc_bind and alter_context PDU's which
**  have fragmented credentials
**  
**  INPUTS:
**
**      assoc           The association.
**      header          The header of the rpc_bind or alter_context PDU.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    
**
**      assoc->security.auth_buffer_info    Contains the pieces of credentials
**                                          we have previously saved.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/


INTERNAL void save_sec_fragment(assoc, 
                                header)

rpc_cn_assoc_p_t        assoc;
rpc_cn_packet_p_t	header;

{
    rpc_cn_bind_auth_value_priv_t       *auth_value;
    unsigned32                          auth_value_len;
    rpc_cn_auth_tlr_t                   *auth_tlr;
    unsigned8                           *auth_buffer;
    unsigned32                          auth_buffer_max;
    unsigned32                          auth_buffer_len;

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
  ("(save_sec_fragment) Alloc'd auth_buffer: %x, auth_buffer_max = %d\n",
                        auth_buffer,
                        auth_buffer_max));
    }

    if ((RPC_CN_PKT_AUTH_LEN (header) + auth_buffer_len) > auth_buffer_max)
    {
        auth_buffer_max += RPC_C_CN_LARGE_FRAG_SIZE;

        RPC_MEM_REALLOC(auth_buffer, unsigned8 *,
                        auth_buffer_max,
                        RPC_C_MEM_CN_PAC_BUF,
                        RPC_C_MEM_WAITOK);

        RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_BIG_PAC,
("(save_sec_fragment) Realloc'd auth_buffer: %x, auth_buffer_max = %d\n",
                        auth_buffer,
                        auth_buffer_max));
    }

    /*
     * Concatenate this security info on to the buffer.
     * 
     * We have to watch out for the checksum at the end of the
     * auth trailer, we only want to recover the KRB_AP_{REQ|REP} message.
     */
    auth_tlr = RPC_CN_PKT_AUTH_TLR(header,RPC_CN_PKT_FRAG_LEN (header));
    auth_value = (rpc_cn_bind_auth_value_priv_t *)auth_tlr->auth_value;
    auth_value_len = RPC_CN_PKT_AUTH_LEN (header) - 
                         auth_value->checksum_length;

    /*
     * For the first packet, copy the header info, for the rest
     * we just need the credential fragment.  We also update
     * the cred_length field in the assembly buffer.
     */

    if (auth_buffer_len == 0)
    {
        if (auth_value_len < auth_buffer_max)
        {
        memcpy(auth_buffer, auth_value, auth_value_len);
        }
        else
        {
            auth_value_len = 0;
        }
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
    ("(save_sec_fragment) Copied to auth_buffer: %x, auth_buffer_len=%d, auth_value_len=%d, auth_buffer_max=%d\n", 
    auth_buffer, auth_buffer_len, auth_value_len, auth_buffer_max));

    auth_buffer_len += auth_value_len;

    
    /*
     * Update our per-association data
     */
    if (assoc->security.auth_buffer_info.auth_buffer)
    {
        RPC_MEM_FREE(assoc->security.auth_buffer_info.auth_buffer,
                     RPC_C_MEM_CN_PAC_BUF);
        assoc->security.auth_buffer_info.auth_buffer = NULL;
    }
    assoc->security.auth_buffer_info.auth_buffer = auth_buffer;
    assoc->security.auth_buffer_info.auth_buffer_len = auth_buffer_len;
    assoc->security.auth_buffer_info.auth_buffer_max = auth_buffer_max;

    return;
}
/* vim:sw=4 ts=4
 * */
