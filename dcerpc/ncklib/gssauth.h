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
#ifndef _GSSAUTH_H
#define _GSSAUTH_H	1
/*
**
**  NAME
**
**      gssauth.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Types and routines private to the "gss negotiate"
**      module. The module provides the following implementations:
**      - gss_mskrb5: gssapi krb5 with GSS_C_DCE_STYLE
**      - gss_negotiate: gssapi spnego with GSS_C_DCE_STYLE
**                       but only with krb5 as spnego mech yet
**
*/

#include <commonp.h>
#include <com.h>
#include <comp.h>
#include <gssauthcn.h>

#include <gssapi/gssapi.h>

/*
 * State block containing all the state of one end of an authenticated
 * connection.
 */

typedef struct rpc_gssauth_info_t {
    rpc_auth_info_t auth_info;  /* This must be the first element. */

    gss_name_t gss_server_name;
    gss_cred_id_t gss_creds;

    /* security context information is available
       here for the server side */
    rpc_gssauth_cn_info_p_t cn_info;

} rpc_gssauth_info_t, *rpc_gssauth_info_p_t;


#define RPC_CN_PKT_SIZEOF_GSSWRAP_AUTH_TLR   (45)
#define RPC_CN_PKT_SIZEOF_GSSGETMIC_AUTH_TLR (37)

/*
 * Prototypes for PRIVATE routines.
 */

PRIVATE rpc_protocol_id_t rpc__gssauth_negotiate_cn_init (
         rpc_auth_rpc_prot_epv_p_t      * /*epv*/,
         unsigned32                     * /*st*/
    );

PRIVATE rpc_protocol_id_t rpc__gssauth_mskrb_cn_init (
         rpc_auth_rpc_prot_epv_p_t      * /*epv*/,
         unsigned32                     * /*st*/
    );

PRIVATE const char *rpc__gssauth_error_map (
	int			/*major_status*/,
	OM_uint32		/*minor_status*/,
	const gss_OID		/*mech*/,
	char			* /*message_buffer*/,
	unsigned32		/*message_length*/,
	unsigned32		* /*st*/
    );

#endif /* _GSSAUTH_H */
