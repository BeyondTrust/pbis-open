/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
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
**      bindcall.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Provides canned routines that may be used in conjunction with the
**      [binding_callout] ACF attribute.  These routines are called from a
**      client stub to possibly modify the binding handle, typically with
**      security information.
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <dce/rpc.h>
#include <dce/stubbase.h>

void rpc_ss_bind_authn_client
(
    rpc_binding_handle_t    *p_bh,      /* [io] Binding handle */
    rpc_if_handle_t         if_h,       /* [in] Interface handle */
    error_status_t          *p_st       /*[out] Return status */
)
{
    unsigned_char_t *princ_name;        /* Server principal name */

    /* Resolve binding handle if not fully bound */
    rpc_ep_resolve_binding(*p_bh, if_h, p_st);
    if (*p_st != rpc_s_ok)
        return;

    /* Get server principal name */
    rpc_mgmt_inq_server_princ_name(
        *p_bh,                          /* binding handle */
        (unsigned32) rpc_c_authn_default, /* default authentication service */
        &princ_name,                    /* server principal name */
        p_st);
    if (*p_st != rpc_s_ok)
        return;

    /* Set auth info in binding handle */
    rpc_binding_set_auth_info(
        *p_bh,                          /* binding handle */
        princ_name,                     /* server principal name */
        (unsigned32) rpc_c_protect_level_default, /* default protection level */
        (unsigned32) rpc_c_authn_default, /* default authentication service */
        NULL,                           /* def. auth credentials (login ctx) */
        (unsigned32) rpc_c_authz_name,  /* authz based on cli principal name */
        p_st);
}
