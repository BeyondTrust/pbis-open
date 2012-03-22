/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
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
**      cn.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types/constants external to NCA Connection RPC
**  Protocol Service for use by other runtime components.
**
**
*/

#ifndef _CN_H
#define _CN_H	1

#include <cnpkt.h>
#include <dce/sec_authn.h>

/*
 * rpc_e_dbg_auth debug switch levels
 */


#define RPC_C_CN_DBG_AUTH_ROUTINE_TRACE 20
#define RPC_C_CN_DBG_AUTH_PKT           7
#define RPC_C_CN_DBG_AUTH_REFRESH       5
#define RPC_C_CN_DBG_AUTH_BIG_PAC       3
#define RPC_C_CN_DBG_AUTH_GENERAL       1


/*
 * rpc_e_dbg_cn_errors debug switch levels
 *
 * Switches to set to generate errors and a status code used only
 * when one of the error debug levels are set.
 */

#define RPC_S_CN_DBG_AUTH_FAILURE               0xdeadbeefU
#define RPC_C_CN_DBG_AUTH_CREATE_INFO           64      /* server */
#define RPC_C_CN_DBG_AUTH_CRED_CHANGED          65      /* client */
#define RPC_C_CN_DBG_AUTH_FMT_CLIENT_REQ        66      /* client */
#define RPC_C_CN_DBG_AUTH_FMT_SERVER_RESP       67      /* server */
#define RPC_C_CN_DBG_AUTH_GET_PROT_INFO         68      /* client & server */
#define RPC_C_CN_DBG_AUTH_PRE_CALL              69      /* client & server */
#define RPC_C_CN_DBG_AUTH_PRE_SEND              70      /* client & server */
#define RPC_C_CN_DBG_AUTH_RECV_CHECK            71      /* client & server */
#define RPC_C_CN_DBG_AUTH_VFY_CLIENT_REQ        72      /* server */
#define RPC_C_CN_DBG_AUTH_VFY_SERVER_RESP       73      /* client */
#define RPC_C_CN_DBG_AUTH_CONTEXT_VALID         74      /* client */
#define RPC_C_CN_DBG_AUTH_CRED_REFRESH          75      /* client */

/*
 * NOTE: rpc_c_cn_large_frag_size must always be at least
 * rpc_c_assoc_must_recv_frag_size as defined in cnassm.h. This is
 * an architectural requirement which is checked in cnfbuf.h.
 */

#define RPC_C_CN_LARGE_FRAG_SIZE        4096
#define RPC_C_CN_SMALL_FRAG_SIZE        256

/*
 * R P C _ C N _ A U T H _ I N F O _ T
 */

typedef struct rpc_cn_auth_info_s_t
{
    struct rpc_cn_auth_epv_s_t  *cn_epv;
} rpc_cn_auth_info_t, *rpc_cn_auth_info_p_t;

/*
 * R P C _ C N _ S E C _ C O N T E X T _ T
 */

#define RPC_C_SEC_STATE_INVALID			0
#define RPC_C_SEC_STATE_INCOMPLETE		1
#define RPC_C_SEC_STATE_COMPLETE		2

#define RPC_C_SEC_FLAGS_SIGN_HEADER             (0x00000001)

typedef struct rpc_cn_sec_context_s_t
{
    rpc_list_t           link;         /* MUST BE 1ST                    */
    unsigned8            sec_state;
    unsigned32           sec_status;
    unsigned32           sec_key_id;
    unsigned32           sec_last_call_id;
    unsigned32           sec_flags;
    rpc_auth_info_t      *sec_info;
    rpc_cn_auth_info_t   *sec_cn_info;
} rpc_cn_sec_context_t, *rpc_cn_sec_context_p_t;

/*
 * R P C _ C N _ S E C _ A U T H _ B U F F E R _ I N F O _ T
 *
 * Used to piece together auth info from rpc_bind and alter_context PDU's
 */
typedef struct rpc_cn_sec_auth_buffer_info_s_t
{
    unsigned32           auth_buffer_len;   /* number of bytes       */
    unsigned32           auth_buffer_max;   /* actual memory used    */
    unsigned8            *auth_buffer;      /* reconstruction buffer */
} rpc_cn_sec_auth_buffer_info_t, rpc_cn_sec_auth_buffer_info_p_t;

/*
 * R P C _ C N _ A S S O C _ S E C _ C O N T E X T _ T
 */

typedef struct rpc_cn_assoc_sec_context_s_t
{
    unsigned                            assoc_have_uuid_crc : 1;    /* server only */
    unsigned32                          assoc_uuid_crc;
    unsigned32                          assoc_next_snd_seq;
    unsigned32                          assoc_next_rcv_seq;
    rpc_cn_sec_context_t                *assoc_current_sec_context; /* server only */
    sec_krb_message			krb_message;	/* for rpc__krb_cn_fmt_client_req */
    rpc_cn_sec_auth_buffer_info_t       auth_buffer_info;
    rpc_list_t                          context_list;   /* rpc_cn_sec_context_t */
} rpc_cn_assoc_sec_context_t, *rpc_cn_assoc_sec_context_p_t;

/*
 * The CN specific authentication protocol EPV.
 */
typedef boolean32 (*rpc_cn_auth_context_valid_fn_t) (
        rpc_cn_sec_context_p_t         /*  sec */,
        unsigned32                   * /* st */
    );

typedef void (*rpc_cn_auth_create_info_fn_t) (
       rpc_authn_level_t                /* authn_level */,
       rpc_authn_flags_t                /* authn_flags */,
       rpc_auth_info_p_t                * /* auth_info */,
       unsigned32                       * /* st*/
    );

typedef boolean32 (*rpc_cn_auth_cred_changed_fn_t) (
        rpc_cn_sec_context_p_t         /*  sec */,
        unsigned32                     * /* st */
    );

/*
 * This routine is not CN specific and could be moved into the
 * protocol-independent auth interface.
 */
typedef void (*rpc_cn_auth_cred_refresh_fn_t) (
        rpc_auth_info_p_t              /*  auth_info */,
        unsigned32                      * /* st */
    );

typedef void (*rpc_cn_auth_fmt_client_req_fn_t) (
        rpc_cn_assoc_sec_context_p_t      /* assoc_sec */,
        rpc_cn_sec_context_p_t            /* sec */,
        pointer_t                         /* auth_value */,
        unsigned32                      * /* auth_value_len */,
        pointer_t                       * /* last_auth_pos */,
        unsigned32                      * /* auth_len_remain */,
        unsigned32                        /* old_server */,
        unsigned32                      * /* st */
    );

typedef void (*rpc_cn_auth_fmt_srvr_resp_fn_t) (
        unsigned32                     /*  verify_st */,
        rpc_cn_assoc_sec_context_p_t   /*  assoc_sec */,
        rpc_cn_sec_context_p_t         /*  sec */,
        pointer_t                      /*  req_auth_value */,
        unsigned32                     /*  req_auth_value_len */,
        pointer_t                      /*  auth_value */,
        unsigned32                      * /* auth_value_len */
    );

typedef void (*rpc_cn_auth_free_prot_info_fn_t) (
        rpc_auth_info_p_t              /*  info */,
        rpc_cn_auth_info_p_t            * /* cn_info */
    );

typedef void (*rpc_cn_auth_get_prot_info_fn_t) (
        rpc_auth_info_p_t              /*  info */,
        rpc_cn_auth_info_p_t           * /* cn_info */,
        unsigned32                      * /* st */
    );

typedef void (*rpc_cn_auth_pre_call_fn_t) (
        rpc_cn_assoc_sec_context_p_t   /*  assoc_sec */,
        rpc_cn_sec_context_p_t         /*  sec */,
        pointer_t                      /*  auth_value */,
        unsigned32                     * /*  auth_value_len */,
        unsigned32                      * /* st */
    );

typedef void (*rpc_cn_auth_pre_send_fn_t) (
        rpc_cn_assoc_sec_context_p_t    /* assoc_sec */,
        rpc_cn_sec_context_p_t          /* sec */,
        rpc_socket_iovec_p_t            /* iov */,
        unsigned32                      /* iovlen */,
        rpc_socket_iovec_p_t            /* out_iov */,
        unsigned32                      *st
    );

typedef void (*rpc_cn_auth_recv_check_fn_t) (
        rpc_cn_assoc_sec_context_p_t    /* assoc_sec */,
        rpc_cn_sec_context_p_t          /* sec */,
        rpc_cn_common_hdr_p_t           /* pdu */,
        unsigned32                      /* pdu_len */,
        unsigned32                      /* cred_len */,
        rpc_cn_auth_tlr_p_t             /* auth_tlr */,
        boolean32                       /* unpack_ints */,
        unsigned32                      * /* st */
    );

typedef void (*rpc_cn_auth_tlr_uuid_crc_fn_t) (
        pointer_t               /* auth_value */,
        unsigned32              /* auth_value_len */,
        unsigned32              * /* uuid_crc */
    );

typedef void (*rpc_cn_auth_tlr_unpack_fn_t) (
        rpc_cn_packet_p_t       /* pkt_p */,
        unsigned32              /* auth_value_len */,
        unsigned8               * /* packed_drep */
    );

typedef boolean32 (*rpc_cn_auth_three_way_fn_t) (void);

typedef void (*rpc_cn_auth_vfy_client_req_fn_t) (
        rpc_cn_assoc_sec_context_p_t    /* assoc_sec */,
        rpc_cn_sec_context_p_t          /* sec */,
        pointer_t                       /* auth_value */,
        unsigned32                      /* auth_value_len */,
	unsigned32		        /* old_client */,
        unsigned32                      * /* st */
    );

typedef void (*rpc_cn_auth_vfy_srvr_resp_fn_t) (
        rpc_cn_assoc_sec_context_p_t    /* assoc_sec */,
        rpc_cn_sec_context_p_t          /* sec */,
        pointer_t                       /* auth_value */,
        unsigned32                      /* auth_value_len */,
        unsigned32                      * /* st */
    );

typedef struct rpc_cn_auth_epv_s_t
{
    rpc_cn_auth_three_way_fn_t          three_way;
    rpc_cn_auth_context_valid_fn_t      context_valid;
    rpc_cn_auth_create_info_fn_t        create_info;
    rpc_cn_auth_cred_changed_fn_t       cred_changed;
    rpc_cn_auth_cred_refresh_fn_t       cred_refresh;
    rpc_cn_auth_fmt_client_req_fn_t     fmt_client_req;
    rpc_cn_auth_fmt_srvr_resp_fn_t      fmt_srvr_resp;
    rpc_cn_auth_free_prot_info_fn_t     free_prot_info;
    rpc_cn_auth_get_prot_info_fn_t      get_prot_info;
    rpc_cn_auth_pre_call_fn_t           pre_call;
    rpc_cn_auth_pre_send_fn_t           pre_send;
    rpc_cn_auth_recv_check_fn_t         recv_check;
    rpc_cn_auth_tlr_uuid_crc_fn_t       tlr_uuid_crc;
    rpc_cn_auth_tlr_unpack_fn_t         tlr_unpack;
    rpc_cn_auth_vfy_client_req_fn_t     vfy_client_req;
    rpc_cn_auth_vfy_srvr_resp_fn_t      vfy_srvr_resp;
} rpc_cn_auth_epv_t , *rpc_cn_auth_epv_p_t;

PRIVATE unsigned32      rpc__cn_crc_compute (
        unsigned8       * /* block */,
        unsigned32      /* block_len */
    );
#endif /* _CN_H */
