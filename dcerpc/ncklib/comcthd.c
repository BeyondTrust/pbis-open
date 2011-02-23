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
**      comcthd.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definition of the Call Thread Services for the Common 
**  Communication Services component. These routines permit
**  a call thread to be created and invoked.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime  */
#include <com.h>        /* Common communications services           */
#include <comprot.h>    /* Common protocol services                 */
#include <comnaf.h>     /* Common network address family services   */
#include <comp.h>       /* Private communications services          */
#include <comcthd.h>    /* Shared call thread services              */


/*
 * The multiplier to apply to a pool's n_threads to get the queue depth.
 */
#ifndef RPC_C_CTHREAD_QUEUE_MULTIPLIER
#  define RPC_C_CTHREAD_QUEUE_MULTIPLIER    8
#endif

/*
 * Call thread states
 */
#define RPC_C_NO_CTHREAD        0       /* no thread exists */
#define RPC_C_IDLE_CTHREAD      1       /* thread exists but is idle */
#define RPC_C_ACTIVE_CTHREAD    2       /* thread has call allocated to it */

/*
 * The call thread table element structure.
 * See the "Call Thread Pool" description for more info...
 *
 * When a thread is idle, it is waiting on its private condition variable.
 *
 * The per element "pool" pointer simply solves the problem of needing
 * the created thread to have two args (without having to alloc memory
 * just for the create phase).  The space really isn't "wasted" since
 * since each thread would otherwise have to keep this var on their stack.
 */
typedef struct
{
    unsigned8                           thread_state;
    dcethread*                           thread_id;
    rpc_cond_t                          thread_cond;
    struct cthread_pool_elt_t           *pool;
    rpc_call_rep_p_t                    call_rep;
} cthread_elt_t, *cthread_elt_p_t;


/*
 * Reserved Pool Call Queue Element.
 *
 * These queue elements form a backing structure that allows
 * us to maintain a call_rep on two pool lists.
 */
typedef struct {
    rpc_list_t                          link;
    struct cthread_pool_elt_t           *pool;
    rpc_call_rep_p_t                    call_rep;
} cthread_queue_elt_t, *cthread_queue_elt_p_t;


/*
 * Call Thread Pools.
 *
 * READ THIS IF YOU WANT TO UNDERSTAND THIS STUFF!
 *
 * This structure exists to accomplish the desired effect of allowing
 * applications to have calls execute using threads from some application
 * defined set of thread pools.  The application declared thread pools are
 * refered to "reserved" pools (due to a previous incarnation of this
 * code which provided -a too limited scheme of- thread reservation on 
 * per-interface basis).
 *
 * The application (via the pool lookup function callout) gets to decide
 * which pool a call should be associated with.  This general mechanism
 * provides the application with the basic hooks to implement any one
 * of a number of schemes for allocating calls to threads.
 *
 * If the application declares that the call should use a reserved pool,
 * a free thread from that pool will be allocated, otherwise, a default
 * pool free thread will be allocated, otherwise, the call will be put
 * on the default and reserved pool queue for execution by the first
 * available default or reserved pool thread.  In any case, the calls
 * for a reserved pool are assigned idle execution threads in the order
 * in which they are received.
 *
 * The default pool is created by rpc_server_listen().  Non-reserved
 * (default pool) threads can execute *any* call (the idle threads are
 * assigned to calls in the order in which they are received).  The total
 * number of call threads in a server is the sum of the threads in the
 * default pool and the reserved pools.
 * 
 * The relationship between n_queued, max_queued and the call_queue
 * requires some explanation.  n_queued and max_queued represent the
 * number and limit respectively of the number of call_queue entries
 * FOR THIS POOL TYPE.  For *reserved pools*, all of these variables
 * make sense in an intuitive way: n_queued always represents the true
 * number of elements on the queue and the number of elements on the
 * queue will not exceed max_queued.
 *
 * The default pool's use of these variables is less intuitive since
 * *all* queued calls are on the default pool's queue.  In this case,
 * the number of elements on the queue can *exceed* the default pool's
 * n_queued and max_queued!  n_queued and max_queued (as stated above)
 * strictly represent the number of default pool calls (i.e. those calls
 * that are not associated with a reserved pool).  The end result is
 * that this accomplishes the desired max queuing limitations - i.e.
 * the maximums are imposed on a per pool basis;  use the queue to
 * to determine of there are actually any queued calls to process,
 * NOT n_queued.
 * 
 * The default pool uses its pool.call_queue to directly link call reps
 * and it does not use the pool.free_queue.  The reserved pools can't
 * directly link the call reps (because call reps only have one rpc_list
 * "thread", because the rpc_list macros only work for a single
 * "thread"...).  Therefore, the reserved pools each maintain their
 * own (static) set of cthread_call_queue_elt_t elements.  Reserved pools'
 * call_queue consists of a set of call_queue_elts that point to the
 * associated call_reps (which are on the default pool queue).  The
 * call_queue_elts are maintained on the pool's pool.free_queue when
 * not in use on the pool.call_queue.
 *
 * Startup / Shutdown processing...  We have a requirement that
 * rpc_server_listen() can be called, return and then be called again
 * (a startup, shutdown, restart sequence).  All call threads should
 * be terminated by a shutdown (to free up resources).  All threads
 * (including those requested by previous calls to
 * rpc_server_create_thread_pool()) must be automatically recreated upon
 * a restart.
 * 
 * Pool creation is two step process.  First, a pool descriptor is
 * allocated (cthread_pool_alloc()).  Subsequently, a pool may be "started"
 * (cthread_pool_start()); this actually creates the call threads.
 * 
 * Shutting down involves stopping all of the call threads in all of
 * the pools (including freeing each pool's call thread table).  The
 * pool descriptors are not freed.  This is necessary to retain information
 * that is needed to restart the server.
 */

typedef struct cthread_pool_elt_t {
    rpc_list_t      link;           /* linked list of pools */
    unsigned16      n_threads;      /* total number of threads in the pool */
    unsigned16      n_idle;         /* number of idle (available) threads */
    cthread_elt_p_t ctbl;           /* the cthreads associated with the pool */
    cthread_elt_p_t idle_cthread;   /* pointer to a known idle cthread */
    unsigned32      n_queued;       /* see above! */
    unsigned32      max_queued;     /* see above! */
    rpc_list_t      call_queue;     /* see above!; list of calls queued */
    rpc_list_t      free_queue;     /* see above!; list of free call_queue elements */
    unsigned        stop : 1;       /* T => pool's threads stop when complete */
    unsigned        queue_elt_alloc : 1;   /* T => start() should allocate queue elts */
} cthread_pool_elt_t, *cthread_pool_elt_p_t;


/*
 * Pools are only associated with the MAJOR version of an interface.
 */

#define IF_VERS_MAJOR(_vers) ((_vers) & 0xffff)
#define IF_VERS_MINOR(_vers) (((_vers) >> 16) & 0xffff)


/*
 * A couple of macros for convienience.
 */

#define CTHREAD_POOL_IS_QUEUE_FULL(p)   ((p)->n_queued >= (p)->max_queued)
#define CTHREAD_POOL_IS_QUEUE_EMPTY(p)  (RPC_LIST_EMPTY ((p)->call_queue))


/*
 * A couple of macros for (fast path) performance.
 */

#define CTHREAD_POOL_LOOKUP_RESERVED(object, if_uuid, if_ver, if_opnum, p, st) \
    { \
        RPC_MUTEX_LOCK_ASSERT (cthread_mutex); \
        if (cthread_pool_lookup_fn == NULL) \
        { \
            *(p) = NULL; \
            *st = 0; \
        } \
        else \
        { \
            rpc_if_id_t if_id; \
            if_id.uuid = *(if_uuid); \
            if_id.vers_major = IF_VERS_MAJOR(if_ver); \
            if_id.vers_minor = IF_VERS_MINOR(if_ver); \
            (*cthread_pool_lookup_fn) (\
                    object, &if_id, if_opnum, \
                    (rpc_thread_pool_handle_t *)p, st); \
        } \
    }

#define CTHREAD_POOL_ASSIGN_THREAD(p, ct) \
    { \
        RPC_MUTEX_LOCK_ASSERT (cthread_mutex); \
        if ((p)->idle_cthread != NULL) \
        { \
            *(ct) = (p)->idle_cthread; \
            (p)->idle_cthread = NULL; \
            assert((*(ct))->thread_state == RPC_C_IDLE_CTHREAD); \
            (*(ct))->thread_state = RPC_C_ACTIVE_CTHREAD; \
            (p)->n_idle--; \
        } \
        else \
        { \
            *(ct) = cthread_pool_assign_thread(p); \
        } \
    }

#define CTHREAD_POOL_IDLE_THREAD(p, ct) \
    { \
        (p)->n_idle++; \
        (p)->idle_cthread = ct; \
    }


 
/*
 * The reserved pools.
 *
 * The pools are linked together via their pool.pool_list field.
 */
INTERNAL rpc_list_t             cthread_reserved_pools;

/*
 * A handle to the special default pool.
 */
INTERNAL cthread_pool_elt_p_t   cthread_default_pool;

/*
 * The maximum number of calls that will be queued for the default
 * thread pool.  This value is settable via the rpc_server_set_thread_pool_qlen
 * function.  If not set, a default value of 8 times the number of
 * default pool threads is used.
 */
INTERNAL unsigned32 cthread_default_call_queue_size;

/*
 * The "reaper's" pool queue and timer.
 *
 * The pools are linked together via their pool.pool_list field.
 */
INTERNAL rpc_list_t             cthread_reaper_queue;
INTERNAL rpc_timer_t            cthread_reaper_timer;

#ifndef RPC_C_CTHREAD_REAPER_FREQ
#  define RPC_C_CTHREAD_REAPER_FREQ     RPC_CLOCK_SEC(3*60)
#endif

/*
 * cthread_mutex protects all of the cthread private structures.
 */
INTERNAL rpc_mutex_t            cthread_mutex;

/*
 * A global that controls the overall ability of RPCs to be assigned
 * to a pool/thread for execution (i.e. it controls rpc__cthread_invoke_null).
 */
INTERNAL boolean                cthread_invoke_enabled;

/*
 * A global that points to the application specified thread pool lookup function.
 */
INTERNAL rpc_thread_pool_fn_t   cthread_pool_lookup_fn;


INTERNAL void cthread_create (
        cthread_elt_p_t          /*cthread*/,
        unsigned32              * /*status*/
    );

INTERNAL void cthread_call_executor (
        cthread_elt_p_t        /*cthread*/
    );

INTERNAL void cthread_reaper (
        pointer_t    /*arg*/
    );

INTERNAL cthread_pool_elt_p_t cthread_pool_alloc (
        unsigned32   /*n_threads*/,
        boolean32    /*is_default_pool*/,
        unsigned32  * /*status*/
    );

INTERNAL void cthread_pool_set_threadcnt (
        cthread_pool_elt_p_t  /*p*/,
        unsigned32   /*n_threads*/,
        unsigned32  * /*status*/
    );

INTERNAL void cthread_pool_free (
        cthread_pool_elt_p_t  /*p*/,
        unsigned32  * /*status*/
    );

INTERNAL void cthread_pool_start (
        cthread_pool_elt_p_t  /*p*/,
        unsigned32  * /*status*/
    );

INTERNAL void cthread_pool_stop (
        cthread_pool_elt_p_t  /*p*/,
        unsigned32  /*wait_flag*/,
        unsigned32  * /*status*/
    );

INTERNAL cthread_elt_p_t cthread_pool_assign_thread (
        cthread_pool_elt_p_t     /*p*/
    );

INTERNAL void cthread_pool_queue_call (
        cthread_pool_elt_p_t     /*p*/,
        rpc_call_rep_p_t         /*call_rep*/,
        unsigned32              * /*status*/
    );

INTERNAL rpc_call_rep_p_t cthread_pool_dequeue_first (
        cthread_pool_elt_p_t     /*p*/
    );

INTERNAL boolean32 cthread_call_dequeue (
        rpc_call_rep_p_t         /*call_rep*/
    );


/*
**++
**
**  ROUTINE NAME:       cthread_create
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Create a call thread and initialize the table entry.
**
**  INPUTS:             
**
**      cthread         The cthread_table entry to use.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

INTERNAL void cthread_create 
(
    cthread_elt_p_t volatile cthread,
    unsigned32              *status
)
{ 
    dcethread*               handle_copy;

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    CODING_ERROR (status);

    /*
     * Create a thread for this entry, passing it a pointer its
     * call thread table entry.  Detach the thread since no one
     * ever joins with the thread (and we don't want it to become
     * forever zombie'd when it terminates).
     */
    DCETHREAD_TRY {
        dcethread_create_throw (&cthread->thread_id,
                    &rpc_g_server_dcethread_attr,
                    (dcethread_startroutine)cthread_call_executor,
                    (dcethread_addr)cthread);

        cthread->thread_state = RPC_C_IDLE_CTHREAD;

        handle_copy = cthread->thread_id;
        dcethread_detach_throw(handle_copy);

        *status = rpc_s_ok;
    } DCETHREAD_CATCH_ALL(THIS_CATCH) {
        *status = rpc_s_cthread_create_failed;
		  /* FIXME MNE */
		  fprintf(stderr, "XXX MIREK: %s: %s: %d: cthread creation failure\n",
				  __FILE__, __PRETTY_FUNCTION__, __LINE__);
    } DCETHREAD_ENDTRY
        
    return;
}

/*
**++
**
**  ROUTINE NAME:       cthread_call_executor
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  The base routine of all call executor threads.  Loop awaiting
**  and processing calls until told to stop.
**      
**  INPUTS:
**
**      cthread         Pointer to the thread's call table element
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

INTERNAL void cthread_call_executor 
(
  cthread_elt_p_t cthread
)
{
    rpc_call_rep_t          *call_rep = NULL;
    rpc_cthread_pvt_info_p_t pvt = NULL;
    cthread_pool_elt_p_t    p = cthread->pool;
    boolean                 skip_startup = true;

    /*
     * Call executors execute with general cancelability disabled
     * until the stub dispatched to the manager.  This prevents the
     * call executor from having a pending cancel delivered to it before
     * the manager is called.
     */
    dcethread_enableinterrupt_throw(0);

    RPC_MUTEX_LOCK (cthread_mutex);
    
    if (CTHREAD_POOL_IS_QUEUE_EMPTY(p))
    {
	skip_startup = false;
    }

    /*
     * Loop executing calls until we're told to exit.
     */
    while (true)
    {
        boolean run_queued_call = false;

	if (!skip_startup) 
	{
	    /*
	     * Update the pool's idle thread info.
	     */
	    CTHREAD_POOL_IDLE_THREAD(p, cthread);

	    /*
	     * Wait for a call assignment (or until we're told to exit).
	     */
	    while (cthread->thread_state == RPC_C_IDLE_CTHREAD && ! p->stop)
	    {
		RPC_COND_WAIT (cthread->thread_cond, cthread_mutex);
	    }

	    /*
	     * If we've been told to stop, then do so.
	     */
	    if (p->stop)
	    {
		break;
	    }

	    /*
	     * Setup the call that was assigned to us.
	     */
	    call_rep = cthread->call_rep;
	    assert(call_rep != NULL);
	    pvt = &call_rep->u.server.cthread;
	}
        /*
         * Execute the call assigned to us, followed by any queued calls.
         */
        do
        {
	    if (!skip_startup) 
	    {
		RPC_DBG_PRINTF (rpc_e_dbg_general, 15,
		    ("(cthread_call_executor) pool %x cthread %x executing call_rep %x\n",
			p, cthread, call_rep));


		/*
		 * Unlock the cthread_mutex while the call is executing.
		 */
		RPC_MUTEX_UNLOCK (cthread_mutex);

		/*
		 * Invoke the routine provided when this thread was invoked
		 * with the argument provided.  The routine is always called
		 * with general cancelability disabled AND WITH THE CALL LOCKED.
		 * Since we don't have reference counts (in the common code)
		 * this call reference and lock is 'handed off' to the routine 
		 * (which is responsible for releasing the lock).  Upon completion
		 * of the 'routine' we can no longer reference the call (it may
		 * no longer exist).
		 */

		RPC_CALL_LOCK(cthread->call_rep);
		(*(pvt->executor)) (pvt->optargs, run_queued_call);

		/*
		 * Reacquire the cthread mutex and check for queued calls.
		 * As the above somment sez; we no longer hold the call lock
		 * at this point.
		 */

		RPC_MUTEX_LOCK (cthread_mutex);
	    }
            /*
             * Select the oldest queued call; remove it from its queue(s)
             * and setup to execute it.
             */

	    skip_startup = false;
            if (CTHREAD_POOL_IS_QUEUE_EMPTY(p))
            {
                run_queued_call = false;
                continue;
            }
            call_rep = cthread_pool_dequeue_first(p);
            pvt = &call_rep->u.server.cthread;

            /*
             * Fill in the thread_h of the protocol specific call
             * handle for use by the protocol module.
             */
            pvt->thread_h = cthread->thread_id;

            /*
             * Update the cthread table entry for this call just to be
             * consistent.
             */
            cthread->call_rep = call_rep;

            /*
             * Indicate there's a queued call to process.
             */
            run_queued_call = true;
        } while (run_queued_call);

        /*
         * Free up this thread to be allocated again.
         */
        cthread->thread_state = RPC_C_IDLE_CTHREAD;
    }

    RPC_DBG_PRINTF (rpc_e_dbg_general, 5,
        ("(cthread_call_executor) pool %x cthread %x stopped\n",
        p, cthread));

    /*
     * Notify others that the cthread is exiting.
     */

    cthread->thread_state = RPC_C_NO_CTHREAD;
    RPC_COND_BROADCAST (cthread->thread_cond, cthread_mutex);

    RPC_MUTEX_UNLOCK (cthread_mutex);
}


/*
**++
**
**  ROUTINE NAME:       cthread_reaper
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Free pools as they become idle 
**  (this is run periodically from the timer thread).
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:
**      cthread_reaper_queue    the queue of waiting to be freed pools
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void cthread_reaper
(
  pointer_t   unused_arg ATTRIBUTE_UNUSED
)
{
    cthread_pool_elt_p_t    p, np;
    unsigned32              i;
    unsigned32              st;
    cthread_elt_p_t         cthread;
    boolean                 free_pool;

    RPC_MUTEX_LOCK (cthread_mutex);

    /*
     * Scan the queue looking for (and freeing) idle pools.
     */
    RPC_LIST_FIRST(cthread_reaper_queue, p, cthread_pool_elt_p_t);
    while (p != NULL)
    {
        free_pool = true;
        if (p->ctbl != NULL)
        {
            /*
             * See if all of the pool's threads have completed.
             */
            for (i = 0, cthread = &p->ctbl[0]; i < p->n_threads; i++, cthread++)
            {
                if (cthread->thread_state != RPC_C_NO_CTHREAD)
                {
                    free_pool = false;
                    break;
                }
            }
        }

        if (! free_pool)
        {
            RPC_LIST_NEXT (p, p, cthread_pool_elt_p_t);
            continue;
        }


        RPC_DBG_PRINTF (rpc_e_dbg_general, 5,
            ("(cthread_reaper) freeing pool %x\n", p));

        /*
         * Remove the pool from the reaper's queue (pool free really
         * frees the storage)... but first, determine the "next pool"
         * so we can continue the scan.
         */

        RPC_LIST_NEXT (p, np, cthread_pool_elt_p_t);
        RPC_LIST_REMOVE (cthread_reaper_queue, p);

        /*
         * Free up the pool's descriptor.
         */
        cthread_pool_free(p, &st);

        /*
         * Continue scanning with the next on the list.
         */
        p = np;
    }

    /*
     * Shutdown the reaper timer when there's nothing to reap.
     */
    if (RPC_LIST_EMPTY(cthread_reaper_queue))
        rpc__timer_clear(&cthread_reaper_timer);

    RPC_MUTEX_UNLOCK (cthread_mutex);
}

/*
**++
**
**  ROUTINE NAME:       cthread_pool_alloc
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Allocate the resources for a pool (cthread_pool_start() actually creates the
**  threads).
**
**  INPUTS:             
**
**      n_threads       number of call threads in the pool
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      p               the created pool
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL cthread_pool_elt_p_t cthread_pool_alloc 
(
    unsigned32  n_threads,
    boolean32   is_default_pool,
    unsigned32  *status
)
{
    cthread_pool_elt_p_t    p = NULL;

    CODING_ERROR (status);

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    /*
     * Check for the sanity of the number of threads.
     */
    if (n_threads == 0)
    {
        *status = rpc_s_max_calls_too_small;
        return p;
    }

    /*
     * Alloc a pool descriptor.
     */
    RPC_MEM_ALLOC (p, 
                   cthread_pool_elt_p_t,
                   sizeof (cthread_pool_elt_t),
                   RPC_C_MEM_CTHREAD_POOL, 
                   RPC_C_MEM_WAITOK);

    if (p == NULL)
    {
        *status = rpc_s_no_memory;
        return p;
    }

    /*
     * Init the fields in the pool descriptor.
     */
    RPC_LIST_INIT (p->link);
    p->n_threads    = n_threads;
    p->n_idle       = 0;
    p->ctbl         = NULL;
    p->idle_cthread = NULL;
    p->n_queued     = 0;

    /*
     * If the application has indicated a preference for the call queue depth
     * of the default pool, use that.  Otherwise, default to 8 times the number
     * of threads in the pool.
     */
    if (is_default_pool && cthread_default_call_queue_size != 0)
        p->max_queued   = cthread_default_call_queue_size;
    else
        p->max_queued   = RPC_C_CTHREAD_QUEUE_MULTIPLIER * n_threads;

    RPC_LIST_INIT (p->call_queue);
    RPC_LIST_INIT (p->free_queue);
    p->stop         = false;
    p->queue_elt_alloc = ! is_default_pool;

    *status = rpc_s_ok;

/*
CLEANUP:
*/
    if (*status != rpc_s_ok)
    {
        if (p != NULL)
            RPC_MEM_FREE (p, RPC_C_MEM_CTHREAD_POOL);
        p = NULL;
    }

    return p;
}

/*
**++
**
**  ROUTINE NAME:       cthread_pool_set_threadcnt
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Modify the number of threads associated with the pool
**  This is not intended to generically work; this is only
**  suppose to work on "idle" pools (alloc'ed but not started,
**  or started and then stopped).
**
**  INPUTS:             
**
**      p               the pool who's count to modify
**      n_threads       the new number of threads
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

INTERNAL void cthread_pool_set_threadcnt 
(
    cthread_pool_elt_p_t p,
    unsigned32  n_threads,
    unsigned32  *status
)
{
    CODING_ERROR (status);

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    /*
     * Check for the sanity of the number of threads.
     */
    if (n_threads == 0)
    {
        *status = rpc_s_max_calls_too_small;
        return;
    }

    p->n_threads    = n_threads;
    
    /*
     * Use a default call queue size if we're operating on a private pool,
     * or if this is the default pool and the application hasn't previously
     * specified a default call queue size for the default pool.
     */
    if (p != cthread_default_pool || cthread_default_call_queue_size == 0)
        p->max_queued   = RPC_C_CTHREAD_QUEUE_MULTIPLIER * n_threads;

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       cthread_pool_free
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Free the (assumed idle) pool's resources.
**
**  INPUTS:             
**
**      p               the pool to free
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

INTERNAL void cthread_pool_free 
(
    cthread_pool_elt_p_t p,
    unsigned32  *status
)
{
    unsigned32              i;
    cthread_elt_p_t         cthread;

    CODING_ERROR (status);

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    /*
     * The assumption is that the pool is idle (all of its threads
     * have terminated).
     */

    /*
     * Clean up and free the ctbl.  If there is a ctbl, the assumption
     * is that all of the ctable's entries have valid initialized cv's.
     */
    if (p->ctbl)
    {
        for (i = 0, cthread = &p->ctbl[0]; i < p->n_threads; i++, cthread++)
        {
            RPC_COND_DELETE (cthread->thread_cond, cthread_mutex);
        }
        RPC_MEM_FREE (p->ctbl, RPC_C_MEM_CTHREAD_CTBL);
        p->ctbl = NULL;
    }

    /*
     * Free up the queue elt table.
     */
    while (! RPC_LIST_EMPTY(p->free_queue))
    {
        cthread_queue_elt_p_t qe;

        RPC_LIST_REMOVE_TAIL(p->free_queue, qe, cthread_queue_elt_p_t);
        RPC_MEM_FREE (qe, RPC_C_MEM_CTHREAD_QETBL);
    }

    /*
     * Free the pool descriptor.
     */
    RPC_MEM_FREE (p, RPC_C_MEM_CTHREAD_POOL);

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       cthread_pool_start
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Start up the call execution threads for an existing pool.
**
**  INPUTS:             
**
**      p               the pool to start
**      n_threads       number of call threads in the pool
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

INTERNAL void cthread_pool_start 
(
    cthread_pool_elt_p_t p,
    unsigned32  *status
)
{
    unsigned32              i;
    cthread_elt_p_t         cthread;

    CODING_ERROR (status);

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);
         
    /*
     * The pool should not currently have any actual call threads.
     */
    if (p->ctbl != NULL)
    {
        RPC_DBG_GPRINTF (
            ("(cthread_pool_start) pool %x orphaning ctbl\n", p));
    }

    /*
     * Allocate the pool's call thread table.
     */
    RPC_MEM_ALLOC (p->ctbl, 
                   cthread_elt_p_t,
                   p->n_threads * (sizeof (cthread_elt_t)),
                   RPC_C_MEM_CTHREAD_CTBL, 
                   RPC_C_MEM_WAITOK);

    if (p->ctbl == NULL)
    {
        *status = rpc_s_no_memory;
        return;
    }

    /*
     * Init the pool's cthread table / create the cthreads.
     * Do this in two phases to ensure that the table is
     * sane in the event that thread creation fails and cleanup
     * is necessary.
     */

    for (i = 0, cthread = &p->ctbl[0]; i < p->n_threads; i++, cthread++)
    {
        cthread->pool = p;
        cthread->thread_state = RPC_C_NO_CTHREAD;
        RPC_COND_INIT (cthread->thread_cond, cthread_mutex);
    }

    for (i = 0, cthread = &p->ctbl[0]; i < p->n_threads; i++, cthread++)
    {
        cthread_create(cthread, status);
        if (*status != rpc_s_ok)
        {
            RPC_DBG_GPRINTF (
                ("(cthread_pool_start) pool %x couldn't create thread %d\n", p, i));
            goto CLEANUP;
        }
    }

    /*
     * Setup additional fields in the pool descriptor.
     */
    p->n_idle       = 0;
    p->idle_cthread = NULL;
    p->n_queued     = 0;
    RPC_LIST_INIT (p->call_queue);
    RPC_LIST_INIT (p->free_queue);

    /*
     * Allocate the pool's queue elements if necessary.
     */
    if (p->queue_elt_alloc)
    {
        for (i = 0; i < p->max_queued; i++)
        {
            cthread_queue_elt_p_t qe;

            RPC_MEM_ALLOC (qe, 
                           cthread_queue_elt_p_t,
                           sizeof (cthread_queue_elt_t),
                           RPC_C_MEM_CTHREAD_QETBL,
                           RPC_C_MEM_WAITOK);

            if (qe == NULL)
            {
                *status = rpc_s_no_memory;
                goto CLEANUP;
            }
            
            qe->pool = p;
            RPC_LIST_ADD_TAIL (p->free_queue, qe, cthread_queue_elt_p_t);
        }
    }

    RPC_DBG_PRINTF (rpc_e_dbg_general, 5, 
        ("(cthread_pool_start) pool %x (%d threads)\n", p, p->n_threads));

    /*
     * Tell the pool's threads to start.
     */
    p->stop         = false;

    *status = rpc_s_ok;

CLEANUP:
    
    if (*status != rpc_s_ok)
    {
        unsigned32  st;

        if (p->ctbl != NULL) 
        {
            cthread_pool_stop(p, true /* wait */, &st);
            p->ctbl = NULL;
        }

        while (! RPC_LIST_EMPTY(p->free_queue))
        {
            cthread_queue_elt_p_t qe;

            RPC_LIST_REMOVE_TAIL(p->free_queue, qe, cthread_queue_elt_p_t);
            RPC_MEM_FREE (qe, RPC_C_MEM_CTHREAD_QETBL);
        }
    }
}

/*
**++
**
**  ROUTINE NAME:       cthread_pool_stop
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Stop the pool's call threadas.
**
**  INPUTS:             
**
**      p               the pool to stop
**      wait_flag       T => wait for threads to stop
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

INTERNAL void cthread_pool_stop 
(
    cthread_pool_elt_p_t p,
    unsigned32 wait_flag,
    unsigned32  *status
)
{
    int                     cs;
    unsigned32              i;
    cthread_elt_p_t         cthread;

    CODING_ERROR (status);

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    /*
     * If there are no threads associated with the pool, we're done.
     */
    if (p->ctbl == NULL)
    {
        *status = rpc_s_ok;
        return;
    }

    RPC_DBG_PRINTF (rpc_e_dbg_general, 5,
            ("(cthread_pool_stop) pool %x (%d threads) stopping\n", 
            p, p->n_threads));

    /*
     * Tell the threads to stop when they complete the current activities.
     */
    p->stop = true;

    /*
     * Unblock any waiting call threads so they detect the 'stop' condition.
     */
    for (i = 0, cthread = &p->ctbl[0]; i < p->n_threads; i++, cthread++)
    {
        if (cthread->thread_state != RPC_C_NO_CTHREAD)
        {
            RPC_COND_SIGNAL (cthread->thread_cond, cthread_mutex);
        }
    }

    /*
     * If not waiting, we're done.
     */
    if (!wait_flag)
    {
        *status = rpc_s_ok;
        return;
    }

    /*
     * Disable cancel delivery while awaiting cthread termination.  This
     * ensures completion and preservation of invariants.  If it becomes
     * necessary, we can allow cancels and setup a cleanup handler and
     * in the event of a cancel, queue the pool to the reaper for final
     * cleanup.
     */
    cs = dcethread_enableinterrupt_throw (0);

    /*
     * Wait for all call threads to complete.
     *
     * We wait on the call thread's private cv; the cthread signals its
     * cv prior to exiting.  While dcethread_join() would have done the
     * trick; this scheme works just as well and is portable to environments
     * that may have difficulty implementing join (i.e. for Kernel RPC).
     */
    for (i = 0, cthread = &p->ctbl[0]; i < p->n_threads; i++, cthread++)
    {
        while (cthread->thread_state != RPC_C_NO_CTHREAD)
        {
            RPC_COND_WAIT (cthread->thread_cond, cthread_mutex);
        }
    }

    /*
     * Restore the cancel state.
     */
    dcethread_enableinterrupt_throw (cs);

    RPC_DBG_PRINTF (rpc_e_dbg_general, 5,
            ("(cthread_pool_stop) pool %x (%d threads) stopped\n", 
            p, p->n_threads));

    /*
     * Clean up and free the ctbl.  If there is a ctbl, the assumption
     * is that all of the ctable's entries have valid initialized cv's.
     */
    for (i = 0, cthread = &p->ctbl[0]; i < p->n_threads; i++, cthread++)
    {
        RPC_COND_DELETE (cthread->thread_cond, cthread_mutex);
    }
    RPC_MEM_FREE (p->ctbl, RPC_C_MEM_CTHREAD_CTBL);
    p->ctbl = NULL;

    /*
     * Free up the queue elt list.
     */
    while (! RPC_LIST_EMPTY(p->free_queue))
    {
        cthread_queue_elt_p_t qe;

        RPC_LIST_REMOVE_TAIL(p->free_queue, qe, cthread_queue_elt_p_t);
        RPC_MEM_FREE (qe, RPC_C_MEM_CTHREAD_QETBL);
    }

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       cthread_pool_assign_thread
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Locate an idle thread in the indicated pool.
**
**  INPUTS:             
**
**      p               the pool to search
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      cthread         the assigned thread (NULL if none found)
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL cthread_elt_p_t cthread_pool_assign_thread 
(
    cthread_pool_elt_p_t    p
)
{
    cthread_elt_p_t cthread = NULL;

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    /*
     * Locate an idle call thread (if one exists).
     */
    if (p->n_idle > 0)
    {
        if (p->idle_cthread != NULL)
        {
            cthread = p->idle_cthread;
            assert(cthread->thread_state == RPC_C_IDLE_CTHREAD);
            p->idle_cthread = NULL;
        }
        else
        {
            cthread_elt_p_t ct;

            for (ct = p->ctbl; ct < &p->ctbl[p->n_threads]; ct++)
            {
                if (ct->thread_state == RPC_C_IDLE_CTHREAD)
                {
                    cthread = ct;
                    break;
                }
            }
        }
    }

    if (cthread != NULL)
    {
        cthread->thread_state = RPC_C_ACTIVE_CTHREAD;
        p->n_idle--;
    }

    return cthread;
}

/*
**++
**
**  ROUTINE NAME:       cthread_pool_queue_call
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Attempt to queue a call for deferred execution.
**
**  INPUTS:             
**
**      p               the call's pool
**      call_rep        the call
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
**          rpc_s_cthread_not_found
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

INTERNAL void cthread_pool_queue_call 
(
    cthread_pool_elt_p_t    p,
    rpc_call_rep_p_t        call_rep,
    unsigned32              *status
)
{
    rpc_cthread_pvt_info_p_t    pvt = &call_rep->u.server.cthread;
    boolean                     is_default_pool = (p == cthread_default_pool);

    CODING_ERROR (status);

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    /*
     * If the queue is full, we're done.
     */
    if (CTHREAD_POOL_IS_QUEUE_FULL (p))
    {
        RPC_DBG_GPRINTF ((
            "(cthread_pool_queue_call) pool %x full call_rep %x\n", p, call_rep));
        *status = rpc_s_cthread_not_found;
        return;
    }

    /*
     * Indicate that the call is queued.
     */
    pvt->is_queued = true;

    /*
     * Always add the call to the default pool's queue.
     *
     * ONLY Update the default pool's n_queued if the call is for the
     * default pool (see the cthread_pool_elt description comments above)!
     */
    RPC_LIST_ADD_TAIL (cthread_default_pool->call_queue, 
                        call_rep, rpc_call_rep_p_t);
    if (is_default_pool)
    {
        pvt->qelt = NULL;
        p->n_queued++;
    }

    /*
     * If it's a reserved pool, add it to its queue too.
     */
    if (! is_default_pool)
    {
        cthread_queue_elt_p_t qelt;

        RPC_LIST_REMOVE_HEAD(p->free_queue, qelt, cthread_queue_elt_p_t);
        assert (qelt != NULL);

        qelt->call_rep = call_rep;
        pvt->qelt = (pointer_t)qelt;

        RPC_LIST_ADD_TAIL (p->call_queue, qelt, cthread_queue_elt_p_t);
        p->n_queued++;
    } 

    RPC_DBG_PRINTF (rpc_e_dbg_general, 5,
        ("(cthread_pool_queue_call) pool %x (now %d) call_rep %x\n", 
            p, p->n_queued, call_rep));

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       cthread_pool_dequeue_first
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:        Remove the first queued call rep from a pool.
**      
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:    
**
**      p               The pool of interest
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      call_rep        The dequeued call rep (may be NULL).
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL rpc_call_rep_p_t cthread_pool_dequeue_first 
(
    cthread_pool_elt_p_t    p
)
{
    rpc_call_rep_p_t            call_rep;
    boolean                     is_default_pool = (p == cthread_default_pool);

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    /*
     * If the queue is empty we're done.
     */
    if (CTHREAD_POOL_IS_QUEUE_EMPTY(p))
    {
        return NULL;
    }

    /*
     * Determine the call rep of interest and then dequeue it.
     */
    if (is_default_pool)
    {
        /*
         * The default pool's queue is the queue of call reps.
         */
        RPC_LIST_FIRST (p->call_queue,
                          call_rep,
                          rpc_call_rep_p_t);
    }
    else
    {
        cthread_queue_elt_p_t       qelt;

        /*
         * The call was really for a reserved pool; determine the
         * call rep via the indirection queue elt.
         */
        RPC_LIST_FIRST (p->call_queue,
                          qelt,
                          cthread_queue_elt_p_t);

        call_rep = qelt->call_rep;
        assert ((cthread_queue_elt_p_t)call_rep->u.server.cthread.qelt == qelt);
    }

    (void) cthread_call_dequeue (call_rep);

    return call_rep;
}

/*
**++
**
**  ROUTINE NAME:       cthread_call_dequeue
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:        Remove a call rep from the call executor
**                      thread waiting queue, if it's there.
**      
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:    
**
**      call_rep        The call rep to be dequeued.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean
**
**                      T => call was previously queued.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean32 cthread_call_dequeue 
(
    rpc_call_rep_p_t        call_rep
)
{
    rpc_cthread_pvt_info_p_t    pvt = &call_rep->u.server.cthread;
    cthread_queue_elt_p_t       qelt = (cthread_queue_elt_p_t)pvt->qelt;
    cthread_pool_elt_p_t        p;

    RPC_MUTEX_LOCK_ASSERT (cthread_mutex);

    /*
     * If call's not queued, were done.
     */
    if (! pvt->is_queued)
    {
        return false;
    }

    /*
     * Dequeue the call from the default pool.
     */
    RPC_LIST_REMOVE (cthread_default_pool->call_queue, call_rep);

    /*
     * The call may or may not been for the default pool.
     */
    if (qelt == NULL)
    {
        /*
         * The call was for the default pool; adjust the
         * default pool queue count (see the cthread_pool_elt
         * description).
         */
        p = cthread_default_pool;
        cthread_default_pool->n_queued--;
    }
    else
    {
        /*
         * The call was really for a reserved pool;
         * remove it from that queue too.
         */
        p = qelt->pool;

        assert (qelt->call_rep == call_rep);
        assert ((cthread_queue_elt_p_t)pvt->qelt == qelt);

        RPC_LIST_REMOVE (p->call_queue, qelt);
        p->n_queued--;

        /*
         * return the queue elt to its free list.
         */
        qelt->call_rep = NULL;
        RPC_LIST_ADD_HEAD  (p->free_queue,
                          qelt,
                          cthread_queue_elt_p_t);
    }

    /*
     * The call is no longer queued.
     */
    pvt->is_queued = false;
    pvt->qelt = NULL;

    RPC_DBG_PRINTF (rpc_e_dbg_general, 5,
        ("(cthread_call_dequeue) pool %x (%d remain) call_rep %x\n", 
        p, p->n_queued, call_rep));

    return true;
}

/*
**++
**
**  ROUTINE NAME:       rpc__cthread_init
**
**  SCOPE:              PRIVATE - declared in comcthd.h
**
**  DESCRIPTION:
**      
**  Initialize the cthread package.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

PRIVATE void rpc__cthread_init 
(
    unsigned32  *status
)
{
    CODING_ERROR (status);

    RPC_MUTEX_INIT (cthread_mutex);

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_create_thread_pool
**
**  SCOPE:              PUBLIC - declared in rpcpvt.idl
**
**  DESCRIPTION:
**      
**  Allocate the resources for a pool (rpc__cthread_pool_start() actually 
**  creates the threads).
**
**  INPUTS:             
**
**      n_threads       number of call threads in the pool
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      phandle         Handle to the new pool.
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

PUBLIC void rpc_server_create_thread_pool (n_threads, phandle, status)

unsigned32      n_threads;
rpc_thread_pool_handle_t *phandle;
unsigned32      *status;

{
    cthread_pool_elt_p_t    p;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    *phandle = NULL;

    RPC_MUTEX_LOCK (cthread_mutex);

    p = cthread_pool_alloc(n_threads, false /* is_default_pool */, status);
    if (*status != rpc_s_ok)
        goto CLEANUP;

    /*
     * Make the newly created pool "public".
     */
    RPC_LIST_ADD_TAIL (cthread_reserved_pools, p, cthread_pool_elt_p_t);
    *phandle = (rpc_thread_pool_handle_t) p;
    
    /*
     * Normally, reserved pools are started up when the default pool
     * gets started, as a consequence of calling rpc_server_listen.
     * However, if the default pool has already been started up, then
     * start up this reserved pool immediately so that it will be available
     * for handling calls. 
     */   
    if (cthread_invoke_enabled)
        cthread_pool_start (p, status);
    
CLEANUP:

    RPC_MUTEX_UNLOCK (cthread_mutex);
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_free_thread_pool
**
**  SCOPE:              PUBLIC - declared in rpcpvt.idl
**
**  DESCRIPTION:
**      
**  Stop the pool's call threads and free the pool resources.
**
**  INPUTS:             
**
**      phandle         Pool to free
**      wait_flag       T => wait for threads to stop
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

PUBLIC void rpc_server_free_thread_pool 
(
    rpc_thread_pool_handle_t *phandle,
    boolean32       wait_flag,
    unsigned32      *status
)
{
    cthread_pool_elt_p_t    p = (cthread_pool_elt_p_t) *phandle;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_MUTEX_LOCK (cthread_mutex);

    /*
     * Remove the pool from the set of reserved pools.
     * For all practical external purposes, the reserved pool
     * no longer exists (though its cthreads may be still executing
     * their current (and queued) calls.
     */
    RPC_LIST_REMOVE (cthread_reserved_pools, p);

    /*
     * Stop the pool's threads (waiting as directed).
     */
    cthread_pool_stop(p, wait_flag, status);

    /*
     * If we waited for the pool to become idle we can immediately free it;
     * otherwise we've got to queue it for eventual freeing (and start up the
     * reaper timer if this is the first item being queued).
     */
    if (wait_flag || p->ctbl == NULL)
    {
        unsigned32  st;
        cthread_pool_free(p, &st);
    }
    else
    {
        if (RPC_LIST_EMPTY(cthread_reaper_queue))
        {
            rpc__timer_set(&cthread_reaper_timer,
                cthread_reaper, NULL, RPC_C_CTHREAD_REAPER_FREQ);
        }
        RPC_LIST_ADD_TAIL (cthread_reaper_queue, p, cthread_pool_elt_p_t);
    }

    *phandle = NULL;

	 /*
CLEANUP:
*/
    RPC_MUTEX_UNLOCK (cthread_mutex);
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_set_thread_pool_fn
**
**  SCOPE:              PUBLIC - declared in rpcpvt.idl
**
**  DESCRIPTION:
**      
**  [Un]Register a thread pool lookup function with the runtime.
**
**  INPUTS:             
**
**      pool_fn         the lookup function - may be NULL
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

PUBLIC void rpc_server_set_thread_pool_fn 
(
    rpc_thread_pool_fn_t pool_fn,
    unsigned32      *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_MUTEX_LOCK (cthread_mutex);

    if (pool_fn != NULL && cthread_pool_lookup_fn != NULL)
    {
        *status = -1; /* !!! already set */
        goto CLEANUP;
    }

    cthread_pool_lookup_fn = pool_fn;   /* be it NULL or otherwise */
    *status = rpc_s_ok;

CLEANUP:

    RPC_MUTEX_UNLOCK (cthread_mutex);
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_set_thread_qlen
**
**  SCOPE:              PUBLIC - declared in rpcpvt.idl
**
**  DESCRIPTION:
**      
**  Adjust the maximum number of queued calls for a specified thread pool.
**
**  INPUTS:             
**
**      phandle         the pool whose queue size is being adjusted
**                      a NULL argument can be used to specify that the
**                      the operation should be applied to the default pool.
**      queue_size      the new size
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
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

PUBLIC void rpc_server_set_thread_pool_qlen 
(
    rpc_thread_pool_handle_t phandle,
    unsigned32 queue_size,
    unsigned32 * status 
)
{
    cthread_pool_elt_p_t pool = (cthread_pool_elt_p_t) phandle;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_MUTEX_LOCK (cthread_mutex);

    *status = rpc_s_ok;

    /*
     * If the caller sent NULL as the pool parameter, apply the operation
     * to the default pool.
     */
    if (pool == NULL)
    {
        cthread_default_call_queue_size = queue_size;

        /*
         * If the default pool hasn't been started yet, we're done; the
         * global value will be used when it does get started up.  If the
         * default pool *has* been started, just update its max_queued 
         * value. 
         */
        if (cthread_default_pool != NULL)
        {
            cthread_default_pool->max_queued = queue_size;
        }
    }
    else
    {
        unsigned32 i;

        /*
         * We're operating on a private pool...
         *
         * If this pool has not been started yet, just record the value for
         * the max queue size.  The actual queue element data structure will
         * get created when the pool is started.
         */
        if (RPC_LIST_EMPTY(pool->free_queue))
        {
            pool->max_queued = queue_size;
        }
        else
        {
            /*
             * This private pool has already been started.
             *
             * Considering that calls may currently be queued for this pool, it
             * would be extremely tricky, not to mention probably not useful, to
             * allow the caller to shrink the call queue length.  Only update the
             * queue length if it's being increased.
             */

            if (queue_size > pool->max_queued)
            {
                /*
                 * Alloc up some more queue elements, and add them to the list.
                 */
                for (i = pool->max_queued; i < queue_size; i++)
                {
                    cthread_queue_elt_p_t qe;

                    RPC_MEM_ALLOC (qe,
                                   cthread_queue_elt_p_t,
                                   sizeof (cthread_queue_elt_t),
                                   RPC_C_MEM_CTHREAD_QETBL,
                                   RPC_C_MEM_WAITOK);

                    if (qe == NULL)
                    {
                        *status = rpc_s_no_memory;
                
                        /*
                         * Try to stay calm...
                         */
                        pool->max_queued = i;

                        RPC_MUTEX_UNLOCK (cthread_mutex);
                        return;
                    }

                    qe->pool = pool;
                    RPC_LIST_ADD_TAIL (pool->free_queue, qe, cthread_queue_elt_p_t);
                }

                pool->max_queued = queue_size;
            }
        }
    }

    RPC_MUTEX_UNLOCK (cthread_mutex);
}

/*
**++
**
**  ROUTINE NAME:       rpc__cthread_start_all
**
**  SCOPE:              PRIVATE - declared in comcthd.h
**
**  DESCRIPTION:
**      
**  Arrange for all the call execution threads to be created and
**  enabled RPC execution. 
**
**  INPUTS:
**
**      default_cthreads The number of default pool call threads which will be
**                      created
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
**          rpc_s_no_memory
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

PRIVATE void rpc__cthread_start_all 
(
    unsigned32              default_pool_cthreads,
    unsigned32              *status
)
{
    cthread_pool_elt_p_t    p;

    CODING_ERROR (status);

    RPC_MUTEX_LOCK (cthread_mutex);

    /*
     * Alloc the default pool if necessary (or just adjust its
     * thread count).
     */
    if (cthread_default_pool == NULL)
    {
        cthread_default_pool = cthread_pool_alloc (
                                    default_pool_cthreads, 
                                    true, /* is_default_pool */
                                    status);
        if (*status != rpc_s_ok)
            goto CLEANUP;
    }
    else
    {
        cthread_pool_set_threadcnt(cthread_default_pool, 
                    default_pool_cthreads, status);
        if (*status != rpc_s_ok)
            goto CLEANUP;
    }

    /*
     * Fire up all of the call executor threads.
     */
    cthread_pool_start (cthread_default_pool, status);
    if (*status != rpc_s_ok)
        goto CLEANUP;

    RPC_LIST_FIRST (cthread_reserved_pools, p, cthread_pool_elt_p_t);
    while (p != NULL)
    {
        cthread_pool_start (p, status);
        if (*status != rpc_s_ok)
            goto CLEANUP;
        RPC_LIST_NEXT (p, p, cthread_pool_elt_p_t);
    }

    /*
     * enable RPC queuing / execution
     */
    cthread_invoke_enabled = true;

    *status = rpc_s_ok;            

CLEANUP:

    RPC_MUTEX_UNLOCK (cthread_mutex);
}

/*
**++
**
**  ROUTINE NAME:       rpc__cthread_stop_all
**
**  SCOPE:              PRIVATE - declared in comcthd.h
**
**  DESCRIPTION:
**      
**  Stop all the call executor threads.  Don't return until all have stopped.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
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

PRIVATE void rpc__cthread_stop_all 
(
    unsigned32              *status
)
{ 

    cthread_pool_elt_p_t    p;
    

    CODING_ERROR (status);

    RPC_MUTEX_LOCK (cthread_mutex);

    /*
     * Disable subsequent call execution processing while we're
     * waiting for the executors to complete.
     */

    cthread_invoke_enabled = false;

    /*
     * Tell each pool to stop.
     */
    cthread_pool_stop(cthread_default_pool, false, status);
    if (*status != rpc_s_ok)
        goto CLEANUP;

    RPC_LIST_FIRST (cthread_reserved_pools, p, cthread_pool_elt_p_t);
    while (p != NULL)
    {
        cthread_pool_stop(p, false, status);
        if (*status != rpc_s_ok)
            goto CLEANUP;
        RPC_LIST_NEXT (p, p, cthread_pool_elt_p_t);
    }
    
    /*
     * Now wait for each pool's threads to complete.
     */
    cthread_pool_stop(cthread_default_pool, true, status);
    if (*status != rpc_s_ok)
        goto CLEANUP;

    RPC_LIST_FIRST (cthread_reserved_pools, p, cthread_pool_elt_p_t);
    while (p != NULL)
    {
        cthread_pool_stop(p, true, status);
        if (*status != rpc_s_ok)
            goto CLEANUP;
        RPC_LIST_NEXT (p, p, cthread_pool_elt_p_t);
    }

    *status = rpc_s_ok;

CLEANUP:

    RPC_MUTEX_UNLOCK (cthread_mutex);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cthread_invoke_null
**
**  SCOPE:              PRIVATE - declared in comcthd.h
**
**  DESCRIPTION:
**
**  Arrange for a call execution thread to (eventually) be allocated to
**  "execute" the RPC.
**      
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      call_rep        The call rep for the incoming call.
**      call_executor   The address of a routine to be called when the
**                      call thread actually wakes up
**      args            A pointer to be passed to the called routine
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
**          rpc_s_cthread_not_found
**          rpc_s_call_queued
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       
**                      call may be queued if no available call executors
**
**--
**/

PRIVATE void rpc__cthread_invoke_null 
(
    rpc_call_rep_p_t        call_rep,
    dce_uuid_p_t                object,
    dce_uuid_p_t                if_uuid,
    unsigned32              if_ver,
    unsigned32              if_opnum,
    rpc_prot_cthread_executor_fn_t cthread_executor,
    pointer_t               args,
    unsigned32              *status
)
{ 
    rpc_cthread_pvt_info_p_t    pvt = &call_rep->u.server.cthread;
    unsigned32                  lookup_fn_st;
    cthread_pool_elt_p_t        p;
    cthread_elt_p_t             cthread;

    CODING_ERROR (status);

    RPC_MUTEX_LOCK (cthread_mutex);

    /*
     * Check to ensure that it's still desireable to queue/execute a call.
     *
     * While strictly speaking we need to examine cthread_invoke_enabled
     * under a mutex, we really don't want to pay the cost in this critical
     * path and I think things will work reasonably safely get by without it.
     * The worst that will happen is that a (couple) extra call(s) will be
     * allowed to be queued / executed during shutdown processing.
     */
    if (cthread_invoke_enabled == false)
    {
        *status = rpc_s_cthread_invoke_disabled;
        goto CLEANUP;
    }

    /*
     * Setup fields in the call rep for subsequent execution.
     */
    pvt->executor = cthread_executor;
    pvt->optargs = args;

    /*
     * Attempt to locate / assign an idle thread (this code is
     * in-line because this is the fast-path).
     */
    CTHREAD_POOL_LOOKUP_RESERVED(object, if_uuid, if_ver, if_opnum, 
        &p, &lookup_fn_st);
    if (lookup_fn_st != 0)
    {
        *status = rpc_s_cthread_not_found;
        goto CLEANUP;
    }

    if (p == NULL)
    {
        /*
         * Only concerned with default pool.
         */
        p = cthread_default_pool;

        CTHREAD_POOL_ASSIGN_THREAD(cthread_default_pool, &cthread);
    }
    else
    {
        /*
         * First assign an idle reserved pool thread; otherwise,
         * assign an idle default pool thread.
         */
        CTHREAD_POOL_ASSIGN_THREAD(p, &cthread);
        if (cthread == NULL)
        {
            CTHREAD_POOL_ASSIGN_THREAD(cthread_default_pool, &cthread);
        }
    }

    /*
     * If we've succeeded in assigning a cthread, arrange for it to
     * actually execute the RPC.  Otherwise, attempt to queue the RPC
     * for deferred execution.
     */
    if (cthread != NULL)
    {
        /*
         * Setup fields in the call rep for subsequent execution.
         */
        pvt->is_queued = false;
        pvt->thread_h = cthread->thread_id;
        cthread->call_rep = call_rep;

        /*
         * Fire up the assigned cthread.
         */
        RPC_COND_SIGNAL(cthread->thread_cond, cthread_mutex);

        *status = rpc_s_ok;
    }
    else
    {
        cthread_pool_queue_call(p, call_rep, status);
        if (*status == rpc_s_ok)
            *status = rpc_s_call_queued;
    }

CLEANUP:

    RPC_MUTEX_UNLOCK (cthread_mutex);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cthread_dequeue
**
**  SCOPE:              PRIVATE - included in comcthd.h
**
**  DESCRIPTION:        Remove a call rep from the call executor
**                      thread waiting queue, if it's there.
**      
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:    
**
**      call_rep        The call rep to be dequeued.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean
**
**                      T => call was previously queued.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean32 rpc__cthread_dequeue 
(
  rpc_call_rep_p_t        call_rep
)
{
    boolean32                   was_dequeued;

    RPC_MUTEX_LOCK (cthread_mutex);

    was_dequeued = cthread_call_dequeue (call_rep);

    RPC_MUTEX_UNLOCK (cthread_mutex);

    return was_dequeued;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cthread_cancel
**
**  SCOPE:              PRIVATE - included in comcthd.h
**
**  DESCRIPTION:        Post a cancel to cthread associated with a call
**      
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:    
**
**      call            The call that the cancel is associated with.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       
**                      a cancel may be posted to the call execution thread
**
**--
**/

PRIVATE void rpc__cthread_cancel 
(
  rpc_call_rep_p_t        call
)
{
    RPC_CALL_LOCK_ASSERT(call);

    if (!call->u.server.cancel.accepting)
        return;

    call->u.server.cancel.count++;

    if (!call->u.server.cancel.queuing)
    {
        rpc_cthread_pvt_info_p_t    pvt = &call->u.server.cthread;

        RPC_MUTEX_LOCK (cthread_mutex);

        dcethread_interrupt_throw(pvt->thread_h);

        RPC_MUTEX_UNLOCK (cthread_mutex);
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc__cthread_cancel_caf
**
**  SCOPE:              PRIVATE - included in comcthd.h
**
**  DESCRIPTION:        Check for pending cancel and flush.
**      
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     
**
**      call            The call that the cancel is associated with.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**                      boolean32 => T iff call had a pending cancel
**
**  SIDE EFFECTS:       
**                      the call will no longer accept cancels
**                      any pending cancels will be flushed (i.e. the
**                      call thread must not have any residual pending
**                      cancels upon completion)
**
**--
**/

PRIVATE boolean32 rpc__cthread_cancel_caf 
(
  rpc_call_rep_p_t        call
)
{
    int oc;

    RPC_CALL_LOCK_ASSERT(call);

    /*
     * In the event this is called multiple times, return something
     * sensible (i.e. return the current "had pending" state).
     */
    if (!call->u.server.cancel.accepting)
    {
        return (call->u.server.cancel.had_pending);
    }

    /*
     * Cancels are no longer accepted by this call.
     */
    call->u.server.cancel.accepting = false;

    /*
     * Determine if the call has a cancel pending (flush any accepted
     * cancels).  Only want to take the expensive path if a cancel request
     * had been previously accepted.
     */
    call->u.server.cancel.had_pending = false;
    if (call->u.server.cancel.count)
    {
#ifndef _PTHREAD_NO_CANCEL_SUPPORT
        oc = dcethread_enableinterrupt_throw(1);
        DCETHREAD_TRY
        {
            dcethread_checkinterrupt();
        }
        DCETHREAD_CATCH(dcethread_interrupt_e)
        {
            call->u.server.cancel.had_pending = true;
        }
        DCETHREAD_ENDTRY
        dcethread_enableinterrupt_throw(oc);
#else
        /*
         * Cancels not supported, so the previously accepted forwarded
         * cancels are still pending.
         */
        call->u.server.cancel.had_pending = true;
#endif
    }
    call->u.server.cancel.count = 0;

    /*
     * Let the caller know if a cancel was pending (without them having 
     * to look at the flag).
     */
    return (call->u.server.cancel.had_pending);
}

/*
**++
**
**  ROUTINE NAME:       rpc__cthread_cancel_enable_post
**
**  SCOPE:              PRIVATE - included in comcthd.h
**
**  DESCRIPTION:        Enable direct posting of cancels to a cthread;
**                      post any previously queued cancels.
**      
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:    
**
**      call            The call that the cancel is associated with.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       
**                      a cancel may be posted to the call execution thread
**
**--
**/

PRIVATE void rpc__cthread_cancel_enable_post 
(
  rpc_call_rep_p_t        call
)
{
    rpc_cthread_pvt_info_p_t    pvt = &call->u.server.cthread;
    unsigned16 cancel_cnt;

    RPC_CALL_LOCK_ASSERT(call);

    RPC_MUTEX_LOCK (cthread_mutex);

    if (call->u.server.cancel.accepting && call->u.server.cancel.queuing)
    {
        call->u.server.cancel.queuing = false;
        for (cancel_cnt = call->u.server.cancel.count; cancel_cnt--; )
        {
            dcethread_interrupt_throw(pvt->thread_h);
        }
    }

    RPC_MUTEX_UNLOCK (cthread_mutex);
}
