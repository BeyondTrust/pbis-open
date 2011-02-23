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
#ifndef _COMPROT_H
#define _COMPROT_H
/*
**
**  NAME
**
**      comprot.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Generic interface to RPC Protocol Services.
**
**
*/

#include <dce/dce.h>


#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************/
/*
 * The Call EPV. 
 */
typedef rpc_call_rep_t *(*rpc_prot_call_start_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        unsigned32                  /* in  */    /*call_options*/,
        rpc_if_rep_p_t              /* in  */    /*ifspec_h*/,
        unsigned32                  /* in  */    /*opn*/,
        rpc_transfer_syntax_t       /* in  */   * /*transfer_syntax*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_call_transmit_fn_t) (
        rpc_call_rep_p_t            /* in  */    /*call_r*/,
        rpc_iovector_p_t            /* in  */    /*call_args*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_call_transceive_fn_t) (
        rpc_call_rep_p_t            /* in  */    /*call_r*/,
        rpc_iovector_p_t            /* in  */    /*in_call_args*/,
        rpc_iovector_elt_t          /* out */   * /*out_call_args*/,
        ndr_format_t                /* out */   * /*remote_ndr_format*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_call_receive_fn_t) (
        rpc_call_rep_p_t            /* in  */    /*call_r*/,
        rpc_iovector_elt_t          /* out */   * /*call_args*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_call_end_fn_t) (
        rpc_call_rep_p_t            /* in,out */ * /*call_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_call_blk_til_free_fn_t) (
        rpc_call_rep_p_t            /* in  */    /*call_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_call_transmit_fault_fn_t) (
        rpc_call_rep_p_t            /* in  */    /*call_r*/,
        rpc_iovector_p_t            /* in  */    /*call_fault_info*/, 
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_call_cancel_fn_t) (
        rpc_call_rep_p_t            /* in  */    /*call_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_call_receive_flt_fn_t) (
        rpc_call_rep_p_t            /* in  */    /*call_r*/,
        rpc_iovector_elt_t          /* out */   * /*fault_info*/,
        ndr_format_t                /* out */   * /*remote_ndr_format*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef boolean32 (*rpc_prot_call_did_mgr_execute_fn_t) (
        rpc_call_rep_p_t            /* in  */    /*call_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef struct
{
    rpc_prot_call_start_fn_t        call_start;
    rpc_prot_call_transmit_fn_t     call_transmit;
    rpc_prot_call_transceive_fn_t   call_transceive;
    rpc_prot_call_receive_fn_t      call_receive;
    rpc_prot_call_end_fn_t          call_end;
    rpc_prot_call_blk_til_free_fn_t call_block_until_free;
    rpc_prot_call_transmit_fault_fn_t call_transmit_fault;
    rpc_prot_call_cancel_fn_t       call_cancel;
    rpc_prot_call_receive_flt_fn_t  call_receive_fault;
    rpc_prot_call_did_mgr_execute_fn_t  call_did_mgr_execute;
} rpc_prot_call_epv_t, *rpc_prot_call_epv_p_t;

/***********************************************************************/
/*
 * The Management EPV. 
 */
typedef unsigned32 (*rpc_prot_mgt_inq_call_sent_fn_t) (
        void
    );

typedef unsigned32 (*rpc_prot_mgt_inq_call_rcvd_fn_t) (
        void
    );

typedef unsigned32 (*rpc_prot_mgt_inq_pkts_sent_fn_t) (
        void
    );

typedef unsigned32 (*rpc_prot_mgt_inq_pkts_rcvd_fn_t) (
        void
    );

typedef struct
{
    rpc_prot_mgt_inq_call_sent_fn_t mgmt_inq_calls_sent;
    rpc_prot_mgt_inq_call_rcvd_fn_t mgmt_inq_calls_rcvd;
    rpc_prot_mgt_inq_pkts_sent_fn_t mgmt_inq_pkts_sent;
    rpc_prot_mgt_inq_pkts_rcvd_fn_t mgmt_inq_pkts_rcvd;
} rpc_prot_mgmt_epv_t, *rpc_prot_mgmt_epv_p_t;

/***********************************************************************/
/*
 * The Binding EPV.
 */
typedef rpc_binding_rep_t *(*rpc_prot_binding_alloc_fn_t) (
        boolean32                   /* in  */    /*is_server*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_binding_init_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );    

typedef void (*rpc_prot_binding_reset_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );    

typedef void (*rpc_prot_binding_changed_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );    

typedef void (*rpc_prot_binding_free_fn_t) (
        rpc_binding_rep_p_t         /* in,out */ * /*binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_binding_inq_addr_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        rpc_addr_p_t                /* out */   * /*rpc_addr*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_binding_inq_client_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        rpc_client_handle_t         /* out */   * /*client_h*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_binding_copy_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*src_binding_r*/,
        rpc_binding_rep_p_t         /* in  */    /*dst_binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_binding_cross_fork_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );
        
typedef struct
{
    rpc_prot_binding_alloc_fn_t         binding_alloc;
    rpc_prot_binding_init_fn_t          binding_init;
    rpc_prot_binding_reset_fn_t         binding_reset;
    rpc_prot_binding_changed_fn_t       binding_changed;
    rpc_prot_binding_free_fn_t          binding_free;
    rpc_prot_binding_inq_addr_fn_t      binding_inq_addr;
    rpc_prot_binding_inq_client_fn_t    binding_inq_client;
    rpc_prot_binding_copy_fn_t          binding_copy;
    rpc_prot_binding_cross_fork_fn_t    binding_cross_fork;
} rpc_prot_binding_epv_t, *rpc_prot_binding_epv_p_t;

/***********************************************************************/
/*
 * The Network EPV.
 */
typedef void (*rpc_prot_net_use_protseq_fn_t) (
        rpc_protseq_id_t            /* in  */    /*pseq_id*/,
        unsigned32                  /* in  */    /*max_calls*/,
        rpc_addr_p_t                /* in  */    /*rpc_addr*/,
        unsigned_char_p_t           /* in  */    /*endpoint*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_net_mon_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        rpc_client_handle_t         /* in  */    /*client_h*/,
        rpc_network_rundown_fn_t    /* in  */    /*rundown*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_net_stop_mon_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        rpc_client_handle_t         /* in  */    /*client_h*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_net_maint_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_net_stop_maint_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_net_close_fn_t) (
        rpc_binding_rep_p_t         /* in  */    /*binding_r*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_net_select_disp_fn_t) (
        rpc_socket_t                /* in  */    /*desc*/,
        pointer_t                   /* in  */    /*priv_info*/,
        boolean32                   /* in  */    /*is_active*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef void (*rpc_prot_net_inq_prot_vers_fn_t) (
        unsigned8                   /* out */   * /*prot_id*/,
        unsigned32                  /* out */   * /*version_major*/,
        unsigned32                  /* out */   * /*version_minor*/,
        unsigned32                  /* out */   * /*st*/
    );

typedef struct
{
    rpc_prot_net_use_protseq_fn_t   network_use_protseq;
    rpc_prot_net_mon_fn_t           network_mon;
    rpc_prot_net_stop_mon_fn_t      network_stop_mon;
    rpc_prot_net_maint_fn_t         network_maint;
    rpc_prot_net_stop_maint_fn_t    network_stop_maint;
    rpc_prot_net_select_disp_fn_t   network_select_disp;
    rpc_prot_net_inq_prot_vers_fn_t network_inq_prot_vers;
    rpc_prot_net_close_fn_t         network_close;
} rpc_prot_network_epv_t, *rpc_prot_network_epv_p_t;


/***********************************************************************/
/*
 * Signature of the fork handling routines.
 *
 */
typedef void (*rpc_prot_fork_handler_fn_t) (  
        rpc_fork_stage_id_t     operation
    );

/***********************************************************************/
/*
 * Signature of the init routine provided.
 *
 * Note: double underscores were added to these names just to diffrentiate
 * them from the names of structure members that bother VAXC
 */
typedef void (*rpc_prot_init_fn_t) (
        rpc_prot_call_epv_p_t       * /*call__epv*/,
        rpc_prot_mgmt_epv_p_t       * /*mgmt__epv*/,
        rpc_prot_binding_epv_p_t    * /*binding__epv*/,
        rpc_prot_network_epv_p_t    * /*network__epv*/,
        rpc_prot_fork_handler_fn_t  * /*fork_handler*/,
        unsigned32                  * /*st*/
    );


/*
 * Declarations of the RPC Protocol Service init routines.
 */
void rpc__ncacn_init (
        rpc_prot_call_epv_p_t       * /*call_epv*/,
        rpc_prot_mgmt_epv_p_t       * /*mgmt_epv*/,
        rpc_prot_binding_epv_p_t    * /*binding_epv*/,
        rpc_prot_network_epv_p_t    * /*network_epv*/,
        rpc_prot_fork_handler_fn_t  * /*fork_handler*/,
        unsigned32                  * /*st*/
    );

void rpc__ncadg_init (
        rpc_prot_call_epv_p_t       * /*call_epv*/,
        rpc_prot_mgmt_epv_p_t       * /*mgmt_epv*/,
        rpc_prot_binding_epv_p_t    * /*binding_epv*/,
        rpc_prot_network_epv_p_t    * /*network_epv*/,
        rpc_prot_fork_handler_fn_t  * /*fork_handler*/,
        unsigned32                  * /*st*/
    );

void rpc__ncatp_init (
        rpc_prot_call_epv_p_t       * /*call_epv*/,
        rpc_prot_mgmt_epv_p_t       * /*mgmt_epv*/,
        rpc_prot_binding_epv_p_t    * /*binding_epv*/,
        rpc_prot_network_epv_p_t    * /*network_epv*/,
        unsigned32                  * /*st*/
    );

#ifdef __cplusplus
}
#endif

#endif /* _COMPROT_H */
