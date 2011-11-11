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
**      ntlmssp.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Client-side support of ntlmssp module.
**
**
*/

#include <ntlmssp.h>
#include <lw/base.h>
#include <gssapi/gssapi.h>

/*
 * Size of buffer used when asking for remote server's principal name
 */
#define MAX_SERVER_PRINC_NAME_LEN 500


INTERNAL unsigned32 rpc_g_ntlmauth_alloc_count = 0;
INTERNAL unsigned32 rpc_g_ntlmauth_free_count = 0;

INTERNAL rpc_auth_rpc_prot_epv_p_t rpc_g_ntlmauth_rpc_prot_epv[RPC_C_PROTOCOL_ID_MAX];

INTERNAL void rpc__ntlmauth_bnd_set_auth (
	unsigned_char_p_t		/* in  */    /*server_princ_name*/,
	rpc_authn_level_t		/* in  */    /*authn_level*/,
	rpc_authn_flags_t               /* in  */    /*authn_flags*/,
	rpc_auth_identity_handle_t	/* in  */    /*auth_identity*/,
	rpc_authz_protocol_id_t		/* in  */    /*authz_protocol*/,
	rpc_binding_handle_t		/* in  */    /*binding_h*/,
	rpc_auth_info_p_t		/* out */    * /*auth_info*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__ntlmauth_srv_reg_auth (
	unsigned_char_p_t		/* in  */    /*server_princ_name*/,
	rpc_auth_key_retrieval_fn_t	/* in  */    /*get_key_func*/,
	pointer_t			/* in  */    /*arg*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__ntlmauth_mgt_inq_def (
	unsigned32			/* out */    * /*authn_level*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__ntlmauth_inq_my_princ_name (
	unsigned32			/* in */     /*princ_name_size*/,
	unsigned_char_p_t		/* out */    /*princ_name*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__ntlmauth_free_info (
	rpc_auth_info_p_t		/* in/out */ * /*info*/
    );

INTERNAL void rpc__ntlmauth_free_key (
	rpc_key_info_p_t		/* in/out */ * /*info*/
    );

INTERNAL error_status_t rpc__ntlmauth_resolve_identity (
	rpc_auth_identity_handle_t	/* in */     /* in_identity*/,
	rpc_auth_identity_handle_t	/* out */    * /*out_identity*/
    );

INTERNAL void rpc__ntlmauth_release_identity (
	rpc_auth_identity_handle_t	/* in/out */ * /*identity*/
    );

INTERNAL void rpc__ntlmauth_inq_sec_context (
	rpc_auth_info_p_t		/* in */     /*auth_info*/,
	void				/* out */    ** /*mech_context*/,
	unsigned32			/* out */    * /*stp*/
    );

INTERNAL void rpc__ntlmauth_inq_access_token(
    rpc_auth_info_p_t auth_info,
    rpc_access_token_p_t* token,
    unsigned32 *stp
    );

INTERNAL rpc_auth_epv_t rpc_g_ntlmauth_epv =
{
	rpc__ntlmauth_bnd_set_auth,
	rpc__ntlmauth_srv_reg_auth,
	rpc__ntlmauth_mgt_inq_def,
	rpc__ntlmauth_inq_my_princ_name,
	rpc__ntlmauth_free_info,
	rpc__ntlmauth_free_key,
	rpc__ntlmauth_resolve_identity,
	rpc__ntlmauth_release_identity,
	rpc__ntlmauth_inq_sec_context,
        rpc__ntlmauth_inq_access_token
};


/*
 * R P C _ _ N T L M A U T H _ B N D _ S E T _ A U T H
 *
 */

INTERNAL void rpc__ntlmauth_bnd_set_auth
(
	unsigned_char_p_t server_name,
	rpc_authn_level_t level,
	rpc_authn_flags_t flags,
	rpc_auth_identity_handle_t auth_ident,
	rpc_authz_protocol_id_t authz_prot,
	rpc_binding_handle_t binding_h,
	rpc_auth_info_p_t *infop,
	unsigned32 *stp
)
{
	unsigned32 st = rpc_s_ok;
	rpc_ntlmssp_auth_ident_t_p auth_info = NULL;
	rpc_ntlmauth_info_p_t ntlmauth_info = NULL;
	gss_name_t gss_server_name = {0};
	unsigned char *str_server_name = NULL;
	gss_buffer_desc username_buf = {0};
	gss_name_t gss_user_name = NULL;
	int gss_rc = 0;
	OM_uint32 minor_status = 0;
	gss_OID_set_desc desired_mech;
	gss_OID_set ret_mech;
	gss_cred_id_t cred_handle = GSS_C_NO_CREDENTIAL;
	OM_uint32 time_rec = 0;
	gss_OID_desc gss_ntlm_oid_desc = {0};
	gss_OID_desc gss_cred_opt_password_oid_desc = {0};
	gss_buffer_desc auth_buffer = {0};

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_bnd_set_auth)\n"));

	rpc_g_ntlmauth_alloc_count++;
	RPC_MEM_ALLOC(ntlmauth_info,
		      rpc_ntlmauth_info_p_t,
		      sizeof (*ntlmauth_info),
		      RPC_C_MEM_NTLMAUTH_INFO,
		      RPC_C_MEM_WAITOK);
	memset(ntlmauth_info, 0, sizeof(*ntlmauth_info));

	if (authz_prot != rpc_c_authz_name)
	{
		st = rpc_s_authn_authz_mismatch;
		goto poison;
	}

	if ((level != rpc_c_authn_level_connect) &&
	    (level != rpc_c_authn_level_pkt_integrity) &&
	    (level != rpc_c_authn_level_pkt_privacy))
	{
		st = rpc_s_unsupported_authn_level;
		goto poison;
	}

	if (flags & (~rpc_c_protect_flags_header_sign))
	{
		st = rpc_s_unsupported_protect_level;
		goto poison;
	}

	// Header signing extension has to be enabled in ntlmssp
	flags |= rpc_c_protect_flags_header_sign;

	if (server_name == NULL ||
	    auth_ident == NULL)
	{
		st = rpc_s_invalid_arg;
		goto poison;
	}

	auth_info = (rpc_ntlmssp_auth_ident_t_p)auth_ident;

	if (authz_prot == rpc_c_authz_name)
	{
		gss_buffer_desc input_name;
		/* GSS_KRB5_NT_PRINCIPAL_NAME */
		gss_OID_desc nt_principal =
		{10, "\x2a\x86\x48\x86\xf7\x12\x01\x02\x02\x01"};
		int gss_rc = 0;
		OM_uint32 minor_status = 0;

		if (server_name == NULL)
		{
			rpc_mgmt_inq_server_princ_name(binding_h,
						       rpc_c_authn_winnt,
						       &str_server_name,
						       &st);
			if (st != rpc_s_ok)
			{
				goto poison;
			}
		}
		else
		{
			str_server_name = rpc_stralloc(server_name);
		}

		input_name.value = (void *)str_server_name;
		input_name.length = strlen((char *)str_server_name);

		gss_rc = gss_import_name(&minor_status,
					 &input_name,
					 &nt_principal,
					 &gss_server_name);
		if (gss_rc != GSS_S_COMPLETE)
		{
			char msg[256] = {0};
			rpc__ntlmauth_error_map(gss_rc, minor_status, GSS_C_NO_OID,
						msg, sizeof(msg), &st);
			RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
				       ("(rpc__gssauth_bnd_set_auth): import: %s\n", msg));
			goto poison;
		}
	}

	gss_ntlm_oid_desc.length                = GSS_MECH_NTLM_LEN;
	gss_ntlm_oid_desc.elements              = GSS_MECH_NTLM;
	gss_cred_opt_password_oid_desc.length   = GSS_CRED_OPT_PW_LEN;
	gss_cred_opt_password_oid_desc.elements = GSS_CRED_OPT_PW;

	username_buf.value  = auth_info->User;
	username_buf.length = auth_info->UserLength;

	gss_rc = gss_import_name(&minor_status,
				 &username_buf,
				 GSS_C_NT_USER_NAME,
				 &gss_user_name);
	if (gss_rc != GSS_S_COMPLETE)
	{
		char msg[256] = {0};
		rpc__ntlmauth_error_map(gss_rc, minor_status, GSS_C_NO_OID,
				        msg, sizeof(msg), &st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__ntlmauth_bnd_set_auth): import: %s\n", msg));
		goto poison;
	}

	desired_mech.elements = (gss_OID)&gss_ntlm_oid_desc;
	desired_mech.count    = 1;

	gss_rc = gss_acquire_cred(&minor_status,
				  gss_user_name,
				  0,
				  &desired_mech,
				  GSS_C_INITIATE,
				  &cred_handle,
				  &ret_mech,
				  &time_rec);
	if (gss_rc != GSS_S_COMPLETE)
	{
		char msg[256] = {0};
		rpc__ntlmauth_error_map(gss_rc, minor_status, GSS_C_NO_OID,
				        msg, sizeof(msg), &st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__ntlmauth_bnd_set_auth): import: %s\n", msg));
		goto poison;
	}

	auth_buffer.value  = auth_info;
	auth_buffer.length = sizeof(*auth_info);

	gss_rc = gss_set_cred_option(&minor_status,
					&cred_handle,
					(gss_OID)&gss_cred_opt_password_oid_desc,
					&auth_buffer);
	if (gss_rc != GSS_S_COMPLETE)
	{
		char msg[256] = {0};
		rpc__ntlmauth_error_map(gss_rc, minor_status, GSS_C_NO_OID,
				        msg, sizeof(msg), &st);
		RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
			("(rpc__ntlmauth_bnd_set_auth): import: %s\n", msg));
		goto poison;
	}

	ntlmauth_info->auth_info.server_princ_name = str_server_name;
	ntlmauth_info->auth_info.authn_level       = level;
	ntlmauth_info->auth_info.authn_flags       = flags;
	ntlmauth_info->auth_info.authn_protocol    = rpc_c_authn_winnt;
	ntlmauth_info->auth_info.authz_protocol    = authz_prot;
	ntlmauth_info->auth_info.is_server         = 0;
	ntlmauth_info->auth_info.u.auth_identity   = auth_ident;

	ntlmauth_info->auth_info.refcount          = 1;

	ntlmauth_info->gss_server_name             = gss_server_name;
	ntlmauth_info->gss_creds                   = cred_handle;

	if (gss_user_name)
	{
		gss_release_name(&minor_status, &gss_user_name);
	}

	*infop = &ntlmauth_info->auth_info;
	*stp = st;
	return;
poison:
	*infop = NULL;
	*stp = st;
	return;
}

INTERNAL void rpc__ntlmauth_init();

void rpc__ntlmauth_init_func(void)
{
	static rpc_authn_protocol_id_elt_t auth[] = {
	{ /* 0 */
		rpc__ntlmauth_init,
		rpc_c_authn_winnt,
		dce_c_rpc_authn_protocol_winnt,
		NULL,
		rpc_g_ntlmauth_rpc_prot_epv
	}
	};

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__module_init_func)\n"));

	rpc__register_authn_protocol(auth, 1);
}

INTERNAL void rpc__ntlmauth_init
(
	rpc_auth_epv_p_t *epv,
	rpc_auth_rpc_prot_epv_tbl_t *rpc_prot_epv,
	unsigned32 *st
)
{
	unsigned32		prot_id;
	rpc_auth_rpc_prot_epv_t *prot_epv;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_negotiate_init)\n"));

	/*
	 * Initialize the RPC-protocol-specific EPVs for the RPC protocols
	 * we work with (ncacn).
	 */
	/* for now only ncacn, as that's what windows uses */
	prot_id = rpc__ntlmauth_cn_init (&prot_epv, st);
	if (*st == rpc_s_ok) {
		rpc_g_ntlmauth_rpc_prot_epv[prot_id] = prot_epv;
	}

	/*
	 * Return information for this ntlmssp authentication service.
	 */
	*epv = &rpc_g_ntlmauth_epv;
	*rpc_prot_epv = rpc_g_ntlmauth_rpc_prot_epv;

	*st = 0;
}

/*
 * R P C _ _ N T L M A U T H _ F R E E _ I N F O
 *
 * Free info.
 */

INTERNAL void rpc__ntlmauth_free_info
(
	rpc_auth_info_p_t *info
)
{
	rpc_ntlmauth_info_p_t ntlmauth_info = NULL;
	unsigned32 st = 0;
	OM_uint32 minor_status = 0;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_free_info)\n"));

	if (info == NULL ||
	    *info == NULL)
	{
		return;
	}

	ntlmauth_info = (rpc_ntlmauth_info_p_t)(*info);

	if (ntlmauth_info->auth_info.server_princ_name)
	{
		rpc_string_free(&ntlmauth_info->auth_info.server_princ_name,
				&st);
	}

	if (ntlmauth_info->gss_server_name)
	{
		gss_release_name(&minor_status,
				 &ntlmauth_info->gss_server_name);
	}

	if (ntlmauth_info->gss_creds)
	{
		gss_release_cred(&minor_status, &ntlmauth_info->gss_creds);
	}

	memset(ntlmauth_info, 0, sizeof(*ntlmauth_info));
	RPC_MEM_FREE(ntlmauth_info, RPC_C_MEM_NTLMAUTH_INFO);

	rpc_g_ntlmauth_free_count++;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
		("(rpc__ntlmauth_free_info) freeing %s auth_info (now %d active).\n",
		ntlmauth_info->auth_info.is_server ? "server" : "client", rpc_g_ntlmauth_alloc_count - rpc_g_ntlmauth_free_count));

	*info = NULL;
}


/*
 * R P C _ _ N T L M A U T H _ M G T _ I N Q _ D E F
 *
 * Return default authentication level
 *
 * !!! should read this from a config file.
 */

INTERNAL void rpc__ntlmauth_mgt_inq_def
(
	unsigned32 *authn_level,
	unsigned32 *stp
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_mgt_inq_def)\n"));

	if (authn_level == NULL)
	{
		*stp = rpc_s_invalid_arg;
		return;
	}

	*authn_level = rpc_c_authn_level_connect;
	*stp = rpc_s_ok;
}


/*
 * R P C _ _ N T L M A U T H _ S R V _ R E G _ A U T H
 *
 */

INTERNAL void rpc__ntlmauth_srv_reg_auth
(
	unsigned_char_p_t server_name ATTRIBUTE_UNUSED,
	rpc_auth_key_retrieval_fn_t get_key_func ATTRIBUTE_UNUSED,
	pointer_t arg ATTRIBUTE_UNUSED,
	unsigned32 *stp
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_srv_reg_auth)\n"));

	*stp = rpc_s_ok;
}


/*
 * R P C _ _ N T L M A U T H _ I N Q _ M Y _ P R I N C _ N A M E
 *
 * All this doesn't matter for this module, but we need the placebo.
 */

INTERNAL void rpc__ntlmauth_inq_my_princ_name
(
	unsigned32 name_size,
	unsigned_char_p_t name,
	unsigned32 *stp
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_inq_my_princ_name)\n"));

	if (name_size > 0) {
		rpc__strncpy(name, (unsigned char *)"", name_size - 1);
	}
	*stp = rpc_s_ok;
}

/*
 * R P C _ _ N T L M A U T H _ F R E E _ KEY
 *
 * Free key.
 */

INTERNAL void rpc__ntlmauth_free_key
(
	rpc_key_info_p_t *info ATTRIBUTE_UNUSED
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_free_key)\n"));
}

/*
 * R P C _ _ N T L M A U T H _ R E S O L V E _ I D E N T I T Y
 *
 * Resolve identity.
 */

INTERNAL error_status_t rpc__ntlmauth_resolve_identity
(
	rpc_auth_identity_handle_t in_identity,
	rpc_auth_identity_handle_t *out_identity
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_resolve_identity)\n"));

	*out_identity = in_identity;
	return 0;
}

/*
 * R P C _ _ N T L M A U T H _ R E L E A S E _ I D E N T I T Y
 *
 * Release identity.
 */

INTERNAL void rpc__ntlmauth_release_identity
(
	rpc_auth_identity_handle_t *identity ATTRIBUTE_UNUSED
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_release_identity)\n"));
}

/*
 * R P C _ _ N T L M A U T H _ I N Q _ S E C _ C O N T E X T
 *
 * Inq sec context.
 */

INTERNAL void rpc__ntlmauth_inq_sec_context
(auth_info, mech_context, stp)
	rpc_auth_info_p_t auth_info;
	void **mech_context;
	unsigned32 *stp;
{
	rpc_ntlmauth_info_p_t ntlmauth_info = NULL;
	rpc_ntlmauth_cn_info_p_t ntlmauth_cn_info = NULL;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__ntlmauth_inq_sec_context)\n"));

	if (auth_info == NULL ||
	    mech_context == NULL)
	{
		*stp = rpc_s_invalid_arg;
		return;
	}

	ntlmauth_info    = (rpc_ntlmauth_info_p_t)auth_info;
	ntlmauth_cn_info = ntlmauth_info->cn_info;

	*mech_context = (void*)ntlmauth_cn_info->gss_ctx;
	*stp = rpc_s_ok;
}

INTERNAL void rpc__ntlmauth_inq_access_token(
    rpc_auth_info_p_t auth_info,
    rpc_access_token_p_t* token,
    unsigned32 *stp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT context = NULL;
    rpc_ntlmauth_info_p_t ntlmauth_info = NULL;
    rpc_ntlmauth_cn_info_p_t ntlmauth_cn_info = NULL;

    if (auth_info == NULL ||
	token == NULL)
    {
	    *stp = rpc_s_invalid_arg;
	    return;
    }

    ntlmauth_info    = (rpc_ntlmauth_info_p_t)auth_info;
    ntlmauth_cn_info = ntlmauth_info->cn_info;

    status = LwMapSecurityCreateContext(&context);
    if (status) goto error;

    status = LwMapSecurityCreateAccessTokenFromGssContext(
        context,
        token,
        ntlmauth_cn_info->gss_ctx);
    if (status) goto error;

    *stp = rpc_s_ok;

cleanup:
    if (context)
    {
	    LwMapSecurityFreeContext(&context);
    }

    return;

error:
    *token = NULL;
    *stp   = -1;

    goto cleanup;
}
