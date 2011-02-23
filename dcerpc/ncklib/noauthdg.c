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
**      rpcdgcom.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The noauth DG authentication module.
**
**
*/

#include <noauthdg.h>

EXTERNAL int rpc_g_noauth_alloc_count;
EXTERNAL int rpc_g_noauth_free_count;

INTERNAL rpc_dg_auth_epv_t rpc_g_noauth_dg_epv =
{
    rpc_c_authn_dce_dummy,      /* "3" */
    16,                         /* 10 bytes overhead, rounded up */
    8,                          /* 8 byte block */
    rpc__noauth_dg_create,
    rpc__noauth_dg_pre_call,
    rpc__noauth_dg_encrypt,
    rpc__noauth_dg_pre_send,
    rpc__noauth_dg_recv_ck,
    rpc__noauth_dg_who_are_you,
    rpc__noauth_dg_way_handler
};


/*
 * R P C _ _ N O A U T H _ D G _ E N C R Y P T
 *
 * Optionally encrypt user data in the packet.
 */

PRIVATE void rpc__noauth_dg_encrypt
(
        rpc_auth_info_p_t               info,
        rpc_dg_xmitq_elt_p_t            xqe,
        unsigned32                      *st
)
{
    *st = rpc_s_ok;
}


/*
 * R P C _ _ N O A U T H _ D G _ P R E _ S E N D
 *
 * Optionally encrypt user data in the packet.
 */

PRIVATE void rpc__noauth_dg_pre_send 
(
        rpc_auth_info_p_t info,
        rpc_dg_xmitq_elt_p_t pkt,
        rpc_dg_pkt_hdr_p_t hdrp,
        rpc_socket_iovec_p_t iov,
        int iovlen,
        pointer_t cksum,
        error_status_t *st
)
{
    *st = rpc_s_ok;
}


/*
 * R P C _ _ N O A U T H _ D G _ R E C V _ C K
 *
 */

PRIVATE void rpc__noauth_dg_recv_ck 
(
        rpc_auth_info_p_t info,
        rpc_dg_recvq_elt_p_t pkt,
        pointer_t cksum,
        error_status_t *st
)
{
    *st = rpc_s_ok;
}


/*
 * R P C _ _ N O A U T H _ D G _ P R E _ C A L L
 *
 */

PRIVATE void rpc__noauth_dg_pre_call 
(
        rpc_auth_info_p_t info,
        handle_t h,
        unsigned32 *st
)
{
    *st = rpc_s_ok;
}


/*
 * R P C _ _ N O A U T H _ D G _ W A Y _ H A N D L E R
 *
 */

PRIVATE void rpc__noauth_dg_way_handler 
(
        rpc_auth_info_p_t info,
        ndr_byte *in_data,
        signed32 in_len,
        signed32 out_max_len,
        ndr_byte *out_data,
        signed32 *out_len,
        unsigned32 *stp
)
{
    sec_krb_message message;
    error_status_t st;
    
    rpc_noauth_info_p_t noauth_info = (rpc_noauth_info_p_t)info;
        
    *out_len = 0;
    
    RPC_DBG_PRINTF(rpc_e_dbg_auth, 2, ("(rpc__noauth_dg_way_handler) %x called back\n", info));
    
    if (noauth_info->status != rpc_s_ok)
    {
        RPC_DBG_GPRINTF(("(rpc__noauth_dg_way_handler) handle was poisoned with %x\n",
            noauth_info->status));
        *stp = noauth_info->status;
        return;
    }

    message.data = 0;
    message.length = 0;
    
    st = sec_krb_dg_build_message (noauth_info->auth_info.u.auth_identity, 0, 0,
        rpc_c_authn_level_none, noauth_info->auth_info.authz_protocol,
        0, 0, 0, &message);

    if (st != rpc_s_ok)
        goto out;
        
    if (message.length > out_max_len)
    {
        st = rpc_s_credentials_too_large;
        goto out;
    }

    memcpy(out_data, message.data, message.length);
    *out_len = message.length;
out:
    sec_krb_message_free(&message);
    *stp = st;
    return;
}


/*
 * R P C _ _ N O A U T H _ D G _ W H O _ A R E _ Y O U
 *
 * Issue challenge to client; decompose response and sanity-check it.
 */

PRIVATE void rpc__noauth_dg_who_are_you 
(
        rpc_auth_info_p_t info,
        handle_t h,
        dce_uuid_t *actuid,
        unsigned32 boot_time,
        unsigned32 *seq,
        dce_uuid_t *cas_uuid,
        unsigned32 *stp
)
{
    rpc_noauth_info_p_t noauth_info = (rpc_noauth_info_p_t)info;
    unsigned char inbuf[12];    /* XXX size */
    unsigned char outbuf[1000]; /* XXX size */
    unsigned_char_p_t server;
    signed32 outlen;
    sec_krb_message message;
    int st;

    /* XXX set up exception handler here around remote call? */
    RPC_DBG_PRINTF(rpc_e_dbg_auth, 2, ("(rpc__noauth_dg_way) %x doing callback\n", info));
    
    /* do call */
    (*conv_v3_0_c_epv.conv_who_are_you_auth)
        (h, actuid, boot_time, inbuf, 0, sizeof(outbuf),
         seq, cas_uuid, outbuf, &outlen, stp);
    
    st = *stp;
    if (st != rpc_s_ok)
    {
        RPC_DBG_GPRINTF(("(rpc__noauth_dg_way) conv_who_are_you_auth failed, st %x\n", st));
        return;
    }
    message.data = outbuf;
    message.length = outlen;

    *stp = sec_krb_dg_decode_message (&message, 0,
        &noauth_info->client_name,
        &noauth_info->client_pac,
        &noauth_info->client_creds,    /* FAKE-EPAC */
        &server,
        &noauth_info->auth_info.authn_level,
        &noauth_info->auth_info.authz_protocol,
        0, 0, 0, 0);
}


/*
 * R P C _ _ N O A U T H _ D G _ C R E A T E
 *
 * Issue challenge to client; decompose response and sanity-check it.
 */

PRIVATE rpc_auth_info_p_t rpc__noauth_dg_create 
(
        unsigned32 *stp
)
{
    rpc_noauth_info_p_t noauth_info;

    RPC_MEM_ALLOC (noauth_info, rpc_noauth_info_p_t, sizeof (*noauth_info), RPC_C_MEM_UTIL, RPC_C_MEM_WAITOK);

    rpc_g_noauth_alloc_count++;
    RPC_DBG_PRINTF(rpc_e_dbg_auth, 1,
        ("(rpc__noauth_dg_create) %x created (now %d active)\n", noauth_info,
            rpc_g_noauth_alloc_count - rpc_g_noauth_free_count));

    memset (noauth_info, '\0', sizeof(*noauth_info));

    RPC_MUTEX_INIT(noauth_info->lock);

    noauth_info->creds_valid = 0;
    noauth_info->level_valid = 0;
    noauth_info->client_valid = 0;

    /*
     * fill in the common auth_info stuff.
     */
    
    noauth_info->auth_info.refcount = 1;
    noauth_info->auth_info.server_princ_name = 0;
    noauth_info->auth_info.authn_level = -1;
    noauth_info->auth_info.authn_protocol = rpc_c_authn_dce_dummy;
    noauth_info->auth_info.authz_protocol = rpc_c_authz_name;
    noauth_info->auth_info.is_server = 1;
    noauth_info->auth_info.u.s.privs = 0;
    { /* FAKE-EPAC */
	noauth_info->auth_info.u.s.creds = 0;
    }
    

    /* XXX do other initialization here. */
    *stp = 0;
    return (rpc_auth_info_p_t) noauth_info;
}


/*
 * R P C _ _ N O A U T H _ D G _ I N I T
 *
 * Tell the datagram runtime about the noauth module.
 *
 */

PRIVATE rpc_protocol_id_t rpc__noauth_dg_init 
(
        rpc_auth_rpc_prot_epv_p_t       *epv,
        unsigned32                      *st
)
{
    *epv = (rpc_auth_rpc_prot_epv_p_t) (&rpc_g_noauth_dg_epv);
    *st = rpc_s_ok;
    return (RPC_C_PROTOCOL_ID_NCADG);
}

