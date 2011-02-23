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
#ifndef _DGSCT_H
#define _DGSCT_H
/*
**
**  NAME:
**
**      dgsct.h
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

#include <dce/dce.h>

/*
 * R P C _ D G _ S C T _ I S _ W A Y _ V A L I D A T E D
 *
 * Return true only if the connection has a WAY validated seq and the
 * client doesn't require us to WAY it just to get it the server's boot
 * time.
 *
 * It's ok to look at the flag without the lock;  once it's set, it
 * never becomes unset - if the test fails we'll end up doing extra
 * work when we may not have needed to.
 */

#define RPC_DG_SCT_IS_WAY_VALIDATED(scte) \
( \
    (scte)->high_seq_is_way_validated && \
    ! (scte)->scall->client_needs_sboot \
)

/*
 * R P C _ D G _ S C T _ R E F E R E N C E
 *
 * Increment the reference count for the SCTE.
 */

#define RPC_DG_SCT_REFERENCE(scte) { \
    assert((scte)->refcnt < 255); \
    (scte)->refcnt++; \
}

/*
 * R P C _ D G _ S C T _ R E L E A S E
 *
 * Release a currently inuse SCTE.
 *
 * If the reference count goes to one, the SCTE is now available for
 * reuse / or GCing.  Update the SCTE's last used timestamp.
 */

#define RPC_DG_SCT_RELEASE(scte) { \
    RPC_LOCK_ASSERT(0); \
    assert((*(scte))->refcnt > 0); \
    if (--(*scte)->refcnt <= 1) \
        (*(scte))->timestamp = rpc__clock_stamp(); \
    *(scte) = NULL; \
}


PRIVATE void rpc__dg_sct_inq_scall (
        rpc_dg_sct_elt_p_t  /*scte*/,
        rpc_dg_scall_p_t * /*scallp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

PRIVATE void rpc__dg_sct_new_call (
        rpc_dg_sct_elt_p_t  /*scte*/,
        rpc_dg_sock_pool_elt_p_t  /*si*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_scall_p_t * /*scallp*/
    );

PRIVATE void rpc__dg_sct_backout_new_call (
        rpc_dg_sct_elt_p_t  /*scte*/,
        unsigned32  /*seq*/
    );

PRIVATE rpc_dg_sct_elt_p_t rpc__dg_sct_lookup (
        dce_uuid_p_t  /*actid*/,
        unsigned32  /*probe_hint*/
    );

PRIVATE rpc_dg_sct_elt_p_t rpc__dg_sct_get (
        dce_uuid_p_t  /*actid*/,
        unsigned32  /*probe_hint*/,
        unsigned32  /*seq*/
    );

PRIVATE rpc_binding_handle_t rpc__dg_sct_make_way_binding (
        rpc_dg_sct_elt_p_t  /*scte*/,
        unsigned32 * /*st*/
    );

PRIVATE void rpc__dg_sct_way_validate (
        rpc_dg_sct_elt_p_t  /*scte*/,
        unsigned32       /*force_way_auth*/,
        unsigned32      * /*st*/
    );
      
PRIVATE void rpc__dg_sct_fork_handler (
        rpc_fork_stage_id_t  /*stage*/
    );

#endif /* _DGSCT_H */
