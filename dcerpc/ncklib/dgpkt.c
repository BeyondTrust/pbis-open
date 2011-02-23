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
**      dgpkt.c
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

#include <dg.h>
#include <dgpkt.h>
#include <dgcall.h>
#include <dgscall.h>
#include <dgccall.h>


/* ========================================================================= */

GLOBAL rpc_dg_pkt_pool_t rpc_g_dg_pkt_pool;

/* ========================================================================= */

INTERNAL rpc_dg_pkt_pool_elt_p_t pkt_alloc (void);


INTERNAL void pkt_free (
        rpc_dg_pkt_pool_elt_p_t  /*pkt*/,
        rpc_dg_call_p_t  /*call*/
    );

INTERNAL void dequeue_pool_waiter (
        rpc_dg_call_p_t  /*call*/,
        rpc_dg_call_p_t * /*head*/,
        rpc_dg_call_p_t * /*tail*/
    );

INTERNAL void scan_waiter_lists (
        rpc_dg_call_p_t  /*call*/
    );

/* ========================================================================= */
        
/*
 * These macros define the pool condition under which it's okay to grant
 * new reservations.  In the general case, there must be enough free packets to 
 * cover all current reservations, plus n to cover the reservation being
 * considered.
 * In the second case, there are a numer of pre-reserved reservations set 
 * aside for use by servers only.
 */

#define RESERVATION_AVAILABLE(n) \
        (pool->free_count + pool->pkt_count >= pool->reservations + (n))

#define SRV_RESERVATION_AVAILABLE(call) \
        (RPC_DG_CALL_IS_SERVER(call) && pool->srv_resv_avail > 0)

/*
 * The macro to update the rationing state.
 *
 * First, determine if we are currently rationing packets.
 * If so, we'll set a flag to indicate this condition.
 * This information will be used by recvq_insert when deciding whether
 * to queue this packet to a call handle.
 *
 * Second, see if the pool is getting low. The xmit_fack routine uses
 * this information to begin backing off the sender.
 * Avoid doing the computation if is_rationing is already set;
 */ 
#define UPDATE_RATIONING_STATE(junk) \
{ \
    rpc_g_dg_pkt_pool.is_rationing = RPC_DG_PKT_RATIONING(0); \
    rpc_g_dg_pkt_pool.low_on_pkts = rpc_g_dg_pkt_pool.is_rationing || \
        (rpc_g_dg_pkt_pool.free_count + rpc_g_dg_pkt_pool.pkt_count <= \
         2 * rpc_g_dg_pkt_pool.reservations); \
}

/* ========================================================================= */
/* 
 * D E Q U E U E _ P O O L _ W A I T E R
 *
 * This routine is used to dequeue a call handle from any location within
 * a list of waiting calls.  Calls may be waiting for 1) a reservation,
 * or 2) a packet; the head/tail pointer will determine which list to
 * search.
 *
 * The packet pool and call handle locks must be held before calling
 * this routine.
 */                                                       
                    
INTERNAL void dequeue_pool_waiter
(
    rpc_dg_call_p_t call, 
    rpc_dg_call_p_t *head, 
    rpc_dg_call_p_t *tail                              
)
{
    rpc_dg_call_p_t waiter = *head, prev = NULL;
                          
    for ( ; waiter != NULL; prev = waiter, waiter = waiter->pkt_chain)
    {                             
        if (waiter == call)
        {
            if (prev == NULL)
                (*head) = call->pkt_chain;
            else
                prev->pkt_chain = call->pkt_chain; 

            if (call->pkt_chain == NULL)
                (*tail) = prev; 

            call->is_in_pkt_chain = false;
            
            /*
             * Decrement our reference to the call.
             */

            if (RPC_DG_CALL_IS_SERVER(call))
            {
                RPC_DG_SCALL_RELEASE_NO_UNLOCK((rpc_dg_scall_p_t *) &call);
            }
            else                            
            {
                RPC_DG_CCALL_RELEASE_NO_UNLOCK((rpc_dg_ccall_p_t *) &call);
            }

            return;
        }
    }                                                                     
    /*
     * Since these lists are internal, we should never be mistaken
     * about what's on them.
     */
    RPC_DBG_GPRINTF(("(dequeue_pool_waiter) No call found\n"));
}                               

/*
 * S C A N _ W A I T E R _ L I S T S 
 *
 * This routine is called when the system is *not* in a rationing state,
 * but there are calls which are blocked waiting for 1) a pool reservation,
 * or 2) a packet.  Precedence is given to reservation waiters.
 * 
 * If it is possible that the caller holds the lock for one of the calls
 * on one of the waiters list, it must send us the address of that call
 * handle.  Since we need to lock a call before signalling it, this allows
 * us to avoid trying to re-lock a call handle that has already been
 * locked by the caller.
 *
 * The pool must be locked before calling this routine.
 */                                                    

INTERNAL void scan_waiter_lists
(
    rpc_dg_call_p_t call
)
{
    rpc_dg_call_p_t waiter = NULL, prev = NULL;
    rpc_dg_pkt_pool_t *pool = &rpc_g_dg_pkt_pool;
       
    RPC_DG_PKT_POOL_LOCK_ASSERT(0);

    /*
     * See if it's possible to wake up a blocked call.  Precedence is
     * given to calls waiting for reservations.  Note that if there are
     * N reservations, and N+1 free packets, it is possible to wake a
     * call waiting for a packet.  However, we will not do so if there
     * are calls waiting for reservations.  In that case, we just return,
     * since a reservation waiter requires N+2 free packets in the pool
     * before it can be granted a reservation.
     */
                                       
    if (pool->rsv_waiters_head != NULL)
    {
        /*
         * There are two places we might find an available reservation.  First
         * see if there are enough free packets in the pool to grant another
         * reservation...
         */
        if (RESERVATION_AVAILABLE(pool->rsv_waiters_head->n_resvs_wait))
        {
            waiter = pool->rsv_waiters_head;
            pool->rsv_waiters_head = waiter->pkt_chain;
        }

       /*
        * ... otherwise, if any of the pre-reserved server reservations
        * are available, see if there are any scalls on the waiters' queue.
        */
       else if (pool->srv_resv_avail > 0)
       {
            for (waiter = pool->rsv_waiters_head; waiter != NULL;
                 prev = waiter, waiter = waiter->pkt_chain)
            {
                if (RPC_DG_CALL_IS_SERVER(waiter)
                    && pool->max_resv_pkt+1 >= waiter->n_resvs_wait)
                {
                    if (prev == NULL)
                         pool->rsv_waiters_head = waiter->pkt_chain;
                    else
                        prev->pkt_chain = waiter->pkt_chain; 

                    if (waiter->pkt_chain == NULL)
                         pool->rsv_waiters_tail = prev; 
 
                    break;
                }
            }
        }
    }    
    else if (pool->pkt_waiters_head != NULL)  
    {
        if (! RPC_DG_PKT_RATIONING(0))
        {
            waiter = pool->pkt_waiters_head;
            pool->pkt_waiters_head = waiter->pkt_chain;
        }
    }    
               
    /*
     * If we weren't able to dequeue a waiter, return now.
     */

    if (waiter == NULL)
        return;

    /*
     * Reset the call's flag, which is protected by the packet pool lock.
     */

    waiter->is_in_pkt_chain = false;

    /*
     * Signalling the call requires that we hold the call's lock.  This
     * involves unlocking the pool first to respect the locking heirarchy
     * (call, then pool).  However, if the call we pulled off the queue
     * happens to be the call we were sent, it's already locked, and
     * the operation is somewhat simpler.
     */

    if (call != waiter)
    {
        RPC_DG_PKT_POOL_UNLOCK(0);
        RPC_DG_CALL_LOCK(waiter);
    }

    rpc__dg_call_signal(waiter);
                  
    /*
     * Since we've removed the call from the waiters' queue, we need 
     * to release that reference.  In the case where we locked the
     * call, unlock it.  Otherwise, leave it the way we found it.
     */

    if (call != waiter)
    {
        if (RPC_DG_CALL_IS_SERVER(waiter))
        {
            RPC_DG_SCALL_RELEASE((rpc_dg_scall_p_t *) &waiter);
        }
        else                            
        {
            RPC_DG_CCALL_RELEASE((rpc_dg_ccall_p_t *) &waiter);
        }

        /*
         * Return the pool locked, as the caller expects.
         */

        RPC_DG_PKT_POOL_LOCK(0);
    }
    else
    {
        if (RPC_DG_CALL_IS_SERVER(waiter))
        {
            RPC_DG_SCALL_RELEASE_NO_UNLOCK((rpc_dg_scall_p_t *) &waiter);
        }
        else                            
        {
            RPC_DG_CCALL_RELEASE_NO_UNLOCK((rpc_dg_ccall_p_t *) &waiter);
        }
    }
}
                                                     


/* 
 * R P C _ _ D G _ P K T _ P O O L _ I N I T
 *
 * Initialize the packet pool structure.
 */

PRIVATE void rpc__dg_pkt_pool_init(void)
{   
    rpc_dg_pkt_pool_elt_p_t   pkt;
    rpc_dg_pkt_pool_t *pool = &rpc_g_dg_pkt_pool;
    unsigned32 i;

    /*
     * Allow the total number of packets in the pool to be settable thru
     * a debug switch.  [# of pkts = 2 ^ (level - 1)]
     */
                 
#ifdef DEBUG
    if (RPC_DBG(rpc_es_dbg_pkt_quota_size, 1))
    {                                                  
        pool->max_pkt_count = 
              1 << (rpc_g_dbg_switches[(int) rpc_es_dbg_pkt_quota_size] - 1);
    }
    else
#endif
    {
        pool->max_pkt_count = RPC_C_DG_PKT_MAX;
    }
                                                      
    pool->pkt_count = pool->max_pkt_count;
    pool->active_rqes = 0;
    pool->active_xqes = 0;
    pool->free_count = 0;
    pool->free_list  = NULL;
    pool->free_list_tail = NULL;

    /*
     * The number of packets required for the largest fragment size
     * possible.
     */
    pool->max_resv_pkt = RPC_C_DG_MAX_NUM_PKTS_IN_FRAG;

    /*
     * We always start out by reserving RPC_C_DG_MAX_NUM_PKTS_IN_FRAG
     * packets for the listener thread, plus max_resv_pkt+1 packets for
     * each of the pre-reserved server-side reservations.
     */                

    pool->reservations = RPC_C_DG_MAX_NUM_PKTS_IN_FRAG
        + ((pool->max_resv_pkt + 1) * RPC_C_DG_PKT_INIT_SERVER_RESVS);

    /*
     * Initialize the count of pre-reserved server-side reservations.
     * If, when a server call tries to make a reservertion, this count is greater than
     * 0, the server call will never need to wait.
     */

    pool->srv_resv_avail = RPC_C_DG_PKT_INIT_SERVER_RESVS;

    /*
     * Initialize the two lists of waiters.  Note that when the head
     * of the list is NULL, the tail pointer is undefined.
     */ 

    pool->pkt_waiters_head  = NULL;
    pool->rsv_waiters_head  = NULL;

    /*
     * Allocate an initial allotment of packet buffers.  See notes in
     * pkt_alloc() below for more details about the makeup of the packets.
     */
                    
    for (i = MIN(RPC_C_DG_PKT_INIT_CNT, pool->pkt_count); i > 0; i--)
    {   
        RPC_MEM_ALLOC(pkt, rpc_dg_pkt_pool_elt_p_t, 
            sizeof(rpc_dg_pkt_pool_elt_t), RPC_C_MEM_DG_PKT_POOL_ELT, 
            RPC_C_MEM_NOWAIT);

        pkt->u.next = pool->free_list;
        pkt->is_on_free_list = true;
        pool->free_list = pkt;
       
        /*
         * Anchor the tail pointer on the first packet allocated.
         */
        if (pool->free_list_tail == NULL)
            pool->free_list_tail = pkt;

        pool->free_count++;
        pool->pkt_count--;
    } 
        
    RPC_MUTEX_INIT(pool->pkt_mutex);
}

/*
 * R P C _ _ D G _ P K T _ P O O L _ F O R K _ H A N D L E R 
 *
 * Handle fork related processing for this module.
 */

PRIVATE void rpc__dg_pkt_pool_fork_handler
(
    rpc_fork_stage_id_t stage
)
{                           
    rpc_dg_pkt_pool_elt_p_t   pkt, next_pkt;
    rpc_dg_pkt_pool_t *pool = &rpc_g_dg_pkt_pool;

    switch ((int)stage)
    {
        case RPC_C_PREFORK:
            break;
        case RPC_C_POSTFORK_PARENT:
            break;
        case RPC_C_POSTFORK_CHILD:  
            /*
             * Free any packets sitting on the free list.
             */
        
            pkt = pool->free_list;
            while (pool->free_count--)                
            {  
                next_pkt = pkt->u.next;
                RPC_MEM_FREE(pkt, RPC_C_MEM_DG_PKT_POOL_ELT);
                pkt = next_pkt;;
            }                      
        
            /*
             * Clear out remaining fields of the packet pool.
             */                            
            /*b_z_e_r_o((char *) &rpc_g_dg_pkt_pool, sizeof(rpc_dg_pkt_pool_t));*/
	    memset( &rpc_g_dg_pkt_pool, 0, sizeof(rpc_dg_pkt_pool_t));
            break;
    }
}
                        
/*
 * P K T _ A L L O C
 *
 * Return a datagram packet.  The packet comes either from the free list
 * we're maintaining, or if the free list is empty, we alloc up a new
 * one.  It is up to the callers of this routine to respect the packet
 * rationing rules; as such, this routine should always be able to return
 * a packet successfully.
 *
 * The packet pool lock must be held before calling this routine
 */

INTERNAL rpc_dg_pkt_pool_elt_p_t pkt_alloc(void)
{
    rpc_dg_pkt_pool_elt_p_t   pkt;
    rpc_dg_pkt_pool_t *pool = &rpc_g_dg_pkt_pool;
              
    RPC_DG_PKT_POOL_LOCK_ASSERT(0);

    /*
     * If there is an packet on the free list, use it.
     */

    if (pool->free_list != NULL)
    {
        pkt = pool->free_list;
        pool->free_list = pool->free_list->u.next;
        pool->free_count--;

        /*
         * If the free list is now empty, then there's no longer a tail.
         */
        if (pool->free_list == NULL)
            pool->free_list_tail = NULL;

#ifdef DEBUG
        if (RPC_DBG_EXACT(rpc_es_dbg_mem,20))
        {
            unsigned32 count;
            rpc_dg_pkt_pool_elt_p_t next_pkt;

            for (count = 0, next_pkt = pool->free_list;
                 next_pkt != NULL;
                 count++, next_pkt = next_pkt->u.next);

            if (pool->free_count != count)
            {
                RPC_DBG_PRINTF(rpc_e_dbg_mem, 20,
                   ("(pkt_alloc) free_count mismatch: free_count (%d) != %d\n",
                                pool->free_count, count));
		/*
		 * rpc_m_dgpkt_pool_corrupt
		 * "(%s) DG packet free pool is corrupted"
		 */
		RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%s"),
		    rpc_svc_dg_pkt,
		    svc_c_sev_fatal | svc_c_action_abort,
		    rpc_m_dgpkt_pool_corrupt,
		    "pkt_alloc" ));
            }
        }
#endif
    } 
    
    /*
     * Else, we'll need to alloc up a new one.
     */

    else 
    {   
        /*
         * Leave this assert un-ifdef'ed... it's cheap enough and it seems
         * to be a reasonably good indicator of other problems related to
         * xq/rq management.  If this assert triggers, it's time to enable the
         * rest of the pkt pool debug stuff to help zero in on the problem
         * (in the past, problems have been users of the pool, not the pool
         * code itself).
         */
        assert(rpc_g_dg_pkt_pool.free_count == 0);

        /*
         * First make sure we haven't already allocated the maximum number
         * of packets.  This should never happen since the callers of
         * this routine are enforcing packet qoutas, which require that
         * at least one packet be available at all times.
         */

        if (pool->pkt_count == 0)
            return (NULL);

        /*
         * Allocate up a new packet.  The size of this packet is the
         * maximum of what we need for either an xqe or an rqe. 
         */
                      
        RPC_MEM_ALLOC(pkt, rpc_dg_pkt_pool_elt_p_t, 
            sizeof(rpc_dg_pkt_pool_elt_t), RPC_C_MEM_DG_PKT_POOL_ELT, 
            RPC_C_MEM_NOWAIT);

        pool->pkt_count--;
    }           

    RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 5, 
            ("(pkt_alloc) pkts %lu\n",
            pool->free_count + pool->pkt_count)); 

    pkt->is_on_free_list = false;

    UPDATE_RATIONING_STATE(0);
    return(pkt);
}

/*
 * R P C _ _ D G _ P K T _ A L L O C _ X Q E
 *
 * Return a transmit queue element (rpc_dg_xmit_q_elt_t).  Before
 * allocating the packet, check to see if we need to be rationing packets.
 * If so, and this call is already using its reserved packet, block the
 * call.
 *
 * Call handle lock must be held.  Acquires the packet pool lock.
 */

PRIVATE rpc_dg_xmitq_elt_p_t rpc__dg_pkt_alloc_xqe
(
    rpc_dg_call_p_t call,
    unsigned32 *st
)
{
    rpc_dg_pkt_pool_elt_p_t pkt;
    rpc_dg_xmitq_elt_p_t xqe = NULL;
    rpc_dg_pkt_pool_t *pool = &rpc_g_dg_pkt_pool;
             
    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * Before taking the packet pool lock, check to see if this call
     * handle is using a private socket, and if the private socket has
     * a private xqe that can be used.
     */
    if (call->sock_ref->is_private)
    {
        if (call->sock_ref->xqe != NULL)
        {
            xqe = call->sock_ref->xqe;
            call->sock_ref->xqe = NULL;

            xqe->next       = NULL;
            xqe->more_data  = NULL;
            xqe->frag_len   = 0;
            xqe->flags      = 0;
            xqe->body_len   = 0;
            xqe->serial_num = 0;
            xqe->in_cwindow = false;

            return (xqe);
        }
        /*
         * This call handle is using a private socket, but the cached
         * xqe is already in use.  We'll need to really allocate one;
         * if the call has not yet made a packet pool reservation, we
         * need to do that now.
         */
        else if (call->n_resvs == 0)
        {
            unsigned32  resv ATTRIBUTE_UNUSED;

            /*
             * Only the client uses the private socket (at least for now). Thus,
             * we can adjust the reservation in the blocking mode.
             *
             * If we ever allow the server callback's use of the private socket,
             * we will need to be careful here.
             *
             * Since we have been running without a reservation, we are still in
             * SBF (Single Buffer Fragment) and need the minimum reservation.
             */
            rpc__dg_pkt_adjust_reservation(call, 1, true);
        }
    }

    RPC_DG_PKT_POOL_LOCK(0);
               
    /*
     * If we are rationing packets, and this call is already using its
     * reserved packet, put the call on the list of packet waiters and
     * go to sleep.
     */
                 
    while (RPC_DG_PKT_RATIONING(0) && call->xq.head != NULL)
    {
        pool->blocked_alloc_xqe++;

        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 3, 
            ("(alloc_xqe) rationing, blocking call, fc %lu pkt %lu [%s]\n",
            pool->free_count, pool->pkt_count, 
            rpc__dg_act_seq_string(&call->xq.hdr)));
            
        /*
         * If the call is not already on the packet waiters' chain, queue
         * it.  Update the call's reference count.
         */

        if (! call->is_in_pkt_chain)
        {
            if (pool->pkt_waiters_head == NULL) 
                pool->pkt_waiters_head = call; 
            else 
               pool->pkt_waiters_tail->pkt_chain = call; 
            pool->pkt_waiters_tail = call; 
            call->pkt_chain = NULL; 
            call->is_in_pkt_chain = true; 

            RPC_DG_CALL_REFERENCE(call);
        }

        RPC_DG_PKT_POOL_UNLOCK(0);  

        rpc__dg_call_wait(call, rpc_e_dg_wait_on_internal_event, st);

        RPC_DG_PKT_POOL_LOCK(0);

        if (*st != rpc_s_ok)
        {   
            /*
             * If there's an error, get off the queue.  (first make sure
             * that the error didn't occur after someone had already
             * removed us from the queue.)
             */

            if (call->is_in_pkt_chain)
                dequeue_pool_waiter(call, &pool->pkt_waiters_head,
                                          &pool->pkt_waiters_tail);

            RPC_DG_PKT_POOL_UNLOCK(0);
            return(NULL);
        }

        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 3, 
            ("(alloc_xqe) call signalled, fc %lu pkt %lu [%s]\n",
            pool->free_count, pool->pkt_count, 
            rpc__dg_act_seq_string(&call->xq.hdr)));
    } 

    /*
     * Before giving up the PKT lock, check to see if this call is on
     * the packet queue.  This could happen if the call got signalled
     * for some other reason, but woke up to find that there was a packet
     * available.
     */

    if (call->is_in_pkt_chain)
        dequeue_pool_waiter(call, &pool->pkt_waiters_head,
                                  &pool->pkt_waiters_tail);

    /*
     * We now have the packet pool locked, and we know that it's
     * appropriate to allocate a packet.
     */

    pkt = pkt_alloc();
       
    /*
     * If the pkt pointer is NULL at this point, someone screwed up the
     * quotas.
     */
       
    if (pkt == NULL)
    {                
        RPC_DG_PKT_POOL_UNLOCK(0);  
        RPC_DBG_GPRINTF(("(rpc__dg_pkt_alloc_xqe) No buffers available\n"));
        *st = rpc_s_coding_error;
        return(NULL);
    }

    rpc_g_dg_pkt_pool.active_xqes++;

    RPC_DG_PKT_POOL_UNLOCK(0);  

    xqe             = &pkt->u.xqe.xqe;     
    xqe->body       = &pkt->u.xqe.pkt;
    xqe->next       = NULL;
    xqe->more_data  = NULL;
    xqe->frag_len   = 0;
    xqe->flags      = 0;
    xqe->body_len   = 0;
    xqe->serial_num = 0;
    xqe->in_cwindow = false;

    return (xqe);
}

/*
 * R P C _ _ D G _ P K T _ A L L O C _ R Q E
 *
 * Allocate a packet for reading in a network datagram.  This routine
 * should always be able to return a packet since every packet initially
 * belongs to the listener thread, which has a packet reservation but
 * never holds onto packets.
 * 
 * In the case of private sockets, the private rqe is always available
 * (conceptually) for reading in a packet;  eventhough this new packet 
 * may then need to be dropped because of rationing.
 */

PRIVATE rpc_dg_recvq_elt_p_t rpc__dg_pkt_alloc_rqe(ccall)
rpc_dg_ccall_p_t ccall;
{ 
    rpc_dg_pkt_pool_elt_p_t pkt;
    rpc_dg_recvq_elt_p_t rqe;

    /*
     * Before taking the packet pool lock, check to see if this call
     * handle is using a private socket, and if the private socket has
     * a private rqe that can be used.
     */
    if (ccall != NULL)
    {
        if (ccall->c.sock_ref->rqe_available == true)
        {
            rqe = ccall->c.sock_ref->rqe;
            ccall->c.sock_ref->rqe_available = false;

            rqe->next   = NULL;
            rqe->more_data = NULL;
            rqe->frag_len = 0;
            rqe->hdrp   = NULL;


            return (rqe);
        }
        /*
         * This call handle is using a private socket, but the cached
         * rqe is already in use.  We'll need to really allocate one;
         * if the call has not yet made a packet pool reservation, we
         * need to do that now.
         */
        else if (ccall->c.n_resvs == 0)
        {
            /*
             * Only the client uses the private socket (at least for now). Thus,
             * we can adjust the reservation in the blocking mode.
             *
             * If we ever allow the server callback's use of the private socket,
             * we will need to be careful here.
             *
             * Note: The ccall hasn't advertised larger fragment size than the
             * minimum size yet. Thus the minimum reservation is required.
             */
            rpc__dg_pkt_adjust_reservation(&ccall->c, 1, true);
        }
    }

    RPC_DG_PKT_POOL_LOCK(0);  


    pkt = pkt_alloc();
       
    /*
     * If the pkt pointer is NULL at this point, someone screwed up the
     * packet_rationing.
     */

    if (pkt == NULL)
    {
        rpc_g_dg_pkt_pool.failed_alloc_rqe++;
        RPC_DG_PKT_POOL_UNLOCK(0);
        RPC_DBG_GPRINTF(("(rpc__dg_pkt_alloc_rqe) No buffers available\n"));  
        return(NULL);
    }

    rpc_g_dg_pkt_pool.active_rqes++;

    RPC_DG_PKT_POOL_UNLOCK(0);  

    pkt->u.rqe.sock_ref = NULL;

    rqe = &pkt->u.rqe.rqe;
    rqe->pkt_real = &pkt->u.rqe.pkt;
    rqe->pkt = (rpc_dg_raw_pkt_p_t) RPC_DG_ALIGN_8(rqe->pkt_real);

    rqe->next   = NULL;
    rqe->more_data = NULL;
    rqe->frag_len = 0;
    rqe->hdrp   = NULL;


    return(rqe);
} 

/*
 * P K T _ F R E E
 *
 * Add a datagram packet onto the free list, and possibly signal a waiting
 * call.  The rules for signalling calls are as follows: 
 *
 *      - If rationing is in effect, see if the caller is on the packet
 *        waiters' list If so, it must be waiting for an XQE, since calls
 *        never block waiting for RQEs.  If the caller's xq->head is
 *        NULL, it is allowed to allocate a packet.  Signal it.
 *
 *      - If not rationing, see if there are calls waiting for either
 *        a reservation or a packet.  If so, signal one.
 *
 * The packet pool lock must be held before calling this routine.  It
 * may be that the caller also holds a call lock.  If it is possible
 * that this locked call might represent a call that is currently waiting
 * on the packet waiters' queue, we need to be sent the address of the
 * call handle.  This enables us to avoid trying to lock the call twice
 * if we happen to pull that call handle off a queue.  
 *
 * This scenario is only possible when this routine is called from the
 * listener thread, since if we are being called by the call thread we
 * can be sure that the thread is not waiting on the queue.  In particular,
 * when the stubs free rqes, the call handle we receive will be NULL, since
 * there is no danger that the call is currently queued.
 */

INTERNAL void pkt_free
(
    rpc_dg_pkt_pool_elt_p_t pkt,
    rpc_dg_call_p_t call
)
{            
    rpc_dg_pkt_pool_t *pool = &rpc_g_dg_pkt_pool;

    RPC_DG_PKT_POOL_LOCK_ASSERT(0);

    assert(pkt != NULL);
    
#ifdef DEBUG
    /* 
     * Do some sanity checking on the packet pool.
     */
    if (RPC_DBG_EXACT(rpc_es_dbg_mem,20))
    {
        unsigned32 count;
        rpc_dg_pkt_pool_elt_p_t next_pkt;

        for (count = 0, next_pkt = pool->free_list;
             next_pkt != NULL;
             count++, next_pkt = next_pkt->u.next)
        {
            if (next_pkt == pkt)
            {
                RPC_DBG_PRINTF(rpc_e_dbg_mem, 20,
                           ("(pkt_free) pkt already on free_list %#08x (%d)\n",
                                pkt, count));
		/*
		 * rpc_m_dgpkt_pool_corrupt
		 * "(%s) DG packet free pool is corrupted"
		 */
		RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%s"),
		    rpc_svc_dg_pkt,
		    svc_c_sev_fatal | svc_c_action_abort,
		    rpc_m_dgpkt_pool_corrupt,
		    "pkt_free" ));
            }
            if (!next_pkt->is_on_free_list)
            {
                RPC_DBG_PRINTF(rpc_e_dbg_mem, 20,
                 ("(pkt_free) free'ed pkt(%#08x) is not marked as free (%d)\n",
                                next_pkt, count));
		/*
		 * rpc_m_dgpkt_pool_corrupt
		 * "(%s) DG packet free pool is corrupted"
		 */
		RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%s"),
		    rpc_svc_dg_pkt,
		    svc_c_sev_fatal | svc_c_action_abort,
		    rpc_m_dgpkt_pool_corrupt,
		    "pkt_free" ));
            }
        }

        if (pool->free_count != count)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mem, 20,
                    ("(pkt_free) free_count mismatch: free_count (%d) != %d\n",
                            pool->free_count, count));
	    /*
	     * rpc_m_dgpkt_pool_corrupt
	     * "(%s) DG packet free pool is corrupted"
	     */
	    RPC_DCE_SVC_PRINTF ((
		DCE_SVC(RPC__SVC_HANDLE, "%s"),
		rpc_svc_dg_pkt,
		svc_c_sev_fatal | svc_c_action_abort,
		rpc_m_dgpkt_pool_corrupt,
		"pkt_free" ));
        }

        if (pkt->is_on_free_list)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mem, 20,
                           ("(pkt_free) double free'ing pkt(%#08x)\n", pkt));
	    /*
	     * rpc_m_dgpkt_bad_free
	     * "(%s) Attempt to free already-freed DG packet"
	     */
	    RPC_DCE_SVC_PRINTF ((
		DCE_SVC(RPC__SVC_HANDLE, "%s"),
		rpc_svc_dg_pkt,
		svc_c_sev_fatal | svc_c_action_abort,
		rpc_m_dgpkt_bad_free,
		"pkt_free" ));
        }
    }
#endif

    if (pkt->is_on_free_list)
    {
        return;
    }

    /*
     * Move the packet onto the tail of the pool's free list.
     */

    if (pool->free_list == NULL)
    {
        pool->free_list = pool->free_list_tail = pkt;
    }
    else
    {
        pool->free_list_tail->u.next = pkt;
        pool->free_list_tail = pkt;
    }

    pkt->u.next = NULL;
    pkt->is_on_free_list = true;
    pool->free_count++;
    
    RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 5, 
            ("(pkt_free) pkts %lu\n",
            pool->free_count + pool->pkt_count));
    /*
     * Giving up this packet may have taken the system out of a rationing
     * state.  If so, try to reclaim any pre-reserved server reservations that
     * we may be outstanding.
     */

    while (RESERVATION_AVAILABLE(pool->max_resv_pkt+1) &&
        pool->srv_resv_avail < RPC_C_DG_PKT_INIT_SERVER_RESVS)
    {
        pool->reservations += (pool->max_resv_pkt + 1);
        pool->srv_resv_avail++;
    }

    /*
     * If the system is currently rationing packets, check to see if
     * the call which just freed this packet is blocked waiting for another
     * packet, and not currently using its reserved packet.  If so, wake
     * it up.
     */   

    if (RPC_DG_PKT_RATIONING(0))
    {
        if (call != NULL && call->is_in_pkt_chain && call->xq.head == NULL)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 3, 
                ("(pkt_free) signalling self\n"));

            dequeue_pool_waiter(call, &pool->pkt_waiters_head,
                                      &pool->pkt_waiters_tail);

            rpc__dg_call_signal(call);                      
        }
    }

    /*
     * If we're not rationing, check to see if there are any blocked calls
     * that could be woken up.
     */

    else if (pool->rsv_waiters_head != NULL || pool->pkt_waiters_head != NULL)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 3, 
            ("(pkt_free) calling list scanner\n"));

        scan_waiter_lists(call);                                     
    }
    UPDATE_RATIONING_STATE(0);
}

/* 
 * R P C _ _ D G _ P K T _ F R E E _ X Q E
 *
 * Return an xqe to the packet pool.  This routine decrements the global
 * count of xqe's in use, and calls a common routine to actually move the
 * packet onto the pool list. 
 *
 * The caller of this routine must hold the call lock for the call handle
 * on which this xqe is currently queued.
 */          

PRIVATE void rpc__dg_pkt_free_xqe
(
    rpc_dg_xmitq_elt_p_t xqe,
    rpc_dg_call_p_t call
)
{
    rpc_dg_xmitq_elt_p_t tmp;
    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * If the packet is being freed by a call handle using a private
     * socket, and the private socket doesn't currently contain a cached
     * xqe, then cache this xqe.
     */

    if (call->sock_ref != NULL &&
       call->sock_ref->is_private &&
       call->sock_ref->xqe == NULL)
    {
        call->sock_ref->xqe = xqe;
        if (xqe->more_data == NULL)
        return;
        else
        {
            xqe = xqe->more_data;
            call->sock_ref->xqe->more_data = NULL;
        }
    }

    RPC_DG_PKT_POOL_LOCK(0);

    do
    {
        tmp = xqe->more_data;
    rpc_g_dg_pkt_pool.active_xqes--;

    pkt_free((rpc_dg_pkt_pool_elt_p_t) xqe, call);
        xqe = tmp;
    } while (xqe != NULL);

    RPC_DG_PKT_POOL_UNLOCK(0);
}

/* 
 * R P C _ _ D G _ P K T _ F R E E _ R Q E _ F O R _ S T U B
 *
 * Return an rqe to the packet pool.  This is a shell routine for callers
 * who know that they don't hold the call lock of any of the calls
 * currently waiting in the packet queue.  In particular, the call thread
 * itself never needs to worry about this deadlock, and so the stubs always
 * call this routine to free rqe's.  
 */

PRIVATE void rpc__dg_pkt_free_rqe_for_stub
(
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc__dg_pkt_free_rqe(rqe, NULL);
}

/* 
 * R P C _ _ D G _ P K T _ F R E E _ R Q E
 *                                        
 * Return an rqe to the packet pool.  This routine decrements the global
 * count of rqe's in use, and calls a common routine to actually move the
 * packet onto the pool list.
 *                       
 * If the caller holds any call handle, locks it must send pass in a pointer
 * to that handle.  Typically this occurs when freeing an rqe which is
 * currently queued on a call handle.   If the caller holds no such locks,
 * it should pass a NULL pointer.
 */                               

PRIVATE void rpc__dg_pkt_free_rqe
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_call_p_t call
)
{
    rpc_dg_pkt_pool_elt_p_t    p = (rpc_dg_pkt_pool_elt_p_t) rqe;
    rpc_dg_recvq_elt_p_t tmp;

    /*
     * Mark the private rqe as available to use.
     */
    if (p->u.rqe.sock_ref != NULL)
    {
        p->u.rqe.sock_ref->rqe_available = true;
        if (rqe->more_data == NULL)
            return;
        else
        {
            rqe = rqe->more_data;
            p->u.rqe.sock_ref->rqe->more_data = NULL;
        }
    }

    RPC_DG_PKT_POOL_LOCK(0);

    do
    {
        tmp = rqe->more_data;
        /*
         * Mark the private rqe as available to use.
         */
        if (((rpc_dg_pkt_pool_elt_p_t)rqe)->u.rqe.sock_ref != NULL)
        {
            ((rpc_dg_pkt_pool_elt_p_t)rqe)->u.rqe.sock_ref->rqe_available = true;
            rqe->more_data = NULL;
        }
        else
        {
            rpc_g_dg_pkt_pool.active_rqes--;
            pkt_free((rpc_dg_pkt_pool_elt_p_t) rqe, call);
        }
        rqe = tmp;
    } while (rqe != NULL);

    RPC_DG_PKT_POOL_UNLOCK(0);
}

/*
 * R P C _ _ D G _ P K T _ A D J U S T _ R E S E R V A T I O N
 *
 * Reserve packets from the packet pool for a call.  Depending on the 'block'
 * argument, this call may block until a reservation is available.
 *
 * This routine has no dependencies on the global lock.  However, to simplify
 * the locking requirements of its callers, it can handle being called with
 * or without the global lock held.  In the case of a thread using a private
 * socket, the adjust_reservation will always be made *without* the global lock
 * held.  For users of shared sockets, this call is always made *with* the 
 * global lock held.
 * 
 * In all cases, it is assumed that the input call handle is locked.
 */                                                              

PRIVATE boolean32 rpc__dg_pkt_adjust_reservation
(
    rpc_dg_call_p_t call,                                
    unsigned32 nreq,
    boolean32 block
)
{
    unsigned32 st = rpc_s_ok;
    boolean32 got_it = false;
    signed32 how_many;
    rpc_dg_pkt_pool_t *pool = &rpc_g_dg_pkt_pool;
    boolean  using_private_socket = call->sock_ref->is_private;

    if (using_private_socket == false)
        RPC_LOCK_ASSERT(0);

    RPC_DG_CALL_LOCK_ASSERT(call);
                                  
    /*
     * Callback handles and WAY/WAY2 handles inherit the reservation
     * made for the original scall/ccall.
     */
    if (call->is_cbk == true)
    {
        if (RPC_DG_CALL_IS_CLIENT(call))
            call->n_resvs = ((rpc_dg_ccall_p_t) call)->cbk_scall->c.n_resvs;
        else
            call->n_resvs = ((rpc_dg_scall_p_t) call)->cbk_ccall->c.n_resvs;

        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
   ("(rpc__dg_pkt_adjust_reservation) for callback inherited %lu(%lu) resvs\n",
                        call->n_resvs, nreq));
        /*
         * Fall through.
         *
         * Note: We don't special-case the callback scall's private socket
         * use. It must reserve some packets.
         */
    }
    else if (RPC_DG_CALL_IS_CLIENT(call) && 
             ((rpc_dg_ccall_p_t) call)->h->is_WAY_binding != 0)
    {                        
        /*
         * We do not allow WAY/WAY2 to adjust the reservation.
         */
        call->n_resvs = ((rpc_dg_ccall_p_t) call)->h->is_WAY_binding;
        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
             ("(rpc__dg_pkt_adjust_reservation) for WAY/WAY2 %lu(%lu) resvs\n",
                        call->n_resvs, nreq));

        return (call->n_resvs >= nreq);
    }

    /*
     * For now, we don't release the already holding reservations.
     * It should be done for KRPC.
     */
    if (nreq <= call->n_resvs)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
              ("(rpc__dg_pkt_adjust_reservation) already has %lu(%lu) resvs\n",
                        call->n_resvs, nreq));
        return true;
    }

    /*
     * Find out how many packets we really need to reserve.
     * We must reserve nreq+1 packets. This additional packet is
     * reserved for the stub. However, we do not recored it in n_resvs.
     * If we already have some reserved packets, then we have the packet
     * for the stub already reserved.
     */

    how_many = nreq - call->n_resvs;

    if (call->n_resvs == 0)
        how_many++;

    RPC_DG_PKT_POOL_LOCK(0); 
       
    /*
     * It may be necessary to block the current call until a reservation can be
     * granted.  Just in case, wrap the following in a loop so that we can 
     * continue trying to acquire a reservation if necessary.
     */
    while (st == rpc_s_ok)
    {
        /*
         * First handle the common case... 
         */
        if (RESERVATION_AVAILABLE(how_many))
        {     
            pool->reservations += how_many;
            call->n_resvs = nreq;
            got_it = true;

            RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
 ("(rpc__dg_pkt_adjust_reservation) available %lu(%lu), current reservations %lu\n",
                            call->n_resvs, nreq, pool->reservations));
            break;
        }
                   
        /*
         * Next, see if there are any server-only reservations available...
         * iff this is the server's initial reservation
         * (should be for RPC_C_DG_MUST_RECV_FRAG_SIZE).
         *
         * Note: pool->max_resv_pkt should never be less than how_many.
         */
        if (call->n_resvs == 0
            && SRV_RESERVATION_AVAILABLE(call)
            && pool->max_resv_pkt+1 >= (unsigned32)how_many)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas,2,
       ("(rpc__dg_pkt_adjust_reservation) using server-only reservation %lu\n",
                            pool->srv_resv_avail));
            call->n_resvs = pool->max_resv_pkt;
            pool->srv_resv_avail--;
            got_it = true;

            RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
 ("(rpc__dg_pkt_adjust_reservation) available %lu(%lu), current reservations %lu\n",
                            call->n_resvs, nreq, pool->reservations));
            break;
        }

        /*
         * It's not possible to grant a reservation at this time.  If the caller 
         * specified non-blocking operation, return now.
         */
        if (!block)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
    ("(rpc__dg_pkt_adjust_reservation) not available %lu(%lu), not blocking\n",
                            call->n_resvs, nreq));

            RPC_DG_PKT_POOL_UNLOCK(0);  
            return false;
        }

        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
("(pkt_rpc__dg_pkt_adjust_reservation) blocking call %lu(%lu), pkts %lu [%s]\n",
            call->n_resvs, nreq,
            pool->free_count + pool->pkt_count, 
            rpc__dg_act_seq_string(&call->xq.hdr)));
            
        call->n_resvs_wait = how_many;

        /*
         * If the call is not already on the reservation waiters' chain, queue
         * it.  Update the call's reference count. 
         */
        if (! call->is_in_pkt_chain)
        {
            if (pool->rsv_waiters_head == NULL) 
                pool->rsv_waiters_head = call; 
            else 
               pool->rsv_waiters_tail->pkt_chain = call; 
            pool->rsv_waiters_tail = call; 
            call->pkt_chain = NULL; 
            call->is_in_pkt_chain = true; 

				printf("here 1\n");
				
            RPC_DG_CALL_REFERENCE(call);
        }

        if (using_private_socket == false)
            RPC_UNLOCK(0);

        RPC_DG_PKT_POOL_UNLOCK(0);

			printf("here 2\n");
        rpc__dg_call_wait(call, rpc_e_dg_wait_on_internal_event, &st);
			printf("here 3\n");
   
        /*
         * Re-acquire all the locks, in the right order.
         */ 
     
        if (using_private_socket == false)
        {
            RPC_DG_CALL_UNLOCK(call);
            RPC_LOCK(0);
            RPC_DG_CALL_LOCK(call);
        }
        RPC_DG_PKT_POOL_LOCK(0);

        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
       ("(pkt_rpc__dg_pkt_adjust_reservation) call signalled, pkts %lu [%s]\n",
            pool->free_count + pool->pkt_count, 
            rpc__dg_act_seq_string(&call->xq.hdr)));
    } 

    /*
     * Before returning, check to see if this call is still on the waiters'
     * queue.  This could happen if 1) the call got signalled for some
     * other reason, but woke up to find that rationing was no longer
     * in effect, or 2) call_wait returned with an error status.
     */
    if (call->is_in_pkt_chain)
        dequeue_pool_waiter(call, &pool->rsv_waiters_head, 
                                  &pool->rsv_waiters_tail);  

    if (got_it == true)
        UPDATE_RATIONING_STATE(0);
    RPC_DG_PKT_POOL_UNLOCK(0);  

    /*
     * Update the original scall/ccall's reservation.
     *
     * What is the locking requirement? Can we assume that the original
     * scall/ccall is always locked when this is called?
     */
    if (got_it == true && call->is_cbk == true)
    {
        if (RPC_DG_CALL_IS_CLIENT(call))
        {
            ((rpc_dg_ccall_p_t) call)->cbk_scall->c.n_resvs = call->n_resvs;
        }
        else
        {
            ((rpc_dg_scall_p_t) call)->cbk_ccall->c.n_resvs = call->n_resvs;
        }
        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
("(rpc__dg_pkt_adjust_reservation) for callback updated the original scall/ccall %lu(%lu) resvs\n",
                        call->n_resvs, nreq));
    }
    return got_it;
}               


/*
 * R P C _ _ D G _ P K T _ C A N C E L _ R E S E R V A T I O N
 *
 * Cancel the reservation owned by this call.  Consider waking up a blocked
 * call if the system is not rationing.
 *
 * The input call handle is expected to be locked.
 */                                                              

PRIVATE void rpc__dg_pkt_cancel_reservation
(
    rpc_dg_call_p_t call                                
)
{ 
    rpc_dg_pkt_pool_t *pool = &rpc_g_dg_pkt_pool;

    RPC_DG_CALL_LOCK_ASSERT(call);
 
    /*
     * It's possible that the call handle does not have a reservation.
     * For instance, it might have been blocked waiting for a reservation
     * when a quit came in.  In such a case, just return.
     */

    if (call->n_resvs == 0)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
                ("(rpc__dg_pkt_cancel_reservation) had no reservation\n"));
        return;
    }

    /*
     * Callback handles and WAY/WAY2 handles inherit the reservation
     * made for the original scall/ccall.  To cancel the reservation,
     * simply reset the handle's flag, but leave the actual reservation
     * count as it is.
     */

    if (call->is_cbk == true ||
        (RPC_DG_CALL_IS_CLIENT(call) && 
        ((rpc_dg_ccall_p_t) call)->h->is_WAY_binding != 0))
    {                        
        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
                ("(rpc__dg_pkt_cancel_reservation) for callback handle\n"));

        call->n_resvs = 0;
        return;
    }
        
    /*
     * Otherwise, we'll really need to modify the pool.
     */

    RPC_DG_PKT_POOL_LOCK(0);  

    pool->reservations -= (call->n_resvs + 1); 
    call->n_resvs = 0;

    /*
     * Giving up this reservation may have taken the system out of a rationing
     * state.  If so, try to reclaim any pre-reserved server reservations that
     * we may have handed out.
     */

    while (RESERVATION_AVAILABLE(pool->max_resv_pkt+1) &&
        pool->srv_resv_avail < RPC_C_DG_PKT_INIT_SERVER_RESVS)
    {
        pool->reservations += (pool->max_resv_pkt + 1);
        pool->srv_resv_avail++;
    }
                                                                           
    RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
            ("(rpc__dg_pkt_cancel_reservation) %lu reservations left\n", pool->reservations));

    /*
     * Now that we've made another reservation/packets available to the system, see if 
     * there are any calls blocked that can be woken up.
     */

    if (pool->rsv_waiters_head != NULL || pool->pkt_waiters_head != NULL)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_pkt_quotas, 2, 
                ("(rpc__dg_pkt_cancel_reservation) calling list scanner\n"));

        scan_waiter_lists(call);                                     
    }
    UPDATE_RATIONING_STATE(0);

    RPC_DG_PKT_POOL_UNLOCK(0);  
}

/*
 * R P C _ M G M T _ S E T _ M A X _ C O N C U R R E N C Y
 *
 * This call allows an application (currently only DFS) to specify how many call
 * threads it must be able to run concurrently.  The runtime uses this information
 * to make sure there are enough packets in the packet pool to satisfy the 
 * requirements of the packet rationing algorithm.
 */                                                              

PUBLIC void rpc_mgmt_set_max_concurrency
(
    unsigned32 max_client_calls,
    unsigned32 max_server_calls,
    unsigned32 *status
)
{
    unsigned32 new_max;

    /*
     * The packet rationing algorithm requires max_resv_pkt+1 packets
     * for each concurrent rpc thread, plus RPC_C_DG_MAX_NUM_PKTS_IN_FRAG
     * for the listener thread.
     */
    new_max = (rpc_g_dg_pkt_pool.max_resv_pkt + 1)
        * (max_client_calls + max_server_calls) + RPC_C_DG_MAX_NUM_PKTS_IN_FRAG;

    RPC_DG_PKT_POOL_LOCK(0);  

    *status = rpc_s_ok;

    /*
     * Only allow the packet pool to be expanded.
     */
    if (new_max > rpc_g_dg_pkt_pool.max_pkt_count)
    {
        rpc_g_dg_pkt_pool.pkt_count += new_max - rpc_g_dg_pkt_pool.max_pkt_count;
        rpc_g_dg_pkt_pool.max_pkt_count = new_max;
    }

    RPC_DG_PKT_POOL_UNLOCK(0);  
}

/*
 * R P C _ M G M T _ G E T _ M A X _ C O N C U R R E N C Y
 *
 * This call allows an application (currently only DFS) to inquire about the 
 * maximum number of call threads that can be run concurrently.
 */                                                              

PUBLIC unsigned32 rpc_mgmt_get_max_concurrency(void)
{
    unsigned32 temp;

    RPC_DG_PKT_POOL_LOCK(0);  

    /*
     * The listener thread has the reservation for
     * RPC_C_DG_MAX_NUM_PKTS_IN_FRAG.
     */

    temp = (rpc_g_dg_pkt_pool.max_pkt_count - RPC_C_DG_MAX_NUM_PKTS_IN_FRAG)
           / (rpc_g_dg_pkt_pool.max_resv_pkt + 1);

    RPC_DG_PKT_POOL_UNLOCK(0);  

    return (temp);
}

/* 
 * R P C _ _ D G _ P K T _ I S _ R A T I O N I N G
 *
 * Return the packet pool's rationing state.
 */          
PRIVATE boolean32 rpc__dg_pkt_is_rationing
(
 boolean32 *low_on_pkts
)
{
    boolean32 is_rationing;

    /*
     * We think that checking flags without the pkt pool locked is ok
     * because there is always a window in which the rationing state may
     * change.
     */

    /*
     * First, determine if we are currently rationing packets.
     */ 
    is_rationing = rpc_g_dg_pkt_pool.is_rationing;

    /*
     * See if the pool is getting low. Only
     * rpc__dg_call_xmit_fack()::dgcall.c uses this information to begin
     * backing off the sender. Avoid doing the computation if
     * is_rationing is already set;
     */
    if (low_on_pkts != NULL)
    {
        *low_on_pkts = rpc_g_dg_pkt_pool.low_on_pkts;
    }


    return(is_rationing);
}
