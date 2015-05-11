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
**      dgscall.h
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

#ifndef _DGSCALL_H
#define _DGSCALL_H

#include <dce/dce.h>


/*
 * R P C _ D G _ S C A L L _ R E L E A S E
 *
 * Decrement the reference count for the SCALL and
 * NULL the reference.
 */

#define RPC_DG_SCALL_RELEASE(scall) { \
    RPC_DG_CALL_LOCK_ASSERT(&(*(scall))->c); \
    assert((*(scall))->c.refcnt > 0); \
    if (--(*(scall))->c.refcnt == 0) \
        rpc__dg_scall_free(*(scall)); \
    else \
        RPC_DG_CALL_UNLOCK(&(*(scall))->c); \
    *(scall) = NULL; \
}

/*
 * R P C _ D G _ S C A L L _ R E L E A S E _ N O _ U N L O C K
 *
 * Like RPC_DG_SCALL_RELEASE, except doesn't unlock the SCALL.  Note
 * that the referencing counting model requires that this macro can be
 * used iff the release will not be the "last one" (i.e., the one that
 * would normally cause the SCALL to be freed).
 */

#define RPC_DG_SCALL_RELEASE_NO_UNLOCK(scall) { \
    RPC_DG_CALL_LOCK_ASSERT(&(*(scall))->c); \
    assert((*(scall))->c.refcnt > 1); \
    --(*(scall))->c.refcnt; \
    *(scall) = NULL; \
}


PRIVATE void rpc__dg_scall_free (rpc_dg_scall_p_t  /*scall*/);


PRIVATE void rpc__dg_scall_reinit (
        rpc_dg_scall_p_t  /*scall*/,
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

PRIVATE rpc_dg_scall_p_t rpc__dg_scall_alloc (
        rpc_dg_sct_elt_p_t  /*scte*/,
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

PRIVATE rpc_dg_scall_p_t rpc__dg_scall_cbk_alloc (
        rpc_dg_ccall_p_t  /*ccall*/,
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

PRIVATE void rpc__dg_scall_orphan_call (
	rpc_dg_scall_p_t  /*scall*/
    );

#endif /* _DGSCALL_H */
