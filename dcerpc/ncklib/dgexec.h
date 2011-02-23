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
**      dgexec.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  
**
**
*/

#ifndef _DGEXEC_H
#define _DGEXEC_H

#include <dce/dce.h>

PRIVATE void rpc__dg_execute_call    (
        pointer_t  /*scall_*/,
        boolean32  /*call_was_queued*/
    );

/*
 * To implement backward compatibilty, declare a pointer to a routine
 * that can call into pre-v2 server stubs.  We declare this function
 * in this indirect way so that it is possible to build servers that
 * don't support backward compatibility (and thus save space).  The
 * compatibility code will only be linked into a server if the server
 * application code itself calls a compatibility function, most likely
 * rpc_$register.  rpc_$register is then responsible for initializing
 * this function pointer so that dg_execute_call can find the compatibilty
 * function.  In this way, libnck has no direct references to the
 * compatibilty code.
 */

typedef void (*rpc__dg_pre_v2_server_fn_t) (
        rpc_if_rep_p_t  /*ifspec*/,
        unsigned32  /*opnum*/,
        handle_t  /*h*/,
        rpc_call_handle_t  /*call_h*/,
        rpc_iovector_elt_p_t  /*iove_ins*/,
        ndr_format_t  /*ndr_format*/,
        rpc_v2_server_stub_epv_t  /*server_stub_epv*/,
        rpc_mgr_epv_t  /*mgr_epv*/,
        unsigned32 * /*st*/
    ); 
                                                               
#endif /* _DGEXEC_H */
