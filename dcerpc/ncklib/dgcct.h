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
**  NAME:
**
**      dgcct.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines
**
**
*/

#ifndef _DGCCT_H
#define _DGCCT_H

#include <dce/dce.h>


/*
 * R P C _ D G _ C C T E _ R E F _ I N I T
 *
 * Initialize a CCTE soft reference.
 */

#define RPC_DG_CCTE_REF_INIT(rp) ( \
    (rp)->ccte = NULL, \
    (rp)->gc_count = 0 \
)

/*
 * R P C _ D G _ C C T _ R E F E R E N C E
 *
 * Increment the reference count for the CCTE. 
 */

#define RPC_DG_CCT_REFERENCE(ccte) { \
    assert((ccte)->refcnt < 255); \
    (ccte)->refcnt++; \
}

/*
 * R P C _ D G _ C C T _ R E L E A S E
 *
 * Release a CCTE and update its last time used (for CCT GC aging).
 * Retain the soft reference so we can reuse it on subsequent calls.
 */

#define RPC_DG_CCT_RELEASE(ccte_ref) { \
    rpc_dg_cct_elt_p_t ccte = (ccte_ref)->ccte; \
    assert(ccte->refcnt > 1); \
    if (--ccte->refcnt <= 1) \
        ccte->timestamp = rpc__clock_stamp(); \
}


#ifdef __cplusplus
extern "C" {
#endif


PRIVATE void rpc__dg_cct_get (
        rpc_auth_info_p_t /*auth_info*/,
        rpc_dg_ccall_p_t /*ccall*/
    );

PRIVATE void rpc__dg_cct_release (
        rpc_dg_ccte_ref_p_t /*ccte_ref*/
    );

PRIVATE void rpc__dg_cct_fork_handler (
        rpc_fork_stage_id_t /*stage*/
    );

#ifdef __cplusplus
}
#endif

#endif /* _DGCCT_H */
