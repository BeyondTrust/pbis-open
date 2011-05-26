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
**      dgsoc.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG socket manipulation routines.
**
**
*/

#include <dg.h>
#include <dgsoc.h>
#include <dgpkt.h>

/* ========================================================================= */

INTERNAL void sock_free (
        rpc_dg_sock_pool_elt_p_t * /*sp_elt*/
    );

INTERNAL void use_protseq (
        boolean32  /*is_server*/,
        rpc_protseq_id_t  /*pseq_id*/,
        rpc_addr_p_t  /*rpc_addr*/,
        unsigned_char_p_t  /*endpoint*/,
        rpc_dg_sock_pool_elt_p_t * /*sock_pool_elt*/,
        unsigned32 * /*st*/
    ); 

/* ========================================================================= */

/*
 * S O C K _ F R E E
 *
 * This routine is called to *really* cleanup and free a pool elt.
 */

INTERNAL void sock_free
(
    rpc_dg_sock_pool_elt_p_t *sp
)
{
    rpc_socket_error_t serr;
    rpc_dg_sock_pool_elt_p_t eltp, *peltp;
    boolean32 is_private_socket = (*sp)->is_private;

    RPC_DG_SOCK_POOL_LOCK_ASSERT(0);
           
    /*
     * Close the socket desc.
     */
    serr = RPC_SOCKET_CLOSE((*sp)->sock);
    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_GPRINTF((
            "(sock_free) Error closing socket %d, error=%d\n",
            (*sp)->sock, RPC_SOCKET_ETOI(serr)));
    }

    /*
     * Decide whether this socket will be on the private or shared list,
     * and then remove it from the pool.
     */
    peltp = (is_private_socket) ? &rpc_g_dg_sock_pool.private_sockets :
                                  &rpc_g_dg_sock_pool.shared_sockets;

    if (*sp == *peltp)
    {
        *peltp = (*sp)->next;
    }
    else
    {
        for (eltp = *peltp; eltp != NULL; eltp = eltp->next)
        {
            if (eltp->next == *sp)
            {
                eltp->next = (*sp)->next;
                break;
            }
        }
    }

    /*
     * For private sockets, free the private packets associated with
     * the socket.
     */
    if (is_private_socket)
    {
        if ((*sp)->xqe != NULL)
            RPC_MEM_FREE((*sp)->xqe, RPC_C_MEM_DG_PKT_POOL_ELT);
        if ((*sp)->rqe != NULL)
            RPC_MEM_FREE((*sp)->rqe, RPC_C_MEM_DG_PKT_POOL_ELT);
        if ((*sp)->rqe_list_len > 0 && (*sp)->rqe_list != NULL)
            rpc__dg_pkt_free_rqe((*sp)->rqe_list, (rpc_dg_call_p_t)(*sp)->ccall);
    }

    rpc_g_dg_sock_pool.num_entries--;
    
    /*
     * Free up the elt's memory.
     */
    RPC_MEM_FREE(*sp, RPC_C_MEM_DG_SOCK_POOL_ELT);

    *sp = NULL;
}

           
/*
 * U S E _ P R O T S E Q
 *
 * Common, internal socket pool allocation routine.  Find/create a suitable
 * socket pool entry and add it to the listener's database.
 */

INTERNAL void use_protseq
(
    boolean32 is_server,
    rpc_protseq_id_t pseq_id,
    rpc_addr_p_t rpc_addr,
    unsigned_char_p_t endpoint,
    rpc_dg_sock_pool_elt_p_t *sock_pool_elt,
    unsigned32 *st
)
{
    boolean32 sock_open = false;
    boolean32 creating_private_socket = false;
    rpc_socket_error_t serr;
    rpc_socket_t socket_desc;
    rpc_dg_sock_pool_elt_p_t eltp;
    unsigned32 sndbuf, rcvbuf;
    unsigned32 priv_sock_count = 0;
    unsigned32 desired_sndbuf, desired_rcvbuf;
    
    *st = rpc_s_ok;

    RPC_DG_SOCK_POOL_LOCK(0);
    
    /*
     * For clients, first see if there's already an open socket of the
     * correct type and if so, return a ref to it. Note: servers always
     * create new entries.
     */
                                    
    if (! is_server)
    {
        unsigned32 max_priv_socks = RPC_C_DG_SOCK_MAX_PRIV_SOCKS;
#ifdef DEBUG
        if (RPC_DBG (rpc_es_dbg_dg_max_psock, 1))
        {
            max_priv_socks =
                (unsigned32)(rpc_g_dbg_switches[(int)rpc_es_dbg_dg_max_psock])
                    - 1;
        }
#endif
        /*
         * First, look in the list of private sockets.  If we find one
         * that is only referenced by the socket pool, use it.  While
         * looking, keep a count of how many private sockets exist for
         * this protseq.  Since there's a limit on the number of private
         * sockets we'll create, we'll use this count later (if we have
         * to create a socket pool entry) to decide whether to create
         * a private or shared socket.
         */

        for (eltp = rpc_g_dg_sock_pool.private_sockets; eltp != NULL;
             eltp = eltp->next)
        {
            if (eltp->pseq_id == pseq_id)
            {
                priv_sock_count++;

                if (! eltp->is_disabled && eltp->refcnt == 1)
                {
                    eltp->refcnt++;     /* for the ref we're returning */
                    *sock_pool_elt = eltp;
                    RPC_DG_SOCK_POOL_UNLOCK(0);

                    RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 3, (
                            "(use_protseq) using private socket\n"));

                    return;
                }
            }
        }

        /*
         * Didn't find a private socket.  If we haven't already created
         * the maximum number of private sockets for this protseq, then
         * we'll create another one.
         */

        if (priv_sock_count < max_priv_socks)
        {
            creating_private_socket = true;
        }
        else
        {
            /*
             * Else, we've maxed out the private sockets.  See if we
             * have a shared socket for this protseq.  If so, return
             * it.  If not, drop through and create a shared socket.
             */
            for (eltp = rpc_g_dg_sock_pool.shared_sockets; eltp != NULL;
                 eltp = eltp->next)
            {
                if (! eltp->is_server && eltp->pseq_id == pseq_id &&
                    ! eltp->is_disabled)
                {
                    eltp->refcnt++;     /* for the ref we're returning */
                    *sock_pool_elt = eltp;
                    RPC_DG_SOCK_POOL_UNLOCK(0);

                    RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 3, (
                            "(use_protseq) using shared socket\n"));

                    return;
                }
            }
        }
    }  
    

    RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 3, (
            "(use_protseq) allocating a %s socket\n",
            creating_private_socket ? "private" : "shared"));

    /* 
     * Allocate a new pool entry and initialize it.
     */ 
    RPC_MEM_ALLOC(eltp, rpc_dg_sock_pool_elt_p_t, 
        sizeof(rpc_dg_sock_pool_elt_t), RPC_C_MEM_DG_SOCK_POOL_ELT, 
        RPC_C_MEM_NOWAIT);
    
    /*
     * Create a network descriptor for this RPC Protocol Sequence.
     */
    serr = rpc__socket_open(pseq_id, NULL, &socket_desc);

    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_GPRINTF(("(use_protseq) Can't create socket, error=%d\n",
            RPC_SOCKET_ETOI(serr)));
        *st = rpc_s_cant_create_sock;
        goto CLEANUP;
    }
    sock_open = true;
     
    /*
     * Bind the socket (Network descriptor) to the RPC address.
     */ 

    rpc_addr->rpc_protseq_id = pseq_id;

    serr = rpc__socket_bind(socket_desc, rpc_addr);

    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_GPRINTF(("(use_protseq) Can't bind socket, error=%d\n",
            RPC_SOCKET_ETOI(serr)));
        *st = rpc_s_cant_bind_sock;
        goto CLEANUP;
    }

    /*
     * Request a change in the amount of system buffering provided
     * to the socket.
     */
    if (creating_private_socket)
    {
        desired_sndbuf = RPC_C_DG_MAX_WINDOW_SIZE_BYTES;
        desired_rcvbuf = RPC_C_DG_MAX_WINDOW_SIZE_BYTES;
    }
    else
    {
        desired_sndbuf = RPC_C_DG_SOCK_DESIRED_SNDBUF;
        desired_rcvbuf = RPC_C_DG_SOCK_DESIRED_RCVBUF;
    }

    serr = rpc__socket_set_bufs(socket_desc,
                                desired_sndbuf, desired_rcvbuf,
                                &sndbuf, &rcvbuf);
            
    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_GPRINTF((
            "(use_protseq) WARNING: Can't set socket bufs, error=%d\n",
            RPC_SOCKET_ETOI(serr)));
    }

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, (
        "(use_protseq) desired_sndbuf %u, desired_rcvbuf %u\n",
        desired_sndbuf, desired_rcvbuf));
    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, (
        "(use_protseq) actual sndbuf %lu, actual rcvbuf %lu\n",
        sndbuf, rcvbuf));
      
    /*
     * For shared sockets, set the socket to do non-blocking IO.
     */

    if (! creating_private_socket)
    {
        rpc__socket_set_nbio(socket_desc);
    }

    /*
     * Set the socket to close itself on exec.
     */
    rpc__socket_set_close_on_exec(socket_desc);

    /*
     * For private sockets, allocate an xqe/rqe pair.  These packets
     * allow calls with small ins/outs to avoid going through the packet
     * pool reservation processing.
     */

    if (creating_private_socket)
    {
        rpc_dg_pkt_pool_elt_p_t   pkt;

        RPC_MEM_ALLOC(pkt, rpc_dg_pkt_pool_elt_p_t,
            sizeof(rpc_dg_pkt_pool_elt_t), RPC_C_MEM_DG_PKT_POOL_ELT,
            RPC_C_MEM_NOWAIT);

        eltp->xqe             = &pkt->u.xqe.xqe;
        eltp->xqe->body       = &pkt->u.xqe.pkt;
        eltp->xqe->next       = NULL;
        eltp->xqe->more_data  = NULL;
        eltp->xqe->frag_len   = 0;
        eltp->xqe->flags      = 0;
        eltp->xqe->body_len   = 0;
        eltp->xqe->serial_num = 0;
        eltp->xqe->in_cwindow = false;

        RPC_MEM_ALLOC(pkt, rpc_dg_pkt_pool_elt_p_t,
            sizeof(rpc_dg_pkt_pool_elt_t), RPC_C_MEM_DG_PKT_POOL_ELT,
            RPC_C_MEM_NOWAIT);

        eltp->rqe = &pkt->u.rqe.rqe;
        eltp->rqe->pkt_real = &pkt->u.rqe.pkt;
        eltp->rqe->pkt = (rpc_dg_raw_pkt_p_t) RPC_DG_ALIGN_8(eltp->rqe->pkt_real);
        eltp->rqe->next   = NULL;
        eltp->rqe->more_data  = NULL;
        eltp->rqe->frag_len   = 0;
        eltp->rqe->hdrp   = NULL;
 
        pkt->u.rqe.sock_ref = eltp;
        eltp->rqe_available = true;
    }
    else
    {
        eltp->xqe = NULL;
        eltp->rqe = NULL;
        eltp->rqe_available = false;
    }

    /*
     * Initialize all fields in the socket pool entry.
     */
    eltp->sock      = socket_desc;
    eltp->rcvbuf    = rcvbuf;
    eltp->sndbuf    = sndbuf;
    eltp->brd_addrs = NULL;
    eltp->ccall     = NULL;
    eltp->rqe_list     = NULL;
    eltp->rqe_list_len = 0;
    eltp->error_cnt = 0;
    eltp->pseq_id   = pseq_id;
    eltp->refcnt    = 2;    /* 1 for table ref + 1 for returned ref */
    eltp->is_server = is_server;
    eltp->is_disabled  = 0;
    eltp->is_private   = creating_private_socket;

    /*
     * Add the new elt to the appropriate list within the socket pool
     * before we unlock it (so others will see it);  note we still have
     * our ref to it.  After this point, call sock_free() to clean things
     * up.
     */
    if (creating_private_socket)
    {
        eltp->next = rpc_g_dg_sock_pool.private_sockets;
        rpc_g_dg_sock_pool.private_sockets = eltp;
    }
    else
    {
        eltp->next = rpc_g_dg_sock_pool.shared_sockets;
        rpc_g_dg_sock_pool.shared_sockets = eltp;
    }

    rpc_g_dg_sock_pool.num_entries++;

    /*
     * Probably best to unlock the pool before calling into the
     * listener abstraction.
     */
    RPC_DG_SOCK_POOL_UNLOCK(0);

    /*
     * For shared sockets, tell the network listener to start listening
     * on this socket (add a reference because we're giving one to it).
     * If this fails, just blow away the new elt.  Note that private
     * sockets do not require the presence of the listener thread.
     */
    if (! creating_private_socket)
    {
        rpc__dg_network_sock_reference(eltp);
        rpc__network_add_desc(socket_desc, is_server, (endpoint == NULL),
                                pseq_id, (pointer_t) eltp, st);

        if (*st != rpc_s_ok)
        {
            RPC_DG_SOCK_POOL_LOCK(0);
            sock_free(&eltp);
            RPC_DG_SOCK_POOL_UNLOCK(0);
        }
    }

    *sock_pool_elt = eltp;
    return;

CLEANUP:

    /*
     * A failure happened; clean things up, returning the previously
     * setup status.
     */
    if (sock_open)
        serr = RPC_SOCKET_CLOSE(socket_desc);

    RPC_MEM_FREE(eltp, RPC_C_MEM_DG_SOCK_POOL_ELT);
    RPC_DG_SOCK_POOL_UNLOCK(0);
    return;
}

         
/*
 * R P C _ _ D G _ N E T W O R K _ U S E _ P R O T S E Q _ S V
 *
 * Server side entry point into socket pool allocation routine. 
 */

PRIVATE void rpc__dg_network_use_protseq_sv
(
    rpc_protseq_id_t pseq_id,
    unsigned32 max_calls ATTRIBUTE_UNUSED,
    rpc_addr_p_t rpc_addr,
    unsigned_char_p_t endpoint,
    unsigned32 *st
)
{   
    rpc_dg_sock_pool_elt_p_t sp_elt;
        
    use_protseq(true, pseq_id, rpc_addr, endpoint, &sp_elt, st);
    if (*st == rpc_s_ok)
        rpc__dg_network_sock_release(&sp_elt);
}


/*
 * R P C _ _ D G _ N E T W O R K _ U S E _ P R O T S E Q _ C L
 *
 * Client side entry point into socket pool allocation routine. 
 * Create a NULL address for the bind, and call the internal
 * use_protseq routine.
 */

PRIVATE void rpc__dg_network_use_protseq_cl
(
    rpc_protseq_id_t pseq_id,
    rpc_dg_sock_pool_elt_p_t *sp
)
{                          
    rpc_addr_il_t addr;
    unsigned32 st;
        
    *sp = NULL;

    /*
     * Specify a null address to bind to.  We do this so
     * the socket gets an endpoint assigned right away.
     */
    addr.len = sizeof addr.sa;
    memset((char *) &addr.sa, 0, addr.len);
    addr.sa.family = rpc_g_protseq_id[pseq_id].naf_id;
        
    use_protseq(false, pseq_id, (rpc_addr_p_t) &addr, NULL, sp, &st);
}


/* 
 * R P C _ _ D G _ N E T W O R K _ D I S A B L E _ D E S C
 *
 * This routine is responsible for removing a socket from the pool of
 * sockets being monitored by the listener thread.  It is called from
 * the macro which updates the error count associated with a socket pool
 * entry, when that count reaches the maximum allowed.
 * Note: the caller's socket pool reference remains intact.
 */

PRIVATE void rpc__dg_network_disable_desc
(
    rpc_dg_sock_pool_elt_p_t sp
)
{                                               
    unsigned32 st;
    boolean is_private;

    RPC_DG_SOCK_POOL_LOCK(0); 

    is_private = sp->is_private;

    /*
     * Make certain it's not already disabled.
     */
    if (sp->is_disabled)
    {
        RPC_DG_SOCK_POOL_UNLOCK(0); 
        return;
    }
       
    RPC_DBG_GPRINTF(("(rpc__dg_network_disable_desc) Disabing socket, error=%d\n",
        sp->sock));

    sp->is_disabled = true;

    RPC_DG_SOCK_POOL_UNLOCK(0); 

    /*
     * For shared sockets, remove the sock from the listener's database
     * and release the reference that we previously gave it.  Note:
     * remove_desc() doesn't return until the listener has completely
     * ceased using the socket/reference (which is why we can safely
     * release its reference).
     */
    if (is_private == false)
    {
        rpc__network_remove_desc(sp->sock, &st);
        rpc__dg_network_sock_release(&sp);
    }
}


/*
 * R P C _ _ D G _ N E T W O R K _ I N I T
 *
 * This routine is called as part of the rpc__init startup 
 * initializations.  It creates the mutex that protects the
 * socket pool.
 */
PRIVATE void rpc__dg_network_init(void)
{
    RPC_MUTEX_INIT(rpc_g_dg_sock_pool.sp_mutex);
}


/*
 * R P C _ _ D G _ N E T W O R K _ F O R K _ H A N D L E R
 * 
 * This routine handles socket pool related fork processing.
 */

PRIVATE void rpc__dg_network_fork_handler
(
    rpc_fork_stage_id_t stage
)
{ 
    rpc_dg_sock_pool_elt_p_t eltp, neltp;

    switch ((int)stage)
    {
        case RPC_C_PREFORK:
            break;
        case RPC_C_POSTFORK_PARENT:
            break;
        case RPC_C_POSTFORK_CHILD:  
            /*
             * In the child of a fork, scan the socket pool
             * for in_use socket descriptors.
             *
             * Note that some of the code below would normally
             * require the socket pool lock.  However, after
             * the fork we are no longer running in a threaded
             * environment.
             */
            for (eltp = rpc_g_dg_sock_pool.private_sockets; eltp != NULL;
                 eltp = neltp)
            {
                (void) RPC_SOCKET_CLOSE(eltp->sock);
                neltp = eltp->next;
                RPC_MEM_FREE(eltp, RPC_C_MEM_DG_SOCK_POOL_ELT);
            }
            rpc_g_dg_sock_pool.private_sockets = NULL;

            for (eltp = rpc_g_dg_sock_pool.shared_sockets; eltp != NULL;
                 eltp = neltp)
            {
                (void) RPC_SOCKET_CLOSE(eltp->sock);
                neltp = eltp->next;
                RPC_MEM_FREE(eltp, RPC_C_MEM_DG_SOCK_POOL_ELT);
            }
            rpc_g_dg_sock_pool.shared_sockets = NULL;
            rpc_g_dg_sock_pool.num_entries = 0;
            break;
    }
}


/*
 * R P C _ _ D G _ N E T W O R K _ S O C K _ R E F E R E N C E
 *
 * Increment the reference count associated with an entry in the socket pool.
 */
PRIVATE void rpc__dg_network_sock_reference(sp)
rpc_dg_sock_pool_elt_p_t sp;  
{  
    RPC_DG_SOCK_POOL_LOCK(0); 
    sp->refcnt++; 
    RPC_DG_SOCK_POOL_UNLOCK(0); 
}


/*
 * R P C _ _ D G _ N E T W O R K _ S O C K _ R E L E A S E
 *
 * Decrement the reference count associated with an entry in the socket pool.
 * If the count falls to 1, only the internal pool's ref remains; if the socket
 * has been marked disabled, free the entry.
 */

PRIVATE void rpc__dg_network_sock_release
(
    rpc_dg_sock_pool_elt_p_t *sp
)
{  
    RPC_DG_SOCK_POOL_LOCK(0); 

    if (--((*sp)->refcnt) == 1 && (*sp)->is_disabled)
        sock_free(sp); 
    else
        (*sp)->ccall = NULL;

    RPC_DG_SOCK_POOL_UNLOCK(0); 
    *sp = NULL;
}

