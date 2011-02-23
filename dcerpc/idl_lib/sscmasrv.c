/*
 * 
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
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
**      sscmasrv.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      CMA machinery used by IDL server stub
**
**  VERSION: DCE 1.0
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


/* The ordering of the following 3 includes should NOT be changed! */
#include <dce/rpc.h>
#include <dce/stubbase.h>
#include <lsysdep.h>

#ifdef MIA
#include <dce/idlddefs.h>
#endif

#ifdef PERFMON
#include <dce/idl_log.h>
#endif
/******************************************************************************/
/*                                                                            */
/*    Set up CMA machinery required by server and client                      */
/*                                                                            */
/******************************************************************************/

#ifndef VMS
    ndr_boolean rpc_ss_server_is_set_up = ndr_false;
#endif

void rpc_ss_init_server_once(
    void
)
{

#ifdef PERFMON
    RPC_SS_INIT_SERVER_ONCE_N;
#endif

    RPC_SS_THREADS_INIT;
    rpc_ss_init_client_once();
    rpc_ss_init_allocate_once();
#ifndef VMS
    rpc_ss_server_is_set_up = ndr_true;
#endif

#ifdef PERFMON
    RPC_SS_INIT_SERVER_ONCE_X;
#endif

}


/******************************************************************************/
/*                                                                            */
/*   Map an exception into a fault code and send a fault packet               */
/*  Old version - no user exceptions                                          */
/*                                                                            */
/******************************************************************************/
void rpc_ss_send_server_exception
(
    rpc_call_handle_t h,
    dcethread_exc *e
)
{
    ndr_ulong_int mapped_code;
    ndr_ulong_int fault_buff;
    rpc_mp_t mp;
    rpc_iovector_t iovec;
    error_status_t st;

#ifdef PERFMON
    RPC_SS_SEND_SERVER_EXCEPTION_N;
#endif

    iovec.num_elt = 1;
    iovec.elt[0].buff_dealloc = NULL;
    iovec.elt[0].flags = rpc_c_iovector_elt_reused;
    iovec.elt[0].buff_addr = (byte_p_t)&fault_buff;
    iovec.elt[0].buff_len = 4;
    iovec.elt[0].data_addr = (byte_p_t)&fault_buff;
    iovec.elt[0].data_len = 4;

    if ( RPC_SS_EXC_MATCHES( e, &rpc_x_invalid_tag ) )
        mapped_code = nca_s_fault_invalid_tag;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_invalid_bound ) )
        mapped_code = nca_s_fault_invalid_bound;
    else if ( RPC_SS_EXC_MATCHES( e, &RPC_SS_THREADS_X_CANCELLED ) )
        mapped_code = nca_s_fault_cancel;
    else if ( RPC_SS_EXC_MATCHES( e, &dcethread_fltdiv_e ) )
        mapped_code = nca_s_fault_fp_div_zero;
    else if ( RPC_SS_EXC_MATCHES( e, &dcethread_fltovf_e ) )
        mapped_code = nca_s_fault_fp_overflow;
    else if ( RPC_SS_EXC_MATCHES( e, &dcethread_aritherr_e ) )
        mapped_code = nca_s_fault_fp_error;
    else if ( RPC_SS_EXC_MATCHES( e, &dcethread_fltund_e ) )
        mapped_code = nca_s_fault_fp_underflow;
    else if ( RPC_SS_EXC_MATCHES( e, &dcethread_illaddr_e ) )
        mapped_code = nca_s_fault_addr_error;
    else if ( RPC_SS_EXC_MATCHES( e, &dcethread_illinstr_e ) )
        mapped_code = nca_s_fault_ill_inst;
    else if ( RPC_SS_EXC_MATCHES( e, &dcethread_intdiv_e ) )
        mapped_code = nca_s_fault_int_div_by_zero;
    else if ( RPC_SS_EXC_MATCHES( e, &dcethread_intovf_e ) )
        mapped_code = nca_s_fault_int_overflow;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_no_memory ) )
        mapped_code = nca_s_fault_remote_no_memory;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_ss_context_mismatch ) )
        mapped_code = nca_s_fault_context_mismatch;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_ss_pipe_empty ) )
        mapped_code = nca_s_fault_pipe_empty;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_ss_pipe_closed ) )
        mapped_code = nca_s_fault_pipe_closed;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_ss_pipe_order ) )
        mapped_code = nca_s_fault_pipe_order;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_ss_pipe_discipline_error ) )
        mapped_code = nca_s_fault_pipe_discipline;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_ss_pipe_comm_error ) )
        mapped_code = nca_s_fault_pipe_comm_error;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_ss_pipe_memory ) )
        mapped_code = nca_s_fault_pipe_memory;
    else if ( RPC_SS_EXC_MATCHES( e, &rpc_x_ss_remote_comm_failure ) )
        mapped_code = nca_s_fault_remote_comm_failure;
    else
        mapped_code = nca_s_fault_unspec;

    rpc_init_mp(mp, &fault_buff);
    rpc_marshall_ulong_int(mp, mapped_code);
    rpc_call_transmit_fault( h, &iovec, &st );

#ifdef PERFMON
    RPC_SS_SEND_SERVER_EXCEPTION_X;
#endif

}

/******************************************************************************/
/*                                                                            */
/*   Map an exception into a fault code and send a fault packet               */
/*  New version - user exceptions                                             */
/*                                                                            */
/******************************************************************************/
void rpc_ss_send_server_exception_2
(
    rpc_call_handle_t h,
    dcethread_exc *e,
    idl_long_int num_user_exceptions,
    dcethread_exc *user_exception_pointers[],
    IDL_msp_t IDL_msp ATTRIBUTE_UNUSED
)
{
    ndr_ulong_int mapped_code;
    ndr_ulong_int fault_buff[2];
    rpc_iovector_t iovec;
    error_status_t st;
    ndr_ulong_int i;
    rpc_mp_t mp;

    for (i=0; i< (unsigned32)num_user_exceptions; i++)
    {
        if (RPC_SS_EXC_MATCHES(e, user_exception_pointers[i]))
        {
            mapped_code = nca_s_fault_user_defined;
            rpc_init_mp(mp, fault_buff);
            rpc_marshall_ulong_int(mp, mapped_code);
            rpc_advance_mp(mp, 4);
            rpc_marshall_ulong_int(mp, i);
            iovec.num_elt = 1;
            iovec.elt[0].buff_dealloc = NULL;
            iovec.elt[0].flags = rpc_c_iovector_elt_reused;
            iovec.elt[0].buff_addr = (byte_p_t)fault_buff;
            iovec.elt[0].buff_len = 8;
            iovec.elt[0].data_addr = (byte_p_t)fault_buff;
            iovec.elt[0].data_len = 8;
            rpc_call_transmit_fault( h, &iovec, &st );
            return;
        }
    }

    /* Exception did not match any user defined exception.
        Call the old (system exception) code */
    rpc_ss_send_server_exception( h, e );
}
