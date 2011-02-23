/*
 * 
 * (c) Copyright 1992 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1992 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1992 DIGITAL EQUIPMENT CORPORATION
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
**  NAME:
**
**      pipesupp.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      Type independent routines to support pipes
**
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


/* The ordering of the following 3 includes should NOT be changed! */
#include <dce/rpc.h>
#include <dce/stubbase.h>
#include <lsysdep.h>

#ifdef PERFMON
#include <dce/idl_log.h>
#endif

#ifdef MIA
#include <dce/idlddefs.h>
#endif

void rpc_ss_initialize_callee_pipe
(
    long pipe_index,    /* Index of pipe in set of pipes in the
                            operation's parameter list */
    long next_in_pipe,     /* Index of next [in] pipe to process */
    long next_out_pipe,     /* Index of next [out] pipe to process */
    long *p_current_pipe,    /* Ptr to index num and dirn of curr active pipe */
    rpc_mp_t *p_mp,         /* Ptr to marshalling pointer */
    rpc_op_t *p_op,     /* Ptr to offset pointer */
    ndr_format_t src_drep,   /* Sender's data representation */
    rpc_iovector_elt_t *p_rcvd_data, /* Addr of received data descriptor */
    rpc_ss_mem_handle *p_mem_h,    /* Ptr to stub memory allocation handle */
    rpc_call_handle_t call_h,
    rpc_ss_ee_pipe_state_t **p_p_pipe_state,    /* Addr of ptr to pipe state block */
    error_status_t *st
)
{
    rpc_ss_ee_pipe_state_t *p_pipe_state;

#ifdef PERFMON
    RPC_SS_INITIALIZE_CALLEE_PIPE_N;
#endif

    p_pipe_state = (rpc_ss_ee_pipe_state_t *)rpc_ss_mem_alloc(
                                    p_mem_h, sizeof(rpc_ss_ee_pipe_state_t));
    if (p_pipe_state == NULL)
    {
        DCETHREAD_RAISE(rpc_x_no_memory);
        return;
    }
    p_pipe_state->pipe_drained = ndr_false;
    p_pipe_state->pipe_filled = ndr_false;
    p_pipe_state->pipe_number = pipe_index;
    p_pipe_state->next_in_pipe = next_in_pipe;
    p_pipe_state->next_out_pipe = next_out_pipe;
    p_pipe_state->p_current_pipe = p_current_pipe;
    p_pipe_state->left_in_wire_array = 0;
    p_pipe_state->p_mp = p_mp;
    p_pipe_state->p_op = p_op;
    p_pipe_state->src_drep = src_drep;
    p_pipe_state->p_rcvd_data = p_rcvd_data;
    p_pipe_state->p_mem_h = p_mem_h;
    p_pipe_state->call_h = call_h;
    p_pipe_state->p_st = st;
    *p_p_pipe_state = p_pipe_state;
    *st = error_status_ok;

#ifdef PERFMON
    RPC_SS_INITIALIZE_CALLEE_PIPE_X;
#endif

}

#ifdef MIA

void rpc_ss_mts_init_callee_pipe
(
    long pipe_index,    /* Index of pipe in set of pipes in the
                            operation's parameter list */
    long next_in_pipe,     /* Index of next [in] pipe to process */
    long next_out_pipe,     /* Index of next [out] pipe to process */
    long *p_current_pipe,    /* Ptr to index num and dirn of curr active pipe */
    struct IDL_ms_t *IDL_msp,       /* Pointer to interpreter state block */
    unsigned long IDL_base_type_offset,  /* Offset of pipe base type definition
                                            in type vector */
    rpc_ss_mts_ee_pipe_state_t **p_p_pipe_state
                                           /* Addr of ptr to pipe state block */
)
{
    rpc_ss_mts_ee_pipe_state_t *p_pipe_state;

#ifdef PERFMON
    RPC_SS_INITIALIZE_CALLEE_PIPE_N;
#endif

    p_pipe_state = (rpc_ss_mts_ee_pipe_state_t *)
                                rpc_ss_mem_alloc(&IDL_msp->IDL_mem_handle,
                                sizeof(rpc_ss_mts_ee_pipe_state_t));
    if (p_pipe_state == NULL)
    {
        DCETHREAD_RAISE(rpc_x_no_memory);
        return;
    }
    p_pipe_state->pipe_drained = ndr_false;
    p_pipe_state->pipe_filled = ndr_false;
    p_pipe_state->pipe_number = pipe_index;
    p_pipe_state->next_in_pipe = next_in_pipe;
    p_pipe_state->next_out_pipe = next_out_pipe;
    p_pipe_state->p_current_pipe = p_current_pipe;
    p_pipe_state->left_in_wire_array = 0;
    p_pipe_state->IDL_msp = IDL_msp;
    p_pipe_state->IDL_base_type_offset = IDL_base_type_offset;
    *p_p_pipe_state = p_pipe_state;

#ifdef PERFMON
    RPC_SS_INITIALIZE_CALLEE_PIPE_X;
#endif

}
#endif
