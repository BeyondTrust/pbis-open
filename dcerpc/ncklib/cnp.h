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
#ifndef _CNP_H
#define _CNP_H	1
/*
**
**  NAME
**
**      cnp.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types/constants internal to NCA Connection RPC
**  Protocol Service and common to all its components.
**
**
*/

#include <cn.h>

/*
 * CN internal status codes. These will not be passed out of CN.
 *	RPC_C_CN_STATEBASE:  determines base value for the state 
 *			     and this value is used to quickly determine 
 *			     by the state evaluation routine, whether a 
 *			     value is a state or action.
 */
#define RPC_S_HEADER_FULL               0x0001beef
#define RPC_C_CN_STATEBASE 		100	
/*
 * Macros for serializing access to the connection protocol code.
 */
#define RPC_CN_LOCK()                   RPC_LOCK(0)
#define RPC_CN_UNLOCK()                 RPC_UNLOCK(0)
#define RPC_CN_LOCK_ASSERT()            RPC_LOCK_ASSERT(0)

/*
 * rpc_e_dbg_general debug switch levels
 */
#define RPC_C_CN_DBG_ROUTINE_TRACE      20
#define RPC_C_CN_DBG_THREADS            2       /* exact */
#define RPC_C_CN_DBG_ERRORS             1
#define RPC_C_CN_DBG_BUFFS              1
#define RPC_C_CN_DBG_GENERAL            1
#define RPC_C_CN_DBG_SECURITY_ERRORS    1

/*
 * rpc_e_dbg_cn_state debug switch levels
 */
#define RPC_C_CN_DBG_ASSOC_SM_TRACE     3
#define RPC_C_CN_DBG_ASSOC_GRP_SM_TRACE 2
#define RPC_C_CN_DBG_CALL_SM_TRACE      1

/*
 * rpc_e_dbg_orphan debug switch levels
 */
#define RPC_C_CN_DBG_ORPHAN             1

/*
 * rpc_e_dbg_cancel debug switch levels
 */
#define RPC_C_CN_DBG_CANCEL             1

/*
 * rpc_e_dbg_cn_pkt debug switch levels
 */
#define RPC_C_CN_DBG_PKT_DUMP           20
#define RPC_C_CN_DBG_PKT                1

/*
 * rpc_e_dbg_cn_errors debug switch levels
 *
 * Switches to set on server to generate bind NAKs and a status code used only
 * when one of the error debug levels are set.
 */
#define RPC_S_CN_DBG_FAILURE            0xdeadbeefU
#define RPC_C_CN_DBG_PROT_VERS_MISMATCH 1 /* server: force a mismatch */
#define RPC_C_CN_DBG_GRP_LKUP_BY_ID     2 /* server: make lkup fail  */
                                          /* (>1 assoc on group) */
                                          /* Must be more than 1 */
                                          /* client thread making */
                                          /* call on group. */ 
#define RPC_C_CN_DBG_GRP_ALLOC          3 /* server: make alloc fail */
#define RPC_C_CN_DBG_HEADER_FULL        4 /* server: make bind ACK too big */
                                          /* for large fragbuf */
#define RPC_C_CN_DBG_GRP_MAX_EXCEEDED   5 /* server: set grp_cur_assoc = */
                                          /* grp_max_assoc */
                                          /* (>1 assoc on group) */
                                          /* Must be more than 1 */
                                          /* client thread making */
                                          /* call on group. */ 
/*
 * Switches to set on server to generate bind ACKs with various
 * pprov reason codes.
 */
#define RPC_C_CN_DBG_IF_LOOKUP          6 /* server: cause the abstract */
                                          /* syntax to not be found */
#define RPC_C_CN_DBG_NO_XFER_SYNTAX     7 /* server: cause no matching xfer */
                                          /* syntax to be found */

/*
 * Various other switches to force errors.
 */
#define RPC_C_CN_DBG_SEC_ALLOC_FAIL     8 /* cause the sec_alloc to */
                                          /* fail */
#define RPC_C_CN_DBG_ASSOC_REQ_FAIL     9 /* cause the association */
                                          /* request eval event to */
                                          /* fail */

/*
 * Error switches to force packet fragmentation.
 */
#define RPC_C_CN_DBG_FRAG_BIND		10 /* bind (and alter_ctx) fragment */
#define RPC_C_CN_DBG_FRAG_BIND_ACK	11 /* bind_ack (and alter_ctx_resp) 
                                              fragment */

#define RPC_CN_DBG_RTN_PRINTF(s) RPC_DBG_PRINTF(rpc_e_dbg_general, \
                                                RPC_C_CN_DBG_ROUTINE_TRACE,\
                                                ("(" #s ")\n"))


/***********************************************************************/
/*
 * R P C _ C N _ M G M T _ T
 */
#include <cnpkt.h>
typedef struct 
{
    unsigned32          calls_sent;
    unsigned32          calls_rcvd;
    unsigned32          pkts_sent;
    unsigned32          pkts_rcvd;
    unsigned32          connections;
    unsigned32          closed_connections;
    unsigned32          alloced_assocs;
    unsigned32          dealloced_assocs;
    unsigned32          aborted_assocs;
    unsigned32          assoc_grps;
    struct cn_pkt_stats_t              /* Breakdown of pkts sent/rcvd by pkt type */
    {                               
        unsigned32 sent;
        unsigned32 rcvd;
    } pstats[RPC_C_CN_PKT_MAX_TYPE + 1];
} rpc_cn_mgmt_t, *rpc_cn_mgmt_p_t;
#define RPC_CN_STATS_INCR(s) (rpc_g_cn_mgmt.s++)

/***********************************************************************/
/*
 * R P C _ C N _ S M _ P R E D _ R E S U L T _ T
 */

#define RPC_C_SM_NO_PREDICATE           255
#define RPC_C_SM_NO_ACTION              255
#define RPC_C_SM_NO_NSTATE              255

/*
 * Returning the current error status can be accomplished by
 * taking no action.  The generic state machine evaluator will
 * automatically return the status of the last action routine
 * invoked.
 */
#define RPC_C_SM_RETURN_ERROR_STATUS RPC_C_SM_NO_ACTION

typedef struct
{
    unsigned8                           nstate;
    unsigned8                           action;
} rpc_cn_sm_pred_result_t, *rpc_cn_sm_pred_result_p_t;

/***********************************************************************/
/*
 * R P C _ C N _ S M _ S T A T E _ E N T R Y _ T
 */

#define RPC_C_NUM_PREDICATES 	3

/* 
 * The rpc_cn_sm_state_entry_t can contain either the action
 * or the next state.  If both are required, then we include the
 * action, which updates the next state internally.  Distinguish
 * between actions and states by numeric value.  States will
 * be some value (usually 0 -> 14) + rpc_c_cn_statebase. 
 */ 
typedef struct
{
    unsigned8                           action;  
} rpc_cn_sm_state_entry_t, *rpc_cn_sm_state_entry_p_t;

#define ILLEGAL_TRANSITION \
    { PROTOCOL_ERROR }

#define ILLEGAL_PREDICATE_RESULT \
    { RPC_C_SM_NO_NSTATE, \
      PROTOCOL_ERROR }

/*
 * R P C _ C N _ S M _ S T A T E _ T B L _ E N T R Y _ T
 */

typedef rpc_cn_sm_state_entry_t         rpc_cn_sm_state_tbl_entry_t[];

/*
 * R P C _ C N _ S M _ A C T I O N _ F N _ T
 */

typedef unsigned32     (*rpc_cn_sm_action_fn_t) (
    pointer_t   /*spc_struct*/,
    pointer_t   /*event_parameter*/,
    pointer_t   /*sm*/);

typedef rpc_cn_sm_action_fn_t      *rpc_cn_sm_action_fn_p_t;

/*
 * R P C _ C N _ S M _ P R E D I C A T E _ F N _ T
 */

typedef unsigned8 (*rpc_cn_sm_predicate_fn_t) (
    pointer_t   /*spc_struct*/,
    pointer_t   /*event_parameter*/);

typedef rpc_cn_sm_predicate_fn_t   *rpc_cn_sm_predicate_fn_p_t;

/*
 * R P C _ C N _ S M _ E V E N T _ E N T R Y _ T
 */

typedef struct
{
    unsigned8                           event_id;
    pointer_t                           event_param;
} rpc_cn_sm_event_entry_t, *rpc_cn_sm_event_entry_p_t;

/*
 * R P C _ C N _ S M _ E V E N T _ L I S T _ T
 */

#define RPC_C_CN_SM_EVENT_LIST_MAX_ENTRIES 2
typedef rpc_cn_sm_event_entry_t
        rpc_cn_sm_event_list_t [ RPC_C_CN_SM_EVENT_LIST_MAX_ENTRIES ]; 

/*
 * R P C _ C N _ S M _ C T L B L K _ T
 */
/*
 * State values are incremented by 100 to distinguish them from
 * action routine indexes which are all < 100.  This was done as
 * an efficiency measure to the engine, rpc__cn_sm_eval_event().
 */ 

/*
 * Performance Table ID defines 
 */
#define       rpc_c_cn_svr_assoc   1  /* server association tbl */
#define       rpc_c_cn_cl_assoc    2  /* client association tbl */
#define       rpc_c_cn_svr_call    3  /* server call rep tbl */
#define       rpc_c_cn_cl_call     4  /* client call rep tbl */
#define       rpc_c_cn_svr_a_g     5  /* server assoc group tbl */
#define       rpc_c_cn_cl_a_g	   6  /* client assoc group tbl */

typedef struct
{
    rpc_cn_sm_state_entry_p_t           *state_tbl;
    rpc_cn_sm_action_fn_t               *action_tbl;
#define RPC_C_SM_CLOSED_STATE           100
    unsigned8                           cur_state;
    unsigned8                           cur_event;
    unsigned32                          action_status;
    unsigned8                           event_list_hindex;
    unsigned8                           event_list_tindex;
#define RPC_C_CN_SM_EVENT_LIST_EMPTY       0
    unsigned8                           event_list_state;
    rpc_cn_sm_event_list_t              event_list;
    unsigned32				tbl_id; 
} rpc_cn_sm_ctlblk_t, *rpc_cn_sm_ctlblk_p_t;


/***********************************************************************/

/*
 * R P C _ C N _ F R A G B U F _ T
 */

#define RPC_C_CN_OVERHEAD_SIZE          4

typedef struct rpc_cn_fragbuf_s_t rpc_cn_fragbuf_t, *rpc_cn_fragbuf_p_t;
typedef void (*rpc_cn_fragbuf_dealloc_fn_t) (rpc_cn_fragbuf_t *);

struct rpc_cn_fragbuf_s_t
{
    rpc_list_t                  link;   /* MUST BE 1ST */
    unsigned32                  max_data_size;
    rpc_cn_fragbuf_dealloc_fn_t fragbuf_dealloc;
    pointer_t                   data_p;
    unsigned32                  data_size;
    unsigned8                   overhead_area[RPC_C_CN_OVERHEAD_SIZE];
    unsigned8                   data_area[1];
};

/***********************************************************************/

/*
 * R P C _ CN _ L O C A L _ I D _ T
 */

typedef struct
{
    unsigned16                          id_seqnum;
    unsigned16                          id_index;
} rpc_cn_local_id_parts_t, *rpc_cn_local_id_parts_p_t;
    
typedef union
{
    unsigned long                       all;
    rpc_cn_local_id_parts_t             parts;
} rpc_cn_local_id_t, *rpc_cn_local_id_p_t;

/*
 * R P C _ C N _ B I N D I N G _ R E P _ T
 */

typedef struct
{
    rpc_binding_rep_t                   common;
    rpc_cn_local_id_t                   grp_id;
    boolean                             being_resolved;
    dcethread*                          resolving_thread_id;
} rpc_cn_binding_rep_t, *rpc_cn_binding_rep_p_t;

/*
 * R P C _ C N _ C A L L _ R E P _ T
 */

typedef struct
{
    rpc_iovector_t                      iov;
    rpc_iovector_elt_t                  iov_elmts[ RPC_C_MAX_IOVEC_LEN - 1];
    unsigned32                          total_acc_byte_count;
    unsigned32                          cur_iov_index;
    unsigned32                          num_free_bytes;
    byte_p_t                            free_byte_ptr;
    unsigned32                          size_of_header;
    unsigned32                          trailer_pad;
} rpc_cn_buffered_output_t, *rpc_cn_buffered_output_p_t;

/*
 * Client cancel state information
 *
 * server_is_accepting : used to indicate when the first fragment
 *                       has been sent.
 * local_count         : # of cancels detected locally but not
 *                       forwarded yet. These may be forwarded
 *                       by setting the PFC_PENDING_ALERT bit in
 *                       the first fragment of a request. 
 * server_count        : # of cancels detected and forwarded
 *                       locally *not* including the PFC_PENDING_ALERT
 *                       bit in the first fragment of a request.
 * server_had_pending  : indicates whether the server completed
 *                       with a pending alert. If so the alert
 *                       should be re-generated before returning
 *                       to the client stub  
 */
typedef struct rpc_cn_cancel_info_s_t
{
    unsigned16                          local_count;
    unsigned16                          server_count;
    rpc_clock_t                         timeout_time;
    rpc_timer_t                         timer;
    unsigned                            timer_running: 1;
    unsigned                            server_is_accepting: 1;
    unsigned                            server_had_pending: 1;
    dcethread*                          thread_h;
} rpc_cn_cancel_info_t, *rpc_cn_cancel_info_p_t;

typedef struct rpc_cn_call_rep_s_t
{
    rpc_call_rep_t                      common;
    rpc_cn_sm_ctlblk_t                  call_state;
    unsigned32                          cn_call_status;
    rpc_binding_rep_t                   *binding_rep;
    struct rpc_cn_assoc_s_t             *assoc;
    rpc_cn_fragbuf_t                    *prot_header;
    rpc_cn_fragbuf_t                    *prot_tlr;
    unsigned32                          max_seg_size;   
    rpc_cn_buffered_output_t            buffered_output;
    unsigned16                          context_id;
    unsigned16                          num_pkts;
    rpc_transfer_syntax_t               transfer_syntax;
    unsigned16                          opnum;
    unsigned                            last_frag_received: 1;
    unsigned                            call_executed: 1;
    rpc_cn_sec_context_t                *sec;
    union
    {
        struct 
        {
            rpc_cn_fragbuf_t            *fault_data;
            rpc_cn_cancel_info_t        cancel;
        } client;
        struct
        {
            dce_uuid_t                      *if_id;
            unsigned32                  if_vers;
            unsigned16                  ihint;
            struct
            {
                unsigned16              local_count; /* common cancel count */
                                                     /* less pending */
                                                     /* alert bit cancel */
            } cancel;
        } server;
    } u;
} rpc_cn_call_rep_t, *rpc_cn_call_rep_p_t;

#define RPC_CN_CREP_SEND_HDR(cp) \
    (((rpc_cn_fragbuf_p_t)((cp)->prot_header))->data_p)
#define RPC_CN_CREP_ACC_BYTCNT(cp) \
    (((cp)->buffered_output).total_acc_byte_count)
#define RPC_CN_CREP_IOV(cp)                (((cp)->buffered_output).iov.elt)
#define RPC_CN_CREP_IOVLEN(cp)             (((cp)->buffered_output).iov.num_elt)
#define RPC_CN_CREP_CUR_IOV_INDX(cp)       (((cp)->buffered_output).cur_iov_index)
#define RPC_CN_CREP_FREE_BYTES(cp)         (((cp)->buffered_output).num_free_bytes)
#define RPC_CN_CREP_FREE_BYTE_PTR(cp)      (((cp)->buffered_output).free_byte_ptr)
#define RPC_CN_CREP_SIZEOF_HDR(cp)         (((cp)->buffered_output).size_of_header)
#define RPC_CN_CREP_SIZEOF_TLR_PAD(cp)     (((cp)->buffered_output).trailer_pad)

/*
 * R P C _ C N _ C R E P _ A D J _ F O R _ T L R
 * 
 * This macro will adjust all the appriate field in the call rep
 * "buffered_output" structure so that an authentication trailer can
 * be added to the packet. Note that the size of the trailer will be
 * rounded up to the nearest 8-byte boundary. This is so that any user
 * data placed in the PDU will also be a multiple of 8-bytes. This
 * rounding will be removed before the packet is sent.
 */
#define RPC_CN_CREP_ADJ_IOV_FOR_TLR(cp, header_p, auth_value_len)\
{\
    (cp)->prot_tlr->data_size = \
    ((auth_value_len + RPC_CN_PKT_SIZEOF_COM_AUTH_TLR + 7) & ~0x07);\
    RPC_CN_CREP_SIZEOF_TLR_PAD (cp) =\
       (cp)->prot_tlr->data_size - \
       auth_value_len - \
       RPC_CN_PKT_SIZEOF_COM_AUTH_TLR; \
    RPC_CN_CREP_SIZEOF_HDR (cp) += (cp)->prot_tlr->data_size;\
    RPC_CN_CREP_FREE_BYTES (cp) -= (cp)->prot_tlr->data_size;\
    RPC_CN_CREP_ACC_BYTCNT (cp) += (cp)->prot_tlr->data_size;\
    RPC_CN_CREP_IOVLEN (cp)++;\
    RPC_CN_PKT_AUTH_LEN (header_p) = auth_value_len;\
    /*\
     * Add the size of the auth trailer (plus padding to\
     * make it an 8-byte multiple to the data length of the\
     * protocol header iovector element. This is so that the\
     * trailer length will be taken into account when adding\
     * user data to the packet. It will be subtracted when the\
     * actual trailer is added to the packet in\
     * rpc__cn_transmit_buffers.\
     */\
    (RPC_CN_CREP_IOV (cp)[0]).data_len = RPC_CN_CREP_SIZEOF_HDR (cp);\
}


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ G R P _ T B L _ T
 */

typedef struct
{
    unsigned16                          grp_count;
    unsigned16                          grp_active_count;
    rpc_timer_t                         grp_client_timer;
    rpc_timer_t                         grp_server_timer;
    struct rpc_cn_assoc_grp_s_t         *assoc_grp_vector;
} rpc_cn_assoc_grp_tbl_t, *rpc_cn_assoc_grp_tbl_p_t;

/*
 * R P C _ C N _ S Y N T A X _ T
 */

typedef void    (*rpc_cn_marshal_fn_t) (void);

typedef struct rpc_cn_syntax_s_t
{
    rpc_list_t                          link;   /* MUST BE 1ST */
    rpc_syntax_id_t                     syntax_abstract_id;
    rpc_syntax_id_t                     syntax_transfer_id;
    unsigned16                          syntax_pres_id;
    unsigned16                          syntax_vector_index;
    rpc_syntax_vector_t                 *syntax_vector;
    rpc_cn_marshal_fn_t                 *syntax_epv;
    unsigned16                          syntax_ihint;
    unsigned                            syntax_valid: 1;
    unsigned32                          syntax_status;
    unsigned32                          syntax_call_id;
} rpc_cn_syntax_t, *rpc_cn_syntax_p_t;


/***********************************************************************/
/*
 * R P C _ A S S O C _ S M _ W O R K _ T
 *
 * This structure is used to hold various pieces of information
 * which are needed by action routines in the association state
 * machine. 
 */
typedef struct
{
    unsigned32                  grp_id;
    rpc_cn_syntax_t             *pres_context;
    boolean                     reuse_context;
    rpc_cn_sec_context_t        *sec_context;
} rpc_cn_assoc_sm_work_t, *rpc_cn_assoc_sm_work_p_t;

/* 
 * R P C _ C N _ A S S O C _ G R P _ T
 */

#define RPC_C_CN_ASSOC_GRP_CLIENT       1
#define RPC_C_CN_ASSOC_GRP_SERVER       2

typedef struct rpc_cn_assoc_grp_s_t
{
    rpc_cn_sm_ctlblk_t                  grp_state;
    unsigned32                          grp_status;
    unsigned16                          grp_flags;
    unsigned16                          grp_refcnt;
    rpc_addr_p_t                        grp_address;
    rpc_addr_p_t                        grp_secaddr;
    rpc_transport_info_p_t              grp_transport_info;
    rpc_cn_local_id_t                   grp_id;
    unsigned16                          grp_max_assoc;
    unsigned16                          grp_cur_assoc;
    unsigned16                          grp_assoc_waiters;
    rpc_cond_t                          grp_assoc_wt;
    rpc_list_t                          grp_assoc_list;
    unsigned16                          grp_callcnt;
    rpc_network_rundown_fn_t            grp_liveness_mntr;
    rpc_cn_local_id_t                   grp_remid;
    unsigned32                          grp_next_key_id;
} rpc_cn_assoc_grp_t, *rpc_cn_assoc_grp_p_t;

/*
 * R P C _ C N _ C T L B L K _ T
 */

typedef struct
{
    unsigned16 volatile                 cn_state;
    unsigned16 volatile                 cn_rcvr_waiters;
    rpc_mutex_t                         cn_rcvr_mutex; /* unused so far */
    rpc_cond_t                          cn_rcvr_cond;
    dcethread*                          cn_rcvr_thread_id;
    unsigned_char_t                     *cn_listening_endpoint;
    rpc_socket_t volatile               cn_sock;
    rpc_addr_p_t                        rpc_addr;
    unsigned volatile                   exit_rcvr : 1;
    unsigned volatile                   in_sendmsg : 1;
    unsigned volatile                   waiting_for_sendmsg_complete : 1;
} rpc_cn_ctlblk_t, *rpc_cn_ctlblk_p_t;

#define RPC_CN_ASSOC_LOCK(__assoc)	RPC_MUTEX_LOCK((__assoc)->cn_ctlblk.cn_rcvr_mutex)
#define RPC_CN_ASSOC_UNLOCK(__assoc)	RPC_MUTEX_UNLOCK((__assoc)->cn_ctlblk.cn_rcvr_mutex)

/*
 * R P C _ C N _ A S S O C _ T
 */

#define RPC_C_CN_ASSOC_CLIENT                   0x00000001
#define RPC_C_CN_ASSOC_SERVER                   0x00000002
#define RPC_C_CN_ASSOC_SHUTDOWN_REQUESTED       0x00000004
#define RPC_C_CN_ASSOC_SCANNED                  0x00000008
#define RPC_C_CN_ASSOC_AUTH_EXPECTED            0x00000010

struct rpc_cn_assoc_s_t
{
    rpc_list_t                          link;   /* MUST BE 1ST */
    rpc_cn_sm_ctlblk_t                  assoc_state;
    unsigned32                          assoc_status;
    unsigned32                          assoc_local_status;
    unsigned16                          assoc_flags;
    unsigned16                          assoc_ref_count;
    unsigned16                          assoc_acb_ref_count;
#define RPC_C_CN_ASSOC_SERVER_MAX_SHUTDOWN_REQ_COUNT 2
    unsigned16                          assoc_shutdown_req_count;

    rpc_cn_local_id_t                   assoc_grp_id;
    unsigned16                          assoc_msg_waiters;
    rpc_cond_t                          assoc_msg_cond;
    unsigned16                          assoc_max_xmit_frag;
    unsigned16                          assoc_max_recv_frag;
    unsigned8                           assoc_vers_minor;
    unsigned32                          assoc_pres_context_id;
    ndr_format_t                        assoc_remote_ndr_format;
    rpc_cn_fragbuf_t                    assoc_dummy_fragbuf;
    rpc_cn_call_rep_t                   *call_rep;
    rpc_cn_ctlblk_t                     cn_ctlblk;
    rpc_list_t                          syntax_list;    /* rpc_cn_syntax_t */
    rpc_list_t                          msg_list;       /* rpc_cn_fragbuf_t */
    rpc_cn_assoc_sec_context_t          security;
    rpc_transport_info_p_t              transport_info;
    rpc_cn_fragbuf_p_t                  raw_packet_p;
    rpc_cn_assoc_sm_work_p_t            assoc_sm_work;
    unsigned32                          bind_packets_sent;
    signed long long                    alter_call_id;
};
// rpc_cn_assoc_t and rpc_cn_assoc_p_t are declared in comsoc.h

/*
 * If KRB_CN is on, or it isn't explicitly off, turn on CN_AUTH.
 */
#if	defined(AUTH_KRB_CN) || !defined(AUTH_KRB_DG)
#define CN_AUTH
#endif

/*
 * Authentication interface macros. These will dispatch through the
 * authentication protocol ID table using an auth prot ID and the CN
 * RPC prot ID.
 */
#ifdef CN_AUTH
#define RPC_CN_AUTH_PROT_EPV(prot)              (rpc_cn_auth_epv_t *)(rpc__auth_rpc_prot_epv(prot, RPC_C_PROTOCOL_ID_NCACN))
#else
#define RPC_CN_AUTH_PROT_EPV(prot)              (rpc_cn_auth_epv_t *)(0xbabababa)
#endif /* CN_AUTH */

#define RPC_CN_AUTH_SEC_THREE_WAY(sec, ind)\
{\
    ind = (*(sec)->sec_cn_info->cn_epv->three_way)();\
}

#define RPC_CN_AUTH_THREE_WAY(prot, ind)\
{\
    rpc_cn_auth_epv_t   *_cn_epv;\
    _cn_epv = RPC_CN_AUTH_PROT_EPV(prot);\
    ind = (*_cn_epv->three_way)();\
}

#define RPC_CN_AUTH_CONTEXT_VALID(sec, st)\
    (*(sec)->sec_cn_info->cn_epv->context_valid)(sec, st)

#define RPC_CN_AUTH_CREATE_INFO(prot, level, info, st)\
{\
    rpc_cn_auth_epv_t   *_cn_epv;\
    _cn_epv = RPC_CN_AUTH_PROT_EPV(prot);\
    (*_cn_epv->create_info)(level, info, st);\
}

#define RPC_CN_AUTH_CRED_CHANGED(sec, st)\
    (*(sec)->sec_cn_info->cn_epv->cred_changed)(sec, st)

#define RPC_CN_AUTH_CRED_REFRESH(auth_info, st)\
{\
    rpc_cn_auth_epv_t   *_cn_epv;\
    _cn_epv = RPC_CN_AUTH_PROT_EPV((auth_info)->authn_protocol);\
    (*_cn_epv->cred_refresh)(auth_info, st);\
}

#define RPC_CN_AUTH_FMT_CLIENT_REQ(assoc_sec, sec, auth_value, auth_value_len, last_auth_pos, auth_len_remain, retry, st)\
    (*(sec)->sec_cn_info->cn_epv->fmt_client_req)(assoc_sec, sec, auth_value, auth_value_len, last_auth_pos, auth_len_remain, retry, st)

#define RPC_CN_AUTH_FMT_SRVR_RESP(verify_st, assoc_sec, sec, req_auth_value, req_auth_value_len, auth_value, auth_value_len)\
    (*(sec)->sec_cn_info->cn_epv->fmt_srvr_resp)(verify_st,assoc_sec,sec,\
                                                 req_auth_value, req_auth_value_len, auth_value, auth_value_len)

#define RPC_CN_AUTH_FREE_PROT_INFO(info, cn_info)\
    (*(*(cn_info))->cn_epv->free_prot_info)(info, cn_info)

#define RPC_CN_AUTH_GET_PROT_INFO(info, cn_info, st)\
{\
    rpc_cn_auth_epv_t   *_cn_epv;\
    _cn_epv = RPC_CN_AUTH_PROT_EPV((info)->authn_protocol);\
    (*_cn_epv->get_prot_info)(info, cn_info, st);\
}

#define RPC_CN_AUTH_PRE_CALL(assoc_sec, sec, auth_value, auth_value_len, st)\
    (*(sec)->sec_cn_info->cn_epv->pre_call)(assoc_sec, sec, auth_value, auth_value_len, st);

#define RPC_CN_AUTH_PRE_SEND(assoc_sec, sec, iovp, iovlen, out_iov, st)\
    (*(sec)->sec_cn_info->cn_epv->pre_send)(assoc_sec, sec, iovp, iovlen, out_iov, st);

#define RPC_CN_AUTH_RECV_CHECK(authn_prot, assoc_sec, sec, pdu, pdu_len, cred_len, auth_tlr, unpack_ints, st)\
{\
    if (sec == NULL)\
    {\
        rpc_cn_auth_epv_t   *_cn_epv;\
        _cn_epv = RPC_CN_AUTH_PROT_EPV(authn_prot);\
        (*_cn_epv->recv_check)(assoc_sec, sec, pdu, pdu_len, cred_len,\
                               auth_tlr, unpack_ints, st);\
    }\
    else\
    {\
        (*(sec)->sec_cn_info->cn_epv->recv_check)(assoc_sec, sec, pdu,\
             pdu_len, cred_len, auth_tlr, unpack_ints, st);\
    }\
}

#define RPC_CN_AUTH_TLR_UUID_CRC(prot, auth_value, auth_value_len, assoc_uuid_crc)\
{\
    rpc_cn_auth_epv_t   *_cn_epv;\
    _cn_epv = RPC_CN_AUTH_PROT_EPV(prot);\
    (*_cn_epv->tlr_uuid_crc)(auth_value, auth_value_len, assoc_uuid_crc);\
}

#define RPC_CN_AUTH_TLR_UNPACK(prot, pkt_p, auth_value_len, packed_drep)\
{\
    rpc_cn_auth_epv_t   *_cn_epv;\
    _cn_epv = RPC_CN_AUTH_PROT_EPV(prot);\
    (*_cn_epv->tlr_unpack)(pkt_p, auth_value_len, packed_drep);\
}

#define RPC_CN_AUTH_VFY_CLIENT_REQ(assoc_sec, sec, auth_value, auth_value_len, old_client, st)\
    (*(sec)->sec_cn_info->cn_epv->vfy_client_req)(assoc_sec, sec, auth_value, auth_value_len, old_client, st)

#define RPC_CN_AUTH_VFY_SRVR_RESP(assoc_sec, sec, auth_value, auth_value_len, st)\
    (*(sec)->sec_cn_info->cn_epv->vfy_srvr_resp)(assoc_sec, sec, auth_value, auth_value_len, st)


/*
 * These macros are RPC protocol independent.
 */
#define RPC_CN_AUTH_ADD_REFERENCE(info)         rpc__auth_info_reference(info)
#define RPC_CN_AUTH_RELEASE_REFERENCE(info)     rpc__auth_info_release(info)
#ifdef CN_AUTH
#define RPC_CN_AUTH_CVT_ID_API_TO_WIRE(id,st)   rpc__auth_cvt_id_api_to_wire(\
id,st)
#define RPC_CN_AUTH_CVT_ID_WIRE_TO_API(id,st)   rpc__auth_cvt_id_wire_to_api(\
id,st)
#define RPC_CN_AUTH_INQ_SUPPORTED(id)           rpc__auth_inq_supported(id)
#else
#define RPC_CN_AUTH_CVT_ID_API_TO_WIRE(id,st)   0
#define RPC_CN_AUTH_CVT_ID_WIRE_TO_API(id,st)   0
#define RPC_CN_AUTH_INQ_SUPPORTED(id)           false
#endif

/*
 * R P C _ C N _ A L I G N _ P T R
 *
 * RPC Pointer Alignment macro
 *
 * Casting to (unsigned long) is needed because bitwise operations are not
 * allowed with pointers as an operand.
 *
 * NOTE: Assumption sizeof(unsigned long) = sizeof(unsigned8 *).  This
 *       assumption may not be correct for all machines.
 */

/* ??? */

#define RPC_CN_ALIGN_PTR(ptr, boundary) \
    ((unsigned long) ((unsigned8 *)(ptr) + ((boundary)-1)) & ~((boundary)-1))

/*
 * R P C _ C N _ A U T H _ R E Q U I R E D
 */

#define RPC_CN_AUTH_REQUIRED(info) ((info != NULL) && \
                                    ((info)->authn_protocol != rpc_c_authn_none))

#define RPC_CN_PKT_AUTH_REQUIRED(info) ((info != NULL) && \
                                    ((info)->authn_level != rpc_c_protect_level_none) && \
                                    ((info)->authn_level != rpc_c_protect_level_connect))

/*
 * R P C _ G _ C N _ A S S O C _ G R P _ T B L
 */

EXTERNAL rpc_cn_assoc_grp_tbl_t rpc_g_cn_assoc_grp_tbl;

/*
 * R P C _ G _ C N _ S Y N T A X _ L O O K A S I D E _ L I S T
 */

#define RPC_C_CN_SYNTAX_LOOKASIDE_MAX           16
EXTERNAL rpc_list_desc_t        rpc_g_cn_syntax_lookaside_list;

/*
 * R P C _ G _ C N _ S E C _ L O O K A S I D E _ L I S T
 */

#define RPC_C_CN_SEC_LOOKASIDE_MAX              16
EXTERNAL rpc_list_desc_t        rpc_g_cn_sec_lookaside_list;

/*
 * R P C _ G _ C N _ A S S O C _ L O O K A S I D E _ L I S T
 */

#define RPC_C_CN_ASSOC_LOOKASIDE_MAX            16
EXTERNAL rpc_list_desc_t        rpc_g_cn_assoc_lookaside_list;

/*
 * R P C _ G _ C N _ C A L L _ R E P _ L O O K A S I D E _ L I S T
 */

#define RPC_C_CN_CALL_LOOKASIDE_MAX             0
EXTERNAL rpc_list_desc_t        rpc_g_cn_call_lookaside_list;

/*
 * R P C _ G _ C N _ B I N D I N G _ L O O K A S I D E _ L I S T
 */

#define RPC_C_CN_BINDING_LOOKASIDE_MAX          8
EXTERNAL rpc_list_desc_t          rpc_g_cn_binding_lookaside_list;


/*
 * R P C _ G _ C N _ [ L G , S M ] _ F R A G B U F _ L I S T
 */

#define RPC_C_CN_FRAGBUF_LOOKASIDE_MAX          0
EXTERNAL rpc_list_desc_t          rpc_g_cn_lg_fbuf_lookaside_list;
EXTERNAL rpc_list_desc_t          rpc_g_cn_sm_fbuf_lookaside_list;

/*
 * R P C _ G _ C N _ L O O K A S I D E _ C O N D
 */

EXTERNAL rpc_cond_t             rpc_g_cn_lookaside_cond;

/*
 * R P C _ G _ C N _ C A L L _ I D
 */

EXTERNAL unsigned32             rpc_g_cn_call_id;

/*
 * R P C _ G _ C N _ M G M T
 */
EXTERNAL rpc_cn_mgmt_t          rpc_g_cn_mgmt;

#endif /* _CNP_H */
