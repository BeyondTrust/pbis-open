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
#ifndef _DGXQ_H
#define _DGXQ_H
/*
**
**  NAME:
**
**      dgxq.h
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
 * R P C _ D G _ X M I T Q _ R E I N I T
 *
 * Reinitialize a transmit queue.
 */

#define RPC_DG_XMITQ_REINIT(xq, call) { \
    if ((xq)->head != NULL) rpc__dg_xmitq_free(xq, call); \
    rpc__dg_xmitq_reinit(xq); \
}

/*
 * R P C _ D G _ X M I T Q _ A W A I T I N G _ A C K _ S E T
 *
 * Mark a transmit queue as wanting an acknowledgement event (fack, response,
 * working, etc.)
 */

#define RPC_DG_XMITQ_AWAITING_ACK_SET(xq) ( \
    (xq)->awaiting_ack = true, \
    xq->awaiting_ack_timestamp = rpc__clock_stamp() \
)

/*
 * R P C _ D G _ X M I T Q _ A W A I T I N G _ A C K _ C L R
 *
 * Mark a transmit queue as no longer wanting an acknowledgement event.
 */

#define RPC_DG_XMITQ_AWAITING_ACK_CLR(xq) ( \
    (xq)->awaiting_ack = false \
)

PRIVATE void rpc__dg_xmitq_elt_xmit (
        rpc_dg_xmitq_elt_p_t  /*xqe*/,
        rpc_dg_call_p_t  /*call*/,
        boolean32  /*block*/
    );

PRIVATE void rpc__dg_xmitq_init (
        rpc_dg_xmitq_p_t  /*xq*/
    );

PRIVATE void rpc__dg_xmitq_reinit (
        rpc_dg_xmitq_p_t  /*xq*/
    );

PRIVATE void rpc__dg_xmitq_free (
        rpc_dg_xmitq_p_t  /*xq*/,
        rpc_dg_call_p_t  /*call*/
    );

PRIVATE void rpc__dg_xmitq_append_pp (
        rpc_dg_call_p_t  /*call*/,
        unsigned32 * /*st*/
    );

PRIVATE boolean rpc__dg_xmitq_awaiting_ack_tmo (
        rpc_dg_xmitq_p_t  /*xq*/,
        unsigned32  /*com_timeout_knob*/
    );

PRIVATE void rpc__dg_xmitq_restart (
        rpc_dg_call_p_t  /*call*/
    );

#endif /* _DGXQ_H */
