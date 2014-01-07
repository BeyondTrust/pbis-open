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
**      schnauthcn.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The netlogon/schannel CN authentication module.
**
**
*/

#include <dce/schannel.h>
#include <schnauth.h>

#include <pwd.h>                /* for getpwuid, etc, for "level_none" */

#include <schnauthcn.h>

INTERNAL boolean32 rpc__schnauth_cn_three_way (void);

INTERNAL boolean32 rpc__schnauth_cn_context_valid (
        rpc_cn_sec_context_p_t           /*sec*/,
        unsigned32                      * /*st*/
    );

INTERNAL void rpc__schnauth_cn_create_info (
       rpc_authn_level_t                 /*authn_level*/,
       rpc_authn_flags_t                 /*authn_flags*/,
       rpc_auth_info_p_t                * /*auth_info*/,
       unsigned32                       * /*st*/
    );

INTERNAL boolean32 rpc__schnauth_cn_cred_changed (
        rpc_cn_sec_context_p_t           /*sec*/,
        unsigned32                      * /*st*/
    );

INTERNAL void rpc__schnauth_cn_cred_refresh (
        rpc_auth_info_p_t                /*auth_info*/,
        unsigned32                      * /*st*/
    );

INTERNAL void rpc__schnauth_cn_fmt_client_req (
        rpc_cn_assoc_sec_context_p_t      /* assoc_sec */,
        rpc_cn_sec_context_p_t            /* sec */,
        pointer_t                         /* auth_value */,
        unsigned32                      * /* auth_value_len */,
        pointer_t                       * /* last_auth_pos */,
        unsigned32                      * /* auth_len_remain */,
        unsigned32                        /* old_server */,
        unsigned32                      * /* st */
    );

INTERNAL void rpc__schnauth_cn_fmt_srvr_resp (
        unsigned32                       /*verify_st*/,
        rpc_cn_assoc_sec_context_p_t     /*assoc_sec*/,
        rpc_cn_sec_context_p_t           /*sec*/,
        pointer_t                        /*req_auth_value*/,
        unsigned32                       /*req_auth_value_len*/,
        pointer_t                        /*auth_value*/,
        unsigned32                      * /*auth_value_len*/
    );

INTERNAL void rpc__schnauth_cn_free_prot_info (
        rpc_auth_info_p_t                /*info*/,
        rpc_cn_auth_info_p_t            * /*cn_info*/
    );

INTERNAL void rpc__schnauth_cn_get_prot_info (
        rpc_auth_info_p_t                /*info*/,
        rpc_cn_auth_info_p_t            * /*cn_info*/,
        unsigned32                      * /*st*/
    );

INTERNAL void rpc__schnauth_cn_pre_call (
        rpc_cn_assoc_sec_context_p_t     /*assoc_sec*/,
        rpc_cn_sec_context_p_t           /*sec*/,
        pointer_t                        /*auth_value*/,
        unsigned32                      * /*auth_value_len*/,
        unsigned32                      * /*st*/
    );

INTERNAL void rpc__schnauth_cn_pre_send (
        rpc_cn_assoc_sec_context_p_t     /*assoc_sec*/,
        rpc_cn_sec_context_p_t           /*sec*/,
        rpc_socket_iovec_p_t             /*iov*/,
        unsigned32                       /*iovlen*/,
	rpc_socket_iovec_p_t             /*out_iov*/,
        unsigned32                      * /*st*/
    );

INTERNAL void rpc__schnauth_cn_recv_check (
        rpc_cn_assoc_sec_context_p_t     /*assoc_sec*/,
        rpc_cn_sec_context_p_t           /*sec*/,
        rpc_cn_common_hdr_p_t            /*pdu*/,
        unsigned32                       /*pdu_len*/,
        unsigned32                       /*cred_len*/,
        rpc_cn_auth_tlr_p_t              /*auth_tlr*/,
        boolean32                        /*unpack_ints*/,
        unsigned32                      * /*st*/
    );

INTERNAL void rpc__schnauth_cn_tlr_uuid_crc (
        pointer_t                /*auth_value*/,
        unsigned32               /*auth_value_len*/,
        unsigned32              * /*uuid_crc*/
    );

INTERNAL void rpc__schnauth_cn_tlr_unpack (
        rpc_cn_packet_p_t        /*pkt_p*/,
        unsigned32               /*auth_value_len*/,
        unsigned8               * /*packed_drep*/
    );

INTERNAL void rpc__schnauth_cn_vfy_client_req (
        rpc_cn_assoc_sec_context_p_t    /* assoc_sec */,
        rpc_cn_sec_context_p_t          /* sec */,
        pointer_t                       /* auth_value */,
        unsigned32                      /* auth_value_len */,
	unsigned32		        /* old_client */,
        unsigned32                      * /* st */
    );

INTERNAL void rpc__schnauth_cn_vfy_srvr_resp (
        rpc_cn_assoc_sec_context_p_t     /*assoc_sec*/,
        rpc_cn_sec_context_p_t           /*sec*/,
        pointer_t                        /*auth_value*/,
        unsigned32                       /*auth_value_len*/,
        unsigned32                      * /*st*/
    );

GLOBAL rpc_cn_auth_epv_t rpc_g_schnauth_cn_epv =
{
    rpc__schnauth_cn_three_way,
    rpc__schnauth_cn_context_valid,
    rpc__schnauth_cn_create_info,
    rpc__schnauth_cn_cred_changed,
    rpc__schnauth_cn_cred_refresh,
    rpc__schnauth_cn_fmt_client_req,
    rpc__schnauth_cn_fmt_srvr_resp,
    rpc__schnauth_cn_free_prot_info,
    rpc__schnauth_cn_get_prot_info,
    rpc__schnauth_cn_pre_call,
    rpc__schnauth_cn_pre_send,
    rpc__schnauth_cn_recv_check,
    rpc__schnauth_cn_tlr_uuid_crc,
    rpc__schnauth_cn_tlr_unpack,
    rpc__schnauth_cn_vfy_client_req,
    rpc__schnauth_cn_vfy_srvr_resp
};


/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_three_way
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

INTERNAL boolean32 rpc__schnauth_cn_three_way (void)
{
    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_three_way)\n"));


    return (false);
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_context_valid
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

INTERNAL boolean32 rpc__schnauth_cn_context_valid 
(
    rpc_cn_sec_context_p_t          sec,
    unsigned32                      *st
)
{
    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_context_valid)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
                    ("(rpc__schnauth_cn_context_valid) time->%x\n", time));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_context_valid) prot->%x level->%x key_id->%x\n",
                    rpc_c_authn_schannel,
                    sec->sec_info->authn_level,
                    sec->sec_key_id));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_CONTEXT_VALID))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return (false);
    }
#endif

    *st = rpc_s_ok;
    return (true);
}


/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_create_info
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

INTERNAL void rpc__schnauth_cn_create_info 
(
    rpc_authn_level_t                authn_level,
    rpc_authn_flags_t                authn_flags,
    rpc_auth_info_p_t                *auth_info,
    unsigned32                       *st
)
{
    rpc_schnauth_info_p_t schnauth_info;

    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_create_info)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_create_info) prot->%x level->%x\n",
                    rpc_c_authn_schannel,
                    authn_level));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_CREATE_INFO))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif

    /*
     * Allocate storage for a schnauth_info structure from heap.
     */
    RPC_MEM_ALLOC(schnauth_info, 
		  rpc_schnauth_info_p_t, 
		  sizeof (rpc_schnauth_info_t), 
		  RPC_C_MEM_SCHNAUTH_INFO, 
		  RPC_C_MEM_WAITOK);

    /*
     * Initialize it.
     */
    memset(schnauth_info, 0, sizeof(rpc_schnauth_info_t));
    RPC_MUTEX_INIT (schnauth_info->lock);

    /*
     * Initialize the common auth_info stuff.
     */
    schnauth_info->auth_info.refcount = 1;
    schnauth_info->auth_info.server_princ_name = '\0';
    schnauth_info->auth_info.authn_level = authn_level;
    schnauth_info->auth_info.authn_protocol = rpc_c_authn_schannel;
    schnauth_info->auth_info.authz_protocol = rpc_c_authz_name;
    schnauth_info->auth_info.is_server = true;

    *auth_info = (rpc_auth_info_t *) schnauth_info;
    *st = rpc_s_ok;
}


/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_cred_changed
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

INTERNAL boolean32 rpc__schnauth_cn_cred_changed 
(
    rpc_cn_sec_context_p_t          sec,
    unsigned32                      *st
)
{
    rpc_schnauth_cn_info_t *schnauth_cn_info ATTRIBUTE_UNUSED;
    rpc_schnauth_info_p_t   schnuth_info ATTRIBUTE_UNUSED;
    boolean32               different_creds;

    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_cred_changed)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_cred_changed) prot->%x level->%x key_id->%x\n",
                    rpc_c_authn_dce_private,
                    sec->sec_info->authn_level,
                    sec->sec_key_id));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_CRED_CHANGED))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return (false);
    }
#endif

    /*
     * Assume that cred is already valid.
     */
    different_creds = false;
    *st = rpc_s_ok;
    return (different_creds);
}


/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_cred_refresh
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

INTERNAL void rpc__schnauth_cn_cred_refresh 
(
    rpc_auth_info_p_t               auth_info,
    unsigned32                      *st
)
{
    rpc_schnauth_info_p_t schnauth_info ATTRIBUTE_UNUSED;

    schnauth_info = (rpc_schnauth_info_p_t)auth_info;

    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_cred_refresh)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_cred_refresh) prot->%x level->%x\n",
                    rpc_c_authn_dce_private,
                    auth_info->authn_level));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_CRED_REFRESH))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif

    /*
     * Assume that cred is already valid.
     */
    *st = rpc_s_ok;
}

INTERNAL unsigned32 rpc__schnauth_cn_inq_flags_supported
(
	unsigned32 authn_flags
)
{
	unsigned st = rpc_s_ok;

	if (authn_flags)
	{
		st = rpc_s_unsupported_authn_level;;
	}

	return st;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_fmt_client_req
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

INTERNAL void rpc__schnauth_cn_fmt_client_req 
(
   rpc_cn_assoc_sec_context_p_t      assoc_sec,
   rpc_cn_sec_context_p_t            sec,
   pointer_t                         auth_value,
   unsigned32                      * auth_value_len,
   pointer_t                       * last_auth_pos ATTRIBUTE_UNUSED,
   unsigned32                      * auth_len_remain ATTRIBUTE_UNUSED,
   unsigned32                        old_server ATTRIBUTE_UNUSED,
   unsigned32                      * st
)
{
    rpc_cn_bind_auth_value_priv_t       *priv_auth_value ATTRIBUTE_UNUSED;
    rpc_cn_auth_info_t                  *auth_info;
    rpc_schnauth_cn_info_t              *schnauth_info;
    struct schn_blob                    schn_creds;

    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_fmt_client_req)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_fmt_client_req) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x\n",
                    rpc_c_authn_schannel,
                    sec->sec_info->authn_level,
                    sec->sec_key_id,
                    assoc_sec->assoc_uuid_crc,
                    assoc_sec->assoc_next_snd_seq,
                    assoc_sec->assoc_next_rcv_seq));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_FMT_CLIENT_REQ))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif
    /*
     * Check if requested authn_flags are supported by this mechanism
     */
    *st = rpc__schnauth_cn_inq_flags_supported(sec->sec_info->authn_flags);
    if (*st != rpc_s_ok)
    {
        return;
    }

    auth_info = sec->sec_cn_info;
    schnauth_info = (rpc_schnauth_cn_info_t*)auth_info;

    schn_creds.base = (uint8*)auth_value;
    schn_creds.len  = 0;

    schn_init_creds(&schnauth_info->sec_ctx, &schn_creds);

    *auth_value_len = (unsigned32) schn_creds.len;

    *st = rpc_s_ok;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_fmt_srvr_resp
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

INTERNAL void rpc__schnauth_cn_fmt_srvr_resp 
(
    unsigned32                      verify_st,
    rpc_cn_assoc_sec_context_p_t    assoc_sec,
    rpc_cn_sec_context_p_t          sec,
    pointer_t                       req_auth_value ATTRIBUTE_UNUSED,
    unsigned32                      req_auth_value_len ATTRIBUTE_UNUSED,
    pointer_t                       auth_value,
    unsigned32                      *auth_value_len
)
{
    rpc_cn_bind_auth_value_priv_t       *priv_auth_value;

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_fmt_srvr_resp)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_fmt_srvr_resp) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x, vfy_client_st->%x\n",
                    rpc_c_authn_schannel,
                    sec->sec_info->authn_level,
                    sec->sec_key_id,
                    assoc_sec->assoc_uuid_crc,
                    assoc_sec->assoc_next_snd_seq,
                    assoc_sec->assoc_next_rcv_seq,
                    verify_st));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_FMT_SERVER_RESP))
    {
        verify_st = RPC_S_CN_DBG_AUTH_FAILURE;
    }
#endif

    assert (verify_st == rpc_s_ok);
    assert (*auth_value_len >= RPC_CN_PKT_SIZEOF_BIND_AUTH_VAL);
    *auth_value_len = RPC_CN_PKT_SIZEOF_BIND_AUTH_VAL;

    priv_auth_value = (rpc_cn_bind_auth_value_priv_t *)auth_value;
    priv_auth_value->assoc_uuid_crc = assoc_sec->assoc_uuid_crc;
    priv_auth_value->sub_type = RPC_C_CN_DCE_SUB_TYPE;
    priv_auth_value->checksum_length = 0;
    priv_auth_value->cred_length = 0;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_free_prot_info
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

INTERNAL void rpc__schnauth_cn_free_prot_info 
(
    rpc_auth_info_p_t               info,    
    rpc_cn_auth_info_p_t            *cn_info
)
{

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_free_prot_info)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_free_prot_info) prot->%x level->%x \n",
                    rpc_c_authn_schannel,
                    info->authn_level));

#ifdef DEBUG
    memset (*cn_info, 0, sizeof (rpc_cn_auth_info_t));
#endif

    RPC_MEM_FREE (*cn_info, RPC_C_MEM_SCHNAUTH_CN_INFO);
    *cn_info = NULL;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_get_prot_info
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

INTERNAL void rpc__schnauth_cn_get_prot_info 
(
    rpc_auth_info_p_t               info,
    rpc_cn_auth_info_p_t            *cn_info,
    unsigned32                      *st
)
{
    rpc_schnauth_info_t           *schnauth_info;
    rpc_schnauth_cn_info_t        *schnauth_cn_info;

    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_get_prot_info)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_get_prot_info) prot->%x level->%x \n",
                    rpc_c_authn_schannel,
                    info->authn_level));

    schnauth_info = (rpc_schnauth_info_t*)info;

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_GET_PROT_INFO))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif

    /*
     * Allocate storage for a schnauth cn info structure from heap.
     */
    RPC_MEM_ALLOC (schnauth_cn_info, 
                   rpc_schnauth_cn_info_p_t, 
                   sizeof (rpc_schnauth_cn_info_t), 
                   RPC_C_MEM_SCHNAUTH_CN_INFO, 
                   RPC_C_MEM_WAITOK);

    /*
     * Initialize it.
     */
    memset (schnauth_cn_info, 0, sizeof(rpc_schnauth_cn_info_t));

    /* Set entry points to schannel functions */
    schnauth_cn_info->cn_info.cn_epv = &rpc_g_schnauth_cn_epv;

    /* Copy available schannel init credentials - temporary hack */
    memcpy(&schnauth_cn_info->sec_ctx,
	   &schnauth_info->sec_ctx,
	   sizeof(struct schn_auth_ctx));

    *cn_info = (rpc_cn_auth_info_t *)schnauth_cn_info;
    *st = rpc_s_ok;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_pre_call
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

INTERNAL void rpc__schnauth_cn_pre_call 
(
    rpc_cn_assoc_sec_context_p_t    assoc_sec,
    rpc_cn_sec_context_p_t          sec,
    pointer_t                       auth_value ATTRIBUTE_UNUSED,
    unsigned32                      *auth_value_len,
    unsigned32                      *st
)
{
    rpc_cn_auth_value_priv_t    *priv_auth_value ATTRIBUTE_UNUSED;

    CODING_ERROR(st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_pre_call)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_pre_call) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x\n",
                    rpc_c_authn_schannel,
                    sec->sec_info->authn_level,
                    sec->sec_key_id,
                    assoc_sec->assoc_uuid_crc,
                    assoc_sec->assoc_next_snd_seq,
                    assoc_sec->assoc_next_rcv_seq));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_PRE_CALL))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif

    *auth_value_len = sizeof(rpc_cn_schnauth_tlr_t);

    *st = rpc_s_ok;
}


/*
 * Sign the packet prior to sending
 */

INTERNAL void rpc__schnauth_cn_wrap_pdu
(
    rpc_cn_assoc_sec_context_p_t    assoc_sec ATTRIBUTE_UNUSED,
    rpc_cn_sec_context_p_t          sec,
    rpc_socket_iovec_p_t            iov,
    unsigned32                      iovlen,
    rpc_socket_iovec_p_t            out_iov,
    unsigned32                      *st
)
{
    const unsigned32 hdr_idx = 0;
    unsigned32 status = rpc_s_ok;
    rpc_schnauth_cn_info_p_t schn_auth;
    rpc_cn_common_hdr_p_t com_hdr;
    rpc_cn_auth_tlr_p_t com_tlr;
    rpc_cn_schnauth_tlr_p_t schn_tlr;
    unsigned32 tlr_idx;
    unsigned32 part_idx;
    unsigned_char_p_t pdu_blob;
    unsigned32 pdu_blob_len;
    unsigned16 packet_len, stub_len, auth_pad_len;
    pointer_t base;
    uint32 sec_level;
    struct schn_auth_ctx *schn_ctx = NULL;
    struct schn_blob input_token, output_token;
    struct schn_tail tail;

    schn_auth = (rpc_schnauth_cn_info_p_t)sec->sec_cn_info;
    schn_ctx  = &schn_auth->sec_ctx;

    tlr_idx = iovlen - 1;
    com_tlr = (rpc_cn_auth_tlr_p_t)(iov[tlr_idx].iov_base);
    schn_tlr = (rpc_cn_schnauth_tlr_p_t)(com_tlr->auth_value);
    com_hdr = (rpc_cn_common_hdr_p_t)(iov[hdr_idx].iov_base);


    /*
     * Calculate header + stub length (including initial padding)
     */

    part_idx = hdr_idx;
    stub_len = 0;
    while (part_idx < tlr_idx) {
	stub_len += iov[part_idx++].iov_len;
    }

    stub_len -= RPC_CN_PKT_SIZEOF_RQST_HDR;

    /* ensure auth trailer padding */
    auth_pad_len = stub_len % 0x0008;

    /*
     * Allocate the entire packet in one piece
     */
    packet_len = stub_len + (RPC_CN_PKT_SIZEOF_RQST_HDR +
			     RPC_CN_PKT_SIZEOF_COM_AUTH_TLR +
			     RPC_CN_PKT_SIZEOF_SCHNAUTH_TLR +
			     auth_pad_len);

    out_iov->iov_len = packet_len;
    RPC_MEM_ALLOC(out_iov->iov_base,
		  pointer_t,
		  out_iov->iov_len,
		  RPC_C_MEM_SCHNAUTH_INFO,
		  RPC_C_MEM_WAITOK);
    if (out_iov->iov_base == NULL) {
        *st = rpc_s_no_memory;
	return;
    }

    memset(out_iov->iov_base, 0, out_iov->iov_len);

    /* Update packet and padding length after adding auth padding bytes */
    com_hdr->frag_len        += auth_pad_len;
    com_tlr->stub_pad_length += auth_pad_len;

    /*
     * Create the packet blob including auth trailer padding
     */

    base = out_iov->iov_base;

    /* header [idx = 0] */
    memcpy(base, (void*)com_hdr, RPC_CN_PKT_SIZEOF_COMMON_HDR);
    base += RPC_CN_PKT_SIZEOF_COMMON_HDR;

    /* stub [idx = 0] (the rest of it following header) */
    memcpy(base, iov[hdr_idx].iov_base + RPC_CN_PKT_SIZEOF_COMMON_HDR,
	   iov[hdr_idx].iov_len - RPC_CN_PKT_SIZEOF_COMMON_HDR);
    base += iov[hdr_idx].iov_len - RPC_CN_PKT_SIZEOF_COMMON_HDR;

    /* stub [idx = 1 .. (tlr_idx - 1)] (if there is any remaining part) */
    part_idx = 1;
    while (part_idx < tlr_idx) {
	/* This packet is longer than 2 parts (header+stub and trailer)
	   so make sure we copy all of them */
	memcpy(base, iov[part_idx].iov_base, iov[part_idx].iov_len);
	base += iov[part_idx].iov_len;
	part_idx++;
    }

    /* auth padding */
    base += auth_pad_len;

    /* common trailer [idx = tlr_idx] */
    memcpy(base, (void*)com_tlr, RPC_CN_PKT_SIZEOF_COM_AUTH_TLR);    
    base += RPC_CN_PKT_SIZEOF_COM_AUTH_TLR;

    /* schannel trailer */
    memcpy(base, (void*)schn_tlr, RPC_CN_PKT_SIZEOF_SCHNAUTH_TLR);
    base += RPC_CN_PKT_SIZEOF_SCHNAUTH_TLR;

    /* set PDU blob pointer and length */
    pdu_blob = out_iov->iov_base + RPC_CN_PKT_SIZEOF_RQST_HDR;
    pdu_blob_len = stub_len + auth_pad_len;

    /* set new location of common and schannel trailer */
    com_tlr = (rpc_cn_auth_tlr_p_t)(pdu_blob + pdu_blob_len);
    schn_tlr = (rpc_cn_schnauth_tlr_p_t)(com_tlr->auth_value);

    switch (com_tlr->auth_level) {
    case rpc_c_protect_level_pkt_integ:
        sec_level = SCHANNEL_SEC_LEVEL_INTEGRITY;
	break;

    case rpc_c_protect_level_pkt_privacy:
        sec_level = SCHANNEL_SEC_LEVEL_PRIVACY;
	break;

    default:
	*st = rpc_s_unsupported_protect_level;
	return;
    }

    input_token.base = (uint8*)pdu_blob;
    input_token.len  = pdu_blob_len;

    status = schn_wrap(schn_ctx, sec_level, &input_token, &output_token, &tail);

    memcpy(pdu_blob, output_token.base, output_token.len);

    memcpy(schn_tlr->signature, tail.signature, 8);
    memcpy(schn_tlr->digest, tail.digest, 8);
    memcpy(schn_tlr->seq_number, tail.seq_number, 8);
    memcpy(schn_tlr->nonce, tail.nonce, 8);

    schn_free_blob(&output_token);

    *st = status;
}


/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_pre_send
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
**                      copied, out_iov->base will be non-NULL.
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

INTERNAL void rpc__schnauth_cn_pre_send 
(
    rpc_cn_assoc_sec_context_p_t    assoc_sec,
    rpc_cn_sec_context_p_t          sec,
    rpc_socket_iovec_p_t            iov,
    unsigned32                      iovlen,
    rpc_socket_iovec_p_t            out_iov,
    unsigned32                      *st
)
{
    unsigned32          ptype;

    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_pre_send)\n"));

    ptype = ((rpc_cn_common_hdr_t *)(iov[0].iov_base))->ptype;

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
                    ("(rpc__schnauth_cn_pre_send) authn level->%x packet type->%x\n", sec->sec_info->authn_level, ptype));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_pre_send) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x ptype->%x\n",
                    rpc_c_authn_schannel,
                    sec->sec_info->authn_level,
                    sec->sec_key_id,
                    assoc_sec->assoc_uuid_crc,
                    assoc_sec->assoc_next_snd_seq,
                    assoc_sec->assoc_next_rcv_seq,
                    ptype));
#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_PRE_SEND))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif

    out_iov->iov_base = NULL;
    switch (ptype)
    {
        case RPC_C_CN_PKT_REQUEST:
        case RPC_C_CN_PKT_RESPONSE:
        {
            rpc__schnauth_cn_wrap_pdu(assoc_sec, sec, iov, iovlen, out_iov, st);
            return;
        }

        case RPC_C_CN_PKT_FAULT:
        case RPC_C_CN_PKT_BIND:
        case RPC_C_CN_PKT_BIND_ACK:
        case RPC_C_CN_PKT_BIND_NAK:
        case RPC_C_CN_PKT_ALTER_CONTEXT:
        case RPC_C_CN_PKT_ALTER_CONTEXT_RESP:
        case RPC_C_CN_PKT_AUTH3:
        case RPC_C_CN_PKT_SHUTDOWN:
        case RPC_C_CN_PKT_REMOTE_ALERT:
        case RPC_C_CN_PKT_ORPHANED:
        default:
        {
            break;
        }
    }

    *st = rpc_s_ok;
}


INTERNAL void rpc__schnauth_cn_unwrap_pdu
(
    rpc_cn_assoc_sec_context_p_t    assoc_sec ATTRIBUTE_UNUSED,
    rpc_cn_sec_context_p_t          sec,
    rpc_cn_common_hdr_p_t           pdu,
    unsigned32                      pdu_len,
    unsigned32                      cred_len,
    rpc_cn_auth_tlr_p_t             auth_tlr,
    boolean32                       unpack_ints,
    unsigned32                      *st
)
{
    unsigned32 status = rpc_s_ok;
    rpc_schnauth_cn_info_p_t schn_auth;
    rpc_cn_schnauth_tlr_p_t schn_tlr;
    unsigned32 sec_level;
    struct schn_auth_ctx *sec_ctx;
    struct schn_blob input_token, output_token;
    struct schn_tail tail;
    unsigned16 auth_len = 0;

    schn_auth = (rpc_schnauth_cn_info_p_t)sec->sec_cn_info;
    schn_tlr = (rpc_cn_schnauth_tlr_p_t)auth_tlr->auth_value;

    switch (auth_tlr->auth_level) {
    case rpc_c_protect_level_pkt_integ:
        sec_level = SCHANNEL_SEC_LEVEL_INTEGRITY;
	break;

    case rpc_c_protect_level_pkt_privacy:
        sec_level = SCHANNEL_SEC_LEVEL_PRIVACY;
	break;

    default:
	*st = rpc_s_unsupported_protect_level;
	return;
    }

    sec_ctx = &schn_auth->sec_ctx;

    auth_len = RPC_CN_PKT_AUTH_LEN((rpc_cn_packet_p_t)pdu);
    if (unpack_ints) {
        SWAB_INPLACE_16(auth_len);
    }

    input_token.base = (uint8*)(pdu) + RPC_CN_PKT_SIZEOF_RESP_HDR;
    input_token.len  = pdu_len - (RPC_CN_PKT_SIZEOF_RESP_HDR +
				  RPC_CN_PKT_SIZEOF_COM_AUTH_TLR +
				  auth_len);

    output_token.base = NULL;
    output_token.len  = 0;

    memcpy(tail.signature,   schn_tlr->signature,  8);
    memcpy(tail.seq_number,  schn_tlr->seq_number, 8);
    memcpy(tail.digest,      schn_tlr->digest,     8);
    memcpy(tail.nonce,       schn_tlr->nonce,      8);

    status = schn_unwrap(sec_ctx, sec_level, &input_token, &output_token,
			 &tail);

    memcpy(input_token.base, output_token.base, output_token.len);
    schn_free_blob(&output_token);

    *st = status;
}


/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_recv_check
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

INTERNAL void rpc__schnauth_cn_recv_check 
(
    rpc_cn_assoc_sec_context_p_t    assoc_sec,
    rpc_cn_sec_context_p_t          sec,
    rpc_cn_common_hdr_p_t           pdu,
    unsigned32                      pdu_len ATTRIBUTE_UNUSED,
    unsigned32                      cred_len ATTRIBUTE_UNUSED,
    rpc_cn_auth_tlr_p_t             auth_tlr,
    boolean32                       unpack_ints ATTRIBUTE_UNUSED,
    unsigned32                      *st
)
{
    rpc_cn_auth_value_priv_t    *priv_auth_value;
    unsigned32                  ptype;
    unsigned32                  authn_level ATTRIBUTE_UNUSED;
    unsigned32                  assoc_uuid_crc ATTRIBUTE_UNUSED;

    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_recv_check)\n"));

    ptype = pdu->ptype;
    priv_auth_value = (rpc_cn_auth_value_priv_t *)(auth_tlr->auth_value);
    authn_level = auth_tlr->auth_level;
    if (ptype == RPC_C_CN_PKT_BIND)
    {
        assoc_uuid_crc = ((rpc_cn_bind_auth_value_priv_t *)priv_auth_value)->assoc_uuid_crc;
    }
    else
    {
        assoc_uuid_crc = assoc_sec->assoc_uuid_crc;
    }

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
                    ("(rpc__schnauth_cn_recv_check) authn level->%x packet type->%x\n", authn_level, ptype));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_recv_check) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x ptype->%x\n",
                    rpc_c_authn_schannel,
                    authn_level,
                    sec->sec_key_id,
                    assoc_uuid_crc,
                    assoc_sec->assoc_next_snd_seq,
                    assoc_sec->assoc_next_rcv_seq,
                    ptype));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_RECV_CHECK))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif

    switch (ptype)
    {
        case RPC_C_CN_PKT_REQUEST:
        case RPC_C_CN_PKT_RESPONSE:
	{
	    rpc__schnauth_cn_unwrap_pdu(assoc_sec, sec, pdu, pdu_len, cred_len,
					auth_tlr, unpack_ints, st);
            return;
        }
        case RPC_C_CN_PKT_BIND:
        case RPC_C_CN_PKT_ALTER_CONTEXT:
            if (pdu->flags & RPC_C_CN_FLAGS_SUPPORT_HEADER_SIGN)
            {
                *st = rpc__schnauth_cn_inq_flags_supported(rpc_c_protect_flags_header_sign);
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
        {
            break;
        }
    }

    *st = rpc_s_ok;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_tlr_uuid_crc
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

INTERNAL void rpc__schnauth_cn_tlr_uuid_crc 
(
    pointer_t               auth_value,
    unsigned32              auth_value_len,
    unsigned32              *uuid_crc
)
{
    rpc_cn_bind_auth_value_priv_t       *priv_auth_value;

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_tlr_uuid_crc)\n"));

    assert (auth_value_len >= RPC_CN_PKT_SIZEOF_BIND_AUTH_VAL);
    priv_auth_value = (rpc_cn_bind_auth_value_priv_t *)auth_value;
    *uuid_crc = priv_auth_value->assoc_uuid_crc;

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_GENERAL,
                    ("(rpc__schnauth_cn_tlr_uuid_crc) assoc_uuid_crc->%x\n", *uuid_crc));
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_tlr_unpack
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

INTERNAL void rpc__schnauth_cn_tlr_unpack 
(
    rpc_cn_packet_p_t       pkt_p ATTRIBUTE_UNUSED,
    unsigned32              auth_value_len ATTRIBUTE_UNUSED,
    unsigned8               *packed_drep ATTRIBUTE_UNUSED
)
{
    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_tlr_unpack)\n"));
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_vfy_client_req
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

INTERNAL void rpc__schnauth_cn_vfy_client_req 
(
    rpc_cn_assoc_sec_context_p_t    assoc_sec,
    rpc_cn_sec_context_p_t          sec,
    pointer_t                       auth_value,
    unsigned32                      auth_value_len ATTRIBUTE_UNUSED,
    unsigned32		            old_client ATTRIBUTE_UNUSED,
    unsigned32                      *st
)
{
    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_vfy_client_req)\n"));
    
    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_vfy_client_req) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x\n",
                    rpc_c_authn_schannel,
                    sec->sec_info->authn_level,
                    sec->sec_key_id,
                    assoc_sec->assoc_uuid_crc,
                    assoc_sec->assoc_next_snd_seq,
                    assoc_sec->assoc_next_rcv_seq));

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_VFY_CLIENT_REQ))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif

    *st = rpc_s_ok;
}

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__schnauth_cn_vfy_srvr_resp
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

INTERNAL void rpc__schnauth_cn_vfy_srvr_resp 
(
    rpc_cn_assoc_sec_context_p_t    assoc_sec,
    rpc_cn_sec_context_p_t          sec,
    pointer_t                       auth_value ATTRIBUTE_UNUSED,
    unsigned32                      auth_value_len ATTRIBUTE_UNUSED,
    unsigned32                      *st
)
{
    CODING_ERROR (st);

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_vfy_srvr_resp)\n"));

    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_PKT,
                    ("(rpc__schnauth_cn_vfy_server_resp) prot->%x level->%x key_id->%x assoc_uuid_crc->%x xmit_seq->%x recv_seq->%x\n",
                    rpc_c_authn_schannel,
                    sec->sec_info->authn_level,
                    sec->sec_key_id,
                    assoc_sec->assoc_uuid_crc,
                    assoc_sec->assoc_next_snd_seq,
                    assoc_sec->assoc_next_rcv_seq));


#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_AUTH_VFY_SERVER_RESP))
    {
        *st = RPC_S_CN_DBG_AUTH_FAILURE;
        return;
    }
#endif

    *st = rpc_s_ok;
}


PRIVATE rpc_protocol_id_t       rpc__schnauth_cn_init 
(
    rpc_auth_rpc_prot_epv_p_t       *epv,
    unsigned32                      *st
)
{
    CODING_ERROR (st);
    RPC_DBG_PRINTF (rpc_e_dbg_auth, RPC_C_CN_DBG_AUTH_ROUTINE_TRACE,
                    ("(rpc__schnauth_cn_init)\n"));

    *epv = (rpc_auth_rpc_prot_epv_p_t) (&rpc_g_schnauth_cn_epv);
    *st = rpc_s_ok;
    return (RPC_C_PROTOCOL_ID_NCACN);
}
