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
**      dgccall.h
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

#ifndef _DGCCALL_H
#define _DGCCALL_H	1

#include <dce/dce.h>

#include <dgccallt.h>

/*
 * R P C _ D G _ C C A L L _ S E T _ S T A T E _ I D L E
 *
 * Remove the call handle from the CCALLT.  Release our reference to
 * our CCTE.  (In the case of CCALLs created to do server callbacks there
 * won't be a ccte_ref.)  Change to the idle state.  If you're trying
 * to get rid of the ccall use rpc__dg_ccall_free_prep() instead.
 */

#define RPC_DG_CCALL_SET_STATE_IDLE(ccall) { \
    if ((ccall)->c.state == rpc_e_dg_cs_final) \
        rpc__dg_ccall_ack(ccall); \
    rpc__dg_ccallt_remove(ccall); \
    if (! (ccall)->c.is_cbk)\
        RPC_DG_CCT_RELEASE(&(ccall)->ccte_ref); \
    RPC_DG_CALL_SET_STATE(&(ccall)->c, rpc_e_dg_cs_idle); \
}

/*
 * R P C _ D G _ C C A L L _ R E L E A S E
 *
 * Decrement the reference count for the CCALL and
 * NULL the reference.
 */

#define RPC_DG_CCALL_RELEASE(ccall) { \
    RPC_DG_CALL_LOCK_ASSERT(&(*(ccall))->c); \
    assert((*(ccall))->c.refcnt > 0); \
    if (--(*(ccall))->c.refcnt == 0) \
        rpc__dg_ccall_free(*(ccall)); \
    else \
        RPC_DG_CALL_UNLOCK(&(*(ccall))->c); \
    *(ccall) = NULL; \
}

/*
 * R P C _ D G _ C C A L L _ R E L E A S E _ N O _ U N L O C K
 *
 * Like RPC_DG_CCALL_RELEASE, except doesn't unlock the CCALL.  Note
 * that the referencing counting model requires that this macro can be
 * used iff the release will not be the "last one" (i.e., the one that
 * would normally cause the CCALL to be freed).
 */

#define RPC_DG_CCALL_RELEASE_NO_UNLOCK(ccall) { \
    RPC_DG_CALL_LOCK_ASSERT(&(*(ccall))->c); \
    assert((*(ccall))->c.refcnt > 1); \
    --(*(ccall))->c.refcnt; \
    *(ccall) = NULL; \
}


#ifdef __cplusplus
extern "C" {
#endif


PRIVATE void rpc__dg_ccall_lsct_inq_scall (
        rpc_dg_ccall_p_t  /*ccall*/,
        rpc_dg_scall_p_t * /*scallp*/
    );

PRIVATE void rpc__dg_ccall_lsct_new_call (
        rpc_dg_ccall_p_t  /*ccall*/,
        rpc_dg_sock_pool_elt_p_t  /*si*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_scall_p_t * /*scallp*/
    );

PRIVATE void rpc__dg_ccall_ack (
        rpc_dg_ccall_p_t /*ccall*/
    );

PRIVATE void rpc__dg_ccall_free (
        rpc_dg_ccall_p_t /*ccall*/
    );

PRIVATE void rpc__dg_ccall_free_prep (
        rpc_dg_ccall_p_t /*ccall*/
    );

PRIVATE void rpc__dg_ccall_timer ( pointer_t /*p*/ );

PRIVATE void rpc__dg_ccall_xmit_cancel_quit (
        rpc_dg_ccall_p_t  /*ccall*/,
        unsigned32 /*cancel_id*/
    );

PRIVATE void rpc__dg_ccall_setup_cancel_tmo (
        rpc_dg_ccall_p_t /*ccall*/
    );

#ifdef __cplusplus
}
#endif


#endif /* _DGCCALL_H */
