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
#ifndef _GSSAUTHCN_H
#define _GSSAUTHCN_H 	1

/*
**  NAME
**
**      gssauthcn.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  The gssauth CN authentication module interface.
**
**
*/

#include <cn.h>

#include <gssapi/gssapi_ext.h>
#include <krb5.h>

typedef struct
{
    rpc_cn_auth_info_t  cn_info;

    gss_ctx_id_t gss_ctx;
    int gss_rc;
} rpc_gssauth_cn_info_t, *rpc_gssauth_cn_info_p_t;

#endif /* _GSSAUTHCN_H */
