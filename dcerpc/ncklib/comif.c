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
**      comif.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Interface Service for the Common Communications Service.  Contains 
**  routines to register an interface, unregister an interface, perform lookups
**  of interface specifications within the Interface Registry Table, which is
**  contained within this module.
**
**
*/

#include <commonp.h>    /* Common internals for RPC runtime system  */
#include <com.h>        /* Externals for Common Services component  */
#include <comp.h>       /* Internals for Common Services component  */

#include <dce/mgmt.h>

/*
 * the size of the interface registry hash table
 * - pick a prime number to avoid collisions
 */
#define RPC_C_IF_REGISTRY_SIZE       31

/*
 *  The Interface Registry Table, where interface specifications
 *  are registered, and upon which the routines contained within
 *  this module perform their actions.  Protected by "if_mutex".
 */
INTERNAL rpc_list_t if_registry[RPC_C_IF_REGISTRY_SIZE] = { {0,0} };
INTERNAL rpc_mutex_t if_mutex;

/*
 * an if registry list entry
 */
typedef struct
{
    rpc_list_t      link;
    rpc_if_rep_p_t  if_spec;
    rpc_mgr_epv_t   default_mepv;
    unsigned        copied_mepv: 1; /* 1 = mepv copied at registration time  */
    unsigned        internal: 1; /* 1 = internal if; don't wildcard unregstr */
    rpc_list_t      type_info_list;
    unsigned32      flags;
    unsigned32      max_calls;
    unsigned32      max_rpc_size;
    rpc_if_callback_fn_t if_callback;
} rpc_if_rgy_entry_t, *rpc_if_rgy_entry_p_t;


/*
 * an if type/info list entry
 */
typedef struct
{
    rpc_list_t      link;
    dce_uuid_t          type;        /* type of object to which entry applies    */
    rpc_mgr_epv_t   mepv;        /* pointer to manager procedures            */
    unsigned        copied_mepv: 1; /* 1 = mepv copied at registration time  */
} rpc_if_type_info_t, *rpc_if_type_info_p_t;


INTERNAL void unregister_if_entry (
        rpc_if_rgy_entry_p_t    /*if_entry*/,
        dce_uuid_p_t                /*mgr_type_uuid*/,
        unsigned32              * /*status*/
    );

/*
**++
**
**  ROUTINE NAME:       rpc__if_init
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Initializes this module.
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

PRIVATE void rpc__if_init
(
    unsigned32 * status 
)
{
    RPC_MUTEX_INIT (if_mutex);
    *status = rpc_s_ok;
}


/*
**++
**
**  ROUTINE NAME:       rpc__if_fork_handler
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

PRIVATE void rpc__if_fork_handler
(
  rpc_fork_stage_id_t stage
)
{   
    unsigned32 i;

    switch ((int)stage)
    {
        case RPC_C_PREFORK:
                break;
        case RPC_C_POSTFORK_PARENT:
                break;
        case RPC_C_POSTFORK_CHILD:  
                /*
                 * Empty the Interface Registry Table
                 */                                  
                for (i = 0; i < RPC_C_IF_REGISTRY_SIZE; i++)
                {
                    if_registry[i].next = NULL;
                    if_registry[i].last = NULL;
                }
                break;
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc_server_register_if
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  See description of "rpc__server_register_if_int".
**
**  INPUTS:
**
**      ifspec_h        Pointer to the ifspec
**
**      mgr_type_uuid   The interface type (if any)
**
**      mgr_epv         The manager epv for this interface
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_type_already_registered
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

PUBLIC void rpc_server_register_if 
(
    rpc_if_handle_t             ifspec_h,
    dce_uuid_p_t                    mgr_type_uuid,
    rpc_mgr_epv_t               mgr_epv,
    unsigned32                  *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    rpc__server_register_if_int
        (ifspec_h, mgr_type_uuid, mgr_epv, 0,
         rpc_c_listen_max_calls_default, -1, NULL, false, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__server_register_if_int
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Perform a hash lookup through the Interface Registry Table to try to
**  locate the interface spec requested. If the search fails, add the
**  interface entry to the appropriate list. Then proceed to insert the type
**  uuid specified into the linked list for the requested interface.  If the
**  requested interface was located by the lookup, simply insert the type
**  uuid.
**
**  If a NULL manager entry point vector is specified, use the one in the
**  ifspec.
**
**  INPUTS:
**
**      ifspec_h        Pointer to the ifspec
**
**      mgr_type_uuid   The interface type (if any)
**
**      mgr_epv         The manager epv for this interface
**
**      flags           Registration flags
**
**      max_calls       Maximum calls
**
**      max_rpc_size    Maximum RPC size (-1 for no maximum)
**
**      if_callback     Security callback
**
**      is_internal     T => manager should not be unregisterable by wildcarding
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_type_already_registered
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

PRIVATE void rpc__server_register_if_int 
(
    rpc_if_handle_t             ifspec_h,
    dce_uuid_p_t                    mgr_type_uuid,
    rpc_mgr_epv_t               mgr_epv,
    unsigned32                  flags,
    unsigned32                  max_calls,
    unsigned32                  max_rpc_size,
    rpc_if_callback_fn_t        if_callback,
    boolean32                   is_internal,
    unsigned32                  *status
)
{ 
    rpc_if_rep_p_t              if_rep = (rpc_if_rep_p_t) ifspec_h;
    rpc_mgr_epv_t               mepv;
    rpc_if_rgy_entry_p_t        if_entry;
    rpc_if_type_info_p_t        type_info = NULL;
    unsigned32                  index;
    unsigned32                  ctr;
    boolean                     copied_mepv = false;
    boolean                     type_info_alloced = false;
    boolean                     type_info_added = false;
    boolean                     if_entry_alloced = false;
    boolean                     if_entry_added = false;
    
    
    CODING_ERROR (status);

    RPC_IF_VALIDATE(if_rep, status);
    if (*status != rpc_s_ok)
    {
        return;
    }

    /* 
     * check to see if a NULL mgr_epv was passed
     * - if so, use the default one in the ifspec
     * - if it's non-NULL, make a copy of it
     */
    if (mgr_epv == (rpc_mgr_epv_t) NULL)
    {
        mepv = if_rep->manager_epv;
        if (mepv == NULL)
        {
            *status = rpc_s_no_mepv;
            return;
        }
    }
    else
    {
        RPC_MEM_ALLOC (
            mepv,
            rpc_mgr_epv_t,
            sizeof (rpc_mgr_epv_t *) +
                (sizeof (rpc_mgr_proc_t) * (if_rep->opcnt - 1)),
            RPC_C_MEM_MGR_EPV,
            RPC_C_MEM_WAITOK);

        if (mepv == NULL)
        {
            *status = rpc_s_no_memory;
            return;
        }

        /*
         * fill the manager epv entries into the copy
         */
        for (ctr = 0; ctr < if_rep->opcnt; ctr++)
        {
            mepv[ctr] = mgr_epv[ctr];
        }

        copied_mepv = true;
    }

    /*
     * compute a hash value using the interface uuid - check the status
     * from dce_uuid_hash to make sure the uuid has a valid format
     */
    index = (dce_uuid_hash (&(if_rep->id), status)) % RPC_C_IF_REGISTRY_SIZE;
    
    if (*status != uuid_s_ok)
    {
        if (copied_mepv)
        {
            RPC_MEM_FREE (mepv, RPC_C_MEM_MGR_EPV);
        }
        return;
    }

    /*
     * take out a lock to protect access to the if registry
     */
    RPC_MUTEX_LOCK (if_mutex);

    /*
     * see if the specified interface already exists in the registry
     */
    RPC_LIST_FIRST (if_registry[index], if_entry, rpc_if_rgy_entry_p_t);

    while (if_entry != NULL)
    {
        /*
         * see if the if uuid and version match
         */
        if ((UUID_EQ (if_entry->if_spec->id, if_rep->id, status))
            && if_entry->if_spec->vers == if_rep->vers)
        {
            break;
        }

        RPC_LIST_NEXT (if_entry, if_entry, rpc_if_rgy_entry_p_t);
    }

    /*
     * if no entry was found, create one and add it to the list
     */
    if (if_entry == NULL)
    {
        RPC_MEM_ALLOC (
            if_entry,
            rpc_if_rgy_entry_p_t,
            sizeof (rpc_if_rgy_entry_t),
            RPC_C_MEM_IF_RGY_ENTRY,
            RPC_C_MEM_WAITOK);
            
        if (if_entry == NULL)
        {
            *status = rpc_s_no_memory;
            goto ERROR_AND_LOCKED;
        }
        if_entry_alloced = true;

        /*
         * initialize the entry
         */
        if_entry->if_spec = if_rep;
        if_entry->default_mepv = NULL;
        if_entry->internal = is_internal;
        RPC_LIST_INIT (if_entry->type_info_list);
        if_entry->flags = flags;
        if_entry->max_calls = max_calls;
        if_entry->max_rpc_size = max_rpc_size;
        if_entry->if_callback = if_callback;

        /*
         * put it on the list for this hash index
         */
        RPC_LIST_ADD_TAIL (if_registry[index], if_entry, rpc_if_rgy_entry_p_t);
        if_entry_added = true;
    }

    /*
     * see if a manager type was specified (and is non-nil)
     */
    if (mgr_type_uuid != NULL && !(dce_uuid_is_nil (mgr_type_uuid, status)))
    {
        /*
         * see if the specified mgr type already exists for this entry
         */
        RPC_LIST_FIRST
            (if_entry->type_info_list, type_info, rpc_if_type_info_p_t);

        while (type_info != NULL)
        {
            /*
             * see if the type uuid matches the one specified
             */
            if (UUID_EQ (type_info->type, *mgr_type_uuid, status))
            {
                *status = rpc_s_type_already_registered;
                goto ERROR_AND_LOCKED;
            }

            RPC_LIST_NEXT (type_info, type_info, rpc_if_type_info_p_t);
        }

        /*
         * if no entry was found, create one and add it to the list
         */
        if (type_info == NULL)
        {
            /*
             * malloc a type uuid/manager epv list element
             */
            RPC_MEM_ALLOC (
                type_info,
                rpc_if_type_info_p_t,
                sizeof (rpc_if_type_info_t),
                RPC_C_MEM_IF_TYPE_INFO,
                RPC_C_MEM_WAITOK);

            if (type_info == NULL)
            {
                *status = rpc_s_no_memory;
                goto ERROR_AND_LOCKED;
            }
            type_info_alloced = true;
                
            /*
             * fill in the supplied type info
             */
            type_info->type = *mgr_type_uuid;
            type_info->mepv = mepv;
            type_info->copied_mepv = copied_mepv;

            /*
             * add the type info entry to the list for this interface
             */
            RPC_LIST_ADD_TAIL
                (if_entry->type_info_list, type_info, rpc_if_type_info_p_t);
            type_info_added = true;
        }
    }
    else
    {
        /*
         * set the default manager epv
         */
        if (if_entry->default_mepv != NULL)
        {
            *status = rpc_s_type_already_registered;
            goto ERROR_AND_LOCKED;
        }
        if_entry->default_mepv = mepv;
        if_entry->copied_mepv = copied_mepv;
    }

    /*
     * free the global lock and return
     */
    RPC_MUTEX_UNLOCK (if_mutex);
    *status = rpc_s_ok;
    return;

ERROR_AND_LOCKED:
    /*
     * perform necessary cleanup, free the global lock and return
     * (the return status already set).
     */

    if (type_info_alloced)
    {
        if (type_info_added)
        {
            RPC_LIST_REMOVE (if_entry->type_info_list, type_info);
        }
        RPC_MEM_FREE (type_info, RPC_C_MEM_IF_TYPE_INFO);
    }
    if (if_entry_alloced)
    {
        if (if_entry_added)
        {
            RPC_LIST_REMOVE (if_registry[index], if_entry);
        }
        RPC_MEM_FREE (if_entry, RPC_C_MEM_IF_RGY_ENTRY);
    }
    if (copied_mepv)
    {
        RPC_MEM_FREE (mepv, RPC_C_MEM_MGR_EPV);
    }

    RPC_MUTEX_UNLOCK (if_mutex);
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_unregister_if_int
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Perform a hash lookup on the Interface Registry Table to locate the
**  specified interface spec. If the search fails, return
**  'rpc_s_unknown_if'.  If a registered interface is located with the
**  UUID and version specified in the if_spec, it is deleted from the
**  Interface Registry Table along with the linked list of Type UUIDs.
**
**  Also returns the pointer to the ifspec in the entry that was
**  unregistered.  This pointer turns out to be needed by the compatibility
**  library.
**
**  INPUTS:
**
**      ifspec_h            Pointer to the ifspec
**
**      mgr_type_uuid       The type uuid of the manager epv to unregister.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rtn_ifspec_h    Pointer to the ifspec that was in the entry that was
**                      unregistered.
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_unknown_if
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

PRIVATE void rpc__server_unregister_if_int 
(
    rpc_if_handle_t             ifspec_h,
    dce_uuid_p_t                    mgr_type_uuid,
    rpc_if_handle_t             *rtn_ifspec_h,
    unsigned32                  *status
)
{
    rpc_if_rep_p_t              if_rep = (rpc_if_rep_p_t) ifspec_h;
    unsigned32                  index;
    rpc_if_rgy_entry_p_t        if_entry, next_if_entry;
    boolean                     found_mgr_type;
    

    CODING_ERROR (status);

    *rtn_ifspec_h = NULL;

    /*
     * take out a lock to protect access to the if registry
     */
    RPC_MUTEX_LOCK (if_mutex);

    /*
     * see what kind of unregister is wanted
     */
    if (if_rep == NULL)
    {
        /*
         * go through the whole registry hash table
         */
        for (index = 0, found_mgr_type = false;
            index < RPC_C_IF_REGISTRY_SIZE; index++)
        {
            /*
             * walk the list of entries for each hash value
             */
            RPC_LIST_FIRST (if_registry[index], if_entry, rpc_if_rgy_entry_p_t);

            while (if_entry != NULL)
            {
                /*
                 * do whatever unregister is required for this entry
                 */
                if (! if_entry->internal)
                {
                    unregister_if_entry (if_entry, mgr_type_uuid, status);

                    if (*status != rpc_s_ok)
                    {
                        if (*status != rpc_s_unknown_mgr_type)
                        {
                            RPC_MUTEX_UNLOCK (if_mutex);
                            return;
                        }
                    }
                    else
                    {
                        found_mgr_type = true;
                    }
                }

                RPC_LIST_NEXT (if_entry, next_if_entry, rpc_if_rgy_entry_p_t);

                /*
                 * See if there are any mgr types left for this interface
                 * entry.  If not, remove this entry from the list for this
                 * hash value, and free the entry.
                 */
                if (RPC_LIST_EMPTY (if_entry->type_info_list) &&
                    if_entry->default_mepv == NULL)
                {
                    RPC_LIST_REMOVE (if_registry[index], if_entry);
    
                    RPC_MEM_FREE (if_entry, RPC_C_MEM_IF_RGY_ENTRY);
                }

                if_entry = next_if_entry;
            }
        }

        if (!found_mgr_type)
        {
            RPC_MUTEX_UNLOCK (if_mutex);
            *status = rpc_s_unknown_mgr_type;
            return;
        }
    }
    else
    {
        /*
         * compute a hash value using the interface uuid - check the status
         * from dce_uuid_hash to make sure the uuid has a valid format
         */
        index = dce_uuid_hash (&(if_rep->id), status) % RPC_C_IF_REGISTRY_SIZE;

        if (*status != uuid_s_ok)
        {
            RPC_MUTEX_UNLOCK (if_mutex);
            return;
        }        

        /*
         * walk the list of entries for this hash value
         */
        RPC_LIST_FIRST (if_registry[index], if_entry, rpc_if_rgy_entry_p_t);

        while (if_entry != NULL)
        {
            if (! if_entry->internal)
            {
                /*
                 * see if the if uuid and version match
                 */
                if ((UUID_EQ (if_entry->if_spec->id, if_rep->id, status))
                    && if_entry->if_spec->vers == if_rep->vers)
                {
                    /*
                     * snag the ifspec now, before the if entry (maybe) gets
                     * freed.
                     */
                    *rtn_ifspec_h = (rpc_if_handle_t) if_entry->if_spec;

                    /*
                     * do whatever unregister is required for this entry
                     */
                    unregister_if_entry (if_entry, mgr_type_uuid, status);

                    if (*status != rpc_s_ok)
                    {
                        RPC_MUTEX_UNLOCK (if_mutex);
                        return;
                    }

                    /*
                     * See if there are any mgr types left for this
                     * interface entry.  If not, remove this entry from
                     * the list for this hash value, and free the entry.
                     */
                    if (RPC_LIST_EMPTY (if_entry->type_info_list) &&
                        if_entry->default_mepv == NULL)
                    {
                        RPC_LIST_REMOVE (if_registry[index], if_entry);

                        RPC_MEM_FREE (if_entry, RPC_C_MEM_IF_RGY_ENTRY);
                    }

                    /*
                     * finished searching
                     */
                    break;
                }
            }

            RPC_LIST_NEXT (if_entry, if_entry, rpc_if_rgy_entry_p_t);
        }

        if (*rtn_ifspec_h == NULL)
        {
            RPC_MUTEX_UNLOCK (if_mutex);
            *status = rpc_s_unknown_if;
            return;
        }
    }

    RPC_MUTEX_UNLOCK (if_mutex);
    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_unregister_if
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  See description of "rpc__server_unregister_int".
**
**  INPUTS:
**
**      ifspec_h            Pointer to the ifspec
**
**      mgr_type_uuid       The type uuid of the manager epv to unregister.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_unknown_if
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

PUBLIC void rpc_server_unregister_if 
(
    rpc_if_handle_t             ifspec_h,
    dce_uuid_p_t                    mgr_type_uuid,
    unsigned32                  *status
)
{
    rpc_if_handle_t             rtn_ifspec_h;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    rpc__server_unregister_if_int (ifspec_h, mgr_type_uuid, &rtn_ifspec_h, status);
}

/*
**  This routine is called by rpc_server_unregister_if() to process a single
**  interface entry in the if registry.
**/

INTERNAL void unregister_if_entry 
(
    rpc_if_rgy_entry_p_t    if_entry,
    dce_uuid_p_t                mgr_type_uuid,
    unsigned32              *status
)
{
    rpc_if_type_info_p_t        type_info;
    rpc_if_type_info_p_t        current_type_info;


    /*
     * see if this is a wildcard operation
     */
    if (mgr_type_uuid == NULL)
    {
        /*
         * free the default manager epv if copied on register
         */
        if (if_entry->copied_mepv)
        {
            RPC_MEM_FREE (if_entry->default_mepv, RPC_C_MEM_MGR_EPV);
        }        

        if_entry->default_mepv = NULL;

        /*
         * walk the type info list and remove all entries
         */
        RPC_LIST_FIRST
            (if_entry->type_info_list, type_info, rpc_if_type_info_p_t);

        while (type_info != NULL)
        {
            /*
             * free the mepv if copied during register
             */
            if (type_info->copied_mepv)
            {
                RPC_MEM_FREE (type_info->mepv, RPC_C_MEM_MGR_EPV);
            }

            /*
             * remove this entry from the list for this interface
             */
            RPC_LIST_REMOVE (if_entry->type_info_list, type_info);

            /*
             * save this entry and get the next one on the list
             */
            current_type_info = type_info;
            
            RPC_LIST_FIRST
                (if_entry->type_info_list, type_info, rpc_if_type_info_p_t);

            /*
             * free the type info entry itself
             */
            RPC_MEM_FREE (current_type_info, RPC_C_MEM_IF_TYPE_INFO);
        }
    }
    else
    {
        /*
         * see if this is an unregister for the default manager epv
         */
        if (dce_uuid_is_nil (mgr_type_uuid, status))
        {
            if (if_entry->default_mepv == NULL)
            {
                *status = rpc_s_unknown_mgr_type;
                return;
            }

            /*
             * free the default manager epv if copied on register
             */
            if (if_entry->copied_mepv)
            {
                RPC_MEM_FREE (if_entry->default_mepv, RPC_C_MEM_MGR_EPV);
            }        

            if_entry->default_mepv = NULL;
        }
        else
        {        
            /*
             * walk the type info list looking for matches
             */
            RPC_LIST_FIRST
                (if_entry->type_info_list, type_info, rpc_if_type_info_p_t);

            while (type_info != NULL)
            {
                /*
                 * only remove entries whose type uuid's match the one specified
                 */
                if (UUID_EQ (type_info->type, *mgr_type_uuid, status))
                {
                    /*
                     * free the mepv if copied during register
                     */
                    if (type_info->copied_mepv)
                    {
                        RPC_MEM_FREE (type_info->mepv, RPC_C_MEM_MGR_EPV);
                    }

                    /*
                     * remove this entry from the list for this interface
                     */
                    RPC_LIST_REMOVE (if_entry->type_info_list, type_info);

                    /*
                     * free the type info entry itself
                     */
                    RPC_MEM_FREE (type_info, RPC_C_MEM_IF_TYPE_INFO);

                    /*
                     * then stop the search
                     */
                    break;
                }

                RPC_LIST_NEXT (type_info, type_info, rpc_if_type_info_p_t);
            }

            if (type_info == NULL)
            {
                *status = rpc_s_unknown_mgr_type;
                return;
            }
        }
    }

    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc__if_lookup
**
**  SCOPE:              PRIVATE - declared in comif.h
**
**  DESCRIPTION:
**      
**  Look into the Interface Registry Table for the Interface requested.
**  The search is conducted in either one or two steps.  First the index
**  from a previous request that is remembered in 'ihint' is tried,  then
**  if this fails to locate the interface, a regular hash lookup is performed.
**  If the interface can't be located 'rpc_s_unknown_if' is returned.
**
**  Once the interface is located, its attached linked list of type UUIDs
**  is searched for the type UUID specified.  If the requested type uuid
**  is not found 'rpc_s_unknown_mgr_type' is returned.  Once the requested
**  interface and type UUID are located a pointer to the interface
**  specification is returned. Also returned are the server stub epv
**  and the manager epv. The index to this Interface registry is saved in
**  the location pointed to by 'ihint', under the assumption that a subsequent
**  lookup may request this same interface.
**
**  INPUTS:
**
**      if_uuid         The interface UUID to lookup
**
**      if_vers         The interface version to lookup
**
**      mgr_type_uuid   The interface type (if any)
**                      (set this to NULL or the nil uuid to cause the
**                      default manager epv to be returned)
**
**  INPUTS/OUTPUTS:
**
**      ihint           A hint of the index into the table for this if
**
**  OUTPUTS:
**
**      ifspec_h        Pointer to the ifspec
**                      (NULL on input if not wanted)
**
**      sepv            The server stub epv for this interface
**                      (NULL on input if not wanted)
**
**      mepv            The manager epv for this interface
**                      (NULL on input if not wanted)
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_unknown_if
**                          rpc_s_unknown_mgr_type
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

PRIVATE void rpc__if_lookup 
(
    dce_uuid_p_t                    if_uuid,
    unsigned32                  if_vers,
    dce_uuid_p_t                    mgr_type_uuid,
    unsigned16                  *ihint,
    rpc_if_rep_p_t              *ifspec,
    rpc_v2_server_stub_epv_t    *sepv,
    rpc_mgr_epv_t               *mepv,
    unsigned32                  *status
)
{
    return rpc__if_lookup2 (if_uuid, if_vers, mgr_type_uuid,
                            ihint, ifspec, sepv, mepv,
                            NULL, NULL, NULL, NULL, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__if_lookup2
**
**  SCOPE:              PRIVATE - declared in comif.h
**
**  DESCRIPTION:
**      
**  Look into the Interface Registry Table for the Interface requested.
**  The search is conducted in either one or two steps.  First the index
**  from a previous request that is remembered in 'ihint' is tried,  then
**  if this fails to locate the interface, a regular hash lookup is performed.
**  If the interface can't be located 'rpc_s_unknown_if' is returned.
**
**  Once the interface is located, its attached linked list of type UUIDs
**  is searched for the type UUID specified.  If the requested type uuid
**  is not found 'rpc_s_unknown_mgr_type' is returned.  Once the requested
**  interface and type UUID are located a pointer to the interface
**  specification is returned. Also returned are the server stub epv
**  and the manager epv. The index to this Interface registry is saved in
**  the location pointed to by 'ihint', under the assumption that a subsequent
**  lookup may request this same interface.
**
**  INPUTS:
**
**      if_uuid         The interface UUID to lookup
**
**      if_vers         The interface version to lookup
**
**      mgr_type_uuid   The interface type (if any)
**                      (set this to NULL or the nil uuid to cause the
**                      default manager epv to be returned)
**
**  INPUTS/OUTPUTS:
**
**      ihint           A hint of the index into the table for this if
**
**  OUTPUTS:
**
**      ifspec_h        Pointer to the ifspec
**                      (NULL on input if not wanted)
**
**      sepv            The server stub epv for this interface
**                      (NULL on input if not wanted)
**
**      mepv            The manager epv for this interface
**                      (NULL on input if not wanted)
**
**      flags           The interface registration flags
**                      (NULL on input if not wanted)
**
**      max_calls       The maximum number of concurrent RPCs
**                      (NULL on input if not wanted)
**
**      max_rpc_size    The maximum size of incoming data blocks
**                      (NULL on input if not wanted)
**
**      if_callback     The security callback function
**                      (NULL on input if not wanted)
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_unknown_if
**                          rpc_s_unknown_mgr_type
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

PRIVATE void rpc__if_lookup2
(
    dce_uuid_p_t                    if_uuid,
    unsigned32                  if_vers,
    dce_uuid_p_t                    mgr_type_uuid,
    unsigned16                  *ihint,
    rpc_if_rep_p_t              *ifspec,
    rpc_v2_server_stub_epv_t    *sepv,
    rpc_mgr_epv_t               *mepv,
    unsigned32                  *flags,
    unsigned32                  *max_calls,
    unsigned32                  *max_rpc_size,
    rpc_if_callback_fn_t        *if_callback,
    unsigned32                  *status
)
{
    rpc_if_rgy_entry_p_t        if_entry = NULL;
    rpc_if_type_info_p_t        type_info;
    unsigned32                  index;
    unsigned32                  entry_count = 0;
    unsigned32                  temp_status;
    
    RPC_LOG_IF_LOOKUP_NTR;
    CODING_ERROR (status);

    /*
     * take out a lock to protect access to the if registry
     */
    RPC_MUTEX_LOCK (if_mutex);
    
    /*
     * see if the interface hint we're given is valid
     */
    if (*ihint != RPC_C_INVALID_IHINT)
    {
        /*
         * extract the hash value from the low byte of the ihint
         * and the list entry count form the high byte
         */
        index = *ihint & 0x00FF;
        entry_count = (*ihint & 0xFF00) >> 8;

        /*
         * try to find a match using the hints provided
         */
        RPC_LIST_LOOKUP
            (if_registry[index], if_entry, rpc_if_rgy_entry_p_t, entry_count);

        if (if_entry != NULL && 
            ! RPC_IF_IS_COMPATIBLE (if_entry, if_uuid, if_vers, status))
        {
            if_entry = NULL;
        }
    }
    else
    {
        /*
         * compute a hash value using the interface uuid - check the status
         * from dce_uuid_hash to make sure the uuid has a valid format
         */
        index = dce_uuid_hash (if_uuid, status) % RPC_C_IF_REGISTRY_SIZE;
    
        if (*status != uuid_s_ok)
        {
            RPC_MUTEX_UNLOCK (if_mutex);
            return;
        }
    }
    
    /*
     * if we got this far and didn't find a match, search the whole list
     * under the current hash value for the given interface
     */
    if (if_entry == NULL)
    {
        RPC_LIST_FIRST (if_registry[index], if_entry, rpc_if_rgy_entry_p_t);

        for (entry_count = 1; if_entry != NULL; entry_count++)
        {
            if (RPC_IF_IS_COMPATIBLE (if_entry, if_uuid, if_vers, status))
            {
                break;
            }

            RPC_LIST_NEXT (if_entry, if_entry, rpc_if_rgy_entry_p_t);
        }
    }

    /*
     * if still no entry was found, report unknown interface
     */
    if (if_entry == NULL)
    {
        *ihint = RPC_C_INVALID_IHINT;
        *status = rpc_s_unknown_if;
        RPC_MUTEX_UNLOCK (if_mutex);
        return;
    }        

    
    /*
     * if a manager type uuid is given, and is not the nil uuid, try to match
     * for a registered type - otherwise, return the default manager epv
     */
    if (mgr_type_uuid != NULL && !(dce_uuid_is_nil (mgr_type_uuid, status)))
    {
        /*
         * scan the type uuid/mgr epv list for a match
         */
        RPC_LIST_FIRST
            (if_entry->type_info_list, type_info, rpc_if_type_info_p_t);

        while (type_info != NULL)
        {
            if (UUID_EQ (type_info->type, *mgr_type_uuid, status))
            {
                if (mepv != NULL)
                {
                    *mepv = type_info->mepv;
                }

                break;
            }

            RPC_LIST_NEXT (type_info, type_info, rpc_if_type_info_p_t);
        }
        
        if (type_info == NULL)
        {
            /*
             * if no 'type' match is found, invalidate the interface
             * hint.  But before giving up entirely, if the interface
             * we're looking for is the internal mgmt interface, try
             * one more time using a NIL type; this lets clients make
             * remote mgmt calls using bindings that might have a typed
             * object in them.
             */
            *ihint = RPC_C_INVALID_IHINT;
            *status = rpc_s_unknown_mgr_type;
            RPC_MUTEX_UNLOCK (if_mutex);

            if (UUID_EQ (((rpc_if_rep_p_t) mgmt_v1_0_s_ifspec)->id, *if_uuid, &temp_status))
            {
                rpc__if_lookup2 (if_uuid, if_vers, NULL, ihint, ifspec, sepv, mepv,
                                 flags, max_calls, max_rpc_size, if_callback, status);
            }

            return;
        }
    }
    else
    {    
        /*
         * The default (nil-type) manager epv is requested,
         * return an error if one isn't registered and a manager epv
         * is wanted.
         */
        if (mepv != NULL)
        {
            if (if_entry->default_mepv == NULL)
            {
                *ihint = RPC_C_INVALID_IHINT;
                *status = rpc_s_unknown_mgr_type;
                RPC_MUTEX_UNLOCK (if_mutex);
                return;
            }
            *mepv = if_entry->default_mepv;
        }
    }

    /*
     * if a complete match is found, return its info (if wanted)
     */
    if (ifspec != NULL)
    {
        *ifspec = if_entry->if_spec;
    }

    if (sepv != NULL)
    {
        *sepv = if_entry->if_spec->server_epv;
    }

    if (flags != NULL)
    {
        *flags = if_entry->flags;
    }

    if (max_calls != NULL)
    {
        *max_calls = if_entry->max_calls;
    }

    if (max_rpc_size != NULL)
    {
        *max_rpc_size = if_entry->max_rpc_size;
    }

    if (if_callback != NULL)
    {
        *if_callback = if_entry->if_callback;
    }

    /*
     * provide a useful hint for future references to this interface
     */
    *ihint = (unsigned16) (index | ((entry_count & 0x00FF) << 8));

    RPC_MUTEX_UNLOCK (if_mutex);
    *status = rpc_s_ok;
    RPC_LOG_IF_LOOKUP_XIT;

    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc_if_inq_id
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Extract the interface id (UUID and version numbers) from the given
**  interface spec.
**
**  INPUTS:
**
**      ifspec_h        Pointer to the ifspec
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      if_id           The interface id.
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

PUBLIC void rpc_if_inq_id 
(
    rpc_if_handle_t             ifspec_h,
    rpc_if_id_t                 *if_id,
    unsigned32                  *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * copy the interface UUID from the if_spec
     */
    if_id->uuid = ((rpc_if_rep_p_t) (ifspec_h))->id;

    /*
     * convert the old form of the version number (single unsigned long)
     * into the new form (major and minor unsigned16's)
     */
    if_id->vers_major = RPC_IF_VERS_MAJOR(((rpc_if_rep_p_t) ifspec_h)->vers);
    if_id->vers_minor = RPC_IF_VERS_MINOR(((rpc_if_rep_p_t) (ifspec_h))->vers);

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc__if_id_compare
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Compares two interface id's and matches based on the version option.
**
**  INPUTS:
**
**      if_id_ref       The reference interface id against which the comparison
**                      is to be made.
**
**      if_id           The interface id to be compared to the reference.
**
**      if_vers_option  The criteria by which the if version numbers are to
**                      be compared. One of:
**                          rpc_c_vers_all
**                          rpc_c_vers_compatible
**                          rpc_c_vers_exact
**                          rpc_c_vers_major_only
**                          rpc_c_vers_upto
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          the result of dce_uuid_equal()
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      true if the interface id is compatible with the reference
**      false if the interface id is not compatible with the reference
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean rpc__if_id_compare 
(
    rpc_if_id_p_t           if_id_ref,
    rpc_if_id_p_t           if_id,
    unsigned32              if_vers_option,
    unsigned32              *status
)
{
    *status = rpc_s_ok;
    
    /*
     * see if the returned if uuid matches the one given
     */
    if (! (UUID_EQ (if_id->uuid, if_id_ref->uuid, status)))
    {
        /*
         * return "incompatible" and the status of dce_uuid_equal()
         */
        return (false);
    }
    else
    {
        /*
         * if they do match, check what the version option is
         */
        switch ((int)if_vers_option)
        {
            /*
             * any version is ok
             */
            case rpc_c_vers_all:
            {
                return (true);
            }

            /*
             * major versions must match, minor version must be greater than
             * or equal to minor version in the reference
             */
            case rpc_c_vers_compatible:
            {
                if (if_id->vers_major == if_id_ref->vers_major &&
                    if_id->vers_minor >= if_id_ref->vers_minor)
                {
                    return (true);
                }
                else
                {
                    return (false);
                }
            }

            /*
             * major and minor versions must match
             */
            case rpc_c_vers_exact:
            {
                if (if_id->vers_major == if_id_ref->vers_major &&
                    if_id->vers_minor == if_id_ref->vers_minor)
                {
                    return (true);
                }
                else
                {
                    return (false);
                }
            }

            /*
             * major versions must match - minor versions are ignored
             */
            case rpc_c_vers_major_only:
            {
                if (if_id->vers_major == if_id_ref->vers_major)
                {
                    return (true);
                }
                else
                {
                    return (false);
                }
            }

            /*
             * major version and minor version must both be less than or
             * equal to their counterparts in the reference
             */
            case rpc_c_vers_upto:
            {
                if (if_id->vers_major < if_id_ref->vers_major)
                {
                    return (true);
                }
                else
                {
                    if (if_id->vers_major == if_id_ref->vers_major &&
                        if_id->vers_minor <= if_id_ref->vers_minor)
                    {
                        return (true);
                    }
                    else
                    {
                        return (false);
                    }
                }
            }
        }
        return (false);
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc_if_id_vector_free
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Free the memory allocated for an rpc_if_id_vector_t.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      if_id_vector    The vector of interface id's to be freed.
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_coding_error
**                          rpc_s_invalid_arg
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

PUBLIC void rpc_if_id_vector_free 
(
    rpc_if_id_vector_p_t    *if_id_vector,
    unsigned32              *status
)
{
    unsigned32              i;
    

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();
    
    /*
     * check to see if if_id_vector is NULL, and if
     * so, return an error status
     */
    if (if_id_vector == NULL)
    {
        *status = rpc_s_invalid_arg;
        return;
    }
    
 
    /*
     * walk the if id vector and free each element
     */
    for (i = 0; i < (*if_id_vector)->count; i++)
    {
        if ((*if_id_vector)->if_id[i] != NULL)
        {
            RPC_MEM_FREE ((*if_id_vector)->if_id[i], RPC_C_MEM_IF_ID);
        }
    }
    
    /*
     * then free the vector itself
     */
    RPC_MEM_FREE ((*if_id_vector), RPC_C_MEM_IF_ID_VECTOR);
    
    /*
     * return a NULL pointer
     */
    *if_id_vector = NULL;
    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc__if_inq_endpoint
**
**  SCOPE:              PRIVATE - declared in comif.h
**
**  DESCRIPTION:
**      
**  With the given interface spec, search through the associated array
**  of RPC Protocol Sequence/endpoint pairs.  Compare the Protocol Sequence
**  ID given to the one in the ifspec (protocol_id's are used instead
**  of protseq id strings to make handling of aliases simpler and
**  localized). Filter out any extraneous info, such as "endpoint=" and
**  other information the user may have put into the endpoint attribute in
**  the idl file.  If the requested endpoint can't be located, return 
**  'rpc_s_endpoint_not_found'.
**
**  INPUTS:
**
**      ifspec_h        Pointer to the ifspec
**
**      protseq_id      The protocol sequence to be matched
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      endpoint        The endpoint contained int he ifspec for the
**                      given protocol sequence.
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_endpoint_not_found
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

PRIVATE void rpc__if_inq_endpoint 
(
    rpc_if_rep_p_t              ifspec,
    rpc_protseq_id_t            protseq_id,
    unsigned_char_t             **endpoint,
    unsigned32                  *status
)
{
    unsigned16              ctr;
    rpc_protseq_id_t        pseq_id;
    unsigned_char_t         *scratch_endpoint;  

    CODING_ERROR (status);
    
    for (ctr = 0; ctr < ifspec->endpoint_vector.count; ctr++)
    {
        pseq_id = rpc__network_pseq_id_from_pseq (
            ifspec->endpoint_vector.endpoint_vector_elt[ctr].rpc_protseq, 
            status);
        if (*status == rpc_s_protseq_not_supported)
        {
            *status = rpc_s_ok;
            continue;
        }
        if (*status != rpc_s_ok)	{
            return;
		  }

        if (pseq_id == protseq_id)
        {
            /*
             * Allocate enough space so we can place brackets around the
             * string before attempting to filter it.  We need 3 extra 
             * bytes, for '[', ']', and '\0'.
             */
            RPC_MEM_ALLOC (
                scratch_endpoint,
                unsigned_char_p_t, 
                (strlen ((char *)
                    ifspec->endpoint_vector.endpoint_vector_elt[ctr].endpoint)
                    +3),
                RPC_C_MEM_STRING,
                RPC_C_MEM_WAITOK);

            scratch_endpoint[0] = '[';
            strcpy ((char *) &scratch_endpoint[1], (char *)
                (ifspec->endpoint_vector.endpoint_vector_elt[ctr].endpoint));
            scratch_endpoint[strlen((char *)
                ifspec->endpoint_vector.endpoint_vector_elt[ctr].endpoint)+1] =
                ']';
            scratch_endpoint[strlen((char *)
                ifspec->endpoint_vector.endpoint_vector_elt[ctr].endpoint)+2] =
                '\0';

            /*
             * Extract just the endpoint portion.
             */
            rpc_string_binding_parse(scratch_endpoint, NULL, NULL, NULL,
                endpoint, NULL, status);

            RPC_MEM_FREE(scratch_endpoint, RPC_C_MEM_STRING);

            return;
        }
    }
    *status = rpc_s_endpoint_not_found;
}

/*
**++
**
**  ROUTINE NAME:       rpc__if_set_wk_endpoint
**
**  SCOPE:              PRIVATE - declared in comif.h
**
**  DESCRIPTION:
**      
**  Set an RPC addr's endpoint based on the well-known endpoint in the
**  given ifspec, if there is one.
**
**  INPUTS:
**
**      ifspec_h        Pointer to the ifspec
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The rpc address in which to set the endpoint
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_too_many_ifs
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

PRIVATE void rpc__if_set_wk_endpoint 
(
  rpc_if_rep_p_t          ifspec,
  rpc_addr_p_t            *rpc_addr,
  unsigned32              *status
)
{
    unsigned_char_p_t       endpoint;
    unsigned32              temp_status;


    CODING_ERROR (status);
    
    /*
     * get the endpoint from the if spec
     */
    rpc__if_inq_endpoint (
        ifspec,
        (*rpc_addr)->rpc_protseq_id,
        &endpoint,
        status);

    if (*status == rpc_s_ok)
    {
        /*
         * call the naf extension service to put the endpoint in the rpc addr
         */
        rpc__naf_addr_set_endpoint (endpoint, rpc_addr, status);
        rpc_string_free (&endpoint, &temp_status);
    }

    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc__if_mgmt_inq_num_registered
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Perform a linear search through the Interface Registry Table.
**  For each slot found to contain a registered Interface specification,
**  increment a counter.  Return the number of active slots located.
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
**      count           The number of registered interfaces.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__if_mgmt_inq_num_registered( void )
{
    unsigned32              entry_count = 0;
    unsigned32              index;
    rpc_if_rgy_entry_p_t    if_entry;
    

    /*
     * take out a lock to protect access to the if registry
     */
    RPC_MUTEX_LOCK (if_mutex);

    /*
     * walk the hash table
     */
    for (index = 0; index < RPC_C_IF_REGISTRY_SIZE; index++)
    {
        /*
         * walk the list under this hash entry
         */
        RPC_LIST_FIRST (if_registry[index], if_entry, rpc_if_rgy_entry_p_t);
        
        while (if_entry != NULL)
        {
            /*
             * don't count internal entries
             */
            if (! if_entry->internal)
            {
                /*
                 * bump the count for each entry in the list
                 */
                entry_count++;
            }

            RPC_LIST_NEXT (if_entry, if_entry, rpc_if_rgy_entry_p_t);
        }
    }

    RPC_MUTEX_UNLOCK (if_mutex);

    /*
     * report the total number of entries found
     */
    return (entry_count);
}

/*
**++
**
**  ROUTINE NAME:       rpc__if_mgmt_inq_if_ids
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Obtain the number of active entries in the Interface Registry Table.
**  Then allocate enough memory to hold a vector of if_id elements.  Scan
**  through the Interface Registry Table and for each active interface copy
**  its UUID and Version into an rpc_if_id_t element. Return a pointer, to
**  the vector of info elements that have been built.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      if_info         Pointer to an if id vector
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_no_interfaces
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

PRIVATE void rpc__if_mgmt_inq_if_ids 
(
    rpc_if_id_vector_p_t    *if_id_vector,
    unsigned32              *status
)
{
    rpc_if_rgy_entry_p_t    if_entry;
    unsigned32              if_count;
    unsigned32              index;
    unsigned32              if_id_index;
    unsigned32              temp_status;
    
    
    CODING_ERROR (status);
    
    /*
     * find the number of registered interfaces
     */
    if ((if_count = rpc__if_mgmt_inq_num_registered()) == 0)
    {
        *status = rpc_s_no_interfaces;
	*if_id_vector = NULL;
        return;
    }
    
    /*
     * allocate memory for the if id vector
     */
    RPC_MEM_ALLOC (
        *if_id_vector,
        rpc_if_id_vector_p_t,
        ((sizeof if_count) + (if_count * sizeof (rpc_if_id_p_t))),
        RPC_C_MEM_IF_ID_VECTOR,
        RPC_C_MEM_WAITOK);

    /*
     * set the count field in the vector
     */
    (*if_id_vector)->count = if_count;
    
    /*
     * take out a lock to protect access to the if registry
     */
    RPC_MUTEX_LOCK (if_mutex);

    /*
     * search through Interface Registry Table
     */
    for (index = 0, if_id_index = 0; index < RPC_C_IF_REGISTRY_SIZE; index++)
    {
        /*
         * walk the list under this hash entry
         */
        RPC_LIST_FIRST (if_registry[index], if_entry, rpc_if_rgy_entry_p_t);
        
        while (if_entry != NULL)
        {
            /*
             * don't report internal entries
             */
            if (! if_entry->internal)
            {
                /*
                 * allocate memory for the if id
                 */
                RPC_MEM_ALLOC (
                    (*if_id_vector)->if_id[if_id_index],
                    rpc_if_id_p_t,
                    sizeof (rpc_if_id_t),
                    RPC_C_MEM_IF_ID,
                    RPC_C_MEM_WAITOK);
                
                /*
                 * extract the if id info for this registry entry
                 */
                rpc_if_inq_id ((rpc_if_handle_t) (if_entry->if_spec),
                    (*if_id_vector)->if_id[if_id_index], status);

                if (*status != rpc_s_ok)
                {
                    /*
                     * If anything went wrong, free the vector allocated; 
                     * but first reset the count field to the right value.
                     */
                    (*if_id_vector)->count = if_id_index;
                    rpc_if_id_vector_free (if_id_vector, &temp_status);
                    RPC_MUTEX_UNLOCK (if_mutex);
                    return;
                }
                if_id_index++;
            }

            RPC_LIST_NEXT (if_entry, if_entry, rpc_if_rgy_entry_p_t);
        }
    }

    RPC_MUTEX_UNLOCK (if_mutex);
    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc_server_inq_if
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Given an interface spec and type ID, return the manager EPV that has
**  been registered for them (if any).
**
**  INPUTS:
**
**      ifspec_h        Pointer to the ifspec
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      mgr_type_uuid   The interface type
**
**      mgr_epv         The manager epv for this interface
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

PUBLIC void rpc_server_inq_if 
(
    rpc_if_handle_t             ifspec_h,
    dce_uuid_p_t                    mgr_type_uuid,
    rpc_mgr_epv_t               *mgr_epv,
    unsigned32                  *status
)
{
    rpc_if_rep_p_t              ifspec = (rpc_if_rep_p_t) ifspec_h;
    unsigned16                  ihint = RPC_C_INVALID_IHINT;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    rpc__if_lookup2 (&ifspec->id, ifspec->vers, mgr_type_uuid, 
                     &ihint, NULL, NULL, mgr_epv,
                     NULL, NULL, NULL, NULL, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc_server_register_if_ex
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  See description of "rpc__server_register_if_int".
**
**  INPUTS:
**
**      ifspec_h        Pointer to the ifspec
**
**      mgr_type_uuid   The interface type (if any)
**
**      mgr_epv         The manager epv for this interface
**
**      flags           Registration flags
**
**      max_calls       The maximum number of calls
**
**      if_callback     Security callback
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_type_already_registered
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

PUBLIC void rpc_server_register_if_ex
(
    rpc_if_handle_t             ifspec_h,
    dce_uuid_p_t                    mgr_type_uuid,
    rpc_mgr_epv_t               mgr_epv,
    unsigned32                  flags,
    unsigned32                  max_calls,
    rpc_if_callback_fn_t        if_callback,
    unsigned32                  *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    rpc__server_register_if_int
        (ifspec_h, mgr_type_uuid, mgr_epv, flags,
         max_calls, -1, if_callback, false, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc_server_register_if_2
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  See description of "rpc__server_register_if_int".
**
**  INPUTS:
**
**      ifspec_h        Pointer to the ifspec
**
**      mgr_type_uuid   The interface type (if any)
**
**      mgr_epv         The manager epv for this interface
**
**      flags           Registration flags
**
**      max_calls       The maximum number of calls
**
**      max_rpc_size    Maximum RPC size in bytes
**
**      if_callback     Security callback
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The result of the operation. One of:
**                          rpc_s_ok
**                          rpc_s_type_already_registered
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

PUBLIC void rpc_server_register_if_2
(
    rpc_if_handle_t             ifspec_h,
    dce_uuid_p_t                    mgr_type_uuid,
    rpc_mgr_epv_t               mgr_epv,
    unsigned32                  flags,
    unsigned32                  max_calls,
    unsigned32                  max_rpc_size,
    rpc_if_callback_fn_t        if_callback,
    unsigned32                  *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    rpc__server_register_if_int
        (ifspec_h, mgr_type_uuid, mgr_epv, flags,
         max_calls, max_rpc_size, if_callback, false, status);
}

