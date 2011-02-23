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
#ifndef _NTLMSSP_H
#define _NTLMSSP_H	1
/*
**
**  NAME
**
**      ntlmssp.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Types and routines private to the ntlmssp
**      module. The module provides the following implementations.
**
*/

#include <commonp.h>
#include <com.h>
#include <comp.h>
#include <ntlmsspcn.h>

#include <gssapi/gssapi.h>

/*
 * State block containing all the state of one end of an authenticated
 * connection.
 */

typedef struct rpc_ntlmauth_info_t {
    rpc_auth_info_t auth_info;  /* This must be the first element. */

    gss_name_t      gss_server_name;
    gss_cred_id_t   gss_creds;

    /* security context information is available
       here for the server side */
    rpc_ntlmauth_cn_info_p_t cn_info;

} rpc_ntlmauth_info_t, *rpc_ntlmauth_info_p_t;


/* 1.3.6.1.4.1.311.2.2.10 */
#define GSS_MECH_NTLM       "\x2b\x06\x01\x04\x01\x82\x37\x02\x02\x0a"
#define GSS_MECH_NTLM_LEN   10

/* 1.3.6.1.4.1.27433.3.1 */
#define GSS_CRED_OPT_PW     "\x2b\x06\x01\x04\x01\x81\xd6\x29\x03\x01"
#define GSS_CRED_OPT_PW_LEN 10

#define RPC_CN_PKT_SIZEOF_NTLMSSP_AUTH_TLR   (16)

/*
 * NTLM Credentials to pass to gss routines.
 * This is a copy (to avoid cirular dependency) of SEC_WINNT_AUTH_IDENTITY
 * type from lsass/include/ntlm/sspintlm.h
 */
typedef struct rpc_ntlmssp_auth_ident
{
    PCHAR User;
    DWORD UserLength;
    PCHAR Domain;
    DWORD DomainLength;
    PCHAR Password;
    DWORD PasswordLength;
    DWORD Flags;
} rpc_ntlmssp_auth_ident_t, *rpc_ntlmssp_auth_ident_t_p;


/*
 * Prototypes for PRIVATE routines.
 */

PRIVATE rpc_protocol_id_t rpc__ntlmauth_cn_init (
         rpc_auth_rpc_prot_epv_p_t      * /*epv*/,
         unsigned32                     * /*st*/
    );

PRIVATE const char *rpc__ntlmauth_error_map (
	int			/*major_status*/,
	OM_uint32		/*minor_status*/,
	const gss_OID		/*mech*/,
	char			* /*message_buffer*/,
	unsigned32		/*message_length*/,
	unsigned32		* /*st*/
    );

#endif /* _NTLMSSP_H */
