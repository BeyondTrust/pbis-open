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
**      comobj.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definition of the Object Services for the Common Communication
**  Services component. These routines are called by an application
**  to register objects and object types with the runtime.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Public common communications services */
#include <comprot.h>    /* Common protocol services */
#include <comnaf.h>     /* Common network address family services */
#include <comp.h>       /* Private common communications services */

/*
 * the size of the object registry hash table
 * - pick a prime number to avoid collisions
 */
#define RPC_C_OBJ_REGISTRY_SIZE         31

/*
 *  The Object Registry Table, where object/type pairs
 *  are registered, and upon which the routines contained within
 *  this module perform their actions.  Protected by "obj_mutex".
 */
INTERNAL rpc_list_t obj_registry[RPC_C_OBJ_REGISTRY_SIZE] = { {0,0} };
INTERNAL rpc_mutex_t obj_mutex;


/*
 * an object registry list entry
 */
typedef struct
{
    rpc_list_t      link;
    dce_uuid_t          object_uuid;
    dce_uuid_t          type_uuid;
} rpc_obj_rgy_entry_t, *rpc_obj_rgy_entry_p_t;


/*
 * Function registered by applicationthrough rpc_object_set_inq_fn.
 */
INTERNAL rpc_object_inq_fn_t inq_fn = NULL;


/*
**++
**
**  ROUTINE NAME:       rpc__obj_init
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

PRIVATE void rpc__obj_init 
(
    unsigned32                  *status
)
{
    CODING_ERROR (status);

    RPC_MUTEX_INIT (obj_mutex);
    *status = rpc_s_ok;
}



/*
**++
**
**  ROUTINE NAME:       rpc__obj_fork_handler
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

PRIVATE void rpc__obj_fork_handler
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

                inq_fn = NULL;
            
                /*
                 * Empty the Object Registry Table
                 */                                  
                for (i = 0; i < RPC_C_OBJ_REGISTRY_SIZE; i++)
                {
                    obj_registry[i].next = NULL;
                    obj_registry[i].last = NULL;
                }
                break;
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc_object_set_type
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Register the type UUID of an object with the Common Communications
**  Service. This routine is used in conjunction with rpc_server_register_if
**  to allow a server to support multiple implementations of the same
**  interface for different object types. Registering objects is required
**  only when generic interfaces are declared (via "rpc_server_register_if").
**  The Common Communications Service will dispatch to a specific
**  implementation, contained in a manager Entry Point Vector, based on the
**  object UUID contained in the binding of the RPC. The Common Communications
**  Service, using the results of a call to this routine, will determine
**  the type UUID of the object. A manager EPV for this type UUID can
**  then be found using the results of a call to the rpc_server_register_if
**  routine.
**
**  INPUTS:  
**
**      object_uuid     The object to be registered.
**
**      type_uuid       The type of the object.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
**          rpc_s_coding_error
**          rpc_s_invalid_object
**          rpc_s_invalid_type
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

PUBLIC void rpc_object_set_type 
(
    dce_uuid_p_t                object_uuid,
    dce_uuid_p_t                type_uuid,
    unsigned32              *status
)
{
    rpc_obj_rgy_entry_p_t   obj_entry;
    unsigned32              index;
    
    
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * check to see if this is a valid, non-null object UUIDs
     */
    if (UUID_IS_NIL (object_uuid, status))
    {
        *status = rpc_s_invalid_object;
        return;
    }

    if (*status != uuid_s_ok)
    {
        *status = rpc_s_invalid_object;
        return;
    }
    
    /*
     * compute a hash value using the object uuid - check the status
     * from dce_uuid_hash to make sure the uuid has a valid format
     */
    index = (dce_uuid_hash (object_uuid, status)) % RPC_C_OBJ_REGISTRY_SIZE;
    
    if (*status != uuid_s_ok)
    {
        return;
    }

    /*
     * take out a global lock on the object registry
     */
    RPC_MUTEX_LOCK (obj_mutex);

    /*
     * search the registry for a matching object
     */
    RPC_LIST_FIRST (obj_registry[index], obj_entry, rpc_obj_rgy_entry_p_t);

    while (obj_entry != NULL)
    {
        if (dce_uuid_equal (&(obj_entry->object_uuid), object_uuid, status))
        {
            break;
        }

        RPC_LIST_NEXT (obj_entry, obj_entry, rpc_obj_rgy_entry_p_t);
    }

    /*
     * if the type UUID is the NIL UUID, remove the type entry
     * for this object if it exists(this is basically an "unregister")
     */
    if (UUID_IS_NIL(type_uuid, status))
    {
        /*
         * remove the object entry from the registry and free it
         */
        if (obj_entry != NULL)
        {
            RPC_LIST_REMOVE (obj_registry[index], obj_entry);
            RPC_MEM_FREE (obj_entry, RPC_C_MEM_OBJ_RGY_ENTRY);
        }
    }
    else
    {
        /*
         * this is a register - if no entry was found, create one
         */
        if (obj_entry == NULL)
        {
            RPC_MEM_ALLOC (
                obj_entry,
                rpc_obj_rgy_entry_p_t,
                sizeof (rpc_obj_rgy_entry_t),
                RPC_C_MEM_OBJ_RGY_ENTRY,
                RPC_C_MEM_WAITOK);
            
            /*
             * initialize the entry
             */
            obj_entry->object_uuid = *object_uuid;

            /*
             * put the object on the list for this hash index
             */
            RPC_LIST_ADD_TAIL
                (obj_registry[index], obj_entry, rpc_obj_rgy_entry_p_t);
        }
        else
        {
            /*
             * see if the type uuid matches the one specified
             */
            if (dce_uuid_equal (&(obj_entry->type_uuid), type_uuid, status))
            {
                RPC_MUTEX_UNLOCK (obj_mutex);
                *status = rpc_s_already_registered;
                return;
            }
        }

        /*
         * set the type uuid for this object entry
         */
        obj_entry->type_uuid = *type_uuid;
    }

    RPC_MUTEX_UNLOCK (obj_mutex);
    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc_object_inq_type
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Consult the object registry for the specified object's type.  If
**  it is not found and the application has registered an inquiry function,
**  call it.  Otherwise, if object is not registered, the type returned
**  is nil_uuid.
**
**  Note: If a NULL value is passed for the type UUID argument the routine
**  will execute, but the type UUID will not be returned. This can be
**  useful to determine if an object is registered without requiring a
**  return value to be supplied.
** 
**  INPUTS:  
**
**      object_uuid     The object being looked up.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      type_uuid       The type of the object.
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok
**          rpc_s_object_not_found
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

PUBLIC void rpc_object_inq_type 
(
    dce_uuid_p_t                object_uuid,
    dce_uuid_t                  *type_uuid,
    unsigned32              *status
)
{
    rpc_obj_rgy_entry_p_t   obj_entry;
    unsigned32              index;


    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * lookups on nil objects return nil types
     */
    if (UUID_IS_NIL (object_uuid, status))
    {
        UUID_CREATE_NIL (type_uuid);
        *status = rpc_s_ok;
        return;
    }

    /*
     * compute a hash value using the object uuid - check the status
     * from dce_uuid_hash to make sure the uuid has a valid format
     */
    index = dce_uuid_hash (object_uuid, status) % RPC_C_OBJ_REGISTRY_SIZE;

    if (*status != uuid_s_ok)
    {
        return;
    }

    /*
     * take out a lock to protect access to the object registry
     */
    RPC_MUTEX_LOCK (obj_mutex);

    /*
     * search the table for the specified object UUID
     */
    RPC_LIST_FIRST (obj_registry[index], obj_entry, rpc_obj_rgy_entry_p_t);

    while (obj_entry != NULL)
    {
        if (dce_uuid_equal (&(obj_entry->object_uuid), object_uuid, status))
        {
            /*
             * if a type uuid is wanted, return it
             */
            if (type_uuid != NULL)
            {
                *type_uuid = obj_entry->type_uuid;
            }

            RPC_MUTEX_UNLOCK (obj_mutex);
            *status = rpc_s_ok;
            return;
        }

        RPC_LIST_NEXT (obj_entry, obj_entry, rpc_obj_rgy_entry_p_t);
    }

    /*
     * If there's an application function to map object to type, call it now.
     * Ensure that a object_not_found failure returns the nil-type.
     */
    if (inq_fn != NULL)
    {
        RPC_MUTEX_UNLOCK (obj_mutex);
        (*inq_fn) (object_uuid, type_uuid, status);
        if (*status == rpc_s_object_not_found)
        {
            UUID_CREATE_NIL (type_uuid);
        }
        return;
    }

    UUID_CREATE_NIL (type_uuid);

    RPC_MUTEX_UNLOCK (obj_mutex);
    *status = rpc_s_object_not_found;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc_object_set_inq_fn
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Supply a function that is called by the runtime to determine the type
**  of objects that have not been set by "rpc_object_set_type".
**
**  INPUTS:
**
**      inq_fn          function that does the inquiry
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
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_object_set_inq_fn 
(
    rpc_object_inq_fn_t     inq_fn_arg,
    unsigned32              *status
)
{
    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    inq_fn = inq_fn_arg;
    *status = rpc_s_ok;
}

