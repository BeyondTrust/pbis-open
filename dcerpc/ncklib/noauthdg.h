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
#ifndef _NOAUTHDG_H
#define _NOAUTHDG_H	1
/*
**
**  NAME
**
**      noauthdgp.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definition of types private to the noauth-datagram glue module.
**
**
*/

#define NCK_NEED_MARSHALLING

#include <dg.h>
#include <noauth.h>
#include <dce/conv.h>

#include <dce/dce.h>


/*
 * For various reasons, it's painful to get at the NDR tag of the
 * underlying data, so we cheat and just encode it in big-endian order.
 */

#define rpc_marshall_be_long_int(mp, bei) \
{       long temp = htonl(bei);            \
        rpc_marshall_long_int (mp, temp);  \
}
      
#define rpc_convert_be_long_int(mp, bei) \
{                                       \
    rpc_unmarshall_long_int(mp, bei);   \
    bei = ntohl(bei);                   \
}

#define rpc_marshall_be_short_int(mp, bei) \
{       short temp = htons(bei);            \
        rpc_marshall_short_int (mp, temp);  \
}
      
#define rpc_convert_be_short_int(mp, bei) \
{                                       \
    rpc_unmarshall_short_int(mp, bei);   \
    bei = ntohs(bei);                   \
}


/*
 * DG EPV routines.
 */


#ifdef __cplusplus
extern "C" {
#endif


void rpc__noauth_dg_pre_call (                         
        rpc_auth_info_p_t               ,
        handle_t                        ,
        unsigned32                      *
    );

rpc_auth_info_p_t rpc__noauth_dg_create (
        unsigned32                      * /*st*/
    );

void rpc__noauth_dg_encrypt (
        rpc_auth_info_p_t                /*info*/,
        rpc_dg_xmitq_elt_p_t            ,
        unsigned32                      * /*st*/
    );

void rpc__noauth_dg_pre_send (                         
        rpc_auth_info_p_t                /*info*/,
        rpc_dg_xmitq_elt_p_t             /*pkt*/,
        rpc_dg_pkt_hdr_p_t               /*hdrp*/,
        rpc_socket_iovec_p_t             /*iov*/,
        int                              /*iovlen*/,
        pointer_t                        /*cksum*/,
        unsigned32                      * /*st*/
    );

void rpc__noauth_dg_recv_ck (                         
        rpc_auth_info_p_t                /*info*/,
        rpc_dg_recvq_elt_p_t             /*pkt*/,
        pointer_t                        /*cksum*/,
        error_status_t                  * /*st*/
    );

void rpc__noauth_dg_who_are_you (                         
        rpc_auth_info_p_t                /*info*/,
        handle_t                        ,
        dce_uuid_t                          *,
        unsigned32                      ,
        unsigned32                      *,
        dce_uuid_t                          *,
        unsigned32                      *
    );

void rpc__noauth_dg_way_handler (
        rpc_auth_info_p_t                /*info*/,
        ndr_byte                        * /*in_data*/,
        signed32                         /*in_len*/,
        signed32                         /*out_max_len*/,
        ndr_byte                        * /*out_data*/,
        signed32                        * /*out_len*/,
        unsigned32                      * /*st*/
    );

#ifdef __cplusplus
}
#endif

#endif /* _NOAUTHDG_H */
