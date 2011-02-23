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
**      mgmt.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definition of the Management Component of the RPC Runtime Sytem.
**  This module contains both Local management functions (those which
**  only execute locally) and Local/Remote management functions (those
**  which can execute either locally or on a remote server). The class
**  into which each function falls is identified in its description.
**  For each Local/Remote function there is a corresponding routine that
**  provides the remote (manager) implementation of its functionality.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime  */
#include <com.h>        /* Common communications services           */
#include <comp.h>       /* Private communications services          */
#include <mgmtp.h>      /* Private management services              */
#include <dce/mgmt.h>   /* Remote RPC Management Interface          */


/*
 * Authorization function to use to check remote access. 
 */

INTERNAL rpc_mgmt_authorization_fn_t authorization_fn = NULL;

/*
 * Default server comm timeout value.
 */
INTERNAL unsigned32 server_com_timeout;

/*
 * Size of buffer used when asking for remote server's principal name
 */

#define MAX_SERVER_PRINC_NAME_LEN 500


/*
 * Forward definitions of network manager entry points (implementation
 * of mgmt.idl).
 */

INTERNAL void inq_if_ids (    
        rpc_binding_handle_t     /*binding_h*/,
        rpc_if_id_vector_p_t    * /*if_id_vector*/,
        unsigned32              * /*status*/
    );

INTERNAL void inq_stats (            
        rpc_binding_handle_t     /*binding_h*/,
        unsigned32              * /*count*/,
        unsigned32              statistics[],
        unsigned32              * /*status*/
    );

INTERNAL boolean32 is_server_listening (            
        rpc_binding_handle_t     /*binding_h*/,
        unsigned32              * /*status*/
    );


INTERNAL void inq_princ_name (            
        rpc_binding_handle_t     /*binding_h*/,
        unsigned32               /*authn_proto*/,
        unsigned32               /*princ_name_size*/,
        idl_char                princ_name[],
        unsigned32              * /*status*/
    );




INTERNAL idl_void_p_t my_allocate (
        idl_size_t  /*size*/
    );

INTERNAL void my_free (
        idl_void_p_t  /*ptr*/
    );

INTERNAL void remote_binding_validate (
        rpc_binding_handle_t     /*binding_h*/,
        unsigned32              * /*status*/
    );



/*
**++
**
**  ROUTINE NAME:       rpc__mgmt_init
**
**  SCOPE:              PRIVATE - declared in cominit.c
**
**  DESCRIPTION:
**      
**  Initialize the management component. Register the remote management
**  interface for the RPC runtime.
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
**  FUNCTION VALUE:
**
**      returns rpc_s_ok if everything went well, otherwise returns
**      an error status
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__mgmt_init(void)

{
    unsigned32              status;
    /*
     * Manager EPV for implementation of mgmt.idl.
     */
    static mgmt_v1_0_epv_t mgmt_v1_0_mgr_epv =
    {
        inq_if_ids,
        inq_stats,
        is_server_listening,
        rpc__mgmt_stop_server_lsn_mgr,
        inq_princ_name
    };

    

    /*
     * register the remote management interface with the runtime
     * as an internal interface
     */
    rpc__server_register_if_int
        ((rpc_if_handle_t) mgmt_v1_0_s_ifspec, NULL, 
        (rpc_mgr_epv_t) &mgmt_v1_0_mgr_epv, 0,
        rpc_c_listen_max_calls_default, -1, NULL,
        true, &status);
                                   
    authorization_fn = NULL;

    server_com_timeout = rpc_c_binding_default_timeout;

    return (status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_inq_com_timeout
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local management function that inquires what the timeout
**  value is in a binding.
**
**  INPUTS:
**
**      binding_h       The binding handle which points to the binding
**                      rep data structure to be read.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      timeout         The relative timeout value used when making a
**                      connection to the location specified in the
**                      binding rep.
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_inq_com_timeout (binding_handle, timeout, status)
        
rpc_binding_handle_t    binding_handle;
unsigned32              *timeout;
unsigned32              *status;

{
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_handle;
    

    RPC_VERIFY_INIT ();
    
    RPC_BINDING_VALIDATE_CLIENT(binding_rep, status);
    if (*status != rpc_s_ok)
        return;

    *timeout = binding_rep->timeout;
    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_inq_server_com_timeout
**
**  SCOPE:              PUBLIC - declared in rpcpvt.idl
**
**  DESCRIPTION:
**      
**  This is a Local management function that returns the default server-side
**  com timeout setting.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**                          RPC Protocol ID in binding handle was invalid.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     unsigned32, the current com timeout setting
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC unsigned32 rpc_mgmt_inq_server_com_timeout (void)

{
    RPC_VERIFY_INIT ();
    
    return (server_com_timeout);
}


/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_inq_if_ids
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local/Remote management function that obtains a vector of
**  interface identifications listing the interfaces registered with the
**  RPC runtime. If a server has not registered any interfaces this routine
**  will return an rpc_s_no_interfaces status code and a NULL if_id_vector.
**  The application is responsible for calling rpc_if_id_vector_free to
**  release the memory used by the vector.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      if_id_vector    A vector of the if id's registered for this server
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc_mgmt_inq_if_ids 
(
    rpc_binding_handle_t    binding_h,
    rpc_if_id_vector_p_t    *if_id_vector,
    unsigned32              *status
)
{
    idl_void_p_t            (*old_allocate) (idl_size_t);
    idl_void_p_t            (*tmp_allocate) (idl_size_t);
    void                    (*old_free) (idl_void_p_t);
    void                    (*tmp_free) (idl_void_p_t);

    RPC_VERIFY_INIT ();
    
    /*
     * if this is a local request, just do it locally
     */
    if (binding_h == NULL)
    {
        rpc__if_mgmt_inq_if_ids (if_id_vector, status);
    }
    else
    {
        remote_binding_validate(binding_h, status);
        if (*status != rpc_s_ok)
            return;

        /*
         * force the stubs to use malloc and free (because the caller is going
         * to have to free the if id vector using rpc_if_id_vector_free() later
         */
        rpc_ss_swap_client_alloc_free
            (my_allocate, my_free, &old_allocate, &old_free);

        /*
         * call the corresponding remote routine to get an if id vector
         */
        (*mgmt_v1_0_c_epv.rpc__mgmt_inq_if_ids)
            (binding_h, if_id_vector, status);

        if (*status == rpc_s_call_cancelled)
            dcethread_interrupt_throw(dcethread_self());
        
        /*
         * restore the memory allocation scheme in effect before we got here
         */
        rpc_ss_swap_client_alloc_free
            (old_allocate, old_free, &tmp_allocate, &tmp_free);
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_inq_stats
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local/Remote management function that obtains statistics
**  about the specified server from the RPC runtime. Each element in the
**  returned argument contains an integer value which can be indexed using
**  the defined statistics constants.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      stats           An vector of statistics values for this server.
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_inq_stats 
(
    rpc_binding_handle_t    binding_h,
    rpc_stats_vector_p_t    *statistics,
    unsigned32              *status
)
{
    unsigned32              i;

    RPC_VERIFY_INIT ();

    /*
     * Allocate a stats vector large enough to hold all the
     * statistics we know about and set the vector count to match
     * the size allocated.
     */
    RPC_MEM_ALLOC (*statistics, rpc_stats_vector_p_t, 
                   sizeof (rpc_stats_vector_t)
                   + (sizeof ((*statistics)->stats) *
                      (rpc_c_stats_array_max_size - 1)),
                   RPC_C_MEM_STATS_VECTOR,
                   RPC_C_MEM_WAITOK);
    (*statistics)->count = rpc_c_stats_array_max_size;
    
    /*  
     * If this is a local request, just do it locally.
     */
    if (binding_h == NULL)
    {
        /*
         * Clear the output array and query each protocol service
         * for its information. Sum the results in the output array.
         */
        memset (&(*statistics)->stats[0], 0, ((*statistics)->count * sizeof (unsigned32)));
        for (i = 0; i < RPC_C_PROTOCOL_ID_MAX; i++)
        {
            if (RPC_PROTOCOL_INQ_SUPPORTED (i))
            {
                (*statistics)->stats[rpc_c_stats_calls_in] +=
                (*rpc_g_protocol_id[i].mgmt_epv->mgmt_inq_calls_rcvd)();

                (*statistics)->stats[rpc_c_stats_calls_out] +=
                (*rpc_g_protocol_id[i].mgmt_epv->mgmt_inq_calls_sent)(); 

                (*statistics)->stats[rpc_c_stats_pkts_in] +=
                (*rpc_g_protocol_id[i].mgmt_epv->mgmt_inq_pkts_rcvd)();

                (*statistics)->stats[rpc_c_stats_pkts_out] +=
                (*rpc_g_protocol_id[i].mgmt_epv->mgmt_inq_pkts_sent)(); 
            }
        }
        *status = rpc_s_ok;
    }
    else
    {
    
        remote_binding_validate(binding_h, status);
        if (*status != rpc_s_ok)
            return;

        /*
         * Call the corresponding remote routine to get remote stats.
         */
        (*mgmt_v1_0_c_epv.rpc__mgmt_inq_stats) (binding_h, 
                                                &(*statistics)->count, 
                                                &(*statistics)->stats[0],
                                                status);

        if (*status == rpc_s_call_cancelled)
            dcethread_interrupt_throw(dcethread_self());
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_stats_vector_free
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine will free the statistics vector memory allocated by
**  and returned by rpc_mgmt_inq_stats.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:    
**
**      stats           A pointer to a pointer to the stats vector.
**                      The contents will be NULL on output.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_stats_vector_free 
(
    rpc_stats_vector_p_t    *statistics,
    unsigned32              *status
)
{
    RPC_MEM_FREE (*statistics, RPC_C_MEM_STATS_VECTOR);
    *statistics = NULL;
    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_is_server_listening
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local/Remote management function that determines if the
**  specified server is listening for remote procedure calls.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      true - if server is listening
**      false - if server is not listening :-)
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC boolean32 rpc_mgmt_is_server_listening 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *status
)
{


    RPC_VERIFY_INIT ();
    
    /*
     * if this is a local request, just do it locally
     */
    if (binding_h == NULL)
    {
        *status = rpc_s_ok;
        return (rpc__server_is_listening());
    }
    else
    {
        remote_binding_validate(binding_h, status);
        if (*status != rpc_s_ok)
            return (false);

        /*
         * call the corresponding remote routine
         */
        (*mgmt_v1_0_c_epv.rpc__mgmt_is_server_listening) (binding_h, status);

        if (*status == rpc_s_call_cancelled)
            dcethread_interrupt_throw(dcethread_self());

        return (*status == rpc_s_ok ? true : false);
     }
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_set_cancel_timeout
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local management function that sets the amount of time the
**  RPC runtime is to wait for a server to acknowledge a cancel before
**  orphaning the call. The application should specify to either wait
**  forever or to wait the length of the time specified in seconds. If the
**  value of seconds is 0 the remote procedure call is orphaned as soon as
**  a cancel is received by the server and control returns immediately to 
**  the client application. The default is to wait forever for the call to
**  complete.
**
**  The value for the cancel timeout applies to all remote procedure calls
**  made in the current thread. A multi-threaded client that wishes to change
**  the default timeout value must call this routine in each thread of
**  execution.
**
**  INPUTS:
**
**      seconds         The number of seconds to wait for an acknowledgement.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_no_memory
**          rpc_s_coding_error
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_set_cancel_timeout 
(
    signed32                seconds,
    unsigned32              *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();
    
    /*
     * set the cancel timeout value in the per-thread context block
     * for this thread
     */
    RPC_SET_CANCEL_TIMEOUT (seconds, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_set_call_timeout
**
**  SCOPE:              PUBLIC - (SHOULD BE) declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local management function that sets the amount of time the
**  RPC runtime is to wait for a server to complete a call.  A timeout of
**  0 means no max call execution time is imposed (this is the default).
**
**  The value for the call timeout applies to all remote procedure calls
**  made using the specified binding handle.
**
**  This function currently is NOT a documented API operation (i.e. it
**  is not in rpc.idl).  At least initially, only the ncadg_ protocols
**  support the use of this timeout. This function is necessary for
**  "kernel" RPC support since kernels don't support a cancel mechanism.
**  User space applications can build an equivalent mechanism using
**  cancels.  This code is not conditionally compiled for the kernel
**  so that we can test it in user space.
**
**  INPUTS:
**
**      binding         The binding handle to use.
**      seconds         The number of seconds to wait for call completion.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_no_memory
**          rpc_s_coding_error
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_set_call_timeout (
        rpc_binding_handle_t     /*binding_h*/,
        unsigned32               /*seconds*/,
        unsigned32              * /*status*/
    );

PUBLIC void rpc_mgmt_set_call_timeout 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              seconds,
    unsigned32              *status
)
{
    rpc_binding_rep_p_t binding_rep = (rpc_binding_rep_p_t) binding_h; 

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_BINDING_VALIDATE_CLIENT(binding_rep, status);
    if (*status != rpc_s_ok)
        return;

    binding_rep->call_timeout_time = RPC_CLOCK_SEC(seconds);
    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_set_com_timeout
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local management function that sets the RPC timeout for a
**  binding. The timeout value is a metric indicating the relative amount
**  of time retries to contact the server should be made. The value 10
**  indicates an unbounded wait. A zero value indicates no wait. Values 1-5
**  favor fast reponse time over correctness in determining whether the server
**  is alive. Values 6-9 favor correctness over response time. The RPC
**  Protocol Service identified by the RPC Protocol ID in the Binding Rep
**  will be notified that the Binding Rep has changed.
**
**  INPUTS:
**
**      binding_h       The binding handle which points to the binding
**                      rep data structure to be modified.
**
**      timeout         The relative timeout value to be used when making
**                      a connection to the location in the binding rep.
**
**                      0   -   rpc_c_binding_min_timeout
**                      5   -   rpc_c_binding_default_timeout
**                      9   -   rpc-c_binding_max_timeout
**                      10  -   rpc_c_binding_infinite_timeout
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**          rpc_s_invalid_timeout
**                          Timeout value is not in the range -1 to 10
**          rpc_s_coding_error
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_set_com_timeout 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              timeout,
    unsigned32              *status
)
{
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;


    CODING_ERROR (status);
    RPC_VERIFY_INIT ();
    
    RPC_BINDING_VALIDATE_CLIENT(binding_rep, status);
    if (*status != rpc_s_ok)
        return;
    
    /*
     * see if the timeout value is valid
     */
    if (/*timeout < rpc_c_binding_min_timeout ||*/
        timeout > rpc_c_binding_max_timeout)
    {
        if (timeout != rpc_c_binding_infinite_timeout)
        {
            *status = rpc_s_invalid_timeout;
            return;
        }
    }
         
    /*
     * copy the new timeout value into the binding rep
     */
    binding_rep->timeout = timeout;

    /*
     * notify the protocol service that the binding has changed
     */
#ifdef FOO

Note: there should be a dispatching routine in com...

    (*rpc_g_protocol_id[binding_rep->protocol_id].binding_epv
        ->binding_changed) (binding_rep, status);

#else

    *status = rpc_s_ok;

#endif
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_set_server_com_timeout
**
**  SCOPE:              PUBLIC - declared in rpcpvt.idl
**
**  DESCRIPTION:
**      
**  This is a Local management function that sets a default RPC timeout for
**  all calls handled by a server.  The timeout value is a metric indicating 
**  the relative amount of time retries to contact the client should be made. 
**  The value 10 indicates an unbounded wait. A zero value indicates no wait. 
**  Values 1-5 favor fast reponse time over correctness in determining whether 
**  the client is alive. Values 6-9 favor correctness over response time. 
**
**  INPUTS:
**
**      timeout         The relative timeout value to be used by all calls
**                      run on this server.
**
**                      0   -   rpc_c_binding_min_timeout
**                      5   -   rpc_c_binding_default_timeout
**                      9   -   rpc-c_binding_max_timeout
**                      10  -   rpc_c_binding_infinite_timeout
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_timeout
**                          Timeout value is not in the range -1 to 10
**          rpc_s_coding_error
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_set_server_com_timeout 
(
 unsigned32              timeout,
 unsigned32 * status
)
{

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();
    
    /*
     * see if the timeout value is valid
     */
    if (/*timeout < rpc_c_binding_min_timeout ||*/
        timeout > rpc_c_binding_max_timeout)
    {
        if (timeout != rpc_c_binding_infinite_timeout)
        {
            *status = rpc_s_invalid_timeout;
            return;
        }
    }

    server_com_timeout = timeout;

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_set_server_stack_size
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local management function that sets the value that the
**  RPC runtime is to use in specifying the the thread stack size when
**  creating call threads. This value will be applied to all threads
**  created for the server.
**
**  INPUTS:
**
**      thread_stack_size   The value to be used by the RPC runtime for
**                      specifying the stack size when creating threads.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_coding_error
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_set_server_stack_size 
(
    unsigned32              thread_stack_size,
    unsigned32              *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

/* !!! the 2nd one is due to a CMA pthreads bug and should go away */
#if !defined(_POSIX_THREAD_ATTR_STACKSIZE) && !defined(_POSIX_PTHREAD_ATTR_STACKSIZE)
    *status = rpc_s_not_supported;
#else
#  ifndef PTHREAD_EXC
    if (dcethread_attr_setstacksize_throw
        (&rpc_g_server_dcethread_attr, thread_stack_size) == -1)
    {
        *status = rpc_s_invalid_arg;
        return;
    }

    *status = rpc_s_ok;
#  else

    dcethread_attr_setstacksize
        (&rpc_g_server_dcethread_attr, thread_stack_size);

    *status = rpc_s_ok;
#  endif
#endif
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_stop_server_listening
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local/Remote management function that directs a server to
**  stop listening for remote procedure calls. On receipt of a stop listening
**  request the RPC runtime stops accepting new remote procedure calls for all
**  registered interfaces. Executing calls are allowed to complete, including
**  callbacks. After alls executing calls complete the rpc_server_listen()
**  routine returns to the caller.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_stop_server_listening 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *status
)
{

    RPC_VERIFY_INIT ();
    
    /*
     * if this is a local request, just do it locally
     */
    if (binding_h == NULL)
    {
        rpc__server_stop_listening (status);
    }
    else
    {

        remote_binding_validate(binding_h, status);
        if (*status != rpc_s_ok)
            return;

        /*
         * call the corresponding remote routine
         */
        (*mgmt_v1_0_c_epv.rpc__mgmt_stop_server_listening) (binding_h, status);

        if (*status == rpc_s_call_cancelled)
            dcethread_interrupt_throw(dcethread_self());
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_inq_server_princ_name
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This is a Local/Remote management function that directs a server to
**  
**  
**  
**  
**  
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_inq_server_princ_name 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              authn_protocol,
    unsigned_char_p_t       *server_princ_name,
    unsigned32              *status
)
{
    unsigned32          dce_rpc_authn_protocol;


    RPC_VERIFY_INIT ();

    RPC_AUTHN_CHECK_SUPPORTED (authn_protocol, status);

    dce_rpc_authn_protocol = 
        rpc_g_authn_protocol_id[authn_protocol].dce_rpc_authn_protocol_id;

    RPC_MEM_ALLOC (
        *server_princ_name,
        unsigned_char_p_t,
        MAX_SERVER_PRINC_NAME_LEN,
        RPC_C_MEM_STRING,
        RPC_C_MEM_WAITOK);
    
    /*
     * if this is a local request, just do it locally
     */
    if (binding_h == NULL)
    {
        rpc__auth_inq_my_princ_name 
            (dce_rpc_authn_protocol, MAX_SERVER_PRINC_NAME_LEN, 
             *server_princ_name, status);
    }
    else
    {
        remote_binding_validate(binding_h, status);
        if (*status != rpc_s_ok)
        {
            RPC_MEM_FREE (*server_princ_name, RPC_C_MEM_STRING);
            return;
        }

        /*
         * call the corresponding remote routine
         */
        (*mgmt_v1_0_c_epv.rpc__mgmt_inq_princ_name)
            (binding_h, 
             dce_rpc_authn_protocol,
             MAX_SERVER_PRINC_NAME_LEN, 
             *server_princ_name,
             status);

        if (*status != rpc_s_ok)
        {
            RPC_MEM_FREE (*server_princ_name, RPC_C_MEM_STRING);
            if (*status == rpc_s_call_cancelled)
                dcethread_interrupt_throw(dcethread_self());
            return;
        }
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_set_authorization_fn
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  A server application calls the rpc_mgmt_set_authorization_fn routine
**  to specify an authorization function to control remote access to
**  the server's remote management routines.
**
**  INPUTS:
**
**      authorization_fn
**                      Authorization function to be used
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_mgmt_set_authorization_fn 
(
    rpc_mgmt_authorization_fn_t authorization_fn_arg,
    unsigned32              *status
)
{
    RPC_VERIFY_INIT ();
    authorization_fn = authorization_fn_arg;
    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       inq_if_ids
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is the manager routine that provides remote access to the
**  rpc_mgmt_inq_if_ids function.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      if_id_vector    A vector of the if id's registered for this server
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_no_memory
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void inq_if_ids 
(
    rpc_binding_handle_t    binding_h,
    rpc_if_id_vector_p_t    *if_id_vector,
    unsigned32              *status
)
{
    rpc_if_id_vector_p_t    local_if_id_vector;
    unsigned32              index;
    unsigned32              temp_status;
    

    if (! rpc__mgmt_authorization_check (binding_h, rpc_c_mgmt_inq_if_ids, 
                               true, status))
    {
        *if_id_vector = NULL;
        return;
    }

    /*
     * call the corresponding local routine to get a local if id vector
     */
    rpc_mgmt_inq_if_ids (NULL, &local_if_id_vector, status);
    
    if (*status != rpc_s_ok)
    {
        *if_id_vector = NULL;
        return;
    }

    /*
     * allocate memory to hold the output argument so that it can be
     * freed by the stubs when we're done
     */
    *if_id_vector = (rpc_if_id_vector_p_t)
        rpc_ss_allocate (((sizeof local_if_id_vector->count) +
            (local_if_id_vector->count * sizeof (rpc_if_id_p_t))));

    if (*if_id_vector == NULL)
    {
        *status = rpc_s_no_memory;
        return;
    }

    /*
     * set the count field in the output vector
     */
    (*if_id_vector)->count = local_if_id_vector->count;
    
    /*
     * walk the local vector and for each element create a copy in the
     * output vector
     */
    for (index = 0; index < local_if_id_vector->count; index++)
    {
        (*if_id_vector)->if_id[index] = (rpc_if_id_p_t)
            rpc_ss_allocate (sizeof (rpc_if_id_t));
            
        if ((*if_id_vector)->if_id[index] == NULL)
        {
            /*
             * if we can't create a copy of any element, free the local
             * vector, free all existing elements in the output vector,
             * and free the output vector itself
             */
            rpc_if_id_vector_free (&local_if_id_vector, &temp_status);

				/* FIXME: mdn 24 Oct 1999: change index >=0 to index > 0 */
            while (index > 0)
            {
                rpc_ss_free ((char *) (*if_id_vector)->if_id[--index]);
            }
            
            rpc_ss_free ((char *) *if_id_vector);
            
            *if_id_vector = NULL;
            *status = rpc_s_no_memory;
            return;
        }

        /*
         * Copy the the entry in the local vector to the output vector.
         */
        (*if_id_vector)->if_id[index]->uuid = 
            local_if_id_vector->if_id[index]->uuid;
        (*if_id_vector)->if_id[index]->vers_major = 
            local_if_id_vector->if_id[index]->vers_major;
        (*if_id_vector)->if_id[index]->vers_minor = 
            local_if_id_vector->if_id[index]->vers_minor;
    }

    /*
     * free the local vector
     */
    rpc_if_id_vector_free (&local_if_id_vector, &temp_status);

    /*
     * return the output vector
     */
    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc__mgmt_stop_server_lsn_mgr
**
**  SCOPE:              PRIVATE - declared in mgmtp.h
**
**  DESCRIPTION:
**      
**  This is the manager routine that provides remote access to the
**  rpc_mgmt_stop_server_listening function.  This routine is PRIVATE
**  instead of INTERNAL so it can be used by the RRPC i/f as well.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__mgmt_stop_server_lsn_mgr 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *status
)
{
    if (! rpc__mgmt_authorization_check (binding_h, rpc_c_mgmt_stop_server_listen, 
                               false, status))
    {
        return;
    }

    rpc_mgmt_stop_server_listening (NULL, status);
}

/*
**++
**
**  ROUTINE NAME:       inq_stats
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is the manager routine that provides remote access to the
**  rpc_mgmt_inq_stats function.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     
**
**      count           The maximum size of the array on input and
**                      the actual size of the array on output.
**
**  OUTPUTS:
**
**      stats           An array of statistics values for this server.
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void inq_stats 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *count,
    unsigned32              statistics[],
    unsigned32              *status
)
{
    rpc_stats_vector_p_t        stats_vector;
    unsigned32                  temp_status;
    unsigned32                  i;

    if (! rpc__mgmt_authorization_check (binding_h, rpc_c_mgmt_inq_stats, 
                               true, status))
    {
        *count = 0;
        return;
    }

    /*
     * Call the corresponding local routine to get the stats array
     */
    rpc_mgmt_inq_stats (NULL, &stats_vector, status);
    if (*status != rpc_s_ok)
    {
        *count = 0;
        return;
    }
     
    *count = stats_vector->count;

    for (i = 0; i < *count; i++)
    {
        statistics[i] = stats_vector->stats[i];
    }
    rpc_mgmt_stats_vector_free (&stats_vector, &temp_status);
}

/*
**++
**
**  ROUTINE NAME:       is_server_listening
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is the manager routine that returns true if it is ever executed to
**  indicate that the server is listening for remote calls.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      true - if server is listening
**      false - if server is not listening :-)
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean32 is_server_listening 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *status
)
{
    if (! rpc__mgmt_authorization_check (binding_h, rpc_c_mgmt_is_server_listen, 
                               true, status))
    {
        return (false);     /* Sort of pointless, since we're answering anyway */
    }

    /*
     * Cogito ergo sum.
     */
    *status = rpc_s_ok;
    return (true);
}

/*
**++
**
**  ROUTINE NAME:       inq_princ_name
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  This is a manager routine that provides a remote caller with the
**  principal name (really one of the principal names) for a server.
**
**  INPUTS:
**
**      binding_h       The binding handle for this remote call.
**
**      authn_proto     The *wire* authentication protocol ID we're
**                      interested in
**
**      princ_name_size The max size of princ_name
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      princ_name      Server's principal name
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
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

INTERNAL void inq_princ_name 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              authn_proto,
    unsigned32              princ_name_size,
    idl_char                princ_name[],
    unsigned32              *status
)
{
    if (! rpc__mgmt_authorization_check (binding_h, rpc_c_mgmt_inq_princ_name,
                               true, status))
    {
        princ_name[0] = '\0';
        return;
    }

    rpc__auth_inq_my_princ_name 
        (authn_proto, princ_name_size, (unsigned_char_p_t) princ_name, status);

    if (*status != rpc_s_ok) 
    {
        princ_name[0] = '\0';
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__mgmt_authorization_check
**
**  SCOPE:              PRIVATE - declared in mgmtp.h
**
**  DESCRIPTION:
**      
**  Routine to check whether a management operation is allowed.  This
**  routine is PRIVATE instead of INTERNAL so it can be used by the RRPC
**  i/f as well.
**
**  INPUTS:
**
**      binding_h       RPC binding handle
**
**      op              Management operation in question
**
**      deflt           What to return in there's no authorization function set 
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean
**
**      return          Whether operation is allowed
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean32 rpc__mgmt_authorization_check 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              op,
    boolean32               deflt,
    unsigned32              *status
)
{
    if (authorization_fn == NULL)
    {
        *status = deflt ? rpc_s_ok : rpc_s_mgmt_op_disallowed;
        return (deflt);
    }
    else
    {
        if ((*authorization_fn) (binding_h, op, status))
        {
            *status = rpc_s_ok;     /* Be consistent */
            return (true);
        }
        else
        {
            *status = rpc_s_mgmt_op_disallowed;
            return (false);
        }
    }
}

/*
**++
**
**  ROUTINE NAME:       my_allocate
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Wrapper around RPC_MEM_ALLOC to use in call to
**  rpc_ss_swap_client_alloc_free.
**
**  INPUTS:
**
**      size            number of bytes to allocate
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     idl_void_p_t
**
**      return          pointer to allocated storage
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL idl_void_p_t my_allocate 
(
    idl_size_t           size
)
{
    idl_void_p_t             ptr;


    RPC_MEM_ALLOC (
        ptr,
        idl_void_p_t,
        size,
        RPC_C_MEM_STRING,
        RPC_C_MEM_WAITOK);

    return (ptr);
}

/*
**++
**
**  ROUTINE NAME:       my_free
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Wrapper around RPC_MEM_FREE to use in call to
**  rpc_ss_swap_client_alloc_free.
**
**  INPUTS:
**
**      ptr             storage to free
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

INTERNAL void my_free 
(
    idl_void_p_t            ptr
)
{
    RPC_MEM_FREE (ptr, RPC_C_MEM_STRING);
}


/*
**++
**
**  ROUTINE NAME:       remote_binding_validate
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Function to make sure a binding is sensible to use as a parameter to
**  one of the local/remote mgmt calls.  "Sensible" means (a) it's a
**  client binding, and (b) it has at least one of an object UUID or an
**  endpoint (so the call has a reasonable chance of making it to a real
**  server process).
**
**  INPUTS:
**
**      binding_h       RPC binding handle
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
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

INTERNAL void remote_binding_validate 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *status
)
{
    rpc_binding_rep_p_t binding_rep = (rpc_binding_rep_p_t) binding_h;


    RPC_BINDING_VALIDATE_CLIENT (binding_rep, status);
    if (*status != rpc_s_ok)
        return;

    if ((! binding_rep->addr_has_endpoint) && UUID_IS_NIL (&binding_rep->obj, status))
    {
        *status = rpc_s_binding_incomplete;
        return;
    }

    *status = rpc_s_ok;
}
