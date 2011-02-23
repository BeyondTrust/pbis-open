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
#ifndef _CNCALL_H
#define _CNCALL_H	1
/*
**
**  NAME
**
**      cncall.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Interface to the NCA Connection Protocol Service's Call Service.
**
**
*/

/*
 * R P C _ _ C N _ C A L L _ S T A R T
 */

PRIVATE rpc_call_rep_t *rpc__cn_call_start (
    rpc_binding_rep_p_t          /* binding_rep */,
    unsigned32                   /* call_options */,
    rpc_if_rep_p_t               /* ifspec_r */,
    unsigned32                   /* opnum */,
    rpc_transfer_syntax_t       * /* xfer_syntax */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ T R A N S M I T
 */

PRIVATE void rpc__cn_call_transmit (
    rpc_call_rep_p_t             /* call_r */,
    rpc_iovector_p_t             /* call_args */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ T R A N S C E I V E
 */

PRIVATE void rpc__cn_call_transceive (
    rpc_call_rep_p_t             /* call_r */,
    rpc_iovector_p_t             /* in_call_args */,
    rpc_iovector_elt_t          * /* out_call_args */,
    ndr_format_t                * /* remote_ndr_format */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ R E C E I V E
 */

PRIVATE void rpc__cn_call_receive (
    rpc_call_rep_p_t             /* call_r */,
    rpc_iovector_elt_t          * /* call_args */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ B L O C K _ U N T I L _ F R E E
 */

PRIVATE void rpc__cn_call_block_until_free (
    rpc_call_rep_p_t             /* call_r */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ A L E R T
 */

PRIVATE void rpc__cn_call_alert (
    rpc_call_rep_p_t             /* call_r */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ E N D
 */

PRIVATE void rpc__cn_call_end (
    rpc_call_rep_p_t            * /* call_r */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ T R A N S M I T _ F A U L T
 */

PRIVATE void rpc__cn_call_transmit_fault (
    rpc_call_rep_p_t             /* call_r */,
    rpc_iovector_p_t             /* call_fault_info */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ R E J E C T
 */

PRIVATE void rpc__cn_call_reject (
    rpc_call_rep_p_t             /* call_r */,
    unsigned32                   /* l_st */);

/*
 * R P C _ _ C N _ C A L L _ R E C E I V E _ F A U L T
 */

PRIVATE void rpc__cn_call_receive_fault (
    rpc_call_rep_p_t             /* call_r */,
    rpc_iovector_elt_t          * /* call_fault_info */,
    ndr_format_t                * /* remote_ndr_format */,
    unsigned32                  * /* st */);

/*
 * R P C _ _ C N _ C A L L _ D I D _ M G R _ E X E C U T E 
 */

PRIVATE boolean32 rpc__cn_call_did_mgr_execute (
    rpc_call_rep_p_t             /* call_r */,
    unsigned32                  * /* st */);

/*
 * R P C _ C N _ C A L L _ N O _ C O N N _ I N D
 */

PRIVATE void rpc__cn_call_no_conn_ind (
    rpc_cn_call_rep_p_t          /* call_r */);

/*
 * R P C _ _ C N _ C A L L _ C C B _ C R E A T E
 */

PRIVATE void rpc__cn_call_ccb_create (
    rpc_cn_call_rep_p_t          /* call_r */);

/*
 * R P C _ _ C N _ C A L L _ C C B _ F R E E
 */

PRIVATE void rpc__cn_call_ccb_free (
    rpc_cn_call_rep_p_t          /* call_r */);

/*
 * R P C _ _ C N _ C A L L _ L O C A L _ C A N C E L
 */

PRIVATE void rpc__cn_call_local_cancel (
    rpc_cn_call_rep_p_t          /* call_rep */,
    volatile boolean32          * /* retry_op */,
    unsigned32                  * /* st */
    );

/*
 * R P C _ _ C N _ C A L L _ S T A R T _ C A N C E L _ T I M E R
 */

PRIVATE void rpc__cn_call_start_cancel_timer (
    rpc_cn_call_rep_p_t      /* call_r */, 
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ C A L L _ S T O P _ C A N C E L _ T I M E R
 */

PRIVATE void rpc__cn_call_stop_cancel_timer (
    rpc_cn_call_rep_p_t     /* call_r */
    );

#endif /* _CNCALL_H */

