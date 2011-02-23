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
**      rpctimer.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types/constants internal to RPC facility and common
**  to all RPC components.
**
**
*/

#include <commonp.h>


/* ========================================================================= */

EXTERNAL rpc_clock_t rpc_g_clock_curr;
GLOBAL boolean32 rpc_g_long_sleep;

INTERNAL rpc_timer_p_t running_list;
INTERNAL rpc_mutex_t   rpc_g_timer_mutex;
INTERNAL rpc_cond_t    rpc_g_timer_cond;
INTERNAL rpc_clock_t   rpc_timer_high_trigger;
INTERNAL rpc_clock_t   rpc_timer_cur_trigger;
INTERNAL rpc_timer_p_t rpc_timer_head, rpc_timer_tail;
INTERNAL unsigned32    stop_timer;
INTERNAL dcethread* timer_task;
INTERNAL boolean timer_task_running = false;
INTERNAL boolean timer_task_was_running = false;

INTERNAL void timer_loop (void);


INTERNAL void rpc__timer_set_int (
        rpc_timer_p_t            /*t*/,
        rpc_timer_proc_p_t       /*proc*/,
        pointer_t                /*parg*/,
        rpc_clock_t              /*freq*/
    );


/*
 * Mutex lock macros 
 */

#define RPC_TIMER_LOCK_INIT(junk)   RPC_MUTEX_INIT (rpc_g_timer_mutex)
#define RPC_TIMER_LOCK(junk)        RPC_MUTEX_LOCK (rpc_g_timer_mutex)
#define RPC_TIMER_UNLOCK(junk)      RPC_MUTEX_UNLOCK (rpc_g_timer_mutex)
#define RPC_TIMER_LOCK_ASSERT(junk) RPC_MUTEX_LOCK_ASSERT(rpc_g_timer_mutex)
#define RPC_TIMER_COND_INIT(junk)   RPC_COND_INIT (rpc_g_timer_cond, rpc_g_timer_mutex)

/* ========================================================================= */


/*
 * T I M E R _ L O O P
 *
 * Periodically get the new clock time and process any ready timer events.
 */

#ifndef NO_RPC_TIMER_THREAD

INTERNAL void timer_loop(void)
{
    RPC_TIMER_LOCK(0);

    while (!stop_timer)
    {
        rpc_clock_t next;
        struct timespec next_ts;
        rpc_clock_t max_step;
        
	/*
	 * It would be real nice if we could figure out a way to get
	 * the system time global variable mapped read-only into our
	 * address space to avoid the !@#$% gettimeofday syscall overhead.
	 */
        rpc__clock_update();
        next = rpc__timer_callout();
	rpc_g_long_sleep = ( next > RPC_CLOCK_SEC(1) );

        /*
         * wake up at least once every 50 seconds, so we don't confuse
         * the underlying rpc__clock_update() code.
         */
        max_step = RPC_CLOCK_SEC(50);   
        if (next == 0 || next > max_step)
            next = max_step;

        if (next > 10)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_timer, 5,
                ("(timer_loop) next event in %d seconds\n", next/RPC_C_CLOCK_HZ));
        }
        rpc_timer_cur_trigger = rpc_g_clock_curr + next;
        rpc__clock_timespec (rpc_timer_cur_trigger, &next_ts);
        RPC_COND_TIMED_WAIT(rpc_g_timer_cond, rpc_g_timer_mutex, &next_ts);
    }

    RPC_TIMER_UNLOCK(0);
}

#endif /* NO_RPC_TIMER_THREAD */

INTERNAL void rpc__timer_prod
(
        rpc_clock_t trigger
)
{
                    
    RPC_DBG_PRINTF(rpc_e_dbg_timer, 5, (
        "(rpc__timer_prod) timer backup; old %d, new %d\n",
        rpc_timer_cur_trigger, trigger
        ));
    /* 
     * forestall "double pokes", which appear to
     * happen occasionally.
     */
    rpc_timer_cur_trigger = trigger;
    RPC_COND_SIGNAL(rpc_g_timer_cond, rpc_g_timer_mutex);
    
}


/*
 **  R P C _ _ T I M E R _ I N I T
 **
 **  Initialize the timer package.
 **/

PRIVATE void rpc__timer_init(void)
{    
    RPC_TIMER_LOCK_INIT (0);
    RPC_TIMER_COND_INIT (0);
    
    running_list            = NULL;
    rpc_timer_high_trigger  = 0;
    rpc_timer_head          = NULL;
    rpc_timer_tail          = NULL;
    rpc_g_clock_curr        = 0;
    rpc_g_long_sleep	    = 0;
    timer_task_running      = false;
    timer_task_was_running  = false;
    
    /*
     * Initialize the current time...
     */
    rpc__clock_update();

#ifndef NO_RPC_TIMER_THREAD
    stop_timer              = 0;
    timer_task_running      = true;
    dcethread_create_throw (
        &timer_task,                            /* new thread    */
        NULL,                   /* attributes    */
        (dcethread_startroutine)timer_loop,    /* start routine */
        NULL);                 /* arguments     */

#endif /* NO_RPC_TIMER_THREAD */
}


#ifdef ATFORK_SUPPORTED
/*
 **  R P C _ _ T I M E R _ F O R K _ H A N D L E R 
 **
 **  Handle timer related fork issues.
 **/ 

PRIVATE void rpc__timer_fork_handler
(
    rpc_fork_stage_id_t stage
)
{  
#ifndef NO_RPC_TIMER_THREAD
    switch ((int)stage)
    {
        case RPC_C_PREFORK:
            RPC_TIMER_LOCK(0);
            timer_task_was_running = false;
            if (timer_task_running)
            {
                stop_timer = 1;
                rpc__timer_prod(0);
                RPC_TIMER_UNLOCK(0);
                dcethread_join_throw(timer_task, NULL);
                RPC_TIMER_LOCK(0);
                /* Why was this ever done?  It doesn't make any sense to
                   perform a detach right after a join
                DCETHREAD_TRY
                {
                    dcethread_detach_throw(timer_task);
                }
                DCETHREAD_CATCH(dcethread_use_error_e)
                {
                }
		DCETHREAD_CATCH(dcethread_badparam_e)
		{
		}
                DCETHREAD_ENDTRY; */
                timer_task_was_running = true;
                timer_task_running = false;
            }
            break;   

        case RPC_C_POSTFORK_PARENT:
            if (timer_task_was_running)
            {
                timer_task_was_running = false;
                timer_task_running = true;
                stop_timer = 0;
                dcethread_create_throw (
                    &timer_task,                          /* new thread    */
                    NULL,                 /* attributes    */
                    (dcethread_startroutine) timer_loop,  /* start routine */
                    NULL);               /* arguments     */
            }
            RPC_TIMER_UNLOCK(0);
            break;

        case RPC_C_POSTFORK_CHILD:  
            timer_task_was_running = false;
            timer_task_running = false;
            stop_timer = 0;
            RPC_TIMER_UNLOCK(0);
            break;
    }
#endif /* NO_RPC_TIMER_THREAD */
}   
#endif /* ATFORK_SUPPORTED */


/*
** R P C _ _ T I M E R _ S E T 
** 
** Lock timer lock, add timer to timer callout queue, and unlock timer lock.
** 
*/

PRIVATE void rpc__timer_set 
(
    rpc_timer_p_t           t,
    rpc_timer_proc_p_t      proc,
    pointer_t               parg,
    rpc_clock_t             freq
)
{    
    RPC_TIMER_LOCK (0);
    rpc__timer_set_int (t, proc, parg, freq);
    RPC_TIMER_UNLOCK (0);
}

/*
**  R P C _ _ T I M E R _ S E T _ I N T
**
**  Insert a routine to be called from the rpc_timer periodic callout queue.
**  The entry address, a single argument to be passed when the routine is
**  called,  together with the frequency with which the routine is to be 
**  called are inserted into a rpc_timer_t structure provided by the 
**  caller and pointed to by 't', which is placed in the queue.
*/


INTERNAL void rpc__timer_set_int 
(
    rpc_timer_p_t           t,
    rpc_timer_proc_p_t      proc,
    pointer_t               parg,
    rpc_clock_t             freq
)
{    
    rpc_timer_p_t       list_ptr, prev_ptr;
    
    /*
     * Insert the routine calling address, the argument to be passed and
     * the frequency into the rpc_timer_t structure.
     */       
    
    t->proc = proc;
    t->parg = parg;
    t->frequency = freq;
    t->trigger = freq + rpc_g_clock_curr;

    /*
     * Insert the rpc_timer_t structure sent by the caller into the
     * rpc_timer queue, by order of the frequency with which it is to be
     * called.
     */
    if (rpc_timer_head != NULL && t->trigger >= rpc_timer_high_trigger)
    {            
        rpc_timer_tail->next = t;
        rpc_timer_tail = t;
        t->next = 0;
        rpc_timer_high_trigger = t->trigger;
        return;
    }          
    
    /*
     * Handle the case in which the element gets inserted somewhere
     * into the middle of the list.
     */                             
    
    prev_ptr = NULL;
    for (list_ptr = rpc_timer_head; list_ptr; list_ptr = list_ptr->next )
    {
        if (t->trigger < list_ptr->trigger)
        {
            if (list_ptr == rpc_timer_head)
            {         
                t->next = rpc_timer_head;
                rpc_timer_head = t;
                /*
                 * If the next time the timer thread will wake up is
                 * in the future, prod it.
                 */
                if (rpc_timer_cur_trigger > t->trigger) 
                    rpc__timer_prod(t->trigger);
            }
            else
            {
                prev_ptr->next = t;
                t->next = list_ptr;
            }
            return;
        }
        prev_ptr = list_ptr;
    }  
    
    /*
     * The only way to get this far is when the head pointer is NULL.  Either
     * we just started up, or possibly there's been no RPC activity and all the
     * monitors have removed themselves.
     */    
    assert (rpc_timer_head == NULL);
    
    rpc_timer_head = rpc_timer_tail = t;
    t->next = NULL;
    rpc_timer_high_trigger = t->trigger;
    if (rpc_timer_cur_trigger > t->trigger) 
        rpc__timer_prod(t->trigger);
}

/*
 **  R P C _ _ T I M E R _ A D J U S T
 **
 **  Search the rpc_timer queue for the rpc_timer_t structure pointed to by
 **   't'.  Then modify the 'frequency' attribute.
 **/

PRIVATE void rpc__timer_adjust 
(
    rpc_timer_p_t           t,
    rpc_clock_t             frequency           
)
{   
    rpc_timer_p_t       ptr;
    
    /*
     * First see if the monitor being adjusted is currently running.
     * If so, just reset the frequency field, when the monitor is rescheduled
     * it will be queued in the proper place.
     */                             
    
    RPC_TIMER_LOCK (0);
    for (ptr = running_list; ptr != NULL; ptr = ptr->next)
    {
        if (t == ptr)
        {
            t->frequency = frequency;
            RPC_TIMER_UNLOCK (0);
            return;
        }
    }
    
    RPC_TIMER_UNLOCK (0);
    /*
     * Otherwise, we need to remove the monitor from its current position
     * in the queue, adjust the frequency, and then replace it in the correct
     * position.
     */
    
    rpc__timer_clear (t);
    t->frequency = frequency;
    rpc__timer_set (t, t->proc, t->parg, t->frequency);
}

/*
 ** R P C _ _ T I M E R _ C L E A R
 **
 **  Remove an rpc_timer_t structure from the rpc_timer scheduling queue.
 **/

PRIVATE void rpc__timer_clear 
(
    rpc_timer_p_t           t
)
{  
    rpc_timer_p_t       list_ptr, prev = NULL;
    
    RPC_TIMER_LOCK (0);
    
    /*
     * First see if the monitor being cleared is currently running.
     * If so, remove it from the running list.
     */  
    
    for (list_ptr = running_list; list_ptr; prev = list_ptr, list_ptr = list_ptr->next)
    {
        if (list_ptr == t)                  
        {
            if (prev == NULL)
                running_list = running_list->next;
            else
                prev->next = list_ptr->next;
            RPC_TIMER_UNLOCK (0);
            return;
        }
    }
    
    /*
     * Next, see if the specified timer element is in the list.
     */
    
    prev = NULL;
    for (list_ptr = rpc_timer_head; list_ptr;
        prev = list_ptr, list_ptr = list_ptr->next)
    {
        if (list_ptr == t)
        {                   
            /*
             * If this element was at the head of the list...
             */
            if (t == rpc_timer_head)
            {
                /*
                 * !!! perhaps need to inform timer thread that next event
                 * is a little further off?
                 */
                rpc_timer_head = t->next;
            }
            else
            {      
                /*
                 * Unlink element from list.  If the element was at the end
                 * of the list, reset the tail pointer and high trigger value.
                 */
                prev->next = t->next;
                if (t->next == NULL)
                {
                    rpc_timer_tail = prev;
                    rpc_timer_high_trigger = prev->trigger;
                }
            }
            RPC_TIMER_UNLOCK (0);
            return;
        }                 
    }   
    
    RPC_TIMER_UNLOCK (0);
}

/*
**  R P C _ _ T I M E R _ C A L L O U T
**
**  Make a pass through the rpc_timer_t periodic callout queue.
**  Make a call to any routines which are ready to run at this time.
**  Return the amount of time until the next routine is scheduled
**  to be run.  Return 0 if the queue is empty.
**  The mutex, RPC_TIMER_LOCK, is LOCKED while this routine is running
**  and while any routines which may be calld are running.
**  
**  the parameter NEXT is set to the "next" time to wake up, if any
**  (suitable for passing to RPC_COND_TIMED_WAIT).
**/

PRIVATE rpc_clock_t rpc__timer_callout (void)
{                              
    unsigned32      ret_val;
    rpc_timer_p_t   ptr, prev;

#if 0    
    RPC_TIMER_LOCK (0);
#endif

    RPC_TIMER_LOCK_ASSERT(0);
    
    /*
     * Go through list, running routines which have trigger counts which
     * have expired.
     */
    
    while (rpc_timer_head && rpc_timer_head->trigger <= rpc_g_clock_curr)
    { 
        ptr = rpc_timer_head;
        rpc_timer_head = rpc_timer_head->next;
        ptr->next = running_list;
        running_list = ptr;

        RPC_TIMER_UNLOCK (0);
        (*ptr->proc) (ptr->parg);
        RPC_TIMER_LOCK (0);
        
        /* 
         * We need to handle two situations here depending on whether
         * the monitor routine has deleted itself from the timer queue.
         * If so, we'll know this from the fact that it will be gone
         * from the running list.  If not, we need to update the head
         * pointer here, and reschedule the current monitor.
         */                                
        
        if (ptr == running_list)
        {
            running_list = running_list->next;
            rpc__timer_set_int (ptr, ptr->proc, ptr->parg, ptr->frequency);
        }
        else
        {
            for (prev = running_list; prev; prev = prev->next)
            {
                if (prev->next == ptr)
                {
                    prev->next = ptr->next;

                    rpc__timer_set_int 
                        (ptr, ptr->proc, ptr->parg, ptr->frequency);

                }
            } 
        }
    }
    
    
    ret_val = (rpc_timer_head == NULL) ? 0
        : (rpc_timer_head->trigger - rpc_g_clock_curr);
    
    RPC_TIMER_LOCK_ASSERT(0);    
    return (ret_val);
} 
