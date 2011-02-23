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
**      noauth.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**      Client-side support of kerberos module.
**
**
*/

#include <noauth.h>
#include <sec_id_pickle.h>

/*
 * Size of buffer used when asking for remote server's principal name
 */
#define MAX_SERVER_PRINC_NAME_LEN 500


GLOBAL unsigned32 rpc_g_noauth_alloc_count = 0;
GLOBAL unsigned32 rpc_g_noauth_free_count = 0;

INTERNAL rpc_auth_rpc_prot_epv_p_t rpc_g_noauth_rpc_prot_epv[RPC_C_PROTOCOL_ID_MAX];

INTERNAL rpc_auth_epv_t rpc_g_noauth_epv =
{
    rpc__noauth_bnd_set_auth,
    rpc__noauth_srv_reg_auth,
    rpc__noauth_mgt_inq_def,
    rpc__noauth_inq_my_princ_name
};


/*
 * R P C _ _ N O A U T H _ B N D _ S E T _ A U T H
 *
 */

PRIVATE void rpc__noauth_bnd_set_auth 
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
    int st, i;
    rpc_noauth_info_p_t noauth_info;

    rpc_g_noauth_alloc_count++;
    RPC_MEM_ALLOC (noauth_info, rpc_noauth_info_p_t, sizeof (*noauth_info), RPC_C_MEM_UTIL, RPC_C_MEM_WAITOK);

    if ((authz_prot != rpc_c_authz_name) &&
        (authz_prot != rpc_c_authz_dce))
    {
        st = rpc_s_authn_authz_mismatch;
        goto poison;
    }

    if (level != rpc_c_authn_level_none)
    {
        st = rpc_s_unsupported_authn_level;
        goto poison;
    }

    /*
     * If no server principal name was specified, go ask for it.
     */
    if (server_name == NULL)
    {
        rpc_mgmt_inq_server_princ_name
            (binding_h, 
             dce_c_rpc_authn_protocol_krb5,
             &server_name,
             stp);
        if (*stp != rpc_s_ok)
            return;
    } else {
        server_name = rpc_stralloc(server_name);
    }
                                    
    RPC_DBG_PRINTF(rpc_e_dbg_auth, 1, (
            "(rpc__noauth_bnd_set_auth) %x created (now %d active)\n", 
            noauth_info, rpc_g_noauth_alloc_count - rpc_g_noauth_free_count));
    
    memset (noauth_info, 0, sizeof(*noauth_info));
    
    RPC_MUTEX_INIT(noauth_info->lock);
    
    noauth_info->auth_info.server_princ_name = server_name;
    noauth_info->auth_info.authn_level = level;
    noauth_info->auth_info.authn_protocol = rpc_c_authn_dce_dummy;
    noauth_info->auth_info.authz_protocol = authz_prot;
    noauth_info->auth_info.is_server = 0;
    noauth_info->auth_info.u.auth_identity = auth_ident;
    
    noauth_info->auth_info.refcount = 1;

    noauth_info->creds_valid = 1;       /* XXX what is this used for? */
    noauth_info->level_valid = 1;
    noauth_info->client_valid = 1;      /* sort of.. */

    *infop = &noauth_info->auth_info;
    noauth_info->status = rpc_s_ok;
    *stp = rpc_s_ok;
    return;
poison:
    *infop = (rpc_auth_info_p_t) &noauth_info->auth_info;
    noauth_info->status = st;
    *stp = st;
    return;
}    
 
#include <comp.h>
void rpc__module_init_func(void)
{
	static rpc_authn_protocol_id_elt_t auth[1] =	{
		{                               /* 0 */
        NULL, 
        rpc_c_authn_none,	/* FIXME: probably incorrect */ 
        dce_c_rpc_authn_protocol_none, 
        NULL,
		  NULL
    }	
	};
	rpc__register_authn_protocol(auth, 1);
}

/*
 * R P C _ _ N O A U T H _ I N I T
 *
 * Initialize the world.
 */

PRIVATE void rpc__noauth_init 
(
        rpc_auth_epv_p_t *epv,
        rpc_auth_rpc_prot_epv_tbl_t *rpc_prot_epv,
        unsigned32 *st
)
{
    unsigned32                  prot_id;
    rpc_auth_rpc_prot_epv_t     *prot_epv;

    /*
     * Initialize the RPC-protocol-specific EPVs for the RPC protocols
     * we work with (ncadg and ncacn).
     */
#ifdef AUTH_DUMMY_DG
    prot_id = rpc__noauth_dg_init (&prot_epv, st);
    if (*st == rpc_s_ok)
    {
        rpc_g_noauth_rpc_prot_epv[prot_id] = prot_epv;
    }
#endif
#ifdef AUTH_DUMMY_CN
    prot_id = rpc__noauth_cn_init (&prot_epv, st);
    if (*st == rpc_s_ok)
    {
        rpc_g_noauth_rpc_prot_epv[prot_id] = prot_epv;
    }
#endif

    /*
     * Return information for this (Kerberos) authentication service.
     */
    *epv = &rpc_g_noauth_epv;
    *rpc_prot_epv = rpc_g_noauth_rpc_prot_epv;

    *st = 0;
}


/*
 * R P C _ _ N O A U T H _ F R E E _ I N F O
 *
 * Free info.
 */

PRIVATE void rpc__noauth_free_info 
(
        rpc_auth_info_p_t *info
)
{
    rpc_noauth_info_p_t noauth_info = (rpc_noauth_info_p_t)*info ;
    char *info_type = (*info)->is_server?"server":"client";
    unsigned32 tst;

    RPC_MUTEX_DELETE(noauth_info->lock);

    if ((*info)->server_princ_name)
        rpc_string_free (&(*info)->server_princ_name, &tst);
    (*info)->u.s.privs = 0;
    if (noauth_info->client_name)
        rpc_string_free (&noauth_info->client_name, &tst);
    sec_id_pac_free (&noauth_info->client_pac);

    memset (noauth_info, 0x69, sizeof(*noauth_info));
    RPC_MEM_FREE (noauth_info, RPC_C_MEM_UTIL);
    rpc_g_noauth_free_count++;
    RPC_DBG_PRINTF(rpc_e_dbg_auth, 1, (
        "(rpc__noauth_release) freeing %s auth_info (now %d active).\n", 
        info_type, rpc_g_noauth_alloc_count - rpc_g_noauth_free_count));
    *info = NULL;
}


/*
 * R P C _ _ N O A U T H _ M G T _ I N Q _ D E F
 *
 * Return default authentication level
 *
 * !!! should read this from a config file.
 */

PRIVATE void rpc__noauth_mgt_inq_def
(
        unsigned32 *authn_level,
        unsigned32 *stp
)
{
    *authn_level = rpc_c_authn_level_none;
    *stp = rpc_s_ok;
}


/*
 * R P C _ _ N O A U T H _ S R V _ R E G _ A U T H
 *
 */

PRIVATE void rpc__noauth_srv_reg_auth 
(
        unsigned_char_p_t server_name,
        rpc_auth_key_retrieval_fn_t get_key_func,
        pointer_t arg,
        unsigned32 *stp
)
{
    *stp = rpc_s_ok;
}


/*
 * R P C _ _ N O A U T H _ I N Q _ M Y _ P R I N C _ N A M E
 *
 * All this doesn't matter for this module, but we need the placebo.
 */

PRIVATE void rpc__noauth_inq_my_princ_name 
(
        unsigned32 name_size,
        unsigned_char_p_t name,
        unsigned32 *stp
)
{
    if (name_size > 0) {
        rpc__strncpy(name, (unsigned char *)"", name_size - 1);
    }
    *stp = rpc_s_ok;
}
