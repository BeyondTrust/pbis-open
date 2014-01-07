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
**      dgcall.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.
**
**
*/

#ifndef _DGCALL_H
#define _DGCALL_H

#include <dce/dce.h>

/* ======================================================================= */

/*
 * Call wait flags.
 *
 * This enumeration defines the types of events that calls need to wait
 * for when calling the call_wait routine.  These values indicate whether
 * the caller is waiting for 1) a network event, such as the arrival
 * of data, facks, etc., or 2) an internal event, such as  
 * packets/reservations to become available.  
 * 
 * Note that this distinction is only meaningful for users of private sockets.
 * Call threads using private sockets wait for network events by sleeping in 
 * recvfrom, and wait for internal events by sleeping on a condition variable.
 * Call threads that use shared sockets always wait on the call handle's
 * condition variable (i.e., always specify rpc_e_dg_wait_on_internal_event).
 */
typedef enum {
    rpc_e_dg_wait_on_internal_event,
    rpc_e_dg_wait_on_network_event
} rpc_dg_wait_event_t;

/* ======================================================================= */


/*
 * R P C _ D G _ C A L L _ R E F E R E N C E
 *
 * Increment the reference count for the CALL.  Note that there's no
 * RPC_DG_CALL_RELEASE; you must use one of RPC_DG_CCALL_RELEASE or
 * RPC_DG_SCALL_RELEASE depending on whether you have a client or server
 * call handle.  (The release function has to call the server/client
 * specific "free" function.)
 */

#define RPC_DG_CALL_REFERENCE(call) { \
    RPC_DG_CALL_LOCK_ASSERT(call); \
    assert((call)->refcnt < 255); \
    (call)->refcnt++; \
}

/*
 * R P C _ D G _ C A L L _ R E I N I T
 *
 * Reinitialize the common part of a call handle.
 * If we are not going to use MBF, turn off the first fack waiting.
 */

#define RPC_DG_CALL_REINIT(call) { \
    (call)->last_rcv_timestamp = 0; \
    (call)->start_time = rpc__clock_stamp(); \
    (call)->status = rpc_s_ok; \
    (call)->blocked_in_receive = false; \
    (call)->priv_cond_signal = false; \
    RPC_DG_XMITQ_REINIT(&(call)->xq, (call)); \
    RPC_DG_RECVQ_REINIT(&(call)->rq); \
    if ((call)->xq.max_frag_size <= RPC_C_DG_MUST_RECV_FRAG_SIZE) \
        (call)->xq.first_fack_seen = true; \
}

/*
 * R P C _ D G _ C A L L _ S E T _ T I M E R
 *
 * Set up a timer for a call handle.  Bump the reference count of the call
 * handle since the reference from the timer queue counts as a reference.
 */

#define RPC_DG_CALL_SET_TIMER(call, proc, freq) { \
    rpc__timer_set(&(call)->timer, (proc), (pointer_t) (call), (freq)); \
    RPC_DG_CALL_REFERENCE(call); \
}

/*
 * R P C _ D G _ C A L L _ S T O P _ T I M E R
 *
 * "Request" that a call's timer be stopped.  This amounts to setting
 * a bit in the call handle.  We use this macro instead of calling
 * "rpc__timer_clear" directly since we really want the timer to stop
 * itself so it can eliminate its reference to the call handle in a
 * race-free fashion.
 */

#define RPC_DG_CALL_STOP_TIMER(call) \
    (call)->stop_timer = true

#ifdef __cplusplus
extern "C" {
#endif

/*
 * R P C _ D G _ C A L L _ S E T _ M A X _ F R A G _ S I Z E
 *
 * Set the max_frag_size and max_resvs in a call handle.
 * Also, if we are not going to use MBF, turn off the first fack
 * waiting.
 */

#define RPC_DG_CALL_SET_MAX_FRAG_SIZE(call, st) \
{ \
    rpc__naf_inq_max_frag_size((call)->addr, \
                               &((call)->xq.max_frag_size), (st)); \
    if (*st != rpc_s_ok) \
        (call)->xq.max_frag_size = RPC_C_DG_MUST_RECV_FRAG_SIZE; \
    else if ((call)->xq.max_frag_size > RPC_C_DG_MAX_FRAG_SIZE) \
        (call)->xq.max_frag_size = RPC_C_DG_MAX_FRAG_SIZE; \
    if ((call)->xq.max_frag_size <= RPC_C_DG_MUST_RECV_FRAG_SIZE) \
        (call)->xq.first_fack_seen = true; \
    RPC_DG_FRAG_SIZE_TO_NUM_PKTS((call)->xq.max_frag_size, \
                                 (call)->max_resvs); \
}

#ifdef DEBUG

PRIVATE char *rpc__dg_call_state_name (
        rpc_dg_call_state_t state
    );

#else

#define rpc__dg_call_state_name(junk) ""

#endif

PRIVATE void rpc__dg_call_xmit_fack (
        rpc_dg_call_p_t  /*call*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        boolean32 /*is_nocall*/
    );

PRIVATE void rpc__dg_call_xmit (
        rpc_dg_call_p_t  /*call*/,
        boolean32 /*block*/
    );

PRIVATE void rpc__dg_call_xmitq_timer (
        rpc_dg_call_p_t /*call*/
    );

PRIVATE void rpc__dg_call_init (
        rpc_dg_call_p_t /*call*/
    );

PRIVATE void rpc__dg_call_free (
        rpc_dg_call_p_t /*call*/
    );

PRIVATE void rpc__dg_call_wait (
        rpc_dg_call_p_t /*call*/,
        rpc_dg_wait_event_t /*event*/,
        unsigned32 * /*st*/
    );

PRIVATE void rpc__dg_call_signal (
        rpc_dg_call_p_t /*call*/
    );

PRIVATE void rpc__dg_call_xmitq_push (
        rpc_dg_call_p_t /*call*/,
        unsigned32 * /*st*/
    );

PRIVATE boolean rpc__dg_call_recvq_insert (
        rpc_dg_call_p_t /*call*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        boolean * /*rqe_is_head_inorder*/
    );

PRIVATE void rpc__dg_call_signal_failure (
        rpc_dg_call_p_t /*call*/,
        unsigned32 /*stcode*/
    );

PRIVATE void rpc__dg_call_local_cancel (
        rpc_dg_call_p_t /*call*/
    );

#ifdef __cplusplus
}
#endif


#endif
