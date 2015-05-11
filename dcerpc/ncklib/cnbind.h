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
#ifndef _CNBIND_H
#define _CNBIND_H	1
/*
**
**  NAME
**
**      cnbind.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Interface to the NCA Connection Protocol Service's Binding Service.
**
**
*/

/*
 * R P C _ _ C N _ B I N D I N G _ A L L O C
 */

PRIVATE rpc_binding_rep_t *rpc__cn_binding_alloc (
    boolean32            /* is_server */,
    unsigned32          * /* st */);

/*
 * R P C _ _ C N _ B I N D I N G _ I N I T
 */

PRIVATE void rpc__cn_binding_init (
    rpc_binding_rep_p_t  /* binding_r */,
    unsigned32          * /* st */);

/*
 * R P C _ _ C N _ B I N D I N G _ R E S E T
 */

PRIVATE void rpc__cn_binding_reset (
    rpc_binding_rep_p_t  /* binding_r */,
    unsigned32          * /* st */);

/*
 * R P C _ _ C N _ B I N D I N G _ C H A N G E D
 */

PRIVATE void rpc__cn_binding_changed (
    rpc_binding_rep_p_t  /* binding_r */,
    unsigned32          * /* st */);

/*
 * R P C _ _ C N _ B I N D I N G _ F R E E
 */

PRIVATE void rpc__cn_binding_free (
    rpc_binding_rep_p_t * /* binding_r */,
    unsigned32          * /* st */);

/*
 * R P C _ _ C N _ B I N D I N G _ I N Q _ A D D R
 */

PRIVATE void rpc__cn_binding_inq_addr (
    rpc_binding_rep_p_t  /* binding_r */,
    rpc_addr_p_t        * /* rpc_addr */,
    unsigned32          * /* st */);

/*
 * R P C _ _ C N _ B I N D I N G _ I N Q _ C L I E N T
 */

PRIVATE void rpc__cn_binding_inq_client (
    rpc_binding_rep_p_t  /* binding_r */,
    rpc_client_handle_t * /* client_h */,
    unsigned32          * /* st */);

/*
 * R P C _ _ C N _ B I N D I N G _ C O P Y
 */

PRIVATE void rpc__cn_binding_copy (
    rpc_binding_rep_p_t  /* src_binding_r */,
    rpc_binding_rep_p_t  /* dst_binding_r */,
    unsigned32          * /* st */);

/*
 * R P C _ _ C N _ B I N D I N G _ C R O S S _ F O R K
 */

PRIVATE void rpc__cn_binding_cross_fork (
    rpc_binding_rep_p_t  /* binding_r */,
    unsigned32          * /* st */);

#endif /* _CNBIND_H */
