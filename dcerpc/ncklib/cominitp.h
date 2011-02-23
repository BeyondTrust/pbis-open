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
#ifndef _COMINITP_H
#define _COMINITP_H
/*
**
**  NAME
**
**      cominitp.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Private interface to the Common Communications Service Initialization
**  Service.
**
**
*/

/***********************************************************************/
/*
 * Note: these are defined for later use of shared images
 */
#include <dce/dce.h>

#ifdef __cplusplus
extern "C" {
#endif

PRIVATE void rpc__load_modules(void);
	
PRIVATE rpc_naf_init_fn_t rpc__load_naf (
        rpc_naf_id_elt_p_t              /*naf*/, 
        unsigned32                      * /*st*/
    );

PRIVATE rpc_prot_init_fn_t rpc__load_prot (
        rpc_protocol_id_elt_p_t         /*rpc_protocol*/,
        unsigned32                      * /*st*/
    );

PRIVATE rpc_auth_init_fn_t rpc__load_auth (
        rpc_authn_protocol_id_elt_p_t   /*auth_protocol*/,
        unsigned32                      * /*st*/
    );

#ifdef __cplusplus
}
#endif


#endif /* _COMINITP_H */
