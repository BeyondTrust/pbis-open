/*
 * Copyright (c) 2010 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Portions of this software have been released under the following terms:
 *
 * (c) Copyright 1989-1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989-1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989-1993 DIGITAL EQUIPMENT CORPORATION
 *
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 * permission to use, copy, modify, and distribute this file for any
 * purpose is hereby granted without fee, provided that the above
 * copyright notices and this notice appears in all source code copies,
 * and that none of the names of Open Software Foundation, Inc., Hewlett-
 * Packard Company or Digital Equipment Corporation be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Neither Open Software
 * Foundation, Inc., Hewlett-Packard Company nor Digital
 * Equipment Corporation makes any representations about the suitability
 * of this software for any purpose.
 *
 * Copyright (c) 2007, Novell, Inc. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Novell Inc. nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
/*
**
**  NAME
**
**      cnrcvr.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  The NCA Connection Protocol Service's Receiver Service.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <ndrp.h>       /* System (machine architecture) dependent definitions */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnnet.h>      /* NCA Connection network service */
#include <cnsm.h>       /* NCA Connection state machine service */
#include <cnassm.h>     /* NCA Connection association state machine */
#include <cnclsm.h>     /* NCA Connection call state machine */
#include <cnpkt.h>      /* NCA Connection packet encoding */
#include <cnfbuf.h>     /* NCA Connection fragment buffer service */
#include <cnassoc.h>    /* NCA Connection association service */
#include <cnrcvr.h>     /* NCA Connection receiver service */
#include <comauth.h>    /* Externals for Auth. Services sub-component */
#include <cncall.h>     /* NCA connection call service */
#include <comcthd.h>    /* Externals for call thread services component */
#include <cncthd.h>     /* NCA Connection call executor service */


/******************************************************************************/
/*
 * Internal variables
 */
/******************************************************************************/
/*
 * P A C K E T _ I N F O _ T A B L E
 *
 * Call and Association packet event information table.
 * This table is indexed by packet type.
 */

typedef struct
{
    unsigned8 class;
    unsigned8 event;
} rpc_cn_pkt_info_t, *rpc_cn_pkt_info_p_t;

#define CALL_CLASS_PKT  0       /* packet is a call related packet */
#define ASSOC_CLASS_PKT 1       /* packet is an association related packet */
#define DGRAM_CLASS_PKT 2       /* packet is a datagram related packet (illegal) */


#ifndef RPC_C_DGRAM_TYPE_PKT
#define RPC_C_DGRAM_TYPE_PKT 0xff     /* this is an arbitrary value */
#endif

INTERNAL rpc_cn_pkt_info_t packet_info_table[] =
{
    { CALL_CLASS_PKT,  RPC_C_CALL_RPC_IND },            /* 00 - request */
    { DGRAM_CLASS_PKT, RPC_C_DGRAM_TYPE_PKT },          /* 01 - ping */
    { CALL_CLASS_PKT,  RPC_C_CALL_RPC_CONF },           /* 02 - response */
    { CALL_CLASS_PKT,  RPC_C_CALL_FAULT },              /* 03 - fault */
    { DGRAM_CLASS_PKT, RPC_C_DGRAM_TYPE_PKT },          /* 04 - working */
    { DGRAM_CLASS_PKT, RPC_C_DGRAM_TYPE_PKT },          /* 05 - nocall */
    { DGRAM_CLASS_PKT, RPC_C_DGRAM_TYPE_PKT },          /* 06 - reject */
    { DGRAM_CLASS_PKT, RPC_C_DGRAM_TYPE_PKT },          /* 07 - ack */
    { DGRAM_CLASS_PKT, RPC_C_DGRAM_TYPE_PKT },          /* 08 - quit */
    { DGRAM_CLASS_PKT, RPC_C_DGRAM_TYPE_PKT },          /* 09 - fack */
    { DGRAM_CLASS_PKT, RPC_C_DGRAM_TYPE_PKT },          /* 10 - quack */
    { ASSOC_CLASS_PKT, RPC_C_ASSOC_IND },               /* 11 - bind */
    { ASSOC_CLASS_PKT, RPC_C_ASSOC_ACCEPT_CONF },       /* 12 - bind ack */
    { ASSOC_CLASS_PKT, RPC_C_ASSOC_REJECT_CONF },       /* 13 - bind nak */
    { ASSOC_CLASS_PKT, RPC_C_ASSOC_ALTER_CONTEXT_IND }, /* 14 - alter context */
    { ASSOC_CLASS_PKT, RPC_C_ASSOC_ALTER_CONTEXT_CONF },/* 15 - alter context response */
    { ASSOC_CLASS_PKT, RPC_C_ASSOC_AUTH3_IND },         /* 16 - auth3 */
    { ASSOC_CLASS_PKT, RPC_C_ASSOC_SHUTDOWN_IND },      /* 17 - shutdown */
    { CALL_CLASS_PKT,  RPC_C_CALL_REMOTE_ALERT_IND },   /* 18 - remote alert */
    { CALL_CLASS_PKT,  RPC_C_CALL_ORPHANED }            /* 19 - orphaned */
};


/******************************************************************************/
/*
 * Internal routine declarations
 */
/******************************************************************************/
/*
 * R E C E I V E _ D I S P A T C H
 */
INTERNAL void receive_dispatch (
        rpc_cn_assoc_p_t        /*assoc*/
    );

/*
 * R E C E I V E _ P A C K E T
 */
INTERNAL void receive_packet (
        rpc_cn_assoc_p_t        /*assoc*/,
        rpc_cn_fragbuf_p_t      * /*fragbuf_p*/,
        rpc_cn_fragbuf_p_t      * /*ovf_fragbuf_p*/,
        unsigned32              * /*st*/
    );

/*
 * R P C _ C N _ S E N D _ F A U L T 
 *
 * This macro will cause a fault PDU to be sent back to the client
 * and will terminate the RPC.
 */
#define RPC_CN_SEND_FAULT(call_r, st) \
{\
    rpc_binding_rep_t   *binding_r; \
\
    rpc__cn_call_reject ((rpc_call_rep_p_t) call_r, st);\
    binding_r = (rpc_binding_rep_t *) (call_r)->binding_rep; \
    RPC_CN_UNLOCK (); \
    rpc__cn_call_end ((rpc_call_rep_p_t *) &(call_r), (unsigned32*)&st); \
    RPC_CN_LOCK (); \
    RPC_BINDING_RELEASE (&binding_r, \
                         (unsigned32*)&st); \
}


/*
**++
**  ROUTINE NAME:       rpc__cn_network_receiver
**
**  SCOPE:              PRIVATE - declared in cnrcvr.h
**
**  DESCRIPTION:        
**
**  This routine constitutes the top-level receiver thread (both client and
**  server) and is invoked by "thread create" in "association
**  lookaside alloc" routine to process incoming packets. 
**
**  It receives packets on an association until terminated by a
**  cancel, the connection breaks or a resource exhaustion problem
**  is hit.
**
**  INPUTS:
**
**      assoc             pointer to an association control block
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       Posts events to the association and call state machines.
**
**--
*/

PRIVATE void rpc__cn_network_receiver 
(
  rpc_cn_assoc_p_t        assoc
)
{
    rpc_socket_error_t  serr;
    volatile boolean    done = false;

    //DO_NOT_CLOBBER(done);
 
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_receiver);

    RPC_DBG_PRINTF (rpc_e_dbg_threads, RPC_C_CN_DBG_THREADS,
        ("####### assoc->%x Entered receiver thread \n", assoc));

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: assoc->%x call_rep->none Receiver thread starting...\n",
                     assoc));

    /*
     * Loop until a cancel is sent to this thread.
     */
    while (!done && !assoc->cn_ctlblk.exit_rcvr)
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: assoc->%x call_rep->none Entering receive loop...\n",
                         assoc));

        /*
         * Lock the global connection mutex to prevent other threads
         * from running while the receiver thread is. This mutex will be
         * released when we block (either explicitly or implicitly by a
         * condition variable wait).
         */
        /* XXX is there any advantage to using a per-association mutex? */
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: Attemping to lock global mutex\n"));
        RPC_CN_LOCK ();
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: Global mutex locked\n"));
        
        /*
         * Wait for a session/transport connection to be established.
         */
        DCETHREAD_TRY
        {
            while (assoc->cn_ctlblk.cn_state != RPC_C_CN_OPEN)
            {
		/*
		 * XXX this check is to mask a race condition where the
		 * assoc appears to be freed under us. Of course, we are
		 * in this test relying on the fact that it is zeroed
		 * upon deallocation and the memory isn't overwritten
		 * which is completely bogus.
		 *
		 * Not sure why this is happening as rpc__cn_assoc_acb_free()
		 * should be waiting to join this thread.
		 */
                if (assoc->cn_ctlblk.cn_rcvr_thread_id == (dcethread*)0)
                {
                    done = true;
                    break;
                }
                assoc->cn_ctlblk.cn_rcvr_waiters++;
                RPC_DBG_PRINTF (rpc_e_dbg_threads, RPC_C_CN_DBG_THREADS,
                    ("####### assoc->%x Waiting for new connection \n", assoc));
                DCETHREAD_TRY 
                {
                    RPC_COND_WAIT (assoc->cn_ctlblk.cn_rcvr_cond,
                                   rpc_g_global_mutex);
                }
                DCETHREAD_CATCH(dcethread_interrupt_e)
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                    ("CN: assoc->%x rcvr free'ed by acb_free\n",
                                     assoc));
                    done = true;
                }
                DCETHREAD_CATCH_ALL(THIS_CATCH)
                {
                    /*
                     * rpc_m_unexpected_exc
                     * "(%s) Unexpected exception was raised"
                     */
                    RPC_DCE_SVC_PRINTF ((
                        DCE_SVC(RPC__SVC_HANDLE, "%s"),
                        rpc_svc_recv,
                        svc_c_sev_fatal | svc_c_action_abort,
                        rpc_m_unexpected_exc,
                        "rpc__cn_network_receiver" ));
                }
                DCETHREAD_ENDTRY

                assoc->cn_ctlblk.cn_rcvr_waiters--;

                if (done == true)
                    break;
                RPC_DBG_PRINTF (rpc_e_dbg_threads, RPC_C_CN_DBG_THREADS,
                    ("####### assoc->%x Got a new connection \n", assoc));
            }

            if (done)
            {
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: assoc->%x call_rep->none Receiver awake ... free'ed\n",
                                 assoc));
            }
            else
            {
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: assoc->%x call_rep->none Receiver awake ... Connection established\n",
                                 assoc));
    
                /*
                 * Increment the association control block's reference count since we
                 * are now using it and receive packets as long as the
                 * connection is open. 
                 */
                RPC_CN_ASSOC_ACB_INC_REF (assoc);

                /*
                 * A connection has been established.
                 */
                RPC_CN_STATS_INCR (connections);
                DCETHREAD_TRY
                {
                    receive_dispatch (assoc);
                }
                DCETHREAD_CATCH(dcethread_interrupt_e)
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
("CN: call_rep->%x assoc->%x desc->%x receiver canceled, caught in rpc__cn_network_receiver()\n",
                                    assoc->call_rep,
                                    assoc,
                                    assoc->cn_ctlblk.cn_sock));
                }
                DCETHREAD_CATCH_ALL(THIS_CATCH)
                {
                    /*
                     * rpc_m_unexpected_exc
                     * "(%s) Unexpected exception was raised"
                     */
                   RPC_DCE_SVC_PRINTF ((
                       DCE_SVC(RPC__SVC_HANDLE, "%s"),
                       rpc_svc_recv,
                       svc_c_sev_fatal | svc_c_action_abort,
                       rpc_m_unexpected_exc,
                       "rpc__cn_network_receiver" ));
                }
                DCETHREAD_ENDTRY

                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                        ("CN: assoc->%x call_rep->none No longer receiving...Close socket\n",
                                         assoc));
                /*
                 * Either the connection was broken or another
                 * thread has sent us a cancel indicating the
                 * connection should be broken. In either case
                 * close the socket and set the connection state
                 * to closed.
                 */
                RPC_CN_STATS_INCR (closed_connections);
                serr = RPC_SOCKET_CLOSE (assoc->cn_ctlblk.cn_sock); /* must not be a cancellation point */
                if (RPC_SOCKET_IS_ERR(serr))
                {
                    /*
                     * The socket close failed.
                     */
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
("(rpc__cn_network_receiver) assoc->%x desc->%x RPC_SOCKET_CLOSE failed, error = %d\n", 
                                     assoc,
                                     assoc->cn_ctlblk.cn_sock,                                  
                                     RPC_SOCKET_ETOI(serr)));
                }
                
                assoc->cn_ctlblk.cn_state = RPC_C_CN_CLOSED;

                /*
                 * Remove any pending cancel on this assoc. Otherwise, it's
                 * possible that the receiver thread will see this cancel after
                 * the next call begins.
                 */
                DCETHREAD_TRY
                {
                    dcethread_checkinterrupt();
                }
                DCETHREAD_CATCH_ALL(THIS_CATCH)
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: assoc->%x rcvr cancel found at acb_dealloc\n",
                        assoc));
                }
                DCETHREAD_ENDTRY

                /*
                 * Deallocate the association control block.
                 */
                rpc__cn_assoc_acb_dealloc (assoc);

                /*
                 * Check if rpc__cn_assoc_acb_free() posted the cancel.
                 */
                DCETHREAD_TRY
                {
                    dcethread_checkinterrupt();
                }
                DCETHREAD_CATCH(dcethread_interrupt_e)
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: assoc->%x rcvr free'ed by acb_dealloc\n",
                        assoc));
                    done = true;
                }
                DCETHREAD_CATCH_ALL(THIS_CATCH)
                {
         	    /*
         	     * rpc_m_unexpected_exc
            	     * "(%s) Unexpected exception was raised"
            	     */
        	    RPC_DCE_SVC_PRINTF ((
        	        DCE_SVC(RPC__SVC_HANDLE, "%s"),
        	        rpc_svc_recv,
        	        svc_c_sev_fatal | svc_c_action_abort,
        	        rpc_m_unexpected_exc,
        	        "rpc__cn_network_receiver" ));
                }
                DCETHREAD_ENDTRY
            }
        }
        DCETHREAD_CATCH(dcethread_interrupt_e)
        {
 	    /*
 	     * rpc_m_unexpected_exc
    	     * "(%s) Unexpected exception was raised"
    	     */
	    RPC_DCE_SVC_PRINTF ((
        	DCE_SVC(RPC__SVC_HANDLE, "%s"),
        	rpc_svc_recv,
        	svc_c_sev_fatal | svc_c_action_abort,
        	rpc_m_unexpected_exc,
        	"rpc__cn_network_receiver" ));
        }
        DCETHREAD_CATCH_ALL(THIS_CATCH)
        {
        }
        DCETHREAD_ENDTRY

        /*
         * Unlock the global connection mutex.
         */
        DCETHREAD_TRY
        {
            RPC_CN_UNLOCK ();
        }
        DCETHREAD_CATCH_ALL(THIS_CATCH)
        {
 	    /*
 	     * rpc_m_unexpected_exc
    	     * "(%s) Unexpected exception was raised"
    	     */
	    RPC_DCE_SVC_PRINTF ((
        	DCE_SVC(RPC__SVC_HANDLE, "%s"),
        	rpc_svc_recv,
        	svc_c_sev_fatal | svc_c_action_abort,
        	rpc_m_unexpected_exc,
        	"rpc__cn_network_receiver" ));
        }
        DCETHREAD_ENDTRY
    } /* end while (!done && !assoc->cn_ctlblk.exit_rcvr) */

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: assoc->%x call_rep->none Receiver thread exiting...\n",
                     assoc));
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       receive_dispatch
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  This is the low-level routine for receiving and dispatching packets.
**
**  This routine is called once per "connection" and will continue to
**  receive and dispatch packets until some kind of error is encountered.
**
**  INPUTS:
**
**      assoc           pointer to an association control block
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void receive_dispatch 
(
  rpc_cn_assoc_p_t        assoc
)
{
    volatile rpc_cn_fragbuf_p_t          fragbuf_p;
    volatile rpc_cn_fragbuf_p_t          ovf_fragbuf_p;
    rpc_cn_call_rep_p_t         call_r;
    volatile unsigned32                  st;
    rpc_cn_packet_p_t           pktp;
    unsigned8                   ptype;
    volatile boolean                     unpack_ints = false;
    volatile unsigned32                  i;
    rpc_cn_syntax_t             *pres_context;
    unsigned32                  auth_st;
    rpc_cn_sec_context_t        *sec_context;
    boolean                     already_unpacked;

    //DO_NOT_CLOBBER(unpack_ints);
    //DO_NOT_CLOBBER(i);
	 
    /*
     * Onetime (auto) initialization.
     */
    st = rpc_s_ok;
    fragbuf_p = NULL;
    ovf_fragbuf_p = NULL;
    call_r = NULL;
    sec_context = NULL;

    /*
     * Main receive processing.
     *
     * We loop, receiving and processing packets until some kind of error
     * is encountered.
     */
    for (i = 0;; i++)
    {
        RPC_LOG_CN_PROCESS_PKT_NTR;

        /*
         * Increment the per-association security context next receive
         * sequence number.
         */
        assoc->security.assoc_next_rcv_seq = i;

        /*
         * Receive a packet from the network.
         */
        DCETHREAD_TRY
        {
            receive_packet (assoc,
                            (rpc_cn_fragbuf_p_t*)&fragbuf_p,
                            (rpc_cn_fragbuf_p_t*)&ovf_fragbuf_p,
                            (unsigned32*)&st);
        }
        DCETHREAD_CATCH(dcethread_interrupt_e)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
("CN: call_rep->%x assoc->%x desc->%x receiver canceled, caught in receive_dispatch()\n",
                            assoc->call_rep, 
                            assoc,
                            assoc->cn_ctlblk.cn_sock));
           st = rpc_s_connection_closed;
        }
        DCETHREAD_CATCH_ALL(THIS_CATCH)
        {
	    /*
	     * rpc_m_unexpected_exc
	     * "(%s) Unexpected exception was raised"
	     */
	    RPC_DCE_SVC_PRINTF ((
	        DCE_SVC(RPC__SVC_HANDLE, "%s"),
	        rpc_svc_recv,
	        svc_c_sev_fatal | svc_c_action_abort,
	        rpc_m_unexpected_exc,
	        "receive_dispatch" ));
        }
        DCETHREAD_ENDTRY

        if (st != rpc_s_ok)
        {
            break;
        }

        already_unpacked = false;

        /*
         * Point to the packet header.
         */
        assert(fragbuf_p != NULL);
        pktp = (rpc_cn_packet_p_t) fragbuf_p->data_p;

        /*
         * Trace the incoming packet.
         */
        RPC_CN_PKT_TRC (pktp);
      
        /*
         * Setup some local variables.
         */
        ptype = RPC_CN_PKT_PTYPE (pktp);

	/*
	 * Make sure that we have a valid packet type.
	 * If not, we return an error, and the caller will close
	 * the connection.
	 */        
        if (/* (ptype < 0) ||*/ (ptype > RPC_C_CN_PKT_MAX_TYPE) ||
            (packet_info_table[ptype].class == DGRAM_CLASS_PKT))
        {
            st = rpc_s_protocol_error;
            break;
        }

        RPC_CN_PKT_DUMP (pktp, fragbuf_p->data_size);

        /*
         * Keep some stats on the packets received.
         */
        RPC_CN_STATS_INCR (pstats[RPC_CN_PKT_PTYPE (pktp)].rcvd);
        RPC_CN_STATS_INCR (pkts_rcvd);

        /*
         * Setup some local variables.
         */
        ptype = RPC_CN_PKT_PTYPE (pktp);


        /*
         * Do some first packet only processing...
         */
        if (i == 0)
        {
            /*
             * Stash the remote NDR format away. Also create boolean
             * to determine whether we have to bother unpacking the
             * packet header.
             */
            NDR_UNPACK_DREP (&(RPC_CN_ASSOC_NDR_FORMAT (assoc)),
                             RPC_CN_PKT_DREP (pktp));
            if ((NDR_DREP_INT_REP (RPC_CN_PKT_DREP (pktp)) != 
                 NDR_LOCAL_INT_REP))
            {
                unpack_ints = true;
            }
            else
            {
                unpack_ints = false;
            }
        }
        else
        {
            /*
             * Sanity check the major and minor version numbers.
             * We let the association state machine do this check
             * in the case BIND packets because the
             * protocol calls for a call reject if the versions 
             * do not match at that point.
             * For subsequent packets, we do the check here.
             */
            if (!(ptype == RPC_C_CN_PKT_BIND) &&
                ((RPC_CN_PKT_VERS (pktp) != RPC_C_CN_PROTO_VERS) ||
                (RPC_CN_PKT_VERS_MINOR (pktp) > RPC_C_CN_PROTO_VERS_MINOR)))
            {
                st = rpc_s_rpc_prot_version_mismatch;
                break;
            }
        }

        auth_st = rpc_s_ok;

        /*
         * Determine whether the received PDU contains an
         * authentication trailer.  We don't care about byte
         * ordering in the following macro invocation because
         * the trailer length is compared with zero to determine
         * whether or not the trailer is present.
         */
        if (RPC_CN_PKT_AUTH_TLR_PRESENT (pktp))
        {
            rpc_cn_auth_tlr_t               *auth_tlr;
            unsigned16                      auth_len;
            unsigned32                      key_id;
            unsigned16                      frag_len;

            /*
             * If the pdu is a bind or alter-context pdu and we need to
             * unpack the packet, save the raw form of the pdu so that
             * the checksum can be computed correctly later.  Do this by 
             * allocating a fragbuf and chaining it to the association
             * control block, then copying the packet to it.  The fragbuf
             * will be deallocated in the state machine after invoking
             * recv_check.
             */
            if ((unpack_ints) &&
                ((ptype == RPC_C_CN_PKT_BIND) ||
                 (ptype == RPC_C_CN_PKT_BIND_ACK) ||
                 (ptype == RPC_C_CN_PKT_ALTER_CONTEXT) ||
                 (ptype == RPC_C_CN_PKT_ALTER_CONTEXT_RESP) ||
                 (ptype == RPC_C_CN_PKT_BIND_NAK) ||
                 (ptype == RPC_C_CN_PKT_AUTH3)))
            {
                if (assoc->raw_packet_p)
                {
                    rpc__cn_fragbuf_free(assoc->raw_packet_p);
                }
                assoc->raw_packet_p = rpc__cn_fragbuf_alloc (true);
                assoc->raw_packet_p->data_size = fragbuf_p->data_size;
                memcpy (assoc->raw_packet_p->data_p,
                        fragbuf_p->data_p,
                        fragbuf_p->data_size);
            }

            /*
             * Locate the authentication trailer in the PDU.
             */
            auth_len = RPC_CN_PKT_AUTH_LEN (pktp);
            if (unpack_ints)
            {
                /* no need to check end_of_pkt since its a copy of pkt data */
                SWAB_INPLACE_16 (auth_len);
            }
            
	    auth_tlr = (rpc_cn_auth_tlr_t *) ((unsigned8 *)(pktp) +
                fragbuf_p->data_size - 
                (auth_len + RPC_CN_PKT_SIZEOF_COM_AUTH_TLR));
            if ( ((unsigned8 *)(auth_tlr) < (unsigned8 *)(pktp)) ||
                ((unsigned8 *)(auth_tlr) > (unsigned8 *)(pktp) + fragbuf_p->data_size) ||
                ((unsigned8 *)(auth_tlr) + auth_len < (unsigned8 *)(pktp)) ||
                ((unsigned8 *)(auth_tlr) + auth_len > (unsigned8 *)(pktp) + fragbuf_p->data_size) )
            {
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: call_rep->%p assoc->%p desc->%p invalid auth_tlr\n",
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock));
                st = rpc_s_protocol_error;
                break;
            }

            /*
             * Find the appropriate security context element using the key ID
             * contained in the auth_value part of the trailer.  Also obtain
             * the size of the credentials in the proper format.
             */
            key_id = auth_tlr->key_id;
            if (unpack_ints)
            {
                /* no need to check end_of_pkt since its a copy of pkt data */
                SWAB_INPLACE_32 (key_id);
            }

            if ((ptype != RPC_C_CN_PKT_BIND) && 
                (ptype != RPC_C_CN_PKT_ALTER_CONTEXT) &&
                (ptype != RPC_C_CN_PKT_BIND_ACK) && 
                (ptype != RPC_C_CN_PKT_ALTER_CONTEXT_RESP))
            {
                rpc_authn_protocol_id_t authn_protocol = rpc_c_authn_none;

                rpc__cn_assoc_sec_lkup_by_id (assoc,
                                              key_id,
                                              &sec_context,
                                              &auth_st);
                
                if (auth_st == rpc_s_ok)
                {
                    authn_protocol = RPC_CN_AUTH_CVT_ID_WIRE_TO_API (auth_tlr->auth_type, &auth_st);
                }

                /*
                 * If a security context was located apply the
                 * per-packet security check. Any errors found in either
                 * locating the security context or during the check will
                 * be handled below according to the type of PDU received.
                 */
                if (auth_st == rpc_s_ok)
                {

                    /*
                     * Note that cred_len is zero for all per-message
                     * packets.
                     */
                    RPC_CN_AUTH_RECV_CHECK (authn_protocol,
                                            &assoc->security,
                                            sec_context,
                                            (rpc_cn_common_hdr_t *)pktp,
                                            fragbuf_p->data_size,
                                            0, /* cred_len */
                                            auth_tlr,
                                            unpack_ints,
                                            &auth_st);
                    if (auth_st == rpc_s_ok)
                    {
                        /*
                         * Unpack the header part of the packet.
                         * This will make it easier to remove from the frag
                         * length any padding that was required to
                         * get the auth trailer 4-byte aligned at
                         * the sender.
                         *
                         * Since recv_check may have moved the auth_tlr,
                         * we get the pointer again.
                         */
                        if (unpack_ints)
                        {
                            st = rpc__cn_unpack_hdr (pktp, fragbuf_p->data_size);
                            if (st != rpc_s_ok)
                            {
                                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                                ("CN: call_rep->%p assoc->%p desc->%p auth rpc__cn_unpack_hdr failed\n",
                                                 assoc->call_rep,
                                                 assoc,
                                                 assoc->cn_ctlblk.cn_sock));
                                break;
                            }
                            already_unpacked = true;
                        }
                        auth_len = RPC_CN_PKT_AUTH_LEN (pktp);
                        auth_tlr = (rpc_cn_auth_tlr_t *) ((unsigned8 *)(pktp) +
                                                          fragbuf_p->data_size - 
                                                          (auth_len
                                                           + 
                                                           RPC_CN_PKT_SIZEOF_COM_AUTH_TLR));
                        if ( ((unsigned8 *)(auth_tlr) < (unsigned8 *)(pktp)) ||
                            ((unsigned8 *)(auth_tlr) > (unsigned8 *)(pktp) + fragbuf_p->data_size) ||
                            ((unsigned8 *)(auth_tlr) + auth_len < (unsigned8 *)(pktp)) ||
                            ((unsigned8 *)(auth_tlr) + auth_len > (unsigned8 *)(pktp) + fragbuf_p->data_size) )
                        {
                            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                            ("CN: call_rep->%p assoc->%p desc->%p invalid auth_tlr in sec context\n",
                                             assoc->call_rep,
                                             assoc,
                                             assoc->cn_ctlblk.cn_sock));
                            st = rpc_s_protocol_error;
                            break;
                        }

                        frag_len = RPC_CN_PKT_FRAG_LEN (pktp);
                        if ( (frag_len > fragbuf_p->data_size) || (frag_len < auth_tlr->stub_pad_length) )
                        {
                            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                            ("CN: call_rep->%p assoc->%p desc->%p invalid frag_len\n",
                                             assoc->call_rep,
                                             assoc,
                                             assoc->cn_ctlblk.cn_sock));
                            st = rpc_s_protocol_error;
                            break;
                        }

                        frag_len -= auth_tlr->stub_pad_length;
                        RPC_CN_PKT_FRAG_LEN (pktp) = frag_len;

                        if (fragbuf_p->data_size < auth_tlr->stub_pad_length)
                        {
                            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                            ("CN: call_rep->%p assoc->%p desc->%p invalid stub_pad_length\n",
                                             assoc->call_rep,
                                             assoc,
                                             assoc->cn_ctlblk.cn_sock));
                            st = rpc_s_protocol_error;
                            break;
                        }
                        fragbuf_p->data_size -= auth_tlr->stub_pad_length;
                    }
                    else
                    {
                        /*
                         * Handle any error which occured while performing
                         * either the recv check or key ID lookup. Errors which
                         * occur on the server for a call class PDU will be
                         * handled later by sending a fault PDU back.
                         *
                         * On the client side this error should just be
                         * handed back to the client thread waiting, if
                         * any.
                         * 
                         * On the server side the error should be reflected
                         * back to the client on either a FAULT
                         * PDU if the recv_check failed on a call class
                         * PDU. If it failed on an assoc class PDU then the
                         * best we can probably do is close the association
                         * (it may be possible to respond with a BIND_NAK
                         * if the recv_check failed on a BIND).
                         */
                        if (assoc->assoc_flags & RPC_C_CN_ASSOC_CLIENT)
                        {
                            (*fragbuf_p->fragbuf_dealloc)(fragbuf_p);
                            assert(sec_context != NULL);
                            sec_context->sec_status = auth_st;
                            RPC_CN_ASSOC_WAKEUP (assoc);
                            continue;
                        }
                        else
                        {
                            dce_error_string_t error_text;
                            int temp_status;

                            dce_error_inq_text(auth_st, (unsigned char*) error_text, &temp_status);
                            /*
			     * rpc_m_call_failed_s
			     * "%s on server failed: %s"
			     */
			    RPC_DCE_SVC_PRINTF ((
				DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
				rpc_svc_recv,
				svc_c_sev_error,
				rpc_m_call_failed_s,
				"RPC_CN_AUTH_RECV_CHECK",
				error_text ));

                            if (packet_info_table[ptype].class == ASSOC_CLASS_PKT)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if ((assoc->assoc_flags & RPC_C_CN_ASSOC_CLIENT) &&
                (ptype == RPC_C_CN_PKT_RESPONSE))
            {
                if (assoc->call_rep == NULL)
                {
                    /* The call was abandoned by the sending thread at the same time that
                       a bind ack came back from the server, so bail out */
                    st = rpc_s_connection_closed;
                    break;
                }
                
                if ((RPC_CN_PKT_AUTH_REQUIRED(assoc->call_rep->binding_rep->auth_info)))
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                    ("CN: auth_info %x\n", assoc->call_rep->binding_rep->auth_info));
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                    ("CN: should not continue further with this PDU\n"));
                    st = rpc_s_authn_level_mismatch;
                    RPC_CN_ASSOC_WAKEUP (assoc);
                    break;
                }
            }
        }

        /*
         * Unpack the packet header, if necessary, and check to see
         * if the packet type is within the legal range of values.
         */
        if (unpack_ints && !already_unpacked)
        {
            st = rpc__cn_unpack_hdr (pktp, fragbuf_p->data_size);
            if (st != rpc_s_ok)
            {
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: call_rep->%p assoc->%p desc->%p rpc__cn_unpack_hdr failed\n",
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock));
                st = rpc_s_connection_closed;
                break;
            }
        }

        /*
         * Finally, post the event to the appropriate state machine
         */
        if (packet_info_table[ptype].class == CALL_CLASS_PKT)
        {
            if (assoc->assoc_flags & RPC_C_CN_ASSOC_CLIENT &&
                assoc->alter_call_id >= 0 &&
                assoc->alter_call_id == RPC_CN_PKT_CALL_ID (pktp))
            {
                /* We got a call-level response to a network-level request.
                   Neither the association state machine nor the call
                   state machine are prepared to handle this.  Abort the
                   association */
                RPC_CN_ASSOC_EVAL_NETWORK_EVENT (assoc, 
                                                 RPC_C_ASSOC_ABORT_REQ,
                                                 fragbuf_p,
                                                 st);
            }
            else if ((ptype == RPC_C_CN_PKT_REQUEST) 
                && 
                (RPC_CN_PKT_FLAGS (pktp) & RPC_C_CN_FLAGS_FIRST_FRAG))
            {
                /*
                 * This is the first fragment of a call request packet.
                 * Allocate a call rep and mark it as being a server
                 * call rep.
                 */
                call_r = (rpc_cn_call_rep_t *) 
                    rpc__list_element_alloc (&rpc_g_cn_call_lookaside_list, 
                                             true); 
                if (call_r == NULL)
                {
                    st = rpc_s_no_memory;
                    break;
                }
                call_r->common.is_server = true;
                
                /*
                 * Place the new call rep in the association for
                 * use in cancel processing.
                 */
                rpc__cn_assoc_push_call (assoc, call_r, (unsigned32*)&st); 
                if (st != rpc_s_ok)
                {
                    assoc->call_rep = NULL;
                    rpc__list_element_free (&rpc_g_cn_call_lookaside_list,
                                            (pointer_t) call_r);
                    break;
                }

                /*
                 * Initialize the server call state machine.
                 */
                rpc__cn_sm_init (rpc_g_cn_server_call_sm,
                                 rpc_g_cn_server_call_action_tbl,
                                 &(call_r->call_state),
				 rpc_c_cn_svr_call);

                call_r->num_pkts = 0;
                call_r->sec = sec_context;
                call_r->cn_call_status = rpc_s_ok;
                call_r->last_frag_received = false;
                call_r->call_executed = false;
                call_r->common.u.server.cthread.is_queued = false;
                call_r->prot_tlr = NULL;

                {
                int i;
                for( i=1; i<RPC_C_MAX_IOVEC_LEN; i++ ) {
                    call_r->buffered_output.iov.elt[i].buff_addr = NULL;
                    call_r->buffered_output.iov.elt[i].buff_dealloc = NULL;
                }
                }

                /*
                 * Initialize some cancel state information. 
                 */
                call_r->common.u.server.cancel.accepting = true;
                call_r->common.u.server.cancel.queuing = true;
                call_r->common.u.server.cancel.had_pending = false;
                call_r->common.u.server.cancel.count = 0;
                call_r->u.server.cancel.local_count = 0;

                /*
                 * Allocate a binding rep and put either a nil UUID
                 * or the object UUID in the request packet header in
                 * it, if present.
                 */
                if (RPC_CN_PKT_FLAGS (pktp) & RPC_C_CN_FLAGS_OBJECT_UUID) 
                { 
                    call_r->binding_rep = 
                    rpc__binding_alloc (true, 
                                        &RPC_CN_PKT_OBJECT (pktp), 
                                        RPC_C_PROTOCOL_ID_NCACN, 
                                        NULL, 
                                        (unsigned32*)&st); 
                } 
                else 
                { 
                    call_r->binding_rep = 
                    rpc__binding_alloc (true, 
                                        &uuid_g_nil_uuid, 
                                        RPC_C_PROTOCOL_ID_NCACN, 
                                        NULL, 
                                        (unsigned32*)&st); 
                } 

                /*
                 * If the binding_alloc failed, simply break out of
                 * the loop to close the connection.
                 */
                if (st != rpc_s_ok)
                {
                    break;
                }

                /*
                 * Put the association group id in the binding rep.
                 */
                ((rpc_cn_binding_rep_t *)call_r->binding_rep)->grp_id 
                    = assoc->assoc_grp_id; 
                call_r->assoc = assoc; 

		/*
		 * If auth protection level is rpc_c_protect_level_connect
		 * the request pdu does not include auth tlr and so auth_len
		 * field is set to zero. This disables security context lookup
		 * by its key_id (transferred in auth_tlr).
		 * In such case pass security context from association itself.
		 */
		if (!sec_context)
		{
                    sec_context = assoc->security.assoc_current_sec_context;
		}

                /*
                 * Attach the auth info, if any, to the new binding
                 * rep. Make sure to add a reference to it.
                 */
                if (sec_context)
                {
                    call_r->binding_rep->auth_info = sec_context->sec_info;
                    RPC_CN_AUTH_ADD_REFERENCE(sec_context->sec_info);
                }

                /*
                 * Attach transport info to the bindng rep
                 */
                call_r->binding_rep->transport_info = assoc->transport_info;
                rpc__transport_info_retain(assoc->transport_info);

                /*
                 * Post the event to the call state machine.
                 */
                RPC_CN_POST_FIRST_CALL_SM_EVENT (call_r,
                                                 assoc, 
                                                 packet_info_table[ptype].event, 
                                                 fragbuf_p, 
                                                 st);

                /*
                 * Now that we have a call rep set up and are in the
                 * call_request state see whether the security PDU
                 * receive check or key ID lookup performed earlier in this routine,
                 * if required, passed. If it failed a fault PDU
                 * will be sent back to the client. 
                 */
                if (st != rpc_s_ok)
                {
                    RPC_CN_SEND_FAULT (call_r, st);
                    fragbuf_p = NULL;
                    continue;
                }
                if (auth_st != rpc_s_ok)
                {
                    RPC_CN_SEND_FAULT (call_r, auth_st);
                    fragbuf_p = NULL;
                    continue;
                }

                /*
                 * Get the i/f id and version using the presentation
                 * context id in the header.
                 */
                rpc__cn_assoc_syntax_lkup_by_id (assoc,
                                                 RPC_CN_PKT_PRES_CONT_ID (pktp),
                                                 &pres_context,
                                                 (unsigned32*)&st);
                if (st != rpc_s_ok)
                {
                    RPC_CN_SEND_FAULT (call_r, st);
                    fragbuf_p = NULL;
                    continue;
                }
                
                call_r->u.server.if_id = &pres_context->syntax_abstract_id.id;
                call_r->u.server.if_vers = pres_context->syntax_abstract_id.version;
                call_r->transfer_syntax.index = pres_context->syntax_vector_index;
                call_r->transfer_syntax.convert_epv = pres_context->syntax_epv;
                call_r->u.server.ihint = pres_context->syntax_ihint;
                call_r->context_id = pres_context->syntax_pres_id;

                /*
                 * Invoke a call thread.
                 */
                rpc__cthread_invoke_null ((rpc_call_rep_p_t) call_r,
                                          &call_r->binding_rep->obj,
                                          call_r->u.server.if_id,
                                          call_r->u.server.if_vers,
                                          (unsigned32) call_r->opnum,
                                          rpc__cn_call_executor,
                                          (pointer_t) call_r,
                                          (unsigned32*)&st);
                
                /*
                 * Check whether the call was queued. If it was, just
                 * set the status code to "ok".
                 */
                if (st == rpc_s_call_queued)
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                    ("CN: call_rep->%x assoc->%x desc->%x call queued\n",
                                     call_r,
                                     assoc,
                                     assoc->cn_ctlblk.cn_sock));
                    st = rpc_s_ok;
                }
                else
                {
                    /*
                     * If we get an error back, then we were unable to
                     * create the call thread.  Send a fault PDU.
                     */
                    if (st != rpc_s_ok)
                    {
                        RPC_CN_SEND_FAULT (call_r, st);
                        fragbuf_p = NULL;
                        continue;
                    }
                }

                /*
                 * Keep some stats on the number of calls received.
                 */
                RPC_CN_STATS_INCR (calls_rcvd);
            }
            else
            {
                RPC_CN_POST_CALL_SM_EVENT (assoc, 
                                           packet_info_table[ptype].event, 
                                           fragbuf_p,
                                           st);
            }
        }
        else 
        {
            RPC_CN_ASSOC_EVAL_NETWORK_EVENT (assoc, 
                                             packet_info_table[ptype].event, 
                                             fragbuf_p,
                                             st);
        }

        /*
         * Unlock the CN global mutex, yield the processor to allow
         * another thread to run and re-acquire the CN global mutex.
         */
        RPC_CN_UNLOCK ();
        dcethread_yield ();
        RPC_CN_LOCK ();

        /*
         * NULL our pointer to the fragment buffer so we'll be forced to get
         * a new one.
         */
        fragbuf_p = NULL;
        RPC_LOG_CN_PROCESS_PKT_XIT;
    } /* end-for (i = 0;; i++) */

    /*
     * Some failure occured while receiving packets. Indicate this
     * failure to the association state machine.
     */
    rpc__cn_assoc_post_error (assoc, st);

    /*
     * If we still have a fragbufs, then deallocate them.
     */
    if (fragbuf_p)
    {
        (*fragbuf_p->fragbuf_dealloc)(fragbuf_p);
    }
    if (ovf_fragbuf_p)
    {
        (*ovf_fragbuf_p->fragbuf_dealloc)(ovf_fragbuf_p);
    }
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       receive_packet
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  This is a (large) wrapper around the socket recvmsg() routine.
**
**  We do this since stream sockets don't guarantee the preservation of RPC
**  packet boundaries.  If we receive partial packets, we have to piece them
**  back together again.  Also, if we receive more than one packet during a
**  read operation, we preserve the excess bytes read and return them the next
**  time we are called.
**
**  INPUTS:
**
**      assoc           pointer to an association control block
**
**  INPUTS/OUTPUTS:
**
**      fragbuf_p       pointer to a fragbuf pointer
**      ovf_fragbuf_p   pointer to a overflow fragbuf pointer
**
**  OUTPUTS:            
**
**      st              the return status of this routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void receive_packet 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_cn_fragbuf_p_t      *fragbuf_p,
  rpc_cn_fragbuf_p_t      *ovf_fragbuf_p,
  unsigned32              *st
)
{
    rpc_cn_fragbuf_t    * volatile fbp;
    volatile unsigned16 frag_length;
    volatile int                 bytes_rcvd = 0;
    rpc_socket_iovec_t  iov;
    volatile rpc_socket_error_t  serr = 0;
    signed32            need_bytes;
    static rpc_addr_p_t addr = NULL;

#if 0
    /* Quit for valgrind report output */
    static int count = 0;

    if (count++ > 60000)
    {
        _exit(0);
    }
#endif

    //DO_NOT_CLOBBER(fbp);
    //DO_NOT_CLOBBER(frag_length);
	 
    RPC_LOG_CN_RCV_PKT_NTR;
    CODING_ERROR (st);
    
    /*
     * One time (auto) initialization.
     */
    fbp = NULL;

    /*
     * Assume the worst is going to happen and zap our return value.
     */
    *fragbuf_p = NULL;

    /*
     * If we have a left over fragment buffer (overflow), then use it.
     */
    if (*ovf_fragbuf_p != NULL)
    {
        fbp = *ovf_fragbuf_p;
        *ovf_fragbuf_p = NULL;
    }

    /*
     * If we need a fragment buffer, then allocate one.
     */
    if (fbp == NULL)
    {
        fbp = rpc__cn_fragbuf_alloc (true);
    }

    /*
     ******************************************************************
     * At this point we must determine which of the following 3
     * situations we are faced with:
     *
     *  1) The fragbuf is empty.
     *  2) The fragbuf contains a partial RPC packet.
     *  3) The fragbuf contains a complete RPC packet.
     *
     * The first thing we do is to check whether or not we have
     * enough data in the current fragbuf to determine the complete
     * RPC packet length.  Note that in the 3rd case, the fragbuf may
     * actually contain more than one RPC packet.
     *
     ******************************************************************
     */

    /*
     * Do we have enough data in the fragbuf to determine the RPC packet
     * length?
     *
     * If we do, we can figure out exactly how many bytes to request on our
     * next read. If we don't, we just ask for the maximum number of bytes
     * that our fragbuf will hold.
     */
    if (fbp->data_size >= RPC_C_CN_FRAGLEN_HEADER_BYTES)
    {
        /*
         * Okay, we have enough of the header to figure out how big
         * this fragment is.
         */
        frag_length = RPC_CN_PKT_FRAG_LEN ((rpc_cn_packet_p_t)(fbp->data_p));

        /*
         * Swap bytes if our integer formats are different.
         */
        if (NDR_DREP_INT_REP(RPC_CN_PKT_DREP((rpc_cn_packet_p_t)fbp->data_p)) 
            != NDR_LOCAL_INT_REP)
        {
            SWAB_INPLACE_16 (frag_length);
        }

        /*
         * Figure out how many bytes we need.
         */
        need_bytes = frag_length - fbp->data_size;
    }
    else
    {
        /*
         * We don't have enough data to compute the fragment length.
         */
        frag_length = 0;

        /*
         * Ask for as many bytes as our fragbuf will hold.
         */
        need_bytes = fbp->max_data_size - fbp->data_size;
    }

    /*
     * Read from the socket until we've read at least one entire packet.
     */
    while (need_bytes > 0)
    {

        /*
         * Initialize our iovector.
         */
        iov.iov_base = (byte_p_t)((unsigned8 *)(fbp->data_p) + fbp->data_size);
        iov.iov_len = need_bytes;

        /*
         * Release the CN global mutex before reading from the
         * socket. This will allow other threads to run if we have to
         * block here.
         */
        RPC_CN_UNLOCK ();
        DCETHREAD_TRY
        {
#ifdef NON_CANCELLABLE_IO
            /*
             * By POSIX definition dcethread_enableasync_throw is not a "cancel
             * point" because it must return an error status and an errno.
             * dcethread_enableasync_throw(1) will not deliver
             * a pending cancel nor will the cancel be delivered asynchronously,
             * thus the need for dcethread_checkinterrupt.
             */
	    dcethread_enableasync_throw(1);
	    dcethread_checkinterrupt();
#endif /* NON_CANCELLABLE_IO */
            serr = rpc__socket_recvmsg(
                assoc->cn_ctlblk.cn_sock,
                &iov,
                1,
                addr,
                (int*) &bytes_rcvd);
#ifdef NON_CANCELLABLE_IO
	    dcethread_enableasync_throw(0);
#endif /* NON_CANCELLABLE_IO */
        }
        DCETHREAD_CATCH (dcethread_interrupt_e)
        {
#ifdef NON_CANCELLABLE_IO
            dcethread_enableasync_throw(0);
#endif /* NON_CANCELLABLE_IO */
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
("CN: call_rep->%x assoc->%x desc->%x receiver canceled, caught in receive_packet()\n",
                             assoc->call_rep, 
                             assoc,
                             assoc->cn_ctlblk.cn_sock));

            /*
             * Re-acquire the CN global mutex and free any fragment
             * buffers we have outstanding.
             */
            RPC_CN_LOCK ();
            if (fbp)
            {
                (*(fbp)->fragbuf_dealloc)(fbp);
            }
            DCETHREAD_RERAISE;
        }
        DCETHREAD_CATCH_ALL(THIS_CATCH)
        {
	    /*
	     * rpc_m_unexpected_exc
	     * "(%s) Unexpected exception was raised"
	     */
	    RPC_DCE_SVC_PRINTF ((
	        DCE_SVC(RPC__SVC_HANDLE, "%s"),
	        rpc_svc_recv,
	        svc_c_sev_fatal | svc_c_action_abort,
	        rpc_m_unexpected_exc,
	        "receive_packet" ));
        }
        DCETHREAD_ENDTRY

        /*
         * Re-acquire the CN global mutex.
         */
        RPC_CN_LOCK ();

        /*
         * Hold off on processing the packet if the sending thread for this
         * connection is currently in a sendmsg.
         */
        while (assoc->cn_ctlblk.in_sendmsg)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                            ("CN: call_rep->%x assoc->%x desc->%x waiting for sendmsg to complete...\n", 
                             assoc->call_rep, 
                             assoc,
                             assoc->cn_ctlblk.cn_sock,
                             bytes_rcvd));
            assoc->cn_ctlblk.waiting_for_sendmsg_complete = true;
            RPC_COND_WAIT (assoc->cn_ctlblk.cn_rcvr_cond,
                           rpc_g_global_mutex);
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                            ("CN: call_rep->%x assoc->%x desc->%x sendmsg complete\n", 
                             assoc->call_rep, 
                             assoc,
                             assoc->cn_ctlblk.cn_sock,
                             bytes_rcvd));
            assoc->cn_ctlblk.waiting_for_sendmsg_complete = false;
        }
        
        /*
         * Process any errors reading the socket or any errors detected by 
         * other threads using this association. These other threads will have
         * cleaned up the assoc state and the receiver thread just has to close
         * the connection now.
         */
        if ((RPC_SOCKET_IS_ERR (serr))
            ||
            (bytes_rcvd == 0)
            ||
            (assoc->assoc_local_status != rpc_s_ok)
            ||
            (assoc->assoc_status != rpc_s_ok))
        {
            /*
             * Make sure the connection is really closed. It could
             * be a zero length sequenced packet socket packet.
             */
            if (rpc__naf_is_connect_closed (assoc->cn_ctlblk.cn_sock, st))
            {
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: call_rep->%x assoc->%x desc->%x connection closed recvmsg failed serr = %x, bytes_rcvd = %d\n",
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock, 
                                 serr,
                                 bytes_rcvd));
                
                /*
                 * Deallocate the fragbuf which we were using.
                 */
                (*fbp->fragbuf_dealloc)(fbp);
                
                *st = rpc_s_connection_closed;
                return;
            }
        }

        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: call_rep->%x assoc->%x desc->%x received %d bytes\n", 
                         assoc->call_rep, 
                         assoc,
                         assoc->cn_ctlblk.cn_sock,
                         bytes_rcvd));

        /*
         * Update the number of bytes received.
         */
        fbp->data_size += bytes_rcvd;

        /*
         * If we don't already know the fragment length, then we now have to go
         * through the same gymnastics that we did before vis a vis determining
         * whether or not we've read enough of this fragment to determine the
         * complete fragment length.
         */
        if ((frag_length == 0) && (fbp->data_size >= RPC_C_CN_FRAGLEN_HEADER_BYTES))
        {
            /*
             * We don't know the fragment length and we have enough bytes to
             * determine it so compute the fragment length.
             */
            frag_length = RPC_CN_PKT_FRAG_LEN ((rpc_cn_packet_p_t)(fbp->data_p));

            /*
             * Swap bytes if our integer formats are different.
             */
            if (NDR_DREP_INT_REP( RPC_CN_PKT_DREP((rpc_cn_packet_p_t)fbp->data_p) ) != NDR_LOCAL_INT_REP)
            {
                SWAB_INPLACE_16 (frag_length);
            }

            /*
             * Sanity check the protocol versions in the header.
             * Except for BIND and BIND_NAK packets.
             */
            {
                unsigned32 ptype;
                ptype = RPC_CN_PKT_PTYPE((rpc_cn_packet_p_t)fbp->data_p);

                if ((ptype != RPC_C_CN_PKT_BIND) &&
                    (ptype != RPC_C_CN_PKT_BIND_NAK) &&
                    ((RPC_CN_PKT_VERS ((rpc_cn_packet_p_t)fbp->data_p) != RPC_C_CN_PROTO_VERS) ||
                     (RPC_CN_PKT_VERS_MINOR ((rpc_cn_packet_p_t)fbp->data_p) != assoc->assoc_vers_minor)))
                {
                    /*
                     * We have incompatible protocol versions.
		     */
		    /*
		     * "(receive_packet) assoc->%x %s: Protocol version mismatch -
		     *            major->%x minor->%x"
                     */
		    RPC_DCE_SVC_PRINTF ((
			DCE_SVC(RPC__SVC_HANDLE, "%x%s%x%x"),
			rpc_svc_cn_pkt,
			svc_c_sev_warning,
			rpc_m_prot_mismatch,
			assoc,
			rpc__cn_pkt_name(ptype),
			RPC_CN_PKT_VERS ((rpc_cn_packet_p_t)fbp->data_p),
			RPC_CN_PKT_VERS_MINOR ((rpc_cn_packet_p_t)fbp->data_p) ));
                }
            }
            
            /*
             * Sanity check the fragment size.
             * Signal an error if this fragment is bigger than a fragbuf.
             */
            if (frag_length > rpc_g_cn_large_frag_size)
            {
                unsigned32 ptype;
                ptype = RPC_CN_PKT_PTYPE((rpc_cn_packet_p_t)fbp->data_p);

                /*
		 * "(receive_packet) assoc->%x frag_length %d in header >
		 *                fragbuf data size %d"
		 */
                RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%x%d%d"),
		    rpc_svc_cn_pkt,
		    svc_c_sev_warning,
		    rpc_m_frag_toobig,
                    assoc,
                    frag_length, 
                    rpc_g_cn_large_frag_size ));

                /*
                 * BIND and ALTER_CONTEXT are allowed to be larger
                 */
                if ((ptype == RPC_C_CN_PKT_BIND) ||
                    (ptype == RPC_C_CN_PKT_ALTER_CONTEXT))
                {
                    rpc_cn_fragbuf_t * volatile tmp;

                    tmp = rpc__cn_fragbuf_alloc_dyn(frag_length);
                    tmp->data_size = fbp->data_size;
                    memcpy(tmp->data_p, fbp->data_p, fbp->data_size);

                    (*fbp->fragbuf_dealloc)(fbp);
                    fbp = tmp;
                }
                else
                {
                   *st = rpc_s_protocol_error;

                   /*
                    * Deallocate the fragbuf which we were using.
                    */
                   (*fbp->fragbuf_dealloc)(fbp);
                   return;
                }
            }
        }

        /*
         * Recalculate how many bytes we need to complete this fragment.
         */
        if (frag_length == 0)
        {
            need_bytes = fbp->max_data_size - fbp->data_size;
        }
        else
        {
            need_bytes = frag_length - fbp->data_size;
        }
    } /* end of while(need_bytes > 0) */

    /*
     * Handle the situation where we have read too much data.
     * This means that we have read into the next packet.
     * So, get an "overflow" fragment buffer and copy the excess data into it.
     */

    /*
     * There is room for some optimization here.  If we have enough bytes to
     * determine the length of the next packet and the whole thing will fit into
     * a small fragbuf, then we should probably use a small one.  For now, we're
     * going to apply the KISS principle.
     */
    if (need_bytes < 0)
    {
        /*
         * Get an overflow fragment buffer.
         */
        *ovf_fragbuf_p = rpc__cn_fragbuf_alloc (true);
        (*ovf_fragbuf_p)->data_size = abs(need_bytes);

        /*
         * Set the fragbuf data size to the fragment length and copy the
         * excess data to the overflow fragment buffer.
         */
        fbp->data_size = frag_length;
        memcpy ((*ovf_fragbuf_p)->data_p,
                (pointer_t)((unsigned8 *)(fbp->data_p) + fbp->data_size),
                (*ovf_fragbuf_p)->data_size);
    }

    /*
     * Return a pointer to the just-received packet along with okay status.
     */
    *fragbuf_p = fbp;
    *st = rpc_s_ok;
    RPC_LOG_CN_RCV_PKT_XIT;
}

