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
**      schnauth.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**      Client-side support of netlogon/schannel module.
**
**
*/

#include <dce/schannel.h>
#include <schnauth.h>

/*
 * Size of buffer used when asking for remote server's principal name
 */
#define MAX_SERVER_PRINC_NAME_LEN 500


GLOBAL unsigned32 rpc_g_schnauth_alloc_count = 0;
GLOBAL unsigned32 rpc_g_schnauth_free_count = 0;

INTERNAL rpc_auth_rpc_prot_epv_p_t rpc_g_schnauth_rpc_prot_epv[RPC_C_PROTOCOL_ID_MAX];

INTERNAL void rpc__schnauth_inq_access_token(
    rpc_auth_info_p_t auth_info,
    rpc_access_token_p_t* token,
    unsigned32 *stp
    );

INTERNAL rpc_auth_epv_t rpc_g_schnauth_epv =
{
    rpc__schnauth_bnd_set_auth,
    rpc__schnauth_srv_reg_auth,
    rpc__schnauth_mgt_inq_def,
    rpc__schnauth_inq_my_princ_name,
    rpc__schnauth_free_info,
    rpc__schnauth_free_key,
    rpc__schnauth_resolve_identity,
    rpc__schnauth_release_identity,
    rpc__schnauth_inq_sec_context,
    rpc__schnauth_inq_access_token,
};


/*
 * R P C _ _ S C H N A U T H _ B N D _ S E T _ A U T H
 *
 */

PRIVATE void rpc__schnauth_bnd_set_auth
(
        unsigned_char_p_t server_name,
        rpc_authn_level_t level,
        rpc_authn_flags_t flags,
        rpc_auth_identity_handle_t auth_ident,
        rpc_authz_protocol_id_t authz_prot,
        rpc_binding_handle_t binding_h   ATTRIBUTE_UNUSED,
        rpc_auth_info_p_t *infop,
        unsigned32 *stp
)
{
    int st = rpc_s_ok;
    rpc_schnauth_info_p_t schnauth_info = NULL;
    rpc_schannel_auth_info_p_t auth_info = NULL;

    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_bnd_set_auth)\n"));

    auth_info = (rpc_schannel_auth_info_p_t)auth_ident;

    rpc_g_schnauth_alloc_count++;
    RPC_MEM_ALLOC(schnauth_info, rpc_schnauth_info_p_t, sizeof(*schnauth_info),
		  RPC_C_MEM_UTIL, RPC_C_MEM_WAITOK);

    /* I don't know yet why it has to be either of values */
    if ((authz_prot != rpc_c_authz_name) &&
	(authz_prot != rpc_c_authz_dce))
    {
	st = rpc_s_authn_authz_mismatch;
    	goto poison;
    }

    /* Enable default level of protection as well as sign&seal of rpc traffic */
    if ((level != rpc_c_authn_level_default) &&
	(level != rpc_c_authn_level_pkt_integrity) &&
	(level != rpc_c_authn_level_pkt_privacy))
    {
	st = rpc_s_unsupported_authn_level;
	goto poison;
    }

    /* No authentication flags are supported */
    if (flags)
    {
        st = rpc_s_unsupported_authn_level;
        goto poison;
    }

    if (auth_info->domain_name != NULL) {
	server_name = rpc_stralloc(auth_info->domain_name);
	if (server_name == NULL) {
            st = rpc_s_no_memory;
	    goto poison;
	}
    }

    RPC_DBG_PRINTF(rpc_e_dbg_auth, 1, (
            "(rpc__schnauth_bnd_set_auth) %x created (now %d active)\n",
	    schnauth_info, rpc_g_schnauth_alloc_count - rpc_g_schnauth_free_count));

    memset(schnauth_info, 0, sizeof(*schnauth_info));

    RPC_MUTEX_INIT(schnauth_info->lock);

    schnauth_info->auth_info.server_princ_name = server_name;
    schnauth_info->auth_info.authn_level       = level;
    schnauth_info->auth_info.authn_protocol    = rpc_c_authn_schannel;
    schnauth_info->auth_info.authz_protocol    = authz_prot;
    schnauth_info->auth_info.is_server         = 0;
    schnauth_info->auth_info.u.auth_identity   = auth_ident;

    schnauth_info->auth_info.refcount = 1;

    /* copy schannel data */
    memcpy(schnauth_info->sec_ctx.session_key, auth_info->session_key, 16);

    schnauth_info->sec_ctx.domain_name  = rpc_stralloc(auth_info->domain_name);
    if (schnauth_info->sec_ctx.domain_name == NULL) {
        st = rpc_s_no_memory;
        goto poison;
    }

    schnauth_info->sec_ctx.fqdn  = rpc_stralloc(auth_info->fqdn);
    if (schnauth_info->sec_ctx.fqdn == NULL) {
        st = rpc_s_no_memory;
        goto poison;
    }

    schnauth_info->sec_ctx.machine_name = rpc_stralloc(auth_info->machine_name);
    if (schnauth_info->sec_ctx.machine_name == NULL) {
        st = rpc_s_no_memory;
        goto poison;
    }

    /* setting auth info on binding handle means that we're initiating
       schannel connection, so ensure "rpc_schn_initiator_flags" set */
    schnauth_info->sec_ctx.sender_flags = rpc_schn_initiator_flags |
                                          auth_info->sender_flags;

poison:
    *infop = (rpc_auth_info_p_t) &schnauth_info->auth_info;
    *stp = st;
}


PRIVATE void rpc__schnauth_init
(
        rpc_auth_epv_p_t *epv,
	rpc_auth_rpc_prot_epv_tbl_t *rpc_prot_epv,
	unsigned32 *st
);


#include <comp.h>
void rpc__schnauth_init_func(void)
{
    static rpc_authn_protocol_id_elt_t auth[1] = {
	{ rpc__schnauth_init,
	  rpc_c_authn_schannel,
	  dce_c_rpc_authn_protocol_schannel,
	  NULL,
	  rpc_g_schnauth_rpc_prot_epv }
    };

    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__module_init_func)\n"));

    rpc__register_authn_protocol(auth, 1);
}


/*
 * R P C _ _ S C H N A U T H _ I N I T
 *
 * Initialize the world.
 */

PRIVATE void rpc__schnauth_init
(
        rpc_auth_epv_p_t *epv,
	rpc_auth_rpc_prot_epv_tbl_t *rpc_prot_epv,
	unsigned32 *st
)
{
    unsigned32                  prot_id;
    rpc_auth_rpc_prot_epv_t     *prot_epv;

    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_init)\n"));

    prot_id = rpc__schnauth_cn_init(&prot_epv, st);
    if (*st == rpc_s_ok)
    {
	rpc_g_schnauth_rpc_prot_epv[prot_id] = prot_epv;
    }

    /*
     * Return information for this (Netlogon/Schannel) authentication service.
     */
    *epv = &rpc_g_schnauth_epv;
    *rpc_prot_epv = rpc_g_schnauth_rpc_prot_epv;

    *st = rpc_s_ok;
}


/*
 * R P C _ _ S C H N A U T H _ F R E E _ I N F O
 *
 * Free info.
 */

PRIVATE void rpc__schnauth_free_info 
(
        rpc_auth_info_p_t *info
)
{
    rpc_schnauth_info_p_t schnauth_info = (rpc_schnauth_info_p_t)*info ;
    unsigned32 tst;

    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_free_info)\n"));

    RPC_MUTEX_DELETE(schnauth_info->lock);

    if ((*info)->server_princ_name)
    {
        rpc_string_free(&(*info)->server_princ_name, &tst);
    }

    (*info)->u.s.privs = 0;

    if (schnauth_info->sec_ctx.machine_name)
    {
	rpc_string_free(&schnauth_info->sec_ctx.machine_name, &tst);
    }

    if (schnauth_info->sec_ctx.domain_name)
    {
	rpc_string_free(&schnauth_info->sec_ctx.domain_name, &tst);
    }

    if (schnauth_info->sec_ctx.fqdn)
    {
	rpc_string_free(&schnauth_info->sec_ctx.fqdn, &tst);
    }

    memset(schnauth_info, 0x69, sizeof(*schnauth_info));
    RPC_MEM_FREE(schnauth_info, RPC_C_MEM_UTIL);
    rpc_g_schnauth_free_count++;
    RPC_DBG_PRINTF(rpc_e_dbg_auth, 1, (
        "(rpc__schnauth_release) freeing %s auth_info (now %d active).\n", 
        (*info)->is_server ? "server" : "client", rpc_g_schnauth_alloc_count - rpc_g_schnauth_free_count));
    *info = NULL;
}


/*
 * R P C _ _ S C H N A U T H _ M G T _ I N Q _ D E F
 *
 * Return default authentication level
 *
 * !!! should read this from a config file.
 */

PRIVATE void rpc__schnauth_mgt_inq_def
(
        unsigned32 *authn_level,
        unsigned32 *stp
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_mgt_inq_def)\n"));
  
    *authn_level = rpc_c_authn_level_pkt_privacy;
    *stp = rpc_s_ok;
}


/*
 * R P C _ _ S C H N A U T H _ S R V _ R E G _ A U T H
 *
 */

PRIVATE void rpc__schnauth_srv_reg_auth 
(
        unsigned_char_p_t server_name  ATTRIBUTE_UNUSED,
        rpc_auth_key_retrieval_fn_t get_key_func  ATTRIBUTE_UNUSED,
        pointer_t arg  ATTRIBUTE_UNUSED,
        unsigned32 *stp
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_srv_reg_auth)\n"));

    *stp = rpc_s_ok;
}


/*
 * R P C _ _ S C H N A U T H _ I N Q _ M Y _ P R I N C _ N A M E
 *
 * All this doesn't matter for this module, but we need the placebo.
 */

PRIVATE void rpc__schnauth_inq_my_princ_name 
(
        unsigned32 name_size,
        unsigned_char_p_t name,
        unsigned32 *stp
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_inq_my_princ_name)\n"));

    if (name_size > 0) {
        rpc__strncpy(name, (unsigned char *)"", name_size - 1);
    }
    *stp = rpc_s_ok;
}


/*
 * R P C _ _ S C H N A U T H _ F R E E _ K E Y
 *
 */

PRIVATE void rpc__schnauth_free_key
(
        rpc_key_info_p_t *key_info  ATTRIBUTE_UNUSED
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_free_key)\n"));
}
 

/*
 * R P C _ _ S C H N A U T H _ R E S O L V E _ I D E N T I T Y
 *
 */

PRIVATE error_status_t rpc__schnauth_resolve_identity
(
        rpc_auth_identity_handle_t in_identity  ATTRIBUTE_UNUSED,
        rpc_auth_identity_handle_t *out_identity  ATTRIBUTE_UNUSED
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_resolve_identity)\n"));

    *out_identity = in_identity;
    return rpc_s_ok;
}


/*
 * R P C _ _ S C H N A U T H _ R E L E A S E _ I D E N T I T Y
 *
 */

PRIVATE void rpc__schnauth_release_identity
(
        rpc_auth_identity_handle_t *identity  ATTRIBUTE_UNUSED
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_release_identity)\n"));
}


/*
 * R P C _ _ S C H N A U T H _ I N Q _ S E C _ C O N T E X T
 *
 */

PRIVATE void rpc__schnauth_inq_sec_context
(
        rpc_auth_info_p_t           auth_info  ATTRIBUTE_UNUSED,
        void                        **mech_context,
        unsigned32                  *st
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
		   ("(rpc__schnauth_inq_sec_context)\n"));

    /* This function is for the server side of schannel which
       we don't support yet */
    *mech_context = NULL;

    *st = rpc_s_ok;
}

INTERNAL void rpc__schnauth_inq_access_token(
    rpc_auth_info_p_t auth_info,
    rpc_access_token_p_t* token,
    unsigned32 *stp
    )
{
    *token = NULL;
    *stp = rpc_s_ok;

    return;
}
