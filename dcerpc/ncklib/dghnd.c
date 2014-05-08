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
**      dghnd.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Routines for manipulating Datagram Specific RPC bindings.
**
**
*/

#include <dg.h>
#include <dghnd.h>
#include <dgcall.h>
#include <dgccall.h>

/* ======================================================================== */

/*
 * R E L E A S E _ C A C H E D _ C C A L L
 *
 * Release any client call binding we might have in the binding binding.
 */

INTERNAL void release_cached_ccall (
        rpc_dg_binding_client_p_t /*h*/
    );

INTERNAL void release_cached_ccall
(
    rpc_dg_binding_client_p_t h                       
)
{
    if (h->ccall == NULL) 
        return;

    RPC_DG_CALL_LOCK(&h->ccall->c);
    rpc__dg_ccall_free_prep(h->ccall);
    RPC_DG_CCALL_RELEASE(&h->ccall);
    /* unlocks as a side effect */
}


/*
 * R P C _ _ D G _ B I N D I N G _ A L L O C
 *
 * Allocate a DG-specific binding rep.
 */

PRIVATE rpc_binding_rep_p_t rpc__dg_binding_alloc
(
    boolean32 is_server,
    unsigned32 *st
)
{
    rpc_binding_rep_p_t h;

    *st = rpc_s_ok;

    if (is_server)
    {
        RPC_MEM_ALLOC(h, rpc_binding_rep_p_t, sizeof(rpc_dg_binding_server_t), 
                        RPC_C_MEM_DG_SHAND, RPC_C_MEM_NOWAIT);
    }
    else
    {
        RPC_MEM_ALLOC(h, rpc_binding_rep_p_t, sizeof(rpc_dg_binding_client_t), 
                        RPC_C_MEM_DG_CHAND, RPC_C_MEM_NOWAIT);
    }

    return(h);
}


/*
 * R P C _ _ D G _ B I N D I N G _ I N I T
 *
 * Initialize the DG-specific part of a client binding binding.
 */

PRIVATE void rpc__dg_binding_init
(
    rpc_binding_rep_p_t h_,
    unsigned32 *st
)
{
    *st = rpc_s_ok;

    if (RPC_BINDING_IS_SERVER(h_))
    {
        rpc_dg_binding_server_p_t h = (rpc_dg_binding_server_p_t) h_;

        h->chand        = NULL;
        h->scall        = NULL;
    }
    else
    {
        rpc_dg_binding_client_p_t h = (rpc_dg_binding_client_p_t) h_;

        h->ccall          = NULL;
        h->server_boot    = 0;
        h->shand          = NULL; 
        h->maint_binding  = NULL;  
        h->is_WAY_binding = 0;
    }
}


/*
 * R P C _ _ D G _ B I N D I N G _ F R E E
 *
 * Free a DG-specific, client-side binding binding.
 */

PRIVATE void rpc__dg_binding_free
(
    rpc_binding_rep_p_t *h_,
    unsigned32 *st
)
{
    /*
     * The binding and the call binding are public at this point (remember
     * the call monitor) hence we need to ensure that things remain
     * orderly.
     */
    RPC_LOCK_ASSERT(0);

    if ((*h_)->is_server == 0)
    {
        release_cached_ccall((rpc_dg_binding_client_p_t) *h_);
        RPC_MEM_FREE(*h_, RPC_C_MEM_DG_CHAND);
    }
    else
    {
        RPC_MEM_FREE(*h_, RPC_C_MEM_DG_SHAND);
    }

    *h_ = NULL;

    *st = rpc_s_ok;
}


/*
 * R P C _ _ D G _ B I N D I N G _ C H A N G E D
 *
 * 
 */

PRIVATE void rpc__dg_binding_changed
(
    rpc_binding_rep_p_t h,
    unsigned32 *st
)
{
    RPC_LOCK(0);    /* !!! this should really be done in common code */

    release_cached_ccall((rpc_dg_binding_client_p_t) h);    /* !!! Crude but effective.  Sufficient? */
    *st = rpc_s_ok;

    RPC_UNLOCK(0);
}


/*
 * R P C _ _ D G _ B I N D I N G _ I N Q _ A D D R
 *
 * 
 */

PRIVATE void rpc__dg_binding_inq_addr
(
    rpc_binding_rep_p_t h ATTRIBUTE_UNUSED,
    rpc_addr_p_t *addr ATTRIBUTE_UNUSED,
    unsigned32 *st ATTRIBUTE_UNUSED
)
{
    /*
     * rpc_m_unimp_call
     * "(%s) Call not implemented"
     */
    RPC_DCE_SVC_PRINTF ((
	DCE_SVC(RPC__SVC_HANDLE, "%s"),
	rpc_svc_general,
	svc_c_sev_fatal | svc_c_action_abort,
	rpc_m_unimp_call,
        "rpc__dg_binding_inq_addr" ));
}


/*
 * R P C _ _ D G _ B I N D I N G _ R E S E T
 *
 * Dissociate the binding from a bound server instance.
 */

PRIVATE void rpc__dg_binding_reset
(
    rpc_binding_rep_p_t h_,
    unsigned32 *st
)
{
    rpc_dg_binding_client_p_t h = (rpc_dg_binding_client_p_t) h_;

    RPC_LOCK(0);    /* !!! this should really be done in common code */

    h->server_boot   = 0;
    release_cached_ccall(h);    /* Crude but effective */
    *st = rpc_s_ok;

    RPC_UNLOCK(0);
}


/*
 * R P C _ _ D G _ B I N D I N G _ C O P Y
 *
 * Copy DG private part of a binding.
 */

PRIVATE void rpc__dg_binding_copy
(
    rpc_binding_rep_p_t src_h_,
    rpc_binding_rep_p_t dst_h_,
    unsigned32 *st
)
{
    rpc_dg_binding_client_p_t src_h = (rpc_dg_binding_client_p_t) src_h_;
    rpc_dg_binding_client_p_t dst_h = (rpc_dg_binding_client_p_t) dst_h_;

    RPC_LOCK(0);    /* !!! this should really be done in common code */

    dst_h->server_boot = src_h->server_boot;
    dst_h->host_bound  = src_h->host_bound;

    *st = rpc_s_ok;

    RPC_UNLOCK(0);
}


/*
 * R P C _ _ D G _ B I N D I N G _ S E R V E R _ T O _ C L I E N T
 *
 * Create a client binding binding suitable for doing callbacks from a
 * server to a client.  The server binding binding of the current call
 * is used as a template for the new client binding.
 */ 

PRIVATE rpc_dg_binding_client_p_t rpc__dg_binding_srvr_to_client
(
    rpc_dg_binding_server_p_t shand,                                               
    unsigned32 *st
)
{                        
    rpc_dg_binding_client_p_t chand;
    rpc_addr_p_t   client_addr;

    *st = rpc_s_ok;    

    /*
     * If we've already made a callback on this binding binding,
     * then use the client binding binding that was previously
     * created.
     */

    if (shand->chand != NULL)
        return(shand->chand);

    /*
     * Otherwise, allocate a client binding binding and bind it to the
     * address associated with the server binding binding.
     */

    rpc__naf_addr_copy(shand->c.c.rpc_addr, &client_addr, st);                   
    chand = (rpc_dg_binding_client_p_t) 
        rpc__binding_alloc(false, &UUID_NIL, RPC_C_PROTOCOL_ID_NCADG,
                                   client_addr, st);
    if (*st != rpc_s_ok)
        return(NULL);
   
    /*
     * Link the binding bindings.  
     */

    chand->shand = shand; 
    shand->chand = chand;

    return(chand);
}

/*
 * R P C _ _ D G _ B I N D I N G _ C R O S S _ F O R K
 *
 * This routine makes it possible for children of forks to use
 * binding handles inherited from their parents.  It does this
 * by vaporizing any state associated with the handle, so that
 * the child is forced to rebuild its own state from scratch.
 */

PRIVATE void rpc__dg_binding_cross_fork
(
    rpc_binding_rep_p_t h_,
    unsigned32 *st
)
{   
    rpc_dg_binding_client_p_t h = (rpc_dg_binding_client_p_t) h_;

    *st = rpc_s_ok;
                            
    /*
     * Drop any state associated with this handle.  This state
     * survives in the parent of the fork, and will be processed
     * correctly there.
     */
    h->ccall = NULL;
    h->shand = NULL;
    h->maint_binding = NULL;
}

#if 0

/*
**++
**
**  ROUTINE NAME:       rpc__cn_binding_cross_fork
**
**  SCOPE:              PRIVATE - declared in cnbind.h
**
**  DESCRIPTION:
**
**      This routine makes it possible for children of forks to use
**      binding handles inherited from their parents.
**
**  INPUTS:
**
**      binding_r       The binding rep to be inherited
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      rpc_s_coding_error
**      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_binding_cross_fork 
(
  rpc_binding_rep_p_t     binding_r ATTRIBUTE_UNUSED,
  unsigned32              *st
)
{
    CODING_ERROR (st);

    /*
     * This is a dummy function to avoid the null reference
     * causing a core dump.
     */

    *st = rpc_s_ok;
}
#endif
