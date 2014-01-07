/*
 * 
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
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
**
**  NAME:
**
**      nidlalfr.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      rpc_ss_allocate, rpc_ss_free and helper thread routines
**
**  VERSION: DCE 1.0
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <dce/rpc.h>
#include <dce/stubbase.h>
#include <lsysdep.h>

#ifdef PERFMON
#include <dce/idl_log.h>
#endif

/******************************************************************************/
/*                                                                            */
/*    rpc_ss_allocate                                                         */
/*                                                                            */
/******************************************************************************/
idl_void_p_t rpc_ss_allocate
(
    idl_size_t size
)
{
    rpc_ss_thread_support_ptrs_t *p_support_ptrs = NULL;
    rpc_void_p_t                 p_new_node = NULL;
    error_status_t               status = rpc_s_ok;

#ifdef PERFMON
    RPC_SS_ALLOCATE_N;
#endif

    rpc_ss_get_support_ptrs( &p_support_ptrs );
    RPC_SS_THREADS_MUTEX_LOCK(&(p_support_ptrs->mutex));
    p_new_node = (rpc_void_p_t)rpc_sm_mem_alloc( p_support_ptrs->p_mem_h, size, &status );
    RPC_SS_THREADS_MUTEX_UNLOCK(&(p_support_ptrs->mutex));

    if (status == rpc_s_no_memory) DCETHREAD_RAISE( rpc_x_no_memory );

#ifdef PERFMON
    RPC_SS_ALLOCATE_X;
#endif

    return(p_new_node);

}

/******************************************************************************/
/*                                                                            */
/*    rpc_ss_free                                                             */
/*                                                                            */
/******************************************************************************/
void rpc_ss_free
(
    idl_void_p_t node_to_free
)
{
    rpc_ss_thread_support_ptrs_t *p_support_ptrs = NULL;

#ifdef PERFMON
    RPC_SS_FREE_N;
#endif

    rpc_ss_get_support_ptrs( &p_support_ptrs );
    RPC_SS_THREADS_MUTEX_LOCK(&(p_support_ptrs->mutex));
    if (p_support_ptrs->p_mem_h->node_table)
        /*
         * Must unregister node or a subsequent alloc could get same addr and
         * nodetbl mgmt would think it was an alias to storage's former life.
         */
        rpc_ss_unregister_node(p_support_ptrs->p_mem_h->node_table,
                               (byte_p_t)node_to_free);
    rpc_ss_mem_release(p_support_ptrs->p_mem_h, (byte_p_t)node_to_free, ndr_true);
    RPC_SS_THREADS_MUTEX_UNLOCK(&(p_support_ptrs->mutex));

#ifdef PERFMON
    RPC_SS_FREE_X;
#endif

}
