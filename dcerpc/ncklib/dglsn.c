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
**      dglsn.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG RR Protocol Engine Routines.  Routines that run in the listener thread.
**
**
*/

#include <dg.h>
#include <dgpkt.h>
#include <dgsct.h>
#include <dgcall.h>
#include <dgscall.h>
#include <dgccall.h>
#include <dgccallt.h>
#include <dgslsn.h>
#include <dgclsn.h>
#include <comfwd.h>
#include <dgfwd.h>
#include <dgxq.h>

#include <dce/conv.h>
#include <dce/convc.h>
#include <dce/ep.h>
#include <dce/mgmt.h>

#include <assert.h>

#include <netinet/in.h>


/* ========================================================================= */

INTERNAL void swab_hdr (
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL unsigned32 recv_pkt (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL unsigned32 recv_pkt_private (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL void do_selective_ack (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_call_p_t  /*call*/,
        unsigned32 * /*window_incr*/,
        unsigned32 * /*rexmit_cnt*/,
        unsigned32  /*curr_serial*/
    );

INTERNAL void do_fack_body (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_call_p_t  /*call*/,
        unsigned32 * /*window_incr*/,
        unsigned32 * /*rexmit_cnt*/,
        unsigned32  /*curr_serial*/
    );

INTERNAL boolean do_fack (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL void marshall_uuid (
        char * /*p*/,
        dce_uuid_p_t  /*uuid*/
    );

/* ========================================================================= */

INTERNAL void conv_stub_who_are_you (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_pkt_hdr_p_t  /*resp_hdrp*/,
        dce_uuid_p_t  /*clt_actid*/,
        unsigned32  /*clt_boot*/
    );

INTERNAL void conv_stub_who_are_you2 (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_pkt_hdr_p_t  /*resp_hdrp*/,
        dce_uuid_p_t  /*clt_actid*/,
        unsigned32  /*clt_boot*/
    );

INTERNAL void conv_stub_are_you_there (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_pkt_hdr_p_t  /*resp_hdrp*/,
        dce_uuid_p_t  /*clt_actid*/,
        unsigned32  /*clt_boot*/
    );

INTERNAL void conv_stub_who_are_you_auth (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_pkt_hdr_p_t  /*resp_hdrp*/,
        dce_uuid_p_t  /*clt_actid*/,
        unsigned32  /*clt_boot*/
    );

typedef void (*conv_stub_t) (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_pkt_hdr_p_t  /*resp_hdrp*/,
        dce_uuid_p_t  /*clt_actid*/,
        unsigned32  /*clt_boot*/
    );

INTERNAL void conv_stub_who_are_you_auth_more (
        rpc_dg_recvq_elt_p_t /*rqe*/,
        rpc_dg_pkt_hdr_p_t /*resp_hdrp*/,
        dce_uuid_p_t /*clt_actid*/,
        unsigned32 /*clt_boot*/
    );

/* ================================================================ */

INTERNAL rpc_dg_recvq_elt_p_t saved_rqe_list = NULL;
INTERNAL unsigned32 saved_rqe_list_len = 0;

/* ================================================================ */
/*
 * This macro is used by the selective ack code to see if the current
 * LSB of a bit mask is set.  The bits of a selective ack mask are found
 * by repeatedly shifting the mask right.
 */

#define FRAG_GOT_ACKED(mask) ((mask & 1) != 0)

/* 
 * Congestion window growth in the slow-start algorithm depends on sending
 * out two packets for each packet which gets acknowledged.  Since we
 * don't request a fack for each packet, we need to do more than just
 * count facks;  we need to determine how many packets were sent in the
 * blast which contained the packet which induced the current fack.
 * (Right?  If we blast two packets, which requests a single fack, when
 * we get the fack back we want to blast 4 more packets.)
 *                                                  
 * In a non-loss situation, this number is simply the number of packets
 * removed from the head of the queue.  However, in the face of
 * retransmissions, it becomes trickier to tell which packets were sent
 * when.  The only assumptions we can make are
 *
 *    1. All packets in a blast are sent left to to right from the xmitq.
 *    2. Within a blast, packet serial numbers increase monotonically.
 *
 * At any time, the xmitq can be looked at as a collection of blasts
 * that have been sent but not yet facked.  Our goal is to find/count
 * the blast which induced the current fack.  This turns out to be simpler
 * to do than to explain.
 *
 * We start with the serial number which induced the *last* fack
 * (curr_ser).  In the no-loss case, the serial number of the first packet
 * in the 'current blast' will be 1 greater than curr_ser.  If we scan
 * past this packet, increment the number by which we will increase the
 * window (window_incr), and begin looking for the next serial number.
 * This search should end when we reach, monotonically, the packet which
 * induced the current fack.
 * 
 * We need to watch out for the case where a fack gets lost, and the
 * current fack is informing us of two blasts worth of received packets.
 * We avoid counting both blasts by noticing that there is a fack request
 * within the current blast being scanned, and starting the count over
 * (unless, of course, the fack request is the one which actually induced
 * the current fack).
 *                     
 * Finally, in the event of heavy loss, the serial number space in the
 * xmitq can become fairly random.  Therefore, if we hit a packet which
 * has a higher serial number than curr_ser, it must be part of a more
 * current blast than the one we're previously scanning.  If it wasn't
 * sent after the packet which induced this fack (compare serial numbers)
 * then consider it the new 'current blast' and start the window_incr
 * over. 
 *
 * Since these caluculations need to be performed in three different
 * places, I have moved them into a macro.  The lost flag is set by the
 * selective ack code when it is scanning a packet which it knows is
 * lost;  basically, it just wants to decide whether to restart
 * window_incr, it never wants to count the current packet in window_incr.
 */

#define TRACK_CURRENT_BLAST(xq, xqe, curr_ser, window_incr, lost)           \
    {                                                                       \
        if ((xqe)->serial_num == (curr_ser) + 1)                            \
        {                                                                   \
            if (! RPC_DG_FLAG_IS_SET((xqe)->flags, RPC_C_DG_PF_NO_FACK) &&  \
                (xqe)->serial_num != (xq)->last_fack_serial)                \
                (*window_incr) = 0;                                         \
            else  if (! lost)                                               \
                (*window_incr)++;                                           \
            curr_ser = (xqe)->serial_num;                                   \
        }                                                                   \
        else if (RPC_DG_SERIAL_IS_LT(curr_ser, (xqe)->serial_num) &&        \
                 RPC_DG_SERIAL_IS_LTE((xqe)->serial_num, (xq)->last_fack_serial))  \
        {                                                                   \
            if (lost ||                                                     \
                (! RPC_DG_FLAG_IS_SET((xqe)->flags, RPC_C_DG_PF_NO_FACK) && \
                (xqe)->serial_num != (xq)->last_fack_serial))               \
                (*window_incr) = 0;                                         \
            else                                                            \
                (*window_incr) = 1;                                         \
            curr_ser = (xqe)->serial_num;                                   \
        }                                                                   \
    }

/* ================================================================ */

#if ! defined(MISPACKED_HDR) && ! defined(MSDOS)

/*
 * S W A B _ H D R
 *
 * Swap bytes in multi-byte integer fields in packet header.  Put results of
 * swapping into "rqe->hdr" and set "rqe->hdrp" to point to that.
 */

INTERNAL void swab_hdr
(
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_dg_pkt_hdr_p_t shdr = (rpc_dg_pkt_hdr_p_t) &rqe->pkt->hdr;
    rpc_dg_pkt_hdr_p_t dhdr = &rqe->hdr;

#define SWAB_HDR_16(field) { \
    dhdr->field = SWAB_16(shdr->field); \
}

#define SWAB_HDR_32(field) { \
    dhdr->field = SWAB_32(shdr->field); \
}

#define SWAB_HDR_UUID(ufield) { \
    dhdr->ufield = shdr->ufield; \
    SWAB_INPLACE_UUID(dhdr->ufield); \
}
    
#define COPY_HDR(field) { \
    dhdr->field = shdr->field; \
}

    COPY_HDR     (_rpc_vers);
    COPY_HDR     (_ptype);
    COPY_HDR     (flags);
    COPY_HDR     (flags2);
    COPY_HDR     (drep[0]);
    COPY_HDR     (drep[1]);
    COPY_HDR     (drep[2]);
    COPY_HDR     (serial_hi);
    SWAB_HDR_UUID(object);
    SWAB_HDR_UUID(if_id);
    SWAB_HDR_UUID(actuid);
    SWAB_HDR_32  (server_boot);
    SWAB_HDR_32  (if_vers);
    SWAB_HDR_32  (seq);
    SWAB_HDR_16  (opnum);
    SWAB_HDR_16  (ihint);
    SWAB_HDR_16  (ahint);
    SWAB_HDR_16  (len);
    SWAB_HDR_16  (fragnum);
    COPY_HDR     (auth_proto);
    COPY_HDR     (serial_lo);

#undef SWAB_HDR_16
#undef SWAB_HDR_32
#undef SWAB_HDR_UUID
#undef COPY_HDR

    rqe->hdrp = &rqe->hdr;
}

#endif


#ifdef MISPACKED_HDR

INTERNAL void unpack_hdr
(
    rpc_dg_recvq_elt_p_t rqe
)
{
    /* !!! Unpack the header !!! */

    rqe->hdrp = &rqe->hdr;

}

#endif /* MISPACKED_HDR */


/*
 * M A R S H A L L _ U U I D 
 *
 */

INTERNAL void marshall_uuid
(
    char *p,
    dce_uuid_p_t uuid
)
{
    *((unsigned32 *) (p +  0)) = uuid->time_low;
    *((unsigned16 *) (p +  4)) = uuid->time_mid;
    *((unsigned16 *) (p +  6)) = uuid->time_hi_and_version;
    *((unsigned8 *)  (p +  8)) = uuid->clock_seq_hi_and_reserved;
    *((unsigned8 *)  (p +  9)) = uuid->clock_seq_low;
    *((unsigned8 *)  (p + 10)) = uuid->node[0];
    *((unsigned8 *)  (p + 11)) = uuid->node[1];
    *((unsigned8 *)  (p + 12)) = uuid->node[2];
    *((unsigned8 *)  (p + 13)) = uuid->node[3];
    *((unsigned8 *)  (p + 14)) = uuid->node[4];
    *((unsigned8 *)  (p + 15)) = uuid->node[5];
}


/*
 * R P C _ _ D G _ G E T _ E P K T _ B O D Y _ S T
 *
 * Unmarshall a packet with a rpc__dg_raw_epkt_body_t body into a
 * unsigned32.
 */

PRIVATE void rpc__dg_get_epkt_body_st
(
    rpc_dg_recvq_elt_p_t rqe,
    unsigned32 *st
)
{
    u_long st_all;
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;

#ifndef MISPACKED_HDR
    st_all = ((rpc_dg_epkt_body_p_t) &rqe->pkt->body)->st;
    if (NDR_DREP_INT_REP(hdrp->drep) != ndr_g_local_drep.int_rep)
        SWAB_INPLACE_32(st_all);
#else
    MISPACKED_HDR_CODE_NEEDED_HERE /* !!! */
#endif

    *st = st_all;
}



/*
 * The conversation manger call execution thread queue.
 *
 * There is a single conversation manager call execution queue.
 * conv requests are added to the queue by the listener thread.
 * The conv execution thread executes and removes these requests.
 */

typedef struct {
    rpc_dg_recvq_elt_p_t    head;
    rpc_dg_recvq_elt_p_t    tail;
    signed32                cnt;
    rpc_mutex_t             m;
    rpc_cond_t              cv;
} convq_t, *convq_p_t;

#ifndef RPC_DG_CONVQ_MAX_LEN
#  define RPC_DG_CONVQ_MAX_LEN    100
#endif

INTERNAL convq_t convq;

INTERNAL dcethread* conv_thread;
INTERNAL boolean convq_running = false, convq_was_running = false;
INTERNAL boolean convq_stop = false;

INTERNAL boolean32 convq_add (
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL void convq_remove (void);

INTERNAL void convq_loop (void);


INTERNAL boolean32 convq_has_act ( dce_uuid_p_t  /*actuid*/);

        
INTERNAL boolean32 handle_conv_int (
        rpc_socket_t  /*sock*/,
        rpc_dg_recvq_elt_p_t  /*rqe1*/
    );


/*
 * C O N V Q _ S T A R T
 *
 * Start the conv thread.
 */

INTERNAL void convq_start(void)
{
    RPC_MUTEX_LOCK(convq.m);
    if (! convq_running)
    {
        convq_running = true;
        dcethread_create_throw(&conv_thread, NULL,
                       (dcethread_startroutine)convq_loop, 
                       NULL);  
    }
    RPC_MUTEX_UNLOCK(convq.m);
}

/*
 * R P C _ _ D G _ C O N V _ I N I T
 *
 * Initialize the convq data.
 */

PRIVATE void rpc__dg_conv_init(void)
{
    convq.head = convq.tail = NULL;
    convq.cnt = 0;
    RPC_MUTEX_INIT(convq.m);
    RPC_COND_INIT(convq.cv, convq.m);
}


/*
 * C O N V Q _ A D D
 *
 * Add to the tail of the queue (and signal waiters as required).
 * Return true iff element was added.
 */

INTERNAL boolean32 convq_add
(
    rpc_dg_recvq_elt_p_t rqe
)
{
    boolean added;

    RPC_MUTEX_LOCK(convq.m);
    if (convq.cnt >= RPC_DG_CONVQ_MAX_LEN)
        added = false;
    else
    {
        added = true;
        if (convq.head == NULL)
            convq.head = rqe;
        else
            convq.tail->next = rqe;
        convq.tail = rqe;
        rqe->next = NULL;
        convq.cnt++;
        if (convq.head == rqe)
            RPC_COND_SIGNAL(convq.cv, convq.m);
    }
    RPC_MUTEX_UNLOCK(convq.m);
    return (added);
}

/*
 * C O N V Q _ R E M O V E
 *
 * Remove the head of the queue.
 * Requires that convq be locked when this is called.
 */

INTERNAL void convq_remove(void)
{
    assert(convq.head != NULL);
    convq.head = convq.head->next;
    if (convq.head == NULL)
        convq.tail = NULL;
    convq.cnt--;
    assert(convq.cnt >= 0);
}

/*
 * C O N V Q _ H A S _ A C T
 *
 * Scan the convq to see if we already have something for the activity.
 * The length of the queue should be constrained by the number of
 * simultaneous in-progress client RPCs (i.e. it should be relatively
 * small), hence we use a simple queue scan.
 * 
 * Note that we're comparing the actid of the WAY call, NOT the client's
 * actid (in the body of the WAY).  For the purposes of detecting pings
 * and duplicate requests for server's WAY duplicate, this is sufficient.
 */

INTERNAL boolean32 convq_has_act(actuid)
dce_uuid_p_t actuid;
{
    rpc_dg_recvq_elt_p_t rqe1;
    unsigned32 st;

    RPC_MUTEX_LOCK(convq.m);
    for (rqe1 = convq.head; rqe1 != NULL; rqe1 = rqe1->next)
        if (UUID_EQ(*actuid, rqe1->hdrp->actuid, &st))
            break;
    RPC_MUTEX_UNLOCK(convq.m);
    return (rqe1 != NULL);
}

/*
 * C O N V Q _ L O O P
 *
 * The conversation manager call execution thread; process the conv requests.
 * An assumption is that this loop is the only consumer of the queue
 * (i.e. the only one to remove queue elements).  A queue element that
 * is being handled remains on the queue to allow convq_has_act() to detect
 * it.
 */

INTERNAL void convq_loop(void)
{
    rpc_dg_recvq_elt_p_t rqe;

    RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1, 
	("(convq_loop) starting up..\n"));

    while (1)
    {
        RPC_MUTEX_LOCK(convq.m);
        while ((convq.head == NULL) && (!convq_stop)) 
            RPC_COND_WAIT(convq.cv, convq.m);
	
	if (convq_stop) 
	    break;
	
        rqe = convq.head;
        RPC_MUTEX_UNLOCK(convq.m);
	
	if (RPC_DBG(rpc_es_dbg_conv_thread, 100))
	{
#ifdef DEBUG
	    struct timespec i;
	    i.tv_sec = rpc_g_dbg_switches[rpc_es_dbg_conv_thread] - 100;
	    i.tv_nsec = 0;
	    RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1,
		("(convq_loop) sleeping for %d sec\n", i.tv_sec));
	    dcethread_delay(&i);
#endif
	}
	
        RPC_LOCK(0);
        (void) handle_conv_int(rqe->sock, rqe);
        RPC_UNLOCK(0);
	RPC_MUTEX_LOCK(convq.m);
        convq_remove();
	RPC_MUTEX_UNLOCK(convq.m);
        rpc__dg_pkt_free_rqe(rqe, NULL);
    }
    RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1, 
	("(convq_loop) shutting down..\n"));

    RPC_MUTEX_UNLOCK(convq.m);
}

#ifdef ATFORK_SUPPORTED
/*
 * R P C _ _ D G _ C O N V _ F O R K _ H A N D L E R
 * 
 * Invoked by rpc__ncadg_fork_handler when handling a fork
 * 
 * We shut down the conv_handler thread before a fork, and restart it
 * afterward in the parent.
 */

PRIVATE void rpc__dg_conv_fork_handler 
(
        rpc_fork_stage_id_t stage
)
{
    error_status_t st;
    rpc_dg_recvq_elt_p_t rqe;
    
    switch((int)stage) 
    {
    case RPC_C_PREFORK:
        RPC_MUTEX_LOCK(convq.m);
        convq_was_running = false;

        if (convq_running) 
        {
            convq_stop = true;
            RPC_COND_SIGNAL (convq.cv, convq.m);
            RPC_MUTEX_UNLOCK(convq.m);
            dcethread_join_throw (conv_thread, (void **) &st);
            RPC_MUTEX_LOCK(convq.m); /* FIXME: wtf
				DCETHREAD_TRY	{
                                    dcethread_detach_throw(conv_thread);
				}
				DCETHREAD_CATCH(dcethread_use_error_e)	{
				}
				DCETHREAD_ENDTRY; */
            convq_running = false;
            convq_was_running = true;
        }
	break;
	
    case RPC_C_POSTFORK_PARENT:
        if (convq_was_running) 
        {
            convq_was_running = false;
            convq_stop = false;
            convq_running = true;
            dcethread_create_throw(&conv_thread, NULL,
                           (dcethread_startroutine)convq_loop, 
                           NULL);  
        }
        RPC_MUTEX_UNLOCK(convq.m);
	break;
	
    case RPC_C_POSTFORK_CHILD:
        convq_was_running = false;
        convq_running = false;
        convq_stop = false;
	
        /* 
         * drop all rqe's in the conv queue
         */
        while ((rqe = convq.head)) 
        {
            convq_remove();
            rpc__dg_pkt_free_rqe (rqe, NULL);
        }

        RPC_MUTEX_UNLOCK(convq.m);
        /*
         * Reset the convq state to 0's.
         */
        memset(&convq, 0, sizeof(convq));
	break;
    }
}
#endif /* ATFORK_SUPPORTED */

/*
 * R P C _ _ D G _ H A N D L E _ C O N V   /   H A N D L E _ C O N V _ I N T
 *
 * This routine is used by a client to respond to calls (callbacks) to
 * the "conv_who_are_you" (WAY), "conv_who_are_you2" (WAY2),
 * "conv_are_you_there" (AYT), "conv_who_are_you_auth" (WAYAUTH), and
 * "conv_who_are_you_auth_more" (WAYAUTHMORE)
 * conversation manager (conv) interfaces.  Rather than handling these
 * as a real callbacks, we just unmarshall, call the manager routines
 * from here, marshall the results into the request packet, and send
 * them back to the (original) server.
 *
 * We handle conv callbacks specially for two reasons.  First, for reasons
 * of protocol correctness, conv callbacks request packets (unlike request
 * backs for "normal" callbacks) do not use the activity ID of the original
 * client.  Torquing conv callbacks into the normal callback machinery
 * turns out to be harder than handling conv callbacks as a special case
 * (i.e., in this procedure).
 *
 * The second reason for the special conv callback treatment is that it
 * allows (will allow) us to split the runtime code into "server" and
 * "client" pieces in such a way that client-only applications don't
 * need to load all the code servers need.  Without the special treatment,
 * the processing of WAY callbacks by clients would need all the server
 * machinery.
 * 
 * This routine assumes the global lock is held (needed by the manager
 * routine which calls callt_lookup).
 *
 * The funny "...1" variable names are to work around a VAX C compiler bug.
 */

INTERNAL boolean32 handle_conv_int
(
    rpc_socket_t sock,
    rpc_dg_recvq_elt_p_t rqe1
)
{
    dce_uuid_t clt_actid1;
    unsigned32 clt_boot1;
    boolean b;
    rpc_dg_pkt_hdr_t resp_hdr;
    rpc_dg_pkt_hdr_p_t hdrp = rqe1->hdrp;
    rpc_dg_raw_pkt_p_t rawpkt = rqe1->pkt;
    rpc_socket_iovec_t iov[2];
    static conv_stub_t conv_stubs[5] =
    {
        conv_stub_who_are_you,
        conv_stub_who_are_you2,
        conv_stub_are_you_there,
        conv_stub_who_are_you_auth,
        conv_stub_who_are_you_auth_more
    };

      
    RPC_LOCK_ASSERT(0);

    /* 
     * Unmarshall (at least the start of) the request packet.  (All the
     * operations in the conv interface currently have the same first
     * three input parameters.) The activity id is the first argument
     * in the packet body, the boot time is the second:
     *
     *      |<----- 32 bits ----->|
     *      +---------------------+
     *   0: |                     |
     *      +                     +
     *   4: |     Activity        |
     *      +       UUID          +
     *   8: |                     |
     *      +                     +
     *  12: |                     |
     *      +---------------------+
     *  16: |     Boot Time       |
     *      +---------------------+
     *
     */

#define CONV_MIN_REQUEST_LEN 20

    assert(hdrp->if_vers == ((rpc_if_rep_p_t) conv_v3_0_c_ifspec)->vers);

    /*
     * There are four types of conv callbacks that must be handled
     * (WAY, WAY2, "are_you_there", WAYauth, and WAYauthMore  callbacks).  
     * The opnum field differentiates the operations.
     */
    if (hdrp->if_vers != ((rpc_if_rep_p_t) conv_v3_0_c_ifspec)->vers
/*        || hdrp->opnum < 0	 ??? */
        || hdrp->opnum > 5
        || hdrp->len < CONV_MIN_REQUEST_LEN)
    {
        RPC_DBG_GPRINTF((
            "(rpc__dg_handle_conv) Curious conv request; if_vers=%u, opnum=%u, len=%u\n",
            hdrp->if_vers, hdrp->opnum, hdrp->len));
        return (true);
    }

#ifndef MISPACKED_HDR
    clt_actid1 = *((dce_uuid_p_t) rawpkt->body.args);
    clt_boot1 = *((unsigned32 *) ((char *) rawpkt->body.args + 16));

    if (NDR_DREP_INT_REP(hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_UUID(clt_actid1);
        SWAB_INPLACE_32(clt_boot1);
    }
#else
#error "MISPACKED_HDR_CODE_NEEDED_HERE"
#endif /* MISPACKED_HDR */
                           
    /*
     * Dispatch to the "stub" for this request.
     */
    
    resp_hdr = *hdrp;
    (*conv_stubs[hdrp->opnum])(rqe1, &resp_hdr, &clt_actid1, clt_boot1);

    /*
     * Finish setting up the packet header.
     */

    RPC_DG_HDR_SET_PTYPE(&resp_hdr, RPC_C_DG_PT_RESPONSE);
    RPC_DG_HDR_SET_DREP(&resp_hdr);
    resp_hdr.flags = 0;

    /*
     * Finish setting up the the iov and send the packet.
     */

    iov[0].iov_base = (byte_p_t) &resp_hdr;
    iov[0].iov_len  = sizeof(resp_hdr);
    iov[1].iov_base = (byte_p_t) rawpkt->body.args;
    iov[1].iov_len  = resp_hdr.len;

    rpc__dg_xmit_pkt(sock, (rpc_addr_p_t) &rqe1->from, iov, 2, &b);

    return (true);
}

/*
 * R P C _ _ D G _ H A N D L E _ C O N V
 *
 * The conv thread handling version of rpc__dg_handle_conv()
 * (see general conv handling description above).
 *
 * Most of the WAY RPC flavors are local fast operations, however the
 * WAY_Auth call (at least for kernel RPC) can be quite slow.  WAY_Auth
 * involves a dispatch to the Auth code which involves constructing an
 * authenticator (encryption).  Furthermore, kernel RPC Auth performs
 * this operation by performing an upcall to a user space helper process.
 * In the absence of some form of WAY call executor thread, the listener
 * thread is blocked during this period (which is clearly undesireable
 * for the kernel).
 *
 * The WAY call thread mechanism is simple...
 * Essentially, queue WAY request pkts for call execution by the
 * conversation thread and do a little protocol maintenance for long
 * running calls and obvious duplicate detection.  The guts of the conv
 * handline remain in the non-thread version of this routine.
 */

PRIVATE boolean32 rpc__dg_handle_conv
(
    rpc_socket_t sock,
    rpc_dg_recvq_elt_p_t rqe1
)
{
    rpc_dg_pkt_hdr_p_t hdrp = rqe1->hdrp;
    rpc_dg_pkt_hdr_t resp_hdr;
    rpc_socket_iovec_t iov[1];
    boolean free_rqe = false;
    boolean b;
    boolean working;

    switch (RPC_DG_HDR_INQ_PTYPE(hdrp))
    {
        case RPC_C_DG_PT_REQUEST:
            /*
             * Handle non way_auth calls immediately unless we're debugging..
             */
            if (hdrp->opnum != 3 && !RPC_DBG(rpc_es_dbg_conv_thread, 50))
            {
                return (handle_conv_int(sock, rqe1));
            }

            /*
             * Otherwise, queue the request.
             */
            if (! convq_running)
                convq_start();

            if (convq_has_act(&rqe1->hdrp->actuid))
            {
                RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1,
		    ("(rpc__dg_handle_conv) duplicate [%s]\n",
                    rpc__dg_act_seq_string(hdrp)));
                free_rqe = true;
            }
            else
            {
                rqe1->sock = sock;
                if (convq_add(rqe1))
                {
                    RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 2,
                        ("(rpc__dg_handle_conv) queued opnum=%u [%s]\n",
                        hdrp->opnum, rpc__dg_act_seq_string(hdrp)));
                }
                else
                {
                    RPC_DBG_GPRINTF(("(rpc__dg_handle_conv) overflow [%s]\n", 
                        rpc__dg_act_seq_string(hdrp)));
                    free_rqe = true;
                }
            }
            break;
        case RPC_C_DG_PT_PING:
            /*
             * Always respond to non way_auth pings with a 'nocall' (unless
             * we're debugging).
             */
            if (hdrp->opnum != 3 && !RPC_DBG(rpc_es_dbg_conv_thread, 50))
            {
                working = false;
            }
            else
            {
                if (! convq_running)
                    convq_start();

                working = convq_has_act(&rqe1->hdrp->actuid);
            }

            resp_hdr = *hdrp;
            RPC_DG_HDR_SET_PTYPE(&resp_hdr, 
                    working ? RPC_C_DG_PT_WORKING : RPC_C_DG_PT_NOCALL);
            RPC_DG_HDR_SET_DREP(&resp_hdr);
            resp_hdr.flags = 0;
            
            iov[0].iov_base = (byte_p_t) &resp_hdr;
            iov[0].iov_len  = sizeof(resp_hdr);
            
            rpc__dg_xmit_pkt(sock, (rpc_addr_p_t) &rqe1->from, iov, 1, &b);

            RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 2, 
		("(rpc__dg_handle_conv) %s [%s]\n",
                working ? "working" : "nocall",
                rpc__dg_act_seq_string(hdrp)));
            free_rqe = true;
            break;
        default:
            RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1,
		("(rpc__dg_handle_conv) Dropped %s [%s]\n", 
                rpc__dg_pkt_name(RPC_DG_HDR_INQ_PTYPE(hdrp)), 
                rpc__dg_act_seq_string(hdrp)));
            free_rqe = true;
            break;
    }

    return (free_rqe);
}


/*
 * C O N V _ S T U B _ W H O _ A R E _ Y O U
 *
 */

INTERNAL void conv_stub_who_are_you
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_pkt_hdr_p_t resp_hdrp,
    dce_uuid_p_t clt_actid,
    unsigned32 clt_boot
)
{
    rpc_dg_raw_pkt_p_t rawpkt = rqe->pkt;
    unsigned32 way_status;
    unsigned32 way_seq;

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
        ("(conv_stub_who_are_you) Responding to WAY callback\n"));

    conv_who_are_you((handle_t) NULL, clt_actid, clt_boot, &way_seq, &way_status);

    /* 
     * Marshall the response packet.
     * 
     * For the WAY callback, the first argument is the sequence number,
     * and the second is the status.
     * 
     *      |<----- 32 bits ----->|
     *      +---------------------+
     *   0: |     Sequence #      |
     *      +---------------------+
     *   4: |       Status        |
     *      +---------------------+
     */

#define WAY_RESPONSE_LEN  8

#ifndef MISPACKED_HDR
    *((unsigned32 *) &rawpkt->body.args[0]) = way_seq;
    *((unsigned32 *) &rawpkt->body.args[4]) = way_status;
#else
#error   "MISPACKED_HDR CODE NEEDED HERE" /* !!! */
#endif

    resp_hdrp->len = WAY_RESPONSE_LEN;
}


/*
 * C O N V _ S T U B _ W H O _ A R E _ Y O U 2
 *
 */

INTERNAL void conv_stub_who_are_you2
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_pkt_hdr_p_t resp_hdrp,
    dce_uuid_p_t clt_actid,
    unsigned32 clt_boot
)
{
    rpc_dg_raw_pkt_p_t rawpkt = rqe->pkt;
    unsigned32 way_status;
    unsigned32 way_seq;
    dce_uuid_t way2_cas_uuid;

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
        ("(conv_stub_who_are_you2) Responding to WAY2 callback\n"));

    conv_who_are_you2((handle_t) NULL, clt_actid, clt_boot, &way_seq, 
                                &way2_cas_uuid, &way_status);

    /*
     * Marshall the response packet.  
     * 
     * For the WAY2 callback, the first argument is the sequence number,
     * the second is the client address space UUID (cas_uuid), and the
     * third is the status.
     *
     *      |<----- 32 bits ----->|
     *      +---------------------+
     *   0: |     Sequence #      |
     *      +---------------------+
     *   4: |      CAS uuid       |  time_low
     *      +----------+----------+
     *   8: | CAS uuid |             time_mid
     *      +----------+
     *  10: | CAS uuid |             time_hi_and_version
     *      +----+-----+
     *  12: |    |                   clock_seq_hi_and_reserved
     *      +----+
     *  13: |    |                   clock_seq_low
     *      +----+-----+
     *  14: | CAS uuid |             node[0..1]
     *      +----------+----------+
     *  16: |      CAS uuid       |  node[2..5]
     *      +----------+----------+
     *  20: |       Status        |
     *      +---------------------+
     */

#define WAY2_RESPONSE_LEN 24

#ifndef MISPACKED_HDR
    *((unsigned32 *) &rawpkt->body.args[0]) = way_seq;
    marshall_uuid(&rawpkt->body.args[4], &way2_cas_uuid);
    *((unsigned32 *) &rawpkt->body.args[20]) = way_status;
#else
#error    "MISPACKED_HDR CODE NEEDED HERE" /* !!! */
#endif

    resp_hdrp->len = WAY2_RESPONSE_LEN;
}


/*
 * C O N V _ S T U B _ A R E _ Y O U _ T H E R E
 *
 */

INTERNAL void conv_stub_are_you_there
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_pkt_hdr_p_t resp_hdrp,
    dce_uuid_p_t clt_actid,
    unsigned32 clt_boot
)
{
    rpc_dg_raw_pkt_p_t rawpkt = rqe->pkt;
    unsigned32 way_status;

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
        ("(conv_stub_are_you_there) Responding to AYT callback\n"));

    conv_are_you_there((handle_t) NULL, clt_actid, clt_boot, &way_status);

    /*
     * Marshall the response packet.  
     * 
     * For the "are you there" callback (AYT) the only argument is the 
     * status.
     * 
     *      |<----- 32 bits ----->|
     *      +---------------------+
     *   0: |       Status        |
     *      +---------------------+
     */

#define AYT_RESPONSE_LEN  4

#ifndef MISPACKED_HDR
    *((unsigned32 *) &rawpkt->body.args[0]) = way_status;
#else
#error    "MISPACKED_HDR_CODE NEEDED HERE" /* !!! */
#endif

    resp_hdrp->len = AYT_RESPONSE_LEN;
}


/*
 * C O N V _ S T U B _ W H O _ A R E _ Y O U _ A U T H
 *
 */

INTERNAL void conv_stub_who_are_you_auth
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_pkt_hdr_p_t resp_hdrp,
    dce_uuid_p_t clt_actid,
    unsigned32 clt_boot
)
{
    rpc_dg_pkt_hdr_p_t rqst_hdrp = rqe->hdrp;
    rpc_dg_raw_pkt_p_t rawpkt = rqe->pkt;
    ndr_byte *in_data, *out_data;
    signed32 in_len, out_len, out_max_len;
    signed32 out_round_len;
    unsigned32 way_status;
    unsigned32 way_seq;
    dce_uuid_t way2_cas_uuid;
    
    /*
     * Unmarshall the remaining arguments:
     * 
     *       |<----- 32 bits ----->|
     *       +---------------------+
     *  20:  |     Input Length    |
     *       +----+----------------+
     *  24:  |    |          input bytes..
     *       +----+----------------+
     *  24+n:|    Input Length     |
     *       +---------------------+
     *  28+n:|    Output Max Len   |
     *       +---------------------+
     */

    in_len = *((signed32 *) &rawpkt->body.args[20]);
    if (NDR_DREP_INT_REP(rqst_hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_32(in_len);
    }

    if ((in_len + 32 != (rqst_hdrp->len)) || (in_len < 0)) 
    {
        RPC_DBG_GPRINTF((
            "(conv_stub_who_are_you_auth) Bad size of input args in WAY_AUTH request; in_len = %d, rqst_hdrp->len=%d\n",
            in_len, rqst_hdrp->len));
        return;
    }

    in_data = (ndr_byte *)(rawpkt->body.args+24);
    out_max_len = *((signed32 *) &rawpkt->body.args[28 + in_len]);
    
    if (NDR_DREP_INT_REP(rqst_hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_32(out_max_len);
    }

	 if (out_max_len > 0)	{
		 if ((unsigned long)out_max_len > (RPC_C_DG_MAX_PKT_SIZE -
					 (RPC_C_DG_RAW_PKT_HDR_SIZE + 40))) 
		 {
			 out_max_len = (RPC_C_DG_MAX_PKT_SIZE -
					 (RPC_C_DG_RAW_PKT_HDR_SIZE + 40));
			 RPC_DBG_GPRINTF((
						 "(conv_stub_who_are_you_auth) Output size truncated to %d\n",
						 out_max_len));
		 }
	 }

    out_data = (ndr_byte *)(rawpkt->body.args+32);
    out_len = 0;
    
    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
        ("(conv_stub_who_are_you_auth) Thinking about responding to WAYAUTH callback\n"));

    conv_who_are_you_auth ((handle_t) NULL, clt_actid, clt_boot,
                           in_data, in_len, out_max_len,
                           &way_seq, &way2_cas_uuid,
                           out_data, &out_len, &way_status);

    /* round up to next multiple of 4 bytes */
    out_round_len = (out_len+3)&~3;

    if (out_round_len > out_max_len) 
    {
        RPC_DBG_GPRINTF(
            ("(conv_stub_who_are_you_auth) Output too long -- %d truncated to %d\n",
            out_len, out_max_len));
        out_len = out_max_len;
    }
        
    /*
     * Marshall the output arguments:
     * 
     * For the WAY_AUTH callback, the first argument is the sequence
     * number, the second is the status, and the third is a counted
     * string of bytes.
     * 
     *       |<----- 32 bits ----->|
     *       +---------------------+
     *   0:  |     Sequence #      |
     *       +---------------------+
     *   4:  |      CAS uuid       |  time_low
     *       +----------+----------+
     *   8:  | CAS uuid |             time_mid
     *       +----------+
     *  10:  | CAS uuid |             time_hi_and_version
     *       +----+-----+
     *  12:  |    |                   clock_seq_hi_and_reserved
     *       +----+
     *  13:  |    |                   clock_seq_low
     *       +----+-----+
     *  14:  | CAS uuid |             node[0..1]
     *       +----------+----------+
     *  16:  |      CAS uuid       |  node[2..5]
     *       +----------+----------+
     *  20:  |  Output Max Length  |
     *       +---------------------+
     *  24:  |  Lower Bound (0)    |
     *       +---------------------+
     *  28:  |  Upper Bound        |
     *       +----+----------------+
     *  32:  |    |          output bytes..
     *       +----+----------------+
     *  32+n:|    Output Length    |
     *       +---------------------+
     *  36+n:|       Status        |
     *       +---------------------+
     */

#ifndef MISPACKED_HDR
    *((unsigned32 *) rawpkt->body.args) = way_seq;
    marshall_uuid(&rawpkt->body.args[4], &way2_cas_uuid);
    *((unsigned32 *) &rawpkt->body.args[20]) = out_max_len;
    *((unsigned32 *) &rawpkt->body.args[24]) = 0;
    *((unsigned32 *) &rawpkt->body.args[28]) = out_len;  
    /* rawpkt->body.args[32 .. 32+(outlen-1)] filled in by stub */
    *((unsigned32 *) &rawpkt->body.args[32 + out_round_len]) = out_len;
    *((unsigned32 *) &rawpkt->body.args[36 + out_round_len]) = way_status;

    resp_hdrp->len = 40 + out_round_len;
#else
#error    "MISPACKED_HDR_CODE NEEDED HERE" /* !!! */
#endif
}

/*
 * C O N V _ S T U B _ W H O _ A R E _ Y O U _ A U T H _ M O R E
 *
 */

INTERNAL void conv_stub_who_are_you_auth_more(rqe, resp_hdrp, clt_actid, 
                                              clt_boot)
rpc_dg_recvq_elt_p_t rqe;
rpc_dg_pkt_hdr_p_t resp_hdrp;
dce_uuid_p_t clt_actid;
unsigned32 clt_boot;
{
    rpc_dg_pkt_hdr_p_t rqst_hdrp = rqe->hdrp;
    rpc_dg_raw_pkt_p_t rawpkt = rqe->pkt;
    ndr_byte *out_data;
    signed32 index, out_len, out_max_len, out_round_len;
    unsigned32 way_status;

    /*
     * Unmarshall the remaining arguments:
     *
     *       |<----- 32 bits ----->|
     *       +---------------------+
     *  20:  |        Index        |
     *       +---------------------+
     *  24:  |    Output Max Len   |
     *       +---------------------+
     */

    index = *((signed32 *) &rawpkt->body.args[20]);
    if (NDR_DREP_INT_REP(rqst_hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_32(index);
    }

    out_max_len = *((signed32 *) &rawpkt->body.args[24]);
    if (NDR_DREP_INT_REP(rqst_hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_32(out_max_len);
    }


    out_data = (ndr_byte *)(rawpkt->body.args+12);
    out_len = 0;

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3,
        ("(conv_stub_who_are_you_auth_more) retrieving PAC[%d-%d]\n",
            index, index + out_max_len - 1));

    conv_who_are_you_auth_more ((handle_t) NULL, clt_actid, clt_boot, index,
                                out_max_len, out_data, &out_len, &way_status);

    /* round up to next multiple of 4 bytes */
    out_round_len = (out_len+3)&~3;

    if (out_round_len > out_max_len)
    {
        RPC_DBG_GPRINTF(
            ("(conv_stub_who_are_you_auth_more) Output truncated from %d to %d\n",
            out_len, out_max_len));
        out_len = out_max_len;
    }

    /*
     * Marshall the output arguments:
     *
     * For the WAY_AUTH callback, the first argument is the sequence
     * number, the second is the status, and the third is a counted
     * string of bytes.
     *
     *       |<----- 32 bits ----->|
     *       +---------------------+
     *   0:  |  Output Max Length  |
     *       +---------------------+
     *   4:  |  Lower Bound (0)    |
     *       +---------------------+
     *   8:  |  Upper Bound        |
     *       +----+----------------+
     *  12:  |    |          output bytes..
     *       +----+----------------+
     *  12+n:|    Output Length    |
     *       +---------------------+
     *  16+n:|       Status        |
     *       +---------------------+
     */

#ifndef MISPACKED_HDR
    *((unsigned32 *) rawpkt->body.args) = out_max_len;
    *((unsigned32 *) &rawpkt->body.args[4]) = 0;
    *((unsigned32 *) &rawpkt->body.args[8]) = out_len;
    /* rawpkt->body.args[12 .. 12+(outlen-1)] filled in by stub */
    *((unsigned32 *) &rawpkt->body.args[12 + out_round_len]) = out_len;
    *((unsigned32 *) &rawpkt->body.args[16 + out_round_len]) = way_status;

    resp_hdrp->len = 20 + out_round_len;
#else
    MISPACKED_HDR_CODE_NEEDED_HERE /* !!! */
#endif
}

/*
 * R P C _ _ D G _ H A N D L E _ C O N V C
 *
 * This routine is used by a server to handle calls to the convc (INDY)
 * interface.  Rather than handling this call as a real call, we just
 * unmarshall, and call the manager routine from here.
 * 
 * We don't treat these calls as real callbacks because we don't want
 * them to get caught up in the packet rationing code.  That is, if the
 * calls were tagged with the 'maybe' attribute and we were low on packets,
 * we would drop the request packet but still start the call.  Since
 * 'maybe' calls don't retransmit, the server side would eventually timeout
 * the call, and the context couldn't be maintained.
 *
 * On the other hand, we could tag the call with the 'idempotent'
 * attribute, but this would mean that if the 'context maintainer' thread
 * tried to do an INDY call to a dead server, it would have to wait for
 * a timeout before going on with the rest of the INDY's for other servers.
 * This delay may cause those other servers to decide that the context
 * had timed out.
 */

PRIVATE void rpc__dg_handle_convc
(
    rpc_dg_recvq_elt_p_t rqe
)
{
    dce_uuid_t cas_uuid;
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    rpc_dg_raw_pkt_p_t rawpkt = rqe->pkt;

      
    RPC_LOCK_ASSERT(0);

    /* 
     * Unmarshall the client address space UUID from the request packet. 
     *
     *      |<----- 32 bits ----->|
     *      +---------------------+
     *   0: |                     |
     *      +                     +
     *   4: |       CAS           |
     *      +       UUID          +
     *   8: |                     |
     *      +                     +
     *  12: |                     |
     *      +---------------------+
     *
     */

#define CONVC_MIN_REQUEST_LEN 16

    assert(hdrp->if_vers == ((rpc_if_rep_p_t) convc_v1_0_c_ifspec)->vers);

    /*
     * Make sure the request looks sane.
     */
    if (hdrp->if_vers != ((rpc_if_rep_p_t) convc_v1_0_c_ifspec)->vers
        || hdrp->opnum != 0
        || hdrp->len < CONVC_MIN_REQUEST_LEN)
    {
        RPC_DBG_GPRINTF((
            "(rpc__dg_handle_convc) Curious convc request; if_vers=%u, opnum=%u, len=%u\n",
            hdrp->if_vers, hdrp->opnum, hdrp->len));
        return;
    }

#ifndef MISPACKED_HDR
    cas_uuid = *((dce_uuid_p_t) rawpkt->body.args);

    if (NDR_DREP_INT_REP(hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_UUID(cas_uuid);
    }
#else
#error    "MISPACKED_HDR_CODE NEEDED HERE" /* !!! */
#endif
                           
    /*
     * Dispatch to the "stub" for this request.  Since this is a "maybe"
     * call, no need to worry about a response.
     */
    
    rpc__dg_convc_indy(&cas_uuid);
}



/*
 * R E C V _ P K T
 *
 * Read a socket for a packet.
 *
 * No lock requirements.
 */

INTERNAL unsigned32 recv_pkt
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_socket_error_t serr;
    int recv_len;
    boolean fwd;
    rpc_dg_ptype_t ptype;
    rpc_socket_iovec_t iov[RPC_C_DG_MAX_NUM_PKTS_IN_FRAG];
    rpc_dg_recvq_elt_p_t tmp;
    unsigned32 cc, num_rqes, i;

    rqe->from.rpc_protseq_id = sp->pseq_id;
    rqe->from.len = sizeof rqe->from.sa;

    iov[0].iov_base = (byte_p_t) rqe->pkt;
    iov[0].iov_len  = sizeof(*rqe->pkt);

    for (i = 1, tmp = rqe->more_data; tmp != NULL; i++, tmp = tmp->more_data)
    {
        iov[i].iov_base = (byte_p_t) &tmp->pkt->body;
        iov[i].iov_len  = sizeof(tmp->pkt->body);
    }
    assert(i <= RPC_C_DG_MAX_NUM_PKTS_IN_FRAG);

    serr = rpc__socket_recvmsg(
        sp->sock,
        iov,
        i,
        (rpc_addr_p_t)(&rqe->from), 
        &recv_len);

    RPC_DG_SOCK_UPDATE_ERR_COUNT(sp, serr);

    
    if (RPC_SOCKET_IS_ERR(serr))
    {
        if (RPC_SOCKET_ERR_EQ(serr, RPC_C_SOCKET_EWOULDBLOCK))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 5, 
                ("(recv_pkt) recvfrom (sock %d) returned EWOULDBLOCK\n", sp->sock));
        }
        else
        {
            RPC_DBG_GPRINTF(("(recv_pkt) recvfrom (sock %d) recv_len = %d, error = %d\n", 
                sp->sock, recv_len, RPC_SOCKET_ETOI(serr)));
        }
        return (0);
    }

    rqe->frag_len = recv_len;
    for (cc = recv_len, tmp = rqe, num_rqes = 0;
         cc != 0 && tmp != NULL;
         tmp = tmp->more_data, num_rqes++)
    { 
        if (cc <= iov[num_rqes].iov_len)
        { 
            tmp->pkt_len = cc;
            cc = 0;
        }
        else
        {
            tmp->pkt_len = iov[num_rqes].iov_len;
            cc -= iov[num_rqes].iov_len;
        }
    }                        
    if (cc != 0)                                       
    {
        RPC_DBG_GPRINTF(("(recv_pkt) suspicious recvmsg recv_len = %d, error = %d\n",
            recv_len, RPC_SOCKET_ETOI(serr)));
        rqe->frag_len = 0;
        return (0);
    }

    /*
     * Do a little sanity check on the "from" info;  discard clearly bogus stuff.
     */

    if (rqe->from.len <= 0 || rqe->from.len > sizeof rqe->from.sa)
    {
        RPC_DBG_GPRINTF(("(recv_pkt) Bad 'from' info\n"));
        rqe->frag_len = 0;
        return (0);
    }

    /*
     * Check the first byte of the header, which is the RPC protocol version.
     */

    if ((rqe->pkt->hdr.hdr[0] & RPC_C_DG_VERS_MASK) != RPC_C_DG_PROTO_VERS)
    {
        RPC_DBG_GPRINTF(("(recv_pkt) Bad RPC version (%u)\n", rqe->pkt->hdr.hdr[0]));
        rqe->frag_len = 0;
        return (0);
    }

#ifndef MISPACKED_HDR

    /*
     * If the integer drep used to construct the packet is not the same
     * as the local integer drep, we must swap the bytes in the integer
     * fields in the header.  (Note that "swab_hdr" updates "rqe->hdrp".
     * See comments in "swab_hdr".)
     *
     * If the integer dreps ARE the same, then if the packet's server
     * boot time field is zero (unspecified), we may want to overwrite
     * it later (in case this is a client->server packet that will be
     * modified to be used as the server->client reply).  Since modifying
     * the packet is disallowed (it interfers with authentication
     * checksumming), copy the header out if the boot time is zero.
     */

    rqe->hdrp = (rpc_dg_pkt_hdr_p_t) &rqe->pkt->hdr;

    if (NDR_DREP_INT_REP(rqe->hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        swab_hdr(rqe);
    }
    else if (rqe->hdrp->server_boot == 0)
    {
        rqe->hdr = *rqe->hdrp;
        rqe->hdrp = &rqe->hdr;
    }

#else

    /*
     * Unpack and convert data representation on header for "mispacked header"
     * systems.
     */

    unpack_hdr(rqe);

#endif

    ptype = RPC_DG_HDR_INQ_PTYPE(rqe->hdrp);

    if (ptype > RPC_C_DG_PT_MAX_TYPE)
    {
        RPC_DBG_GPRINTF(("(recv_pkt) Bad pkt type (%d)\n", ptype));
        rqe->frag_len = 0;
        rqe->hdrp = NULL;
        return (0);
    }

    RPC_DG_STATS_INCR(pkts_rcvd);
    RPC_DG_STATS_INCR(pstats[ptype].rcvd);

    /* 
     * If this is a forwarded packet, take care of it.  
     */

    fwd = RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_FORWARDED);

    if (fwd)
    {
        /*
         * Clear forwarding bits in packets that shouldn't have them.  Pre-2.0
         * systems accidentally set them as a result of initializing its
         * header from the header of some packet it received.  It confuses
         * us to see them set on "wrong" packets.  (Pre-2.0 systems didn't
         * do authentication, so we don't have to worry about "corrupting"
         * the packet by doing this.)
         */
    
        if (RPC_DG_PT_IS(ptype, 
                         (1 << RPC_C_DG_PT_RESPONSE) | 
                         (1 << RPC_C_DG_PT_FAULT)    | 
                         (1 << RPC_C_DG_PT_WORKING)  | 
                         (1 << RPC_C_DG_PT_NOCALL)   | 
                         (1 << RPC_C_DG_PT_REJECT)   | 
                         (1 << RPC_C_DG_PT_QUACK)    |
                         (1 << RPC_C_DG_PT_FACK)))
        {
            rqe->hdrp->flags  &= ~RPC_C_DG_PF_FORWARDED;
            rqe->hdrp->flags2 &= ~RPC_C_DG_PF2_FORWARDED_2;
            fwd = false;
        }

        else if (RPC_DG_HDR_FLAG2_IS_SET(rqe->hdrp, RPC_C_DG_PF2_FORWARDED_2))
        {
            /*
             * We DON'T handle two-packet forwards here -- we let the
             * request packet processing handle them.  (All two-packet
             * forwards are requests.)
             */
        }

        else 
        {
            rpc_dg_fpkt_p_t fpkt = (rpc_dg_fpkt_p_t) rqe->pkt;
            struct sockaddr *sp = (struct sockaddr *) &rqe->from.sa;
            unsigned16 i, j;
            unsigned16 fwd_len;
        
            /*
             * Jam the source address and drep from the top of the body into
             * the right places, then slide the real body up.  The proceed
             * as if none of this nightmare had happened.
             */

            rqe->from.len = fpkt->fhdr.len;
            *sp = fpkt->fhdr.addr;
            rqe->hdrp->drep[0] = fpkt->fhdr.drep[0];
            rqe->hdrp->drep[1] = fpkt->fhdr.drep[1];
            rqe->hdrp->drep[2] = fpkt->fhdr.drep[2];
            fwd_len = rqe->pkt_len - 
                (RPC_C_DG_RAW_PKT_HDR_SIZE + sizeof(rpc_dg_fpkt_hdr_t));
            rqe->frag_len -= sizeof(rpc_dg_fpkt_hdr_t);
            rqe->pkt_len -= sizeof(rpc_dg_fpkt_hdr_t);

            for (i = 0, j = fwd_len; j > 0; i++, j--)
            {
                rqe->pkt->body.args[i] = fpkt->body.args[i];
            }

            /*
             * For the benefit of the authentication module, clear the
             * forwarded bit in the raw header; the original incarnation
             * of this packet had it cleared.  Note that we don't have to
             * do any of this for 2-part forwarding since the 2nd packet
             * comes in "clean" (no forwarding bits set).
             */

            rqe->pkt->hdr.hdr[RPC_C_DG_RPHO_FLAGS] &= ~RPC_C_DG_PF_FORWARDED;
        }
    }

    if (! fwd && (unsigned32)recv_len < (rqe->hdrp->len + RPC_C_DG_RAW_PKT_HDR_SIZE))
    {
        RPC_DBG_GPRINTF(("(recv_pkt) Inconsistent lengths:  recvfrom len = %u, len from hdr (+ hdr size) = %u\n",
                recv_len, rqe->hdrp->len + RPC_C_DG_RAW_PKT_HDR_SIZE));
        rqe->frag_len = 0;
        rqe->hdrp = NULL;
        return (0);
    }

    return (num_rqes);
}

/*
 * R E C V _ P K T _ P R I V A T E
 *
 * Read in data from a private socket.  This routine is a jacket for the
 * recv_pkt routine above.  It serves two purposes.  First, it unlocks the
 * the call handle, so that the timer thread can inspect it while it is
 * blocked in recvfrom.  Second, we protect against cancels by surrounding
 * the recv_pkt call with a TRY/CATCH.
 *
 * The call handle is presumed to be locked.
 */
INTERNAL unsigned32 recv_pkt_private(sp, rqe)
rpc_dg_sock_pool_elt_p_t sp;
rpc_dg_recvq_elt_p_t rqe;
{
    volatile unsigned32 recv_flag = 0;
    rpc_dg_ccall_p_t ccall = sp->ccall;
    rpc_dg_call_p_t volatile call;
    int prev_cancel_state;

    //DO_NOT_CLOBBER(recv_flag);
    //DO_NOT_CLOBBER(call);
	 
    /*
     * Check if we are executing the callback.
     */
    if (ccall->cbk_scall != NULL
         && ccall->cbk_scall->has_call_executor_ref == true)
        call = (rpc_dg_call_p_t) ccall->cbk_scall;
    else
        call = (rpc_dg_call_p_t) ccall;

    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * We need to be able to wake this thread up in the event of a
     * timeout.  Since the thread will be blocked on I/O, the only
     * way to do this is to post a cancel against the thread.
     *
     * If the application has disabled general cancelability, we
     * need to turn it on here, temporarily.
     */
    prev_cancel_state = dcethread_enableinterrupt_throw(0);

    DCETHREAD_TRY
    {
        call->blocked_in_receive = 1;
        RPC_DG_CALL_UNLOCK(call);

        RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 5, (
                "(recv_pkt_private) blocking in recv_pkt\n"));

#ifdef NON_CANCELLABLE_IO
        dcethread_enableasync_throw(1);
        dcethread_checkinterrupt();
#endif /* NON_CANCELLABLE_IO */

        recv_flag = recv_pkt(sp, rqe);

#ifdef NON_CANCELLABLE_IO
        dcethread_enableasync_throw(0);
#endif /* NON_CANCELLABLE_IO */

        RPC_DG_CALL_LOCK(call);
        call->blocked_in_receive = 0;
    
        /*
         * There's a window, between when we returned from recvfrom and 
         * when we got the call handle relocked.   If a cancel was posted, 
         * by the runtime trying to simulate a cond_signal, we need to eat 
         * it here. 
         */
        if (call->priv_cond_signal == true)
        {
            RPC_DG_CALL_UNLOCK(call);
            dcethread_checkinterrupt();
        }
    }
    DCETHREAD_CATCH(dcethread_interrupt_e)
    {
#ifdef NON_CANCELLABLE_IO
        dcethread_enableasync_throw(0);
#endif

        RPC_DG_CALL_LOCK(call);

        call->blocked_in_receive = 0;

        RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 3, (
                "(recv_pkt_private) caught a thread cancel posted by %s\n",
                 call->priv_cond_signal?"runtime":"user"));

        /*
         * If this cancel wasn't thrown by the runtime itself, it must
         * have been a real cancel, thrown by the application.  There are
         * two possibilities here:
         *
         *    1. cancelability was initially enabled, which means that this 
         *       cancel needs to be sent on to the server, or
         *    2. the application had disabled cancelability for this thread,
         *       which means that this cancel needs to still be pending when
         *       this call finishes.
         */
        if (! call->priv_cond_signal)
        {
            if (prev_cancel_state == 1)
                rpc__dg_call_local_cancel(call);
            else
            {
                /*
                 * Record the cancel in the call handle, but modify the 
                 * cancel state so that the cancel does not get forwarded
                 * to the server.
                 */ 
                ccall->cancel.local_count++;
                ccall->cancel.server_is_accepting = false;
            }
        }
        /*
         * Reset the flag.
         */
        call->priv_cond_signal = false;
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
#ifdef NON_CANCELLABLE_IO
        dcethread_enableasync_throw(0);
#endif

        RPC_DG_CALL_LOCK(call);

        call->blocked_in_receive = 0;

        RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 3, (
                "(recv_pkt_private) caught an unknown exception\n"));

        rpc__dg_call_signal_failure(call, (unsigned32) -1);
    } DCETHREAD_ENDTRY

    /*
     * Restore the original cancelability state.
     */
    dcethread_enableinterrupt_throw(prev_cancel_state);
    return (recv_flag);
}

/*
 * D O _ S E L E C T I V E _ A C K 
 *
 * Process the selective ACK portion of a fack body.
 *
 */

INTERNAL void do_selective_ack
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_call_p_t call,
    unsigned32 *window_incr,
    unsigned32 *rexmit_cnt, 
    unsigned32 serial_cnt
)
{
    rpc_dg_fackpkt_body_p_t bodyp = (rpc_dg_fackpkt_body_p_t) &rqe->pkt->body;
    rpc_dg_xmitq_elt_p_t xq_curr, xq_prev, rexmitq_tail = NULL;
    rpc_dg_xmitq_p_t xq = &call->xq;
    unsigned32 mask, selack_fragnum, i, j, *selack;
    unsigned16 selack_len, curr_serial = (unsigned16) serial_cnt;

    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * If by some means (e.g. lossy/reordering/dup) there are no unacknowledged
     * pkts, we're done.
     */
    if (xq->head == NULL)
    {
        return;
    }

    selack_len  = bodyp->selack_len;
    selack = &bodyp->selack[0];      
 
    if (NDR_DREP_INT_REP(rqe->hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_16(selack_len);
    }                                    

    /*
     * The caller of this routine already freed all xqes up to the fragnum
     * mentioned in the fack.  Of course, due to lossy/dup/reordered
     * environments, these mentioned fragments may have already been
     * freed.  The first bit in the selack mask corresponds to the fack's
     * fragnum + 1. The significant relationship is that the first selack
     * mask fragnum is *less than or equal to* the xq head's fragnum.
     *
     * In general, it's best to not make any assumptions regarding whether
     * or not the xq head is specified as facked in the selack mask.
     * Of course, the very nature of selacks means that there will be
     * "holes" in the xq (unacknowledged frag) list.  So, in general,
     * we need to skip selack bits for frags that have already been
     * facked (i.e. aren't on the xq).
     */

    /*
     * The first unacked frag of interest is at the head of the list.
     */

    xq_prev = NULL;
    xq_curr = xq->head;

    /*
     * Loop through the array of bit masks.
     *
     * selack_fragnum holds the fragment number corresponding to the
     * current selack mask bit under examination. The first selack mask
     * bit corresponds to the fragment immediately after the one explicitly
     * mentioned in the fack.
     */

    selack_fragnum = rqe->hdrp->fragnum + 1;
    for (i = 0; i < selack_len; i++)
    {      
        if (NDR_DREP_INT_REP(rqe->hdrp->drep) != ndr_g_local_drep.int_rep)
        {
            SWAB_INPLACE_32(selack[i]);
        }                                    

        for (mask = selack[i], j = 0; j < 32; mask >>= 1, j++, selack_fragnum++)
        {     
            /*
             * If the last mask element has no more bits set, leave the loop
             * early.  Also, if we have gotten to the end of the xmitq
             * early (which might happen after a duplicate selective ack)
             * we're done.
             */

            if ((mask == 0 && i == selack_len - 1UL) || xq_curr == NULL)
                break;

            /*
             * If the selack fragnum hasn't yet caught up to the
             * current xq fragment, just advance to the next selack bit/frag.
             */
            if (selack_fragnum != xq_curr->fragnum)
                continue;

            /*
             * Determine the disposition of (and process) the current
             * fragment.  Advance to the next xq fragment.
             */
            if (FRAG_GOT_ACKED(mask))
            {                       
                rpc_dg_xmitq_elt_p_t xq_next;

                /* 
                 * Update blast and window info and
                 * remove the facked pkt from the queue.
                 */

                TRACK_CURRENT_BLAST(xq, xq_curr, curr_serial, window_incr, false);

                /*
                 * If appropriate, decrement the counts of outstanding
                 * packets and outstanding fack requests.
                 */
                if (xq_curr->in_cwindow)
                {
                    xq->cwindow_size--;
                    if (! RPC_DG_FLAG_IS_SET(xq_curr->flags, RPC_C_DG_PF_NO_FACK) ||
                        RPC_DG_FLAG_IS_SET(xq_curr->flags, RPC_C_DG_PF_LAST_FRAG))
                        xq->freqs_out--;
                }

                xq_next = xq_curr->next;
                if (xq_curr == xq->head)
                {
                    xq->head = xq_next;
                }
                else
                {
                    xq_prev->next = xq_next;
                }
                if (xq_curr == xq->tail)
                {
                    xq->tail = xq_prev;
                }
                rpc__dg_pkt_free_xqe(xq_curr, call);
                xq_curr = xq_next; 
            }
            else
            {
                /*
                 * The fragment is still unacknowledged; don't remove it.
                 * This packet is lost and requires retransmission if
                 * its serial number is less than the one that induced
                 * this ack.  If it is, bump the rexmit_cnt, and add
                 * it to the xmitq's rexmitq.
                 */
                                                 
                if (RPC_DG_SERIAL_IS_LT(xq_curr->serial_num, xq->last_fack_serial))
                {  
                    TRACK_CURRENT_BLAST(xq, xq_curr, curr_serial, window_incr, true);
                    
                    /*
                     * If appropriate, decrement the counts of outstanding
                     * packets and outstanding fack requests.
                     */
            
                    if (xq_curr->in_cwindow)
                    {
                        xq_curr->in_cwindow = false;
                        xq->cwindow_size--;
                        if (! RPC_DG_FLAG_IS_SET(xq_curr->flags, RPC_C_DG_PF_NO_FACK))
                        {
                            xq->freqs_out--;
                            if (RPC_DG_FRAGNUM_IS_LT(xq_curr->fragnum, rqe->hdrp->fragnum))
                                (*window_incr) = 0;
                        }
                    }

                    (*rexmit_cnt)++;        
                    if (xq->rexmitq == NULL)
                        xq->rexmitq = xq_curr;
                    else
                        rexmitq_tail->next_rexmit = xq_curr;
    
                    xq_curr->next_rexmit = NULL;
                    rexmitq_tail = xq_curr;        
                }
                xq_prev = xq_curr;
                xq_curr = xq_curr->next;
            }
        }   

        /*
         * If we've already exhausted the xq elements, stop scanning.
         */
        if (xq_curr == NULL)
            break;
    } 

    /*
     * Search for additional transmited unacked xq frags that should
     * be retransmitted due to serial number information.  At this point,
     * we know that the remaining elements on the queue have serial numbers
     * *greater* the fragment that induced this fack, hence such pkts
     * aren't part of the blast associated with this fack and hence
     * blast / window size tracking are inappropriate.
     *
     * Note that when recovering from lost packets we can't make any
     * assumptions on the order of serial numbers in the queue.  This
     * means that without scanning the entire queue we can't know how
     * many packets need to be retransmitted.  
     *
     * This technique is not very scalable, and when we allow  for larger
     * window sizes we may want to handle the scanning with two
     * doubly-linked lists, one ordered by fragnum, and one ordered by
     * serial number. For today, we can get by with a less heavy handed
     * approach.
     */

    for ( ; xq_curr != xq->first_unsent; xq_curr = xq_curr->next)
    {       
        if (! RPC_DG_SERIAL_IS_LTE(xq_curr->serial_num, xq->last_fack_serial))
            continue;

        /*
         * If appropriate, decrement the counts of outstanding
         * packets and outstanding fack requests.
         */

        if (xq_curr->in_cwindow)
        {
            xq_curr->in_cwindow = false;
            xq->cwindow_size--;

            if (! RPC_DG_FLAG_IS_SET(xq_curr->flags, RPC_C_DG_PF_NO_FACK) ||
                RPC_DG_FLAG_IS_SET(xq_curr->flags, RPC_C_DG_PF_LAST_FRAG))
                xq->freqs_out--;
        }

        (*rexmit_cnt)++;        
        if (xq->rexmitq == NULL)
            xq->rexmitq = xq_curr;
        else
            rexmitq_tail->next_rexmit = xq_curr;

        xq_curr->next_rexmit = NULL;
        rexmitq_tail = xq_curr;        
    }                 
}

/*
 * D O _ F A C K _ B O D Y
 *
 * Process the contents of a fack body.
 */

INTERNAL void do_fack_body
(
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_call_p_t call,
    unsigned32 *window_incr, 
    unsigned32 *rexmit_cnt, 
    unsigned32 curr_serial
)
{
#ifndef MISPACKED_HDR

    unsigned32 max_tsdu, max_frag_size, our_min;
    unsigned32 snd_frag_size;

    rpc_dg_fackpkt_body_p_t bodyp = (rpc_dg_fackpkt_body_p_t) &rqe->pkt->body;

    RPC_DG_CALL_LOCK_ASSERT(call);

    /*
     * Make sure we understand the fack packet body version.
     */

    if (bodyp->vers != RPC_C_DG_FACKPKT_BODY_VERS)
        return;

    /*
     * Extract the fack sender's offered window size from the fack body.
     */

    call->xq.window_size = bodyp->window_size; 
    max_tsdu      = bodyp->max_tsdu;
    max_frag_size = bodyp->max_frag_size;

    if (NDR_DREP_INT_REP(rqe->hdrp->drep) != ndr_g_local_drep.int_rep)
    {
        SWAB_INPLACE_16(call->xq.window_size);
        SWAB_INPLACE_32(max_tsdu);
        SWAB_INPLACE_32(max_frag_size);
    }                                    
             
    RPC_DBG_PRINTF(rpc_e_dbg_recv, 6, (
        "(do_fack_body) <-- ws %lu (KB), max_tsdu %lu, max_frag_size %lu\n", 
        call->xq.window_size, max_tsdu, max_frag_size));

    /*
     * Determine the correct fragment size to use for this conversation.
     * On this side we're restricted to using the min of the maximum
     * fragment accepted by the local service interface, and the maximum
     * fragment size that we've determined for the path to the remote
     * address. We also cannot exceed the max fragment size for our peer's
     * local service interface.
     */

    our_min = MIN(call->xq.max_snd_tsdu, call->xq.max_frag_size);

    max_frag_size = MIN(max_tsdu, max_frag_size);
    snd_frag_size = MIN(our_min, max_frag_size);

    /*
     * Fragment sizes must be 0 MOD 8.  Make it so.
     */

    snd_frag_size &= ~ 0x7;

    if (snd_frag_size == 0)
        snd_frag_size = RPC_C_DG_INITIAL_MAX_PKT_SIZE;

    /*
     * The fack body length serves as a minor version number.  If its
     * length indicates that a selective ack mask is present, go process
     * it.
     */
                
    if (rqe->hdrp->len > 12)
    {
        do_selective_ack( rqe, call, window_incr, rexmit_cnt, curr_serial);
    }

    /*
     * Before updating our snd_frag_size, make sure that we have enough
     * packets reserved.
     */
    if (call->n_resvs >= call->max_resvs)
    {
        /*
         * We already have the maximum reservation.
         */
        call->xq.snd_frag_size = snd_frag_size;
    }
    else if (call->xq.push)
    {
        /*
         * Keep using the current snd_frag_size;
         * since everything is already queued.
         */
    }
    else
    {
        unsigned32 resv;
        RPC_DG_FRAG_SIZE_TO_NUM_PKTS(snd_frag_size, resv);
        if (call->n_resvs == resv
            || rpc__dg_pkt_adjust_reservation(call, resv, false))
        {
            call->xq.snd_frag_size = snd_frag_size;
        }
        /*
         * Else keep using the current snd_frag_size.
         */
    }

    RPC_DBG_PRINTF(rpc_e_dbg_recv, 6, (
        "(do_fack_body) <-- our snd_tsdu %lu, max fs %lu, snd fs %lu\n", 
       call->xq.max_snd_tsdu, call->xq.max_frag_size, call->xq.snd_frag_size));

#ifdef DEBUG
    if (RPC_DBG (rpc_es_dbg_dg_max_window_size, 1))
    {
        unsigned32 window_size =
           (unsigned32)(rpc_g_dbg_switches[(int)rpc_es_dbg_dg_max_window_size]);

        if (call->xq.window_size > window_size)
        {
            call->xq.window_size = window_size;
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 6,
                           ("(do_fack_body) <-- ws dropped to %lu (KB)\n",
                            call->xq.window_size));
        }
    }
#endif
    /*
     * We need to adjust the window size from kilobytes to
     * the number of fragments, if the window size > 2.
     * See also rpc__dg_call_xmit_fack()::dgcall.c .
     */
    if (call->xq.window_size > 2)
        call->xq.window_size = ((unsigned32)call->xq.window_size << 10)
                                   / call->xq.snd_frag_size;

    RPC_DBG_PRINTF(rpc_e_dbg_recv, 6,
                   ("(do_fack_body) <-- adjusted ws %lu (fragments)\n",
                    call->xq.window_size));
#else

#error    "MISPACKED_HDR_CODE NEEDED HERE" /* !!! */

#endif /* MISPACKED_HDR */
}


/*
 * D O _ F A C K
 *
 * Packet handler for "fack" packets.
 */

INTERNAL boolean do_fack
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    rpc_dg_call_p_t call;
    boolean sent_data;

    RPC_LOCK_ASSERT(0);

    /*
     * Get the appropriate call handle for this fack.  In case the fack
     * arrived on a server socket, it's easy.  (The fack must be facking
     * the out's the server has sent.)  Facks arriving on client sockets
     * are trickier (see below).
     *
     * !!! even this path isn't that simple... the handling of a fack
     * for large INs of a callback aren't handled; good thing callbacks
     * aren't supported.  Also looks like there should really be checking
     * of the scall state (fack may be for previously completed call and
     * should just be dropped).
     */

    if (RPC_DG_SOCKET_IS_SERVER(sp))
    {
        rpc_dg_sct_elt_p_t scte;
        rpc_dg_scall_p_t scall;

        scte = rpc__dg_sct_lookup(&hdrp->actuid, hdrp->ahint);
        if (scte == NULL)
            return (RPC_C_DG_RDF_FREE_RQE);
        rpc__dg_sct_inq_scall(scte, &scall, rqe);
        RPC_DG_SCT_RELEASE(&scte);
        if (scall == NULL || hdrp->seq != scall->c.call_seq)
        {
            if (scall != NULL)
                RPC_DG_CALL_UNLOCK(&scall->c);
            return (RPC_C_DG_RDF_FREE_RQE);
        }
        call = &scall->c;
    }
    else
    {
        rpc_dg_ccall_p_t ccall;

        /*
         * Distinguish between (a) facks of in's on calls, and (b) facks
         * on out's on callbacks.  They both come in on client sockets
         * and we have to sort them out based on (1) whether there's
         * a callback in progress, and (2) if there IS a callback, the
         * sequence numbers of the (i) original call, (ii) the callback,
         * and (iii) the fack packet itself.  Pretty horrible, eh?  Who
         * designed this protocol anyway?  (Two different types of facks
         * might have been useful.)
         */
        ccall = rpc__dg_ccallt_lookup(&hdrp->actuid, RPC_C_DG_NO_HINT);
        if (ccall == NULL)
        {
            return (RPC_C_DG_RDF_FREE_RQE);
        }

        /*
         * !!! this should really be using rpc__dg_ccall_lsct_inq_scall()
         * to get / determine if there is an scall present.  I'd fix it to
         * do that, but it looks like other things are already messed up...
         * (e.g. looks like the (cbk)scall that ends up getting used isn't
         * locked); just leave it be since we aren't supporting callbacks 
         * anyway.
         */
        if (ccall->cbk_scall != NULL && !ccall->c.is_cbk && 
            hdrp->seq == ccall->cbk_scall->c.call_seq)
        {
            call = &ccall->cbk_scall->c;
           /*
            * We must also check to see if we are in the "orphan" state.
            * If in the "orphan" state, the client thread is waiting for
            * a "quack" in end_call(); it must not be awakened by do_fack().
            * Besides, the call is done anyway if it has transitioned to
            * "orphan", the incoming fack is unimportant now.
            */
            if (call->state == rpc_e_dg_cs_orphan)
            {
                RPC_DG_CCALL_RELEASE(&ccall);
                return (RPC_C_DG_RDF_FREE_RQE);
            }
        }    
        else if (rpc__dg_do_common_response(sp, rqe, ccall))
        {
            call = &ccall->c;
        }
        else
        {
            RPC_DG_CCALL_RELEASE(&ccall);
            return (RPC_C_DG_RDF_FREE_RQE);
        }

    }

    /*
     * Perform the boot time processing / verification that couldn't
     * be done in recv_dispatch() (this is a virtual duplicate of the
     * code there).  If this is a c->s pkt verify the boot time if one
     * is specified (initialize rqe's having an unspecified time so that
     * subsequent transmits using this rqe have a valid boot time).  (Note
     * that s->c verification is done in the call to
     * rpc__dg_do_common_response() above.)
     */
    
    if (RPC_DG_CALL_IS_SERVER(call))
    {
        if (! rpc__dg_svr_chk_and_set_sboot(rqe, sp))
        {
            return (RPC_C_DG_RDF_FREE_RQE);
        }
    }

    rpc__dg_fack_common(sp, rqe, call, &sent_data);

    /*
     * Release or unlock the call handle.  This is a little grubby because
     * the call handle might have come from either a CCALLT lookup or
     * from an SCTE.  The lookup returns a locked, referenced CCALL which
     * must be released.  The SCTE contains an SCALL which we've locked
     * and must now simply unlock.
     */

    if (RPC_DG_CALL_IS_CLIENT(call)) 
    {
        rpc_dg_ccall_p_t ccall = (rpc_dg_ccall_p_t) call;
        RPC_DG_CCALL_RELEASE(&ccall);
    }
    else
    {
        RPC_DG_CALL_UNLOCK(call);
    }

    return (RPC_C_DG_RDF_FREE_RQE);
}


/*
 * R P C _ _ D G _ D O _ F A C K _ C O M M O N 
 * 
 * Common logic for received "fack" and "nocall" packets.
 */

PRIVATE void rpc__dg_fack_common
(
    rpc_dg_sock_pool_elt_p_t sp ATTRIBUTE_UNUSED,
    rpc_dg_recvq_elt_p_t rqe,
    rpc_dg_call_p_t call,
    boolean *sent_data
)
{
#define CHECK_INITIALIZED(var) \
	do {\
		if (! var##_initialized)\
		{ fprintf(stderr, "%s: accessing uninitialized variable %s in %s at %d\n", \
				__PRETTY_FUNCTION__, #var, __FILE__, __LINE__);\
			abort();\
		} \
		} while (0)

    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    rpc_dg_xmitq_p_t xq;
    rpc_dg_xmitq_elt_p_t next_xqe, temp;
    rpc_dg_fackpkt_body_p_t bodyp = (rpc_dg_fackpkt_body_p_t) &rqe->pkt->body;
    unsigned32 rexmit_cnt = 0, window_incr = 0, ready_to_go;
    unsigned32 curr_serial = 0;
	 boolean curr_serial_initialized = false;
    signed32 blast_size;
    boolean using_selacks;                   
    boolean first_fack = false;


    call->last_rcv_timestamp = rpc__clock_stamp();
    xq = &call->xq;

    if (xq->first_fack_seen == false)
    {
        xq->first_fack_seen = first_fack = true;
    }

    /*
     * A fack satisifies any xmitq awaiting acklowlegement activity.
     */

    RPC_DG_XMITQ_AWAITING_ACK_CLR(xq);
    
    /*
     * Lose whatever retransmission queue had been formed.  The fack
     * provides the latest information on the state of the queue, overiding
     * what the call timer or a previous fack has told us.
     */

    xq->rexmitq = NULL;

    /*
     * Remove packets mentioned in the inclusive portion of the fack
     * from the transmit queue.
     *            
     * In order to decide on how many packets to send in the next blast, 
     * keep track of how many packets were in the blast which contained
     * the packet which induced this fack.  As in slow-start, we'll send
     * twice this number in the next blast.
     *               
     * For backward compatibility, find out if the receiver is using
     * selective acks.  We need to do this here because in order to
     * accurately guage the number of packets that were received in the
     * last blast, we need to know the serial number of the packet which
     * induced this fack.  We will perform selective ack processing if
     * the the fack packet version matches, and the packet is longer
     * than 12 bytes.  (The packet length serves as a minor version number
     * here because we didn't want to break existing 2.0 code with this
     * change.)
     *
     * The curr_serial value is used to track packets within blasts
     * from the ximtq.
     */

    using_selacks = (hdrp->len > 12 && 
                     bodyp->vers == RPC_C_DG_FACKPKT_BODY_VERS);

    if (using_selacks)
    {
        curr_serial = xq->last_fack_serial;
		  curr_serial_initialized = true;
        xq->last_fack_serial = 
                ((rpc_dg_fackpkt_body_p_t) &rqe->pkt->body)->serial_num;

        if (NDR_DREP_INT_REP(hdrp->drep) != ndr_g_local_drep.int_rep)
        {
            SWAB_INPLACE_16(xq->last_fack_serial);
        }                                    
    }
           
    for ( ; xq->head != xq->first_unsent 
              && RPC_DG_FRAGNUM_IS_LTE(xq->head->fragnum, rqe->hdrp->fragnum); )
    {
        next_xqe = xq->head->next;
                          
        if (using_selacks)	{
			  CHECK_INITIALIZED(curr_serial);
            TRACK_CURRENT_BLAST(xq, xq->head, curr_serial, &window_incr, false);
		  }
        else
            window_incr++;

        /*
         * If this packet is in the current congestion window, decrement
         * it from the count, and check to see if we should also decrement
         * the count of outstanding facks.
         */

        if (xq->head->in_cwindow)
        {      
            xq->cwindow_size--;

            if (! RPC_DG_FLAG_IS_SET(xq->head->flags, RPC_C_DG_PF_NO_FACK))
                xq->freqs_out--;
        }
        
        /* 
         * Remove the packet from the queue *before* freeing it.  Since
         * some of the packet rationing logic is carried out by the
         * free_xqe routine, it needs to know the accurate state of this
         * call handle.
         */

        temp = xq->head;
        xq->head = next_xqe;
        rpc__dg_pkt_free_xqe(temp, call);
    }
    
    if (xq->head == NULL)
    {
        xq->tail = NULL;
    }
                         
    /*
     * If the fack has a body, process it.
     */

    if (hdrp->len > 0)
    {
		 CHECK_INITIALIZED(curr_serial);
        do_fack_body(rqe, call, &window_incr, &rexmit_cnt, curr_serial);
    }
     
    /*
     * Set the size of the next blast to twice the number of packets
     * that were successfully received in the same blast as this fack.
     * Note that we may be adding in previously determined, but as yet
     * unsent, value of blast_size.  This smoothes out the window growth
     * when facks for small blast sizes are coming in back to back.  The
     * blast_size will be cut down later if if the size grows beyond
     * a max limit.  If window_incr is 0, whatever blast induced this
     * ack has already been sent again and we've lost its history.
     * If this happens, start the window over.
     */

    blast_size = MAX(MIN(xq->blast_size + (2 * window_incr), 
                     xq->max_blast_size), 2);

    if (xq->cwindow_size + blast_size > xq->window_size)
        blast_size = MAX(xq->window_size - xq->cwindow_size, 0);

    /*
     * Check to see if we should adjust the max blast size allowed for
     * this conversation.  If are seeing dropped packets, and we had
     * previously increased the blast size, start pulling back on it.
     * If we aren't losing packets the xq_timer will eventually get
     * decremented to 0, and we can try increasing the blast size.  Note
     * that each time we fiddle with this we increase the throttle, which
     * increases the time between possible adjustments.
     */

    if (rexmit_cnt > 0)
    {      
        if (xq->max_blast_size > RPC_C_DG_INITIAL_MAX_BLAST_SIZE)
        {  
            /*
             * If we have previously raised the blast size,
             * bring it down some.  Note that after we raise the blast
             * size, we won't find out the result until after a RTT. 
             * Keep lowering the blast size for each lost packet, until we
             * reach the initial setting.  If only one packet is lost in the
             * RTT, we may get by with only a partial drop in the blast size.
             * 
             * Each time we make such an adjustment, raise the timer throttle.
             * This increases the time between checks of the blast size.  We do
             * this to avoid oscillating in the face of lost packets.  We really
             * are only interested in the situation in which we're being
             * far too cautious, i.e. fast, reliable networks.  If there's any
             * loss happening, be conservative.
             */

            xq->max_blast_size -= 2;
            xq->xq_timer_throttle++;
            xq->xq_timer_throttle = MIN(xq->xq_timer_throttle, 1000);
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 5, (
                "(do_fack) Lowering blast size %lu\n", xq->max_blast_size));
        }
        xq->xq_timer = xq->xq_timer_throttle * RPC_C_DG_INITIAL_XQ_TIMER;
        xq->high_cwindow = 0;
    }
    else if (xq->xq_timer != 0)
    {
        xq->xq_timer--;
    }

    if (xq->xq_timer == 0)	{
        if (xq->high_cwindow < xq->window_size)
        {                
            xq->max_blast_size = MIN(xq->max_blast_size + 2, RPC_C_DG_MAX_BLAST_SIZE);
            xq->xq_timer = xq->xq_timer_throttle * RPC_C_DG_INITIAL_XQ_TIMER;
            RPC_DBG_PRINTF(rpc_e_dbg_recv, 5, (
                "(do_fack) Raising blast size %lu\n", xq->max_blast_size));
        } 
        else
        {
            xq->xq_timer_throttle++;
            xq->xq_timer_throttle = MIN(xq->xq_timer_throttle, 1000);
            xq->xq_timer = xq->xq_timer_throttle * RPC_C_DG_INITIAL_XQ_TIMER;
            xq->high_cwindow = 0;
        }
	 }
    /* 
     * Now see if the thread is keeping up.  We hope that the thread
     * will have queued up enough packets to cover the new blast size
     * (i.e. # of new pkts = blast_size - rexmit_cnt).  There can be
     * two reasons why it hasn't: First, room in the cwindow may have
     * opened up by more than a max blast size, as can happen when we
     * get back-to-back facks.  Second, the client thread may really
     * not be queueing up packets fast enough.  In either case, we are
     * (temporarily) unable to fully replenish the cwindow.  Therefore,
     * lower the blast size to what we are really prepared to send.  This
     * value inhibits the cwindow from growing.  Note that it is possible
     * for the blast size to be set to 0.  This is ok, because the client
     * thread will never see this value since it won't be woken up until
     * there is another fack or a timeout, and the values have been
     * overridden.
     */

    ready_to_go = rexmit_cnt + (xq->first_unsent == NULL ? 0 :
                  xq->next_fragnum - (signed16) xq->first_unsent->fragnum);

    xq->blast_size = (ready_to_go < (unsigned32)blast_size) ? ready_to_go : (unsigned32) blast_size;


    RPC_DBG_PRINTF(rpc_e_dbg_recv, 5, (
        "(do_fack%s%s) frag %lu, cws %lu, bs %lu, fo %lu\n", 
        ready_to_go < (unsigned32)blast_size ? "-slow-q" : "",
        rexmit_cnt > 0 ? "-loss" : "",
        rqe->hdrp->fragnum, xq->cwindow_size, xq->blast_size, 
        xq->freqs_out));

    /*
     * If there is data ready to be sent, do the right thing to get it
     * sent.  The macro below will decide if the client/call thread is
     * is still around to do the transmission.  If not, it will have
     * to be sent from the listener thread.  For calls using a private
     * socket, do nothing;  since we were called by the call thread itself,
     * we just want to return so that it can do the transmission.
     *
     * If this is the first fack seen by the user of the shared socket,
     * we need to wake up the call thread to continue the transmission
     * because it's waiting for the first fack in
     * rpc__dg_call_transmit_int()::dg.c .
     */                   

    *sent_data = RPC_DG_CALL_READY_TO_SEND(call);

    if (! call->sock_ref->is_private &&
        (*sent_data ||
         (first_fack && xq->next_fragnum == 1 &&
          !call->is_cbk && call->state == rpc_e_dg_cs_xmit)))
    {
        RPC_DG_START_XMIT(call);
    }

    /*
     * Receipt of a fack should generally induce us to do re-xmits in
     * a more timely fashion.  So reset the re-xmit timer, UNLESS there's
     * no room in the window (in which case we don't want to go back
     * to clobbering the "full" receiver with re-xmits).
     */

    if (xq->window_size > 0)
    {
        xq->rexmit_timeout = RPC_C_DG_INITIAL_REXMIT_TIMEOUT;
    }
#undef CHECK_INITIALIZED
}


/*
 * D O _ B A D _ P K T
 *
 * Packet handler for packets with bogus packet types.
 */

INTERNAL rpc_dg_rd_flags_t do_bad_pkt (
        rpc_dg_sock_pool_elt_p_t /*sp*/,
        rpc_dg_recvq_elt_p_t /*rqe*/
    );

INTERNAL rpc_dg_rd_flags_t do_bad_pkt
(
    rpc_dg_sock_pool_elt_p_t sp ATTRIBUTE_UNUSED,
    rpc_dg_recvq_elt_p_t rqe ATTRIBUTE_UNUSED
)
{
    RPC_DBG_GPRINTF(("(do_bad_pkt) Bad packet; ptype = %u\n", 
                (unsigned int) RPC_DG_HDR_INQ_PTYPE(rqe->hdrp)));
    return (RPC_C_DG_RDF_FREE_RQE);
}

/*
 * R E C V _ D I S P A T C H
 *
 * Process a received packet from the specified connection.
 *
 * Returns "dispatch flags".  See "rpc_dg_rd_flags_t" in "dg.h".
 */

INTERNAL rpc_dg_rd_flags_t recv_dispatch (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

INTERNAL unsigned32 recv_dispatch
(
    rpc_dg_sock_pool_elt_p_t sp,
    rpc_dg_recvq_elt_p_t rqe
)
{
    rpc_dg_pkt_hdr_p_t hdrp = rqe->hdrp;
    unsigned16 ihint = hdrp->ihint;
    rpc_dg_ptype_t ptype;
    rpc_dg_ccall_p_t ccall;
    rpc_dg_scall_p_t scall;
    unsigned32 st;
    rpc_dg_rd_flags_t rdflags;
    rpc_dg_call_p_t locked_call = NULL;

    /*
     * If forwarding is enabled...
     *
     * Potentially forward pkts that arrive on server sockets.
     * Pkts arriving on client sockets are clearly for this server
     * (they're "responses" to RPCs being made by this server).
     *
     * Don't try to forward to a nil interface uuid. The interface
     * uuid will only be nil when it's a 1.5.1 buggy ack or reject packet.
     *
     * As an optimization for the rpcd, if we see that the call is being
     * made on the endpoint-map interface, avoid trying to do the lookup 
     * in the forward map; ept calls don't get forwarded, they're handled
     * by the forwarding server (rpcd).
     *
     * Given all this, the general stratagy is to give the forwarding 
     * function first crack at all packets.  If the forwarder is not able
     * to handle the packet, then we consider handling it ourself.
     *
     * One caveat: before trying to handle the call, we first check to 
     * see if we recognize the interface that the call is being made on.
     * If we don't, we drop the packet.  Otherwise, we run the risk of
     * returning the "unknown interface" reject to any client whose 
     * server doesn't happen to be running at the time (for example, if
     * the client had pulled a stale binding handle out of the name
     * space.
     */

    if ((rpc_g_fwd_fn != NULL) && 
        (sp->is_server) && 
        (! UUID_IS_NIL(&hdrp->if_id, &st)) &&
        (! UUID_EQ(hdrp->if_id, ((rpc_if_rep_p_t) ept_v3_0_c_ifspec)->id, &st)))
    {
        unsigned32 temp_ret = rpc__dg_fwd_pkt(sp, rqe);

        switch(temp_ret)
        {
            case FWD_PKT_DONE:
                return (RPC_C_DG_RDF_FREE_RQE);
                break;

            case FWD_PKT_DELAYED:
                return(0);
                break;

            case FWD_PKT_NOTDONE:
                /* continue */
                break;
        }

        /*
         * The forwarding function wasn't able to handle the packet.  Check
         * to see if the call is being made on an interface that we support.
         */
        RPC_LOCK(0);
        rpc__if_lookup
            (&hdrp->if_id, hdrp->if_vers, NULL, &ihint, NULL, NULL, NULL, &st);
        RPC_UNLOCK(0);

        if (st != rpc_s_ok) 
        {
            /*
             * We're going to drop this request packet, which means that the
             * client will eventually fail with a comm timeout.  Rather than
             * actually making it wait, we can induce the comm timeout with
             * a reject.
             */
            if (! RPC_DG_HDR_FLAG_IS_SET(hdrp, RPC_C_DG_PF_BROADCAST |
                                               RPC_C_DG_PF_MAYBE))
            {
                RPC_DBG_GPRINTF(("rejecting un-forwardable call\n"));
                rpc__dg_xmit_error_body_pkt((sp)->sock,
                    (rpc_addr_p_t) &rqe->from, rqe->hdrp, RPC_C_DG_PT_REJECT,
                    nca_s_comm_failure);
            }
            return (RPC_C_DG_RDF_FREE_RQE);
        }
        
        /*
         * Finally, in the case of the rpc mgmt i/f, if the client really
         * wants the call to be handled by the rpcd it will make the call using 
         * a nil object UUID.  If the object UUID is not nil, the call must have
         * been intended for some server that is not currently running, and we 
         * should not handle the call locally.
         */
        if ((UUID_EQ(hdrp->if_id, ((rpc_if_rep_p_t) mgmt_v1_0_s_ifspec)->id, &st)) &&
            (! UUID_IS_NIL(&hdrp->object, &st)))
        {
            /*
             * We're going to drop this request packet, which means that the
             * client will eventually fail with a comm timeout.  Rather than
             * actually making it wait, we can induce the comm timeout with
             * a reject.
             */
            if (! RPC_DG_HDR_FLAG_IS_SET(hdrp, RPC_C_DG_PF_BROADCAST |
                                               RPC_C_DG_PF_MAYBE))
            {
                RPC_DBG_GPRINTF(("rejecting un-forwardable mgmt call\n"));
                rpc__dg_xmit_error_body_pkt((sp)->sock,
                    (rpc_addr_p_t) &rqe->from, rqe->hdrp, RPC_C_DG_PT_REJECT,
                    nca_s_comm_failure);
            }
            return (RPC_C_DG_RDF_FREE_RQE);
        }
    }

    /*  
     * Handle packet locally.
     */

    /*
     * First check for packets received on private sockets.
     */
    if (sp->is_private)
    {
        /*
         * For private sockets, we try to anticipate the receipt of
         * response packets and jump right to the response packet handler.
         * For any other type of packet, we go through the normal dispatch
         * code.
         */
        ptype = RPC_DG_HDR_INQ_PTYPE(rqe->hdrp);
        if (RPC_DG_PT_IS(ptype, (1 << RPC_C_DG_PT_RESPONSE)) &&
                 UUID_EQ(rqe->hdrp->actuid, sp->ccall->c.call_actid, &st))
        {
            rdflags = rpc__dg_do_response(sp, rqe, sp->ccall);
            return (rdflags);
        }
        else
        {
            /*
             * The "normal" dispatch code will not expect this handle
             * to be locked.
             *
             * Check if we are executing the callback.
             */
            if (sp->ccall->cbk_scall != NULL
                && sp->ccall->cbk_scall->has_call_executor_ref == true)
                locked_call = (rpc_dg_call_p_t) sp->ccall->cbk_scall;
            else
                locked_call = (rpc_dg_call_p_t) sp->ccall;

            RPC_DG_CALL_UNLOCK(locked_call);
        }
    }

    RPC_LOCK(0);

    ptype = (rpc_dg_ptype_t) RPC_DG_HDR_INQ_PTYPE(hdrp);
    /*
     * Detect all conversation manager related client->server packets
     * and hand them off to a handler (saving each pkt handler from
     * performing this check).  The handler which will generate and send
     * the appropriate response to the server and free the rqe.
     * See comments above "rpc__dg_handle_conv" for more details on WAY
     * processing.
     *
     * WARNING: the position of this processing is critical... we need
     * to ensure that we don't search the ccallt for the WAY's activity id.
     * If the client and server are in the same address space the WAY
     * client has a ccallt for the WAY activity id (which would otherwise
     * make us attempt to do "real callback" kinds of things).
     * 
     * We know all WAY client->server pkts arrive on a client socket.
     * This allows us (fortunately) to only have the uuid check overhead
     * incurred in what we wouldn't consider the "fast path" (we do suffer
     * the overhead for the "real callback" fast path... tanstaafl).
     * !!! We avoid the additional cost of checking for a fack since those
     * can't happen (the current implementation just can't handle multifrag
     * conv calls).
     */
    if (RPC_DG_SOCKET_IS_CLIENT(sp) && RPC_DG_PT_IS_CTOS(ptype)
        && UUID_EQ(hdrp->if_id, ((rpc_if_rep_p_t) conv_v3_0_c_ifspec)->id, &st))
    {
        boolean drop;

	drop = ! rpc__dg_svr_chk_and_set_sboot (rqe, sp);
        if (! drop) 
	{
            drop = rpc__dg_handle_conv(sp->sock, rqe);
	}

        RPC_UNLOCK(0);

        /*
         * When handling private sockets, the caller expects that the call
         * handle will be locked when we return.
         */
        if (sp->is_private && locked_call != NULL)
        {
            RPC_DG_CALL_LOCK(locked_call);
        }
        return (drop ? RPC_C_DG_RDF_FREE_RQE : 0 /* DON'T free the rqe */);
    }

    ccall = NULL;
    scall = NULL;

    /*
     * We want to have the logic for determining the call handle the
     * packet is associated with in once place.  Unfortunately, doing
     * this for request and fack packets is a little tricky, so we let
     * the handlers for those packet types do it themselves.  For the
     * other packet types we figure it out here and just pass them the
     * result.  Note that the handlers for those types must still cope
     * with the possibility that the call handle is NULL (i.e. that there
     * isn't a call handle associated with the activity mentioned in
     * the packet).  Note also that the call handle we pass (a) is locked,
     * and (b) has had its reference count bumped already.
     */

    if (! RPC_DG_PT_IS(ptype, 
                ((1 << RPC_C_DG_PT_REQUEST) | (1 << RPC_C_DG_PT_FACK))))
    {
        rpc_dg_call_p_t call;

        /*
         * If the packet arrived on a server socket, look up the packet's
         * activity ID in the SCT.  If it arrived on a client socket,
         * lookup it up in the CCALLT.  Note that in either case, the
         * packet may be part of a callback packet stream, which we can
         * tell by noticing whether it is a "client->server" or
         * "server->client" packet.
         */

        RPC_LOCK_ASSERT(0);

        if (RPC_DG_SOCKET_IS_SERVER(sp))
        {
            rpc_dg_sct_elt_p_t scte;

            scte = rpc__dg_sct_lookup(&hdrp->actuid, hdrp->ahint);
            if (scte == NULL) 
            {
                scall = NULL;
            }
            else
            {
                rpc__dg_sct_inq_scall(scte, &scall, rqe);
                RPC_DG_SCT_RELEASE(&scte);
                if (scall != NULL)
                {
                    RPC_DG_CALL_REFERENCE(&scall->c);
                    if (scall->cbk_ccall != NULL && RPC_DG_PT_IS_STOC(ptype))
                    {
                        ccall = scall->cbk_ccall;
                        RPC_DG_CALL_LOCK(&ccall->c);
                        RPC_DG_CALL_REFERENCE(&ccall->c);
                        RPC_DG_SCALL_RELEASE(&scall);
                    }
                }
            }
        }
        else
        {
            ccall = rpc__dg_ccallt_lookup(&hdrp->actuid, RPC_C_DG_NO_HINT);
            if (ccall != NULL)
            {
                if (RPC_DG_PT_IS(
                        ptype, 
                        (1 << RPC_C_DG_PT_PING ) | 
                        (1 << RPC_C_DG_PT_ACK) | 
                        (1 << RPC_C_DG_PT_QUIT)))
                {
                    rpc__dg_ccall_lsct_inq_scall(ccall, &scall);
                    if (scall != NULL)
                    {
                        RPC_DG_CALL_REFERENCE(&scall->c);
                        RPC_DG_CCALL_RELEASE(&ccall);
                    }
                }
            }
        }

        assert((scall == NULL) || (ccall == NULL));

        /*
         * If we've found a call, do a little common processing.
         */

        call = (scall != NULL) ? &scall->c : &ccall->c;
        if (call != NULL)
        {
            call->last_rcv_timestamp = rpc__clock_stamp();
            /*
             * Clear the "awaiting ack event" flag if this packet looks
             * relevant to this call.  Note that we don't let "nocall"s
             * clear the flag because they're not a positive indication
             * that the server is really making progress.  Note also
             * that the "fack" and "request" processing code takes care
             * of clearing the flag explicitly (since those packets don't
             * cause us to end up here).  (The "request" path ends up
             * clearing the flag as part of scall reinit.)
             */
            if (ptype != RPC_C_DG_PT_NOCALL && rqe->hdrp->seq >= call->call_seq)
            {
                RPC_DG_XMITQ_AWAITING_ACK_CLR(&call->xq);
            }
        }
    }

    /*
     * Switch on the packet type.  Note that for the non-special cases,
     * the call handle is locked and the reference count incremented;
     * we release the reference (and implicitly the lock) upon return
     * from the packet handler.
     * 
     * All packet type handler routines perform server boot time and
     * packet sequence filtering.  It is important to note the order
     * in which the filtering is done.  In client to server handlers
     * the server boot time is checked so that servers recieve only packets
     * destined for the current server: all other packets are dropped.
     * In server to client handlers packet sequnce is checked first to
     * 'weed out' old packets then server boot time is checked.  This
     * is done to prevent old packets from the wrong server instance
     * from causing faults to be signaled.
     */

    switch ((int)ptype) 
    {
        /*
         * Special cases.  See above comments.
         */
        case RPC_C_DG_PT_FACK:      rdflags = do_fack(sp, rqe); break;
        case RPC_C_DG_PT_REQUEST:   rdflags = rpc__dg_do_request(sp, rqe); break;

        /*
         * Client -> server packet types.
         */
        case RPC_C_DG_PT_PING:      rdflags = rpc__dg_do_ping(sp, rqe, scall); break;
        case RPC_C_DG_PT_ACK:       rdflags = rpc__dg_do_ack(sp, rqe, scall); break;
        case RPC_C_DG_PT_QUIT:      rdflags = rpc__dg_do_quit(sp, rqe, scall); break;

        /*
         * Server -> client packet types.
         */
        case RPC_C_DG_PT_RESPONSE:  rdflags = rpc__dg_do_response(sp, rqe, ccall); break;
        case RPC_C_DG_PT_FAULT:     rdflags = rpc__dg_do_fault(sp, rqe, ccall); break;
        case RPC_C_DG_PT_WORKING:   rdflags = rpc__dg_do_working(sp, rqe, ccall); break;
        case RPC_C_DG_PT_NOCALL:    rdflags = rpc__dg_do_nocall(sp, rqe, ccall); break;
        case RPC_C_DG_PT_REJECT:    rdflags = rpc__dg_do_reject(sp, rqe, ccall); break;
        case RPC_C_DG_PT_QUACK:     rdflags = rpc__dg_do_quack(sp, rqe, ccall); break;

        /*
         * Invalid packet type.
         */
        default:                    rdflags = do_bad_pkt(sp, rqe); break;
    }

    if (scall != NULL)
    {
        RPC_DG_SCALL_RELEASE(&scall);
    }
    else if (ccall != NULL)
    {
        RPC_DG_CCALL_RELEASE(&ccall);
    }

    RPC_UNLOCK(0);

    /*
     * When handling private sockets, the caller expects that the call
     * handle will be locked when we return.
     */
    if (sp->is_private && locked_call != NULL)
    {
        RPC_DG_CALL_LOCK(locked_call);
    }

    return (rdflags);
}


/*
 * R P C _ _ D G _ N E T W O R K _ S E L E C T _ D I S P A T C H
 *
 * Check for input on the specified socket.  Process any packet we receive.
 */

PRIVATE void rpc__dg_network_select_dispatch
(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    pointer_t sp_,
    boolean32 is_active,
    unsigned32 *st
)
{
    rpc_dg_sock_pool_elt_p_t sp = (rpc_dg_sock_pool_elt_p_t) sp_;
    rpc_dg_recvq_elt_p_t rqe;
    boolean in_burst;
    boolean in_listener_thread = sp->is_private == false;
    static byte_t junk_buf[16];
    static unsigned32 purged_pkts = 0;
    rpc_dg_recvq_elt_p_t tmp;
    unsigned32 i;
    unsigned32 num_rqes;
    unsigned32 n_pkts;
    rpc_dg_recvq_elt_p_t *rqe_list;
    unsigned32 *rqe_list_len;

    assert(sp->sock == sock);
          
    /*
     * If this socket has been disabled, don't even bother trying to
     * read from it.  
     */
    if (RPC_DG_SOCK_IS_DISABLED(sp))
    {                    
        *st = rpc_s_socket_failure;
        return;
    }

    /*
     * Determine the number of pkts need to be allocated.
     */
    if (in_listener_thread)
    {
        /*
         * The listener thread has the reservation for the largest fragment
         * size possible.
         */
        n_pkts = RPC_C_DG_MAX_NUM_PKTS_IN_FRAG;
        rqe_list = &saved_rqe_list;
        rqe_list_len = &saved_rqe_list_len;
        rqe = saved_rqe_list;
    }
    else
    {
        /*
         * The private socket user has either
         *    1) no reservation (using the private xqe/rqe pair),
         *    2) the minimum reservation for RPC_C_DG_MUST_RECV_FRAG_SIZE
         * or 3) the reservation for the advertised fragment size.
         *
         * Thus, we can only use whatever we reserved. Fortunately, the
         * sender will not send the fragment larger than what we have been
         * sending.
         */

        n_pkts = sp->ccall->c.n_resvs;
        if (n_pkts == 0)
        {
            n_pkts = 1;
            RPC_DBG_PRINTF(rpc_e_dbg_dg_sockets, 3,
                           ("(rpc__dg_network_select_dispatch) private socket with no patcket reservation.\n"));
        }
        rqe_list = &(sp->rqe_list);
        rqe_list_len = &(sp->rqe_list_len);
        rqe = sp->rqe_list;
    }

    *st = rpc_s_ok;

    /*
     * Loop as long as we get packets that appear to be in the middle
     * of a burst (i.e., have the "frag" bit set, don't ask for a "fack",
     * and aren't the last frag of a request.  The idea is to reduce
     * the number of times "select" is called (by our caller) by guessing
     * when there's probably another packet in the socket.  At worst,
     * since we're doing non-blocking receives, we'll occasionally do
     * a receive on an empty socket.  Never set the in_burst flag for
     * private sockets, since they never call select.
     */

    do {

        /*
         * Allocate packet buffers.
         */
        while (*rqe_list_len < n_pkts)
        {
            rqe = rpc__dg_pkt_alloc_rqe(sp->ccall);
            rqe->more_data = *rqe_list;
            *rqe_list = rqe;
            (*rqe_list_len)++;
        }
        
        assert(rqe == *rqe_list); 
        
        /*
         * There should always be an available rqe thanks to rationing AND
         * the fact that there is only a single listener thread.  However,
         * in the event that something's amiss, let's not panic or enter
         * an endless select / select_dispatch loop... just read the
         * pkt, throw it away and hope the situation is only temporary.
         */
        if (rqe == NULL)
        {
            int recv_len;

            purged_pkts++;
            (void) rpc__socket_recvfrom(sp->sock, junk_buf, sizeof(junk_buf),
                                        NULL, &recv_len);
            RPC_DBG_GPRINTF(("(rpc__dg_select_dispatch) discarded pkt !!!\n"));
            return;
        }

        if ((num_rqes =
             (in_listener_thread ? recv_pkt(sp, rqe) :
                                   recv_pkt_private(sp, rqe))) == 0)
        {
            return;
        }

        in_burst = 
            in_listener_thread &&
            RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_FRAG) &&
            RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_NO_FACK) &&
            ! RPC_DG_HDR_FLAG_IS_SET(rqe->hdrp, RPC_C_DG_PF_LAST_FRAG);
  
        /*
         * Actually process the packet.  If the socket is not active,
         * just free the rqe.  Otherwise, call "recv_dispatch and use
         * the returned receive dispatch flags to see if we have anything
         * to do.
         */
        
        if (! is_active) 
        {
            rqe->next = NULL;
            rqe->frag_len = 0;
            rqe->hdrp = NULL;
        }
        else
        {
            rpc_dg_rd_flags_t rdflags;

            /*
             * Remove rqe from the cache.
             */
            for (i = 0; i < num_rqes - 1; i++)     
                *rqe_list = (*rqe_list)->more_data;

            tmp = (*rqe_list)->more_data;
            (*rqe_list)->more_data = NULL;
            *rqe_list = tmp;
            (*rqe_list_len) -= num_rqes;

            rdflags = recv_dispatch(sp, rqe);

            if ((rdflags & RPC_C_DG_RDF_YIELD) != 0)
            {
                /*
                 * We have been informed by the packet handler that there is 
                 * a call thread that has been cond-signalled.  This happens 
                 * when 1) all of a call's INs/OUTs have arrived, or 2) enough
                 * data has arrived that we want the call thread to drain its
                 * queue.
                 *
                 * Formerly, we did a pthread_yield() at this point so that
                 * the ready-to-run call didn't have to wait for the listener
                 * thread to make it all the way back into select before the 
                 * thread switch occurred.
                 *
                 * This technique turned out to be inappropriate in the case
                 * of a many-threaded server running CPU-bound manager routines.
                 * In this scenario, the listener thread does not get run often
                 * enough to adequately service data coming in from the network.
                 *
                 * For this reason, I've removed the pthread_yield() call from
                 * this clause.  However, I'm reluctant to pull out all of the
                 * logic that results in the 'yield' flag being handed back
                 * from the packet handlers, because I'm not convinced that 
                 * we can't find a way to make use of this information.
                 */
            }
            if ((rdflags & RPC_C_DG_RDF_FREE_RQE) != 0)
            {
                /*
                 * Return rqe to the cache.
                 */
                tmp = rqe;
                tmp->next = NULL;
                tmp->frag_len = 0;
                tmp->hdrp = NULL;
                while (tmp->more_data != NULL)
                    tmp = tmp->more_data;
                tmp->more_data = *rqe_list;
                *rqe_list = rqe;
                (*rqe_list_len) += num_rqes;
            }
        }

    } while (in_burst);
}
