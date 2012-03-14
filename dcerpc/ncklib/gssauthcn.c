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
**  NAME
**
**      gssauthcn.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  The gssauth CN authentication module.
**
**
*/

#include <gssauth.h>
#include <gssauthcn.h>

INTERNAL boolean32 rpc__gssauth_cn_three_way (void);

INTERNAL boolean32 rpc__gssauth_cn_context_valid (
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_negotiate_cn_create_info (
       rpc_authn_level_t		/* in */     /*authn_level*/,
       rpc_authn_flags_t                /* in */     /*authn_flags */,
       rpc_auth_info_p_t		/* out */    * /*auth_info*/,
       unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_mskrb_cn_create_info (
       rpc_authn_level_t		/* in */     /*authn_level*/,
       rpc_authn_flags_t                /* in */     /*authn_flags */,
       rpc_auth_info_p_t		/* out */    * /*auth_info*/,
       unsigned32			/* out */    * /*st*/
    );

INTERNAL boolean32 rpc__gssauth_cn_cred_changed (
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_cn_cred_refresh (
	rpc_auth_info_p_t		/* in */     /*auth_info*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_cn_fmt_client_req (
	rpc_cn_assoc_sec_context_p_t	/* in */     /*assoc_sec*/,
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	pointer_t			/* in/out */ /*auth_value*/,
	unsigned32			/* in/out */ * /*auth_value_len*/,
	pointer_t			/* in/out */ * /*last_auth_pos*/,
	unsigned32			/* out */    * /*auth_len_remain*/,
	unsigned32			/* in */     /*old_server*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_cn_fmt_srvr_resp (
	unsigned32			/* in */     /*verify_st*/,
	rpc_cn_assoc_sec_context_p_t	/* in */     /*assoc_sec*/,
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	pointer_t			/* in */     /*req_auth_value*/,
	unsigned32			/* in */     /*req_auth_value_len*/,
	pointer_t			/* in/out */ /*auth_value*/,
	unsigned32			/* in/out */ * /*auth_value_len*/
    );

INTERNAL void rpc__gssauth_cn_free_prot_info (
	rpc_auth_info_p_t		/* in */     /*info*/,
	rpc_cn_auth_info_p_t		/* in/out */ * /*cn_info*/
    );

INTERNAL void rpc__gssauth_cn_get_prot_info (
	rpc_auth_info_p_t		/* in */     /*info*/,
	rpc_cn_auth_info_p_t		/* out */    * /*cn_info*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_cn_pre_call (
	rpc_cn_assoc_sec_context_p_t	/* in */     /*assoc_sec*/,
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	pointer_t			/* in/out */ /*auth_value*/,
	unsigned32			/* in/out */ * /*auth_value_len*/,
	unsigned32			/* in/out */ * /*st*/
    );

INTERNAL void rpc__gssauth_cn_pre_send (
	rpc_cn_assoc_sec_context_p_t	/* in */     /*assoc_sec*/,
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	rpc_socket_iovec_p_t		/* in/out */ /*iov*/,
	unsigned32			/* in */     /*iovlen*/,
	rpc_socket_iovec_p_t		/* out */    /*out_iov*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_cn_recv_check (
	rpc_cn_assoc_sec_context_p_t	/* in */     /*assoc_sec*/,
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	rpc_cn_common_hdr_p_t		/* in */     /*pdu*/,
	unsigned32			/* in */     /*pdu_len*/,
	unsigned32			/* in */     /*cred_len*/,
	rpc_cn_auth_tlr_p_t		/* in */     /*auth_tlr*/,
	boolean32			/* in */     /*unpack_ints*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_cn_tlr_uuid_crc (
	pointer_t			/* in */     /*auth_value*/,
	unsigned32			/* in */     /*auth_value_len*/,
	unsigned32			/* out */    * /*uuid_crc*/
    );

INTERNAL void rpc__gssauth_cn_tlr_unpack (
	rpc_cn_packet_p_t		/* in */     /*pkt_p*/,
	unsigned32			/* in */     /*auth_value_len*/,
	unsigned8			/* out */    * /*packed_drep*/
    );

INTERNAL void rpc__gssauth_cn_vfy_client_req (
	rpc_cn_assoc_sec_context_p_t	/* in */     /*assoc_sec*/,
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	pointer_t			/* in */     /*auth_value*/,
	unsigned32			/* in */     /*auth_value_len*/,
	unsigned32			/* in */     /*old_client*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_cn_vfy_srvr_resp (
	rpc_cn_assoc_sec_context_p_t	/* in */     /*assoc_sec*/,
	rpc_cn_sec_context_p_t		/* in */     /*sec*/,
	pointer_t			/* in/out */ /*auth_value*/,
	unsigned32			/* in */     /*auth_value_len*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL rpc_cn_auth_epv_t rpc_g_gssauth_negotiate_cn_epv =
{
	rpc__gssauth_cn_three_way,
	rpc__gssauth_cn_context_valid,
	rpc__gssauth_negotiate_cn_create_info,
	rpc__gssauth_cn_cred_changed,
	rpc__gssauth_cn_cred_refresh,
	rpc__gssauth_cn_fmt_client_req,
	rpc__gssauth_cn_fmt_srvr_resp,
	rpc__gssauth_cn_free_prot_info,
	rpc__gssauth_cn_get_prot_info,
	rpc__gssauth_cn_pre_call,
	rpc__gssauth_cn_pre_send,
	rpc__gssauth_cn_recv_check,
	rpc__gssauth_cn_tlr_uuid_crc,
	rpc__gssauth_cn_tlr_unpack,
	rpc__gssauth_cn_vfy_client_req,
	rpc__gssauth_cn_vfy_srvr_resp
};

INTERNAL rpc_cn_auth_epv_t rpc_g_gssauth_mskrb_cn_epv =
{
	rpc__gssauth_cn_three_way,
	rpc__gssauth_cn_context_valid,
	rpc__gssauth_mskrb_cn_create_info,
	rpc__gssauth_cn_cred_changed,
	rpc__gssauth_cn_cred_refresh,
	rpc__gssauth_cn_fmt_client_req,
	rpc__gssauth_cn_fmt_srvr_resp,
	rpc__gssauth_cn_free_prot_info,
	rpc__gssauth_cn_get_prot_info,
	rpc__gssauth_cn_pre_call,
	rpc__gssauth_cn_pre_send,
	rpc__gssauth_cn_recv_check,
	rpc__gssauth_cn_tlr_uuid_crc,
	rpc__gssauth_cn_tlr_unpack,
	rpc__gssauth_cn_vfy_client_req,
	rpc__gssauth_cn_vfy_srvr_resp
};

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_three_way
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      Determine whether the authentication protocol requires a
**      3-way authentication handshake. If true the client is expected to
**      provide an rpc_auth3 PDU before the security context is fully
**      established and a call can be made.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean32
**
**      True if the authentication protocol requires a 3-way
**      authentication handshake.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean32 rpc__gssauth_cn_three_way (void)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_three_way)\n"));

	return false;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_context_valid
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      Determine whether the established security context will be
**      valid (i. e. timely) for the next 300 seconds. If
**      not this routine will try to renew the context.
**      If it cannot be renewed false is returned. This is
**      called from the client side.
**
**  INPUTS:
**
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean32
**
**      True if security context identified by the auth
**      information rep and RPC auth information rep will
**      still be valid in 300 seconds, false if not.
**
**  SIDE EFFECTS:
**
**      The context may be renewed.
**
**--
**/

INTERNAL boolean32 rpc__gssauth_cn_context_valid
(
	rpc_cn_sec_context_p_t	sec,
	unsigned32		*st
)
{
	CODING_ERROR (st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_context_valid)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
		("(rpc__gssauth_cn_context_valid) time->%x\n", time));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_context_valid) prot->%x level->%x key_id->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_CONTEXT_VALID)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return false;
	}
#endif

	*st = rpc_s_ok;
	return true;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_create_info
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      Create an auth information rep data structure with and
**      add a reference to it. This is called on the server
**      side. The fields will be initialized to NULL values.
**      The caller should fill them in based on information
**      decrypted from the authenticator passed in the bind
**      request.
**
**  INPUTS:
**
**      authn_level     The authentication level to be applied over this
**                      security context.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      info            A pointer to the auth information rep structure
**                      containing RPC protocol indenpendent information.
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:
**
**      The newly create auth info will have a reference count of 1.
**
**--
**/

INTERNAL void rpc__gssauth_cn_create_info
(
	rpc_authn_protocol_id_t authn_protocol,
	rpc_authn_level_t	authn_level,
	rpc_authn_flags_t       authn_flags,
	rpc_auth_info_p_t	*auth_info,
	unsigned32		*st
)
{
	rpc_gssauth_info_p_t gssauth_info;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_create_info)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_create_info) prot->%x level->%x\n",
		authn_protocol,
		authn_level));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_CREATE_INFO)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif

	if ((authn_level != rpc_c_authn_level_connect) &&
	    (authn_level != rpc_c_authn_level_pkt_integrity) &&
	    (authn_level != rpc_c_authn_level_pkt_privacy)) {
		*st = rpc_s_unsupported_authn_level;
		return;
	}

	/* Accept header signing if requested */
	authn_flags &= rpc_c_protect_flags_header_sign;

	/*
	 * Allocate storage for a gssauth info structure from heap.
	 */
	RPC_MEM_ALLOC(gssauth_info,
		      rpc_gssauth_info_p_t,
		      sizeof (rpc_gssauth_info_t),
		      RPC_C_MEM_GSSAUTH_INFO,
		      RPC_C_MEM_WAITOK);
	memset (gssauth_info, 0, sizeof(rpc_gssauth_info_t));

	/*
	 * Initialize the common auth_info stuff.
	 */
	gssauth_info->auth_info.refcount = 1;
	gssauth_info->auth_info.server_princ_name = '\0';
	gssauth_info->auth_info.authn_level = authn_level;
	gssauth_info->auth_info.authn_flags = authn_flags;
	gssauth_info->auth_info.authn_protocol = authn_protocol;
	gssauth_info->auth_info.authz_protocol = rpc_c_authz_name;
	gssauth_info->auth_info.is_server = true;

	*auth_info = &gssauth_info->auth_info;
	*st = rpc_s_ok;
}

INTERNAL void rpc__gssauth_negotiate_cn_create_info
(
	rpc_authn_level_t	authn_level,
	rpc_authn_flags_t       authn_flags,
	rpc_auth_info_p_t	*auth_info,
	unsigned32		*st
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_negotiate_cn_create_info)\n"));

	rpc__gssauth_cn_create_info(rpc_c_authn_gss_negotiate,
				    authn_level,
				    authn_flags,
				    auth_info,
				    st);
}

INTERNAL void rpc__gssauth_mskrb_cn_create_info
(
	rpc_authn_level_t	authn_level,
	rpc_authn_flags_t       authn_flags,
	rpc_auth_info_p_t	*auth_info,
	unsigned32		*st
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_mskrb_cn_create_info)\n"));

	rpc__gssauth_cn_create_info(rpc_c_authn_gss_mskrb,
				    authn_level,
				    authn_flags,
				    auth_info,
				    st);
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_cred_changed
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      Determine whether the client's credentials stored in the
**      security context are different from those in the auth info.
**      If they are not the same return true, else false.
**
**  INPUTS:
**
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean32
**
**      True if the credentials identified by the auth
**      information rep and RPC auth information rep are different,
**      false if not.
**
**      The md5 checksum algorithm requires the use of the session key
**      to encrypt the CRC(assoc_uuid).  Since the session key will
**      change when the credential changes, this routine sets the flag
**      indicating that a (potentially) valid encrypted crc is now
**      invalid, forcing a recomputation.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean32 rpc__gssauth_cn_cred_changed
(
	rpc_cn_sec_context_p_t	sec,
	unsigned32		*st
)
{
	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_cred_changed)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_cred_changed) prot->%x level->%x key_id->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_CRED_CHANGED)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return false;
	}
#endif

	/*
	 * Assume that cred is already valid.
	 */
	*st = rpc_s_ok;
	return false;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_cred_refresh
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      Determine whether the client's credentials are still
**      valid. If not this routine will try to renew the credentials.
**      If they cannot be renewed an error is returned. This routine
**      is called from the client side.
**
**  INPUTS:
**
**      auth_info       A pointer to the auth information rep
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_cred_refresh
(
	rpc_auth_info_p_t	auth_info,
	unsigned32		*st
)
{
	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_cred_refresh)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_cred_refresh) prot->%x level->%x\n",
		auth_info->authn_protocol,
		auth_info->authn_level));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_CRED_REFRESH)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif

	/*
	 * Assume that cred is already valid.
	 */
	*st = rpc_s_ok;
}

PRIVATE const char *rpc__gssauth_error_map
(
	int			major_status,
	OM_uint32		minor_status,
	const gss_OID		mech,
	char			*message_buffer,
	unsigned32		message_length,
	unsigned32		*st
)
{
	switch (minor_status) {
	case (OM_uint32) KRB5KRB_AP_ERR_BAD_INTEGRITY:
		*st = rpc_s_auth_bad_integrity;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_TKT_EXPIRED:
		*st = rpc_s_auth_tkt_expired;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_TKT_NYV:
		*st = rpc_s_auth_tkt_nyv;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_REPEAT:
		*st = rpc_s_auth_repeat;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_NOT_US:
		*st = rpc_s_auth_not_us;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_BADMATCH:
		*st = rpc_s_auth_badmatch;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_SKEW:
		*st = rpc_s_auth_skew;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_BADADDR:
		*st = rpc_s_auth_badaddr;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_BADVERSION:
		*st = rpc_s_auth_badversion;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_MSG_TYPE:
		*st = rpc_s_auth_msg_type;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_MODIFIED:
		*st = rpc_s_auth_modified;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_BADORDER:
		*st = rpc_s_auth_badorder;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_BADKEYVER:
		*st = rpc_s_auth_badkeyver;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_NOKEY:
		*st = rpc_s_auth_nokey;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_MUT_FAIL:
		*st = rpc_s_auth_mut_fail;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_BADDIRECTION:
		*st = rpc_s_auth_baddirection;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_METHOD:
		*st = rpc_s_auth_method;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_BADSEQ:
		*st = rpc_s_auth_badseq;
		break;
	case (OM_uint32) KRB5KRB_AP_ERR_INAPP_CKSUM:
		*st = rpc_s_auth_inapp_cksum;
		break;
	default:
		if (major_status == GSS_S_CONTINUE_NEEDED) {
		    *st = rpc_s_partial_credentials;
		    break;
		}
		if (major_status == GSS_S_CONTEXT_EXPIRED)
		{
		    *st = rpc_s_auth_tkt_expired;
		    break;
		}
		*st = rpc_s_auth_method;
		break;
	}

	if (message_buffer && message_length) {
		OM_uint32 min2;
		gss_buffer_desc major_msg;
		gss_buffer_desc minor_msg;
		OM_uint32 msg_ctx = 0;

		major_msg.value = NULL;
		major_msg.length = 0;
		minor_msg.value = NULL;
		minor_msg.length = 0;

		gss_display_status(&min2, major_status, GSS_C_GSS_CODE,
				   NULL, &msg_ctx, &major_msg);
		gss_display_status(&min2, minor_status, GSS_C_MECH_CODE,
				   mech, &msg_ctx, &minor_msg);

		snprintf((char *)message_buffer, message_length - 1,
			 "major: %*s (%u) minor: %*s (%u) => dcerpc: (0x%08x)\n",
			 (int)major_msg.length, (char *)major_msg.value,
			 major_status, (int)minor_msg.length,
			 (char *)minor_msg.value, minor_status,
			 (unsigned int)*st);
		message_buffer[message_length - 1] = '\0';

		gss_release_buffer(&min2, &major_msg);
		gss_release_buffer(&min2, &minor_msg);

		return (const char *)message_buffer;
	}

	return NULL;
}

#define	__SPNEGO_OID_LENGTH 6
#define	__SPNEGO_OID "\053\006\001\005\005\002"
	static const gss_OID_desc rpc__gssauth_spnego_oid = {__SPNEGO_OID_LENGTH, __SPNEGO_OID};
#define __GSS_MECH_KRB5_OID_LENGTH 9
#define __GSS_MECH_KRB5_OID "\052\206\110\206\367\022\001\002\002"
	static const gss_OID_desc rpc__gssauth_krb5_oid = {__GSS_MECH_KRB5_OID_LENGTH, __GSS_MECH_KRB5_OID};

INTERNAL int rpc__gssauth_select_mech
(
	OM_uint32		*minor_status,
	rpc_authn_protocol_id_t	authn_protocol,
	gss_OID			*req_mech
)
{
	gss_OID selected_mech = GSS_C_NO_OID;

	*minor_status = 0;

	switch (authn_protocol) {
	case rpc_c_authn_gss_negotiate:
		selected_mech = (gss_OID)&rpc__gssauth_spnego_oid;
		break;
	case rpc_c_authn_gss_mskrb:
		selected_mech = (gss_OID)&rpc__gssauth_krb5_oid;
		break;
	}

	*req_mech = selected_mech;
	return GSS_S_COMPLETE;
}

INTERNAL int rpc__gssauth_select_flags
(
	OM_uint32		*minor_status,
	rpc_authn_level_t	authn_level,
	OM_uint32		*req_flags
)
{
#ifndef GSS_C_DCE_STYLE
#define GSS_C_DCE_STYLE 4096
#endif
	OM_uint32 flags = GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG |
                          GSS_C_SEQUENCE_FLAG | GSS_C_DCE_STYLE;
	*minor_status = 0;

	switch (authn_level) {
	case rpc_c_authn_level_connect:
		break;
	case rpc_c_protect_level_pkt_integ:
		flags |= GSS_C_INTEG_FLAG;
		break;
	case rpc_c_protect_level_pkt_privacy:
		flags |= GSS_C_INTEG_FLAG | GSS_C_CONF_FLAG;
		break;
	}

	*req_flags = flags;
	return GSS_S_COMPLETE;
}

INTERNAL int rpc__gssauth_create_client_token
(
	OM_uint32			*minor_status,
	const rpc_cn_sec_context_p_t	sec,
	const gss_cred_id_t		gss_creds,
	const gss_name_t		gss_server_name,
	gss_ctx_id_t			*gss_ctx,
	gss_buffer_desc			*output_token
)
{
	int gss_rc;
	gss_OID req_mech = GSS_C_NO_OID;
	OM_uint32 req_flags;
	gss_buffer_desc input_token;

	gss_rc = rpc__gssauth_select_mech(minor_status,
					  sec->sec_info->authn_protocol,
					  &req_mech);
	if (gss_rc != GSS_S_COMPLETE) {
		return gss_rc;
	}

	gss_rc = rpc__gssauth_select_flags(minor_status,
					   sec->sec_info->authn_level,
					   &req_flags);
	if (gss_rc != GSS_S_COMPLETE) {
		return gss_rc;
	}

	input_token.value = NULL;
	input_token.length = 0;
	gss_rc = gss_init_sec_context(minor_status,
				      gss_creds,
				      gss_ctx,
				      gss_server_name,
				      req_mech,
				      req_flags,
				      0,
				      NULL,
				      &input_token,
				      NULL,
				      output_token,
				      NULL,
				      NULL);
	if (gss_rc == GSS_S_COMPLETE) {
				*minor_status = -1;
/*TODO: how can that happen*/	return GSS_S_FAILURE;
	} else if (gss_rc != GSS_S_CONTINUE_NEEDED) {
		return gss_rc;
	}

	return GSS_S_CONTINUE_NEEDED;
}

INTERNAL int rpc__gssauth_verify_server_token
(
	OM_uint32			*minor_status,
	const rpc_cn_sec_context_p_t	sec,
	const gss_cred_id_t		gss_creds,
	const gss_name_t		gss_server_name,
	gss_ctx_id_t			gss_ctx,
	gss_buffer_desc			input_token,
	gss_buffer_desc			*output_token
)
{
	int gss_rc;

	gss_OID req_mech = GSS_C_NO_OID;
	OM_uint32 req_flags;

	gss_rc = rpc__gssauth_select_mech(minor_status,
					  sec->sec_info->authn_protocol,
					  &req_mech);
	if (gss_rc != GSS_S_COMPLETE) {
		return gss_rc;
	}

	gss_rc = rpc__gssauth_select_flags(minor_status,
					   sec->sec_info->authn_protocol,
					   &req_flags);
	if (gss_rc != GSS_S_COMPLETE) {
		return gss_rc;
	}

	gss_rc = gss_init_sec_context(minor_status,
				      gss_creds,
				      &gss_ctx,
				      gss_server_name,
				      req_mech,
				      req_flags,
				      0,
				      NULL,
				      &input_token,
				      NULL,
				      output_token,
				      NULL,
				      NULL);
	if (gss_rc != GSS_S_COMPLETE) {
		return gss_rc;
	}

	return GSS_S_COMPLETE;
}

INTERNAL unsigned32 rpc__gssauth_cn_inq_flags_supported
(
	unsigned32 authn_flags
)
{
	unsigned st = rpc_s_ok;

	/*
	 * Mask off all known flags to see if there's any remaining
	 * (i.e. unknown) flag set. Return error if there is.
	 */
	if (authn_flags & ~rpc_c_protect_flags_header_sign)
	{
		st = rpc_s_unsupported_protect_level;
	}

	return st;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_fmt_client_req
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will format the auth_value field of
**      either an rpc_bind or rpc_alter_context PDU. This is
**      called from the client side association state machine.
**
**  INPUTS:
**
**      assoc_sec       A pointer to per-association security context
**                      including association UUID CRC and sequence numbers.
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**      auth_value      A pointer to the auth_value field in the rpc_bind or
**                      rpc_alter_context PDU authentication trailer.
**
**  INPUTS/OUTPUTS:
**
**      auth_value_len  On input, the lenght, in bytes of the available space
**                      for the auth_value field. On output, the lenght in
**                      bytes used in encoding the auth_value field. Zero if
**                      an error status is returned.
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

#define RPC__GSSAUTH_CN_AUTH_MAX_LEN 160

INTERNAL void rpc__gssauth_cn_fmt_client_req
(
	rpc_cn_assoc_sec_context_p_t	assoc_sec ATTRIBUTE_UNUSED,
	rpc_cn_sec_context_p_t		sec,
	pointer_t			auth_value,
	unsigned32			*auth_value_len,
	pointer_t			*last_auth_pos ATTRIBUTE_UNUSED,
	unsigned32			*auth_len_remain,
	unsigned32			old_server ATTRIBUTE_UNUSED,
	unsigned32			*st
)
{
	rpc_gssauth_info_p_t gssauth_info = (rpc_gssauth_info_p_t)sec->sec_info;
	rpc_gssauth_cn_info_p_t gssauth_cn_info = (rpc_gssauth_cn_info_p_t)sec->sec_cn_info;
	int gss_rc;
	OM_uint32 minor_status;
	gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_fmt_client_req)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_fmt_client_req) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id,
		assoc_sec->assoc_uuid_crc,
		assoc_sec->assoc_next_snd_seq,
		assoc_sec->assoc_next_rcv_seq));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_FMT_CLIENT_REQ)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif
	/*
	 * Check if requested authn_flags are supported by this mechanism
	 */
	*st = rpc__gssauth_cn_inq_flags_supported(sec->sec_info->authn_flags);
	if (*st != rpc_s_ok)
	{
		return;
	}

	if (gssauth_cn_info->gss_ctx != GSS_C_NO_CONTEXT) {
		/*
		 * transfer the output of rpc__gssauth_verify_server_token()
		 */
		output_token.length = assoc_sec->krb_message.length;
		output_token.value = assoc_sec->krb_message.data;
		assoc_sec->krb_message.length = 0;
		assoc_sec->krb_message.data = NULL;
	} else {
		gss_rc = rpc__gssauth_create_client_token(&minor_status,
							  sec,
							  gssauth_info->gss_creds,
							  gssauth_info->gss_server_name,
							  &gssauth_cn_info->gss_ctx,
							  &output_token);
		gssauth_cn_info->gss_rc = gss_rc;
		if (gss_rc != GSS_S_CONTINUE_NEEDED) {
			char msg[256];
			rpc__gssauth_error_map(gss_rc, minor_status,
					       (gss_OID)&rpc__gssauth_krb5_oid,
					       msg, sizeof(msg), st);
			RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
				("(rpc__gssauth_cn_fmt_client_req): %s\n", msg));
			return;
		}
	}

	if (*auth_value_len < output_token.length) {

		if (*auth_value_len < RPC__GSSAUTH_CN_AUTH_MAX_LEN) {
			gss_release_buffer(&minor_status, &output_token);
			RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
				("(rpc__gssauth_cn_fmt_client_req): auth_value_len[%u] < %u\n",
				(unsigned int)*auth_value_len,
				RPC__GSSAUTH_CN_AUTH_MAX_LEN));
			*st = rpc_s_credentials_too_large;
			return;
		}
		/*
		 * we'll reformat the PDU in the pre_send function
		 * because we need to generate a PDU larger than the
		 * max fragment size
		 */
		assoc_sec->krb_message.length = output_token.length;
		assoc_sec->krb_message.data = output_token.value;

		*auth_value_len = RPC__GSSAUTH_CN_AUTH_MAX_LEN;
		*auth_len_remain = 0;
		memset((unsigned_char_p_t)auth_value, 0xAF, *auth_value_len);

		*st = rpc_s_ok;
		return;
	}

	*auth_value_len = output_token.length;
	*auth_len_remain = 0;

	memcpy((unsigned_char_p_t)auth_value,
	       output_token.value,
	       output_token.length);

	gss_release_buffer(&minor_status, &output_token);

	*st = rpc_s_ok;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_fmt_srvr_resp
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will format the auth_value field of
**      either an rpc_bind_ack or rpc_alter_context_response
**      PDU. The authentication protocol encoding in the
**      credentials field of auth_value should correspond
**      to the status returned from rpc__auth_cn_vfy_
**      client_req()  routine. This credentials field, when
**      decoded by rpc__auth_cn_vfy_srvr_resp(),  should
**      result in the same error code being returned. If the
**      memory provided is not large enough an authentication
**      protocol specific error message will be encoded in
**      the credentials field indicating this error. This is
**      called from the server side association state machine.
**
**  INPUTS:
**
**      verify_st       The status code returned by rpc__auth_cn_verify_
**                      client_req().
**      assoc_sec       A pointer to per-association security context
**                      including association UUID CRC and sequence numbers.
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**      req_auth_value  A pointer to the auth_value field in the
**                      rpc_bind or rpc_alter_context PDU authentication trailer.
**      req_auth_value_len The length, in bytes, of the
**                      req_auth_value field.
**      auth_value      A pointer to the auth_value field in the rpc_bind or
**                      rpc_alter_context PDU authentication trailer.
**
**  INPUTS/OUTPUTS:
**
**      auth_value_len  On input, the length, in bytes of the available space
**                      for the auth_value field. On output, the length in
**                      bytes used in encoding the auth_value field. Zero if
**                      an error status is returned.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_fmt_srvr_resp
(
	unsigned32			verify_st,
	rpc_cn_assoc_sec_context_p_t    assoc_sec,
	rpc_cn_sec_context_p_t		sec,
	pointer_t			req_auth_value ATTRIBUTE_UNUSED /*TODO*/,
	unsigned32			req_auth_value_len ATTRIBUTE_UNUSED /*TODO*/,
	pointer_t			auth_value,
	unsigned32			*auth_value_len
)
{
	OM_uint32 minor_status;
	gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_fmt_srvr_resp)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_fmt_srvr_resp) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x, vfy_client_st->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id,
		assoc_sec->assoc_uuid_crc,
		assoc_sec->assoc_next_snd_seq,
		assoc_sec->assoc_next_rcv_seq,
		verify_st));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_FMT_SERVER_RESP)) {
		verify_st = RPC_S_CN_DBG_AUTH_FAILURE;
	}
#endif

	/*
	 * transfer the output of rpc__gssauth_cn_vfy_client_req()
	 */
	output_token.length = assoc_sec->krb_message.length;
	output_token.value = assoc_sec->krb_message.data;
	assoc_sec->krb_message.length = 0;
	assoc_sec->krb_message.data = NULL;

	if (*auth_value_len < output_token.length) {
		gss_release_buffer(&minor_status, &output_token);
		*auth_value_len = 0;
		return;
	}

	*auth_value_len = output_token.length;

	memcpy((unsigned_char_p_t)auth_value,
	       output_token.value,
	       output_token.length);

	gss_release_buffer(&minor_status, &output_token);
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_free_prot_info
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will free an NCA Connection RPC auth
**      information rep.
**
**  INPUTS:
**
**      info            A pointer to the auth information rep structure
**                      containing RPC protocol indenpendent information.
**
**  INPUTS/OUTPUTS:
**
**      cn_info         A pointer to the RPC auth information rep structure
**                      containing NCA Connection specific
**                      information. NULL on output.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_free_prot_info
(
	rpc_auth_info_p_t	info,
	rpc_cn_auth_info_p_t	*cn_info
)
{
	rpc_gssauth_cn_info_p_t gssauth_cn_info = (rpc_gssauth_cn_info_p_t)*cn_info;
	OM_uint32 minor_status;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_free_prot_info)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_free_prot_info) prot->%x level->%x \n",
		info->authn_protocol,
		info->authn_level));

	if (gssauth_cn_info->gss_ctx != GSS_C_NO_CONTEXT) {
		gss_delete_sec_context(&minor_status,
				       &gssauth_cn_info->gss_ctx,
				       GSS_C_NO_BUFFER);
		gssauth_cn_info->gss_ctx = GSS_C_NO_CONTEXT;
	}

#ifdef DEBUG
	memset (gssauth_cn_info, 0, sizeof (rpc_gssauth_cn_info_p_t));
#endif

	RPC_MEM_FREE (gssauth_cn_info, RPC_C_MEM_GSSAUTH_CN_INFO);
	*cn_info = NULL;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_get_prot_info
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will create and return an NCA Connection
**      RPC auth information rep.
**
**  INPUTS:
**
**      info            A pointer to the auth information rep structure
**                      containing RPC protocol indenpendent information.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      cn_info         A pointer to the RPC auth information rep structure
**                      containing NCA Connection specific information.
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_get_prot_info
(
	rpc_auth_info_p_t		info,
	rpc_cn_auth_info_p_t		*cn_info,
	unsigned32			*st
)
{
        rpc_gssauth_cn_info_t   *gssauth_cn_info = NULL;
	rpc_gssauth_info_t      *gssauth_info = (rpc_gssauth_info_p_t)info;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_get_prot_info)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_get_prot_info) prot->%x level->%x \n",
		info->authn_protocol,
		info->authn_level));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_GET_PROT_INFO)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif

	/*
	 * Allocate storage for a gssauth cn info structure from heap.
	 */
	RPC_MEM_ALLOC(gssauth_cn_info,
		      rpc_gssauth_cn_info_p_t,
		      sizeof (rpc_gssauth_cn_info_t),
		      RPC_C_MEM_GSSAUTH_CN_INFO,
		      RPC_C_MEM_WAITOK);

	/*
	 * Initialize it.
	 */
	memset (gssauth_cn_info, 0, sizeof(rpc_gssauth_cn_info_t));

	/*
	 * Set pointer to connection specific auth info if we're
	 * on the server side.
	 */
	if (info->is_server) {
		gssauth_info->cn_info = gssauth_cn_info;
	}

	*cn_info = &gssauth_cn_info->cn_info;
	*st = rpc_s_ok;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_pre_call
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will format the auth_value field of
**      a call level PDU, namely an rpc_request, rpc_response
**      rpc_fault, rpc_remote_alert or rpc_orphaned PDU. It will
**      also format the auth_value field of the association level
**      rpc_shutdown PDU. This does
**      not include generating any checksums in the auth_value_field
**      or encrypting of data corresponding to the authentication
**      level. That will be done in the rpc__cn_auth_pre_send
**      routine. This is called on both client and server when a the
**      data structures and header template for a call is being set up.
**
**  INPUTS:
**
**      assoc_sec       A pointer to per-association security context
**                      including association UUID CRC and sequence numbers.
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**      auth_value      A pointer to the auth_value field in the rpc_bind or
**                      rpc_alter_context PDU authentication trailer.
**
**  INPUTS/OUTPUTS:
**
**      auth_value_len  On input, the lenght, in bytes of the available space
**                      for the auth_value field. On output, the lenght in
**                      bytes used in encoding the auth_value field. Zero if
**                      an error status is returned.
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_pre_call
(
	rpc_cn_assoc_sec_context_p_t	assoc_sec,
	rpc_cn_sec_context_p_t		sec,
	pointer_t			auth_value ATTRIBUTE_UNUSED /*TODO*/,
	unsigned32			*auth_value_len ATTRIBUTE_UNUSED /*TODO*/,
	unsigned32			*st
)
{
	CODING_ERROR(st);

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_pre_call)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_pre_call) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id,
		assoc_sec->assoc_uuid_crc,
		assoc_sec->assoc_next_snd_seq,
		assoc_sec->assoc_next_rcv_seq));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_PRE_CALL)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif

	if (sec->sec_info->authn_level == rpc_c_authn_level_connect) {
		*auth_value_len = 0;
	} else {
		*auth_value_len = RPC__GSSAUTH_CN_AUTH_MAX_LEN;
	}
	*st = rpc_s_ok;
}

INTERNAL void rpc__gssauth_cn_get_packet_mic
(
	rpc_cn_sec_context_p_t		sec,
	const char			*comment,
	unsigned8			header_size,
	rpc_socket_iovec_p_t		iov,
	unsigned32			iovlen,
	rpc_socket_iovec_p_t		out_iov,
	unsigned32			*st
)
{
	rpc_gssauth_cn_info_p_t gssauth_cn_info = (rpc_gssauth_cn_info_p_t)sec->sec_cn_info;
	rpc_gssauth_info_p_t gssauth_info = (rpc_gssauth_info_p_t)sec->sec_info;
	rpc_cn_common_hdr_p_t hdr = NULL;
	rpc_cn_auth_tlr_p_t tlr = NULL;
	unsigned32 i = 0;
	unsigned32 pdu_iov_num = iovlen - 1;
	unsigned32 auth_iov_idx = iovlen - 1;
	unsigned32 tlr_iov_idx = iovlen - 1;
	unsigned32 pdu_len = 0;
	unsigned8 auth_pad_len = 0;
	unsigned32 wrap_len = 0;
	unsigned32 wrap_idx = 0;
	unsigned_char_p_t wrap_base = NULL;
	OM_uint32 maj_stat = 0;
	OM_uint32 min_stat = 0;
	gss_buffer_desc sign_buffer = {0};
	gss_buffer_desc signature = {0};

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_get_packet_mic)\n"));

	if (iovlen < 2) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_get_packet_mic): %s: iovlen[%u]\n",
			comment, (unsigned int)iovlen));
		*st = rpc_m_no_stub_data;
		return;
	}

	if (iov[0].iov_len < header_size) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_get_packet_mic): %s: iov[0].iov_len[%u] < header_size[%u]\n",
			comment, (unsigned int)iov[0].iov_len,(unsigned int)header_size));
		*st = rpc_m_no_stub_data;
		return;
	}

	if (iov[auth_iov_idx].iov_len != RPC_CN_PKT_SIZEOF_COM_AUTH_TLR+RPC__GSSAUTH_CN_AUTH_MAX_LEN) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_get_packet_mic): %s: iov[%u].iov_len[%u] != %u\n",
			comment,(unsigned int)auth_iov_idx,
			(unsigned int)iov[auth_iov_idx].iov_len,
			RPC_CN_PKT_SIZEOF_COM_AUTH_TLR));
		*st = rpc_s_auth_field_toolong;
		return;
	}

	if (iov[tlr_iov_idx].iov_len < RPC_CN_PKT_SIZEOF_COM_AUTH_TLR)
	{
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_get_packet_mic): %s: iov[%u].iov_len[%u] < %u\n",
			comment,(unsigned int)tlr_iov_idx,
			(unsigned int)iov[tlr_iov_idx].iov_len,
			RPC_CN_PKT_SIZEOF_COM_AUTH_TLR));
		*st = rpc_s_auth_field_toolong;
		return;
	}

	/*
	 * Calculate padding length
	 */
	for (i = 0; i < pdu_iov_num; i++)
	{
		pdu_len += iov[i].iov_len;
	}

	pdu_len      -= header_size;
	auth_pad_len  = 16 - (pdu_len % 16);

	/*
	 * Prepare wrapping for the whole packet (copy iov blobs
	 * - all except the last one)
	 */
	wrap_len = header_size + pdu_len + auth_pad_len +
		   RPC_CN_PKT_SIZEOF_COM_AUTH_TLR + RPC__GSSAUTH_CN_AUTH_MAX_LEN;

	RPC_MEM_ALLOC(wrap_base,
		      unsigned_char_p_t,
		      wrap_len,
		      RPC_C_MEM_CN_ENCRYPT_BUF,
		      RPC_C_MEM_WAITOK);

	wrap_idx = 0;
	hdr = (rpc_cn_common_hdr_p_t)iov[0].iov_base;

	hdr->auth_len = RPC_CN_PKT_SIZEOF_GSSGETMIC_AUTH_TLR;
	hdr->frag_len = RPC_CN_PKT_SIZEOF_RESP_HDR +
                        pdu_len + auth_pad_len +
                        RPC_CN_PKT_SIZEOF_COM_AUTH_TLR +
                        hdr->auth_len;

	tlr = (rpc_cn_auth_tlr_p_t)iov[tlr_iov_idx].iov_base;

	tlr->stub_pad_length = auth_pad_len;

	/* Copy the DCE/RPC header and the PDU */
	for (i = 0; i < pdu_iov_num; i++)
	{
		memcpy(&wrap_base[wrap_idx],
		       iov[i].iov_base,
		       iov[i].iov_len);
		wrap_idx += iov[i].iov_len;
	}

	/* Ensure padding bytes zeroed */
	memset(&wrap_base[wrap_idx], 0, auth_pad_len);
	wrap_idx += auth_pad_len;

	/* Copy the DCE/RPC trailer (the common part) because it may
	   be needed in case header signing has been negotiated */
	memcpy(&wrap_base[wrap_idx],
	       iov[tlr_iov_idx].iov_base,
	       RPC_CN_PKT_SIZEOF_COM_AUTH_TLR);
	wrap_idx += RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;


	if (gssauth_info->auth_info.authn_flags &&
	    rpc_c_protect_flags_header_sign)
	{
		sign_buffer.length = wrap_idx;
		sign_buffer.value  = wrap_base;
	}
	else
	{
		sign_buffer.length = pdu_len + auth_pad_len;
		sign_buffer.value  = &(wrap_base[header_size]);
	}

	maj_stat = gss_get_mic(&min_stat,
			       gssauth_cn_info->gss_ctx,
			       GSS_C_QOP_DEFAULT,
			       &sign_buffer,
			       &signature);
	if (maj_stat != GSS_S_COMPLETE) {
		char msg[256];
		rpc__gssauth_error_map(maj_stat, min_stat,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_get_packet_mic): %s: %s\n", comment, msg));
		/* *st is already filled */
		goto cleanup;
	}

	if (signature.length > RPC__GSSAUTH_CN_AUTH_MAX_LEN) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_get_packet_mic): %s: auth_len[%u] > auth_max_len[%u]\n",
			comment, (unsigned int)signature.length, (unsigned int)RPC__GSSAUTH_CN_AUTH_MAX_LEN));
		*st = rpc_s_auth_method;
		goto cleanup;
	}

	/* GSS-API signature */
	memcpy(&wrap_base[wrap_idx],
	       signature.value,
	       signature.length);
	wrap_idx += signature.length;

	/* set the out_iov blob to be sent */
	out_iov->iov_base = wrap_base;
	out_iov->iov_len  = wrap_idx;

	*st = rpc_s_ok;

cleanup:
	maj_stat = gss_release_buffer(&min_stat,
				      &signature);
	if (maj_stat != GSS_S_COMPLETE) {
		char msg[256];
		rpc__gssauth_error_map(maj_stat, min_stat,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_get_packet_mic): %s: %s\n", comment, msg));
		/* *st is already filled */
	}

	return;
}

INTERNAL void rpc__gssauth_cn_wrap_packet
(
	rpc_cn_sec_context_p_t		sec,
	const char			*comment,
	unsigned8			header_size,
	rpc_socket_iovec_p_t		iov,
	unsigned32			iovlen,
	rpc_socket_iovec_p_t		out_iov,
	unsigned32			*st
)
{
	rpc_gssauth_cn_info_p_t gssauth_cn_info = (rpc_gssauth_cn_info_p_t)sec->sec_cn_info;
	rpc_gssauth_info_p_t gssauth_info = (rpc_gssauth_info_p_t)sec->sec_info;
	rpc_cn_common_hdr_p_t hdr = NULL;
	rpc_cn_auth_tlr_p_t tlr = NULL;
	unsigned32 i = 0;
	unsigned32 pdu_iov_num = iovlen - 1;
	unsigned32 auth_iov_idx = iovlen - 1;
	unsigned32 tlr_iov_idx = iovlen - 1;
	unsigned32 pdu_len = 0;
	unsigned8 auth_pad_len = 0;
	unsigned16 auth_len = 0;
	unsigned32 wrap_len = 0;
	unsigned32 wrap_idx = 0;
	unsigned_char_p_t wrap_base = NULL;
	OM_uint32 maj_stat = 0;
	OM_uint32 min_stat = 0;
	int conf_req = 1;
 	int conf_state = 0;
	gss_iov_buffer_desc output_iov[4] = { {0}, {0}, {0}, {0} };
	gss_buffer_desc gssauth_buffer = {0};
	unsigned32 gss_buffer_idx = 0;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_wrap_packet)\n"));

	if (iovlen < 2) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: iovlen[%u]\n",
			comment, (unsigned int)iovlen));
		*st = rpc_m_no_stub_data;
		return;
	}

	if (iov[0].iov_len < header_size) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: iov[0].iov_len[%u] < header_size[%u]\n",
			comment, (unsigned int)iov[0].iov_len,(unsigned int)header_size));
		*st = rpc_m_no_stub_data;
		return;
	}

	if (iov[auth_iov_idx].iov_len != RPC_CN_PKT_SIZEOF_COM_AUTH_TLR+RPC__GSSAUTH_CN_AUTH_MAX_LEN) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: iov[%u].iov_len[%u] != %u\n",
			comment,(unsigned int)auth_iov_idx,
			(unsigned int)iov[auth_iov_idx].iov_len,
			RPC_CN_PKT_SIZEOF_COM_AUTH_TLR));
		*st = rpc_s_auth_field_toolong;
		return;
	}

	if (iov[tlr_iov_idx].iov_len < RPC_CN_PKT_SIZEOF_COM_AUTH_TLR)
	{
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: iov[%u].iov_len[%u] < %u\n",
			comment,(unsigned int)tlr_iov_idx,
			(unsigned int)iov[tlr_iov_idx].iov_len,
			RPC_CN_PKT_SIZEOF_COM_AUTH_TLR));
		*st = rpc_s_auth_field_toolong;
		return;
	}

	/*
	 * Calculate padding length
	 */
	for (i = 0; i < pdu_iov_num; i++)
	{
		pdu_len += iov[i].iov_len;
	}

	pdu_len      -= header_size;
	auth_pad_len  = 16 - (pdu_len % 16);

	/*
	 * Prepare wrapping for the whole packet (copy iov blobs
	 * - all except the last one)
	 */
	wrap_len = header_size + pdu_len + auth_pad_len +
		   RPC_CN_PKT_SIZEOF_COM_AUTH_TLR + RPC__GSSAUTH_CN_AUTH_MAX_LEN;

	RPC_MEM_ALLOC(wrap_base,
		      unsigned_char_p_t,
		      wrap_len,
		      RPC_C_MEM_CN_ENCRYPT_BUF,
		      RPC_C_MEM_WAITOK);

	wrap_idx = 0;
	tlr = (rpc_cn_auth_tlr_p_t)iov[tlr_iov_idx].iov_base;

	tlr->stub_pad_length = auth_pad_len;

	/* Copy the DCE/RPC header and the PDU */
	for (i = 0; i < pdu_iov_num; i++)
	{
		memcpy(&wrap_base[wrap_idx],
		       iov[i].iov_base,
		       iov[i].iov_len);
		wrap_idx += iov[i].iov_len;
	}
	hdr = (rpc_cn_common_hdr_p_t)wrap_base;

	/* Ensure padding bytes zeroed */
	memset(&wrap_base[wrap_idx], 0, auth_pad_len);
	wrap_idx += auth_pad_len;

	/* Copy the DCE/RPC trailer (the common part) because it may
	   be needed in case header signing has been negotiated */
	memcpy(&wrap_base[wrap_idx],
	       iov[tlr_iov_idx].iov_base,
	       RPC_CN_PKT_SIZEOF_COM_AUTH_TLR);
	wrap_idx += RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;

	/* GSS-API header to be generated */
	output_iov[gss_buffer_idx++].type = GSS_IOV_BUFFER_TYPE_HEADER |
                                            GSS_IOV_BUFFER_FLAG_ALLOCATE;

	if (gssauth_info->auth_info.authn_flags &&
	    rpc_c_protect_flags_header_sign)
	{
		/* DCE/RPC header (if header signing has been enabled) */
		output_iov[gss_buffer_idx].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
		output_iov[gss_buffer_idx].buffer.value  = &(wrap_base[0]);
		output_iov[gss_buffer_idx].buffer.length = header_size;

		gss_buffer_idx++;
	}
	
	/* RPC PDU (for signing and sealing) */
	output_iov[gss_buffer_idx].type          = GSS_IOV_BUFFER_TYPE_DATA;
	output_iov[gss_buffer_idx].buffer.value  = &(wrap_base[header_size]);
	output_iov[gss_buffer_idx].buffer.length = pdu_len + auth_pad_len;

	gss_buffer_idx++;

	if (gssauth_info->auth_info.authn_flags &&
	    rpc_c_protect_flags_header_sign)
	{
		/* DCE/RPC common part of auth trailer
		   (if header signing has been enabled) */
		output_iov[gss_buffer_idx].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
		output_iov[gss_buffer_idx].buffer.value  = &(wrap_base[header_size + pdu_len + auth_pad_len]);
		output_iov[gss_buffer_idx].buffer.length = RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;

		gss_buffer_idx++;
	}

	// First determine how long the gss header will be
	maj_stat = gss_wrap_iov_length(&min_stat,
				gssauth_cn_info->gss_ctx,
				conf_req,
				GSS_C_QOP_DEFAULT,
				&conf_state,
				output_iov,
				sizeof(output_iov)/sizeof(output_iov[0]));
	if (maj_stat != GSS_S_COMPLETE) {
		char msg[256];
		rpc__gssauth_error_map(maj_stat, min_stat,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: %s\n", comment, msg));
		/* *st is already filled */
		goto cleanup;
	}

	if (conf_req != conf_state) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: conf_req[%u] != conf_state[%u]\n",
			comment, (unsigned int)conf_req, (unsigned int)conf_state));
		*st = rpc_s_auth_method;
		goto cleanup;
	}

	gssauth_buffer = output_iov[0].buffer;
	auth_len       = gssauth_buffer.length;

	// Fill in the correct fragment size and auth size values before doing
	// the real encryption.
	hdr->auth_len = auth_len;
	hdr->frag_len = header_size +
                        pdu_len + auth_pad_len +
                        RPC_CN_PKT_SIZEOF_COM_AUTH_TLR +
                        hdr->auth_len;


	maj_stat = gss_wrap_iov(&min_stat,
				gssauth_cn_info->gss_ctx,
				conf_req,
				GSS_C_QOP_DEFAULT,
				&conf_state,
				output_iov,
				sizeof(output_iov)/sizeof(output_iov[0]));
	if (maj_stat != GSS_S_COMPLETE) {
		char msg[256];
		rpc__gssauth_error_map(maj_stat, min_stat,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: %s\n", comment, msg));
		/* *st is already filled */
		goto cleanup;
	}

	if (conf_req != conf_state) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: conf_req[%u] != conf_state[%u]\n",
			comment, (unsigned int)conf_req, (unsigned int)conf_state));
		*st = rpc_s_auth_method;
		goto cleanup;
	}

	gssauth_buffer = output_iov[0].buffer;
	if (gssauth_buffer.length != auth_len)
	{
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: GSS auth header size was %u from gss_wrap_iov_length and is now %u from gss_wrap_iov\n",
			comment, (unsigned int)auth_len, (unsigned int)gssauth_buffer.length));
		*st = rpc_s_auth_method;
		goto cleanup;
	}

	if (auth_len > RPC__GSSAUTH_CN_AUTH_MAX_LEN) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: auth_len[%u] > auth_max_len[%u]\n",
			comment, (unsigned int)auth_len, (unsigned int)RPC__GSSAUTH_CN_AUTH_MAX_LEN));
		*st = rpc_s_auth_method;
		goto cleanup;
	}

	/* GSS-API trailer (yes, gss_wrap_iov auth header is in fact a trailer in DCE/RPC) */
	memcpy(&wrap_base[wrap_idx],
	       gssauth_buffer.value,
	       gssauth_buffer.length);
	wrap_idx += gssauth_buffer.length;

	/* set the out_iov blob to be sent */
	out_iov->iov_base = wrap_base;
	out_iov->iov_len  = wrap_idx;

	*st = rpc_s_ok;

cleanup:
	maj_stat = gss_release_iov_buffer(&min_stat,
					  output_iov,
					  sizeof(output_iov)/sizeof(output_iov[0]));
	if (maj_stat != GSS_S_COMPLETE) {
		char msg[256];
		rpc__gssauth_error_map(maj_stat, min_stat,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_wrap_packet): %s: %s\n", comment, msg));
		/* *st is already filled */
	}

	return;
}

INTERNAL void rpc__gssauth_cn_create_large_frag
(
	rpc_cn_assoc_sec_context_p_t	assoc_sec,
	const char			*comment,
	unsigned8			header_size,
	rpc_socket_iovec_p_t		iov,
	unsigned32			iovlen,
	rpc_socket_iovec_p_t		out_iov,
	unsigned32			*st
)
{
	rpc_cn_common_hdr_p_t hdr;
	unsigned32 i;
	unsigned32 min_len = 0;
	unsigned32 wrap_len = 0;
	unsigned32 wrap_idx = 0;
	unsigned_char_p_t wrap_base;
	OM_uint32 min_stat;
	gss_buffer_desc output_token;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_create_large_frag)\n"));

	output_token.length = assoc_sec->krb_message.length;
	output_token.value = assoc_sec->krb_message.data;
	assoc_sec->krb_message.length = 0;
	assoc_sec->krb_message.data = NULL;

	if (iovlen < 1) {
		gss_release_buffer(&min_stat, &output_token);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_create_large_frag): %s: iovlen[%u]\n",
			comment, (unsigned int)iovlen));
		*st = rpc_m_no_stub_data;
		return;
	}
	if (iov[0].iov_len < header_size) {
		gss_release_buffer(&min_stat, &output_token);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_create_large_frag): %s: iov[0].iov_len[%u] < header_size[%u]\n",
			comment, (unsigned int)iov[0].iov_len,(unsigned int)header_size));
		*st = rpc_m_no_stub_data;
		return;
	}

	wrap_len = 0;
	for (i=0; i < iovlen; i++) {
		wrap_len += iov[i].iov_len;
	}

	min_len = header_size + RPC_CN_PKT_SIZEOF_COM_AUTH_TLR + RPC__GSSAUTH_CN_AUTH_MAX_LEN;

	if (wrap_len < min_len) {
		gss_release_buffer(&min_stat, &output_token);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_create_large_frag): %s: wrap_len[%u] < min_len[%u]\n",
			comment, (unsigned int)wrap_len,(unsigned int)min_len));
		*st = rpc_m_no_stub_data;
		return;
	}

	wrap_len -= RPC__GSSAUTH_CN_AUTH_MAX_LEN;
	wrap_len += output_token.length;

	RPC_MEM_ALLOC(wrap_base,
		      unsigned_char_p_t,
		      wrap_len,
		      RPC_C_MEM_CN_ENCRYPT_BUF,
		      RPC_C_MEM_WAITOK);

	wrap_idx = 0;
	hdr = (rpc_cn_common_hdr_p_t)&wrap_base[wrap_idx];
	for (i=0; i < iovlen; i++) {
		memcpy(&wrap_base[wrap_idx], iov[i].iov_base, iov[i].iov_len);
		wrap_idx += iov[i].iov_len;
	}

	wrap_idx -= RPC__GSSAUTH_CN_AUTH_MAX_LEN;

	memcpy(&wrap_base[wrap_idx], output_token.value, output_token.length);
	wrap_idx += output_token.length;

	hdr->frag_len = wrap_idx;
	hdr->auth_len = output_token.length;

	out_iov->iov_base = wrap_base;
	out_iov->iov_len = wrap_len;

	gss_release_buffer(&min_stat, &output_token);

	*st = rpc_s_ok;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_pre_send
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will perform per-packet security
**      processing on a packet before it is sent. This
**      includes checksumming and encryption.
**
**      Note that in some cases, the data is copied to
**      a contiguous buffer for checksumming and
**      encryption.  In these cases, the contiguous
**      iov element should be used instead of the original
**      iovector.
**
**  INPUTS:
**
**      assoc_sec       A pointer to per-association security context
**                      including association UUID CRC and sequence numbers.
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**      iov             A pointer to the iovector containing the PDU
**                      about to be sent. The appropriate per-packet security
**                      services will be applied to it.
**      iovlen          The length, in bytes, of the PDU.
**      out_iov         An iovector element.  This iovector element
**                      will describe packet if the original iov
**                      had to be copied.  If the original iov was
**                      copied, out_iov->iov_base will be non-NULL.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_pre_send
(
	rpc_cn_assoc_sec_context_p_t	assoc_sec,
	rpc_cn_sec_context_p_t		sec,
	rpc_socket_iovec_p_t		iov,
	unsigned32			iovlen,
	rpc_socket_iovec_p_t		out_iov,
	unsigned32			*st
)
{
	unsigned32 ptype = 0;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_pre_send)\n"));

	ptype = ((rpc_cn_common_hdr_t *)(iov[0].iov_base))->ptype;
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
		("(rpc__gssauth_cn_pre_send) authn level->%x packet type->%x\n",
		sec->sec_info->authn_level, ptype));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_pre_send) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x ptype->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id,
		assoc_sec->assoc_uuid_crc,
		assoc_sec->assoc_next_snd_seq,
		assoc_sec->assoc_next_rcv_seq,
		ptype));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_PRE_SEND)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif

	out_iov->iov_base = NULL;

	switch (ptype) {
	case RPC_C_CN_PKT_REQUEST:
		if (sec->sec_info->authn_level == rpc_c_authn_level_connect) {
			*st = rpc_s_ok;
			return;
		}

		if (sec->sec_info->authn_level == rpc_c_authn_level_pkt_integrity)
		{
			rpc__gssauth_cn_get_packet_mic(sec, "request", RPC_CN_PKT_SIZEOF_RQST_HDR,
						       iov, iovlen, out_iov, st);
		}
		else if (sec->sec_info->authn_level == rpc_c_authn_level_pkt_privacy)
		{
			rpc__gssauth_cn_wrap_packet(sec, "request", RPC_CN_PKT_SIZEOF_RQST_HDR,
						    iov, iovlen, out_iov, st);
		}

		return;

	case RPC_C_CN_PKT_RESPONSE:
		if (sec->sec_info->authn_level == rpc_c_authn_level_connect) {
			*st = rpc_s_ok;
			return;
		}

		if (sec->sec_info->authn_level == rpc_c_authn_level_pkt_integrity)
		{
			rpc__gssauth_cn_get_packet_mic(sec, "response", RPC_CN_PKT_SIZEOF_RQST_HDR,
						       iov, iovlen, out_iov, st);
		}
		else if (sec->sec_info->authn_level == rpc_c_authn_level_pkt_privacy)
		{
			rpc__gssauth_cn_wrap_packet(sec, "response", RPC_CN_PKT_SIZEOF_RESP_HDR,
						    iov, iovlen, out_iov, st);
		}

		return;

	case RPC_C_CN_PKT_BIND:
		if (assoc_sec->krb_message.length == 0) {
			*st = rpc_s_ok;
			return;
		}

		rpc__gssauth_cn_create_large_frag(assoc_sec, "bind",
						  RPC_CN_PKT_SIZEOF_BIND_HDR,
						  iov, iovlen, out_iov, st);
		return;

	case RPC_C_CN_PKT_ALTER_CONTEXT:
		if (assoc_sec->krb_message.length == 0) {
			*st = rpc_s_ok;
			return;
		}

		rpc__gssauth_cn_create_large_frag(assoc_sec, "alter_context",
						  RPC_CN_PKT_SIZEOF_ALT_CTX_HDR,
						  iov, iovlen, out_iov, st);
		return;

	case RPC_C_CN_PKT_FAULT:
	case RPC_C_CN_PKT_BIND_ACK:
	case RPC_C_CN_PKT_BIND_NAK:
	case RPC_C_CN_PKT_ALTER_CONTEXT_RESP:
	case RPC_C_CN_PKT_AUTH3:
	case RPC_C_CN_PKT_SHUTDOWN:
	case RPC_C_CN_PKT_REMOTE_ALERT:
	case RPC_C_CN_PKT_ORPHANED:
	default:
		break;
	}

	*st = rpc_s_ok;
}

INTERNAL void rpc__gssauth_cn_verify_packet_mic
(
	rpc_cn_sec_context_p_t		sec,
	const char			*comment,
	unsigned8			header_size,
	rpc_cn_common_hdr_p_t		pkt,
	unsigned32			pkt_len,
	unsigned32			cred_len ATTRIBUTE_UNUSED /*TODO*/,
	rpc_cn_auth_tlr_p_t		auth_tlr,
	boolean32			unpack_ints,
	unsigned32			*st
)
{
	rpc_gssauth_cn_info_p_t gssauth_cn_info = (rpc_gssauth_cn_info_p_t)sec->sec_cn_info;
	rpc_gssauth_info_p_t gssauth_info = (rpc_gssauth_info_p_t)sec->sec_info;
	OM_uint32 maj_stat = 0;
	OM_uint32 min_stat = 0;
	gss_qop_t qop_state = 0;
	unsigned_char_p_t pdu_base = NULL;
	unsigned32 pdu_len = 0;
	unsigned_char_p_t gssauth_base = NULL;
	unsigned16 gssauth_len = 0;
	unsigned8 auth_pad_len = 0;
	gss_buffer_desc verify_buffer = {0};
	gss_buffer_desc gssauth_buffer = {0};

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_verify_packet_mic)\n"));

	gssauth_len = pkt->auth_len;
	if (unpack_ints)
        {
		SWAB_INPLACE_16(gssauth_len);
	}

	/* Get rpc payload and gss trailer pointers */
	auth_pad_len = auth_tlr->stub_pad_length;
	if (unpack_ints)
	{
		SWAB_INPLACE_16(auth_pad_len);
	}

	pdu_base     = ((unsigned_char_p_t)pkt) + header_size;
	pdu_len      = pkt_len - (header_size + RPC_CN_PKT_SIZEOF_COM_AUTH_TLR + gssauth_len + auth_pad_len);
	gssauth_base = ((unsigned_char_p_t)auth_tlr) + RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;

	if (gssauth_info->auth_info.authn_flags &&
	    rpc_c_protect_flags_header_sign)
	{
		verify_buffer.length = pkt_len - gssauth_len;
		verify_buffer.value  = pkt;
	}
	else
	{
		verify_buffer.length = pdu_len + auth_pad_len;
		verify_buffer.value  = pdu_base;
	}

	/* GSS auth trailer */
	gssauth_buffer.value   = gssauth_base;
	gssauth_buffer.length  = gssauth_len;

	maj_stat = gss_verify_mic(&min_stat,
				  gssauth_cn_info->gss_ctx,
				  &verify_buffer,
				  &gssauth_buffer,
				  &qop_state);
	if (maj_stat != GSS_S_COMPLETE) {
		char msg[256];
		rpc__gssauth_error_map(maj_stat, min_stat,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_verify_packet_mic): %s: %s\n",
			comment, msg));
		/* *st is already filled */
		goto cleanup;
	}

	*st = rpc_s_ok;

cleanup:
	return;
}

INTERNAL void rpc__gssauth_cn_unwrap_packet
(
	rpc_cn_sec_context_p_t		sec,
	const char			*comment,
	unsigned8			header_size,
	rpc_cn_common_hdr_p_t		pkt,
	unsigned32			pkt_len,
	unsigned32			cred_len ATTRIBUTE_UNUSED /*TODO*/,
	rpc_cn_auth_tlr_p_t		auth_tlr,
	boolean32			unpack_ints,
	unsigned32			*st
)
{
	rpc_gssauth_cn_info_p_t gssauth_cn_info = (rpc_gssauth_cn_info_p_t)sec->sec_cn_info;
	rpc_gssauth_info_p_t gssauth_info = (rpc_gssauth_info_p_t)sec->sec_info;
	OM_uint32 maj_stat = 0;
	OM_uint32 min_stat = 0;
	int sealed = 1;          /* packet is expected to be sealed */
	int conf_state = 0;
	gss_qop_t qop_state = 0;
	unsigned_char_p_t pdu_base = NULL;
	unsigned32 pdu_len = 0;
	unsigned_char_p_t gssauth_base = NULL;
	unsigned16 gssauth_len = 0;
	unsigned8 auth_pad_len = 0;
	gss_iov_buffer_desc output_iov[4] = {{0}, {0}, {0}, {0}};
	gss_buffer_desc hdr_buffer = {0};
	gss_buffer_desc pdu_buffer = {0};
	gss_buffer_desc tlr_buffer = {0};
	gss_buffer_desc gssauth_buffer = {0};
	unsigned32 gss_buffer_idx = 0;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_unwrap_packet)\n"));

	gssauth_len = pkt->auth_len;
	if (unpack_ints)
        {
		SWAB_INPLACE_16(gssauth_len);
	}

	/* Get rpc payload and gss trailer pointers */
	auth_pad_len = auth_tlr->stub_pad_length;
	if (unpack_ints)
	{
		SWAB_INPLACE_16(auth_pad_len);
	}

	pdu_base     = ((unsigned_char_p_t)pkt) + header_size;
	pdu_len      = pkt_len - (header_size + RPC_CN_PKT_SIZEOF_COM_AUTH_TLR + gssauth_len + auth_pad_len);
	gssauth_base = ((unsigned_char_p_t)auth_tlr) + RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;

	/* DCE/RPC header (for signing check if enabled) */
	hdr_buffer.value       = pkt;
	hdr_buffer.length      = header_size;

	/* RPC PDU (for signing check and decryption) */
	pdu_buffer.value       = pdu_base;
	pdu_buffer.length      = pdu_len + auth_pad_len;

	/* DCE/RPC common part of trailer (for signing check if enabled) */
	tlr_buffer.value       = auth_tlr;
	tlr_buffer.length      = RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;

	/* GSS trailer which is a GSS auth header here */
	gssauth_buffer.value   = gssauth_base;
	gssauth_buffer.length  = gssauth_len;

	/* Prepare output_iov buffers to be verified and decrypted */
	output_iov[gss_buffer_idx].type   = GSS_IOV_BUFFER_TYPE_HEADER;
	output_iov[gss_buffer_idx].buffer = gssauth_buffer;

	gss_buffer_idx++;

	if (gssauth_info->auth_info.authn_flags &&
	    rpc_c_protect_flags_header_sign)
	{
		output_iov[gss_buffer_idx].type   = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
		output_iov[gss_buffer_idx].buffer = hdr_buffer;

		gss_buffer_idx++;
	}

	output_iov[gss_buffer_idx].type   = GSS_IOV_BUFFER_TYPE_DATA;
	output_iov[gss_buffer_idx].buffer = pdu_buffer;

	gss_buffer_idx++;

	if (gssauth_info->auth_info.authn_flags &&
	    rpc_c_protect_flags_header_sign)
	{
		output_iov[gss_buffer_idx].type   = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
		output_iov[gss_buffer_idx].buffer = tlr_buffer;

		gss_buffer_idx++;
	}

	maj_stat = gss_unwrap_iov(&min_stat,
				  gssauth_cn_info->gss_ctx,
				  &conf_state,
				  &qop_state,
				  output_iov,
				  sizeof(output_iov)/sizeof(output_iov[0]));
	if (maj_stat != GSS_S_COMPLETE) {
		char msg[256];
		rpc__gssauth_error_map(maj_stat, min_stat,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_unwrap_packet): %s: %s\n",
			comment, msg));
		/* *st is already filled */
		goto cleanup;
	}

	if (conf_state == 0 && sealed) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_unwrap_packet): %s: packet signed should be also sealed\n",
			comment));
		*st = rpc_s_auth_bad_integrity;
		goto cleanup;

	} else if (conf_state == 1 && !sealed) {
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_unwrap_packet): %s: packet sealed while expected to be signed\n",
			comment));
		*st = rpc_s_auth_bad_integrity;
		goto cleanup;
	}

	*st = rpc_s_ok;

cleanup:
	return;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_recv_check
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will perform per-packet security
**      processing on a packet after it is received. This
**      includes decryption and verification of checksums.
**
**  INPUTS:
**
**      assoc_sec       A pointer to per-association security context
**                      including association UUID CRC and sequence numbers.
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**      pdu             A pointer to the PDU about to be sent. The appropriate
**                      per-packet security services will be applied to it.
**      pdu_len         The length, in bytes, of the PDU.
**      cred_len        The length, in bytes, of the credentials.
**      auth_tlr        A pointer to the auth trailer.
**      unpack_ints     A boolean indicating whether the integer rep
**                      of fields in the pdu need to be adjusted for
**                      endian differences.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_recv_check
(
	rpc_cn_assoc_sec_context_p_t    assoc_sec,
	rpc_cn_sec_context_p_t		sec,
	rpc_cn_common_hdr_p_t		pdu,
	unsigned32			pdu_len,
	unsigned32			cred_len,
	rpc_cn_auth_tlr_p_t		auth_tlr,
	boolean32			unpack_ints,
	unsigned32			*st
)
{
	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_recv_check)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
		("(rpc__gssauth_cn_recv_check) authn level->%x packet type->%x\n",
		sec->sec_info->authn_level,
		pdu->ptype));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_recv_check) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x ptype->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id,
		assoc_sec->assoc_uuid_crc,
		assoc_sec->assoc_next_snd_seq,
		assoc_sec->assoc_next_rcv_seq,
		pdu->ptype));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_RECV_CHECK)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif

	switch (pdu->ptype) {
	case RPC_C_CN_PKT_REQUEST:
		if (sec->sec_info->authn_level == rpc_c_authn_level_connect)
		{
			*st = rpc_s_ok;
			return;
		}

		if (sec->sec_info->authn_level == rpc_c_authn_level_pkt_integrity)
		{
			rpc__gssauth_cn_verify_packet_mic(sec, "request", RPC_CN_PKT_SIZEOF_RQST_HDR,
							  pdu, pdu_len, cred_len, auth_tlr,
							  unpack_ints, st);
		}
		else if (sec->sec_info->authn_level == rpc_c_authn_level_pkt_privacy)
		{
			rpc__gssauth_cn_unwrap_packet(sec, "request", RPC_CN_PKT_SIZEOF_RQST_HDR,
						      pdu, pdu_len, cred_len, auth_tlr,
						      unpack_ints, st);
		}
		return;

	case RPC_C_CN_PKT_RESPONSE:
		if (sec->sec_info->authn_level == rpc_c_authn_level_connect)
		{
			*st = rpc_s_ok;
			return;
		}

		if (sec->sec_info->authn_level == rpc_c_authn_level_pkt_integrity)
		{
			rpc__gssauth_cn_verify_packet_mic(sec, "response", RPC_CN_PKT_SIZEOF_RQST_HDR,
							  pdu, pdu_len, cred_len, auth_tlr,
							  unpack_ints, st);
		}
		else if (sec->sec_info->authn_level == rpc_c_authn_level_pkt_privacy)
		{
			rpc__gssauth_cn_unwrap_packet(sec, "response", RPC_CN_PKT_SIZEOF_RQST_HDR,
						      pdu, pdu_len, cred_len, auth_tlr,
						      unpack_ints, st);
		}
		return;

	case RPC_C_CN_PKT_BIND:
	case RPC_C_CN_PKT_ALTER_CONTEXT:
		if (pdu->flags & RPC_C_CN_FLAGS_SUPPORT_HEADER_SIGN)
		{
			*st = rpc__gssauth_cn_inq_flags_supported(rpc_c_protect_flags_header_sign);
			if (*st == rpc_s_ok)
			{
				sec->sec_info->authn_flags |= rpc_c_protect_flags_header_sign;
			}
			else
			{
				sec->sec_info->authn_flags &= ~rpc_c_protect_flags_header_sign;
			}

			/* If header signing is not supported it is enough to clear the flag */
			*st = rpc_s_ok;
		}
		break;

	case RPC_C_CN_PKT_FAULT:
	case RPC_C_CN_PKT_BIND_ACK:
	case RPC_C_CN_PKT_BIND_NAK:
	case RPC_C_CN_PKT_ALTER_CONTEXT_RESP:
	case RPC_C_CN_PKT_AUTH3:
	case RPC_C_CN_PKT_SHUTDOWN:
	case RPC_C_CN_PKT_REMOTE_ALERT:
	case RPC_C_CN_PKT_ORPHANED:
	default:
		break;
	}

	*st = rpc_s_ok;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_tlr_uuid_crc
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will locate and return the association
**      UUID CRC contained in the auth_value field of an
**      authentication trailer of an rpc_bind, rpc_bind_ack,
**      rpc_alter_context or rpc_alter_context_response PDU.
**
**  INPUTS:
**
**      auth_value      A pointer to the auth_value field in an authentication
**                      trailer.
**      auth_value_len  The length, in bytes, of the auth_value field.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      assoc_uuid_crc  The association UUID CRC contained in the auth_value
**                      field.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_tlr_uuid_crc
(
	pointer_t		auth_value ATTRIBUTE_UNUSED /*TODO*/,
	unsigned32		auth_value_len ATTRIBUTE_UNUSED /*TODO*/,
	unsigned32		*uuid_crc
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_tlr_uuid_crc)\n"));

	*uuid_crc = 0;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
		("(rpc__gssauth_cn_tlr_uuid_crc) assoc_uuid_crc->%x\n", *uuid_crc));
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_tlr_unpack
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will byte swap all the appropriate fields
**      of the the auth_value field of an authentication
**      trailer. It will also convert any characters from
**      the remote representation into the local, for example,
**      ASCII to EBCDIC.
**
**  INPUTS:
**
**      auth_value_len  The length, in bytes, of the auth_value field.
**      packed_drep     The packed Networ Data Representation, (see NCA RPC
**                      RunTime Extensions Specification Version OSF TX1.0.9
**                      pre 1003 for details), of the remote machine.
**
**  INPUTS/OUTPUTS:
**
**      pkt_p           A pointer to the entire packet.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_tlr_unpack
(
	rpc_cn_packet_p_t	pkt_p ATTRIBUTE_UNUSED /*TODO*/,
	unsigned32		auth_value_len ATTRIBUTE_UNUSED /*TODO*/,
	unsigned8		*packed_drep ATTRIBUTE_UNUSED /*TODO*/
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_tlr_unpack)\n"));
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_vfy_client_req
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will decode the auth_value field of
**      either an rpc_bind or rpc_alter_context PDU. Any
**      error encountered while authenticating the client
**      will result in an error status return. The contents
**      of the credentials field includes the authorization
**      data. This is called from the server side association
**      state machine. Note that upon successful return
**      the auth information rep will contain the client's
**      authorization protocol and data.
**
**  INPUTS:
**
**      assoc_sec       A pointer to per-association security context
**                      including association UUID CRC and sequence numbers.
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**      auth_value      A pointer to the auth_value field in the rpc_bind or
**                      rpc_alter_context PDU authentication trailer.
**      auth_value_len  The length, in bytes, of auth_value.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_vfy_client_req
(
	rpc_cn_assoc_sec_context_p_t	assoc_sec,
	rpc_cn_sec_context_p_t		sec,
	pointer_t			auth_value,
	unsigned32			auth_value_len,
	unsigned32			old_client ATTRIBUTE_UNUSED /*TODO*/,
	unsigned32			*st
)
{
	rpc_gssauth_cn_info_p_t gssauth_cn_info = (rpc_gssauth_cn_info_p_t)sec->sec_cn_info;
	int gss_rc;
	OM_uint32 minor_status = 0;
	gss_buffer_desc input_token, output_token = GSS_C_EMPTY_BUFFER;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_vfy_client_req)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_vfy_client_req) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id,
		assoc_sec->assoc_uuid_crc,
		assoc_sec->assoc_next_snd_seq,
		assoc_sec->assoc_next_rcv_seq));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_VFY_CLIENT_REQ)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif

	input_token.value = auth_value;
	input_token.length = auth_value_len;
	gss_rc = gss_accept_sec_context(&minor_status,
					&gssauth_cn_info->gss_ctx,
					NULL,
					&input_token,
					NULL,
					NULL,
					NULL,
					&output_token,
					NULL,
					NULL,
					NULL);
        gssauth_cn_info->gss_rc = gss_rc;
        if (gss_rc == GSS_S_CONTINUE_NEEDED) {
                char msg[256];
                rpc__gssauth_error_map(gss_rc, minor_status,
                                       (gss_OID)&rpc__gssauth_krb5_oid,
                                       msg, sizeof(msg), st);
                RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
                        ("(rpc__gssauth_cn_vfy_client_req): %s: output_token.length[%u]\n",
                        msg, (unsigned int)output_token.length));
                /*
                 * we still transfer the buffer to the client
                 * but fail the auth in rpc__gssauth_cn_fmt_srvr_resp()
                 */
        } else if (gss_rc != GSS_S_COMPLETE) {
                char msg[256];
                rpc__gssauth_error_map(gss_rc, minor_status,
                                       (gss_OID)&rpc__gssauth_krb5_oid,
                                       msg, sizeof(msg), st);
                RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
                        ("(rpc__gssauth_cn_vfy_client_req): %s\n", msg));
		return;
        } else {
                *st = rpc_s_ok;
        }

	assoc_sec->krb_message.length = output_token.length;
	assoc_sec->krb_message.data = output_token.value;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__gssauth_cn_vfy_srvr_resp
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine will decode auth_value field either an
**      rpc_bind_ack or rpc_alter_context_response PDU. If the
**      credentials field of the auth_value field contains an
**      authentication protocol specific encoding of an error
**      this will be returned as an error status code. This is
**      called from the client side association state machine.
**      Note that upon successful return the auth information
**      rep will contain the client's authorization protocol
**      and data.
**
**  INPUTS:
**
**      assoc_sec       A pointer to per-association security context
**                      including association UUID CRC and sequence numbers.
**      sec             A pointer to security context element which includes
**                      the key ID, auth information rep and RPC auth
**                      information rep.
**      auth_value      A pointer to the auth_value field in the rpc_bind or
**                      rpc_alter_context PDU authentication trailer.
**      auth_value_len  The length, in bytes, of auth_value.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void rpc__gssauth_cn_vfy_srvr_resp
(
	rpc_cn_assoc_sec_context_p_t	assoc_sec,
	rpc_cn_sec_context_p_t		sec,
	pointer_t			auth_value,
	unsigned32			auth_value_len,
	unsigned32			*st
)
{
	rpc_gssauth_info_p_t gssauth_info = (rpc_gssauth_info_p_t)sec->sec_info;
	rpc_gssauth_cn_info_p_t gssauth_cn_info = (rpc_gssauth_cn_info_p_t)sec->sec_cn_info;
	int gss_rc;
	OM_uint32 minor_status;
	gss_buffer_desc input_token, output_token = GSS_C_EMPTY_BUFFER;

	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_cn_vfy_srvr_resp)\n"));

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
		("(rpc__gssauth_cn_vfy_server_resp) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x\n",
		sec->sec_info->authn_protocol,
		sec->sec_info->authn_level,
		sec->sec_key_id,
		assoc_sec->assoc_uuid_crc,
		assoc_sec->assoc_next_snd_seq,
		assoc_sec->assoc_next_rcv_seq));

#ifdef DEBUG
	if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_AUTH_VFY_SERVER_RESP)) {
		*st = RPC_S_CN_DBG_AUTH_FAILURE;
		return;
	}
#endif

	if (gssauth_cn_info->gss_ctx != GSS_C_NO_CONTEXT &&
	    gssauth_cn_info->gss_rc == GSS_S_COMPLETE) {
		/* todo only when ready */
		goto done;
	}

	input_token.value = auth_value;
	input_token.length = auth_value_len;
	gss_rc = rpc__gssauth_verify_server_token(&minor_status,
						  sec,
						  gssauth_info->gss_creds,
						  gssauth_info->gss_server_name,
						  gssauth_cn_info->gss_ctx,
						  input_token,
						  &output_token);
	gssauth_cn_info->gss_rc = gss_rc;
	if (gss_rc == GSS_S_CONTINUE_NEEDED) {
		char msg[256];
		rpc__gssauth_error_map(gss_rc, minor_status,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_vfy_srvr_resp): %s: output_token.length[%u]\n",
			msg, (unsigned int)output_token.length));
		/*
		 * we still transfer the buffer to the client
		 * but fail the auth in rpc__gssauth_cn_fmt_srvr_resp()
		 */
	} else if (gss_rc != GSS_S_COMPLETE) {
		char msg[256];
		rpc__gssauth_error_map(gss_rc, minor_status,
				       (gss_OID)&rpc__gssauth_krb5_oid,
				       msg, sizeof(msg), st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__gssauth_cn_vfy_srvr_resp): %s\n", msg));
		return;
	}

done:
	assoc_sec->krb_message.length = output_token.length;
	assoc_sec->krb_message.data = output_token.value;

	*st = rpc_s_ok;
}


PRIVATE rpc_protocol_id_t rpc__gssauth_negotiate_cn_init
(
	rpc_auth_rpc_prot_epv_p_t	*epv,
	unsigned32			*st
)
{
	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_negotiate_cn_init)\n"));

	*epv = (rpc_auth_rpc_prot_epv_p_t)&rpc_g_gssauth_negotiate_cn_epv;
	*st = rpc_s_ok;
	return RPC_C_PROTOCOL_ID_NCACN;
}

PRIVATE rpc_protocol_id_t rpc__gssauth_mskrb_cn_init
(
	rpc_auth_rpc_prot_epv_p_t	*epv,
	unsigned32			*st
)
{
	CODING_ERROR(st);
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_mskrb_cn_init)\n"));

	*epv = (rpc_auth_rpc_prot_epv_p_t)&rpc_g_gssauth_mskrb_cn_epv;
	*st = rpc_s_ok;
	return RPC_C_PROTOCOL_ID_NCACN;
}
