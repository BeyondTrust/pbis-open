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
**  NAME
**
**      comcall.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definition of the Call Services for the Common Communication
**  Services component. These routines are called by the stub to
**  perform the actual communications for an RPC. A call handle
**  is created, and is subsequently used to dispatch to the appropriate
**  communications protocol service.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime  */
#include <com.h>        /* Common communications services           */
#include <comprot.h>    /* Common protocol services                 */
#include <comnaf.h>     /* Common network address family services   */
#include <comp.h>       /* Private communications services          */

/*
**++
**
**  ROUTINE NAME:       rpc_call_start
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Begin a Remote Procedure Call. This is the first in a sequence of calls
**  by the client stub. It returns the information needed to marshal input
**  arguments. This routine is intended for use by the client stub only.
**
**  INPUTS:
**
**      binding_h       The binding handle which identifies the protocol
**                      stack to which the RPC is being made.
**
**      flags           The options to be applied to this RPC. One of these
**                      options must be set. They may not be ORed together.
**
**                      rpc_c_call_non_idempotent
**                      rpc_c_call_idempotent
**                      rpc_c_call_maybe
**                      rpc_c_call_brdcst
**
**      ifspec_h        The interface specification handle containing the
**                      interface UUID and version in which the RPC is
**                      contained. Also contains client stub supported
**                      transfer syntaxes.
**
**      opnum           The operation number in the interface the RPC
**                      corresponds to.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      xfer_syntax     The negotiated transfer syntax. The stub will use
**                      this to marshal the input arguments and unmarshal
**                      the output arguments of the RPC.
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**          rpc_s_coding_error
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      call_h          The handle which uniquely identifies this RPC.
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_start 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              flags,
    rpc_if_handle_t         ifspec_h,
    unsigned32              opnum,
    rpc_call_handle_t       *call_handle,
    rpc_transfer_syntax_t   *xfer_syntax,
    unsigned32              *status
)
{
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;
    rpc_call_rep_p_t        call_rep;

    RPC_LOG_CALL_START_NTR;
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();


    RPC_BINDING_VALIDATE(binding_rep, status);
    if (*status != rpc_s_ok)
    {
        *call_handle = NULL;
        return;              
    }
    RPC_IF_VALIDATE((rpc_if_rep_p_t) ifspec_h, status);
    if (*status != rpc_s_ok)
    {
        *call_handle = NULL;
        return;
    }

    /*
     * dispatch to the appropriate protocol service to get a call handle
     */
    call_rep = (*rpc_g_protocol_id[binding_rep->protocol_id].call_epv
        ->call_start)
            (binding_rep, flags, (rpc_if_rep_p_t) ifspec_h,
            opnum, xfer_syntax, status);

    if (*status == rpc_s_ok)
    {
        *call_handle = (rpc_call_handle_t) call_rep;

        /*
         * initialize the common part of the call handle
         */
        call_rep->protocol_id = binding_rep->protocol_id;
    }

    RPC_LOG_CALL_START_XIT;
}

/*
**++
**
**  ROUTINE NAME:       rpc_call_transmit
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Transmit a vector of marshaled arguments to the remote thread. Use the
**  call handle as the identifier of the RPC being performed. This routine
**  is intended for use by the client or server stub only.
**
**  INPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**      call_args       The marshaled RPC arguments being transmitted to the
**                      remote thread.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_coding_error
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_transmit 
(
    rpc_call_handle_t       call_h,
    rpc_iovector_p_t        call_args,
    unsigned32              *status
)
{ 
    RPC_LOG_CALL_TRANSMIT_NTR;
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    (*rpc_g_protocol_id[((rpc_call_rep_p_t) (call_h))->protocol_id].call_epv
        ->call_transmit)
            ((rpc_call_rep_p_t) call_h, call_args, status);

    RPC_LOG_CALL_TRANSMIT_XIT;
}

/*
**++
**
**  ROUTINE NAME:       rpc_call_transceive
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Transmit a vector of marshaled arguments to the remote thread. Use the
**  call handle as the identifier of the RPC being performed. Block until
**  the first buffer of marshaled output arguments has been received. This
**  routine is intended for use by the client or server stub only.
**
**  INPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**      in_call_args    The marshaled RPC input arguments being transmitted
**                      to the remote thread.
**
**      out_call_args   The marshaled RPC output arguments received from the
**                      remote thread.
**
**      remote_ndr_fmt  The Network Data Representation format of the remote
**                      machine. This is used by the stub to unmarshal
**                      arguments encoded using NDR as the trasnfer syntax.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_transceive 
(
    rpc_call_handle_t       call_h,
    rpc_iovector_p_t        in_call_args,
    rpc_iovector_elt_t      *out_call_args,
    ndr_format_t            *remote_ndr_fmt,
    unsigned32              *status
)
{ 
    RPC_LOG_CALL_TRANSCEIVE_NTR;
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    (*rpc_g_protocol_id[((rpc_call_rep_p_t) (call_h))->protocol_id].call_epv
        ->call_transceive)
            ((rpc_call_rep_p_t) call_h, in_call_args,
            out_call_args, remote_ndr_fmt, status);

    RPC_LOG_CALL_TRANSCEIVE_XIT;
}

/*
**++
**
**  ROUTINE NAME:       rpc_call_receive
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Return a buffer of marshaled arguments from the remote thread. This
**  routine is intended for use by the client or server stub only.
**
**  INPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      call_args       The marshaled RPC arguments received from the
**                      remote thread.
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_receive 
(
    rpc_call_handle_t       call_h,
    rpc_iovector_elt_t      *call_args,
    unsigned32              *status
)
{
    RPC_LOG_CALL_RECEIVE_NTR;
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    (*rpc_g_protocol_id[((rpc_call_rep_p_t) (call_h))->protocol_id].call_epv
        ->call_receive)
            ((rpc_call_rep_p_t) call_h, call_args, status);

    RPC_LOG_CALL_RECEIVE_XIT;
}

/*
**++
**
**  ROUTINE NAME:       rpc_call_block_until_free
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine will block until all marshaled RPC output arguments have
**  been transmitted and acknowledged. It is provided for use by the server
**  stub when the marshaled arguments are contained in buffers which are on
**  the stack.
**
**  INPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_block_until_free 
(
    rpc_call_handle_t       call_h,
    unsigned32              *status
)
{ 
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    (*rpc_g_protocol_id[((rpc_call_rep_p_t) (call_h))->protocol_id].call_epv
        ->call_block_until_free) ((rpc_call_rep_p_t) call_h, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_call_cancel
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Forward a cancel to the remote RPC thread by the call handle
**  provided. This routine is intended for use by the client stub only.
**
**  INPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_cancel 
(
    rpc_call_handle_t       call_h,
    unsigned32              *status
)
{ 
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    (*rpc_g_protocol_id[((rpc_call_rep_p_t) (call_h))->protocol_id].call_epv
        ->call_cancel) ((rpc_call_rep_p_t) call_h, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_call_end
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  End a Remote Procedure Call. This is the last in a sequence of calls by
**  the client or server stub. This routine is intended for use by the
**  client stub only.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_coding_error
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_end 
(
    rpc_call_handle_t       *call_h,
    unsigned32              *status
)
{ 
    RPC_LOG_CALL_END_NTR;
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    (*rpc_g_protocol_id[((rpc_call_rep_p_t) (*call_h))->protocol_id].call_epv
        ->call_end) ((rpc_call_rep_p_t *) call_h, status);

    RPC_LOG_CALL_END_XIT;
}

/*
**++
**
**  ROUTINE NAME:       rpc_call_transmit_fault
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Forward an exception to the remote RPC thread identified by the call
**  handle. This routine is intended for use by the client or server stub
**  only.
**
**  INPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**      call_fault_info The marshaled fault information.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_transmit_fault 
(
    rpc_call_handle_t       call_h,
    rpc_iovector_p_t        call_fault_info,
    unsigned32              *status
)
{ 
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    (*rpc_g_protocol_id[((rpc_call_rep_p_t) (call_h))->protocol_id].call_epv
        ->call_transmit_fault)
            ((rpc_call_rep_p_t) call_h, call_fault_info, status);
} 

/*
**++
**
**  ROUTINE NAME:       rpc_call_receive_fault
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Return a buffer of marshaled fault information from the remote thread. This
**  routine is intended for use by the client or server stub only.
**
**  INPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      fault_info      The marshaled fault information received from the
**                      remote thread.
**
**      remote_ndr_fmt  The Network Data Representation format of the remote
**                      machine. This is used by the stub to unmarshal
**                      arguments encoded using NDR as the trasnfer syntax.
**
**
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_call_receive_fault (call_h, fault_info, remote_ndr_fmt, status)

rpc_call_handle_t       call_h;
rpc_iovector_elt_t      *fault_info;
ndr_format_t            *remote_ndr_fmt;
unsigned32              *status;
        
{
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    (*rpc_g_protocol_id[((rpc_call_rep_p_t) (call_h))->protocol_id].call_epv
        ->call_receive_fault)
            ((rpc_call_rep_p_t) call_h, fault_info, remote_ndr_fmt, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc_call_did_mgr_execute
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Return a boolean indicating whether the manager routine for the
**  RPC identified by the call handle has begun executing.
**
**  INPUTS:
**
**      call_h          The call handle which uniquely identifies this RPC.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          any of the RPC Protocol Service status codes
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean32
**
**                      true if the manager routine has begun
**                      executing, false otherwise.
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC boolean32 rpc_call_did_mgr_execute 
(
    rpc_call_handle_t       call_h,
    unsigned32              *status
)
{
    CODING_ERROR (status);
    
    /*
     * dispatch to the appropriate protocol service
     */
    return ((*rpc_g_protocol_id[((rpc_call_rep_p_t) (call_h))->protocol_id].call_epv
             ->call_did_mgr_execute)
            ((rpc_call_rep_p_t) call_h, status));
}
