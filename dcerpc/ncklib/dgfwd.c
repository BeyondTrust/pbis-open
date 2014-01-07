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
**      dgfwd.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG Packet Forwarding handler
**
**
*/

#include <dg.h>
#include <comfwd.h>
#include <dgfwd.h>
#include <dgpkt.h>

/* ======= */

INTERNAL rpc_dg_fpkt_hdr_t fhdr;

/* ======= */

INTERNAL void fwd_reject (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL void fwd_forward (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_addr_p_t  /*fwd_addr*/
    );

INTERNAL void fwd_delayed (
    rpc_dg_sock_pool_elt_p_t /*sp*/,
    rpc_dg_recvq_elt_p_t /*rqe*/
    );

/* ======= */

/*
 * R P C _ _ D G _ F W D _ P K T
 *
 * Attempt to forward a packet.  By the time we're called, it's been
 * determined that the packets NOT for the local server.  Call the
 * forwarding map function to see what it wants to do with the packet.
 * It can say "drop", "reject", or "forward".  In the last case, it
 * supplies the RPC addr of a place to forward to.  The "reject" case
 * causes us to send a reject packet to the client.
 *
 * For now, perform all forwarding  processing from the context of the
 * calling (listener) thread (versus some queued / forwarder thread
 * approach).
 *
 * Can return three values:
 *     FWD_PKT_NOTDONE	- caller should handle packet
 *     FWD_PKT_DONE     - we handled the packet, ok to free it
 *     FWD_PKT_DELAYED  - we saved it, don't handle it, don't free it.
 * 
 * WARNING: This implementation is NOT re-enterent.
 */

PRIVATE unsigned32 rpc__dg_fwd_pkt (
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe)
{
    rpc_dg_pkt_hdr_p_t  hdrp = rqe->hdrp;
    rpc_if_id_t         if_id;
    unsigned32          rpc_prot_vers_major; 
    unsigned32          rpc_prot_vers_minor; 
    rpc_addr_p_t        fwd_addr;
    rpc_fwd_action_t    fwd_action;
    unsigned32          st;


    /*
     * First determine whether or not the pkt is for the forwarder server.
     * There are a few approaches that could be taken.  We can (a) filter
     * based on interface id (i.e. if the pkt is for a interface that
     * is supported by the forwarder server, including conv_, then we
     * can handle it just like a normal pkt).  We could (b) try to do
     * something based on a combination of activity id and interface
     * ids (e.g. is the pkt for an activity that we know about...).  Lastly
     * (perhaps) we could (c) see if we can find a forwarder match and
     * if not then just handle it normally.
     *
     * For now we use method (c) since most pkts to the forwarder will
     * be for forwarding not for operations on the forwarder.  Note:
     * NCS 1.5.1 RPC_C_DG_PT_ACK and RPC_C_DG_PT_REJECT pkts do not have
     * a valid interface id so we just assume that they must be for the
     * forwarder server (i.e. they couldn't have ever been forwarded).
     * Also, a pkt with interface id nil is not valid so don't try to 
     * forward it.
     */

    if (RPC_DG_HDR_INQ_PTYPE(hdrp) == RPC_C_DG_PT_ACK || 
        RPC_DG_HDR_INQ_PTYPE(hdrp) == RPC_C_DG_PT_REJECT || 
        UUID_IS_NIL(&hdrp->if_id, &st))
    {
        return (FWD_PKT_NOTDONE);
    }

    if_id.uuid = hdrp->if_id;
    if_id.vers_major = RPC_IF_VERS_MAJOR(hdrp->if_vers);
    if_id.vers_minor = RPC_IF_VERS_MINOR(hdrp->if_vers);

    rpc_prot_vers_major = RPC_IF_VERS_MAJOR(RPC_C_DG_PROTO_VERS);
    rpc_prot_vers_minor = RPC_IF_VERS_MINOR(RPC_C_DG_PROTO_VERS);

    /*
     * Invoke the endpoint mapper's registered forwarding map
     * function to locate an appropriate forwarding addr.
     */

    /* !!! RPC_UNLOCK_ASSERT(0); couldn't fix mainline com.h so forget it for now */
    (* rpc_g_fwd_fn) (
                    &hdrp->object,
                    &if_id,
                    &ndr_g_transfer_syntax.id,
                    (rpc_protocol_id_t) RPC_C_PROTOCOL_ID_NCADG,
                    rpc_prot_vers_major,
                    rpc_prot_vers_minor,
                    (rpc_addr_p_t) &rqe->from,
                    &hdrp->actuid,
                    &fwd_addr,
                    &fwd_action,
                    &st);

    if (st != rpc_s_ok)
    {
        RPC_DBG_GPRINTF(
            ("(rpc__dg_fwd_pkt) fwd map function returned error (st=%08lx, ptype=%s) [%s]\n", 
            st,
            rpc__dg_pkt_name(RPC_DG_HDR_INQ_PTYPE(hdrp)), 
            rpc__dg_act_seq_string(hdrp)));
        return (FWD_PKT_NOTDONE);
    }

    /*
     * Do what we're told to do with this packet.
     */

    switch (fwd_action)
    {
        case rpc_e_fwd_drop:
            RPC_DBG_PRINTF(rpc_e_dbg_general, 10, 
                ("(rpc__dg_forward_pkt) dropping (ptype=%s) [%s]\n", 
                rpc__dg_pkt_name(RPC_DG_HDR_INQ_PTYPE(hdrp)), 
                rpc__dg_act_seq_string(hdrp)));
            return (FWD_PKT_NOTDONE);

        case rpc_e_fwd_reject:
            fwd_reject(sp, rqe);
            return (FWD_PKT_DONE);

        case rpc_e_fwd_forward:
            fwd_forward(sp, rqe, fwd_addr);
            return (FWD_PKT_DONE);

        case rpc_e_fwd_delayed:
            fwd_delayed(sp, rqe);
            return(FWD_PKT_DELAYED);
			default:
				fprintf(stderr, "%s: unhandled fwd_action %d[%x]; aborting\n",
						__PRETTY_FUNCTION__, fwd_action, fwd_action);
				abort();
    }
}


/*
 * F W D _ R E J E C T
 *
 * Send a reject packet back to the client that sent us this packet.  (Don't
 * reject if the packet was broadcast.)
 */

INTERNAL void fwd_reject (
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe)
{

    RPC_DBG_PRINTF(rpc_e_dbg_general, 10, 
        ("(fwd_forward) rejecting (ptype=%s) [%s]\n", 
        rpc__dg_pkt_name(RPC_DG_HDR_INQ_PTYPE(rqe->hdrp)), 
        rpc__dg_act_seq_string(rqe->hdrp)));

    if (! RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_BROADCAST))
    {
        rpc__dg_xmit_error_body_pkt(
            sp->sock, (rpc_addr_p_t) &rqe->from, rqe->hdrp, RPC_C_DG_PT_REJECT, 
            nca_s_unk_if);      /* !!! status could be better */
    }
}


/*
 * F W D _ F O R W A R D
 *
 * Send the pkt on it's way, embellished with the address and drep of the
 * original sender (i.e. the pkt looks like a rpc_dg_fpkt_t).
 */

INTERNAL void fwd_forward (
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_addr_p_t fwd_addr)
{
    rpc_dg_pkt_hdr_p_t  hdrp = rqe->hdrp;
    rpc_dg_raw_pkt_hdr_p_t rhdrp = &rqe->pkt->hdr;
    rpc_socket_iovec_t  iov[3];
    boolean             b;
    unsigned32          st;

    RPC_DBG_PRINTF(rpc_e_dbg_general, 10, 
        ("(fwd_forward) forwarding (ptype=%s) [%s]\n", 
        rpc__dg_pkt_name(RPC_DG_HDR_INQ_PTYPE(hdrp)), 
        rpc__dg_act_seq_string(hdrp)));


#ifndef MISPACKED_HDR

    /*
     * Create the fpkt hdr.
     */

    fhdr.len = rqe->from.len;

    /*b_c_o_p_y((byte_p_t) &rqe->from.sa, (byte_p_t) &fhdr.addr, fhdr.len);*/
    memmove((byte_p_t)&fhdr.addr, (byte_p_t)&rqe->from.sa, fhdr.len) ;

    fhdr.drep[0] = hdrp->drep[0];
    fhdr.drep[1] = hdrp->drep[1];
    fhdr.drep[2] = hdrp->drep[2];
    fhdr.drep[3] = 0;

    /*
     * Set up the 1st two components of the forwarded pkt.  Note that
     * the header we're sending out is the original (raw) one we received
     * (i.e., not the potentially byte-swapped one).  Authentication
     * checksumming requires that we do this.  (The checksum is produced
     * on the original header and the forwardee must do the same.)   The
     * forwardee will byte swap again (on a copy, like we did) if it
     * has too.
     *
     * Note that on MISPACKED_HDR systems we're counting on the fact
     * that rpc__dg_xmit_pkt will recognize (based on iov[0].len) that
     * the header must NOT be compressed.
     */

    iov[0].iov_base = (byte_p_t) rhdrp;
    iov[0].iov_len  = RPC_C_DG_RAW_PKT_HDR_SIZE;
    iov[1].iov_base = (byte_p_t) &fhdr;
    iov[1].iov_len  = sizeof(rpc_dg_fpkt_hdr_t);

    /*
     * Note that the original pkt may already be the max size, forcing
     * us to forward the pkt in two pieces.  We use the
     * "initial_max_pkt_size" for comparison so that a 2.0 rpcd can work
     * correctly with 1.5.1 servers (that are bound to a 1.5.1 libnck).
     * N.B. don't confuse all this with packet fragmentation and
     * reassembly, which is network-visible, not a intra-machine hack like
     * this is.
     *
     * Note that the current form (size) of the fpkt header's forwarding
     * address (a sockaddr) limits our ability to perform forwarding
     * for "large" address transports.  There are several schemes that
     * we can use in the future (once we have to deal with a larger address
     * transport) to work around this deficiency.  We could (a) decide
     * to use a pkt header flag to tag new format fwd pkts, (b) just
     * use more space (a new format) for for those transports that require
     * it and let recv_pkt() deduce the format knowing the transport,
     * (c) make clients using such transports not use this forwarding
     * mechanism (i.e. clients would call ep_map() directly at the start
     * of a call).  This latter scheme may become necessary once the
     * address exceeds some threashold since there's only so much space
     * in a pkt once and the address gets too large, this forwarding
     * scheme becomes undesireable, there won't be much (any) room left
     * for data.
     * 
     * Mark the pkt as forwarded and send it (them).  Note that we're
     * setting bits in the original packet header (via "rhdrp"), NOT
     * the potentially byte-swapped header (via "hdrp").
     */

    rhdrp->hdr[RPC_C_DG_RPHO_FLAGS] |= RPC_C_DG_PF_FORWARDED;

    if (rqe->pkt_len + sizeof(rpc_dg_fpkt_hdr_t) <= RPC_C_DG_INITIAL_MAX_PKT_SIZE)
    {
        iov[2].iov_base = ((byte_p_t) rqe->pkt) + RPC_C_DG_RAW_PKT_HDR_SIZE;
        iov[2].iov_len  = rqe->pkt_len - RPC_C_DG_RAW_PKT_HDR_SIZE;

        rpc__dg_xmit_pkt(sp->sock, fwd_addr, iov, 3, &b);
    }
    else
    {
        unsigned8 orig_len_b0 = rhdrp->hdr[RPC_C_DG_RPHO_LEN];
        unsigned8 orig_len_b1 = rhdrp->hdr[RPC_C_DG_RPHO_LEN + 1];

        /*
         * The first piece just contains the sender's address 
         */

        rhdrp->hdr[RPC_C_DG_RPHO_FLAGS2] |= RPC_C_DG_PF2_FORWARDED_2;

        rhdrp->hdr[RPC_C_DG_RPHO_LEN]     = 0;
        rhdrp->hdr[RPC_C_DG_RPHO_LEN + 1] = 0;

        rpc__dg_xmit_pkt(sp->sock, fwd_addr, iov, 2, &b);
        if (b)
        {
            /*
             * The second piece just contains the original pkt.
             */

            rhdrp->hdr[RPC_C_DG_RPHO_FLAGS]  &= ~RPC_C_DG_PF_FORWARDED;
            rhdrp->hdr[RPC_C_DG_RPHO_FLAGS2] &= ~RPC_C_DG_PF2_FORWARDED_2;

            rhdrp->hdr[RPC_C_DG_RPHO_LEN]     = orig_len_b0;
            rhdrp->hdr[RPC_C_DG_RPHO_LEN + 1] = orig_len_b1;

            iov[1].iov_base = ((byte_p_t) rqe->pkt) + RPC_C_DG_RAW_PKT_HDR_SIZE;
            iov[1].iov_len  = rqe->pkt_len - RPC_C_DG_RAW_PKT_HDR_SIZE;

            rpc__dg_xmit_pkt(sp->sock, fwd_addr, iov, 2, &b);
        }
    }

#else

#error "No code for MISPACKED_HDR!" 

#endif

    rpc__naf_addr_free(&fwd_addr, &st);
}

/*
 * Variables for the list of delayed packets
 */
typedef struct pkt_list_element {
	struct pkt_list_element *next;
	rpc_dg_recvq_elt_p_t rqe;
	rpc_dg_sock_pool_elt_p_t sp;
} pkt_list_element_t;

INTERNAL rpc_mutex_t                  fwd_list_mutex;
INTERNAL pkt_list_element_t           *delayed_pkt_head = NULL;

/*
 * I N I T _ F W D _ L I S T _ M U T E X
 * 
 * Routine to init the fwd_list mutex
 */
PRIVATE void rpc__dg_fwd_init(void)
{

   RPC_MUTEX_INIT(fwd_list_mutex);

}

/*
 * F W D _ D E L A Y E D
 *
 * Save the original packet as is until rpc__server_fwd_resolve_delayed
 * is called to tell us where to send it.
 */

INTERNAL void fwd_delayed (
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe)
{
    pkt_list_element_t	*new_pkt;

    /* save the packet on our list */
    RPC_MEM_ALLOC(new_pkt,
                  pkt_list_element_t *,
                  sizeof(pkt_list_element_t),
                  RPC_C_MEM_UTIL,
                  RPC_C_MEM_WAITOK);

    new_pkt->rqe = rqe;
    new_pkt->sp = sp;
    /* incremenet the reference count */
    rpc__dg_network_sock_reference(sp);

    RPC_MUTEX_LOCK(fwd_list_mutex);

    new_pkt->next = delayed_pkt_head;
    delayed_pkt_head = new_pkt;

    RPC_MUTEX_UNLOCK(fwd_list_mutex);

    return;
}

/*
 * R P C _ _ S E R V E R _ F W D _ R E S O L V E _ D E A L Y E D
 * 
 * Remove specified packet from the list of delayed packets
 * and do what we are told with it
 */
PRIVATE void rpc__server_fwd_resolve_delayed(
    dce_uuid_p_t             actuuid,
    rpc_addr_p_t	fwd_addr,
    rpc_fwd_action_t	*fwd_action,
    unsigned32		*status)

{
    rpc_dg_sock_pool_elt_p_t 	sp;
    rpc_dg_recvq_elt_p_t 	rqe = (rpc_dg_recvq_elt_p_t)-1;
    rpc_dg_pkt_hdr_p_t  	hdrp;
    pkt_list_element_t          *ep, *last_ep = NULL;
    unsigned32 			st;

    /* get the requsted packet from the list */
    *status = rpc_s_not_found;

    RPC_MUTEX_LOCK(fwd_list_mutex);

    ep = delayed_pkt_head;
    while (ep != NULL)
    {
        hdrp = ep->rqe->hdrp;
        if (dce_uuid_equal(&(hdrp->actuid), actuuid, &st) && (st == rpc_s_ok))
        {
            /* found - remove it from the list */
            rqe = ep->rqe;
            sp = ep->sp;
            if (last_ep == NULL)
            {
                delayed_pkt_head  = ep->next;
            }
            else
            {
                last_ep->next  = ep->next;
            }
            RPC_MEM_FREE(ep, RPC_C_MEM_UTIL);
            *status = rpc_s_ok;
            break;
        }
        last_ep = ep;
        ep = ep->next;
    }
    RPC_MUTEX_UNLOCK(fwd_list_mutex);

    if (*status != rpc_s_ok)
    {
        return;
    }

    /*
     * Do what we're told to do with this packet.
     */
    switch (*fwd_action)
    {
        case rpc_e_fwd_drop:
            RPC_DBG_PRINTF(rpc_e_dbg_general, 10, 
               ("(rpc__server_fwd_resolve_delayed) dropping (ptype=%s) [%s]\n", 
                rpc__dg_pkt_name(RPC_DG_HDR_INQ_PTYPE(rqe->hdrp)), 
                rpc__dg_act_seq_string(rqe->hdrp)));
            break;

        case rpc_e_fwd_reject:
            fwd_reject(sp, rqe);
            break;

        case rpc_e_fwd_forward:
            fwd_forward(sp, rqe, fwd_addr);
            break;

        default:
            *status = rpc_s_not_supported;
            break;
    }
    rpc__dg_network_sock_release(&sp);
	 if (rqe == (rpc_dg_recvq_elt_p_t)-1)	{
		 fprintf(stderr, "%s: bad rqe: aborting\n", __PRETTY_FUNCTION__);
		 abort();
	 }
    rpc__dg_pkt_free_rqe(rqe, NULL);
    return;
}
