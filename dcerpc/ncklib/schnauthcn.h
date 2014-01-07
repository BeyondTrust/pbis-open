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
#ifndef _SCHNAUTHCN_H
#define _SCHNAUTHCN_H	1

/*
**  NAME
**
**      schnauthcn.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The netlogon/schannel CN authentication module interface.
**
**
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cn.h>

#include <schannel.h>

typedef struct rpc_schnauth_cn_info
{
    rpc_cn_auth_info_t  cn_info;

    /*
     * Schannel security context
     */

    struct schn_auth_ctx sec_ctx;

} rpc_schnauth_cn_info_t, *rpc_schnauth_cn_info_p_t;


typedef struct rpc_schnauth_creds
{
    unsigned32 flags1;
    unsigned32 flags2;
    unsigned_char_p_t domain_name;
    unsigned_char_p_t machine_name;
} rpc_schnauth_creds_t, *rpc_schnauth_creds_p_t;


typedef struct rpc_cn_schnauth_tlr
{
    unsigned8 signature[8];
    unsigned8 seq_number[8];
    unsigned8 digest[8];
    unsigned8 nonce[8];

} rpc_cn_schnauth_tlr_t, *rpc_cn_schnauth_tlr_p_t;

#define RPC_CN_PKT_SIZEOF_SCHNAUTH_TLR  32


EXTERNAL rpc_cn_auth_epv_t rpc_g_schnauth_cn_epv;

#endif /* _SCHNAUTHCN_H */
