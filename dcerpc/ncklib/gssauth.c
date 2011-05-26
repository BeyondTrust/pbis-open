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
**      gssauth.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Client-side support of gss-spnego and gss-krb5 module.
**
**
*/

#include <gssauth.h>
#include <lw/base.h>

/*
 * Size of buffer used when asking for remote server's principal name
 */
#define MAX_SERVER_PRINC_NAME_LEN 500


INTERNAL unsigned32 rpc_g_gssauth_alloc_count = 0;
INTERNAL unsigned32 rpc_g_gssauth_free_count = 0;

INTERNAL rpc_auth_rpc_prot_epv_p_t rpc_g_gssauth_negotiate_rpc_prot_epv[RPC_C_PROTOCOL_ID_MAX];
INTERNAL rpc_auth_rpc_prot_epv_p_t rpc_g_gssauth_mskrb_rpc_prot_epv[RPC_C_PROTOCOL_ID_MAX];

INTERNAL void rpc__gssauth_negotiate_bnd_set_auth (
	unsigned_char_p_t		/* in  */    /*server_princ_name*/,
	rpc_authn_level_t		/* in  */    /*authn_level*/,
	rpc_auth_identity_handle_t	/* in  */    /*auth_identity*/,
	rpc_authz_protocol_id_t		/* in  */    /*authz_protocol*/,
	rpc_binding_handle_t		/* in  */    /*binding_h*/,
	rpc_auth_info_p_t		/* out */    * /*auth_info*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_mskrb_bnd_set_auth (
	unsigned_char_p_t		/* in  */    /*server_princ_name*/,
	rpc_authn_level_t		/* in  */    /*authn_level*/,
	rpc_auth_identity_handle_t	/* in  */    /*auth_identity*/,
	rpc_authz_protocol_id_t		/* in  */    /*authz_protocol*/,
	rpc_binding_handle_t		/* in  */    /*binding_h*/,
	rpc_auth_info_p_t		/* out */    * /*auth_info*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_srv_reg_auth (
	unsigned_char_p_t		/* in  */    /*server_princ_name*/,
	rpc_auth_key_retrieval_fn_t	/* in  */    /*get_key_func*/,
	pointer_t			/* in  */    /*arg*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_mgt_inq_def (
	unsigned32			/* out */    * /*authn_level*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_inq_my_princ_name (
	unsigned32			/* in */     /*princ_name_size*/,
	unsigned_char_p_t		/* out */    /*princ_name*/,
	unsigned32			/* out */    * /*st*/
    );

INTERNAL void rpc__gssauth_free_info (
	rpc_auth_info_p_t		/* in/out */ * /*info*/
    );

INTERNAL void rpc__gssauth_free_key (
	rpc_key_info_p_t		/* in/out */ * /*info*/
    );

INTERNAL error_status_t rpc__gssauth_resolve_identity (
	rpc_auth_identity_handle_t	/* in */     /* in_identity*/,
	rpc_auth_identity_handle_t	/* out */    * /*out_identity*/
    );

INTERNAL void rpc__gssauth_release_identity (
	rpc_auth_identity_handle_t	/* in/out */ * /*identity*/
    );

INTERNAL void rpc__gssauth_inq_sec_context (
	rpc_auth_info_p_t		/* in */     /*auth_info*/,
	void				/* out */    ** /*mech_context*/,
	unsigned32			/* out */    * /*stp*/
    );

INTERNAL void rpc__gssauth_inq_access_token(
    rpc_auth_info_p_t auth_info,
    rpc_access_token_p_t* token,
    unsigned32 *stp
    );

INTERNAL rpc_auth_epv_t rpc_g_gssauth_negotiate_epv =
{
	rpc__gssauth_negotiate_bnd_set_auth,
	rpc__gssauth_srv_reg_auth,
	rpc__gssauth_mgt_inq_def,
	rpc__gssauth_inq_my_princ_name,
	rpc__gssauth_free_info,
	rpc__gssauth_free_key,
	rpc__gssauth_resolve_identity,
	rpc__gssauth_release_identity,
	rpc__gssauth_inq_sec_context,
        rpc__gssauth_inq_access_token
};

INTERNAL rpc_auth_epv_t rpc_g_gssauth_mskrb_epv =
{
	rpc__gssauth_mskrb_bnd_set_auth,
	rpc__gssauth_srv_reg_auth,
	rpc__gssauth_mgt_inq_def,
	rpc__gssauth_inq_my_princ_name,
	rpc__gssauth_free_info,
	rpc__gssauth_free_key,
	rpc__gssauth_resolve_identity,
	rpc__gssauth_release_identity,
	rpc__gssauth_inq_sec_context,
        rpc__gssauth_inq_access_token
};

/*
 * R P C _ _ G S S A U T H _ B N D _ S E T _ A U T H
 *
 */

INTERNAL void rpc__gssauth_bnd_set_auth
(
	unsigned_char_p_t server_name,
	rpc_authn_level_t level,
	rpc_authn_protocol_id_t authn_protocol,
	rpc_auth_identity_handle_t auth_ident,
	rpc_authz_protocol_id_t authz_prot,
	rpc_binding_handle_t binding_h,
	rpc_auth_info_p_t *infop,
	unsigned32 *stp
)
{
	unsigned32 st;
	rpc_gssauth_info_p_t gssauth_info;
	unsigned_char_p_t str_server_name;
	gss_name_t gss_server_name;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_bnd_set_auth)\n"));

	rpc_g_gssauth_alloc_count++;
	RPC_MEM_ALLOC(gssauth_info,
		      rpc_gssauth_info_p_t,
		      sizeof (*gssauth_info),
		      RPC_C_MEM_GSSAUTH_INFO,
		      RPC_C_MEM_WAITOK);
	memset (gssauth_info, 0, sizeof(*gssauth_info));

	if ((authz_prot != rpc_c_authz_name) &&
	    (authz_prot != rpc_c_authz_gss_name)) {
		st = rpc_s_authn_authz_mismatch;
		goto poison;
	}

	if ((level != rpc_c_authn_level_connect) &&
	    (level != rpc_c_authn_level_pkt_integrity) &&
	    (level != rpc_c_authn_level_pkt_privacy)) {
		st = rpc_s_unsupported_authn_level;
		goto poison;
	}

	/*
	 * If no server principal name was specified, go ask for it.
	 */
	if (authz_prot == rpc_c_authz_name) {
		gss_buffer_desc input_name;
		/* GSS_KRB5_NT_PRINCIPAL_NAME */
		gss_OID_desc nt_principal =
		{10, "\x2a\x86\x48\x86\xf7\x12\x01\x02\x02\x01"};
		int gss_rc;
		OM_uint32 minor_status = 0;

		if (server_name == NULL) {
			rpc_mgmt_inq_server_princ_name(binding_h,
						       authn_protocol,
						       &str_server_name,
						       &st);
			if (st != rpc_s_ok) {
				goto poison;
			}
		} else {
			str_server_name = rpc_stralloc(server_name);
		}

		input_name.value = (void *)str_server_name;
		input_name.length = strlen((char *)str_server_name);

		gss_rc = gss_import_name(&minor_status,
					 &input_name,
					 &nt_principal,
					 &gss_server_name);
		if (gss_rc != GSS_S_COMPLETE) {
			char msg[256];
			rpc__gssauth_error_map(gss_rc, minor_status, GSS_C_NO_OID,
					       msg, sizeof(msg), &st);
			RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
				("(rpc__gssauth_bnd_set_auth): import: %s\n", msg));
			goto poison;
		}
	} else if (authz_prot != rpc_c_authz_gss_name) {
		gss_buffer_desc output_name;
		int gss_rc;
		OM_uint32 minor_status = 0;

		gss_server_name = (gss_name_t)server_name;
		server_name = NULL;

		if (gss_server_name == GSS_C_NO_NAME) {
			/*
			 * the caller passes GSS_C_NO_NAME, we'll pass it down
			 * later, if the caller wants an autolookup
			 * rpc_c_authz_name should be used
			 */
			gss_server_name = GSS_C_NO_NAME;
			str_server_name = NULL;
		} else {
			gss_rc = gss_duplicate_name(&minor_status,
						    gss_server_name,
						    &gss_server_name);
			if (gss_rc != GSS_S_COMPLETE) {
				char msg[256];
				rpc__gssauth_error_map(gss_rc, minor_status, GSS_C_NO_OID,
						       msg, sizeof(msg), &st);
				RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
					("(rpc__gssauth_bnd_set_auth): duplicate: %s\n", msg));
				goto poison;
			}

			gss_rc = gss_display_name(&minor_status,
						  gss_server_name,
						  &output_name,
						  NULL);
			if (gss_rc != GSS_S_COMPLETE) {
				char msg[256];
				rpc__gssauth_error_map(gss_rc, minor_status, GSS_C_NO_OID,
						       msg, sizeof(msg), &st);
				RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
					("(rpc__gssauth_bnd_set_auth): display: %s\n", msg));
				goto poison;
			}

			RPC_MEM_ALLOC(str_server_name,
				      unsigned_char_p_t,
				      output_name.length + 1,
				      RPC_C_MEM_STRING,
				      RPC_C_MEM_WAITOK);
			rpc__strncpy(str_server_name,
				     output_name.value,
				     output_name.length);

			gss_release_buffer(&minor_status,
					   &output_name);
		}
	}

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
		("(rpc__gssauth_bnd_set_auth) %x created (now %d active)\n",
		gssauth_info, rpc_g_gssauth_alloc_count - rpc_g_gssauth_free_count));

	gssauth_info->auth_info.server_princ_name = str_server_name;
	gssauth_info->auth_info.authn_level = level;
	gssauth_info->auth_info.authn_protocol = authn_protocol;
	gssauth_info->auth_info.authz_protocol = authz_prot;
	gssauth_info->auth_info.is_server = 0;
	gssauth_info->auth_info.u.auth_identity = auth_ident;

	gssauth_info->auth_info.refcount = 1;

	gssauth_info->gss_server_name = gss_server_name;
	gssauth_info->gss_creds = (gss_cred_id_t)auth_ident;

	*infop = &gssauth_info->auth_info;
	*stp = rpc_s_ok;
	return;
poison:
/*TODO: should we really return *infop while returning an error???*/
	*infop = &gssauth_info->auth_info;
	*stp = st;
	return;
}

INTERNAL void rpc__gssauth_negotiate_bnd_set_auth
(
	unsigned_char_p_t server_name,
	rpc_authn_level_t level,
	rpc_auth_identity_handle_t auth_ident,
	rpc_authz_protocol_id_t authz_prot,
	rpc_binding_handle_t binding_h,
	rpc_auth_info_p_t *infop,
	unsigned32 *stp
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_negotiate_bnd_set_auth)\n"));

	rpc__gssauth_bnd_set_auth(server_name,
				  level,
				  rpc_c_authn_gss_negotiate,
				  auth_ident,
				  authz_prot,
				  binding_h,
				  infop,
				  stp);
}

INTERNAL void rpc__gssauth_mskrb_bnd_set_auth
(
	unsigned_char_p_t server_name,
	rpc_authn_level_t level,
	rpc_auth_identity_handle_t auth_ident,
	rpc_authz_protocol_id_t authz_prot,
	rpc_binding_handle_t binding_h,
	rpc_auth_info_p_t *infop,
	unsigned32 *stp
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_mskrb_bnd_set_auth)\n"));

	rpc__gssauth_bnd_set_auth(server_name,
				  level,
				  rpc_c_authn_gss_mskrb,
				  auth_ident,
				  authz_prot,
				  binding_h,
				  infop,
				  stp);
}

INTERNAL void rpc__gssauth_negotiate_init();
INTERNAL void rpc__gssauth_mskrb_init();

void rpc__gssauth_init_func(void)
{
	static rpc_authn_protocol_id_elt_t auth[2] = {
	{ /* 0 */
		rpc__gssauth_negotiate_init,
		rpc_c_authn_gss_negotiate,
		dce_c_rpc_authn_protocol_gss_negotiate,
		NULL,
		rpc_g_gssauth_negotiate_rpc_prot_epv
	},
	{ /* 1 */
		rpc__gssauth_mskrb_init,
		rpc_c_authn_gss_mskrb,
		dce_c_rpc_authn_protocol_gss_mskrb,
		NULL,
		rpc_g_gssauth_mskrb_rpc_prot_epv
	}
	};

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__module_init_func)\n"));

	rpc__register_authn_protocol(auth, 2);
}

INTERNAL void rpc__gssauth_negotiate_init
(
	rpc_auth_epv_p_t *epv,
	rpc_auth_rpc_prot_epv_tbl_t *rpc_prot_epv,
	unsigned32 *st
)
{
	unsigned32		prot_id;
	rpc_auth_rpc_prot_epv_t *prot_epv;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_negotiate_init)\n"));

	/*
	 * Initialize the RPC-protocol-specific EPVs for the RPC protocols
	 * we work with (ncacn).
	 */
	/* for now only ncacn, that's what windows uses */
	prot_id = rpc__gssauth_negotiate_cn_init (&prot_epv, st);
	if (*st == rpc_s_ok) {
		rpc_g_gssauth_negotiate_rpc_prot_epv[prot_id] = prot_epv;
	}

	/*
	 * Return information for this gss_negotiate (SPNEGO) authentication service.
	 */
	*epv = &rpc_g_gssauth_negotiate_epv;
	*rpc_prot_epv = rpc_g_gssauth_negotiate_rpc_prot_epv;

	*st = 0;
}

INTERNAL void rpc__gssauth_mskrb_init
(
	rpc_auth_epv_p_t *epv,
	rpc_auth_rpc_prot_epv_tbl_t *rpc_prot_epv,
	unsigned32 *st
)
{
	unsigned32		prot_id;
	rpc_auth_rpc_prot_epv_t *prot_epv;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_mskrb_init)\n"));

	/*
	 * Initialize the RPC-protocol-specific EPVs for the RPC protocols
	 * we work with (ncacn).
	 */
	/* for now only ncacn, that's what windows uses */
	prot_id = rpc__gssauth_mskrb_cn_init (&prot_epv, st);
	if (*st == rpc_s_ok) {
		rpc_g_gssauth_mskrb_rpc_prot_epv[prot_id] = prot_epv;
	}

	/*
	 * Return information for this (KRB5) authentication service.
	 */
	*epv = &rpc_g_gssauth_mskrb_epv;
	*rpc_prot_epv = rpc_g_gssauth_mskrb_rpc_prot_epv;

	*st = 0;
}

/*
 * R P C _ _ G S S A U T H _ F R E E _ I N F O
 *
 * Free info.
 */

INTERNAL void rpc__gssauth_free_info
(
	rpc_auth_info_p_t *info
)
{
	rpc_gssauth_info_p_t gssauth_info = (rpc_gssauth_info_p_t)*info ;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_free_info)\n"));

	if (gssauth_info->auth_info.server_princ_name) {
		unsigned32 st;
		rpc_string_free(&gssauth_info->auth_info.server_princ_name, &st);
	}

	if (gssauth_info->gss_server_name != GSS_C_NO_NAME) {
		OM_uint32 minor_status;
		gss_release_name(&minor_status, &gssauth_info->gss_server_name);
		gssauth_info->gss_server_name = GSS_C_NO_NAME;
	}

	memset(gssauth_info, 0x69, sizeof(*gssauth_info));
	RPC_MEM_FREE(gssauth_info, RPC_C_MEM_GSSAUTH_INFO);

	rpc_g_gssauth_free_count++;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
		("(rpc__gssauth_free_info) freeing %s auth_info (now %d active).\n",
		(*info)->is_server ? "server" : "client", rpc_g_gssauth_alloc_count - rpc_g_gssauth_free_count));

	*info = NULL;
}


/*
 * R P C _ _ G S S A U T H _ M G T _ I N Q _ D E F
 *
 * Return default authentication level
 *
 * !!! should read this from a config file.
 */

INTERNAL void rpc__gssauth_mgt_inq_def
(
	unsigned32 *authn_level,
	unsigned32 *stp
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_mgt_inq_def)\n"));

	*authn_level = rpc_c_authn_level_pkt_privacy;
	*stp = rpc_s_ok;
}


/*
 * R P C _ _ G S S A U T H _ S R V _ R E G _ A U T H
 *
 */

INTERNAL void rpc__gssauth_srv_reg_auth
(
	unsigned_char_p_t server_name ATTRIBUTE_UNUSED,
	rpc_auth_key_retrieval_fn_t get_key_func ATTRIBUTE_UNUSED,
	pointer_t arg ATTRIBUTE_UNUSED,
	unsigned32 *stp
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_srv_reg_auth)\n"));

	*stp = rpc_s_ok;
}


/*
 * R P C _ _ G S S A U T H _ I N Q _ M Y _ P R I N C _ N A M E
 *
 * All this doesn't matter for this module, but we need the placebo.
 */

INTERNAL void rpc__gssauth_inq_my_princ_name
(
	unsigned32 name_size,
	unsigned_char_p_t name,
	unsigned32 *stp
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_inq_my_princ_name)\n"));

	if (name_size > 0) {
		rpc__strncpy(name, (unsigned char *)"", name_size - 1);
	}
	*stp = rpc_s_ok;
}

/*
 * R P C _ _ G S S A U T H _ F R E E _ KEY
 *
 * Free key.
 */

INTERNAL void rpc__gssauth_free_key
(
	rpc_key_info_p_t *info ATTRIBUTE_UNUSED
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_free_key)\n"));
}

/*
 * R P C _ _ G S S A U T H _ R E S O L V E _ I D E N T I T Y
 *
 * Resolve identity.
 */

INTERNAL error_status_t rpc__gssauth_resolve_identity
(
	rpc_auth_identity_handle_t in_identity,
	rpc_auth_identity_handle_t *out_identity
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_resolve_identity)\n"));

	*out_identity = in_identity;
	return 0;
}

/*
 * R P C _ _ G S S A U T H _ R E L E A S E _ I D E N T I T Y
 *
 * Release identity.
 */

INTERNAL void rpc__gssauth_release_identity
(
	rpc_auth_identity_handle_t *identity ATTRIBUTE_UNUSED
)
{
	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_release_identity)\n"));
}

/*
 * R P C _ _ G S S A U T H _ I N Q _ S E C _ C O N T E X T
 *
 * Inq sec context.
 */

INTERNAL void rpc__gssauth_inq_sec_context
(auth_info, mech_context, stp)
	rpc_auth_info_p_t auth_info;
	void **mech_context;
	unsigned32 *stp;
{
	rpc_gssauth_info_p_t gssauth_info = NULL;
	rpc_gssauth_cn_info_p_t gssauth_cn_info = NULL;

	RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		("(rpc__gssauth_inq_sec_context)\n"));

	gssauth_info = (rpc_gssauth_info_p_t)auth_info;
	gssauth_cn_info = gssauth_info->cn_info;

	*mech_context = (void*)gssauth_cn_info->gss_ctx;
	*stp = rpc_s_ok;
}

INTERNAL void rpc__gssauth_inq_access_token(
    rpc_auth_info_p_t auth_info,
    rpc_access_token_p_t* token,
    unsigned32 *stp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT context = NULL;
    rpc_gssauth_info_p_t gssauth_info = NULL;
    rpc_gssauth_cn_info_p_t gssauth_cn_info = NULL;
    
    gssauth_info = (rpc_gssauth_info_p_t)auth_info;
    gssauth_cn_info = gssauth_info->cn_info;
    
    status = LwMapSecurityCreateContext(&context);
    if (status) goto error;
    
    status = LwMapSecurityCreateAccessTokenFromGssContext(
        context,
        token,
        gssauth_cn_info->gss_ctx);
    if (status) goto error;
    
    *stp = rpc_s_ok;

cleanup:
    if (context)
    {
	    LwMapSecurityFreeContext(&context);
    }

    return;

error:

    *stp = -1;
    
    goto cleanup;
}
