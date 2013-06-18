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
**  NAME:
**
**      dgutl.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Utility routines for the NCA RPC datagram protocol implementation.
**
**
*/

#ifndef _DGUTL_H
#define _DGUTL_H

/* ========================================================================= */

#include <dce/dce.h>


#ifndef RPC_DG_PLOG

#define RPC_DG_PLOG_RECVFROM_PKT(hdrp, bodyp)
#define RPC_DG_PLOG_SENDMSG_PKT(iov, iovlen)
#define RPC_DG_PLOG_LOSSY_SENDMSG_PKT(iov, iovlen, lossy_action)
#define rpc__dg_plog_pkt(hdrp, bodyp, recv, lossy_action)

#else

#define RPC_DG_PLOG_RECVFROM_PKT(hdrp, bodyp) \
    { \
        if (RPC_DBG(rpc_es_dbg_dg_pktlog, 100)) \
            rpc__dg_plog_pkt((hdrp), (bodyp), true, 0); \
    }

#define RPC_DG_PLOG_SENDMSG_PKT(iov, iovlen) \
    { \
        if (RPC_DBG(rpc_es_dbg_dg_pktlog, 100)) \
            rpc__dg_plog_pkt((rpc_dg_raw_pkt_hdr_p_t) (iov)[0].base,  \
                    (iovlen) < 2 ? NULL : (rpc_dg_pkt_body_p_t) (iov)[1].base,  \
                    false, 3); \
    }

#define RPC_DG_PLOG_LOSSY_SENDMSG_PKT(iov, iovlen, lossy_action) \
    { \
        if (RPC_DBG(rpc_es_dbg_dg_pktlog, 100)) \
            rpc__dg_plog_pkt((rpc_dg_raw_pkt_hdr_p_t) (iov)[0].base,  \
                    (iovlen) < 2 ? NULL : (rpc_dg_pkt_body_p_t) (iov)[1].base,  \
                    false, lossy_action); \
    }


PRIVATE void rpc__dg_plog_pkt (
        rpc_dg_raw_pkt_hdr_p_t  /*hdrp*/,
        rpc_dg_pkt_body_p_t  /*bodyp*/,
        boolean32  /*recv*/,
        unsigned32  /*lossy_action*/
    );

PRIVATE void rpc__dg_plog_dump (
         /*void*/
    );

#endif

/* ========================================================================= */

PRIVATE void rpc__dg_xmit_pkt (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t  /*addr*/,
        rpc_socket_iovec_p_t  /*iov*/,
        int  /*iovlen*/,
        boolean * /*b*/
    );

PRIVATE void rpc__dg_xmit_hdr_only_pkt (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t  /*addr*/,
        rpc_dg_pkt_hdr_p_t  /*hdrp*/,
        rpc_dg_ptype_t  /*ptype*/
    );

PRIVATE void rpc__dg_xmit_error_body_pkt (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t  /*addr*/,
        rpc_dg_pkt_hdr_p_t  /*hdrp*/,
        rpc_dg_ptype_t  /*ptype*/,
        unsigned32  /*errst*/
    );

PRIVATE char *rpc__dg_act_seq_string (
        rpc_dg_pkt_hdr_p_t  /*hdrp*/
    );

PRIVATE char *rpc__dg_pkt_name (
        rpc_dg_ptype_t  /*ptype*/
    );

PRIVATE unsigned16 rpc__dg_uuid_hash (
        dce_uuid_p_t  /*uuid*/
    );

#endif /* _DGUTL_H */
