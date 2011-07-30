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
**  NAME
**
**      cnassoc.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  The NCA Connection Protocol Service's Association Service.
**
**
 */

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnid.h>       /* NCA Connection local ID service */
#include <cnrcvr.h>     /* NCA Connection receiver thread */
#include <cnnet.h>      /* NCA Connection network service */
#include <cnpkt.h>	/* NCA Connection packet encoding */
#include <cnsm.h>       /* NCA Connection state machine service */
#include <cnassm.h>     /* NCA Connection association state machine */
#include <cnasgsm.h>    /* NCA Connection association group state machine */
#include <comauth.h>    /* Externals for Auth. Services sub-component   */
#include <cncall.h>     /* NCA connection call service */
#include <cnassoc.h>    

#include <dce/rpcexc.h>
#include <syslog.h>

#include <lwio/lwio.h>



/******************************************************************************/
/*
 * Internal variables
 */
/******************************************************************************/
/*
 * We want to serialize the group create operation.  We do this so
 * that we do not wind up with multiple groups if multiple threads 
 * from a client make RPCs to the same server.  This would be especially
 * painful if there are context handles since each of these groups will
 * have to maintain an association.  (If all the associations were in
 * a single group, only one association needs to be maintained.)
 *
 * The following variables simulate a mutex around the group
 * create operation.  We call this the grp_new mutex.  We do not
 * use a real mutex because mutex waits are non-cancellable.  We
 * want this operation to be cancellable to support cancel timeouts
 * even when there is no association.
 *
 * To acquire the grp_new mutex, a thread must set grp_new_in_progress to
 * true. A thread must acquire this mutex prior to creating a group.  
 * If you have nested calls to assoc_request, only the outermost
 * [initial] call to assoc_request should acquire the grp_new mutex.
 *
 * To release the grp_new mutex, a thread should clear grp_new_in_progress
 * and do a condition broadcast on grp_new_wt.  Only the call frame
 * that acquired the grp_new mutex should release it.
 *
 * grp_new_waiters is an optimization.  It is the number of threads 
 * waiting for the grp_new mutex.  A thread must
 * increment this prior to waiting for the grp_new mutex.
 * We don't signal the grp_new_wt condition variable if 
 * there are no waiters when the grp_new mutex is released.
 */
INTERNAL rpc_cond_t             grp_new_wt;
INTERNAL unsigned16             grp_new_waiters;
INTERNAL boolean32              grp_new_in_progress;


/******************************************************************************/
/*
 * Internal routine declarations
 */
/******************************************************************************/
/*
 * R P C _ _ C N _ A S S O C _ O P E N
 */

INTERNAL void rpc__cn_assoc_open (
    rpc_cn_assoc_p_t             /*assoc*/,
    rpc_addr_p_t                 /*rpc_addr*/,
    rpc_if_rep_p_t               /*if_r*/,
    rpc_cn_local_id_t            /*grp_id*/,
    rpc_auth_info_p_t            /*auth_info*/,
    rpc_transport_info_p_t       /*transport_info*/,
    rpc_transfer_syntax_t       * /*syntax*/,
    unsigned16                  * /*context_id*/,
    rpc_cn_sec_context_p_t      * /*sec*/,
    unsigned32                  * /*st*/);

/*
 * R P C _ _ C N _ A S S O C _ A L T E R _ C O N T E X T
 */

INTERNAL void rpc__cn_assoc_alter_context (
    rpc_cn_assoc_p_t             /*assoc*/,
    rpc_if_rep_p_t               /*if_r*/,
    rpc_auth_info_p_t            /*info*/,
    rpc_transfer_syntax_t       * /*syntax*/,
    unsigned16                  * /*context_id*/,
    rpc_cn_sec_context_p_t      * /*sec*/,
    unsigned32                  * /*st*/);

/*
 * R P C _ _ C N _ A S S O C _ R E C L A I M
 */

INTERNAL void rpc__cn_assoc_reclaim (
    rpc_cn_local_id_t            /*grp_id*/,
    unsigned32                   /*type*/,
    boolean32			 /*loop*/);

/*
 * R P C _ _ C N _ A S S O C _ T I M E R _ R E C L A I M
 */

INTERNAL void rpc__cn_assoc_timer_reclaim (
    pointer_t                   /*type*/);

/*
 * R P C _ _ C N _ A S S O C _ A C B _ A L L O C
 */

INTERNAL rpc_cn_assoc_t *rpc__cn_assoc_acb_alloc (
    boolean32                    /*wait*/,
    unsigned32                   /*type*/,
    unsigned32                  * /*st*/);

/*
 * R P C _ _ C N _ A S S O C _ G R P _ I N I T
 */

INTERNAL void rpc__cn_assoc_grp_init (
    rpc_cn_assoc_grp_p_t         /*assoc_grp*/,
    unsigned32                   /*index*/);

/*
 * R P C _ _ C N _ A S S O C _ G R P _ C R E A T E 
 */

INTERNAL rpc_cn_local_id_t rpc__cn_assoc_grp_create (
    unsigned32                  * /*st*/);

/*
 * R P C _ _ C N _ A S S O C _ S Y N T A X _ A L L O C
 */

INTERNAL rpc_cn_syntax_t *rpc__cn_assoc_syntax_alloc (
    rpc_if_rep_p_t               /*if_r*/,
    unsigned32                  * /*st*/);


/******************************************************************************/
/*
 * Macros
 */
/******************************************************************************/


/******************************************************************************/
/*
 * Local defines
 */
/******************************************************************************/

/*
 * The number of association groups to allocate when the table needs
 * to grow.
 */
#define RPC_C_ASSOC_GRP_ALLOC_SIZE              10

/*
 * The default number of associations allowed on an association
 * group.
 */
#define RPC_C_ASSOC_GRP_MAX_ASSOCS_DEFAULT      0xffffffff

/*
 * The maximum resource wait in seconds. This is the maximum amount
 * of time trying to request an association. This value is specified
 * in Appendix A of the NCA connection architecture spec.
 */
#define RPC_C_ASSOC_MAX_RESOURCE_WAIT           300

/*
 * The client idle connection disconnect time in seconds.
 * This value is specified in Appendix A of the NCA connection
 * architecture spec. 
 */
#define RPC_C_ASSOC_CLIENT_DISC_TIMER           300

/*
 * The server idle connection disconnect time in seconds.
 * This value is specified in Appendix A of the NCA connection
 * architecture spec. 
 */
#define RPC_C_ASSOC_SERVER_DISC_TIMER           300

/*
 * The initial amount of time in seconds to wait before retrying an
 * association request. This corresponds to the slot time in the
 * exponential backoff algorithm used.
 */
#define RPC_C_ASSOC_INITIAL_WAIT_INTERVAL       1

/*
 * The maximum amount of time in seconds after each connection
 * request attempt.
 */
#define RPC_C_ASSOC_MAX_WAIT_INTERVAL           5


/******************************************************************************/
/*
 * Routine definitions
 */
/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_request
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine is called by the client call state machine
**  when an association is needed to perform an RPC over. The
**  returned association will contain a connection to the
**  desired address space as will have had the full presentation
**  negotiation performed over it.
**
**  INPUTS:             
**
**      call_r          The call rep for the RPC.
**      binding_r       The binding rep containing the server
**                      address space address.
**      if_r            The interface rep through which this RPC is
**                      being made containing the presentation
**                      negotiation information.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      syntax          The negotiated transfer syntax.
**      context_id      The context id of the transfer syntax
**                      assigned by the client runtime.
**      sec             The pointer to the negotiated security
**                      context element, NULL if none.
**      st              The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_tsyntaxes_unsupported
**                      rpc_s_unknown_if
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      return          The association. NULL if not created.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_cn_assoc_t *rpc__cn_assoc_request 
(
  rpc_cn_call_rep_p_t     call_r,
  rpc_cn_binding_rep_p_t  binding_r,
  rpc_if_rep_p_t          if_r,
  rpc_transfer_syntax_t   *syntax,
  unsigned16              *context_id,
  rpc_cn_sec_context_p_t  *sec,
  unsigned32              *st
)
{
    rpc_cn_assoc_t      * volatile assoc = NULL;
    rpc_cn_assoc_grp_t  * volatile assoc_grp = NULL;
    rpc_addr_p_t        rpc_addr;
    unsigned32 volatile wait_interval, total_wait;
    rpc_cn_local_id_t   grp_id;
    rpc_cn_local_id_t volatile rem_grp_id;
    struct timespec     timespec;
    struct timespec     abstime;

    /*
     * i_hold_grp_new_mutex means that this call frame holds the 
     * grp_new mutex. The grp_new_mutex is described above.
     */
    volatile boolean    i_hold_grp_new_mutex;

    volatile boolean32  retry_op;
    volatile boolean    old_server = true; /* MS servers are 5.0 servers */
    unsigned32          temp_st;

    //DO_NOT_CLOBBER(assoc);
    //DO_NOT_CLOBBER(assoc_grp);
    //DO_NOT_CLOBBER(wait_interval);
    //DO_NOT_CLOBBER(total_wait);
    //DO_NOT_CLOBBER(rem_grp_id);
    //DO_NOT_CLOBBER(i_hold_grp_new_mutex);
    //DO_NOT_CLOBBER(old_server);
 
    RPC_LOG_CN_ASSOC_REQ_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_request);
    CODING_ERROR (st);

    /*
     * The CN RPC protocol looks at the binding timeout as a binary
     * value. If the timeout is infinite it will try forever to
     * establish a connection. If not it will try once and rely on the
     * transport protocol below to perform the appropriate retries.
     * It would be nice if we could turn some generic transport knob
     * to tell it how hard to try but as of today there is no such facility.
     */
    wait_interval = RPC_C_ASSOC_INITIAL_WAIT_INTERVAL;
    i_hold_grp_new_mutex = false;
    total_wait = 0;

    while (true)
    {
        /*
         * Ideally we'd like to use an association which already exists,
         * is open, and has already had the transfer syntax negotiated for
         * the abstract syntax (interface UUID and version) given. To
         * find a cached association we must first find an association
         * group representing the server address space given in the
         * binding rep. There are two ways of finding an association
         * group. First we can use the group ID contained in the
         * connection part of the binding rep. This will get us directly
         * to an association group. If that doesn't work we can use the
         * RPC address contained in the common part of the binding rep.
         * This means we have to scan and compare possibly all the
         * association groups.
         */
        grp_id = rpc__cn_assoc_grp_lkup_by_id (binding_r->grp_id,
                                               RPC_C_CN_ASSOC_GRP_CLIENT,
                                               binding_r->common.transport_info,
                                               st);
        if (*st != rpc_s_ok)
        {
            /*
             * The assoc group was not found using the group ID in the
             * binding rep. Now try the RPC address in the binding rep.
             */
            grp_id = rpc__cn_assoc_grp_lkup_by_addr (binding_r->common.rpc_addr, 
                                                     binding_r->common.transport_info,
                                                     RPC_C_CN_ASSOC_GRP_CLIENT, 
                                                     st);
        }
        
        /*
         * Check whether an association group was found.
         */
        assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
        if (assoc_grp != NULL)
        {
            /*
             * We now have an association group. Save the group id in the
             * binding rep for use on subsequent lookups and look for an open
             * association on the group.
             */
            binding_r->grp_id.all = assoc_grp->grp_id.all;
            RPC_LIST_FIRST (assoc_grp->grp_assoc_list, 
                            assoc,
                            rpc_cn_assoc_p_t);            

            /*
             * Cycle through all the associations on this group
             * looking for an unallocated one.
             */
            while (assoc != NULL)
            {
                /*
                 * Look at the reference count of the association to determine
                 * whether it is allocated.
                 */
                if (assoc->assoc_ref_count == 0)
                {
                    /*
                     * Store the call rep in the association for use
                     * in cancel timeout processing.
                     */
                    assoc->call_rep = call_r;

                    /*
                     * Allocate the association.
                     * Send an association allocate request through
                     * the association state machine. This will put
                     * the association into the active state and
                     * will prevent other threads from allocating it.
                     */
                    rpc__cn_assoc_alloc (assoc, st);
                    if (*st == rpc_s_ok)
                    {
                        /*
                         * This association is active. Send an alter
                         * presentation context event through the
                         * association state machine if necessary.
                         */
                        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                        ("CN: call_rep->%x assoc->%x desc->%x negotiating presentation syntax over existing association\n",
                                         assoc->call_rep,
                                         assoc,
                                         assoc->cn_ctlblk.cn_sock));
                        rpc__cn_assoc_alter_context (assoc, 
                                                     if_r, 
                                                     binding_r->common.auth_info,
                                                     syntax,
                                                     context_id, 
                                                     sec,
                                                     st);
                        
                        RPC_LOG_CN_ASSOC_REQ_XIT;
                        if (*st != rpc_s_ok)
                        {
                            /*
                             * The alter context request failed. 
                             * Just deallocate the association and
                             * return NULL.
                             */
                            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                            ("CN: call_rep->%x assoc->%x desc->%x presentation negotiation failed st = %x\n",
                                             assoc->call_rep,
                                             assoc,
                                             assoc->cn_ctlblk.cn_sock,
                                             *st));
                            
                            /* 
                             * The call is about to be orphaned, so remove it from the assoc
                             */
                            if (assoc->call_rep == call_r)
                            {
                                assoc->call_rep = NULL;
                            }
                            
                            rpc__cn_assoc_dealloc (assoc, 
                                                   call_r,
                                                   &temp_st);
                            return (NULL);
                        }
                        else
                        {
                            /*
                             * The alter context request succeeded.
                             * Return the association.
                             */
                            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                            ("CN: call_rep->%x assoc->%x desc->%x presentation negotiation succeeded\n",
                                             assoc->call_rep,
                                             assoc,
                                             assoc->cn_ctlblk.cn_sock));

                            /*
                             * Update transport information on binding handle
                             */
                            
                            rpc__transport_info_release(binding_r->common.transport_info);
                            binding_r->common.transport_info = assoc->transport_info;
                            rpc__transport_info_retain(binding_r->common.transport_info);

                            return (assoc);
                        }
                    }
                }
                RPC_LIST_NEXT (assoc, assoc, rpc_cn_assoc_p_t);
            } /* while (assoc != NULL) */

            /*
             * Remember the secondary address for this group we're
             * going to need it below to establish a new
             * session/transport connection.
             */
            rpc_addr = assoc_grp->grp_secaddr;

            /*
             * Remember the remote group id for this group. It will
             * be sent to the server when the association is requested.
             */
            rem_grp_id = assoc_grp->grp_remid;
        } /* end if (assoc_grp != NULL) */

        else /* assoc_grp == NULL */
        {

            /*
             * Use the address in the binding rep to establish the
             * new session/transport connection.
             */
            rpc_addr = binding_r->common.rpc_addr;
            RPC_CN_LOCAL_ID_CLEAR (rem_grp_id);

            /*
             * We don't have an association group at this point.
             * We want to serialize the creation of association
             * groups to prevent multiple association
             * groups to the same server address space.
             * We therefore acquire the grp_new mutex.
             *
             * Note: we can still get multiple association
             * groups to the same address space if we have
             * recursive calls to assoc_request.  We can also
             * get them if multiple threads from the same client
             * use different protocol sequences to call the same
             * server.  But we don't expect either of these to be
             * frequent occurences.
             */

            /*
             * Attempt to acquire the grp_new mutex.
             */
            if (grp_new_in_progress)
            {
                /*
                 * Some other thread holds the grp_new mutex. We'll
                 * have to record ourself as a waiter for it and be
                 * patient.
                 */
                grp_new_waiters++;

                while (grp_new_in_progress)
                {
                    /*
                     * Since this is a cancellable operation we'll set
                     * up an exception handler.
                     */
                    retry_op = true;
                    DCETHREAD_TRY
                    {
                        RPC_COND_WAIT (grp_new_wt,
					   rpc_g_global_mutex);
                    }
                    DCETHREAD_CATCH (dcethread_interrupt_e)
                    {
                        /*
                         * Record the fact that a cancel has been
                         * detected. Also, start a timer if this is
                         * the first cancel detected.
                         */
                        rpc__cn_call_local_cancel (call_r,
                                                   &retry_op, 
                                                   st);
                        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                                        ("(rpc__cn_assoc_request) call_rep->%x assoc->%x desc->%x cancel caught before association setup\n",
                                         call_r,
                                         assoc,
                                         assoc->cn_ctlblk.cn_sock));
                    }
                    DCETHREAD_ENDTRY

                    /*
                     * If the cancel timer expired decrement the
                     * count of threads waiting to establish a new
                     * association and return.
                     */
                    if (!retry_op)
                    {
                        grp_new_waiters--;
                        return (NULL);
                    }
                }

                /*
                 * We're done waiting. Try for an existing
                 * association or at least existing group by going
                 * through the loop again.
                 */
                grp_new_waiters--;
                continue;
            }
            else  /* grp_new_in_progress is false; i.e., grp_new mutex
                   * is available.
                   */
            {
                /*
                 * There is no other thread opening a new
                 * association. Acquire the grp_new mutex.
                 * We also set i_hold_grp_new_mutex.
                 *
                 * Note: If we get here, we are by definition, in
                 * the outermost call to assoc_request.
                 */
                grp_new_in_progress = true;
                i_hold_grp_new_mutex = true;

            }
        } /* end else (assoc_grp == NULL) */

        /*
         * Determine whether the association group we have at
         * this point, if any, can support another association.
         */
        if ((assoc_grp == NULL) 
             ||
            (assoc_grp->grp_cur_assoc < assoc_grp->grp_max_assoc))
        {
            /*
             * We can add another association to the group (if we
             * have one) if it can be established. First create an
             * association control block structure and its receiver thread.
             * We don't check the return value because it always
             * returns rpc_s_ok.
             */
            assoc = rpc__cn_assoc_acb_alloc (true,
                                             RPC_C_CN_ASSOC_CLIENT, 
                                             st);
	    if (*st != rpc_s_ok) {
		if (i_hold_grp_new_mutex) {
		    grp_new_in_progress = false;
		    RPC_COND_BROADCAST (grp_new_wt, rpc_g_global_mutex);
		}
		return NULL;
	    }

            /*
             * We have an association control block. Send an
             * association request event through its state machine.
             * Set the association reference count to 1 so that this
             * association will not be allocated by another thread
             * while still being set up.
             */
            RPC_CN_ASSOC_ACB_INC_REF (assoc);
            assoc->assoc_ref_count++;
            if (old_server)
            {
                assoc->assoc_vers_minor = RPC_C_CN_PROTO_VERS_COMPAT;
            }

            /*
             * Store the call rep in the association for use
             * in cancel timeout processing.
             */
            assoc->call_rep = call_r;
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                            ("CN: call_rep->%x assoc->%x desc->%x establishing connection & negotiating presentation syntax\n",
                             assoc->call_rep,
                             assoc,
                             assoc->cn_ctlblk.cn_sock));

            rpc__cn_assoc_open (assoc, 
                                rpc_addr,
                                if_r,
                                rem_grp_id,
                                binding_r->common.auth_info,
                                binding_r->common.transport_info,
                                syntax, 
                                context_id, 
                                sec,
                                st);

            /*
             * Release the grp_new mutex if we locked it.
             */
            if (i_hold_grp_new_mutex)
            {
                /*
                 * A new association has just been opened so reset
                 * the flag to allow other thread's which want to open
                 * new associations to do so.
                 */
                grp_new_in_progress = false;
                RPC_COND_BROADCAST (grp_new_wt,
                                    rpc_g_global_mutex);
            }
            RPC_CN_ASSOC_ACB_DEC_REF (assoc);
            assoc->assoc_ref_count--;
            if (*st == rpc_s_ok)
            {
                /*
                 * Allocate the association.
                 * Send an association allocate request through the association state
                 * machine. This will put the association into the active state.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: call_rep->%x assoc->%x desc->%x presentation negotiation succeeded\n",
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock));
                
                rpc__cn_assoc_alloc (assoc, st);
                if (*st == rpc_s_ok)
                {
                    /*
                     * We now have an association group. Save the group id in the
                     * binding rep for use on subsequent lookups and look for an open
                     * association on the group.
                     */
                    binding_r->grp_id.all = assoc->assoc_grp_id.all;

                    /*
                     * Update transport information on binding handle
                     */
                    
                    rpc__transport_info_release(binding_r->common.transport_info);
                    binding_r->common.transport_info = assoc->transport_info;
                    rpc__transport_info_retain(binding_r->common.transport_info);

                    /*
                     * The association has been allocated and is now
                     * ready for an RPC. 
                     */
                    RPC_LOG_CN_ASSOC_REQ_XIT;
                    return (assoc);
                }
            }
            else
            {
                /*
                 * The association open failed. We may have failed
                 * while trying to establish a session/transport
                 * connection or during or after presentation
                 * negotiation. If the connect request failed the
                 * association will be in the closed state and the acb
                 * just has to be dealloced to go back on the
                 * lookaside list. If the presentation
                 * negotiation failed the association will already
                 * on a group. We can also just deallocate the
                 * acb in this case since it is attached to an
                 * association group. If a cancel timeout occured
                 * the negotiation will eventually complete and the
                 * association will be placed on a group.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: call_rep->%x assoc->%x desc->%x presentation negotiation failed st = %x\n",
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock,
                                 *st));

                /* 
                 * The call is about to be orphaned, so remove it from the assoc
                 */
                if (assoc->call_rep == call_r)
                {
                    assoc->call_rep = NULL;
                }
                
                if ((old_server == false) &&
		    (*st == rpc_s_connection_closed) &&
                    ((assoc->assoc_vers_minor == RPC_C_CN_PROTO_VERS_COMPAT) ||
                     (assoc->bind_packets_sent > 1)))
                {
                    /* Probably a 5.0 server with fragmented BINDs - retry */
                    old_server = true;
                    RPC_CN_ASSOC_ACB_INC_REF (assoc);
                    rpc__cn_assoc_acb_dealloc (assoc);
                    assoc = NULL;
                    continue;
                }

                RPC_CN_ASSOC_ACB_INC_REF (assoc);
                rpc__cn_assoc_acb_dealloc (assoc);
                assoc = NULL;

                /*
                 * Return a failure immediately if:
                 *
                 * 1) the association open failed because it was
                 *    rejected or a local error occurred OR
                 *
                 * 2) the connection request failed and the binding
                 *    timeout was less than 6
                 */
                if (!rpc__cn_network_connect_fail (*st))
                {
                    return (NULL);
                }
                else
                {
                    if (binding_r->common.timeout < 6)
                    {
                        return (NULL);
                    }
                }
            } /* end else (*st != rpc_s_ok) */
        } /* end if (assoc_grp == NULL) || (assoc_grp->grp_cur_assoc */
          /* < assoc_grp->grp_max_assoc) */ 
        else
        {
            *st = rpc_s_assoc_grp_max_exceeded;
        }

        /*
         * We've dropped to this point in the for loop if 
         *
         * 1) the connection request failed because it either timed
         *    out or the server rejected it and the binding timeout
         *    was 6 or higher or
         *
         * 2) the maximum number of associations were reached on a
         *    particular group.
         *
         * In any case we will try to reclaim some open
         * associations, and perform an exponential backoff.
         */
        timespec.tv_sec = MIN (RPC_RANDOM_GET (0, wait_interval), 
                               RPC_C_ASSOC_MAX_WAIT_INTERVAL);
        timespec.tv_nsec = 0;
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: call_rep->%x assoc->%x desc->%x backing off %d seconds before retrying ...\n",
                         call_r,
                         NULL,
                         0,
                         timespec.tv_sec));
        if (!RPC_CN_LOCAL_ID_VALID (grp_id))
        {
            rpc__cn_assoc_reclaim (grp_id, RPC_C_CN_ASSOC_GRP_CLIENT, true);
            RPC_CN_UNLOCK ();
            retry_op = true;
            DCETHREAD_TRY
            {
                dcethread_delay (&timespec);
            }
            DCETHREAD_CATCH (dcethread_interrupt_e)
            {
                RPC_CN_LOCK ();

                /*
                 * Record the fact that a cancel has been
                 * detected. Also, start a timer if this is
                 * the first cancel detected.
                 */
                rpc__cn_call_local_cancel (call_r,
                                           &retry_op,
                                           st);
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                                ("(rpc__cn_assoc_request) call_rep->%x assoc->%x desc->%x cancel caught before association setup\n", 
                                 call_r,
                                 NULL,
                                 0));
                RPC_CN_UNLOCK ();
            }
            DCETHREAD_ENDTRY
            RPC_CN_LOCK ();
            if (!retry_op)
            {
                return (NULL);
            }
        }
        else
        {
            assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
            rpc__cn_assoc_reclaim (assoc_grp->grp_id,
                                   RPC_C_CN_ASSOC_GRP_CLIENT,
				   true);
            assoc_grp->grp_assoc_waiters++;
            dcethread_get_expiration (&timespec, &abstime);
            retry_op = true;
            DCETHREAD_TRY
            {
                RPC_COND_TIMED_WAIT (assoc_grp->grp_assoc_wt,
                                     rpc_g_global_mutex,
                                     &abstime);
            }
            DCETHREAD_CATCH (dcethread_interrupt_e)
            {
                /*
                 * Record the fact that a cancel has been
                 * detected. Also, start a timer if this is
                 * the first cancel detected.
                 */
                rpc__cn_call_local_cancel (call_r,
                                           &retry_op, 
                                           st);
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                                ("(rpc__cn_assoc_request) call_rep->%x cancel caught before association setup\n", 
                                 call_r,
                                 NULL,
                                 0));
            }
            DCETHREAD_ENDTRY
            if (!retry_op)
            {
                return (NULL);
            }
            assoc_grp->grp_assoc_waiters--;
        }

        total_wait = total_wait + wait_interval;
        if (binding_r->common.timeout != rpc_c_binding_infinite_timeout)
        {
            if (total_wait > 
                MIN ((binding_r->common.timeout * 6), RPC_C_ASSOC_MAX_RESOURCE_WAIT))
            {
                break;
            }
        }
        wait_interval = MIN ((wait_interval * 2), RPC_C_ASSOC_MAX_WAIT_INTERVAL);
    } /* while (true) */

    /*
     * The status code returned from here will be the last one
     * returned from the rpc__socket_connect call.
     */
    return (NULL);
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_listen
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine is called when a new connection has been accepted.
**  A new association will be set up, including its receiver
**  thread, and returned.
**
**  INPUTS:             
**
**      desc            The network descriptor of the new connection.
**      endpoint        The local endpoint the connection came into.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      return          The association. NULL if not created.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_cn_assoc_t *rpc__cn_assoc_listen 
(
   rpc_socket_t            newsock,
   unsigned_char_p_t       endpoint,
   unsigned32              *st
)
{
    rpc_cn_assoc_t      *assoc;
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_transport_info_p_t transport_info = NULL;

    RPC_LOG_CN_ASSOC_LIS_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_listen);
    CODING_ERROR (st);
    
    /*
     * Create a server association.
     */
    assoc = rpc__cn_assoc_acb_alloc (false, RPC_C_CN_ASSOC_SERVER, st);
    if (*st != rpc_s_ok)
    {
        /*
         * The creation failed. The caller will close the socket.
         */
        return (NULL);
    }
    
    /*
     * Indicate that there is a valid connection.
     */
    assoc->cn_ctlblk.cn_state = RPC_C_CN_OPEN;
    assoc->cn_ctlblk.cn_sock  = newsock;
    assoc->cn_ctlblk.cn_listening_endpoint = endpoint;
    
    /* Attempt to query socket for transport information */
    serr = rpc__socket_inq_transport_info(newsock, &transport_info);
    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                        ("(rpc__cn_assoc_listen) desc->%x rpc__socket_inq_transport_info failed, error = %d\n",
                         assoc->cn_ctlblk.cn_sock, 
                         serr));
    }
    else
    {
        rpc__transport_info_release(assoc->transport_info);
        assoc->transport_info = transport_info;
    }

    /*
     * A connection is now set up. Tell the receiver thread to begin
     * receiving on the connection.
     */
    if (assoc->cn_ctlblk.cn_rcvr_waiters)
    {
        RPC_COND_SIGNAL (assoc->cn_ctlblk.cn_rcvr_cond, 
                         rpc_g_global_mutex);
    }
    else
    {
        RPC_DBG_PRINTF (rpc_e_dbg_threads, RPC_C_CN_DBG_THREADS,
	    ( "####### assoc->%x We're not signalling here\n", assoc ));
    }
    *st = rpc_s_ok;
    RPC_LOG_CN_ASSOC_LIS_XIT;
    return (assoc);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_alloc
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**      This routine will send an association allocate request
**      through the association state and allocate the association
**      control block.
**
**  INPUTS:             
**
**      assoc           The association being allocated
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_assoc_alloc 
(
  rpc_cn_assoc_p_t        assoc,
  unsigned32              *st
)
{
    RPC_LOG_CN_ASSOC_ALLOC_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_alloc);
    CODING_ERROR(st);

    RPC_CN_STATS_INCR (alloced_assocs);
    RPC_CN_ASSOC_ACB_INC_REF (assoc);
    RPC_CN_ASSOC_EVAL_USER_EVENT (assoc, 
                                  RPC_C_ASSOC_ALLOCATE_REQ,
                                  NULL,
                                  *st);
    RPC_LOG_CN_ASSOC_ALLOC_XIT;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_dealloc
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will send a deallocate event through the
**  association state machine and deallocate the association
**  control block.
**
**  INPUTS:             
**
**      assoc           The association being deallocated
**	call_rep	The call rep of the association.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_assoc_dealloc 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_cn_call_rep_p_t     call_rep,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_fragbuf_t    *fragbuf;

    RPC_LOG_CN_ASSOC_DEALLOC_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_dealloc);
    CODING_ERROR(st);

    *st = rpc_s_ok;
    if (assoc != NULL)
    {
        /*
         * Send a deallocate request through the association state
         * machine. Make sure to clear the association status code
         * first so that the deallocate request will be processed (the
         * macro EVAL_USER_EVENT checks that the association status is
         * OK before processing the event).
         */
        RPC_CN_STATS_INCR (dealloced_assocs);
        assoc->assoc_status = rpc_s_ok;
        RPC_CN_ASSOC_EVAL_USER_EVENT (assoc, 
                                      RPC_C_ASSOC_DEALLOCATE_REQ,
                                      NULL,
                                      *st);
        
        /*
         * Signal any threads waiting for an association that they can
         * retry. Only client threads wait for an existing association.
         */
        assoc_grp = RPC_CN_ASSOC_GRP (assoc->assoc_grp_id);
        if ((assoc_grp != NULL) && 
            (assoc_grp->grp_assoc_waiters) &&
            (assoc->assoc_flags & RPC_C_CN_ASSOC_CLIENT))
        {
            RPC_COND_SIGNAL (assoc_grp->grp_assoc_wt,
                             rpc_g_global_mutex);
        }
        
        /*
         * If the passed call_rep is the same as the call rep in the passed
         * assoc, then it's ok to flush the receive queue of the association
         * just in case this call was orphaned. (The alter context request
         * failed in rpc__cn_assoc_request().)
         * If both the call rep in the passed assoc and the assoc in the
         * passed call rep are NULL, then it's ok to flush the receive queue
         * of the association just in case this call was orphaned. (The link
         * was broken by rpc__cn_assoc_pop_call().)
         * Otherwise, don't flush the receive queue - it means that this
         * call rep belongs to an orphaned or being about to finish call.
         * The call rep in the assoc refers to a queued call to be executed.
         */
        if (call_rep == assoc->call_rep
            || (assoc->call_rep == NULL && call_rep->assoc == NULL))
        {

            /*
             * Flush the receive queue of the association just in case this
             * call was orphaned.
             */
            RPC_LIST_FIRST (assoc->msg_list, 
                            fragbuf,
                            rpc_cn_fragbuf_p_t);            
            while (fragbuf != NULL)
            {
                rpc_cn_fragbuf_t    *next_fragbuf;
    
                RPC_LIST_NEXT (fragbuf,
                               next_fragbuf,
                               rpc_cn_fragbuf_p_t);            
                if (fragbuf->fragbuf_dealloc != NULL)
                {
                    (*fragbuf->fragbuf_dealloc)(fragbuf);
                }
                fragbuf = next_fragbuf;
            }
            RPC_LIST_INIT (assoc->msg_list);
        }

        /*
         * Deallocate the association control block.
         */
        rpc__cn_assoc_acb_dealloc (assoc);
    }
    RPC_LOG_CN_ASSOC_DEALLOC_XIT;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_abort
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**
**  This routine will send an abort association request through the
**  association state machine.  The caller of this routine is assumed 
**  to not have a reference to the acb (i.e. it may be the association
**  timer reclaimation thread).
**
**  INPUTS:
**
**      assoc           The association.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_assoc_abort 
(
  rpc_cn_assoc_p_t        assoc,
  unsigned32              *st
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_abort);
    
    RPC_CN_STATS_INCR (aborted_assocs);

    /*
     * Send an association abort request through the association
     * state machine. Make sure to clear the association status code
     * first so the event will be processed.
     */
    assoc->assoc_status = rpc_s_ok;
    RPC_CN_ASSOC_EVAL_USER_EVENT (assoc, 
                                  RPC_C_ASSOC_ABORT_REQ,
                                  NULL,
                                  *st);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_pop_call
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will return the call rep contained on the association.
**
**  INPUTS:             
**
**      assoc           The association containing the call rep.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      call_r          The call rep.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_cn_call_rep_t *rpc__cn_assoc_pop_call 
(
  rpc_cn_assoc_p_t   assoc,
  rpc_cn_call_rep_p_t  call_rep
)
{
    rpc_cn_call_rep_t   *call_r;
    rpc_cn_assoc_grp_t  *assoc_grp;
    unsigned32          st;

    RPC_LOG_CN_ASSOC_POP_CALL_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_pop_call);

    if (assoc != NULL)
    {
        /*
         * Decrement the association group's total call count. If it
         * becomes zero, it's a server association group and the current
         * state is call wait send a no calls indication event through
         * the group's state machine. 
         */
        assoc_grp = RPC_CN_ASSOC_GRP (assoc->assoc_grp_id);
        if (assoc_grp != NULL)
        {
            assoc_grp->grp_callcnt--;
            if ((assoc_grp->grp_flags & RPC_C_CN_ASSOC_GRP_SERVER) 
                &&
                (assoc_grp->grp_callcnt == 0)
                &&
                (assoc_grp->grp_state.cur_state ==
                 RPC_C_SERVER_ASSOC_GRP_CALL_WAIT)) 
            {
                RPC_CN_ASSOC_GRP_EVAL_EVENT (assoc_grp, 
                                             RPC_C_ASSOC_GRP_NO_CALLS_IND,
                                             assoc,
                                             assoc_grp->grp_status);
            }
        }
        
        /*
         * If this was the last call on a client association send a
         * calls done event through its state machine.
         */
        if (assoc->assoc_flags & RPC_C_CN_ASSOC_CLIENT) 
        {
            /*
             * Make sure to clear the association status code first
             * so this event will be processed.
             */
            assoc->assoc_status = rpc_s_ok;
            RPC_CN_ASSOC_EVAL_USER_EVENT (assoc, 
                                          RPC_C_ASSOC_CALLS_DONE,
                                          NULL,
                                          st);
        }
        
        /*
         * Save a pointer to the call rep on the association.
         */
        call_r = assoc->call_rep;

        /*
         * If the passed call_rep is the same as the call rep in the 
         * passed assoc, then it's ok to break the link. Otherwise,
         * don't change the assoc - it means that this call rep belongs
         * to an orphaned call. The assoc refers to a queued call to be
         * executed.
         */
        if (call_rep == call_r)
        {
            assoc->call_rep = NULL;
        }

        /*
         * Break the connection from the call rep back to the association.
         */
        call_rep->assoc = NULL;

        RPC_LOG_CN_ASSOC_POP_CALL_XIT;
    }
    else
    {
        call_r = NULL;
    }

    return (call_r);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_push_call
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will put a call rep on an association.
**
**  INPUTS:             
**
**      assoc           The association containing the call rep.
**      call_r          The call rep to be placed.
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

PRIVATE void rpc__cn_assoc_push_call 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_cn_call_rep_p_t     call_r,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;

    RPC_LOG_CN_ASSOC_PUSH_CALL_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_push_call);

    /*
     * Push the new call rep onto the association.
     */
    assoc->call_rep = call_r;

    /*
     * Increment the association group's total call count.
     */
    assoc_grp = RPC_CN_ASSOC_GRP (assoc->assoc_grp_id);
    if (assoc_grp != NULL)
    {
        *st = rpc_s_ok;
        assoc_grp->grp_callcnt++;
    }
    else
    {
        *st = rpc_s_assoc_grp_not_found;
    }

    RPC_LOG_CN_ASSOC_PUSH_CALL_XIT;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_queue_frag
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will queue a fragment buffer on the end of the
**  message list of an association. In addition it will notify any
**  thread waiting for a fragment buffer on this association that
**  one is available.
**
**  INPUTS:             
**
**      assoc           The association containing the queue of
**                      fragment buffers.
**      fragbuf         The fragment buffer to queue.
**      signal          true if the condition variable should be signalled.
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

PRIVATE void rpc__cn_assoc_queue_frag 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_cn_fragbuf_p_t      fragbuf,
  boolean32               signal
)
{
    RPC_LOG_CN_ASSOC_Q_FRAG_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_queue_frag);
    
    /*
     * Queue the fragment buffer to the tail of the list.
     */
    RPC_LIST_ADD_TAIL (assoc->msg_list, fragbuf, rpc_cn_fragbuf_p_t);
    
    /*
     * Notify any waiting threads that there's a buffer on this
     * association if the caller indicated it and there are waiters.
     */
    if (signal && assoc->assoc_msg_waiters)
    {
        RPC_COND_SIGNAL (assoc->assoc_msg_cond, 
                         rpc_g_global_mutex);
    }
    RPC_LOG_CN_ASSOC_Q_FRAG_XIT;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_queue_dummy_frag
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will queue the built-in dummy fragment buffer on
**  the end of the  message list of an association. In addition it will notify 
**  any thread waiting for a fragment buffer on this association that
**  one is available.
**
**  INPUTS:             
**
**      assoc           The association containing the queue of
**                      fragment buffers.
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

PRIVATE void rpc__cn_assoc_queue_dummy_frag 
(
  rpc_cn_assoc_p_t        assoc
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_queue_dummy_frag);
    
    /*
     * Queue the fragment buffer to the tail of the list.
     */
    RPC_LIST_ADD_TAIL (assoc->msg_list, 
                       &assoc->assoc_dummy_fragbuf,
                       rpc_cn_fragbuf_p_t);
    
    /*
     * Notify any waiting threads that there's a buffer on this
     * association if the caller indicated it and there are waiters.
     */
    if (assoc->assoc_msg_waiters)
    {
        RPC_COND_SIGNAL (assoc->assoc_msg_cond, 
                         rpc_g_global_mutex);
    }
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_receive_frag
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will receive a fragment over the connection
**  attached to an association.
**
**  INPUTS:             
**
**      assoc           The association to receive from.
**      fragbuf         The place to put the received fragment.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**                      rpc_s_ok
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_assoc_receive_frag 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_cn_fragbuf_p_t      *fragbuf,
  unsigned32              *st
)
{
    volatile boolean32  retry_op;
    rpc_cn_call_rep_p_t call_rep;
    rpc_binding_rep_p_t binding_r;

    RPC_LOG_CN_ASSOC_RECV_FRAG_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_receive_frag);
    CODING_ERROR(st);
    
    /*
     * Wait for the receiver thread to put something on the queue.
     */
    retry_op = true;
    while ((assoc->assoc_status == rpc_s_ok) && (RPC_LIST_EMPTY (assoc->msg_list)))
    {
        /*
         * Save the assoc's call_rep before we begin the wait.
         */
        call_rep = assoc->call_rep;
        binding_r = call_rep->binding_rep;
        assert(binding_r != NULL);

        assoc->assoc_msg_waiters++;

        /*
         * Since this is a cancellable operation we'll set
         * up an exception handler.
         */
        RPC_LOG_TRY_PRE;
        DCETHREAD_TRY
        RPC_LOG_TRY_POST;
        {
            RPC_COND_WAIT (assoc->assoc_msg_cond,
                           rpc_g_global_mutex);
        }
        RPC_LOG_CATCH_PRE;
        DCETHREAD_CATCH (dcethread_interrupt_e)
        RPC_LOG_CATCH_POST;
        {
            RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                            ("(rpc__cn_assoc_receive_frag) call_rep->%x assoc->%x desc->%x cancel caught\n", 
                             assoc->call_rep,
                             assoc,
                             assoc->cn_ctlblk.cn_sock));
            rpc__cn_call_local_cancel (call_rep,
                                       &retry_op,
                                       st);
        }
        RPC_LOG_ENDTRY_PRE;
        DCETHREAD_ENDTRY
        RPC_LOG_ENDTRY_POST;

        assoc->assoc_msg_waiters--;

        /*
         * If the call_rep is different from the original call, it means
         * the original call was orphaned and another call began in 
         * another call executor thread (on this assoc). In this
         * case we don't want to read a fragbuf (it doesn't belong to us).
         * We'll get here when the stop_orphan_action_rtn either signals
         * the assoc_msg_cond condition variable cancels the call executor
         * thread.
         */
        if (call_rep != assoc->call_rep)
        {
            *st = rpc_s_call_orphaned;
            return;
        }

        /*
         * If a cancel was caught and the operation should not be
         * retried just return now. The error status is already set up.
         */
        if (!retry_op) 
        {
            return;
        }
    }
    
    /*
     * Remove a fragment from the queue.
     */
    RPC_LIST_REMOVE_HEAD (assoc->msg_list, 
                          *fragbuf, 
                          rpc_cn_fragbuf_p_t);

    
    *st = assoc->assoc_status;
    RPC_LOG_CN_ASSOC_RECV_FRAG_XIT;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_send_frag
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will send a vector of fragments over the connection
**  attached to an association.
**
**  INPUTS:             
**
**      assoc           The association to send over.
**      iovector        The iovector containing the buffers to be sent.
**      sec             The security context element containing
**                      information required to apply the authn
**                      level requested, NULL if none.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_connection_aborted
**                      rpc_s_connection_closed
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

PRIVATE void rpc__cn_assoc_send_frag 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_iovector_p_t        iovector,
  rpc_cn_sec_context_p_t  sec,
  unsigned32              *st
)
{
    rpc_socket_iovec_t          iov[RPC_C_MAX_IOVEC_LEN];
    volatile int                iovcnt;
    rpc_socket_iovec_t          * volatile iovp;
    rpc_socket_iovec_t          out_iov;
    static rpc_addr_p_t         addr = NULL;
    volatile unsigned32         bytes_to_send;
    volatile boolean32          free_iov_buffer;
    volatile boolean32          retry_op;
    volatile rpc_socket_error_t serr;
    volatile int                cc;
    byte_p_t                    volatile save_base = NULL;
   
    //DO_NOT_CLOBBER(iovcnt);
    //DO_NOT_CLOBBER(iovp);
    //DO_NOT_CLOBBER(bytes_to_send);
    //DO_NOT_CLOBBER(free_iov_buffer);
    //DO_NOT_CLOBBER(save_base);

    RPC_LOG_CN_ASSOC_SEND_FRAG_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_send_frag);
    CODING_ERROR(st);

    if (!assoc->cn_ctlblk.cn_sock)
    {
        *st = rpc_s_connection_closed;
        return;
    }

    memset(&iov[0], 0, sizeof(iov));

    /*
     * Convert the RPC iovector array of buffers passed in the
     * system defined iov array of buffers to be handed to the
     * socket interface. 
     */
    RPC_CN_NETWORK_IOVECTOR_TO_IOV (iovector, 
                                    iov, 
                                    iovcnt,
                                    bytes_to_send); 
    iovp = &iov[0];

    /*
     * Apply the authentication level requested, if any.
     */
    free_iov_buffer = false;
    if (sec != NULL)
    {
        RPC_CN_AUTH_PRE_SEND (&assoc->security,
                              sec,
                              iovp,
                              iovcnt,
                              &out_iov,
                              st);
        if (*st != rpc_s_ok)
        {
            if (assoc->assoc_flags & RPC_C_CN_ASSOC_SERVER)
            {
		dce_error_string_t error_text;
		int temp_status;

		dce_error_inq_text(*st, (unsigned char*) error_text, &temp_status);

		/*
		 * rpc_m_call_failed_s
		 * "%s on server failed: %s"
		 */
                RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
		    rpc_svc_auth,
		    svc_c_sev_error,
		    rpc_m_call_failed_s,
		    "RPC_CN_AUTH_PRE_SEND",
		    error_text ));
            }
            return;
        };

        /*
         * The pre_send routine may have had to coalesce the iov
         * elements given to it into a single buffer for encryption.
         * If so then we should send that instead of what was given
         * to pre_send.
         */
        if (out_iov.iov_base != NULL)
        {
            iovp = &out_iov;
            iovcnt = 1;
	    bytes_to_send = iovp->iov_len;
            free_iov_buffer = true;
            save_base = iovp->iov_base;
        }
    }

    /*
     * Now send the constructed system iov array on the connection
     * identified in the association control block.
     */
    serr = 0;
    retry_op = true;
    while ((bytes_to_send) && (!RPC_SOCKET_IS_ERR (serr)))
    {
        RPC_LOG_TRY_PRE;
        DCETHREAD_TRY
        RPC_LOG_TRY_POST;
        {
            /*
             * Unlock the global mutex in case we get flow blocked. This
             * will allow other threads to continue. Set the "in_sendmsg"
             * flag to indicate to the receiver thread that we are in sendmsg
             * so it should not send any events through the state machine
             * until we are done.
             */
            assoc->cn_ctlblk.in_sendmsg = true;
            RPC_CN_UNLOCK ();

#ifdef NON_CANCELLABLE_IO
            /*
             * By posix definition dcethread_enableasync is not a "cancel
             * point" because it must return an error status and an errno.
             * dcethread_enableasync(1) will not deliver
             * a pending cancel nor will the cancel be delivered asynchronously,
             * thus the need for dcethread_testcancel.
             */
            dcethread_enableasync_throw(1);
	    dcethread_checkinterrupt_throw();
#endif /* NON_CANCELLABLE_IO */
            serr = rpc__socket_sendmsg (
                assoc->cn_ctlblk.cn_sock, 
                iovp, 
                iovcnt,
                addr,
                (int*) &cc);

#ifdef NON_CANCELLABLE_IO
	    dcethread_enableasync_throw(0);
#endif /* NON_CANCELLABLE_IO */
            /*
             * A sendmsg has just completed. Re-aquire the global mutex
             * and notify our receiver thread that we are done if it is
             * waiting.
	     *
	     * NOTE that we also need to reacquire the mutex in
	     * every DCETHREAD_CATCH clause associated with this TRY/ENDTRY.
             */
            RPC_CN_LOCK ();
            assoc->cn_ctlblk.in_sendmsg = false;

        }
        RPC_LOG_CATCH_PRE;
        DCETHREAD_CATCH (dcethread_interrupt_e)
        RPC_LOG_CATCH_POST;
        {
#ifdef NON_CANCELLABLE_IO
            dcethread_enableasync_throw(0);
#endif /* NON_CANCELLABLE_IO */
            /*
             * A sendmsg has just completed. Re-aquire the global mutex
             * and notify our receiver thread that we are done if it is
             * waiting.
	     */
	    RPC_CN_LOCK ();
            assoc->cn_ctlblk.in_sendmsg = false;

            /*
             * The test for cancel is before any data has been sent.
             * Set count of bytes of data sent to zero.
             */
            cc = 0;
            /*
             * Do NOT forward the cancel to the server just record
             * that it was detected.
             * Note: this is a copy of rpc__cn_call_local_cancel()
             * with the call to rpc__cn_call_forward_cancel() removed.
             */
            if (RPC_CALL_IS_CLIENT (((rpc_call_rep_t *) assoc->call_rep)))
            {
                /*
                 * Record the cancel that was just detected.
                 */
                assoc->call_rep->u.client.cancel.local_count++;
                rpc__cn_call_start_cancel_timer (assoc->call_rep, st);
                if (*st == rpc_s_ok)
                {
                    retry_op = true;
                }
                else
                {
                    retry_op = false;
                }
            }
            else
            {
                retry_op = false;
                *st = rpc_s_call_cancelled;
            }

            RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                            ("(rpc__cn_assoc_send_frag) call_rep->%x assoc->%x desc->%x cancel caught\n", 
                             assoc->call_rep,
                             assoc,
                             assoc->cn_ctlblk.cn_sock));

        }
        DCETHREAD_CATCH (dcethread_SIGPIPE_e)
        {
#ifdef NON_CANCELLABLE_IO
	    dcethread_enableasync_throw(0);
#endif
            /*
             * A sendmsg has just completed. Re-aquire the global mutex
             * and notify our receiver thread that we are done if it is
             * waiting.
	     */
	    RPC_CN_LOCK ();
            assoc->cn_ctlblk.in_sendmsg = false;

            cc = -1;
            serr = RPC_C_SOCKET_EPIPE;
        }
        RPC_LOG_ENDTRY_PRE;
        DCETHREAD_ENDTRY
        RPC_LOG_ENDTRY_POST;

        if (assoc->cn_ctlblk.waiting_for_sendmsg_complete)
        {
            RPC_COND_SIGNAL (assoc->cn_ctlblk.cn_rcvr_cond,
                             rpc_g_global_mutex);
        }

        /*
         * If a cancel was caught and the operation should not be
         * retried just return now. The error status is already set
         * up.
         */
        if (!retry_op)
        {
            return;
        }

        /*
         * Check the operation for an error.
         */
        if (RPC_SOCKET_IS_ERR (serr))
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                            ("(rpc__cn_assoc_send_frag) call_rep->%x assoc->%x desc->%x SENDMSG failed, error=%d\n",
                             assoc->call_rep,
                             assoc,
                             assoc->cn_ctlblk.cn_sock,
                             RPC_SOCKET_ETOI(serr)));
            /*
             * The transmit failed. Find out why.
             */
            switch (serr)
            {
                case (RPC_C_SOCKET_ECONNRESET):
                assoc->assoc_status = rpc_s_connection_aborted;
                *st = rpc_s_connection_aborted;
                break;
                
                default:
                assoc->assoc_status = rpc_s_connection_closed;
                *st = rpc_s_connection_closed;
            }
            break;
        }
        
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("CN: call_rep->%x assoc->%x desc->%x sent %d bytes\n", 
                         assoc->call_rep, 
                         assoc, 
                         assoc->cn_ctlblk.cn_sock,
                         cc));
        bytes_to_send -= cc;
        if (bytes_to_send)
        {
            RPC_CN_NETWORK_IOV_ADJUST (iovp, iovcnt, cc);
        }
    }

    /*
     * Free the security allocated contiguous buffer is we have to.
     */
    if (free_iov_buffer)
    {
        RPC_MEM_FREE (save_base,
                      RPC_C_MEM_CN_ENCRYPT_BUF);
    }

    if (!RPC_SOCKET_IS_ERR (serr))
    {
        /*
         * Keep some stats on the packets sent.
         */
        RPC_CN_STATS_INCR (pstats[RPC_CN_PKT_PTYPE (((rpc_cn_packet_p_t) iovector->elt[0].data_addr))].sent);
        RPC_CN_STATS_INCR (pkts_sent);
        *st = rpc_s_ok;
        RPC_CN_PKT_TRC (((rpc_cn_packet_p_t) iovector->elt[0].data_addr));
        RPC_CN_IOV_DUMP (iovector);

        /*
         * Increment the per-association security context next send
         * sequence number.
         */
        assoc->security.assoc_next_snd_seq++;
    }
    RPC_LOG_CN_ASSOC_SEND_FRAG_XIT;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_send_fragbuf
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**      This routine will convert a fragbuf to a vector of fragments
**      and send them over a connection. If indicated the fragbuf will 
**      then be freed.
**
**  INPUTS:             
**
**      assoc           The association to send over.
**      fragbuf         The fragbuf to be sent.
**      sec             The security context element containing
**                      information required to apply the authn
**                      level requested, NULL if none.
**      freebuf         Flags to indicate we should free the fragbuf.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
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

PRIVATE void rpc__cn_assoc_send_fragbuf 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_cn_fragbuf_p_t      fragbuf,
  rpc_cn_sec_context_p_t  sec,
  boolean32                freebuf,
  unsigned32              *st
)
{
    rpc_iovector_t              iovector;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_send_fragbuf);

    /*
     * The PDU is ready to be sent. Put it into an RPC
     * iovector structure.
     */
    iovector.num_elt = 1;
    iovector.elt[0].flags = 0;
    iovector.elt[0].data_addr = (byte_p_t) fragbuf->data_p;
    iovector.elt[0].data_len = fragbuf->data_size;
    
    /*
     * Now actually send the PDU.
     */
    rpc__cn_assoc_send_frag (assoc, &iovector, sec, st);
    
    /*
     * Free up the fragment buffer we used whether the send
     * succeeded or failed.
     */
    if (freebuf)
        (*fragbuf->fragbuf_dealloc)(fragbuf);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_syntax_negotiate
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine takes as input a list of abstract syntaxes. Each
**  abstract syntax has a list of transfer syntaxes associated with
**  it. This routine will determine whether any of the transfer syntaxes
**  associated with an abstract syntax are supported. Since an abstract
**  syntax is really an interface UUID and version this is done by
**  performing an interface lookup which returns the interface
**  specification data structure. This structure contains the list of
**  transfer syntaxes the stub supports. Note that this is another
**  place we'd have to expand the list of transfer syntaxes if procdural
**  marshaling were being supported.
**
**  INPUTS:
**
**      assoc           The association over which the negotiation
**                      is taking place.
**      pres_cont_list  A list of abstract syntaxes. Each abstract
**                      syntax contains a list of transfer syntaxes.
**
**  INPUTS/OUTPUTS:     
**
**      size            On input, the size left for formatting the
**                      presentation result list. On output the
**                      actual size taken for it. 
**
**  OUTPUTS:            
**
**      pres_result_list A list of negotiated transfer syntaxes. One
**                      for each abstract syntax in the
**                      presentation context list.
**      st              The return status of this routine.
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

PRIVATE void rpc__cn_assoc_syntax_negotiate 
(
  rpc_cn_assoc_p_t                assoc,
  rpc_cn_pres_cont_list_p_t       pres_cont_list,
  unsigned32                      *size,
  rpc_cn_pres_result_list_t       *pres_result_list,
  unsigned32                      *st
)
{
    unsigned16                  ihint;
    rpc_if_rep_t                *if_r;
    boolean                     syntax_match;
    rpc_cn_syntax_t             *pres_context;
    unsigned32                  i, j, k;

    CODING_ERROR (st);
    RPC_LOG_CN_ASSOC_SYN_NEG_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_syntax_negotiate);
    
    if ((i = sizeof (rpc_cn_pres_result_list_t) +
        (sizeof (rpc_cn_pres_result_t) * 
         (pres_cont_list->n_context_elem - 1))) > *size)
    {
        *st = RPC_S_HEADER_FULL;
        *size = 0;
        return;
    }

    *size = i;
    *st = rpc_s_ok;

    /*
     * The number of results has to be the same as the number of
     * context elements in the list.
     */
    pres_result_list->n_results = pres_cont_list->n_context_elem;
    
    /*
     * For each element in the presentation context list determine
     * if the server stub supports a common transfer syntaxes.
     */
    for (i = 0; i < pres_cont_list->n_context_elem; i++)
    {
        /*
         * Find the interface specification for the abstract syntax
         * (really interface UUID and version). The interface spec
         * contains the list of transfer syntaxes supported by the stub.
         */
        ihint = RPC_C_INVALID_IHINT;
        rpc__if_lookup (&pres_cont_list->pres_cont_elem[i].abstract_syntax.id,
                        pres_cont_list->pres_cont_elem[i].abstract_syntax.version,
                        NULL,
                        &ihint,
                        &if_r,
                        NULL,
                        NULL,
                        st);

#ifdef DEBUG
        if (RPC_DBG_EXACT (rpc_es_dbg_cn_errors,
                           RPC_C_CN_DBG_IF_LOOKUP))
        {
            *st = RPC_S_CN_DBG_FAILURE;
        }
#endif

        if (*st != rpc_s_ok)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                            ("CN: call_rep->%x assoc->%x desc->%x presentation negotiation failed - abstract syntax not registered - st = %x\n", 
                             assoc->call_rep, 
                             assoc, 
                             assoc->cn_ctlblk.cn_sock,
                             *st));
            
            /*
             * There is no interface registered for the abstract
             * syntax given. Fill in the result for this abstract
             * syntax in the result list.
             */
            pres_result_list->pres_results[i].result =
            RPC_C_CN_PCONT_PROVIDER_REJECTION; 
            
            pres_result_list->pres_results[i].reason = 
            RPC_C_CN_PPROV_ABSTRACT_SYNTAX_NOT_SUPPORTED;
            
            memset ((char *)&(pres_result_list->pres_results[i].transfer_syntax),
                    0,
                    sizeof (rpc_cn_pres_syntax_id_t));
        }
        else
        {
            /*
             * An interface was found. Now try and find a transfer
             * syntax common to both the list given in the
             * presentation context list and the list in the
             * interface spec.
             */
            for (j = 0, syntax_match = false; 
                 (!syntax_match) &&
                 (j < pres_cont_list->pres_cont_elem[i].n_transfer_syn); 
                 j++)
            {
                for (k = 0; k < if_r->syntax_vector.count; k++)
                {
                    if (RPC_CN_ASSOC_SYNTAX_EQUAL (
                       &(pres_cont_list->pres_cont_elem[i].transfer_syntaxes[j]),
                       &(if_r->syntax_vector.syntax_id[k]),
                       &st))
                    {
                        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                        ("CN: call_rep->%x assoc->%x desc->%x presentation syntax negotiated\n", 
                                         assoc->call_rep,
                                         assoc,
                                         assoc->cn_ctlblk.cn_sock));
                        
                        /*
                         * We have a transfer syntax which matches.
                         * Allocate a syntax element, fill it in,
                         * and put it on the association syntax list.
                         * Mark the the syntax appropriately in the presentation
                         * results list and skip to the next
                         * abstract syntax. Note that the transfer
                         * syntax UUID is not filled in. The server
                         * stub requires only the syntax vector index.
                         */
                        pres_context = rpc__cn_assoc_syntax_alloc (if_r, st);

                        pres_context->syntax_ihint = ihint;
                        pres_context->syntax_pres_id = pres_cont_list->pres_cont_elem[i].pres_context_id;
                        pres_context->syntax_valid = true;
                        pres_context->syntax_vector_index = k;
                        RPC_LIST_ADD_TAIL (assoc->syntax_list, pres_context, rpc_cn_syntax_p_t);
                        
                        pres_result_list->pres_results[i].result =
                        RPC_C_CN_PCONT_ACCEPTANCE; 
                        
                        pres_result_list->pres_results[i].transfer_syntax = 
                        pres_cont_list->pres_cont_elem[i].transfer_syntaxes[j];
                        
                        syntax_match = true;
                        break;
                    }
                } /* end for (k...) */
            } /* end for (j...) */

#ifdef DEBUG
            if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                              RPC_C_CN_DBG_NO_XFER_SYNTAX))
            {
                syntax_match = false;
            }
#endif

            /*
             * See if a syntax was matched for this abstract syntax.
             */
            if (!syntax_match)
            {
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                                ("CN: call_rep->%x assoc->%x desc->%x presentation negotiation failed - no matching transfer syntax\n", 
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock));
                
                /*
                 * No matching syntax was found. Mark the
                 * presentation result list appropriately.
                 */
                pres_result_list->pres_results[i].result =
                RPC_C_CN_PCONT_PROVIDER_REJECTION; 
                
                pres_result_list->pres_results[i].reason = 
                RPC_C_CN_PPROV_PROPOSED_TRANSFER_SYNTAXES_NOT_SUPPORTED;
                
                memset ((char *)&(pres_result_list->pres_results[i].transfer_syntax),
                        0,
                        sizeof (rpc_cn_pres_syntax_id_t));
            } /* end if (!syntax_match) */
        } /* end else (st != rpc_s_unkwown_if) */
    } /* end for (i = 0; i < pres_cont_list->n_context_elem; i++) */

    *st = rpc_s_ok;
    RPC_LOG_CN_ASSOC_SYN_NEG_XIT;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_syntax_lkup_by_id
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will lookup the presentation context element by
**  matching the presentation context id.
**
**  INPUTS:             
**
**      assoc           The association being looked up.
**	context_id	The context id of the presentation context being looked up.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      pres_context    The pointer to the matching presentation
**                      context element.
**	st		The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_context_id_not_found
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

PRIVATE void rpc__cn_assoc_syntax_lkup_by_id 
(
  rpc_cn_assoc_p_t		assoc,
  unsigned32                      context_id,
  rpc_cn_syntax_p_t               *pres_context,
  unsigned32    		        *st
)
{
    RPC_LOG_CN_ASSOC_SYN_LKUP_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_syntax_lkup_by_id);
    CODING_ERROR(st);

    /*
     * Scan the syntax list on the association for a match on the
     * context id.
     */
    RPC_LIST_FIRST (assoc->syntax_list,
                    *pres_context,
                    rpc_cn_syntax_p_t);
    while (*pres_context != NULL)
    {
        /*
         * Compare the presentation context id in the syntax element
         * with the one passed in.
         */
        if ((*pres_context)->syntax_pres_id == context_id)
        {
            /*
             * We have a match. Return it.
             */
            *st = rpc_s_ok;
            return;
        }
        RPC_LIST_NEXT (*pres_context, *pres_context, rpc_cn_syntax_p_t);
    }
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                    ("CN: call_rep->%x assoc->%x desc->%x presentation context for context id given not found context_id->%x\n",
                     assoc->call_rep,
                     assoc,
                     assoc->cn_ctlblk.cn_sock,
                     context_id));
    *st = rpc_s_context_id_not_found;
    RPC_LOG_CN_ASSOC_SYN_LKUP_XIT;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_syntax_lkup_by_cl
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will lookup the presentation context element by
**  matching the call id.
**
**  INPUTS:             
**
**      assoc           The association being looked up.
**	call_id	        The call id of the presentation context being looked up.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      pres_context    The pointer to the matching presentation
**                      context element.
**	st		The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_call_id_not_found
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

PRIVATE void rpc__cn_assoc_syntax_lkup_by_cl 
(
rpc_cn_assoc_p_t		assoc,
unsigned32                      call_id,
rpc_cn_syntax_p_t               *pres_context,
unsigned32    		        *st
)
{
    RPC_LOG_CN_ASSOC_SYN_LKUP_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_syntax_lkup_by_cl);
    CODING_ERROR(st);

    /*
     * Scan the syntax list on the association for a match on the
     * context id.
     */
    RPC_LIST_FIRST (assoc->syntax_list,
                    *pres_context,
                    rpc_cn_syntax_p_t);
    while (*pres_context != NULL)
    {
        /*
         * Compare the presentation context id in the syntax element
         * with the one passed in.
         */
        if ((*pres_context)->syntax_call_id == call_id)
        {
            /*
             * We have a match. Return it.
             */
            *st = rpc_s_ok;
            return;
        }
        RPC_LIST_NEXT (*pres_context, *pres_context, rpc_cn_syntax_p_t);
    }
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                    ("CN: call_rep->%x assoc->%x desc->%x presentation context for call id given not found call_id->%x\n",
                     assoc->call_rep,
                     assoc,
                     assoc->cn_ctlblk.cn_sock,
                     call_id));
    *st = rpc_s_call_id_not_found;
    RPC_LOG_CN_ASSOC_SYN_LKUP_XIT;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_sec_lkup_by_id
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will lookup the security context element on an
**  association corresponding to the key ID given.
**
**  INPUTS:             
**
**      assoc           The association being looked up.
**	key_id	        The key ID of the security context being looked up.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      sec             The security context element.
**	st		The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_key_id_not_found
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

PRIVATE void rpc__cn_assoc_sec_lkup_by_id 
(
  rpc_cn_assoc_p_t		        assoc,
  unsigned32                              key_id,
  rpc_cn_sec_context_p_t                  *sec,
  unsigned32    		                *st
)
{
    rpc_cn_sec_context_t                *sec_context;
    
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_sec_lkup_by_id);
    CODING_ERROR(st);

    /*
     * Scan the security context list on the association for a match on the
     * context id.
     */
    RPC_LIST_FIRST (assoc->security.context_list,
                    sec_context,
                    rpc_cn_sec_context_p_t);
    while (sec_context != NULL)
    {
        /*
         * Compare the key ID in the security context element
         * with the one passed in.
         */
        if (sec_context->sec_key_id == key_id)
        {
            /*
             * We have a match. Return the pointer to the security
             * context element.
             */
            *sec = sec_context;
            *st = rpc_s_ok;
            return;
        }
        RPC_LIST_NEXT (sec_context, sec_context, rpc_cn_sec_context_p_t);
    }
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_SECURITY_ERRORS,
                    ("CN: call_rep->%x assoc->%x desc->%x no matching security context element for key id key_id->%x\n",
                     assoc->call_rep,
                     assoc,
                     assoc->cn_ctlblk.cn_sock,
                     key_id));
    *sec = NULL;
    *st = rpc_s_key_id_not_found;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_sec_lkup_by_cl
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**
**  This routine will lookup the security context element on an
**  association corresponding to the call id given.
**
**  INPUTS:
**
**      assoc           The association being looked up.
**	call_id	        The call ID of the security context being looked up.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      sec             The security context element.
**	st		The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_call_id_not_found
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

PRIVATE void rpc__cn_assoc_sec_lkup_by_cl
(
  rpc_cn_assoc_p_t		        assoc,
  unsigned32                            call_id,
  rpc_cn_sec_context_p_t                *sec,
  unsigned32    		        *st
)
{
    rpc_cn_sec_context_t                *sec_context;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_sec_lkup_by_cl);
    CODING_ERROR(st);

    /*
     * Scan the security context list on the association for a match on the
     * context id.
     */
    RPC_LIST_FIRST (assoc->security.context_list,
                    sec_context,
                    rpc_cn_sec_context_p_t);
    while (sec_context != NULL)
    {
        /*
         * Compare the key ID in the security context element
         * with the one passed in.
         */
        if (sec_context->sec_last_call_id == call_id)
        {
            /*
             * We have a match. Return the pointer to the security
             * context element.
             */
            *sec = sec_context;
            *st = rpc_s_ok;
            return;
        }
        RPC_LIST_NEXT (sec_context, sec_context, rpc_cn_sec_context_p_t);
    }
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_SECURITY_ERRORS,
                    ("CN: call_rep->%x assoc->%x desc->%x no matching security context element for call id call_id->%x\n",
                     assoc->call_rep,
                     assoc,
                     assoc->cn_ctlblk.cn_sock,
                     call_id));
    *sec = NULL;
    *st = rpc_s_call_id_not_found;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_post_error
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine is called when the network receiver thread
**  encounters a fatal local error. Errors with the connection
**  are sent through the association state machine. This
**  routine will close the connection, post a no connection indication
**  event to the association state machine and mark the association with
**  the appropriate status code
**
**  INPUTS:             
**
**      assoc           The association.
**      st              The error encountered.
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

PRIVATE void rpc__cn_assoc_post_error 
(
  rpc_cn_assoc_p_t        assoc,
  unsigned32              st
)
{
    unsigned32          local_st;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_post_error);

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("(rpc__cn_assoc_post_error) st->%08x cn_state->%d cur_state->%d\n",
                     st, assoc->cn_ctlblk.cn_state, assoc->assoc_state.cur_state));

    if ((assoc->cn_ctlblk.cn_state != RPC_C_CN_CLOSED)
        &&
        (assoc->assoc_state.cur_state != RPC_C_SM_CLOSED_STATE))
    {
        /*
         * First close the connection on the association. We will ignore
         * any error status from this routine. 
         */
	if (st != rpc_s_connection_closed) {
            rpc__cn_network_close_connect (assoc, &local_st);
	}

        /*
         * Post a no connection indication event to the association
         * state machine. This will transition the association into the
         * correct state and set the association message list condition
         * variable. Setting this will wake up any threads blocked
         * waiting for receive data.
         *
         * Make sure to clear the association status code first
         * so this event will be processed.
         */
        assoc->assoc_status = rpc_s_ok;
        RPC_CN_ASSOC_EVAL_USER_EVENT (assoc, 
                                      RPC_C_ASSOC_NO_CONN_IND,
                                      NULL,
                                      st);
    }
    else
    {
        return;
    }

    /*
     * Mark the association with the status code passed in as an
     * argument.
     */
    assoc->assoc_status = st;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_sm_protocol_error
**
**  SCOPE:              PRIVATE
**
**  DESCRIPTION:
**
**  Action routine invoked when an illegal transition is detected.
**  This routine writes an error message to stdout and DIEs.
**
**  INPUTS:
**
**      spc_struct      The special structure which is passed to the
**                      state machine event evaluation routine.
**			This is assumed to be the assoc.
**
**      event_param     The event specific argument.
**
**  INPUTS/OUTPUTS:
**      sm              The control block from the event
**                      evaluation routine.  Input is the current
**                      status and event for the control block.
**                      Output is the next state or updated
**                      current state, for the control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     unsigned32
**
**  SIDE EFFECTS:       output is printed on stdout.
**
**--
**/
PRIVATE unsigned32 rpc__cn_assoc_sm_protocol_error 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_t      *assoc;

    assoc = (rpc_cn_assoc_t *) spc_struct;

    /*
     * "Illegal state transition detected in CN {server|client} association
     * state machine [cur_state: %s, cur_event: %s, assoc: %x]"
     */
    if (assoc->assoc_flags & RPC_C_CN_ASSOC_SERVER)
    {
        RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s%s%x"),
            rpc_svc_cn_state,
            svc_c_sev_fatal | svc_c_action_abort,
            rpc_m_cn_ill_state_trans_sa,
rpc_g_cn_assoc_server_states[assoc->assoc_state.cur_state-RPC_C_CN_STATEBASE],
rpc_g_cn_assoc_server_events[assoc->assoc_state.cur_event-RPC_C_CN_STATEBASE],
            assoc ));
    }
    else
    {
        RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s%s%x"),
            rpc_svc_cn_state,
            svc_c_sev_fatal | svc_c_action_abort,
            rpc_m_cn_ill_state_trans_ca,
rpc_g_cn_assoc_client_states[assoc->assoc_state.cur_state-RPC_C_CN_STATEBASE],
rpc_g_cn_assoc_client_events[assoc->assoc_state.cur_event-RPC_C_CN_STATEBASE],
            assoc ));
    }
	/* FIXME: what should be returned ? */
	 return 0;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_status_to_prej
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  This routine will translate a local status code into a
**  presentation provider bind NACK reason code.
**
**  INPUTS:
**
**      st              The local status code.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      unsigned32      The presentation provider reason code.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__cn_assoc_status_to_prej 
(
unsigned32      st
)
{
    switch ((int)st)
    {
        case rpc_s_assoc_grp_max_exceeded:
        case RPC_S_HEADER_FULL:
        return (RPC_C_CN_PREJ_LOCAL_LIMIT_EXCEEDED);
        
        case rpc_s_rpc_prot_version_mismatch:
        return (RPC_C_CN_PREJ_PROTOCOL_VERSION_NOT_SUPPORTED);
        
        default:
        return (RPC_C_CN_PREJ_REASON_NOT_SPECIFIED);
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_prej_to_status
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  This routine will translate a presentation provider bind NACK
**  reason code into a local status code.
**  
**
**  INPUTS:
**
**      prej      The presentation provider reason code.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      unsigned32      The local status code.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__cn_assoc_prej_to_status 
(
unsigned32      prej
)
{
    switch ((int)prej)
    {
        case RPC_C_CN_PREJ_LOCAL_LIMIT_EXCEEDED:
        case RPC_C_CN_PREJ_TEMPORARY_CONGESTION:
        return (rpc_s_server_too_busy);

        case RPC_C_CN_PREJ_PROTOCOL_VERSION_NOT_SUPPORTED:
        return (rpc_s_rpc_prot_version_mismatch);

	/* Win 2003 server (and possibly other Microsoft products) return
	 * a bind_nak with reject reason 0 (reason not specified) if a second
	 * bind session is created to the same named pipe from a different
	 * tcp connection.
	 *
	 * To work around this issue, we treat it like the server rejected the
	 * bind because there are too many remote connections. This causes
	 * the bind to be retried, or an existing binding is reused when it
	 * becomes free (which works for win 2003 server).
	 */
        case RPC_C_CN_PREJ_REASON_NOT_SPECIFIED:
        return (rpc_s_too_many_rem_connects);

        default:
        return (rpc_s_unknown_reject);
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_call_pprov_to_status
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  This routine will translate a presentation provider bind NACK
**  reason code into a local status code.
**  
**
**  INPUTS:
**
**      pprov      The presentation provider reason code.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      unsigned32      The local status code.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__cn_assoc_pprov_to_status 
(
unsigned32      pprov
)
{
    switch ((int)pprov)
    {
        case RPC_C_CN_PPROV_LOCAL_LIMIT_EXCEEDED:
        return (rpc_s_server_too_busy);

        case RPC_C_CN_PPROV_PROPOSED_TRANSFER_SYNTAXES_NOT_SUPPORTED:
        return (rpc_s_tsyntaxes_unsupported);

        case RPC_C_CN_PPROV_ABSTRACT_SYNTAX_NOT_SUPPORTED: 
        return (rpc_s_unknown_if);

        case RPC_C_CN_PPROV_REASON_NOT_SPECIFIED:
        default:
        return (rpc_s_unknown_reject);
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_open
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This routine will send an association request event through
**  the association state machine. This routine will then wait
**  until the association request has been accepted. It will
**  then allocate the association and record the negotiated
**  transfer syntax. 
**
**  INPUTS:             
**
**      assoc           The association.
**      call_r          The call rep which will hold the binding and
**                      interface reps for the state machine action routines.
**      binding_r       The binding rep containing the server address.
**      if_r            The interface specification rep containing 
**                      the abstract syntax and supported
**                      transfer syntaxes.
**      info            The authentication information structure
**                      containing the per-principal-pair security
**                      information, NULL if none. 
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      syntax          The negotiated transfer syntax.
**      context_id      The transfer syntax's context id.
**      sec             The negotiated security context element,
**                      NULL if none.
**      st              The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_tsyntaxes_unsupported
**                      rpc_s_unknown_if
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

INTERNAL void rpc__cn_assoc_open 
(
  rpc_cn_assoc_p_t        assoc,
  rpc_addr_p_t            rpc_addr,
  rpc_if_rep_p_t          if_r,
  rpc_cn_local_id_t       grp_id,
  rpc_auth_info_p_t       auth_info,
  rpc_transport_info_p_t  transport_info,
  rpc_transfer_syntax_t   *syntax,
  unsigned16              *context_id,
  rpc_cn_sec_context_p_t  *sec,
  unsigned32              *st
)
{
    rpc_cn_syntax_t             *pres_context;
    rpc_cn_sec_context_t        *sec_context;
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_assoc_sm_work_t      assoc_sm_work;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_open);
    CODING_ERROR (st);
    
    /*
     * Init the work structure needed by the action routine.
     */
    memset (&assoc_sm_work, 0, sizeof (rpc_cn_assoc_sm_work_t));

    /*
     * Allocate and init a presentation context element and add it
     * to the tail of the association presentation context element
     * list. Also, put a pointer to it in the state machine work structure.
     */
    pres_context = rpc__cn_assoc_syntax_alloc (if_r, st);
    if (*st != rpc_s_ok)
    {
        return;
    }
    RPC_LIST_ADD_TAIL (assoc->syntax_list, pres_context, rpc_cn_syntax_p_t);
    assoc_sm_work.pres_context = pres_context;

    /*
     * Determine whether security is required for the RPC being
     * executed.
     */
    if (RPC_CN_AUTH_REQUIRED (auth_info))
    {
        /*
         * Allocate and init a security context element and add it
         * to the tail of the association security context element
         * list. Also, put a pointer to it in the state machine work structure.
         */
        sec_context = rpc__cn_assoc_sec_alloc (auth_info, st);
        if (*st != rpc_s_ok)
        {
            RPC_LIST_REMOVE (assoc->syntax_list, pres_context);
            rpc__cn_assoc_syntax_free (&pres_context);
            return;
        }
        RPC_LIST_ADD_TAIL (assoc->security.context_list, sec_context, rpc_cn_sec_context_p_t);
        assoc_sm_work.sec_context = sec_context;
    }
    else
    {
        sec_context = NULL;
    }

    /*
     * Set the association group ID in the state machine work
     * structure.
     */
    assoc_sm_work.grp_id = grp_id.all;

    /*
     * Record the address to which the connection is being made in
     * the association.
     */


    /*
     *  Here is another one of those marvelous chances for the rpc_addr to
     *  become a dangling reference.  We make sure that this association
     *  has it's own rpc_addr memory.
    */
    {
        rpc_addr_p_t    rpc_addr1;

        rpc__naf_addr_copy(rpc_addr, &rpc_addr1, st);

        if ( *st != rpc_s_ok )
            return;

        assoc->cn_ctlblk.rpc_addr = rpc_addr1;
    }

    assoc->transport_info = transport_info;

    if (transport_info)
    {
        rpc__transport_info_retain(transport_info);
    }

    /*
     * This seems redundant, but we need this structure in the association
     * in case we have to retry the bind due to protocol version mismatch.
     */
    assoc->assoc_sm_work = &assoc_sm_work;

    /*
     * Send an association request through the association state
     * machine. This will create a transport connection and return
     * an association in the open state with the transport syntax
     * and optional security context negotiated.
     */
    RPC_CN_ASSOC_EVAL_USER_EVENT (assoc, 
                                  RPC_C_ASSOC_REQ,
                                  &assoc_sm_work,
                                  *st);

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_ASSOC_REQ_FAIL))
    {
        assoc->assoc_status = RPC_S_CN_DBG_FAILURE;
    }
#endif

    *st = assoc->assoc_status;
    if (*st != rpc_s_ok)
    {
        RPC_LIST_REMOVE (assoc->syntax_list, pres_context);
        rpc__cn_assoc_syntax_free (&pres_context);
        if (RPC_CN_AUTH_REQUIRED (auth_info))
        {
            RPC_LIST_REMOVE (assoc->security.context_list, sec_context);
            rpc__cn_assoc_sec_free (&sec_context);
        }
        return;
    }

    /*
     * Wait for both presentation and optional security context
     * negotiations to complete either successfully or with an error.
     */
    while (!(pres_context->syntax_valid) 
           ||
           (RPC_CN_AUTH_REQUIRED (auth_info) && (sec_context->sec_state != RPC_C_SEC_STATE_COMPLETE)))
    {
        rpc__cn_assoc_receive_frag (assoc, &fragbuf, st);
        if (*st != rpc_s_ok)
        {
            return;
        }

        /*
         * If either the presentation or security negotiations
         * complete with an error return.
         */
        if (pres_context->syntax_status != rpc_s_ok)
        {
            *st = pres_context->syntax_status;
            return;
        }
        if (RPC_CN_AUTH_REQUIRED (auth_info) && (sec_context->sec_status != rpc_s_ok))
        {
            *st = sec_context->sec_status;
            return;
        }
        if (RPC_CN_AUTH_REQUIRED (auth_info) && (sec_context->sec_state == RPC_C_SEC_STATE_INCOMPLETE))
        {
            RPC_CN_ASSOC_EVAL_USER_EVENT (assoc,
                                          RPC_C_ASSOC_ALTER_CONTEXT_REQ,
                                          &assoc_sm_work,
                                          *st);
        }
    }

    /*
     * The negotiations have completed. Copy out the results. Note
     * that the presentation syntax UUID is not copied. The client
     * stub does not require it.
     */
    *sec = sec_context;
    syntax->index = pres_context->syntax_vector_index;
    syntax->convert_epv = pres_context->syntax_epv;
    *context_id = pres_context->syntax_pres_id;

    /*
     * Mark the call_rep PDU header with the negotiated minor
     * version number.
     */
    RPC_CN_PKT_VERS_MINOR ((rpc_cn_packet_p_t) 
        RPC_CN_CREP_SEND_HDR (assoc->call_rep)) = assoc->assoc_vers_minor;

    *st = rpc_s_ok;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_alter_context
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This routine will either locate a previously established or
**  estblish a new presentation and optional security context for the
**  current RPC. 
**
**  INPUTS:             
**
**      assoc           The association.
**      if_r            The interface specification rep containing 
**                      the abstract syntax and supported
**                      transfer syntaxes.
**      info            The authentication information structure
**                      containing the per-principal-pair security
**                      information, NULL if none. 
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      syntax          The negotiated transfer syntax.
**      context_id      The transfer syntax's context id.
**      sec             The negotiated security context element,
**                      NULL if none.
**      st              The return status of this routine.
**                      rpc_s_ok
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

INTERNAL void rpc__cn_assoc_alter_context 
(
rpc_cn_assoc_p_t        assoc,
rpc_if_rep_p_t          if_r,
rpc_auth_info_p_t       info,
rpc_transfer_syntax_t   *syntax,
unsigned16              *context_id,
rpc_cn_sec_context_p_t  *sec,
unsigned32              *st
)
{
    rpc_cn_syntax_t             *pres_context;
    rpc_cn_sec_context_t        *sec_context;
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_assoc_sm_work_t      assoc_sm_work;
    boolean                     pres_context_setup;
    boolean                     sec_context_setup = false;
    boolean32                   sec_cred_changed = false;
    rpc_cn_assoc_grp_t          * volatile assoc_grp = NULL;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_alter_context);
    CODING_ERROR (st);

    /*
     * Clear local booleans to indicate whether a presentation context and
     * optional security context are set up.
     */
    pres_context_setup = false;

    /*
     * Scan all previously negotiated presentation contexts.
     */
    RPC_LIST_FIRST (assoc->syntax_list, pres_context, rpc_cn_syntax_p_t);
    while (pres_context != NULL)
    {
        /*
         * If the interface UUID and version in the input argument
         * ifspec match the presentation context abstract UUID and
         * version this presentation context can be use for the
         * current RPC.
         */
#if (uuid_c_version == 1)
        if ((memcmp (&pres_context->syntax_abstract_id.id, 
                     &if_r->id,
                     sizeof (dce_uuid_t)) == 0)
#else
***Make sure memcmp works on this version of UUIDs***
#endif
            &&
            (pres_context->syntax_abstract_id.version == if_r->vers))
        {
            /*
             * Handle the case where a previous negotiation was
             * orphaned and failed with a presentation provider reject
             * (an rpc_bind_ack or rpc_alter_context_response was returned).
             */
            if (pres_context->syntax_status != rpc_s_ok)
            {
                *st = pres_context->syntax_status;
                return;
            }
            pres_context_setup = true;
            break;
        }
        RPC_LIST_NEXT (pres_context, pres_context, rpc_cn_syntax_p_t);
    }

    /*
     * Determine whether the current RPC requires security.
     */
    if (RPC_CN_AUTH_REQUIRED (info))
    {
        /*
         * Scan all previously negotiated security contexts.
         */
        RPC_LIST_FIRST (assoc->security.context_list, sec_context, rpc_cn_sec_context_p_t);
        while (sec_context != NULL)
        {
            /*
             * If the auth info passed in equals the auth info in
             * the security context element this security context can
             * be use for the current RPC.
             */
            if (info == sec_context->sec_info)
            {
                /*
                 * This security context element matches the auth
                 * info we're looking for, which means the client and
                 * server principal IDs as well as the authn protocol
                 * match. We'll at least reuse the memory for this
                 * structure even if we have to send an
                 * rpc_alter_context PDU to update it.
                 */

                /*
                 * Determine whether the credentials have been
                 * refreshed since this security context was set up.
                 * If so, we'll have to send an rpc_alter_context PDU
                 * to update the security context with the new
                 * credentials. 
                 */
                sec_cred_changed = RPC_CN_AUTH_CRED_CHANGED (sec_context, st);
                break;
            }
            RPC_LIST_NEXT (sec_context, sec_context, rpc_cn_sec_context_p_t);
        }
    }
    else
    {
        sec_context = NULL;
    }

    /*
     * Determine whether either a presentation or security
     * context negotiation has to occur.
     */
    if ((!pres_context_setup)
        ||
        (RPC_CN_AUTH_REQUIRED (info) && (sec_context == NULL))
        ||
        (RPC_CN_AUTH_REQUIRED (info) && (sec_cred_changed)))
    {
        /*
         * Init the work structure needed by the action routine.
         */
        memset (&assoc_sm_work, 0, sizeof (rpc_cn_assoc_sm_work_t));
        
        if (!pres_context_setup)
        {
            /*
             * We didn't find a presentation context element on
             * the association for the current RPC. Allocate and
             * init a presentation context element and add it 
             * to the tail of the association presentation context element
             * list. Also, put a pointer to it in the state machine work structure.
             */
            pres_context = rpc__cn_assoc_syntax_alloc (if_r, st);
            if (*st != rpc_s_ok)
            {
                return;
            }
            RPC_LIST_ADD_TAIL (assoc->syntax_list, pres_context, rpc_cn_syntax_p_t);
            assoc_sm_work.pres_context = pres_context;
        }
        else
        {
            /* MSRPC seems to expect a context negotiation even if we aren't
               changing the presentation context, so just send what we already
               have */
            assoc_sm_work.pres_context = pres_context;
            assoc_sm_work.reuse_context = TRUE;
        }
        

        /*
         * If authentication is required on this call, we may have
         * to send an rpc_alter_context PDU. The only time one doesn't
         * have to be sent is when a security context has been
         * established which has valid credentials and has been
         * successfully established previously.
         */
        if (RPC_CN_AUTH_REQUIRED (info))
        {
            /*
             * If the security context pointer is NULL we didn't
             * find a match above. This means we need to allocate
             * memory for a new security context element and send an
             * rpc_alter_context PDU to set it up.
             */
            if (sec_context == NULL)
            {
                /*
                 * Allocate and init a security context element and add it
                 * to the tail of the association security context element
                 * list. Also, put a pointer to it in the state machine work structure.
                 */
                sec_context = rpc__cn_assoc_sec_alloc (info, st);
                if (*st != rpc_s_ok)
                {
                    if (!pres_context_setup)
                    {
                        RPC_LIST_REMOVE (assoc->syntax_list, pres_context);
                        rpc__cn_assoc_syntax_free (&pres_context);
                    }
                    return;
                }
                sec_context_setup = false;
                RPC_LIST_ADD_TAIL (assoc->security.context_list, sec_context, rpc_cn_sec_context_p_t);
                assoc_sm_work.sec_context = sec_context;
            }
            else
            {
                sec_context_setup = true;
            }

            /*
             * The indication that a previous security context setup
             * has failed is when the status code is not rpc_s_ok in
             * it. This condition, or a credential change, means that
             * an rpc_alter_context PDU should be sent to update the
             * existing security context.
             */
            if ((sec_context->sec_status != rpc_s_ok)
                ||
                sec_cred_changed)
            { 
                assoc_sm_work.sec_context = sec_context;
            }
        }

        assoc_grp = RPC_CN_ASSOC_GRP(assoc->assoc_grp_id);
        assoc_sm_work.grp_id = assoc_grp->grp_remid.all;

        /*
         * Evaluate an alter_context event through the client
         * association state machine. This will result in an
         * alter_context PDU being transmitted to the server.
         */
        RPC_CN_ASSOC_EVAL_USER_EVENT (assoc, 
                                      RPC_C_ASSOC_ALTER_CONTEXT_REQ,
                                      &assoc_sm_work,
                                      *st);
        *st = assoc->assoc_status;
        if (*st != rpc_s_ok)
        {
            if (!pres_context_setup)
            {
                RPC_LIST_REMOVE (assoc->syntax_list, pres_context);
                rpc__cn_assoc_syntax_free (&pres_context);
            }

            if (RPC_CN_AUTH_REQUIRED (info) && (!sec_context_setup))
            {
                RPC_LIST_REMOVE (assoc->security.context_list, sec_context);
                rpc__cn_assoc_sec_free (&sec_context);
            }
            return;
        }

    }        

    /*
     * Wait for both presentation and optional security context
     * negotiations to complete either successfully or with an error.
     */
    while (!(pres_context->syntax_valid) 
           ||
           (RPC_CN_AUTH_REQUIRED (info) && (sec_context->sec_state != RPC_C_SEC_STATE_COMPLETE)))
    {
        rpc__cn_assoc_receive_frag (assoc, &fragbuf, st);
        if (*st != rpc_s_ok)
        {
            return;
        }
        /*
         * If either the presentation or security negotiations
         * complete with an error return.
         */
        if (pres_context->syntax_status != rpc_s_ok)
        {
            *st = pres_context->syntax_status;
            return;
        }
        if (RPC_CN_AUTH_REQUIRED (info) && (sec_context->sec_status != rpc_s_ok))
        {
            *st = sec_context->sec_status;
            return;
        }
    }

    /*
     * The negotiations have completed. Copy out the results. Note
     * that the presentation syntax UUID is not copied. The client
     * stub does not require it.
     */
    *sec = sec_context;
    syntax->index = pres_context->syntax_vector_index;
    syntax->convert_epv = pres_context->syntax_epv;
    *context_id = pres_context->syntax_pres_id;
    *st = rpc_s_ok;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_sec_alloc
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will allocate and intialize a security context element.
**
**  INPUTS:             
**
**      info            The per-principal-pair security information.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE rpc_cn_sec_context_t *rpc__cn_assoc_sec_alloc 
(
  rpc_auth_info_p_t       info,
  unsigned32              *st
)
{
    rpc_cn_sec_context_t        *sec_context;
    rpc_cn_auth_info_t          *cn_info;

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_SEC_ALLOC_FAIL))
    {
        *st = RPC_S_CN_DBG_FAILURE;
        return (NULL);
    }
#endif

    /*
     * Allocate a CN specific per-principal-pair security
     * information structure.
     */
    RPC_CN_AUTH_GET_PROT_INFO (info, &cn_info, st);
    if (*st != rpc_s_ok)
    {
        if (info->is_server)
        {
            dce_error_string_t error_text;
            int temp_status;

            dce_error_inq_text(*st, (unsigned char*) error_text, &temp_status);

	    /*
	     * rpc_m_call_failed_s
	     * "%s on server failed: %s"
	     */
            RPC_DCE_SVC_PRINTF ((
		DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
		rpc_svc_auth,
		svc_c_sev_error,
		rpc_m_call_failed_s,
		"RPC_CN_AUTH_GET_PROT_INFO",
		error_text ));
        }
        return (NULL);
    }
    cn_info->cn_epv = RPC_CN_AUTH_PROT_EPV (info->authn_protocol);

    /*
     * Allocate a security context element and init it.
     */
    sec_context = (rpc_cn_sec_context_t *) 
        rpc__list_element_alloc (&rpc_g_cn_sec_lookaside_list, true);
    if (sec_context == NULL) {
        *st = rpc_s_no_memory;
        return NULL;
    }
    sec_context->sec_state = RPC_C_SEC_STATE_INVALID;
    sec_context->sec_status = rpc_s_ok;
    sec_context->sec_info = info;
    sec_context->sec_cn_info = cn_info;

    /*
     * Note that the credentials themselves in the auth_info
     * structure were already checked for expiration in
     * rpc__cn_call_start() on the client side. It therefore isn't
     * necessary to do any more checking here since they were either
     * refreshed or an error was returned back in call_start.
     * 
     * On the server, the client has not been authenticated yet.
     * This security context may therefore be invalid.
     *
     * Add a reference to the auth information structure since a
     * pointer to it is being kept in the security context element.
     */
    RPC_CN_AUTH_ADD_REFERENCE (info);
    return (sec_context);
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_sec_free
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will free a security context element,
**  dereference the per-principal-pair security information, and free
**  the CN specific per-principal-pair security information.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     
**
**      sec             The security context element, NULL on output.
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

PRIVATE void rpc__cn_assoc_sec_free 
(
  rpc_cn_sec_context_p_t  *sec
)
{
    /*
     * Free the CN specific per-principal-pair security
     * information structure.
     */
    RPC_CN_AUTH_FREE_PROT_INFO ((*sec)->sec_info, &(*sec)->sec_cn_info);

    /*
     * Remove the reference to the auth information structure.
     */
    RPC_CN_AUTH_RELEASE_REFERENCE ((rpc_auth_info_p_t *) &(*sec)->sec_info);

    /*
     * Free the security context element.
     */
    rpc__list_element_free (&rpc_g_cn_sec_lookaside_list,
                            (pointer_t) (*sec));

    *sec = NULL;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_syntax_alloc
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This routine will allocate and intialize a presentation context element.
**
**  INPUTS:             
**
**      if_r            The interface spec.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

INTERNAL rpc_cn_syntax_t *rpc__cn_assoc_syntax_alloc 
(
  rpc_if_rep_p_t          if_r,
  unsigned32              *st
)
{
    rpc_cn_syntax_t             *pres_context;

    pres_context = 
        (rpc_cn_syntax_t *)rpc__list_element_alloc (&rpc_g_cn_syntax_lookaside_list,
                                                 true);
    if (pres_context == NULL) {
        *st = rpc_s_no_memory;
        return NULL;
    }
    pres_context->syntax_valid = false;
    pres_context->syntax_status = rpc_s_ok;
    pres_context->syntax_abstract_id.id = if_r->id;
    pres_context->syntax_abstract_id.version = if_r->vers;
    pres_context->syntax_vector = &if_r->syntax_vector;
    pres_context->syntax_epv = NULL;
    *st = rpc_s_ok;
    return (pres_context);
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_syntax_free
**
**  SCOPE:              PRIVATE - declated in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will free a presentation context element.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     
**
**      syntax          The presentation context element, NULL on output.
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

PRIVATE void rpc__cn_assoc_syntax_free 
(
  rpc_cn_syntax_p_t       *syntax
)
{
    /*
     * Free the presentation context element.
     */
    rpc__list_element_free (&rpc_g_cn_syntax_lookaside_list,
                            (pointer_t) *syntax);
    *syntax = NULL;
}

void rpc_close_idle_associations
(
    void
)
{
    rpc__cn_assoc_timer_reclaim((pointer_t) RPC_C_CN_ASSOC_GRP_CLIENT);
    rpc__cn_assoc_timer_reclaim((pointer_t) RPC_C_CN_ASSOC_GRP_SERVER);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_timer_reclaim
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This routine is called at timed intervals to close
**  associations which have been inactive.
**
**  INPUTS:             
**
**      type            The type of associations to reclaim.
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

INTERNAL void rpc__cn_assoc_timer_reclaim 
(
 pointer_t       type 
)
{
    rpc_cn_local_id_t   grp_id;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_timer_reclaim);

    /*
     * Reclaim associations of the given type. Provide an invalid
     * group id so that no group is off limits to the policy.
     */
    RPC_CN_LOCAL_ID_CLEAR (grp_id);
    RPC_CN_LOCK ();
    rpc__cn_assoc_reclaim (grp_id, (unsigned long) type, true);
    RPC_CN_UNLOCK ();
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_reclaim
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This routine is called to invoke the local policy algorithm
**  to determine which associations/connections should be closed.
**
**  INPUTS:             
**
**      grp_id          The association group id over which an open
**                      association is needed.
**      type            The type of associations to be reclaimed.
**
**      loop            If loop is false, we will stop after finding 
**                      the first association to shut down.
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

INTERNAL void rpc__cn_assoc_reclaim 
(
  rpc_cn_local_id_t       grp_id,
  unsigned32              type,
  boolean32		loop
)
{
    unsigned32          i;
    boolean		shutdown_or_abort = false;
    rpc_cn_assoc_t      *assoc;
    unsigned32          st;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_reclaim);

    /*
     * Check whether there are any active groups.
     */
    if (rpc_g_cn_assoc_grp_tbl.grp_active_count != 0)
    {
        /*
         * Scan all the association groups in the group table.
         */
        for (i = 0;
             i < rpc_g_cn_assoc_grp_tbl.grp_count; 
             i++)
        {
            /*
             * Check whether the group id given matches, the group is active
             * and of the right type.  Also, check if there is any active
             * context handle between a client and server (i.e. the association
             * will be allowed to shutdown if the group reference count
             * is zero or there is more than one association on the group).
             * Note that for future enhancement, we should implement the
             * checking on the activeness of the context handle as a predicate 
             * through server association FSM.
             */
	    if ((!RPC_CN_LOCAL_ID_EQUAL
		(rpc_g_cn_assoc_grp_tbl.assoc_grp_vector[i].grp_id, grp_id))
		&&
		(rpc_g_cn_assoc_grp_tbl.assoc_grp_vector[i].grp_flags & type)
		&&
		(rpc_g_cn_assoc_grp_tbl.assoc_grp_vector[i].grp_state.cur_state
		 == RPC_C_ASSOC_GRP_ACTIVE)
		&&
		(rpc_g_cn_assoc_grp_tbl.assoc_grp_vector[i].grp_refcnt == 0
		 ||
		 rpc_g_cn_assoc_grp_tbl.assoc_grp_vector[i].grp_cur_assoc > 1)
		)
	    {
                /*
                 * The group id doesn't match and the group is of the
                 * correct type and is active. Scan the associations on its
                 * list.
                 */
                RPC_LIST_FIRST 
		    (rpc_g_cn_assoc_grp_tbl.assoc_grp_vector[i].grp_assoc_list,
                    assoc,
                    rpc_cn_assoc_p_t);            
                while (assoc != NULL)
                {
                    /*
                     * Check whether the association is free for
                     * reclaimation. 
                     */
                    if (assoc->assoc_ref_count == 0)
                    {
                       /*
                         * Increment the association reference count so
                         * that this association won't be deallocated by
                         * the receiver thread when we try to send the
                         * shutdown request.
                         */
                        RPC_CN_ASSOC_ACB_INC_REF(assoc);

                        /*
                         * This association has no active references.
                         */
                        if (assoc->assoc_flags & RPC_C_CN_ASSOC_SCANNED)
                        {
                            /* 
                             * The association has been scanned previsouly.
                             * Check the shutdown request count to see if
                             * it reached the maximum allowed
                             */
                            if (0 /* Windows RPC runtime never sends shutdowns */ &&
                                assoc->assoc_shutdown_req_count <=
                                RPC_C_CN_ASSOC_SERVER_MAX_SHUTDOWN_REQ_COUNT)
                            {
                                /*
                                 * Haven't reached the maximum shutdown
                                 * request count, send an additional shutdown 
                                 * message to the client.  We do this by sending
                                 * a shutdown request through its state machine
                                 * and increase the shutdown request count.
                                 */
                                RPC_CN_ASSOC_EVAL_USER_EVENT (assoc, 
                                                              RPC_C_ASSOC_SHUTDOWN_REQ,
                                                              NULL,
                                                              st);
                                assoc->assoc_shutdown_req_count++ ;
				shutdown_or_abort = true;
                            }
                            else
                            {
                                /*
                                 * Architecture spec requires that a client 
                                 * responds to a shutdown message.  If we get
                                 * no response from repeated shutdowns, we 
                                 * assume that the client is not working 
                                 * properly and abort the association.
                                 *
                                 * We have sent the client the maximum number
                                 * of shutdown requests.  The client has not
                                 * initiated an orderly shutdown process.  The
                                 * client might be hung.  It's time to abort
                                 * the association!
                                 */
                                rpc__cn_assoc_abort (assoc, &st);
				shutdown_or_abort = true;
                            }
                        }
                        else
                        {
                            /*
                             * The association has no active
                             * references but also was not previously
                             * scanned. Mark it as scanned now.
                             */
                            assoc->assoc_flags |= RPC_C_CN_ASSOC_SCANNED;
                        }

                        /*
                         * Done with the shutdown attempt, so decrement the
                         * reference count. This is done via the acb dealloc
                         * routine.
                         */
                        rpc__cn_assoc_acb_dealloc (assoc);
                    }
		    if (loop == false && shutdown_or_abort == true)
			return;
                    RPC_LIST_NEXT (assoc, assoc, rpc_cn_assoc_p_t);
                } /* end while (assoc != NULL) */
            } /* end if ((!RPC_CN_LOCAL_ID (...)) */
        } /* end for (i = 0; ... ) */
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_acb_alloc
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  This routine will create and initialize, if needed, an
**  association control block by allocating one off the
**  association lookaside list. 
**
**  INPUTS:
**
**      wait            A boolean indicating whether this routine is
**                      allowed to block waiting for memory if necessary.
**      type            The type of association requested (client or server).
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      assoc           The initialized association.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL rpc_cn_assoc_t *rpc__cn_assoc_acb_alloc 
(
boolean32       wait,
unsigned32      type,
unsigned32      *st
)
{
    rpc_cn_assoc_t      *assoc;
    
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_acb_alloc);
    CODING_ERROR(st);

    /*
     * Allocate an association control block. The returned block
     * will have a receiver thread assigned to it which will be
     * blocked on a the connection condition variable contained in the
     * association. Once a connection has been established this
     * condition variable will be set and the receiver thread will
     * begin waiting for data on the connection.
     */
    assoc = (rpc_cn_assoc_t *) 
        rpc__list_element_alloc (&rpc_g_cn_assoc_lookaside_list, wait);
    if (assoc == NULL) {
        *st = rpc_s_no_memory;
        return (NULL);
    }

    assoc->cn_ctlblk.rpc_addr = NULL;

    /*
     * Find out what type of association is being created and
     * initialize the state tables in the association accordingly.
     */
    if (type == RPC_C_CN_ASSOC_CLIENT)
    {
        /*
         * Initialize the association with the client tables.
         */
        rpc__cn_sm_init (rpc_g_cn_client_assoc_sm,
                         rpc_g_cn_client_assoc_act_tbl,
                         &(assoc->assoc_state),
			 rpc_c_cn_cl_assoc );
    }
    else
    {
        /*
         * Initialize the association with the server tables.
         */
        rpc__cn_sm_init (rpc_g_cn_server_assoc_sm,
                         rpc_g_cn_server_assoc_act_tbl,
                         &(assoc->assoc_state),
			 rpc_c_cn_svr_assoc );
    }
    assoc->assoc_flags |= type;

    /*
     * We use our version number until we negotiate different.
     */
    assoc->assoc_vers_minor = RPC_C_CN_PROTO_VERS_MINOR;

    *st = rpc_s_ok;
    return (assoc);
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_acb_dealloc
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**
**  This routine will return an association control block to the
**  association lookaside list.
**
**  INPUTS:
**
**      assoc           The association to be freed.
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

PRIVATE void rpc__cn_assoc_acb_dealloc 
(
  rpc_cn_assoc_p_t   assoc
)
{
    rpc_cn_fragbuf_t            *fragbuf;
    rpc_cn_syntax_t             *pres_context;
    rpc_cn_sec_context_t        *sec_context;

    RPC_LOG_CN_ASSOC_ACB_DEAL_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_acb_dealloc);

    RPC_CN_ASSOC_ACB_DEC_REF (assoc);

    if (assoc->assoc_acb_ref_count == 0)
    {
        /*
         * Clear the pointer to the endpoint of the socket
         * descriptor contained in the connection control block.
         * Note that this memory is NOT freed. It is used by all
         * connections which come in on the descriptor in the
         * select pool and as such cannot be freed until this
         * descriptor is removed (there is no way to remove it right
         * now.) 
         */
        assoc->cn_ctlblk.cn_listening_endpoint = NULL;        

        /*
         * Free all fragment buffers on the association message list
         * and clear the waiters field.
         */
        assoc->assoc_msg_waiters = 0;
        RPC_LIST_FIRST (assoc->msg_list, 
                        fragbuf,
                        rpc_cn_fragbuf_p_t);            
        while (fragbuf != NULL)
        {
            rpc_cn_fragbuf_t    *next_fragbuf;

            RPC_LIST_NEXT (fragbuf,
                           next_fragbuf,
                           rpc_cn_fragbuf_p_t);            
            if (fragbuf->fragbuf_dealloc != NULL)
            {
                (*fragbuf->fragbuf_dealloc)(fragbuf);
            }
            fragbuf = next_fragbuf;
        }
        RPC_LIST_INIT (assoc->msg_list);
             
        /*
         * Free all the syntax elements on the association syntax
         * list.
         */
        RPC_LIST_FIRST (assoc->syntax_list, 
                        pres_context,
                        rpc_cn_syntax_p_t);            
        while (pres_context != NULL)
        {
            rpc_cn_syntax_t     *next_pres_context;

            RPC_LIST_NEXT (pres_context, next_pres_context, rpc_cn_syntax_p_t);
            rpc__cn_assoc_syntax_free (&pres_context);
            pres_context = next_pres_context;
        }
        RPC_LIST_INIT (assoc->syntax_list);

        /*
         * Free all the security context elements.
         */
        RPC_LIST_FIRST (assoc->security.context_list, 
                        sec_context,
                        rpc_cn_sec_context_p_t);            
        while (sec_context != NULL)
        {
            rpc_cn_sec_context_t        *next_sec_context;

            RPC_LIST_NEXT (sec_context, next_sec_context, rpc_cn_sec_context_p_t);
            rpc__cn_assoc_sec_free (&sec_context);
            sec_context = next_sec_context;
        }
        RPC_LIST_INIT (assoc->security.context_list);

        memset (&assoc->security, 0, sizeof (rpc_cn_assoc_sec_context_t));

        /*
         * Free the call rep on the assoc.
         */
        if (assoc->call_rep != NULL)
        {
            /*
             * If this assoc is pointing to a call_rep that doesn't point
             * back to it then this assoc is probably a client assoc that
             * failed the presentation negotiation sequence and the call
             * rep already went back to the heap.  Hence this call_rep is
             * probably paired with another association at this time and
             * so it would be wrong to release the assoc from it.
             */
            if ( assoc->call_rep->assoc == assoc )
                assoc->call_rep->assoc = NULL;
            assoc->call_rep = NULL;
        }

        
        rpc__transport_info_release(assoc->transport_info);
        assoc->transport_info = NULL;
        assoc->cn_ctlblk.cn_state = RPC_C_SM_CLOSED_STATE;
        assoc->assoc_status = rpc_s_ok;
        assoc->assoc_local_status = rpc_s_ok;
        RPC_CN_LOCAL_ID_CLEAR (assoc->assoc_grp_id);
        assoc->assoc_flags = 0;
        RPC_CN_ASSOC_CONTEXT_ID (assoc) = 0;
        assoc->assoc_max_xmit_frag = 0;
        assoc->assoc_max_recv_frag = 0;
        assoc->assoc_vers_minor = RPC_C_CN_PROTO_VERS_MINOR;
        assoc->assoc_ref_count = 0;
        assoc->assoc_shutdown_req_count = 0;
        assoc->cn_ctlblk.exit_rcvr = false;
        assoc->cn_ctlblk.in_sendmsg = false;
        assoc->cn_ctlblk.waiting_for_sendmsg_complete = false;
        assoc->assoc_sm_work = NULL;

        /*
         * Free the rpc addr memory of the assoc.
         */
	{
	    error_status_t st;

	    if (assoc->cn_ctlblk.rpc_addr)
	        rpc__naf_addr_free (&assoc->cn_ctlblk.rpc_addr, &st);

	    assoc->cn_ctlblk.rpc_addr = NULL;
        }

        /*
         * Finally free the association to the association lookaside
         * list.
         */
        rpc__list_element_free (&rpc_g_cn_assoc_lookaside_list,
                                (pointer_t) assoc);
    }
    RPC_LOG_CN_ASSOC_ACB_DEAL_XIT;
}

/******************************************************************************/


/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_acb_create
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**
**  This routine will initialize an association control block
**  which is allocated from heap by rpc__list_element_alloc. It
**  will create the mutexes and condition variables as well as
**  create the receiver thread.
**
**  INPUTS:
**
**      assoc           The association control block to be initialized.
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

PRIVATE void rpc__cn_assoc_acb_create 
(
  rpc_cn_assoc_p_t        assoc
)
{
    RPC_LOG_CN_ASSOC_ACB_CR_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_acb_create);
    
    memset (assoc, 0, sizeof (rpc_cn_assoc_t));
    assoc->alter_call_id = -1;
    RPC_COND_INIT (assoc->cn_ctlblk.cn_rcvr_cond, rpc_g_global_mutex); 
    RPC_COND_INIT (assoc->assoc_msg_cond, rpc_g_global_mutex);

    /*
     * Create the receiver thread.
     */
    RPC_DBG_PRINTF (rpc_e_dbg_threads, RPC_C_CN_DBG_THREADS,
        ( "####### assoc->%x Created receiver thread\n", assoc ));

    DCETHREAD_TRY {
    dcethread_create_throw (&(assoc->cn_ctlblk.cn_rcvr_thread_id),
                    &rpc_g_default_dcethread_attr,
                    (dcethread_startroutine) rpc__cn_network_receiver,
                    (dcethread_addr) assoc);
    } DCETHREAD_CATCH_ALL(THIS_CATCH) {
        DCETHREAD_RERAISE;
    } DCETHREAD_ENDTRY

    RPC_LOG_CN_ASSOC_ACB_CR_XIT;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_acb_free
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**
**  This routine will free the mutexes, condition variables and
**  receiver thread contained in an association control block before the
**  rpc__list_element_free routine returns it to heap storage.
**
**  INPUTS:
**
**      assoc           The association control block being freed.
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

PRIVATE void rpc__cn_assoc_acb_free 
(
 rpc_cn_assoc_p_t        assoc
)
{
    rpc_cn_ctlblk_t     * volatile ccb;
    dcethread*          current_thread_id;
    void *		dcethread_exit_status;
    int                 prev_cancel_state;

    //DO_NOT_CLOBBER(ccb);

    RPC_LOG_CN_ASSOC_ACB_FR_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_acb_free);
    
    /*
     * Get a pointer to the connection control block contained in
     * the association control block.
     */
    ccb = &assoc->cn_ctlblk;

#ifdef notdef
    if (ccb->cn_rcvr_thread_id == (dcethread*) NULL)
    {
        RPC_LOG_CN_ASSOC_ACB_FR_XIT;
        return;
    }
#endif

    /*
     * Determine whether we are now running in the receiver thread.
     */
    current_thread_id = dcethread_self();
    if (dcethread_equal (current_thread_id, ccb->cn_rcvr_thread_id) )
    {
        /*
         * We are the receiver thread.
         * Send a cancel to ourselves so we will exit out of
         * the while loop in rpc__cn_network_receiver() without
         * having to reference the assoc which are going to
         * (possibly) free soon.
         */
        RPC_COND_DELETE (ccb->cn_rcvr_cond, rpc_g_global_mutex);
        RPC_COND_DELETE (assoc->assoc_msg_cond, rpc_g_global_mutex);
        ccb->exit_rcvr = true;
        dcethread_detach_throw (ccb->cn_rcvr_thread_id);
        dcethread_interrupt_throw (ccb->cn_rcvr_thread_id);
    }
    else
    {
        /*
         * The receiver thread is now running with the CN global mutex
         * locked. We don't want to blow away anything in the association
         * or connection control block until we're sure the receiver
         * thread has terminated. To ensure this send the receiver thread a
         * cancel and then wait until it exits by using the
         * dcethread_join() function. This function will not return
         * until the receiver thread has terminated.
         */
        ccb->exit_rcvr = true;
        dcethread_interrupt_throw (ccb->cn_rcvr_thread_id);

        /*
         * Since this is a cancellable operation we'll turn cancels
         * off here. There's nothing we can do with them at this point.
         * The CN mutex must be unlocked also while we do the join.
         * This is because the receiver thread may need to lock the CN
         * mutex in order to exit. For instance, the receiver thread
         * may be blocked on the condition variable waiting for a new
         * connection. The process of unblocking from this condition
         * variable by definition acquires the CN mutex. 
         */
        prev_cancel_state = dcethread_enableinterrupt_throw (0);
        RPC_CN_UNLOCK ();
        dcethread_join_throw (ccb->cn_rcvr_thread_id,
                      &dcethread_exit_status);
        RPC_CN_LOCK ();
        dcethread_enableinterrupt_throw (prev_cancel_state);

        /*
         * Now that the receiver thread has terminated we can delete the
         * receiver thread and association receive queue condition variable.
         */
        RPC_COND_DELETE (ccb->cn_rcvr_cond, rpc_g_global_mutex);
        RPC_COND_DELETE (assoc->assoc_msg_cond, rpc_g_global_mutex);
    }

    {
        error_status_t st;

        if (ccb->rpc_addr)
            rpc__naf_addr_free (&ccb->rpc_addr, &st);
    }

    RPC_LOG_CN_ASSOC_ACB_FR_XIT;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_init
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This routine will initialize all the fields of the
**  association group given.
**
**  INPUTS:             
**
**      assoc_grp       The association group to be initialized.
**      index           The index part of the local ID assigned to
**                      this group.
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

INTERNAL void rpc__cn_assoc_grp_init 
(
 rpc_cn_assoc_grp_p_t    assoc_grp,
 unsigned32              index
)
{
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_init);
    
    /*
     * Initialize all the fields in the association group given.
     */
    memset (assoc_grp, 0, sizeof (rpc_cn_assoc_grp_t));
    rpc__cn_gen_local_id (index, &assoc_grp->grp_id);
    assoc_grp->grp_max_assoc = (typeof(assoc_grp->grp_max_assoc))(RPC_C_ASSOC_GRP_MAX_ASSOCS_DEFAULT);
    RPC_COND_INIT (assoc_grp->grp_assoc_wt, rpc_g_global_mutex);
    RPC_CN_STATS_INCR (assoc_grps);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_create
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  This routine will expand the existing assoc group table by a
**  fixed amount. It does this by determining the size
**  of the old table, allocating a piece of memory that size plus
**  some increment and copying the old table into it. The
**  old table is then freed.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     ID of the first association group in
**                      the expanded area of the new table.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL rpc_cn_local_id_t rpc__cn_assoc_grp_create 
(
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *new_assoc_grp;
    rpc_cn_local_id_t   grp_id;
    unsigned16          old_count;
    unsigned16          new_count;
    unsigned32          i;
    
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_create);
    CODING_ERROR (st);
    
    /*
     * Compute the size of the new table.
     */
    old_count = rpc_g_cn_assoc_grp_tbl.grp_count;
    new_count = old_count + RPC_C_ASSOC_GRP_ALLOC_SIZE; 
    
    /*
     * First allocate a new association group table larger than the
     * existing by a fixed amount.
     */
    RPC_MEM_ALLOC (new_assoc_grp, 
                   rpc_cn_assoc_grp_p_t, 
                   sizeof(rpc_cn_assoc_grp_t) * new_count,
                   RPC_C_MEM_CN_ASSOC_GRP_BLK, 
                   RPC_C_MEM_WAITOK);
    if (new_assoc_grp == NULL) {
        *st = rpc_s_no_memory;
        RPC_CN_LOCAL_ID_CLEAR (grp_id);
        return (grp_id);
    }
 
    /*
     * If there is an old association group table copy it into the
     * new table and free it.
     */
    if (rpc_g_cn_assoc_grp_tbl.assoc_grp_vector != NULL)
    {
        memcpy (new_assoc_grp, 
                rpc_g_cn_assoc_grp_tbl.assoc_grp_vector, 
                (old_count * sizeof (rpc_cn_assoc_grp_t)));
        for (i = 0; i < old_count; i++)
        {
            /*
             * Relocate the "last" pointer in the head of the grp_assoc_list.
             * We don't check group's state because they must be all active.
             * Otherwise, this function never get called. (grp_assoc_list.next
             * shouldn't be NULL.)
             */
            if (new_assoc_grp[i].grp_assoc_list.next != NULL)
            {
                ((rpc_list_p_t)(new_assoc_grp[i].grp_assoc_list.next))->last =
                    (pointer_t)&new_assoc_grp[i].grp_assoc_list;
            }
        }        

        RPC_MEM_FREE (rpc_g_cn_assoc_grp_tbl.assoc_grp_vector,
                      RPC_C_MEM_CN_ASSOC_GRP_BLK); 
    }
    
    /*
     * Update the count of association groups and point to the new
     * association group table.
     */
    rpc_g_cn_assoc_grp_tbl.grp_count = new_count;
    rpc_g_cn_assoc_grp_tbl.assoc_grp_vector = new_assoc_grp;
    
    /*
     * Initialize the association groups in the table just
     * allocated.
     */
    for (i = old_count; i < new_count; i++)
    {
        rpc__cn_assoc_grp_init (&new_assoc_grp[i], i);
    }        
    
    /*
     * Return a pointer to the beginning of the new area in the
     * new table.
     */
    grp_id = new_assoc_grp[old_count].grp_id;
    *st = rpc_s_ok;
    return (grp_id);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_alloc
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will locate an inactive association group slot
**  in the association table, possibly by increasing the
**  size of the table. This association group slot will be
**  initialized and the association will be added to the list.
**
**  INPUTS:            
**
**      rpc_addr        The primary RPC address of the group.
**      type            The type of associations on this group.
**      rem_id          The remote group ID.
**      
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE rpc_cn_local_id_t rpc__cn_assoc_grp_alloc 
(
  rpc_addr_p_t            rpc_addr,
  rpc_transport_info_p_t  transport_info,
  unsigned32              type,
  unsigned32              rem_id,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_local_id_t   grp_id;
    unsigned32          i;
    boolean             found_assoc_grp;
    
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_alloc);
    CODING_ERROR (st);

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors, RPC_C_CN_DBG_GRP_ALLOC))
    {
        *st = RPC_S_CN_DBG_FAILURE;
        RPC_CN_LOCAL_ID_CLEAR (grp_id);
        return(grp_id);
    }
#endif

    /*
     * Ideally we'd like to locate an association group in the
     * existing table which is not being used. Worst case we'll
     * increase the size of the table to get a new group.
     *
     * Get a pointer to the group vector so this code will be easier
     * to read.
     */
    assoc_grp = rpc_g_cn_assoc_grp_tbl.assoc_grp_vector;
    for (i = 0, found_assoc_grp = false; 
         i < rpc_g_cn_assoc_grp_tbl.grp_count; 
         i++)
    {
        if (assoc_grp[i].grp_state.cur_state == RPC_C_ASSOC_GRP_CLOSED)
        {
            assoc_grp = &assoc_grp[i];
            found_assoc_grp = true;
            break;
        }
    }
    
    if (!found_assoc_grp)
    {
        /*
         * The association group table will have to be expanded to
         * get a free group.
         */
        grp_id = rpc__cn_assoc_grp_create (st);
        if (!RPC_CN_LOCAL_ID_VALID (grp_id))
        {
            return (grp_id);
        }
        else
        {
            assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
        }
    }

    /*
     * At this point we have a group which has just been initialized.
     *
     * Copy the RPC address, if given, into the group. 
     */
    if (rpc_addr != NULL)
    {
        rpc__naf_addr_copy (rpc_addr, &assoc_grp->grp_address, st);
        assoc_grp->grp_secaddr = NULL;
        if (*st != rpc_s_ok)
        {
            RPC_CN_LOCAL_ID_CLEAR (grp_id);
            return (grp_id);
        }
    }

    assoc_grp->grp_transport_info = transport_info;

    if (transport_info)
    {
        rpc__transport_info_retain(transport_info);
    }
    
    /*
     * Set up the type of this association group.
     */
    assoc_grp->grp_flags |= type;

    /*
     * Initialize the remote group id.
     */
    assoc_grp->grp_remid.all = rem_id;

    /*
     * Initialize the state machine control block in the group based
     * on its type.
     */
    if (type == RPC_C_CN_ASSOC_GRP_CLIENT)
    {
        /*
         * Initialize with the client state tables.
         */
        rpc__cn_sm_init (rpc_g_cn_client_grp_sm,
                         rpc_g_cn_client_grp_action_tbl,
                         &(assoc_grp->grp_state),
			 rpc_c_cn_cl_a_g );
    }
    else
    {
        /*
         * Initialize with the server state tables.
         */
        rpc__cn_sm_init (rpc_g_cn_server_grp_sm,
                         rpc_g_cn_server_grp_action_tbl,
                         &(assoc_grp->grp_state),
			 rpc_c_cn_svr_a_g );
    }

    /*
     * Finally send a new association event through the group state
     * machine. 
     */
    RPC_CN_ASSOC_GRP_EVAL_EVENT (assoc_grp, 
                                 RPC_C_ASSOC_GRP_NEW,
                                 NULL,
                                 assoc_grp->grp_status);

    /*
     * Increment the the number of active groups in the association
     * group table.
     */
    rpc_g_cn_assoc_grp_tbl.grp_active_count++;
    *st = assoc_grp->grp_status;
    grp_id = assoc_grp->grp_id;
    return (grp_id);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_dealloc
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will any resources on an association group. It
**  is assumed the association list is empty.
**
**  INPUTS:            
**
**      grp_id          The id of the group being deallocated.
**      
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

PRIVATE void rpc__cn_assoc_grp_dealloc 
(
  rpc_cn_local_id_t       grp_id
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_addr_p_t        rpc_addr;
    unsigned32          st;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_dealloc);
    
    /*
     * Check whether a primary RPC address exists on the group.
     */
    assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
    if ((rpc_addr = assoc_grp->grp_address) != NULL)
    {
        /*
         * Free the RPC address.
         */
        rpc__naf_addr_free (&assoc_grp->grp_address, &st);
    }
    
    /*
     * Check whether a secondary RPC address exists on the group.
     */
    if ((rpc_addr != assoc_grp->grp_secaddr) 
        && 
        (assoc_grp->grp_secaddr != NULL))
    {
        /*
         * Free the RPC address.
         */
        rpc__naf_addr_free (&assoc_grp->grp_secaddr, &st);
    }
    
    if (assoc_grp->grp_transport_info)
    {
        rpc__transport_info_release(assoc_grp->grp_transport_info);
        assoc_grp->grp_transport_info = NULL;
    }

    /*
     * Set up other fields in the group for the next time it is
     * allocated.
     */
    RPC_CN_LOCAL_ID_CLEAR (assoc_grp->grp_remid);
    assoc_grp->grp_flags = 0;
    assoc_grp->grp_address = NULL;
    assoc_grp->grp_secaddr = NULL;
    assoc_grp->grp_refcnt = 0;
    assoc_grp->grp_max_assoc = (typeof(assoc_grp->grp_max_assoc))(RPC_C_ASSOC_GRP_MAX_ASSOCS_DEFAULT);
    assoc_grp->grp_cur_assoc = 0;
    assoc_grp->grp_assoc_waiters = 0;
    assoc_grp->grp_status = rpc_s_ok;
    assoc_grp->grp_next_key_id = 0;
    RPC_LIST_INIT (assoc_grp->grp_assoc_list); 
    assoc_grp->grp_liveness_mntr = NULL;
    assoc_grp->grp_callcnt = 0;

    /*
     * Regenerate the local ID since there may still be bindings
     * which have the old ID of this association group in it.
     */
    rpc__cn_gen_local_id (assoc_grp->grp_id.parts.id_index, &assoc_grp->grp_id);

    /*
     * Decrement the the number of active groups in the association
     * group table.
     */
    rpc_g_cn_assoc_grp_tbl.grp_active_count--;
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_add_assoc
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will add the specified association to the
**  association list in the group.
**
**  INPUTS:            
**
**      grp_id          The association group id.
**      assoc           The association to be added.
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

PRIVATE void rpc__cn_assoc_grp_add_assoc 
(
  rpc_cn_local_id_t       grp_id,
  rpc_cn_assoc_p_t        assoc
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_add_assoc);
    
    /*
     * Add the new association to the group by sending an add
     * association to group event through the group state machine.
     */
    assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
    RPC_CN_ASSOC_GRP_EVAL_EVENT (assoc_grp,
                                 RPC_C_ASSOC_GRP_ADD_ASSOC,
                                 assoc,
                                 assoc_grp->grp_status);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_rem_assoc
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will remove an association from the association
**  list on a group. If this results in the list becoming empty
**  the RPC address on the association group is freed and the
**  group is initialized.
**
**  INPUTS:            
**
**      grp_id          The association group id.
**      assoc           The association to be removed.
**      
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_assoc_grp_rem_assoc 
(
  rpc_cn_local_id_t       grp_id,
  rpc_cn_assoc_p_t        assoc
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;

    
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_rem_assoc);
    
    /*
     * If the association group passed in is NULL then this
     * operation is a no-op.
     */
    if (RPC_CN_LOCAL_ID_VALID (grp_id))
    {
        /*
         * Remove the association from the group by sending a remove
         * association from group event through the group state machine.
         */
        assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
        RPC_CN_ASSOC_GRP_EVAL_EVENT (assoc_grp, 
                                     RPC_C_ASSOC_GRP_REM_ASSOC,
                                     assoc,
                                     assoc_grp->grp_status);

        /*
         * If this is a server association group and there are no
         * calls currently running on this association group and we
         * are in the call wait state then post a no calls
         * indication event to the group state machine.
         */
        if ((assoc_grp->grp_flags & RPC_C_CN_ASSOC_GRP_SERVER) 
            &&
            (assoc_grp->grp_callcnt == 0)
            &&
            (assoc_grp->grp_state.cur_state ==
             RPC_C_SERVER_ASSOC_GRP_CALL_WAIT)) 
        {
            RPC_CN_ASSOC_GRP_EVAL_EVENT (assoc_grp, 
                                         RPC_C_ASSOC_GRP_NO_CALLS_IND,
                                         assoc,
                                         assoc_grp->grp_status);
        }
    }
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_lkup_by_addr
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will scan the association group table for a
**  group which is active and whose RPC address matches that given
**  as an input argument. If a matching group is found a pointer to it is
**  returned otherwise a NULL is returned.
**
**  INPUTS:             
**
**      rpc_addr        The address of a remote address space.
**      type            The type of association group (client or server).
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_assoc_grp_not_found
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      return          The id of the association group pointer found.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_cn_local_id_t rpc__cn_assoc_grp_lkup_by_addr 
(
  rpc_addr_p_t            rpc_addr,
  rpc_transport_info_p_t  transport_info,
  unsigned32              type,
  unsigned32              *st
)
{
    boolean             addrs_equal;
    unsigned32          i;
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_local_id_t   grp_id;
    
    RPC_LOG_CN_GRP_ADDR_LKUP_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_lkup_by_addr);
    CODING_ERROR (st);

    /*
     * Check whether a valid RPC address was given for the lookup.
     */
    if (rpc_addr != NULL)
    {
        /*
         * Get a pointer to the group vector so this code will be easier
         * to read.
         */
        assoc_grp = rpc_g_cn_assoc_grp_tbl.assoc_grp_vector;
        
        /*
         * An association group will be located by using the RPC address
         * given. 
         */
        for (i = 0; i < rpc_g_cn_assoc_grp_tbl.grp_count; i++)
        {
            /*
             * Check all groups which are the right type and are active
             */
            if ((assoc_grp[i].grp_flags & type) 
                && 
                (assoc_grp[i].grp_state.cur_state == RPC_C_ASSOC_GRP_ACTIVE))
            {

                /*
                 * The association group has associations. Compare the RPC
                 * address given against the primary address in the
                 * association group.
                 */
                addrs_equal = rpc__naf_addr_compare (rpc_addr,
                                                     assoc_grp[i].grp_address,
                                                     st);
                /*
                 * If the input argument RPC address matched that in
                 * the association group return it.
                 */
                if (addrs_equal && rpc__transport_info_equal(assoc_grp[i].grp_transport_info, transport_info))
                {
                    *st = rpc_s_ok;
                    return (assoc_grp[i].grp_id);
                }
            }
        }
    }    

    /*
     * There were no matching association groups. Return NULL.
     */
    *st = rpc_s_assoc_grp_not_found;
    RPC_CN_LOCAL_ID_CLEAR (grp_id);
    RPC_LOG_CN_GRP_ADDR_LKUP_XIT;
    return (grp_id);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_lkup_by_remid
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will scan the association group table for a
**  group whose remote group id matches that given as an input
**  argument. If a matching group is found a pointer to it is
**  returned otherwise a NULL is returned.
**
**  INPUTS:             
**
**      rem_id          The remote group id.
**      type            The type of association group (client or server).
**      rpc_addr        The address of the server to which the
**                      association was established.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_assoc_grp_not_found
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      return          The id of the association group pointer found.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_cn_local_id_t rpc__cn_assoc_grp_lkup_by_remid 
(
  unsigned32              rem_id,
  unsigned32              type,
  rpc_addr_p_t            rpc_addr,
  unsigned32              *st
)
{
    unsigned32          i;
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_local_id_t   grp_id;

    RPC_LOG_CN_GRP_REMID_LKUP_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_lkup_by_remid);
    CODING_ERROR (st);
    
    /*
     * Get a pointer to the group vector so this code will be easier
     * to read.
     */
    assoc_grp = rpc_g_cn_assoc_grp_tbl.assoc_grp_vector;
    
    /*
     * An association group will be located by using the RPC address
     * given. 
     */
    for (i = 0; i < rpc_g_cn_assoc_grp_tbl.grp_count; i++)
    {
        /*
         * Check all groups which are the right type and are active.
         */
        if ((assoc_grp[i].grp_flags & type) 
            && 
            (assoc_grp[i].grp_state.cur_state == RPC_C_ASSOC_GRP_ACTIVE))
        {
            /*
             * The association group has associations. Compare the remote
             * group id given against the remote group id in the
             * association group.
             */
            if (assoc_grp[i].grp_remid.all == rem_id)
            {
                boolean             addrs_equal;

                /*
                 * Check whether the primary address on the
                 * association group matches that to which the
                 * association was established.
                 */
                addrs_equal = rpc__naf_addr_compare (rpc_addr,
                                                     assoc_grp[i].grp_address,
                                                     st);
                if (addrs_equal)
                {
                    *st = rpc_s_ok;
                    RPC_LOG_CN_GRP_REMID_LKUP_XIT;
                    return (assoc_grp[i].grp_id);
                }
            }
        }
    }
    
    /*
     * There were no matching association groups. Return NULL.
     */
    *st = rpc_s_assoc_grp_not_found;
    RPC_CN_LOCAL_ID_CLEAR (grp_id);
    return (grp_id);
}


/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_lkup_by_id
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will match the group ID given against the entry
**  in the association group table. If the group is active and
**  entire group ID given matches that in the association group that
**  group is returned otherwise an invalid local id is returned.
**
**  INPUTS:             
**
**      grp_id          The association group ID to compare against.
**      type            The type of association group (client or server).
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_assoc_grp_not_found
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      return          The id of the association group pointer found.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_cn_local_id_t rpc__cn_assoc_grp_lkup_by_id 
(
  rpc_cn_local_id_t       grp_id,
  unsigned32              type,
  rpc_transport_info_p_t  transport_info,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_local_id_t   ret_grp_id;

    RPC_LOG_CN_GRP_ID_LKUP_NTR;
    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_lkup_by_id);
    CODING_ERROR (st);

#ifdef DEBUG
    if (RPC_DBG_EXACT(rpc_es_dbg_cn_errors,
                      RPC_C_CN_DBG_GRP_LKUP_BY_ID))
    {
        *st = rpc_s_assoc_grp_not_found;
        RPC_CN_LOCAL_ID_CLEAR (ret_grp_id);
        return (ret_grp_id);
    }
#endif
        
    /*
     * First determine if we were given a valid group ID.
     */
    if (RPC_CN_LOCAL_ID_VALID (grp_id) &&
        grp_id.parts.id_index < rpc_g_cn_assoc_grp_tbl.grp_count)
    {
        /*
         * An association group will be located by using the lower 16
         * bits of the id as an index into the association group table.
         */
        assoc_grp = RPC_CN_ASSOC_GRP (grp_id);

        /*
         * To use this association group the entire group id must match
         * and must be the right type.
         */
        if (RPC_CN_LOCAL_ID_EQUAL (assoc_grp->grp_id, grp_id) &&
            (assoc_grp->grp_flags & type) &&
            (assoc_grp->grp_state.cur_state == RPC_C_ASSOC_GRP_ACTIVE) &&
            (rpc__transport_info_equal(assoc_grp->grp_transport_info, transport_info)))
        {
            *st = rpc_s_ok;
            RPC_LOG_CN_GRP_ID_LKUP_XIT;

            return (grp_id);
        }
    }
    
    *st = rpc_s_assoc_grp_not_found;
    RPC_CN_LOCAL_ID_CLEAR (ret_grp_id);
    return (ret_grp_id);
}



/******************************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_assoc_grp_tbl_init
**
**  SCOPE:              PRIVATE - declared in cnassoc.h
**
**  DESCRIPTION:
**      
**  This routine will initialize the association group table.
**
**  INPUTS:             none
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

PRIVATE void rpc__cn_assoc_grp_tbl_init (void)
{
    unsigned32          st;
    unsigned32          server_disc_time = 0;
    unsigned32          client_disc_time = 0;
    char                *x;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_assoc_grp_tbl_init);

    /*
     * Init the new association condition variable, count and wakeup
     * variables internal to this module.
     */
    RPC_COND_INIT (grp_new_wt,
                   rpc_g_global_mutex); 
    grp_new_in_progress = false;
    grp_new_waiters = 0;

    /*
     * Init the association group counts.
     */
    rpc_g_cn_assoc_grp_tbl.grp_count = 0;
    rpc_g_cn_assoc_grp_tbl.grp_active_count = 0;

    /*
     * Init the association group vector to NULL.
     */
    rpc_g_cn_assoc_grp_tbl.assoc_grp_vector = NULL;

    /*
     * Start the client and server reclaimation timers with the
     * values as specified in the NCA Connection Architecture
     * spec.
     */
    x = getenv("RPC_CLIENT_DISC_TIME");
    if (x != NULL)
        client_disc_time = (unsigned32)atoi(x);
   if (client_disc_time == 0)
        client_disc_time = RPC_C_ASSOC_CLIENT_DISC_TIMER;

    rpc__timer_set (&rpc_g_cn_assoc_grp_tbl.grp_client_timer,
                    (rpc_timer_proc_p_t) rpc__cn_assoc_timer_reclaim,
                    (pointer_t) RPC_C_CN_ASSOC_GRP_CLIENT,
                    RPC_CLOCK_SEC (client_disc_time));
    
    x = getenv("RPC_SERVER_DISC_TIME");
    if (x != NULL)
        server_disc_time = (unsigned32)atoi(x);
   if (server_disc_time == 0)
        server_disc_time = RPC_C_ASSOC_SERVER_DISC_TIMER;

    rpc__timer_set (&rpc_g_cn_assoc_grp_tbl.grp_server_timer,
                    (rpc_timer_proc_p_t) rpc__cn_assoc_timer_reclaim,
                    (pointer_t) RPC_C_CN_ASSOC_GRP_SERVER,
                    RPC_CLOCK_SEC (server_disc_time));

    /*
     * Put some association groups in the table so this doesn't have
     * to be done on the first RPC.
     */
    (void) rpc__cn_assoc_grp_create (&st);
}       


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_grp_sm_protocol_error
**
**  SCOPE:              PRIVATE
**
**  DESCRIPTION:
**
**  Action routine invoked when an illegal transition is detected.
**  This routine writes an error message to stdout and DIEs.
**
**  INPUTS:
**
**      spc_struct      The special structure which is passed to the
**                      state machine event evaluation routine.
**			This is assumed to be the assoc grp.
**
**      event_param     The event specific argument.
**
**  INPUTS/OUTPUTS:     
**      sm              The control block from the event
**                      evaluation routine.  Input is the current
**                      status and event for the control block.
**                      Output is the next state or updated
**                      current state, for the control block.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     unsigned32
**
**  SIDE EFFECTS:       output is printed on stdout.
**
**--
**/

PRIVATE unsigned32     rpc__cn_grp_sm_protocol_error 
(
  pointer_t       spc_struct,
  pointer_t       event_param ATTRIBUTE_UNUSED,
  pointer_t       sm ATTRIBUTE_UNUSED
)
{
    rpc_cn_assoc_grp_t  *assoc_grp ATTRIBUTE_UNUSED;

    RPC_CN_DBG_RTN_PRINTF(rpc__cn_grp_sm_protocol_error);
    assoc_grp = (rpc_cn_assoc_grp_t *) spc_struct;

    /*
     * "Illegal state transition detected in CN {server|client} association
     * group state machine [cur_state: %d, cur_event: %d, grp: %x]"
     */
    RPC_DCE_SVC_PRINTF ((
	DCE_SVC(RPC__SVC_HANDLE, "%d%d%x"),
	rpc_svc_cn_state,
	svc_c_sev_fatal | svc_c_action_abort,
        (assoc_grp->grp_flags & RPC_C_CN_ASSOC_GRP_SERVER) ?
	    rpc_m_cn_ill_state_trans_sg : rpc_m_cn_ill_state_trans_cg,
        assoc_grp->grp_state.cur_state,
        assoc_grp->grp_state.cur_event,
        assoc_grp ));
	 return 0;
}
