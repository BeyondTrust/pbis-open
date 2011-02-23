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
**      dgslive.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Routines for monitoring liveness of clients.
**
**
*/

#include <dg.h>
#include <dgsct.h>
#include <dgslive.h>

#include <dce/convc.h>
#include <dce/conv.h>

/* ========================================================================= */

INTERNAL void network_monitor_liveness    (void);

/* ========================================================================= */

/*
 * Number of seconds before declaring a monitored client dead.
 */

#define LIVE_TIMEOUT_INTERVAL   120

/*
 * The client table is a hash table with seperate chaining, used by the
 * server runtime to keep track of client processes which it has been
 * asked to monitor. 
 *
 * This table is protected by the global lock.
 */

#define CLIENT_TABLE_SIZE 29    /* must be prime */

INTERNAL rpc_dg_client_rep_p_t client_table[CLIENT_TABLE_SIZE];
         
#define CLIENT_HASH_PROBE(cas_uuid, st) \
    (rpc__dg_uuid_hash(cas_uuid) % CLIENT_TABLE_SIZE)

/*
 * static variables associated with running a client monitoring thread.
 * 
 * All are protected by the monitor_mutex lock.
 */

INTERNAL rpc_mutex_t    monitor_mutex;
INTERNAL rpc_cond_t     monitor_cond;
INTERNAL dcethread*  monitor_task;
INTERNAL boolean    monitor_running = false;
INTERNAL boolean    monitor_was_running = false;
INTERNAL boolean    stop_monitor = false;
INTERNAL unsigned32 active_monitors = 0;

/* ========================================================================= */

/*
 * F I N D _ C L I E N T 
 *
 * Utility routine for looking up a client handle, by UUID, in the
 * global client_rep table.
 */

INTERNAL rpc_dg_client_rep_p_t find_client (
        dce_uuid_p_t /*cas_uuid*/
    );

INTERNAL rpc_dg_client_rep_p_t find_client
(
    dce_uuid_p_t cas_uuid
)
{
    rpc_dg_client_rep_p_t client;
    unsigned16 probe;
    unsigned32 st;
                        
    probe = CLIENT_HASH_PROBE(cas_uuid, &st);
    client = client_table[probe];

    while (client != NULL) 
    {
        if (dce_uuid_equal(cas_uuid, &client->cas_uuid, &st))
            return(client);
        client = client->next;
    }
    return(NULL);
}


/*
 * R P C _ _ D G _ N E T W O R K _ M O N
 *
 * This routine is called, via the network listener service, by a server
 * stub which needs to maintain context for a particular client.  The
 * client handle is provided, and in the event that the connection to
 * the client is lost, that handle will be presented to the rundown routine
 * specified. 
 * 
 * The actual client rep structure is created during the call to
 * binding_inq_client and is stored in a global table at that time.  When
 * successful, this routine merely associates a rundown function pointer
 * with the appropriate client rep structure in the table.
 */

PRIVATE void rpc__dg_network_mon
(
    rpc_binding_rep_p_t binding_r ATTRIBUTE_UNUSED,
    rpc_client_handle_t client_h,
    rpc_network_rundown_fn_t rundown,
    unsigned32 *st
)
{            
    rpc_dg_client_rep_p_t ptr, client = (rpc_dg_client_rep_p_t) client_h;
    unsigned16 probe;
    dce_uuid_p_t cas_uuid = (dce_uuid_p_t) &client->cas_uuid;

    RPC_MUTEX_LOCK(monitor_mutex);
   
    /*
     * Hash into the client rep table based on the handle's UUID.
     * Scan the chain to find the client handle.
     */
                  
    probe = CLIENT_HASH_PROBE(cas_uuid, st);
    ptr = client_table[probe]; 

    while (ptr != NULL)
    {
        if (ptr == client)
            break;
        ptr = ptr->next;
    }

    /*           
     * If the handle passed in is not in the table, it must be bogus.
     * Also, make sure that we are not already monitoring this client,
     * indicated by a non-NULL rundown routine pointer.
     */

    if (ptr == NULL || ptr->rundown != NULL)
    {    
        *st = -1;         /* !!! Need a real error value */
        RPC_MUTEX_UNLOCK(monitor_mutex);
        return;
    }

    /*
     * (Re)initialize the table entry, and bump the count of active monitors.
     */

    client->rundown  = rundown;
    client->last_update = rpc__clock_stamp();
    active_monitors++;

    /*
     * Last, make sure that the monitor timer routine is running. 
     */

    if (! monitor_running)
    {
        monitor_running = true;
        dcethread_create_throw(&monitor_task, NULL, 
            (dcethread_startroutine) network_monitor_liveness, 
            NULL);  
    }                         

    *st = rpc_s_ok;
    RPC_MUTEX_UNLOCK(monitor_mutex);
}


/*
 * R P C _ _ D G _ N E T W O R K _ S T O P _ M O N
 *
 * This routine is called, via the network listener service, by a server stub
 * when it wishes to discontinue maintaining context for a particular client. 
 * The client will no longer be monitored if the rundown function pointer
 * is set to NULL.  The actual client handle structure is maintained, with
 * reference from the SCTE, to avoid doing another callback if the client 
 * needs to be monitored again.
 */
 
PRIVATE void rpc__dg_network_stop_mon
(
    rpc_binding_rep_p_t binding_r ATTRIBUTE_UNUSED,
    rpc_client_handle_t client_h,
    unsigned32 *st
)
{
    rpc_dg_client_rep_p_t client = (rpc_dg_client_rep_p_t) client_h;
    rpc_dg_client_rep_p_t ptr;
    dce_uuid_p_t cas_uuid = &client->cas_uuid;
    unsigned16 probe;
                 
    RPC_MUTEX_LOCK(monitor_mutex);

    /*
     * Hash into the client rep table based on the client handle's UUID.
     */
  
    probe = CLIENT_HASH_PROBE(cas_uuid, st);
    ptr = client_table[probe];
       
    /*
     * Scan down the hash chain, looking for the reference to the client
     * handle
     */
                   
    while (ptr != NULL) {
        if (ptr == client)
        {   
            /*
             * To stop monitoring a client handle requires only that 
             * the rundown function pointer be set to NULL.
             */           
         
            if (client->rundown != NULL)
            {
                client->rundown = NULL;
                active_monitors--;
            }
            RPC_MUTEX_UNLOCK(monitor_mutex);
            *st = rpc_s_ok;
            return;
        }
        ptr = ptr->next;
    }

    *st = -1;               /* !!! attempt to remove unmonitored client */
    RPC_MUTEX_UNLOCK(monitor_mutex);
}


/*
 * N E T W O R K _ M O N I T O R _ L I V E N E S S 
 *
 * This routine runs as the base routine of a thread; it periodically
 * checks for lost client connections.  We can't run this routine from
 * the timer queue (and thread) because it calls out to the application
 * (stub) rundown routines and we can't tie up the timer while we do
 * that.
 */

INTERNAL void network_monitor_liveness(void)
{
    rpc_dg_client_rep_p_t client;
    unsigned32 i;
    struct timespec next_ts;

    RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1, 
                   ("(network_monitor_liveness) starting up...\n"));

    RPC_MUTEX_LOCK(monitor_mutex);

    while (stop_monitor == false)
    {
        /*
         * Awake every 60 seconds.
         */
        rpc__clock_timespec(rpc__clock_stamp()+60, &next_ts);

        RPC_COND_TIMED_WAIT(monitor_cond, monitor_mutex, &next_ts);
        if (stop_monitor == true)
            break;

        for (i = 0; i < CLIENT_TABLE_SIZE; i++)
        {                                     
            client = client_table[i];
    
            while (client != NULL && active_monitors != 0)
            {      
                if (client->rundown != NULL &&
                    rpc__clock_aged(client->last_update, 
                                    RPC_CLOCK_SEC(LIVE_TIMEOUT_INTERVAL)))
                {                 
                    /*
                     * If the timer has expired, call the rundown routine.
                     * Stop monitoring the client handle by setting its rundown
                     * routine pointer to NULL.
                     */
    
                    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
                        ("(network_monitor_liveness_timer) Calling rundown function\n"));
                            
                    RPC_MUTEX_UNLOCK(monitor_mutex);
                    (*client->rundown)((rpc_client_handle_t)client);
                    RPC_MUTEX_LOCK(monitor_mutex);

                    /*
                     * The monitor is no longer active.
                     */
                    client->rundown = NULL;
                    active_monitors--;
                }
                client = client->next;
            }

            if (active_monitors == 0)
            {
                /*
                 * While we were executing the rundown function and opened the
                 * mutex, the fork handler might try to stop us.
                 */
                if (stop_monitor == true)
                    break;
                /*
                 * Nothing left to monitor, so terminate the thread.
                 */
                dcethread_detach_throw(monitor_task);
                monitor_running = false;
                RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1, 
                    ("(network_monitor_liveness) shutting down (no active)...\n"));
                RPC_MUTEX_UNLOCK(monitor_mutex);
                return;
            }
        }
    }
    RPC_DBG_PRINTF(rpc_e_dbg_conv_thread, 1, 
                   ("(network_monitor_liveness) shutting down...\n"));

    RPC_MUTEX_UNLOCK(monitor_mutex);
}


/*
 * R P C _ _ D G _ C O N V C _ I N D Y 
 *
 * Server manager routine for monitoring the liveness of clients.
 */

PRIVATE void rpc__dg_convc_indy
(
    dce_uuid_t *cas_uuid
)
{
    rpc_dg_client_rep_p_t client;
                
    RPC_MUTEX_LOCK(monitor_mutex);

    client = find_client(cas_uuid);

    if (client != NULL)
    {
        client->last_update = rpc__clock_stamp();
    }
    RPC_MUTEX_UNLOCK(monitor_mutex);
}


/*
 * R P C _ _ D G _ B I N D I N G _ I N Q _ C L I E N T
 *
 * Inquire what client address space a binding handle refers to.
 */

PRIVATE void rpc__dg_binding_inq_client
(
    rpc_binding_rep_p_t binding_r,
    rpc_client_handle_t *client_h,
    unsigned32 *st
)
{       
    rpc_dg_binding_server_p_t shand = (rpc_dg_binding_server_p_t) binding_r;
    rpc_dg_scall_p_t scall = shand->scall;
    rpc_binding_handle_t h;
    dce_uuid_t cas_uuid;
    rpc_dg_client_rep_p_t client;
    unsigned32 temp_seq, tst;
                              
    *st = rpc_s_ok;

    /*
     * Lock down and make sure we're in an OK state.
     */

    RPC_LOCK(0);
    RPC_DG_CALL_LOCK(&scall->c);
                      
    if (scall->c.state == rpc_e_dg_cs_orphan)
    {
        *st = rpc_s_call_orphaned;
        RPC_DG_CALL_UNLOCK(&scall->c);
        RPC_UNLOCK(0);
        return;
    }
    
    /*
     * See if there is already a client handle associated with the scte
     * associated with this server binding handle.  If there is, just
     * return it.
     */

    if (scall->scte->client != NULL)
    {
        *client_h = (rpc_client_handle_t) scall->scte->client;
        RPC_DG_CALL_UNLOCK(&scall->c);
        RPC_UNLOCK(0);
        return;
    }

    /*
     * No client handle.  We need to do a call back to obtain a UUID
     * uniquely identifying this particular instance of the client.
     */

    h = rpc__dg_sct_make_way_binding(scall->scte, st);

    RPC_DG_CALL_UNLOCK(&scall->c);
    RPC_UNLOCK(0);

    if (h == NULL)
    {
        return;
    }

    RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
        ("(binding_inq_client) Doing whats-your-proc-id callback\n"));

    DCETHREAD_TRY
    {
        (*conv_v3_0_c_epv.conv_who_are_you2)
            (h, &scall->c.call_actid, rpc_g_dg_server_boot_time, 
            &temp_seq, &cas_uuid, st);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        *st = rpc_s_who_are_you_failed;
    }
    DCETHREAD_ENDTRY

    rpc_binding_free(&h, &tst);

    if (*st != rpc_s_ok)
        return;

    /*
     * Check to see if the UUID returned has already been built into
     * a client handle associated with another scte.  Since we have no
     * way of mapping actids to processes, we can't know that two actid
     * are in the same address space until we get the same address space
     * UUID from both.  In this case it is necessary to use the same
     * client handle for both actids.
     */
             
    RPC_LOCK(0);          
    RPC_DG_CALL_LOCK(&scall->c);

    if (scall->c.state == rpc_e_dg_cs_orphan)
    {
        *st = rpc_s_call_orphaned;
        RPC_DG_CALL_UNLOCK(&scall->c);
        RPC_UNLOCK(0);                                     
        return;
    }
    
    RPC_MUTEX_LOCK(monitor_mutex);

    client = find_client(&cas_uuid);

    if (client != NULL)
    {   
        client->refcnt++;
        scall->scte->client = client;
    }
    else
    {
        /*
         * If not, alloc up a client handle structure and thread
         * it onto the table.
         */

        unsigned16 probe;

        probe = CLIENT_HASH_PROBE(&cas_uuid, st);

        RPC_MEM_ALLOC(client, rpc_dg_client_rep_p_t, sizeof *client, 
            RPC_C_MEM_DG_CLIENT_REP, RPC_C_MEM_NOWAIT);

        client->next = client_table[probe];
        client->rundown = NULL;
        client->last_update = 0;
        client->cas_uuid = cas_uuid;

        client_table[probe] = client;
        scall->scte->client = client;
        client->refcnt = 2;
    }  

    RPC_MUTEX_UNLOCK(monitor_mutex);
    RPC_DG_CALL_UNLOCK(&scall->c);
    RPC_UNLOCK(0);                                     
    
    *client_h = (rpc_client_handle_t) client; 
}
  

/*
 * R P C _ _ D G _ M O N I T O R _  I N I T
 *
 * This routine performs any initializations required for the network
 * listener service maintain/monitor functions.
 */

PRIVATE void rpc__dg_monitor_init(void)
{               

    /*
     * Initialize the count of handles currently being monitored.
     */

    active_monitors = 0;
    monitor_running = false;
    monitor_was_running = false;
    stop_monitor = false;
    RPC_MUTEX_INIT(monitor_mutex);
    RPC_COND_INIT(monitor_cond, monitor_mutex);
}

#ifdef ATFORK_SUPPORTED
/*
 * R P C _ _ D G _ M O N I T O R _ F O R K 
 *
 * Handle fork related processing for this module.
 */

PRIVATE void rpc__dg_monitor_fork_handler
(
    rpc_fork_stage_id_t stage
)
{                           
    unsigned32 i;
    unsigned32 st;

    switch ((int)stage)
    {
    case RPC_C_PREFORK:
        RPC_MUTEX_LOCK(monitor_mutex);
        monitor_was_running = false;
        
        if (monitor_running) 
        {
            stop_monitor = true;
            RPC_COND_SIGNAL(monitor_cond, monitor_mutex);
            RPC_MUTEX_UNLOCK(monitor_mutex);
            dcethread_join_throw (monitor_task, (void **) &st);
            RPC_MUTEX_LOCK(monitor_mutex); /* FIXME: wtf
				DCETHREAD_TRY	{
            	dcethread_detach_throw(monitor_task);
				}
				DCETHREAD_CATCH(dcethread_use_error_e)
				{}
				DCETHREAD_ENDTRY; */
            monitor_running = false;
            /*
             * The monitor thread may have nothing to do.
             */
            if (active_monitors != 0)
                monitor_was_running = true;
            stop_monitor = false;
        }
        break;
    case RPC_C_POSTFORK_PARENT:
        if (monitor_was_running) 
        {
            monitor_was_running = false;
            monitor_running = true;
            stop_monitor = false;
            dcethread_create_throw(&monitor_task, NULL, 
                           (dcethread_startroutine) network_monitor_liveness, 
                           NULL);  
        }
        RPC_MUTEX_UNLOCK(monitor_mutex);
        break;
    case RPC_C_POSTFORK_CHILD:  
        monitor_was_running = false;
        monitor_running = false;
        stop_monitor = false;

        /*
         * Initialize the count of handles currently being monitored.
         */
        
        active_monitors = 0;
        for (i = 0; i < CLIENT_TABLE_SIZE; i++)
            client_table[i] = NULL;

        RPC_MUTEX_UNLOCK(monitor_mutex);
        break;
    }
}
#endif /* ATFORK_SUPPORTED */
  
/*
 * R P C _ _ D G _ C L I E N T _ F R E E
 *
 * This routine frees the memory associated with a client handle (created 
 * for the purpose of monitoring client liveness).  It is called by the 
 * the RPC_DG_CLIENT_RELEASE macro when the last scte which refers to this
 * client handle is freed.  The client handle is also removed from the 
 * client table.
 */

PRIVATE void rpc__dg_client_free
(
    rpc_client_handle_t client_h
)
{
    unsigned16 probe;
    rpc_dg_client_rep_p_t client = (rpc_dg_client_rep_p_t) client_h;
    rpc_dg_client_rep_p_t ptr, prev = NULL;

    RPC_MUTEX_LOCK(monitor_mutex);
             
    /*
     * Hash into the client rep table based on the client handle's UUID.
     */
  
    probe = CLIENT_HASH_PROBE(&client->cas_uuid, &st);
    ptr = client_table[probe];
       
    /*
     * Scan down the hash chain, looking for the reference to the client
     * handle
     */
                   
    while (ptr != NULL) 
    {
        if (ptr == client)
        {   
            if (prev == NULL)
                client_table[probe] = ptr->next;
            else
                prev->next = ptr->next;

            RPC_MEM_FREE(client, RPC_C_MEM_DG_CLIENT_REP);

            RPC_DBG_PRINTF(rpc_e_dbg_general, 3, 
                ("(client_free) Freeing client handle\n"));
    
            RPC_MUTEX_UNLOCK(monitor_mutex);
            return;
        }          

        prev = ptr;
        ptr = ptr->next;
    }
    RPC_MUTEX_UNLOCK(monitor_mutex);
}
