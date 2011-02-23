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
**      dgccallt.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  Handle client call table (CCALLT).
**
**
*/

#include <dg.h>
#include <dgcall.h>
#include <dgccall.h>
#include <dgccallt.h>


/*
 * R P C _ _ D G _ C C A L L T _ I N S E R T
 *
 * Add a client call handle to the CCALLT.  Increment the call handle's reference
 * count.
 */

PRIVATE void rpc__dg_ccallt_insert
(
    rpc_dg_ccall_p_t ccall
)
{
    unsigned16 probe = ccall->c.actid_hash % RPC_DG_CCALLT_SIZE;

    RPC_LOCK_ASSERT(0);

    ccall->c.next = (rpc_dg_call_p_t) rpc_g_dg_ccallt[probe];
    rpc_g_dg_ccallt[probe] = ccall;
    RPC_DG_CALL_REFERENCE(&ccall->c);
}


/*
 * R P C _ _ D G _ C C A L L T _ R E M O V E
 *
 * Remove a client call handle from the CCALLT.  Decrement the call
 * handle's reference count.  This routine can be called only if there
 * are references to the CCALL other than the one from the CCALLT.
 */

PRIVATE void rpc__dg_ccallt_remove
(
    rpc_dg_ccall_p_t ccall
)
{
    unsigned16 probe = ccall->c.actid_hash % RPC_DG_CCALLT_SIZE;
    rpc_dg_ccall_p_t scan_ccall, prev_scan_ccall;

    RPC_LOCK_ASSERT(0);
    RPC_DG_CALL_LOCK_ASSERT(&ccall->c);
    assert(ccall->c.refcnt > 1);

    /*
     * Scan down the hash chain.
     */

    scan_ccall = rpc_g_dg_ccallt[probe];
    prev_scan_ccall = NULL;

    while (scan_ccall != NULL) {
        if (scan_ccall == ccall) {
            if (prev_scan_ccall == NULL)
                rpc_g_dg_ccallt[probe] = (rpc_dg_ccall_p_t) scan_ccall->c.next;
            else
                prev_scan_ccall->c.next = scan_ccall->c.next;
            RPC_DG_CCALL_RELEASE_NO_UNLOCK(&scan_ccall);
            return;
        }
        else {
            prev_scan_ccall = scan_ccall;
            scan_ccall = (rpc_dg_ccall_p_t) scan_ccall->c.next;
        }
    }

    assert(false);          /* Shouldn't ever fail to find the ccall */
}


/*
 * R P C _ _ D G _ C C A L L T _ L O O K U P
 *
 * Find a client call handle in the CCALLT based on an activity ID and
 * probe hint (ahint).  Return the ccall or NULL if no matching entry
 * is found.  Note that a probe_hint of RPC_C_DG_NO_HINT can be used if
 * the caller doesn't have a hint.  If we're returning a ccall, it will
 * be locked and have it's reference count incremented already.
 */

PRIVATE rpc_dg_ccall_p_t rpc__dg_ccallt_lookup
(
    dce_uuid_p_t actid,
    unsigned32 probe_hint
)
{
    rpc_dg_ccall_p_t ccall;
    unsigned16 probe;
    unsigned32 st;
    boolean once = false;

    RPC_LOCK_ASSERT(0);

    /*
     * Determine the hash chain to use
     */

    if (probe_hint == RPC_C_DG_NO_HINT || probe_hint >= RPC_DG_CCALLT_SIZE)
        probe = rpc__dg_uuid_hash(actid) % RPC_DG_CCALLT_SIZE;
    else
        probe = probe_hint;

    /*
     * Scan down the probe chain, reserve and return a matching SCTE.
     */

RETRY:
    ccall = rpc_g_dg_ccallt[probe];

    while (ccall != NULL) 
    {
        if (UUID_EQ(*actid, ccall->c.call_actid, &st)) 
        {
            RPC_DG_CALL_LOCK(&ccall->c);
            RPC_DG_CALL_REFERENCE(&ccall->c);
            return(ccall);
        }
        else
            ccall = (rpc_dg_ccall_p_t) ccall->c.next;
    }

    /*
     * No matching entry found.  If we used the provided hint, try
     * recomputing the probe and if this yields a new probe, give it
     * one more try.
     */

    if (probe == probe_hint && !once) 
    {
        once = true;
        probe = rpc__dg_uuid_hash(actid) % RPC_DG_CCALLT_SIZE;
        if (probe != probe_hint)
            goto RETRY;
    }

    return(NULL);
}

/*
 * R P C _ _ D G _ C C A L L T _ F O R K _ H A N D L E R 
 *
 * Handle fork related processing for this module.
 */
PRIVATE void rpc__dg_ccallt_fork_handler
(
    rpc_fork_stage_id_t stage
)
{                           
    unsigned32 i;

    switch ((int)stage)
    {
        case RPC_C_PREFORK:
            break;
        case RPC_C_POSTFORK_PARENT:
            break;
        case RPC_C_POSTFORK_CHILD:  
            /*
             * Clear out the Client Call Handle Table
             */                                
        
            for (i = 0; i < RPC_DG_CCALLT_SIZE; i++)
                rpc_g_dg_ccallt[i] = NULL;
            break;
    }
}

