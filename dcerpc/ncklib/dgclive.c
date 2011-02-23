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
**      dgclive.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Routines for maintaining liveness of clients.
**
**
*/

#include <dg.h>
#include <dgglob.h>

#include <dce/conv.h>
#include <dce/convc.h>

/* =============================================================================== */

/*
 * Define a linked list element which the client runtime can use to keep
 * track of which servers it needs to maintain liveness with.  
 *
 * Here's the list elements...
 */
typedef struct maint_elt_t
{
    struct maint_elt_t   *next;       /* -> to next entry in list.       */
    rpc_binding_rep_p_t  shand;       /* -> to server binding handle     */
    unsigned8            refcnt;      /* -> # of entries for this server */
} maint_elt_t, *maint_elt_p_t;

/*
 * And here's the head of the list...
 */
INTERNAL maint_elt_p_t maint_head;

/*
 * And here's the mutex that protects the list...
 */
INTERNAL rpc_mutex_t   rpc_g_maint_mutex;
INTERNAL rpc_cond_t    maintain_cond;

INTERNAL boolean maintain_thread_running = false;
INTERNAL boolean maintain_thread_was_running = false;
INTERNAL boolean stop_maintain_thread = false;

INTERNAL dcethread*  maintain_task;
  
/*
 * Mutex lock macros 
 */

#define RPC_MAINT_LOCK_INIT()   RPC_MUTEX_INIT (rpc_g_maint_mutex)
#define RPC_MAINT_LOCK()        RPC_MUTEX_LOCK (rpc_g_maint_mutex)
#define RPC_MAINT_UNLOCK()      RPC_MUTEX_UNLOCK (rpc_g_maint_mutex)

/* ========================================================================= */

INTERNAL void * network_maintain_liveness (void*);

/* ========================================================================= */

/*
 * R P C _ _ D G _ N E T W O R K _ M A I N T
 *
 * This function is called, via the network listener service, by a client
 * stub which needs to maintain context with a server.  A copy of the
 * binding handle is made and entered into a list associated with a timer
 * monitor.  This monitor will then periodically send an identifier to
 * the server to assure it that this client is still alive.
 */

PRIVATE void rpc__dg_network_maint
(
    rpc_binding_rep_p_t binding_r,
    unsigned32 *st
)
{               
    maint_elt_p_t maint;
    rpc_dg_binding_client_p_t chand = (rpc_dg_binding_client_p_t) binding_r;
  
    *st = rpc_s_ok;

    RPC_MAINT_LOCK();

    /*
     * First, we need to traverse the list of maintained contexts to see
     * if this server is already on it.  If we find a matching  address,
     * we can just return.
     */
                                            
    for (maint = maint_head; maint != NULL; maint = maint->next)
    {
        if (rpc__naf_addr_compare(maint->shand->rpc_addr, binding_r->rpc_addr, st))
        {          
            /*
             * If we find a matching element, store a pointer to it in the
             * client binding handle (so we don't have to do these compares
             * when maint_stop gets called) and note the reference.
             */

            chand->maint_binding = maint->shand;
            maint->refcnt++;
            RPC_MAINT_UNLOCK();
            return;       
        }
    }
      
    /*
     * Need to make a new entry in the maintain list.  Alloc up a
     * list element.
     */

    RPC_MEM_ALLOC(maint, maint_elt_p_t, sizeof *maint, 
            RPC_C_MEM_DG_MAINT, RPC_C_MEM_NOWAIT);

    /*
     * Make our own copy of the binding handle (so we have a handle to 
     * send INDY's to this server).  Reference the new binding in the
     * client handle and note the reference.
     */
  
    rpc_binding_copy((rpc_binding_handle_t) chand, 
                     (rpc_binding_handle_t *) &maint->shand, st);
    chand->maint_binding = maint->shand;
    maint->refcnt = 1;

    /*
     * and thread it onto the head of the list.
     */                                    

    maint->next = maint_head;
    maint_head = maint;
        
    /*
     * If the binding handle had any authentication info associated with
     * it, free it up now.   We don't want the convc_indy() calls using
     * authentication.
     */
    rpc_binding_set_auth_info((rpc_binding_handle_t) maint->shand, NULL, 
                       rpc_c_protect_level_none, rpc_c_authn_none, NULL, 
                       rpc_c_authz_none, st);
        
    /*
     * Finally, check to make sure the 'context maintainer' thread has
     * been started, and if not, start it up.
     */

    if (! maintain_thread_running)
    {     
        maintain_thread_running = true;
        dcethread_create_throw(&maintain_task, NULL, 
            (void*)network_maintain_liveness, 
            NULL);  
    }

    RPC_MAINT_UNLOCK();
}

/*
 * R P C _ _ D G _ N E T W O R K _ S T O P _ M A I N T
 *
 * This routine is called, via the network listener service, by a client stub
 * when it wishes to discontinue maintaining context with a server.  
 */

PRIVATE void rpc__dg_network_stop_maint
(
    rpc_binding_rep_p_t binding_r,
    unsigned32 *st
)
{
    maint_elt_p_t maint, prev = NULL;
    rpc_dg_binding_client_p_t chand = (rpc_dg_binding_client_p_t) binding_r;
                                   
    RPC_MAINT_LOCK();

    /*
     * Search through the list for the element which references this
     * binding handle.                                         
     */

    for (maint = maint_head; maint != NULL; maint = maint->next)
    {     
        if (chand->maint_binding == maint->shand)               
        {                    
            /*
             * Remove the reference from the binding handle, and decrement 
             * the reference count in the list element.  If the count
             * falls to 0, remove the element from the list.
             */
            
            chand->maint_binding = NULL;
            if (--maint->refcnt == 0) 
            {
                if (prev == NULL)
                    maint_head = maint->next;
                else                              
                    prev->next = maint->next;
                rpc_binding_free((rpc_binding_handle_t *)&maint->shand, st);
                RPC_MEM_FREE(maint, RPC_C_MEM_DG_MAINT);
            }          
            *st = rpc_s_ok;
            RPC_MAINT_UNLOCK();
            return;
        }
        prev = maint;
    }
                         
    RPC_MAINT_UNLOCK();
    *st = -1;           /*!!! didn't find it, need real error value here */
}

/*
 * R P C _ _ D G _ N E T W O R K _ C L O S E
 *
 * This routine is called, via the network listener service, by a client stub
 * when it wishes to disconnect frmo a server.
 */

PRIVATE void rpc__dg_network_close
(
    ATTRIBUTE_UNUSED rpc_binding_rep_p_t binding_r,
    unsigned32 *st
)
{
    /* this is a NOOP for datagram transports */
    *st = rpc_s_ok;
}

/*
 * N E T W O R K _ M A I N T A I N _ L I V E N E S S
 *
 * Base routine for the thread which periodically sends out the address
 * space UUID of this process.  This UUID uniquely identifies this
 * particular instance of this particular client process for use in
 * maintaing context with servers.
 */

INTERNAL void * network_maintain_liveness(void * unused ATTRIBUTE_UNUSED)
{
    maint_elt_p_t ptr;
    struct timespec next_ts;

    RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1, 
                   ("(network_maintain_liveness) starting up...\n"));

    RPC_MAINT_LOCK();

    while (stop_maintain_thread == false)
    {
        /*
         * Send out INDYs every 20 seconds.
         */
        rpc__clock_timespec(rpc__clock_stamp()+20, &next_ts);

        RPC_COND_TIMED_WAIT(maintain_cond, rpc_g_maint_mutex, &next_ts);
        if (stop_maintain_thread == true)
            break;

        for (ptr = maint_head; ptr != NULL; ptr = ptr->next)
        {                
            RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
                ("(maintain_liveness_timer) Doing convc_indy call\n"));
                              
            (*convc_v1_0_c_epv.convc_indy)((handle_t) ptr->shand, 
                                                   &rpc_g_dg_my_cas_uuid);
        }                
                  
        /*
         * See if the list is empty...
         */
        
        if (maint_head == NULL)
        {
            /*
             * Nothing left to do, so terminate the thread.
             */
			  /* FIXME: MNE */
			  DCETHREAD_TRY	{
            dcethread_detach_throw(maintain_task);
			  }
			  DCETHREAD_CATCH_ALL(THIS_CATCH) {
				  fprintf(stderr, "XXX MIREK: %s: %s: %d: caught exception from detach\n",
						  __FILE__, __PRETTY_FUNCTION__, __LINE__);
				  DCETHREAD_RERAISE;
			  }
			  DCETHREAD_ENDTRY;
            maintain_thread_running = false;
            break;
        }
    }
    RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1, 
                   ("(network_maintain_liveness) shutting down%s...\n",
                    (maint_head == NULL)?" (no active)":""));

    RPC_MAINT_UNLOCK();
	 return NULL;
}

/*
 * R P C _ _ D G _ M A I N T A I N _ I N I T
 *
 * This routine performs any initializations required for the network
 * listener service maintain/monitor functions.  Note that way2 liveness
 * callbacks are handled by the conversation manager interface; we do
 * not need to register the interfaces because the runtime intercepts
 * and handles way2 callbacks as part of listener thread request
 * processing.
 */
   
PRIVATE void rpc__dg_maintain_init(void)
{               
    unsigned32 st;

    /*
     * Gen the address space UUID that we will send to servers
     * to indicate that we're still alive.
     */

    dce_uuid_create(&rpc_g_dg_my_cas_uuid, &st); 
    if (st != rpc_s_ok)
    {
	/*
	 * rpc_m_cant_create_uuid
	 * "(%s) Can't create UUID"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	    rpc_svc_general,
	    svc_c_sev_fatal | svc_c_action_exit_bad,
	    rpc_m_cant_create_uuid,
	    "rpc__dg_maintain_init" ));
    }

    /*
     * Initialize a private mutex.
     */

    RPC_MAINT_LOCK_INIT(); 
    RPC_COND_INIT(maintain_cond, rpc_g_maint_mutex);

    maint_head = NULL;
    maintain_thread_running = false;
    maintain_thread_was_running = false;
    stop_maintain_thread = false;
}

#ifdef ATFORK_SUPPORTED
/*
 * R P C _ _ D G _ M A I N T A I N _ F O R K _ H A N D L E R
 *
 * Handle fork related processing for this module.
 */

PRIVATE void rpc__dg_maintain_fork_handler
(
    rpc_fork_stage_id_t stage
)
{                           
    unsigned32 st;

    switch ((int)stage)
    {
    case RPC_C_PREFORK:
        RPC_MAINT_LOCK();
        maintain_thread_was_running = false;
        
        if (maintain_thread_running) 
        {
            stop_maintain_thread = true;
            RPC_COND_SIGNAL(maintain_cond, rpc_g_maint_mutex);
            RPC_MAINT_UNLOCK();
            dcethread_join_throw (maintain_task, (void**) &st);
            RPC_MAINT_LOCK();/* FIXME: wtf
				DCETHREAD_TRY	{
                                    dcethread_detach_throw(maintain_task);
				}
				DCETHREAD_CATCH(dcethread_use_error_e)	{
				}
				DCETHREAD_ENDTRY; */
            maintain_thread_running = false;
            maintain_thread_was_running = true;
        }
        break;
    case RPC_C_POSTFORK_PARENT:
        if (maintain_thread_was_running) 
        {
            maintain_thread_was_running = false;
            maintain_thread_running = true;
            stop_maintain_thread = false;
            dcethread_create_throw(&maintain_task, NULL, 
                           network_maintain_liveness, 
                           NULL);  
        }
        RPC_MAINT_UNLOCK();
        break;
    case RPC_C_POSTFORK_CHILD:  
        maintain_thread_was_running = false;
        maintain_thread_running = false;
        stop_maintain_thread = false;

        /*
         * Clear out the list... we should free resources...
         */ 
        maint_head = NULL;

        RPC_MAINT_UNLOCK();
        break;
    }
}
#endif /* ATFORK_SUPPORTED */
