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
**      comnet.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Network Listener Service Interface.
**
**      This file provides (1) all of the PUBLIC Network Listener Service
**      API operations, and (2) the "portable" PRIVATE service operations.
**      
**
**
*/

#include <commonp.h>    /* Common internals for RPC Runtime system  */
#include <com.h>        /* Externals for Common Services component  */
#include <comprot.h>    /* Externals for common Protocol Services   */
#include <comnaf.h>     /* Externals for common NAF Services        */
#include <comp.h>       /* Internals for Common Services component  */
#include <comcthd.h>    /* Externals for Call Thread sub-component  */
#include <comnetp.h>    /* Internals for Network sub-component      */
#include <comfwd.h>     /* Externals for Common Services Fwd comp   */

/*
*****************************************************************************
*
* local data structures
*
*****************************************************************************
*/

/*
 * Miscellaneous Data Declarations
 */

/*
 * Data Declarations for rpc_network_inq_protseqs()
 *
 * Note: These are setup at initialization time
 */

INTERNAL rpc_protseq_vector_p_t psv = NULL; /* ptr to local protseq vector */
INTERNAL int    psv_size;	/* mem alloc size for protseq vector  */
INTERNAL int    psv_str_size;	/* mem alloc size for protseq strings */

#define PSV_SIZE        sizeof (rpc_protseq_vector_t) + \
                        RPC_C_PROTSEQ_MAX * (RPC_C_PROTSEQ_ID_MAX-1)

/*
 * The state of the listener thread that need to be shared across modules.
 */
INTERNAL rpc_listener_state_t       listener_state;

/*
 * Integer indicating how many threads are in "rpc_server_listen".
 */
INTERNAL int volatile               in_server_listen;

/*
 * Condition variable signaled to shutdown "rpc_server_listen" threads.
 */
INTERNAL rpc_cond_t                 shutdown_cond;
INTERNAL boolean volatile           do_shutdown;




/*
 * forward declarations of internal (static) functions
 */

INTERNAL void bv_alloc (
        rpc_binding_vector_p_t      /*old_vec*/,
        rpc_binding_vector_p_t      * /*new_vec*/,
        unsigned32                  * /*status*/
    );

/*
**++
**
**  ROUTINE NAME:       rpc_server_inq_bindings
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Return the bindings for this server to which RPCs may be made.
**  Note that object UUIDs are not part of these bindings.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      binding_vector  The vector of valid bindings to this server.
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_no_bindings
**                          rpc_s_coding_error
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

PUBLIC void rpc_server_inq_bindings 
(
    rpc_binding_vector_p_t  *binding_vec,
    unsigned32              *status
)
{
    unsigned int                     nd_index;       /* network info table index    */
    unsigned int                     bv_index;       /* binding vector index        */
    unsigned32                     av_index;       /* RPC Address vector index    */
    rpc_binding_vector_p_t  bvp, new_bvp;   /* local ptr to binding vector */
    rpc_naf_id_t            nafid;          /* network family id           */
    rpc_addr_vector_p_t     addr_vec;       /* rpc addrs of network desc   */
    rpc_binding_rep_p_t     binding_rep;
    unsigned int                     i;
    unsigned32              xstatus;


    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    *binding_vec = NULL;        /* assume the worst */

    /*
     * Allocate up an initial binding vector.  Note that we might need
     * to allocate up a larger one later if the one we get now turns out
     * to not be big enough
     */
    bv_alloc ((rpc_binding_vector_p_t) NULL, &bvp, status);

    if (*status != rpc_s_ok)
    {
        return;
    }

    bv_index = 0;

    /*
     * For each socket we know about...
     */
    for (nd_index = 0; nd_index < listener_state.high_water; nd_index++)
    {
        rpc_listener_sock_p_t lsock = &listener_state.socks[nd_index];

        /*
         * Consider only sockets that are in use and for server usage.
         */
        if (lsock->busy && lsock->is_server)
        {
            nafid = RPC_PROTSEQ_INQ_NAF_ID (lsock->protseq_id);

            /*
             * Get all the RPC Addresses represented by this descriptor.
             */
            (*RPC_NAF_INQ_EPV(nafid)->naf_desc_inq_addr)
                (lsock->protseq_id, lsock->desc, &addr_vec, status);

            if (*status != rpc_s_ok)
            {
                break;
            }

            /*
             * For each RPC Address...
             */
            for (av_index = 0; av_index < addr_vec->len; av_index++)
            {
                /*
                 * If we've exceeded the size of the current vector,
                 * allocate up a new one.
                 */
                if (bv_index >= bvp->count)
                {
                    bv_alloc (bvp, &new_bvp, status);

                    if (*status != rpc_s_ok)
                    {
                        break;
                    }

                    bvp = new_bvp;
                }

                /*
                 * Allocate a binding with this RPC Address.
                 */
                binding_rep =
                    rpc__binding_alloc (false, &uuid_g_nil_uuid, 
                        lsock->protocol_id, addr_vec->addrs[av_index], status);

                if (*status != rpc_s_ok)
                {
                    break;
                }

                /*
                 * The rpc_addr reference has been handed off to the
                 * binding, make sure that it isn't freed.
                 */
                addr_vec->addrs[av_index] = NULL;

                binding_rep->addr_is_dynamic = lsock->is_dynamic;
                bvp->binding_h[bv_index] = (rpc_binding_handle_t) binding_rep;

                bv_index++;     /* bump for next binding vector entry */
            }

            /*
             * Free up the allocated addr vector (and any addrs that
             * haven't been given to a binding).
             */
            rpc__naf_addr_vector_free (&addr_vec, &xstatus);

            /*
             * If there was previously an error we're done.
             */
            if (*status != rpc_s_ok)
                break;
        }
    }

    /*
     * Return with status if there aren't any bindings.
     */
    if (bv_index == 0 && *status == rpc_s_ok)
    {
        *status = rpc_s_no_bindings;
    }

    /*
     * If everything went fine, return the bindings.
     * Otherwise free resources before returning (retain the original error).
     */
    if (*status == rpc_s_ok)
    {
        bvp->count = bv_index;
        *binding_vec = bvp;
    }
    else
    {
        for (i = 0; i < bv_index; i++)
        {
            rpc_binding_free
                ((rpc_binding_handle_t *) &bvp->binding_h[i], &xstatus);
        }

        RPC_MEM_FREE (bvp, RPC_C_MEM_BINDING_VEC);
        *binding_vec = NULL;
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_listen
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine begins listening to the network for RPC requests.
**
**  INPUTS:
**
**   max_calls          The maximum number of concurrent calls which this
**                      server will process.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_already_listening
**                          rpc_s_no_protseqs_registered
**                          rpc_s_coding_error
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

PUBLIC void rpc_server_listen 
(
    unsigned32              max_calls,
    unsigned32              *status
)
{
    int i;
    boolean found = false;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_MUTEX_LOCK (listener_state.mutex);

    if (in_server_listen++ == 0)
    {
        /*
         * See if there are any server sockets.  We must add them to the real
         * listener so it'll start select'ing on them.
         */
        for (i = 0; i < listener_state.high_water; i++)
        {
            rpc_listener_sock_p_t lsock = &listener_state.socks[i];

            if (lsock->busy && lsock->is_server)
            {
                found = true;
                if (! lsock->is_active)
                {
                    rpc__nlsn_activate_desc (&listener_state, i, status);
                }
            }
        }

        /*
         * If we scanned the whole table and found no server sockets, there's
         * no point being here.
         */
        if (! found)
        {
            *status = rpc_s_no_protseqs_registered;
            --in_server_listen;
            RPC_MUTEX_UNLOCK (listener_state.mutex);
            return;
        }

        /*
         * Clear the status of the listener state table.
         */
        listener_state.status = rpc_s_ok;

        /*
         * Fire up the cthreads.
         */
        rpc__cthread_start_all (max_calls, status);
        if (*status != rpc_s_ok)
        {
            --in_server_listen;
            RPC_MUTEX_UNLOCK (listener_state.mutex);
            return;
        }

        RPC_DBG_PRINTF (rpc_e_dbg_general, 2, ("(rpc_server_listen) cthreads started\n"));

        do_shutdown = false;
    }

    /*
     * Wait until someone tells us to stop listening.
     */
    DCETHREAD_TRY
    {
        while (!do_shutdown)
        {
            RPC_COND_WAIT (shutdown_cond, listener_state.mutex);
        }
    }
    DCETHREAD_FINALLY
    {
        RPC_DBG_GPRINTF (("(rpc_server_listen) Shutting down...\n"));

        if (--in_server_listen == 0)
        {
            /*
             * Make the real listener stop listening on our server sockets now.
             */

            for (i = 0; i < listener_state.high_water; i++)
            {
                rpc_listener_sock_p_t lsock = &listener_state.socks[i];

                if (lsock->busy && lsock->is_server && lsock->is_active)
                {
                    rpc__nlsn_deactivate_desc (&listener_state, i, status);
                }
            }

            /*
             * Set return status from the value in the listener state table.
             */
            *status = listener_state.status;

            /*
             * Stop all the call executors (gracefully).
             *
             * Unlock the listener mutex while awaiting cthread termination.
             * Failure to do this can result in deadlock (e.g. if a cthread/RPC
             * tries to execute a "stop listening" while we're blocked with
             * the listener's mutex held).
             */

            RPC_MUTEX_UNLOCK (listener_state.mutex);

            /*
             * Stop call threads after closing listener sockets to avoid hang
             * if a request is received immediately after listener thread has
             * stopped call threads (HP fix JAGad42160).
             */
            rpc__cthread_stop_all (status);

            RPC_DBG_PRINTF (rpc_e_dbg_general, 2, ("(rpc_server_listen) cthreads stopped\n"));
        }
        else
        {
            *status = rpc_s_ok;
        }
    }
    DCETHREAD_ENDTRY
}

/*
**++
**
**  ROUTINE NAME:       rpc__server_stop_listening
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Cause the thread that called "rpc_server_listen" to gracefully return
**  from that routine.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation.
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

PRIVATE void rpc__server_stop_listening 
(
    unsigned32              *status
)
{
    CODING_ERROR (status);

    RPC_MUTEX_LOCK (listener_state.mutex);

    if (! in_server_listen)
    {
        *status = rpc_s_not_listening;
        RPC_MUTEX_UNLOCK (listener_state.mutex);
        return;
    }

    do_shutdown = true;
    RPC_COND_BROADCAST(shutdown_cond, listener_state.mutex);

    RPC_MUTEX_UNLOCK (listener_state.mutex);

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc__server_is_listening
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Return true iff there's a thread in "rpc_server_listen".
**  
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
**  FUNCTION VALUE:     boolean
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean32 rpc__server_is_listening (void)
{
    /*
     * We could lock, but there doesn't seem much point--the state could change
     * as soon as we unlock.
     */
    return (in_server_listen);
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_use_all_protseqs
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine tells the Common Communication Service to listen for RPCs
**  on all supported (by both the Common Communication Service and the
**  operating system) RPC Protocol Sequences.
**
**  INPUTS:
**
**      max_calls       The maximum number of concurrent calls which this
**                      server will process.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
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

PUBLIC void rpc_server_use_all_protseqs 
(
    unsigned32                  max_calls,
    unsigned32                  *status
)
{
    unsigned int                         i;
    rpc_protseq_vector_p_t      psvp;
    unsigned32                  my_status;
    

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * Get a vector of valid protocol sequences supported by the
     * Common Communications Service.
     */
    rpc_network_inq_protseqs (&psvp, status);

    if (*status != rpc_s_ok)
    {
		 RPC_DBG_PRINTF(rpc_es_dbg_general, 1, (("inq_protseqs failed\n")));
        return;
    }

    /*
     * Register each of the protocol sequences.
     */
    for (i = 0; i < psvp->count; i++)
    {
        rpc_server_use_protseq (psvp->protseq[i], max_calls, status);

        if (*status != rpc_s_ok)
        {
            break;
        }
    }

    /*
     * Something's got to be done here to see if there were any
     * errors registering the valid protocol sequences.  If there
     * were any errors, we should de-register the ones registered,
     * free the vector and return an error.
     */
     
    /*
     * Now free the protocol sequence vector.
     */
    rpc_protseq_vector_free (&psvp, &my_status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_use_protseq
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine creates a descriptor for the desired Network Address
**  Family and adds it to the pool of descriptors being listened on.  It
**  uses a dynamically assigned name for the descriptor for the Network
**  Address Family Service.
**
**  INPUTS:
**
**      rpc_protseq     The RPC protocol sequence to be used.
**  
**      max_calls       The maximum number of concurrent calls which this
**                      server will process on this RPC protocol sequence.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
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

PUBLIC void rpc_server_use_protseq 
(
    unsigned_char_p_t       rpc_protseq,
    unsigned32              max_calls,
    unsigned32              *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    rpc_server_use_protseq_ep (rpc_protseq, max_calls, NULL, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_use_protseq_if
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine is the same as rpc_server_use_protseq() except the
**  descriptor name is the name contained in the interface specification for
**  the given Network Address Family Service.
**
**  INPUTS:
**
**      rpc_protseq     The RPC protocol sequence to be used.
**  
**      max_calls       The maximum number of concurrent calls which this
**                      server will process on this RPC protocol sequence.
**
**      ifspec_h        The interface specification containing the endpoint
**                      to be used for this RPC protocol sequence.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
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

PUBLIC void rpc_server_use_protseq_if (rpc_protseq, max_calls, ifspec_h, status)

unsigned_char_p_t       rpc_protseq;
unsigned32              max_calls;
rpc_if_handle_t         ifspec_h;
unsigned32              *status;

{
    unsigned_char_p_t       endpoint = NULL;
    unsigned32              temp_status;
    rpc_protseq_id_t        pseq_id;


    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    pseq_id = rpc__network_pseq_id_from_pseq (rpc_protseq, status);
    if (*status != rpc_s_ok)
    {
        return;
    }

    rpc__if_inq_endpoint ((rpc_if_rep_p_t) ifspec_h, pseq_id, &endpoint, status);

    if (*status != rpc_s_ok)
    {
        return;
    }

    rpc_server_use_protseq_ep (rpc_protseq, max_calls, endpoint, status);
    rpc_string_free (&endpoint, &temp_status);
}


/*
**++
**
**  ROUTINE NAME:       rpc_server_use_all_protseqs_if
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine tells the RPC runtime to listen for RPCs on all the
**  protocol sequences for which the specified interface has well-known
**  endpoints.
**
**  INPUTS:
**
**      max_calls       The maximum number of concurrent calls which this
**                      server will process.
**
**      ifspec_h        The interface specification containing the endpoints
**                      to be used for this RPC protocol sequence.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
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

PUBLIC void rpc_server_use_all_protseqs_if 
(
    unsigned32              max_calls,
    rpc_if_handle_t         ifspec_h,
    unsigned32              *status
)
{
    unsigned int                         i;
    rpc_protseq_vector_p_t      psvp;
    unsigned32                  my_status;
    unsigned_char_p_t           endpoint;
    rpc_protseq_id_t            pseq_id;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * Get a vector of valid protocol sequences supported by the
     * Common Communications Service.
     */
    rpc_network_inq_protseqs (&psvp, status);

    if (*status != rpc_s_ok)
    {
        return;
    }

    /*
     * For each valid protocol sequence, see if the ifspec has an endpoint
     * for it.  If it does, try to create a socket based on it.
     */
    for (i = 0; i < psvp->count; i++)
    {

        pseq_id = rpc__network_pseq_id_from_pseq (psvp->protseq[i], status);

        if (*status != rpc_s_ok)
        {
            break;
        }

        rpc__if_inq_endpoint ((rpc_if_rep_p_t) ifspec_h, pseq_id,
            &endpoint, status);

        if (*status == rpc_s_endpoint_not_found)
        {
            *status = rpc_s_ok;
            continue;
        }
        if (*status != rpc_s_ok)
        {
            break;
        }

        rpc_server_use_protseq_ep (psvp->protseq[i], max_calls, endpoint, status);

        rpc_string_free (&endpoint, &my_status);

        if (*status != rpc_s_ok)
        {
            break;
        }
    }

    /*
     * Something's got to be done here to see if there were any
     * errors registering the valid protocol sequences.  If there
     * were any errors, we should de-register the ones registered,
     * free the vector and return an error.
     */
     
    /*
     * Now free the protocol sequence vector.
     */
    rpc_protseq_vector_free (&psvp, &my_status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__server_register_fwd_map
**
**  SCOPE:              PRIVATE - declared in comfwd.h
**
**  DESCRIPTION:
**      
**  Register a forwarding map function with the runtime.  This registered
**  function will be called by the protocol services to determine an
**  appropriate forwarding endpoint for a received pkt that is not for
**  any of the server's registered interfaces.
**
**  INPUTS:
**
**      map_fn          The Forwarding Map function to be used.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
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

PRIVATE void rpc__server_register_fwd_map 
(
  rpc_fwd_map_fn_t    map_fn,
  unsigned32          *status
)
{
    CODING_ERROR (status);

    *status = rpc_s_ok;

    rpc_g_fwd_fn = map_fn;
}



/*
**++
**
**  ROUTINE NAME:       rpc_network_inq_protseqs
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Return all protocol sequences supported by both the Common
**  Communication Service and the operating system.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      protseq_vec     The vector of RPC protocol sequences supported by
**                      this RPC runtime system.
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_no_protseqs
**                          rpc_s_no_memory
**                          rpc_s_coding_error
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

PUBLIC void rpc_network_inq_protseqs (protseq_vec, status)

rpc_protseq_vector_p_t  *protseq_vec;
unsigned32              *status;

{
    unsigned32              psid;       /* loop index into protseq id table */
    unsigned_char_p_t       ps;         /* pointer to protseq string        */
    rpc_protseq_vector_p_t  pvp;        /* local pointer to protseq vector  */


    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * Return with status if there aren't any protocol sequences.
     */
    if (psv->count == 0)
    {
        *status = rpc_s_no_protseqs;
        return;
    }

    /*
     * Mem alloc the return vector plus the required string space.
     */
    RPC_MEM_ALLOC (
        pvp,
        rpc_protseq_vector_p_t,
        psv_size + psv_str_size,
        RPC_C_MEM_PROTSEQ_VECTOR,
        RPC_C_MEM_WAITOK);

    *protseq_vec = pvp;

    /*
     * Copy the local protseq vector to the users return vector
     * and setup a pointer to the start of the returned strings.
     */
    /* b_c_o_p_y ((char *) psv, (char *) pvp, psv_size); */
    memmove((char *)pvp, (char *)psv, psv_size) ;
    ps = (unsigned_char_p_t) (((char *)pvp) + psv_size);
 
    /*
     * Loop through the local protocol sequence id table:
     *   - copy each protseq string to the return vector string space
     *   - bump the string space pointer
     */
    for (psid = 0; psid < psv->count; psid++)
    {
        pvp->protseq[psid] = ps;
        strcpy ((char *) ps, (char *) psv->protseq[psid]);
        ps += strlen ((char *) ps) + 1;
    }

    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc_network_is_protseq_valid
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine determines whether the Common Communications Service
**  supports a given RPC Protocol Sequence.
**
**  INPUTS:
**
**      rpc_protseq     The RPC protocol sequence whose validity is to be
**                      determined.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      return  -   true if the protocol sequence is supported
**                  false if the protocol sequence is not supported
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC boolean32 rpc_network_is_protseq_valid (rpc_protseq, status)

unsigned_char_p_t       rpc_protseq;
unsigned32              *status;

{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * Find the correct entry in the RPC Protocol Sequence ID table using the
     * RPC Protocol Sequence string passed in as an argument.
     */
    (void) rpc__network_pseq_id_from_pseq (rpc_protseq, status);

    if (*status == rpc_s_ok)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc_protseq_vector_free
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine will free the RPC Protocol Sequence strings pointed to in
**  the vector and the vector itself.
**
**  Note: The service that allocates this vector (rpc_network_inq_protseqs())
**      mem alloc()'s the memory required for the vector in one large chunk.
**      We therefore don't have to play any games, we just free once
**      for the base vector pointer.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      protseq_vec     The vector of RPC protocol sequences to be freed.
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
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

PUBLIC void rpc_protseq_vector_free (protseq_vector, status)

rpc_protseq_vector_p_t  *protseq_vector;
unsigned32              *status;

{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();
    
    RPC_MEM_FREE (*protseq_vector, RPC_C_MEM_PROTSEQ_VECTOR);

    *protseq_vector = NULL;

    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc_network_monitor_liveness
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine tells the Common Communication Service to call the routine
**  provided if communications are lost to the process represented by the
**  client handle provided.
**
**  INPUTS:
**
**      binding_h       The binding on which to monitor liveness.
**
**      client_handle   The client for which liveness is to be monitored.
**
**      rundown_fn      The routine to be called if communications are lost.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_invalid_binding
**                          rpc_s_coding_error
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

PUBLIC void rpc_network_monitor_liveness 
(
    rpc_binding_handle_t    binding_h,
    rpc_client_handle_t     client_handle,
    rpc_network_rundown_fn_t rundown_fn,
    unsigned32              *status
)
{
    rpc_protocol_id_t       protid;
    rpc_prot_network_epv_p_t net_epv;
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_BINDING_VALIDATE(binding_rep, status);
    if (*status != rpc_s_ok)
        return;

    /*
     * Get the protocol id from the binding handle (binding_rep) 
     */

    protid = binding_rep->protocol_id;
    net_epv = RPC_PROTOCOL_INQ_NETWORK_EPV (protid);


    /*
     * Pass through to the network protocol routine.
     */
    (*net_epv->network_mon)
        (binding_rep, client_handle, rundown_fn, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_network_stop_monitoring
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine tells the Common Communication Service to cancel
**  rpc_network_monitor_liveness.
**
**  INPUTS:
**
**      binding_h       The binding on which to stop monitoring liveness.
**
**      client_handle   The client for which liveness monitoring is to be
**                      stopped.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_invalid_binding
**                          rpc_s_coding_error
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

PUBLIC void rpc_network_stop_monitoring (binding_h, client_h, status)

rpc_binding_handle_t        binding_h;
rpc_client_handle_t         client_h;
unsigned32                  *status;

{
    rpc_protocol_id_t       protid;
    rpc_prot_network_epv_p_t net_epv;
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_BINDING_VALIDATE(binding_rep, status);
    if (*status != rpc_s_ok)
        return;

    /*
     * Get the protocol id from the binding handle (binding_rep) 
     */
    protid = binding_rep->protocol_id;
    net_epv = RPC_PROTOCOL_INQ_NETWORK_EPV (protid);


    /*
     * Pass through to the network protocol routine.
     */
    (*net_epv->network_stop_mon)
        (binding_rep, client_h, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_network_maintain_liveness
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine tells the Common Communication Service to actively keep
**  communications alive with the process identified in the binding.
**
**  INPUTS:
**
**      binding_h       The binding on which to maintain liveness.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_invalid_binding
**                          rpc_s_coding_error
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

PUBLIC void rpc_network_maintain_liveness (binding_h, status)

rpc_binding_handle_t    binding_h;
unsigned32              *status;

{
    rpc_protocol_id_t       protid;
    rpc_prot_network_epv_p_t net_epv;
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_BINDING_VALIDATE(binding_rep, status);
    if (*status != rpc_s_ok)
        return;

    /*
     * Get the protocol id from the binding handle (binding_rep) 
     */
    protid = binding_rep->protocol_id;
    net_epv = RPC_PROTOCOL_INQ_NETWORK_EPV (protid);


    /*
     * Pass through to the network protocol routine.
     */
    (*net_epv->network_maint) (binding_rep, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_network_stop_maintaining
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine tells the Common Communication Service to cancel
**  rpc_network_maintain_liveness.
**
**  INPUTS:
**
**      binding_h       The binding on which to stop maintaining liveness.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_invalid_binding
**                          rpc_s_coding_error
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

PUBLIC void rpc_network_stop_maintaining 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *status
)
{
    rpc_protocol_id_t       protid;
    rpc_prot_network_epv_p_t net_epv;
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_BINDING_VALIDATE(binding_rep, status);
    if (*status != rpc_s_ok)
        return;

    /*
     * Get the protocol id from the binding handle (binding_rep) 
     */
    protid = binding_rep->protocol_id;
    net_epv = RPC_PROTOCOL_INQ_NETWORK_EPV (protid);


    /*
     * Pass through to the network protocol routine.
     */
    (*net_epv->network_stop_maint)
        (binding_rep, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc_network_close
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine tells the Common Communication Service to remove
**  any associations underlying the binding handle.
**
**  INPUTS:
**
**      binding_h       The binding on which to close the association
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_invalid_binding
**                          rpc_s_coding_error
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

PUBLIC void rpc_network_close 
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *status
)
{
    rpc_protocol_id_t       protid;
    rpc_prot_network_epv_p_t net_epv;
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_BINDING_VALIDATE(binding_rep, status);
    if (*status != rpc_s_ok)
        return;

    /*
     * Get the protocol id from the binding handle (binding_rep) 
     */
    protid = binding_rep->protocol_id;
    net_epv = RPC_PROTOCOL_INQ_NETWORK_EPV (protid);


    /*
     * Pass through to the network protocol routine.
     */
    (*net_epv->network_close)
        (binding_rep, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__network_add_desc
**
**  SCOPE:              PRIVATE - declared in comnet.h
**
**  DESCRIPTION:
**      
**  This routine adds a descriptor to the pool of descriptors being
**  listened on.
**
**  INPUTS:
**
**      desc            The descriptor to be added to the pool.
**
**      is_server       A flag indicating if this is a server descriptor.
**
**      rpc_protseq_id  The RPC protocol sequence by which this descriptor
**                      is used.
**
**      priv_info       Private info associated with this descriptor.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_max_descs_exceeded
**                          rpc_s_coding_error
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

PRIVATE void rpc__network_add_desc 
(
    rpc_socket_t            desc,
    boolean32               is_server,
    boolean32               is_dynamic,
    rpc_protseq_id_t        rpc_protseq_id,
    pointer_t               priv_info,
    unsigned32              *status
)
{
    int                     nd, old_hiwat;
    rpc_listener_sock_p_t   lsock;


    CODING_ERROR (status);

    RPC_MUTEX_LOCK (listener_state.mutex);

    /*
     * See if there are any free slots in the Network Info Table.
     */
    if (listener_state.num_desc >= RPC_C_SERVER_MAX_SOCKETS)
    {
        *status = rpc_s_max_descs_exceeded;
        RPC_MUTEX_UNLOCK (listener_state.mutex);
        return;
    }

    /*
     * Find a free slot in the Network Info Table.
     */
    for (nd = 0; nd < listener_state.high_water; nd++)
    {
        if (! listener_state.socks[nd].busy)
        {
            break;
        }
    }

    lsock = &listener_state.socks[nd];

    /*
     * Place the Network descriptor, the RPC Protocol Sequence Id and the
     * private info pointer in this slot, bump the count of the descriptors
     * in the table.
     */
    lsock->busy        = true;
    lsock->is_server   = is_server;
    lsock->is_dynamic  = is_dynamic;
    lsock->is_active   = false;
    lsock->desc        = desc;
    lsock->protseq_id  = rpc_protseq_id;
    lsock->priv_info   = priv_info;
    lsock->protocol_id = RPC_PROTSEQ_INQ_PROT_ID (rpc_protseq_id);
    lsock->network_epv = RPC_PROTOCOL_INQ_NETWORK_EPV (lsock->protocol_id);

    listener_state.num_desc++;
    old_hiwat = listener_state.high_water;
    if (nd == listener_state.high_water)
    {
        listener_state.high_water++;
    }
    
    /*
     * Activate the descriptor to the real listener only if
     * "rpc_server_listen" has been called or if this is a client socket.
     * (Don't want to activate server sockets until "rpc_server_listen"
     * is called since we don't want to get any server-related I/O until
     * we're really a server!)
     */
    if (in_server_listen || ! is_server)
    {
        rpc__nlsn_activate_desc (&listener_state, nd, status);
        if (*status != rpc_s_ok)
        {
            lsock->busy = false;
            listener_state.high_water = old_hiwat;
            listener_state.num_desc--;
        }
    }
    else
    {
        *status = rpc_s_ok;
    }

    RPC_MUTEX_UNLOCK (listener_state.mutex);
}

/*
**++
**
**  ROUTINE NAME:       rpc__network_remove_desc
**
**  SCOPE:              PRIVATE - declared in comnet.h
**
**  DESCRIPTION:
**      
**  This routine removes a descriptor from the pool of descriptors being
**  listened on. This routine should be called only by protocol
**  services which are attempting to register mutiple descriptors to the
**  pool as a result of a use_protseq call. In this case the descriptors
**  which were previously added should be closed immediately by the
**  protocol service and hence will not use up system resources with
**  buffered connect requests or datagrams.
**
**  INPUTS:
**
**      desc            The descriptor to be removed from the pool.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_max_descs_exceeded
**                          rpc_s_desc_not_registered
**                          rpc_s_coding_error
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

PRIVATE void rpc__network_remove_desc 
(
    rpc_socket_t            desc,
    unsigned32              *status
)
{
    int                     nd, found_nd, maxnd; 
    boolean                 found_server_socket = false;


    CODING_ERROR (status);

    RPC_MUTEX_LOCK (listener_state.mutex);

    /*
     * Find the slot in the Network Info Table which has the Network
     * descriptor given (and locate the new high water mark).
     */
    for (nd = 0, maxnd = -1, found_nd = -1; 
         nd < listener_state.high_water; 
         nd++)
    {
        if (listener_state.socks[nd].busy)
        {
            if (listener_state.socks[nd].desc == desc)
            {
                found_nd = nd;
            }   
            else if (listener_state.socks[nd].is_server)
                found_server_socket = true;

            maxnd = nd;
        }
    }

    if (found_nd == -1)
    {
        *status = rpc_s_desc_not_registered;
        RPC_MUTEX_UNLOCK (listener_state.mutex);
        return;
    }
     
    /*
     * If we just removed the last server socket, and there's a
     * thread sitting in rpc_server_listen(), wake it up. 
     */
    if (! found_server_socket && in_server_listen)
    {
        listener_state.status = rpc_s_no_protseqs_registered;
        do_shutdown = true;
        RPC_COND_BROADCAST (shutdown_cond, listener_state.mutex);
    }

    /*
     * Mark the slot free, decrement the number of active slots in the table,
     * update the high water mark (it may be the same as before).
     */
    listener_state.socks[found_nd].busy = false;
    listener_state.num_desc--;
    listener_state.high_water = maxnd + 1;

    /*
     * Do what's necessary to convey this deletion to the listener.
     */
    rpc__nlsn_deactivate_desc (&listener_state, found_nd, status);
    RPC_MUTEX_UNLOCK (listener_state.mutex);
}

/*
**++
**
**  ROUTINE NAME:       rpc__network_init
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Initialization for this module.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_no_memory
**                          rpc_s_coding_error
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

PRIVATE void rpc__network_init 
(
    unsigned32              *status
)
{
    int                     pseq_id;    /* protocol sequence id/index   */


    CODING_ERROR (status);

    /*
     * Initialize our mutex.  Initialize our conditional variable used
     * for shutdown indication.  Note that the mutex covers ALL the state
     * in this module, not just the values in "listener_state".
     */

    RPC_MUTEX_INIT (listener_state.mutex);
    RPC_COND_INIT (listener_state.cond, listener_state.mutex);

    RPC_COND_INIT (shutdown_cond, listener_state.mutex);
    do_shutdown = false;

    /*
     * Allocate a local protseq vector structure.
     */
    RPC_MEM_ALLOC(psv, rpc_protseq_vector_p_t, PSV_SIZE,
        RPC_C_MEM_PROTSEQ_VECTOR, RPC_C_MEM_WAITOK);

    psv->count = 0;                     /* zero out the count */
    psv_size = 0;                       /* zero out the vector malloc size */
    psv_str_size = 0;                   /* zero out the string malloc size */

    /*
     * Loop through the protocol sequence id table and ...
     *
     *   test each protocol sequence to see if it is supported and ...
     *     if so:
     *      - fetch the pointer to the protseq
     *      - bump the amount of string memory required
     *      - bump the number of supported protseq's
     */
    for (pseq_id = 0; pseq_id < RPC_C_PROTSEQ_ID_MAX; pseq_id++)
    {
        if (RPC_PROTSEQ_INQ_SUPPORTED (pseq_id))
        {
            psv->protseq[psv->count] = RPC_PROTSEQ_INQ_PROTSEQ (pseq_id);
            psv_str_size += strlen ((char *) psv->protseq[psv->count]) + 1;
            psv->count++;
        }
    }
    /*
     * Figure the total amount of memory required for the return vector.
     */
    psv_size += sizeof (rpc_protseq_vector_t)       /* sizeof basic struct */
        + (RPC_C_PROTSEQ_MAX * (psv->count - 1));   /* sizeof protseq ptrs */

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc__network_set_priv_info
**
**  SCOPE:              PRIVATE - declared in comnet.h
**
**  DESCRIPTION:
**      
**  This routine changes the private information stored with a descriptor
**  being listened on.
**
**  INPUTS:
**
**      desc            The descriptor whose private info is to be set.
**
**      priv_info       The private info to set.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_desc_not_registered
**                          rpc_s_coding_error
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

PRIVATE void rpc__network_set_priv_info 
(
    rpc_socket_t            desc,
    pointer_t               priv_info,
    unsigned32              *status
)
{
    int                     i;


    CODING_ERROR (status);

    /*
     * scan for the entry whose descriptor matches the requested
     * descriptor and set the corresponding entry's private info
     */

    for (i = 0; i < listener_state.high_water; i++)
    {
        if (listener_state.socks[i].busy && listener_state.socks[i].desc == desc)
        {
            listener_state.socks[i].priv_info = priv_info;
            *status = rpc_s_ok;
            return;
        }
    }
    *status = rpc_s_desc_not_registered;
}

/*
**++
**
**  ROUTINE NAME:       rpc__network_inq_priv_info
**
**  SCOPE:              PRIVATE - declared in comnet.h
**
**  DESCRIPTION:
**      
**  This routine returns the private information stored with the given
**  descriptor.
**
**  INPUTS:
**
**      desc            The descriptor whose private info is to be returned.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      priv_info       The private info stored with this descriptor.
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_desc_not_registered
**                          rpc_s_coding_error
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

PRIVATE void rpc__network_inq_priv_info 
(
    rpc_socket_t            desc,
    pointer_t               *priv_info,
    unsigned32              *status
)
{
    int                     i;


    CODING_ERROR (status);

    /*
     * scan for the entry whose descriptor matches the requested
     * descriptor and get the corresponding entry's private info
     */

    for (i = 0; i < listener_state.high_water; i++)
    {
        if (listener_state.socks[i].busy && listener_state.socks[i].desc == desc)
        {
            *priv_info = listener_state.socks[i].priv_info;
            *status = rpc_s_ok;
            return;
        }
    }

    *status = rpc_s_desc_not_registered;
}



/*
**++
**
**  ROUTINE NAME:       rpc__network_inq_prot_version
**
**  SCOPE:              PRIVATE - declared in comnet.h
**
**  DESCRIPTION:
**      
**  Return the version number of the RPC protocol sequence requested.
**
**  INPUTS:
**
**      rpc_protseq_id  The protocol sequence id whose architected protocol id
**                      and version number is to be returned.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      prot_id         The RPC protocol sequence protocol id.
**	version_major	The RPC protocol sequence major version number.
**	version_minor	The RPC protocol sequence minor version number.
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
**                          rpc_s_invalid_rpc_protseq
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

PRIVATE void rpc__network_inq_prot_version 
(
    rpc_protseq_id_t        rpc_protseq_id,
    unsigned8               *prot_id,
    unsigned32		*version_major,
    unsigned32		*version_minor,
    unsigned32              *status
)
{
    rpc_protocol_id_t           rpc_prot_id;
    rpc_prot_network_epv_p_t    net_epv;

    CODING_ERROR (status);

    /*
     * Check that protocol sequence is supported by this host
     */
    if (! RPC_PROTSEQ_INQ_SUPPORTED(rpc_protseq_id))
    {
        *status = rpc_s_protseq_not_supported;
        return;
    }

    rpc_prot_id = RPC_PROTSEQ_INQ_PROT_ID (rpc_protseq_id);
    net_epv = RPC_PROTOCOL_INQ_NETWORK_EPV (rpc_prot_id);

    (*net_epv->network_inq_prot_vers)
        (prot_id, version_major, version_minor, status);
    
}

/*
**++
**
**  ROUTINE NAME:       rpc__network_protseq_id_from_protseq
**
**  SCOPE:              PRIVATE - declared in comnet.h
**
**  DESCRIPTION:
**      
**  This routine searches the RPC Protocol Sequence ID table and returns
**  the Protocol Sequence ID for the given RPC Protocol Sequence string.
**
**  INPUTS:
**
**      rpc_protseq     The RPC protocol sequence whose id is to be returned.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_invalid_rpc_protseq
**                          rpc_s_protseq_not_supported
**                          rpc_s_coding_error
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_protocol_id_t
**
**      The RPC protocol sequence id.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_protocol_id_t rpc__network_pseq_id_from_pseq 
(
    unsigned_char_p_t       rpc_protseq,
    unsigned32              *status
)
{
    rpc_protocol_id_t       pseqid;


    CODING_ERROR (status);

    /*
     * special-case the protseqs "ip" and "dds" for backwards compatibility
     */
    if ((strcmp ((char *) rpc_protseq, "ip")) == 0)
    {
        pseqid = RPC_C_PROTSEQ_ID_NCADG_IP_UDP; 

        /*
         * Verify whether the protocol sequence ID is supported.
         */
        if (RPC_PROTSEQ_INQ_SUPPORTED (pseqid))
        {
            *status = rpc_s_ok;
            return (pseqid);
        }
        else
        {
            *status = rpc_s_protseq_not_supported;
            return (RPC_C_INVALID_PROTSEQ_ID);
        }
    }

    if ((strcmp ((char *) rpc_protseq, "dds")) == 0)
    {
        pseqid = RPC_C_PROTSEQ_ID_NCADG_DDS; 

        /*
         * Verify whether the protocol sequence ID is supported.
         */
        if (RPC_PROTSEQ_INQ_SUPPORTED (pseqid))
        {
            *status = rpc_s_ok;
            return (pseqid);
        }
        else
        {
            *status = rpc_s_protseq_not_supported;
            return (RPC_C_INVALID_PROTSEQ_ID);
        }
    }

    /*
     * The protseq is not a special case string. Check the vector of
     * supported protocol sequences.
     */
    for (pseqid = 0; pseqid < RPC_C_PROTSEQ_ID_MAX; pseqid++)
    {
        if ((strcmp ((char *) rpc_protseq,
            (char *) RPC_PROTSEQ_INQ_PROTSEQ (pseqid))) == 0)
        {
            /*
             * Verify whether the protocol sequence ID is supported.
             */
            if (RPC_PROTSEQ_INQ_SUPPORTED (pseqid))
            {
                *status = rpc_s_ok;
                return (pseqid);
            }
            else
            {
                *status = rpc_s_protseq_not_supported;
                return (RPC_C_INVALID_PROTSEQ_ID);
            }
        }
    }

    /*
     * If we got this far the protocol sequence given is not valid.
    *status = rpc_s_invalid_rpc_protseq;
     */
	 /* not supported or invalid; it's doesn't really matter, does it? */
	 *status = rpc_s_protseq_not_supported;
    return (RPC_C_INVALID_PROTSEQ_ID);
}

/*
**++
**
**  ROUTINE NAME:       rpc__network_pseq_from_pseq_id
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Return the protseq (string) rep for the protseq_id.
**
**  This is an internal routine that needs to be relatively streamlined
**  as it is used by the forwarding mechanism.  As such we assume that
**  the input args (created by some other runtime component) are valid.
**  Additionally, we return a pointer to the global protseq string.  If
**  future callers of this function want to do other manipulations of
**  the string, they can copy it to private storage.
**
**  INPUTS:
**
**      protseq_id      A *valid* protocol sequence id.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      protseq         Pointer to the protseq_id's string rep.
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
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

PRIVATE void rpc__network_pseq_from_pseq_id 
(
    rpc_protseq_id_t    protseq_id,
    unsigned_char_p_t   *protseq,
    unsigned32          *status
)
{
    CODING_ERROR (status);

    *protseq = RPC_PROTSEQ_INQ_PROTSEQ (protseq_id);

    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc__network_inq_local_addr
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Return a rpc_addr bound the local host.
**
**  INPUTS:
**
**      protseq_id      A *valid* protocol sequence id.
**      endpoint        A endpoint string (may be NULL)
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr        Pointer to "local host" rpc_addr.
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
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

PRIVATE void rpc__network_inq_local_addr 
(
    rpc_protseq_id_t    pseq_id,
    unsigned_char_p_t   endpoint,
    rpc_addr_p_t        *rpc_addr,
    unsigned32          *status
)
{
    rpc_socket_error_t      serr;
    rpc_socket_t            desc;
    rpc_addr_vector_p_t     addr_vector = NULL;
    boolean                 have_addr = false;
    boolean                 have_desc = false;
    boolean                 have_addr_vec = false;
    unsigned32              temp_status;

    CODING_ERROR (status);

    /*
     * Create a network descriptor for this RPC Protocol Sequence.
     */
    serr = rpc__socket_open(pseq_id, NULL, &desc);

    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_GPRINTF(
            ("rpc__network_inq_local_addr: cant create - serror %d\n",
            RPC_SOCKET_ETOI(serr)));
        *status = rpc_s_cant_create_sock;
        goto CLEANUP;
    }
    have_desc = true;

    /*
     * Allocate and initialized an rpc_addr which contains the necessary
     * info for the subsequent rpc__socket_bind() call.
     */
    rpc__naf_addr_alloc (
        pseq_id,
        RPC_PROTSEQ_INQ_NAF_ID (pseq_id),
        (unsigned_char_p_t) NULL,   /* dynamic endpoint */
        (unsigned_char_p_t) NULL,   /* netaddr */
        (unsigned_char_p_t) NULL,   /* netoptions */
        rpc_addr,
        status);

    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }
    have_addr = true;

    /*
     * Bind the socket (Network descriptor) to the (dynamic) RPC address.
     */
    serr = rpc__socket_bind(desc, *rpc_addr);

    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_GPRINTF(
            ("rpc__network_inq_local_addr: cant bind - serror %d\n",
            RPC_SOCKET_ETOI(serr)));
        *status = rpc_s_cant_bind_sock;
        goto CLEANUP;
    }

    /*
     * Determine a local address associated with the descriptor.
     */
    rpc__naf_desc_inq_addr (pseq_id, desc, &addr_vector, status);

    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }
    have_addr_vec = true;

    if (addr_vector->len == 0)
    {
        *status = rpc_s_no_addrs;
        goto CLEANUP;
    }

    /*
     * Update the rpc_addr with the (still dynamic) local addr.
     */
    rpc__naf_addr_overcopy (addr_vector->addrs[0], rpc_addr, status);

    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    /*
     * Now set the endpoint.
     */
    rpc__naf_addr_set_endpoint(endpoint, rpc_addr, status);

    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    *status = rpc_s_ok;

CLEANUP:

    if (have_desc)
        RPC_SOCKET_CLOSE(desc);

    if (have_addr_vec)
        rpc__naf_addr_vector_free (&addr_vector, &temp_status);

    if (*status != rpc_s_ok && have_addr)
        rpc__naf_addr_free(rpc_addr, &temp_status);

    return;
}


/*
**++
**
**  ROUTINE NAME:       rpc_server_use_protseq_ep
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Register an RPC protocol sequence for use by the runtime.
**
**  INPUTS:
**
**      rpc_protseq     The RPC protocol sequence to be registered.
**
**      max_calls       The maximum number of concurrent calls to be
**                      allowed for this protocol sequence.
**
**      endpoint        The endpoint to be used for this protocol sequence.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_cant_create_sock
**                          rpc_s_no_memory
**                          rpc_s_cant_bind_sock
**                          rpc_s_coding_error
**                          rpc_s_invalid_endpoint_format
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

PUBLIC void rpc_server_use_protseq_ep
    (rpc_protseq, max_calls, endpoint, status)

unsigned_char_p_t       rpc_protseq;
unsigned32              max_calls;
unsigned_char_p_t       endpoint;
unsigned32              *status;

{
    rpc_protseq_id_t        pseq_id;
    rpc_naf_id_t            naf_id;
    rpc_protocol_id_t       prot_id;
    rpc_prot_network_epv_p_t net_epv;
    rpc_naf_epv_p_t         naf_epv;
    unsigned32              temp_status;
    rpc_addr_p_t            rpc_addr;
    unsigned_char_p_t       endpoint_copy;
    unsigned32              count;
	
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_DBG_PRINTF(rpc_es_dbg_general, 1, ("use_protseq %s[%s]\n", rpc_protseq, endpoint));
	 
    /*
     * Until both protocol services fully implement this argument, we'll
     * ignore the value provided and use the default instead.
     */
    max_calls = rpc_c_protseq_max_reqs_default;

    /*
     * Find the correct entry in the RPC Protocol Sequence ID table using the
     * RPC Protocol Sequence string passed in as an argument.
     */
    pseq_id = rpc__network_pseq_id_from_pseq (rpc_protseq, status);
    if (*status != rpc_s_ok)
    {
        return;
    }

    /*
     * Make a copy of the endpoint, removing unwanted escape chars.
     */
    endpoint_copy = NULL;

    if (endpoint != NULL)
    {
        count = strlen ((char*) endpoint);
        RPC_MEM_ALLOC (
            endpoint_copy,
            unsigned_char_p_t,
            count + 1,
            RPC_C_MEM_STRING,
            RPC_C_MEM_WAITOK);
        /* fixed by lukeh - escaping was not terminating correctly */
        memset(endpoint_copy, 0, count + 1);

        /*
         * copy the string, filtering out escape chars
         */
        {
            unsigned int i;
            unsigned_char_p_t	p1, p2;
            for (i = 0, p1 = endpoint_copy, p2 = endpoint;
                        i < count; 
                        i++, p2++)
            {
                if (*p2 != '\\') 
                {
                    *p1++ = *p2;
                }
                /*
                 * Copy escaped escapes, i.e., "\\" ==> "\"
                 */
                else if (((count - i) > 1) && (p2[1] == '\\'))
                {
                    *p1++ = *p2;
                }
            }
        }
        endpoint_copy[count] = '\0';
    }

    /*
     * Call the rpc__naf_addr_alloc() through the Network Address Family EPV
     * with the RPC Protocol Sequence ID, the Network Address Family ID, an
     * endpoint and all other fields set to NULL.  This will return an
     * initialized rpc_addr which contains the necessary info for the
     * subsequent rpc__socket_bind() call (in the protocol service's
     * network_use_protseq function).
     */
    naf_id = RPC_PROTSEQ_INQ_NAF_ID (pseq_id);
    naf_epv = RPC_NAF_INQ_EPV (naf_id); /* pointer to the epv   */

    (*naf_epv->naf_addr_alloc) (
        pseq_id,                    /* in  - protocol sequence id       */
        naf_id,                     /* in  - network address family id  */
        endpoint_copy,              /* in  - endpoint address (pointer) */
        (unsigned_char_p_t) NULL,   /* in  - network address  (pointer) */
        (unsigned_char_p_t) NULL,   /* in  - network options  (pointer) */
        &rpc_addr,                  /* out - rpc address      (pointer) */
        status);                    /* out - status           (pointer) */

    if (*status != rpc_s_ok)
    {
        rpc_string_free (&endpoint_copy, &temp_status);
        return;
    }

    /*
     * Find the Network EPV in the RPC Protocol table using the RPC
     * Protocol ID found in the RPC Protocol Sequence ID table entry.
     */
    prot_id = RPC_PROTSEQ_INQ_PROT_ID (pseq_id);
    net_epv = RPC_PROTOCOL_INQ_NETWORK_EPV (prot_id);

    /*
     * Call the protocol service to do the real work of creating the
     * socket(s), setting them up right, and adding them (via calls
     * to rpc__network_add_desc).
     */
    (*net_epv->network_use_protseq)
                        (pseq_id, max_calls, rpc_addr, endpoint_copy, status);


    /*
     * Free the rpc_addr we allocated above.
     */
    (*naf_epv->naf_addr_free)(&rpc_addr, &temp_status);

    if (endpoint_copy != NULL)
    {
        rpc_string_free (&endpoint_copy, &temp_status);
    }
}

/*
**++
**
**  ROUTINE NAME:       bv_alloc
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Allocate a binding vector.  If "old_vec" is non-NULL, copy its contents
**  into the newly allocated vector.
**
**  INPUTS:
**
**      old_vec         The source binding vector (optional).
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      new_vec         The binding vector to be created.
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_no_memory
**                          rpc_s_coding_error
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

#define BINDING_VEC_INCR_LEN 1

INTERNAL void bv_alloc (old_vec, new_vec, status)

rpc_binding_vector_p_t  old_vec;
rpc_binding_vector_p_t  *new_vec;
unsigned32              *status;

{
    rpc_binding_vector_p_t  bvp;
    int                     bv_size;    /* sizeof the binding vector */
    int                     i;
    int                     new_count, old_count;


    CODING_ERROR (status);

    *new_vec = NULL;        /* assume the worst */

    old_count = (old_vec == NULL) ? 0 : old_vec->count;
    new_count = old_count + BINDING_VEC_INCR_LEN;

    /*
     * Allocate up a vector to hold bindings.  We don't know how many
     * bindings we're going to end up with, so this is only a guess.
     * we may need to realloc later.
     */
    bv_size = sizeof (rpc_binding_vector_t)         /* sizeof basic struct */
            + sizeof (handle_t) * (new_count - 1);  /* sizeof binding      */

    /*
     * Allocate the vector.
     */
    RPC_MEM_ALLOC(bvp, rpc_binding_vector_p_t, bv_size,
        RPC_C_MEM_BINDING_VEC, RPC_C_MEM_WAITOK);

    bvp->count = new_count;

    /*
     * Copy the old vector's contents and then free it.
     * NULL out unused entries.
     */
    for (i = 0; i < old_count; i++)
    {
        bvp->binding_h[i] = old_vec->binding_h[i];
    }

    if (old_vec != NULL)
    {
        RPC_MEM_FREE (old_vec, RPC_C_MEM_BINDING_VEC); 
    }

    for (i = old_count; i < new_count; i++)
    {
        bvp->binding_h[i] = NULL;
    }

    *new_vec = bvp;
    *status = rpc_s_ok;
}

#ifdef ATFORK_SUPPORTED
/*
**++
**
**  ROUTINE NAME:       rpc__network_fork_handler
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Initializes this module.
**
**  INPUTS:             stage   The stage of the fork we are 
**                              currently handling.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
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

PRIVATE void rpc__network_fork_handler
(
  rpc_fork_stage_id_t stage
)
{   
    switch ((int)stage)
    {
    case RPC_C_PREFORK:
        rpc__nlsn_fork_handler(&listener_state, stage);
        break;
    case RPC_C_POSTFORK_CHILD:  
        /*
         * Reset the listener_state table to 0's.
         */
        /*b_z_e_r_o((char *)&listener_state, sizeof(listener_state));*/
        memset( &listener_state, 0, sizeof listener_state );
        /*
         * Reset the global forwarding map function variable.
         */
        rpc_g_fwd_fn = NULL;
        /* fall through */
    case RPC_C_POSTFORK_PARENT:
        rpc__nlsn_fork_handler(&listener_state, stage);
        break;
    }
}
#endif
