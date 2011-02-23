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
**      dgutl.c
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

#include <dg.h>
#include <dgrq.h>
#include <dgxq.h>

/* ========================================================================= */

#ifdef RPC_DG_PLOG

    /*
     * Stuff for the packet log.
     */

#ifndef RPC_C_DG_PKT_LOG_SIZE
#  define RPC_C_DG_PKT_LOG_SIZE 256
#endif

#define MAX_LOGGED_PKT_BODY_LEN 16

typedef struct {
    rpc_clock_t         timestamp;
    unsigned32          lossy_action;
    rpc_dg_raw_pkt_hdr_t hdr;
    byte_t              body[MAX_LOGGED_PKT_BODY_LEN];
} pktlog_elt_t, *pktlog_elt_p_t;

/*
 * Don't make the pkt log buffer INTERNAL (static) we want it to show
 * up in the symbol table.
 */

PRIVATE pktlog_elt_p_t rpc_g_dg_pkt_log;
PRIVATE unsigned32 rpc_g_dg_pkt_log_bytes;

#ifdef STATIC_DG_PKTLOG
INTERNAL pktlog_elt_t _pkt_log[RPC_C_DG_PKT_LOG_SIZE];
#endif

INTERNAL unsigned16 pkt_log_index;

#endif

/* ========================================================================= */

#ifdef DEBUG

/*
 * Buffer for "rpc__dg_act_seq_string".
 *
 * No lock requirements.
 */

INTERNAL char act_seq_string_buff[64]; 

#ifndef NO_SPRINTF
#  define RPC__DG_ACT_SEQ_SPRINTF   sprintf
#else
#  define RPC__DG_ACT_SEQ_SPRINTF   rpc__dg_act_seq_sprintf
#endif

#endif /* _D_E_B_U_G */

/* ========================================================================= */

#ifdef RPC_DG_PLOG

/*
 * R P C _ _ D G _ P L O G _ P K T
 *
 * Add a pkt to the pkt log.  Save only the first MAX_LOGGED_PKT_BODY_LEN
 * bytes of the pkt body.
 */

PRIVATE void rpc__dg_plog_pkt
(
    rpc_dg_raw_pkt_hdr_p_t hdrp,
    rpc_dg_pkt_body_p_t bodyp,
    boolean32 recv,
    unsigned32 lossy_action     /* (0)drop, (1)?, (2)rexmit, (3)normal */
)
{
    pktlog_elt_p_t pp;

    if (rpc_g_dg_pkt_log == NULL)
    {
        rpc_g_dg_pkt_log_bytes = RPC_C_DG_PKT_LOG_SIZE * sizeof(pktlog_elt_t);
#ifdef STATIC_DG_PKTLOG
        rpc_g_dg_pkt_log = _pkt_log;
#else
        RPC_MEM_ALLOC(rpc_g_dg_pkt_log, pktlog_elt_p_t,
                rpc_g_dg_pkt_log_bytes,
                RPC_C_MEM_DG_PKTLOG, RPC_C_MEM_NOWAIT);
        if (rpc_g_dg_pkt_log == NULL)
            return;
        /* b_z_e_r_o_(rpc_g_dg_pkt_log, rpc_g_dg_pkt_log_bytes);*/
        memset(rpc_g_dg_pkt_log, 0, rpc_g_dg_pkt_log_bytes);
#endif
    }

    pp = &rpc_g_dg_pkt_log[pkt_log_index];

    pkt_log_index = (pkt_log_index + 1) % RPC_C_DG_PKT_LOG_SIZE;

    pp->timestamp = rpc__clock_stamp();
    pp->lossy_action = lossy_action;

    /*b_c_o_p_y_(hdrp, &pp->hdr, sizeof(rpc_dg_raw_pkt_hdr_t));*/
    memmove( &pp->hdr, hdrp, sizeof(rpc_dg_raw_pkt_hdr_t));

#ifndef MISPACKED_HDR
    /* b_c_o_p_y_ (bodyp, ((char *) pp->body), 
          MIN(MAX_LOGGED_PKT_BODY_LEN, ((rpc_dg_pkt_hdr_p_t) hdrp)->len));*/

    memmove( pp->body, bodyp, 
          MIN(MAX_LOGGED_PKT_BODY_LEN, ((rpc_dg_pkt_hdr_p_t) hdrp)->len)) ;
#else

/* b_c_o_p_y(bodyp, ((char *) pp->body), 
       MIN(MAX_LOGGED_PKT_BODY_LEN, ...extracted from raw hdr... hdrp->len));*/

    memmove( pp->body, bodyp, 
          MIN(MAX_LOGGED_PKT_BODY_LEN, ...extracted from raw hdr... hdrp->len));
#endif

    if (recv)
#ifndef MISPACKED_HDR
        ((rpc_dg_pkt_hdr_p_t)(&pp->hdr))->_rpc_vers |= 0x80;
#else
        set bit in raw hdr
#endif
}


/*
 * R P C _ _ D G _ P L O G _ D U M P
 *
 * Dump the packet log.
 */

void (*rpc__dg_plog_dump_addr)(void) = rpc__dg_plog_dump; /* !!! dde hack */

PRIVATE void rpc__dg_plog_dump(void)
{
    unsigned16 i;
    unsigned32 st;
    static char *lossy_action = "d?r "; /* (0)drop, (1)?, (2)rexmit, (3)normal */

    RPC_LOCK_ASSERT(0);

    if (rpc_g_dg_pkt_log == NULL)
    {
	RPC_DBG_PRINTF(rpc_e_dbg_dg_pktlog, 1,
            ("rpc__dg_plog_dump called, but DG Pkt Logging never enabled\n") );
        return;
    }

    RPC_DBG_PRINTF(rpc_e_dbg_dg_pktlog, 1,
	("tstamp   ver ptyp f1 f2     seq/fnum/sn    ihnt ahnt  len              interface/ver/op                            activity                  sboot                    object              drep   at\n") );
    RPC_DBG_PRINTF(rpc_e_dbg_dg_pktlog, 1,
	("---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n") );

    for (i = 0; i < RPC_C_DG_PKT_LOG_SIZE; i++)
    {
        pktlog_elt_p_t p = &rpc_g_dg_pkt_log[i];
        unsigned_char_p_t obj, iface, act;
        rpc_dg_pkt_hdr_p_t hdrp;

#ifndef MISPACKED_HDR
        hdrp = (rpc_dg_pkt_hdr_p_t) &p->hdr;
#else
        hdrp = converted local rep of raw hdr
#endif

        if (p->timestamp == 0)
            break;

        dce_uuid_to_string(&hdrp->object, &obj, &st);
        dce_uuid_to_string(&hdrp->if_id, &iface, &st);
        dce_uuid_to_string(&hdrp->actuid, &act, &st);

        RPC_DBG_PRINTF(rpc_e_dbg_dg_pktlog, 1,
	    ("%08x %c%c%1u %-4.4s %02x %02x %08x/%04x/%04x %04x %04x %4d %s/%02u/%03u %s %9u %s %02x%02x%02x %02x\n",
                p->timestamp,
                (hdrp->_rpc_vers & 0x80) ? 'R' : lossy_action[p->lossy_action],
                ((i + 1) % RPC_C_DG_PKT_LOG_SIZE == pkt_log_index) ? '*' : ' ',
                hdrp->_rpc_vers & 0x7f, 
                rpc__dg_pkt_name(RPC_DG_HDR_INQ_PTYPE(hdrp)),
                hdrp->flags, hdrp->flags2,
                hdrp->seq, hdrp->fragnum,
                hdrp->serial_hi << 8 | hdrp->serial_lo,
                hdrp->ihint, hdrp->ahint,
                hdrp->len, 
                iface, hdrp->if_vers, hdrp->opnum, act, 
                hdrp->server_boot, obj, 
                hdrp->drep[0], hdrp->drep[1], hdrp->drep[2], 
                hdrp->auth_proto) );

        rpc_string_free(&obj, &st);
        rpc_string_free(&act, &st);
        rpc_string_free(&iface, &st);
    }
}

#endif


/*
 * R P C _ _ D G _ A C T _ S E Q _ S T R I N G
 *
 * Return a pointer to a printed packet's activity-UID/seq/fragnum string.
 *
 * No lock requirements.
 */

PRIVATE char *rpc__dg_act_seq_string
(
    rpc_dg_pkt_hdr_p_t hdrp ATTRIBUTE_UNUSED
)
{
#ifndef DEBUG

    return("");

#else

    RPC__DG_ACT_SEQ_SPRINTF(act_seq_string_buff, "%s, %lu.%u",
                            rpc__uuid_string(&hdrp->actuid), (unsigned long) hdrp->seq, hdrp->fragnum);
    return(act_seq_string_buff);

#endif /* D_E_B_U_G_ */
}


/*
 * R P C _ _ D G _ P K T _ N A M E  
 *
 * Return the string name for a type of packet.  This can't simply be
 * a variable because of the vagaries of global libraries.
 *
 * No lock requirements.
 */

PRIVATE char *rpc__dg_pkt_name
(
    rpc_dg_ptype_t ptype ATTRIBUTE_UNUSED
)
{
#ifndef DEBUG

    return("");

#else

    static char *names[RPC_C_DG_PT_MAX_TYPE + 1] = {
        "request",
        "ping",
        "response",
        "fault",
        "working",
        "nocall",
        "reject",
        "ack",
        "quit",
        "fack",
        "quack"
    };

    return((int) ptype > RPC_C_DG_PT_MAX_TYPE ? "BOGUS PACKET TYPE" : names[(int) ptype]);

#endif /* D_E_B_U_G_ */
}


/*
 * R P C _ _ D G _ X M I T _ P K T
 *
 * Send the packet described by the iov to the specified addr using the
 * specified socket.  
 *
 * This is intended as a utility routine for non-critical processing.
 *
 * The DG packet hdr is always iov[0] and may require MISPACKED_HDR
 * processing;  iov[1..(iovlen-1)] are optional.  Note that iov[0] might
 * be a pointer to an "rpc_dg_pkt_hdr_t" or an "rpc_dg_raw_pkt_hdr_t".
 * On a non-MISPACKED_HDR machine, there's no difference between these
 * two structures so there's nothing special we have to do.  On a
 * MISPACKED_HDR machine, we can tell which kind was passed by looking
 * at iov[0].len -- if it's the size of a raw packet header, iov[0].base
 * points to a raw packet header and should be left alone; otherwise
 * it's the non-raw flavor and must be compressed.
 * 
 * The output boolean is set according to the success of sendmsg.
 */

PRIVATE void rpc__dg_xmit_pkt
(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_socket_iovec_p_t iov,
    int iovlen,
    boolean *b
)
{
#ifdef MISPACKED_HDR
    rpc_dg_raw_pkt_hdr_t raw_hdr;
    rpc_dg_pkt_hdr_p_t hdrp;
#endif
    rpc_socket_error_t serr;
    int sendcc = 0;
    int sentcc;
    int i;

#ifdef MISPACKED_HDR
    if (iov[0].len != RPC_C_DG_RAW_PKT_HDR_SIZE)
    {
        /* !!! ...compress hdr pointed to by iov[0] into raw_hdr... !!! */
        hdrp = (iov)[0].base;
        compress_hdr(hdrp, &raw_hdr);
        (iov)[0].iov_base = (caddr_t) &raw_hdr;
    }
#endif

    for (i = 0; i < iovlen; i++)
        sendcc += iov[i].iov_len;

    *b = true;
    RPC_DG_SOCKET_SENDMSG_OOL(sock, iov, iovlen, addr, &sentcc, &serr);
    if (RPC_SOCKET_IS_ERR(serr) || sentcc != sendcc) 
    {
        RPC_DBG_GPRINTF(("(rpc__dg_xmit_pkt) sendmsg failed, sendcc = %d, sentcc = %d, error = %d\n", 
            sendcc, sentcc, RPC_SOCKET_ETOI(serr)));
        *b = false;
    }

    RPC_DG_STATS_INCR(pkts_sent);
    RPC_DG_STATS_INCR(pstats[RPC_DG_HDR_INQ_PTYPE(((rpc_dg_pkt_hdr_p_t) iov[0].iov_base))].sent);

#ifdef MISPACKED_HDR
    iov[0].iov_base = hdrp;
#endif
}


/*
 * R P C _ _ D G _ X M I T _ H D R _ O N L Y _ P K T
 *
 * Send a header only packet of the specified packet type.  Create a
 * new packet header using the provided prototype header.  This is intended
 * as a utility routine for non-critical processing.
 */

PRIVATE void rpc__dg_xmit_hdr_only_pkt
(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_dg_pkt_hdr_p_t hdrp,
    rpc_dg_ptype_t ptype
)
{
    rpc_socket_iovec_t iov[1];
    rpc_dg_pkt_hdr_t hdr;
    boolean b;

    /*
     * Create a pkt header initialized with the prototype's contents.
     */

    hdr = *hdrp;

    RPC_DG_HDR_SET_VERS(&hdr);
    RPC_DG_HDR_SET_PTYPE(&hdr, ptype);
    RPC_DG_HDR_SET_DREP(&hdr);

    hdr.flags       = 0;
    hdr.flags2      = 0;
    hdr.len         = 0;

    /*
     * Setup the iov and send the packet.
     */

    iov[0].iov_base = (byte_p_t) &hdr;
    iov[0].iov_len  = RPC_C_DG_RAW_PKT_HDR_SIZE;

    rpc__dg_xmit_pkt(sock, addr, iov, 1, &b);
}


/*
 * R P C _ _ D G _ X M I T _ E R R O R _ B O D Y _ P K T
 *
 * Transmit a packet that only has a packet header and a error status
 * body.  Create a new packet header using the provided prototype header.
 * This routine deals handles any necessary MISPACKED_HDR processing
 * for the error status body.
 */

PRIVATE void rpc__dg_xmit_error_body_pkt
(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_dg_pkt_hdr_p_t hdrp,
    rpc_dg_ptype_t ptype,
    unsigned32 errst
)
{
    rpc_socket_iovec_t iov[2];
    rpc_dg_pkt_hdr_t hdr;
#ifndef MISPACKED_HDR
    rpc_dg_epkt_body_t body;
#else
    rpc_dg_raw_epkt_body_t body;
#endif
    boolean b;

    /*
     * Create a pkt header initialized with the prototype's contents.
     */

    hdr = *hdrp;

    RPC_DG_HDR_SET_VERS(&hdr);
    RPC_DG_HDR_SET_PTYPE(&hdr, ptype);
    RPC_DG_HDR_SET_DREP(&hdr);

    hdr.flags       = 0;
    hdr.flags2      = 0;
    hdr.len         = RPC_C_DG_RAW_EPKT_BODY_SIZE;


    /*
     * Create the error body packet's body.
     */

#ifndef MISPACKED_HDR
    body.st = errst;
#else
#error "extract and pack the 32 bit status code into the body" /*!!! */
#endif

    /*
     * Setup the iov and send the packet.
     */

    iov[0].iov_base = (byte_p_t) &hdr;
    iov[0].iov_len  = RPC_C_DG_RAW_PKT_HDR_SIZE;
    iov[1].iov_base = (byte_p_t) &body;
    iov[1].iov_len  = hdr.len;

    rpc__dg_xmit_pkt(sock, addr, iov, 2, &b);

    RPC_DBG_GPRINTF(("(rpc__dg_xmit_call_error_body_pkt) \"%s\" - st 0x%x sent\n",
        rpc__dg_pkt_name(ptype), errst));
}


/*
 * R P C _ _ D G _ M G M T _ I N Q _ C A L L S _ S E N T
 *
 */

PRIVATE unsigned32 rpc__dg_mgmt_inq_calls_sent(void)
{
    return (rpc_g_dg_stats.calls_sent);
}


/*
 * R P C _ _ D G _ M G M T _ I N Q _ C A L L S _ R C V D
 *
 */

PRIVATE unsigned32 rpc__dg_mgmt_inq_calls_rcvd(void)
{
    return (rpc_g_dg_stats.calls_rcvd);
}


/*
 * R P C _ _ D G _ M G M T _ I N Q _ P K T S _ S E N T
 *
 */

PRIVATE unsigned32 rpc__dg_mgmt_inq_pkts_sent(void)
{
    return (rpc_g_dg_stats.pkts_sent);
}


/*
 * R P C _ _ D G _ M G M T _ I N Q _ P K T S _ R C V D
 *
 */

PRIVATE unsigned32 rpc__dg_mgmt_inq_pkts_rcvd(void)
{
    return (rpc_g_dg_stats.pkts_rcvd);
}



/*
 * R P C _ _ D G _ S T A T S _ P R I N T
 *
 */

PRIVATE void rpc__dg_stats_print(void)
{
    unsigned16 i;

    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("RPC DG Protocol Statistics\n") );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("--------------------------------------------------------\n") );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Calls sent:            %9lu\n", rpc_g_dg_stats.calls_sent) );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Calls rcvd:            %9lu\n", rpc_g_dg_stats.calls_rcvd) );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Pkts sent:             %9lu\n", rpc_g_dg_stats.pkts_sent) );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Pkts rcvd:             %9lu\n", rpc_g_dg_stats.pkts_rcvd) );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Broadcasts sent:       %9lu\n", rpc_g_dg_stats.brds_sent) );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Dups sent:             %9lu\n", rpc_g_dg_stats.dups_sent) );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Dups rcvd:             %9lu\n", rpc_g_dg_stats.dups_rcvd) );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Out of orders rcvd:    %9lu\n", rpc_g_dg_stats.oo_rcvd) );

    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("\nBreakdown by packet type               sent            rcvd\n") );
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("------------------------------------------------------------------\n") );

    for (i = 0; i <= RPC_C_DG_PT_MAX_TYPE; i++)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	    ("(%02u) %-10s                   %9lu             %9lu\n",
                i, rpc__dg_pkt_name(i),
                rpc_g_dg_stats.pstats[i].sent, 
                rpc_g_dg_stats.pstats[i].rcvd) );
    }
}



/*
 * R P C _ _ D G _ U U I D _ H A S H
 *
 * A significantly faster (though likely not as good) version of dce_uuid_hash() 
 * (this is the NCS 1.5.1 implementation).
 */

PRIVATE unsigned16 rpc__dg_uuid_hash
(
    dce_uuid_p_t uuid
)
{
    unsigned32 *lp = (unsigned32 *) uuid;
    unsigned32 hash = lp[0] ^ lp[1] ^ lp[2] ^ lp[3];

    return ((hash >> 16) ^ hash);
}
