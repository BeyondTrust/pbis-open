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
**      dgpkt.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Routines for managing the datagram packet pool.
**
**
*/

#ifndef _DGPKT_H
#define _DGPKT_H

/* ========================================================================= */
/*
 * Datagram Packet Rationing
 *
 * Every active call requires a packet pool "reservation".  The purpose
 * of this reservation is to guarantee that the call can always make
 * progress.
 *
 * "Progress" is defined as the ability to exchange at least one packet
 * at a time over a conversation.
 * 
 * "Rationing" is the condition where calls are only allowed to use
 * their reserved packet.  
 * 
 * The condition that invokes rationing is: the number of available
 * packets in the free pool is less than or equal to the number of
 * packet reservations.
 */

/*
 * Define the maximum number of packet's (used for both xqe's and rqe's)
 * that we'll ever allocate.
 */
           
#include <dce/dce.h>


#ifndef RPC_C_DG_PKT_MAX
#define RPC_C_DG_PKT_MAX    100000
#endif

/*
 * Define the number of packets that should be allocated during
 * initialization to seed the free list.
 */    

#ifndef RPC_C_DG_PKT_INIT_CNT
#define RPC_C_DG_PKT_INIT_CNT       48
#endif
  
/*
 * One of the conditions on which the packet rationing scheme is based is
 * that, when rationing is necessary, all RPC threads will be able to drain
 * their queues and use only the packets they have reserved.  One possible
 * scenario is which this wouldn't be the case is if in a group of N
 * inter-communicating processes (where N can be 1, in the case of KRPC)
 * all packets got queued up by clients before the rationing state was
 * entered.  In such a case, there would be no reservations to grant to
 * the server sides of the calls, and thus no way for the clients to drain.
 * 
 * Avoiding this potential deadlock requires that we guarantee that at least
 * one server call can be started for such a group of N processes.  Since N can
 * be 1, it follows that every process must be guaranteed that it can always
 * start at least one server call.
 * 
 * We guarantee the capability of running server calls by setting aside
 * reservations for that purpose at startup.  A single reservation would be
 * sufficient to guarantee the correctness of the algorithm, but making a few
 * reservations will increase the fairness in the worst of cases.
 */
#ifndef RPC_C_DG_PKT_INIT_SERVER_RESVS
#define RPC_C_DG_PKT_INIT_SERVER_RESVS  5
#endif

/*
 * The  following structure is used to build the free packet pool.  Since
 * the pool contains both xqe's and rqe's, we need a third, neutral pointer
 * which can be used to access either type of transmission element.
 */
                       
typedef struct rpc_dg_pkt_pool_elt_t {
    union {
        struct rpc_dg_pkt_pool_elt_t *next; /* for building a free list */
        struct {
            rpc_dg_xmitq_elt_t xqe;
            rpc_dg_pkt_body_t  pkt;
        } xqe;                             /* for handling xqe's       */
        struct {
            rpc_dg_recvq_elt_t rqe;
            rpc_dg_raw_pkt_t   pkt;
            unsigned32         pad1;       /* Need some padding to get */ 
            unsigned32         pad2;       /* 0 MOD 8 byte alignment   */
            rpc_dg_sock_pool_elt_p_t sock_ref;
        } rqe;                             /* for handling rqe's       */
    } u;
    boolean is_on_free_list;     
} rpc_dg_pkt_pool_elt_t, *rpc_dg_pkt_pool_elt_p_t;

/*
 * The packet pool is a global structure which contains:
 *       - a mutex to lock when modifying this structure
 *       - the max # of packets that can be allocated
 *       - a counter of the number of packets remaining to be allocated 
 *       - number of packet reservations currently held
 *       - counters of the number of currently active xqe's/rqe's
 *         and failed rqe allocations (this should always be 0 if things
 *         are working) and xqe allocation attempts that had to block.
 *       - a counter of the number of packets on the free list
 *       - a linked list of free packets
 *       - a pointer to the tail of the free list
 *       - pointers to the head and tail of a linked list of call
 *         handles that are waiting to be signalled when a packet
 *         becomes available.
 *       - pointers to the head and tail of a linked list of call
 *         handles that are waiting to be signalled when a packet
 *         reservation becomes available.
 */

typedef struct rpc_dg_pkt_pool_t {
    rpc_mutex_t             pkt_mutex;
    unsigned32              max_pkt_count; 
    unsigned32              pkt_count; 
    unsigned32              reservations;
    unsigned32              srv_resv_avail;
    unsigned32              max_resv_pkt; /* # of pkts for the largest fragment */
    unsigned32              active_rqes;
    unsigned32              active_xqes;
    unsigned32              failed_alloc_rqe;
    unsigned32              blocked_alloc_xqe;
    unsigned32              free_count;
    unsigned                is_rationing: 1;    /* T => is rationing */
    unsigned                low_on_pkts: 1;     /* T => pkt pool is low */
    rpc_dg_pkt_pool_elt_p_t free_list;
    rpc_dg_pkt_pool_elt_p_t free_list_tail;
    rpc_dg_call_p_t pkt_waiters_head, pkt_waiters_tail;        
    rpc_dg_call_p_t rsv_waiters_head, rsv_waiters_tail;
} rpc_dg_pkt_pool_t;  

EXTERNAL rpc_dg_pkt_pool_t rpc_g_dg_pkt_pool;
                   
/* 
 * Macros for locking/unlocking the packet pool's mutex.
 */                                     

#define RPC_DG_PKT_POOL_LOCK(junk)    RPC_MUTEX_LOCK(rpc_g_dg_pkt_pool.pkt_mutex)
#define RPC_DG_PKT_POOL_UNLOCK(junk)  RPC_MUTEX_UNLOCK(rpc_g_dg_pkt_pool.pkt_mutex)
#define RPC_DG_PKT_POOL_LOCK_ASSERT(junk) \
                          RPC_MUTEX_LOCK_ASSERT(rpc_g_dg_pkt_pool.pkt_mutex)

/*
 * Macro to determine if we are in the packet rationing state.  
 *
 * This macro requires that the packet pool is locked.
 */ 
                                                         
#define RPC_DG_PKT_RATIONING(junk) \
        (rpc_g_dg_pkt_pool.free_count + rpc_g_dg_pkt_pool.pkt_count <= \
                   rpc_g_dg_pkt_pool.reservations)

/* ========================================================================= */

PRIVATE void rpc__dg_pkt_pool_init    (void);


PRIVATE void rpc__dg_pkt_pool_fork_handler    (
        rpc_fork_stage_id_t  /*stage*/
    );

PRIVATE rpc_dg_xmitq_elt_p_t rpc__dg_pkt_alloc_xqe    (
        rpc_dg_call_p_t  /*call*/,
        unsigned32 * /*st*/
    );

PRIVATE rpc_dg_recvq_elt_p_t rpc__dg_pkt_alloc_rqe    (
        rpc_dg_ccall_p_t  /*ccall*/
    );

PRIVATE void rpc__dg_pkt_free_xqe    (                  
        rpc_dg_xmitq_elt_p_t  /*pkt*/,
        rpc_dg_call_p_t  /*call*/
    );

PRIVATE void rpc__dg_pkt_free_rqe_for_stub    (
        rpc_dg_recvq_elt_p_t  /*pkt*/
    );

PRIVATE void rpc__dg_pkt_free_rqe    (
        rpc_dg_recvq_elt_p_t  /*pkt*/,
        rpc_dg_call_p_t  /*call*/
    );
     
PRIVATE boolean32 rpc__dg_pkt_adjust_reservation    (
        rpc_dg_call_p_t  /*call*/,
        unsigned32 /*nreq*/,
        boolean32  /*block*/
    );

PRIVATE void rpc__dg_pkt_cancel_reservation    (
        rpc_dg_call_p_t  /*call*/
    );

PRIVATE boolean32 rpc__dg_pkt_is_rationing    (
        boolean32 * /*low_on_pkts*/
    );
#endif /* _DGPKT_H */
