/*
 * 
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
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
**      rpclist.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  This module contains routines to maintain lookaside list descriptors.
**
**
*/

#include <commonp.h>

GLOBAL rpc_lookaside_rcb_t rpc_g_lookaside_rcb =
{
    RPC_C_LOOKASIDE_RES,
    0,
    RPC_C_LOOKASIDE_RES_MAX_WAIT,
    RPC_C_LOOKASIDE_RES_WAIT,
	 RPC_MUTEX_INITIALIZER,
	 RPC_COND_INITIALIZER
};

/*
**++
**
**  ROUTINE NAME:       rpc__list_desc_init
**
**  SCOPE:              PRIVATE - declared in rpclist.h
**
**  DESCRIPTION:
**
**  Initializes a lookaside list descriptor.  The maximum size and element
**  size are set as part of this operation.
**
**  INPUTS:
**
**      list_desc       List descriptor which is to be initialized.
**
**      max_size        The maximum length of the lookaside list.
**
**      element_size    The size of each list element.
**
**      element_type    The type of each list element (from rpcmem.h).
**
**      alloc_rtn       The element specific routine to be
**                      called when allocating from heap. If this is
**                      NULL no routine will be called.
**
**      free_rtn        The element specific alloc routine to be
**                      called when freeing to heap. If this is NULL
**                      no routine will be called.
**
**      mutex           The list specific mutex used to protect the
**                      integrity of the list. If the NULL is
**                      provided the global lookaside list mutex and
**                      condition variable will be used.It is used when 
**                      blocking on or signalling condition
**                      variables. Note that the neither 
**                      rpc__list_element_alloc or _free ever
**                      explicitly acquires or releases this mutex.
**                      This must be done by the caller.
**
**      cond            The list specific condition variable
**                      associated with the above mutex. Valid iff
**                      mutex is not NULL.
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

PRIVATE void rpc__list_desc_init 
(
    rpc_list_desc_p_t               list_desc,
    unsigned32                      max_size,
    unsigned32                      element_size,
    unsigned32                      element_type,
    rpc_list_element_alloc_fn_t     alloc_rtn,
    rpc_list_element_free_fn_t      free_rtn,
    rpc_mutex_p_t                   mutex,
    rpc_cond_p_t                    cond
)
{
    list_desc->max_size = max_size;
    list_desc->cur_size = 0;
    list_desc->element_size = element_size;
    list_desc->element_type = element_type;
    list_desc->alloc_rtn = alloc_rtn;
    list_desc->free_rtn = free_rtn;
    if (mutex == NULL)
    {
        list_desc->use_global_mutex = true;
    }
    else
    {
        list_desc->use_global_mutex = false;
        list_desc->mutex = mutex;
        list_desc->cond = cond;
    }
    RPC_LIST_INIT (list_desc->list_head);
}

/*
**++
**
**  ROUTINE NAME:       rpc__list_element_alloc
**
**  SCOPE:              PRIVATE - declared in rpclist.h
**
**  DESCRIPTION:
**
**  Remove the first element in the lookaside list and return a pointer
**  to it.  If the lookaside list is empty, an element of the size
**  indicated in the lookaside list descriptor will be allocated from
**  the heap.
**
**  INPUTS:             none
**
**      list_desc       The lookaside list descriptor.
**
**      block           true if the alloc should block for heap to become free
**                      false if it is not to block
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
**      return          Pointer to the allocated list element.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE pointer_t rpc__list_element_alloc 
(
    rpc_list_desc_p_t       list_desc,
    boolean32               block
)
{
    volatile pointer_t  element;
    unsigned32          wait_cnt;
    struct timespec     delta;
    struct timespec     abstime;

    RPC_LOG_LIST_ELT_ALLOC_NTR;

    for (wait_cnt = 0;
         wait_cnt < rpc_g_lookaside_rcb.max_wait_times;
         wait_cnt++)
    {
        /*
         * Acquire the global resource control lock for all lookaside
         * lists if the caller doesn't have their own lock.
         */
        if (list_desc->use_global_mutex)
        {
            RPC_MUTEX_LOCK (rpc_g_lookaside_rcb.res_lock);
        }

        /*
         * Try allocating a structure off the lookaside list given.
         */
        if (list_desc->cur_size > 0)
        {
#define DEBUG 1
#ifdef DEBUG
            if (list_desc->list_head.next == NULL)
            {
		/*
		 * rpc_m_lookaside_corrupt
		 * "(%s) Lookaside list is corrupted"
		 */
		RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%s"),
		    rpc_svc_general,
		    svc_c_sev_fatal | svc_c_action_abort,
		    rpc_m_lookaside_corrupt,
		    "rpc__list_element_alloc" ));
            }
#endif
            list_desc->cur_size--;
            RPC_LIST_REMOVE_HEAD (list_desc->list_head, element, pointer_t);

            /*
             * Release the global resource control lock for all lookaside
             * lists if the caller doesn't have their own lock.
             */
            if (list_desc->use_global_mutex)
            {
                RPC_MUTEX_UNLOCK (rpc_g_lookaside_rcb.res_lock);
            }
            break;
        }
        else
        {
            /*
             * Release the global resource control lock if the
             * caller doesn't have their own lock for all lookaside lists
             * since the structure was available on the lookaside list.
             *
             * We do it now because allocating an element from heap is a relatively
             * time consuming operation.
             */
            if (list_desc->use_global_mutex)
            {
                RPC_MUTEX_UNLOCK (rpc_g_lookaside_rcb.res_lock);
            }

            /*
             * The lookaside list is empty. Try and allocate from
             * heap.
             */
            RPC_MEM_ALLOC (element,
                           pointer_t,
                           list_desc->element_size,
                           list_desc->element_type,
                           RPC_C_MEM_NOWAIT);

            if (element == NULL)
            {
                /*
                 * The heap allocate failed. If the caller indicated
                 * that we should not block return right now.
                 */
                if (block == false)
                {
                    break;
                }

                delta.tv_sec = rpc_g_lookaside_rcb.wait_time;
                delta.tv_nsec = 0;
                dcethread_get_expiration (&delta, &abstime);

                /*
                 * If we are using the global lookaside list lock
                 * then reaquire the global lookaside list lock and
                 * wait on the global lookaside list condition 
                 * variable otherwise use the caller's mutex and
                 * condition variable. 
                 */
                if (list_desc->use_global_mutex)
                {
                    RPC_MUTEX_LOCK (rpc_g_lookaside_rcb.res_lock);
                    RPC_COND_TIMED_WAIT (rpc_g_lookaside_rcb.wait_flg,
                                         rpc_g_lookaside_rcb.res_lock,
                                         &abstime); 
                    RPC_MUTEX_UNLOCK (rpc_g_lookaside_rcb.res_lock);
                }
                else
                {
                    RPC_COND_TIMED_WAIT (*list_desc->cond,
                                         *list_desc->mutex,
                                         &abstime); 
                }

                /*
                 * Try to allocate the structure again.
                 */
                continue;
            }
            else
            {
                /*
                 * The RPC_MEM_ALLOC succeeded. If an alloc routine
                 * was specified when the lookaside list was inited
                 * call it now.
                 */
                if (list_desc->alloc_rtn != NULL)
                {
                    /*
                     * Catch any exceptions which may occur in the
                     * list-specific alloc routine. Any exceptions
                     * will be caught and the memory will be freed.
                     */
                    DCETHREAD_TRY
                    {
                        (*list_desc->alloc_rtn) (element);
                    }
                    DCETHREAD_CATCH_ALL(THIS_CATCH)
                    {
                        RPC_MEM_FREE (element, list_desc->element_type);
                        element = NULL;
			/*
			 * rpc_m_call_failed_no_status
			 * "%s failed"
			 */
			RPC_DCE_SVC_PRINTF ((
			    DCE_SVC(RPC__SVC_HANDLE, "%s"),
			    rpc_svc_general,
			    svc_c_sev_fatal | svc_c_action_abort,
			    rpc_m_call_failed_no_status,
			    "rpc__list_element_alloc/(*list_desc->alloc_rtn)(element)" ));
                    }
                    DCETHREAD_ENDTRY
                }
                break;
            }
        }
    }
    if (element != NULL) {
        ((rpc_list_p_t)element)->next = NULL;
        ((rpc_list_p_t)element)->last = NULL;
    }
    RPC_LOG_LIST_ELT_ALLOC_XIT;
    return (element);
}


/*
**++
**
**  ROUTINE NAME:       rpc__list_desc_free
**
**  SCOPE:              PRIVATE - declared in rpclist.h
**
**  DESCRIPTION:
**
**  Returns an element to the lookaside list.  If this would result
**  in the current size of the lookaside list becoming greater than
**  the maximum size, the element will be returned to the heap instead.
**
**  INPUTS:
**
**      list_desc       The list descriptor.
**
**      list_element    Pointer to the element to be freed.
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

PRIVATE void rpc__list_element_free 
(
    rpc_list_desc_p_t       list_desc,
    pointer_t               list_element
)
{
    RPC_LOG_LIST_ELT_FREE_NTR;

    assert(list_desc != NULL);
    assert(list_element != NULL);

    /*
     * Acquire the global resource control lock for all lookaside
     * lists if the caller doesn't have their own lock.
     */
    if (list_desc->use_global_mutex)
    {
        RPC_MUTEX_LOCK (rpc_g_lookaside_rcb.res_lock);
    }
    
    if (list_desc->cur_size < list_desc->max_size)
    {
        list_desc->cur_size++;

        RPC_LIST_ADD_TAIL (list_desc->list_head, list_element, pointer_t);

        /*
         * Now check whether any other thread is waiting for a lookaside list
         * structure. 
         */
        if (rpc_g_lookaside_rcb.waiter_cnt > 0)
        {
            /*
             * There are waiters. Signal the global lookaside list
             * condition variable if the caller doesn't have one.
             * Otherwise signal the caller provided condition
             * variable.
             */
            if (list_desc->use_global_mutex)
            {
                RPC_COND_SIGNAL (rpc_g_lookaside_rcb.wait_flg, 
                                 rpc_g_lookaside_rcb.res_lock);
            }
            else
            {
                RPC_COND_SIGNAL (*list_desc->cond,
                                 *list_desc->mutex);
            }
        }

        /*
         * Release the global resource control lock for all lookaside
         * lists if the caller doesn't have their own lock 
         * since the structure has now been added to the list.
         */
        if (list_desc->use_global_mutex)
        {
            RPC_MUTEX_UNLOCK (rpc_g_lookaside_rcb.res_lock);
        }
    }
    else
    {
        /*
         * If a free routine was specified when this lookaside list
         * was inited call it now.
         */
        if (list_desc->free_rtn != NULL)
        {
            (list_desc->free_rtn) (list_element);
        }

        /*
         * Release the global resource control lock for all
         * lookaside lists if the caller doesn't have their own lock.
         * 
         * We do it now because freeing an element to the heap is a relatively
         * time consuming operation.
         */
        if (list_desc->use_global_mutex)
        {
            RPC_MUTEX_UNLOCK (rpc_g_lookaside_rcb.res_lock);
        }
       
	memset (list_element, 0, list_desc->element_size); 
        RPC_MEM_FREE (list_element, list_desc->element_type);
    }

    RPC_LOG_LIST_ELT_FREE_XIT;
}

/*
**++
**
**  ROUTINE NAME:       rpc__list_fork_handler
**
**  SCOPE:              PRIVATE - declared in rpclist.h
**
**  DESCRIPTION:
**
**  Perform fork-related processing, depending on what stage of the 
**  fork we are currently in.
**
**  INPUTS:
**
**      stage           The current stage of the fork process.
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

PRIVATE void rpc__list_fork_handler
(
    rpc_fork_stage_id_t     stage
)
{
    switch ((int)stage)
    {
        case RPC_C_PREFORK:
                break;
        case RPC_C_POSTFORK_PARENT:
                break;
        case RPC_C_POSTFORK_CHILD:  
                /*
                 * Reset the lookaside waiter's count.
                 */
                rpc_g_lookaside_rcb.waiter_cnt = 0;
                break;
    }  
}
