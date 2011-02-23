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
#ifndef _DGRQ_H
#define _DGRQ_H
/*
**
**  NAME:
**
**      dgrq.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)                                 
**
**  ABSTRACT:
**
**  DG protocol service routines
**
**
*/

#include <dce/dce.h>

/*
 * R P C _ D G _ R E C V Q _ E L T _ F R O M _ I O V E C T O R _ E L T
 *
 * Given an IO vector element, return the associated receive queue element.
 * This macro is used by internal callers of the "comm_receive/transceive"
 * path so that can get back a receieve queue element and look at the
 * header.  This macro depends on how "comm_receive" works.
 */

#define RPC_DG_RECVQ_ELT_FROM_IOVECTOR_ELT(iove) \
    ((rpc_dg_recvq_elt_p_t) (iove)->buff_addr)

/*
 * R P C _ D G _ R E C V Q _ R E I N I T
 *
 * Reinitialize a receive queue.
 */

#define RPC_DG_RECVQ_REINIT(rq) { \
    if ((rq)->head != NULL) rpc__dg_recvq_free(rq); \
    rpc__dg_recvq_init(rq);     /* !!! Maybe be smarter later -- this may be losing "history" */ \
}

/*
 * R P C _ D G _ R E C V Q _ I O V E C T O R _ S E T U P
 *
 * Setup the return iovector element.
 *
 * NOTE WELL that other logic depends on the fact that the "buff_addr"
 * field of iovector elements points to an "rpc_dg_recvq_elt_t" (rqe).
 * See comments by RPC_DG_RECVQ_ELT_FROM_IOVECTOR_ELT.
 */

#define RPC_DG_RECVQ_IOVECTOR_SETUP(data, rqe) { \
    (data)->buff_dealloc  = (rpc_buff_dealloc_fn_t) rpc__dg_pkt_free_rqe_for_stub; \
    (data)->buff_addr     = (byte_p_t) (rqe); \
    (data)->buff_len      = sizeof *(rqe); \
    (data)->data_addr     = (byte_p_t) &(rqe)->pkt->body; \
    (data)->data_len      = ((rqe)->hdrp != NULL) ? \
                                MIN((rqe)->hdrp->len, \
                                    (rqe)->pkt_len - RPC_C_DG_RAW_PKT_HDR_SIZE) : \
                                (rqe)->pkt_len; \
}

PRIVATE void rpc__dg_recvq_init ( rpc_dg_recvq_p_t  /*rq*/);


PRIVATE void rpc__dg_recvq_free ( rpc_dg_recvq_p_t  /*rq*/);

#endif /* _DGRQ_H */
