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
**
**  NAME:
**
**    COMTWR.C
**
**  FACILITY:
**
**    Remote procedure call (RPC) 
**  
**  ABSTRACT:
**
**  Set of routines supporting operations that create and manipulate the
**  canonical representation of a protocol tower.
**
**
**/

#include <commonp.h>    /* Private defs for Common component            */
#include <com.h>        /* Private COM defs for other RPC components    */
#include <comp.h>       /* Privates for COM Services component          */
#include <comtwr.h>     /* Private tower defs                           */
#include <comtwrref.h>  /* Private tower ref defs for other RPC components */


/*
**++
**  ROUTINE NAME:       rpc__tower_free
**
**  SCOPE:              PRIVATE - declared in comtwr.h
**
**  DESCRIPTION:
**
**  Releases memory used by a tower that was dynamically created.
**  Towers are dynamically created by calling the 
**  rpc__tower_from_tower_ref routine.
**
**  INPUTS:             none
**
**  INPUT/OUTPUTS:          
**
**      tower           Canonical representation of a protocol tower.
**                      Nulled on return.
**
**  OUTPUTS:            
**
**      status          Returns the status code from the free
**                      operation. This status code indicates whether the 
**                      routine completed successfully and, if not, why.
**                      Returns rpc_s_ok.
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
*/

PRIVATE void rpc__tower_free 
(
    twr_p_t             *tower,
    unsigned32          *status
)
{
    CODING_ERROR (status);

    RPC_MEM_FREE (*tower, RPC_C_MEM_TOWER);
    *tower = NULL;

    *status = rpc_s_ok;
    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_from_tower_ref
**
**  SCOPE:              PRIVATE - declared in comtwr.h
**
**  DESCRIPTION:
**
**  This routine creates the canonical representation of a protocol tower 
**  from a runtime reference representation of a protocol tower. 
**
**  INPUTS:             
**
**      tower_ref       Runtime reference representation of a protocol tower.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:            
**
**      tower           Returns a pointer to the canonical representation
**                      of a protocol tower.
**
**      status          Returns the status code from the
**                      tower from tower_ref operation. This status code 
**                      indicates whether the routine completed
**                      successfully and, if not, why.
**                      Returns 
**                          rpc_s_ok
**                      or status from a called routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      void
**
**  SIDE EFFECTS:       none
**
**--
*/

PRIVATE void rpc__tower_from_tower_ref 
(
    rpc_tower_ref_p_t       tower_ref,
    twr_p_t                 *tower,
    unsigned32              *status
)
{
    byte_p_t                tower_p;
    unsigned16              twr_rep_16;
    unsigned32              i, 
                            floor_size,
                            octet_length;


    /* 
     * Calculate the size of the tower octet string.
     */
    for (i = 0, octet_length = 0;
         i < tower_ref->count; i++)
    {
        octet_length += (tower_ref->floor[i]->prot_id_count +
                         RPC_C_TOWER_FLR_LHS_COUNT_SIZE +
                         tower_ref->floor[i]->address_count +
                         RPC_C_TOWER_FLR_RHS_COUNT_SIZE);
    }

    octet_length += RPC_C_TOWER_FLR_COUNT_SIZE;

    /*
     * Allocate the tower structure to hold the 
     * canonical representation of the tower.
     */
    RPC_MEM_ALLOC ( 
        *tower, 
        twr_p_t, 
        sizeof (twr_t) + octet_length - 1,
        RPC_C_MEM_TOWER, RPC_C_MEM_WAITOK );

    /*
     * Initialize the tower length in the tower structure.
     */
    (*tower)->tower_length = octet_length;

    /*
     * Form the tower octet string starting
     * with the tower floor count size.
     */
    tower_p = (*tower)->tower_octet_string;

    /*
     * Convert the tower count to little endian 
     * and copy it to the octet string.
     */
    twr_rep_16 = tower_ref->count;
    RPC_RESOLVE_ENDIAN_INT16 (twr_rep_16);
    memcpy ((char *)tower_p, 
            (char *)&twr_rep_16, RPC_C_TOWER_FLR_COUNT_SIZE);


    tower_p += RPC_C_TOWER_FLR_COUNT_SIZE;

    /*
     * And now copy each tower floor to the octet string.
     */
    for (i=0; i < tower_ref->count; i++)
    {
        floor_size = 
            RPC_C_TOWER_FLR_LHS_COUNT_SIZE  + 
            tower_ref->floor[i]->prot_id_count +
            RPC_C_TOWER_FLR_RHS_COUNT_SIZE  +
            tower_ref->floor[i]->address_count;

        memcpy ((char *)tower_p, (char *)tower_ref->floor[i]->octet_string,
                floor_size);

        tower_p += floor_size;
    }

    *status = rpc_s_ok;
    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_to_tower_ref
**
**  SCOPE:              PRIVATE - declared in comtwr.h
**
**  DESCRIPTION:
**
**  Creates a runtime reference representation of a protocol tower 
**  from a canonical representation of a protocol tower.
**
**  INPUTS:             
**
**      tower           Canonical representation of a protocol tower.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:            
**
**      tower_ref       Returns a pointer to a reference representation 
**                      of a protocol tower.
**      status          Returns the status code from the tower to tower ref
**                      operation. This status code indicates whether the 
**                      routine completed successfully and, if not, why.
**                      Returns
**                          rpc_s_ok
**                      or status from a called routine.
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
*/

PRIVATE void rpc__tower_to_tower_ref 
(
    twr_p_t             tower,
    rpc_tower_ref_p_t   *tower_ref,
    unsigned32          *status
)
{
    unsigned16              floor_count;


    CODING_ERROR (status);

    if (tower->tower_length < RPC_C_TOWER_FLR_COUNT_SIZE)
    {
        *status = rpc_s_not_rpc_tower;
        return;
    }

    /*
     * Get the tower floor count and correct for proper endian.
     */
    memcpy ((char *) &floor_count, (char *) tower->tower_octet_string, 
            RPC_C_TOWER_FLR_COUNT_SIZE);
    RPC_RESOLVE_ENDIAN_INT16 (floor_count);

    rpc__tower_verify (tower->tower_octet_string, tower->tower_length, floor_count, status);
    if (*status != rpc_s_ok)
    {
        return;
    }

    /*
     * Allocate and initialize the tower reference structure to be returned.
     */
    rpc__tower_ref_alloc (tower->tower_octet_string, floor_count, 1, 
        tower_ref, status);

    /*
     * Return status from the tower ref allocate operation.
     */
    return;
}


/*
**++
**  ROUTINE NAME:       rpc_tower_to_binding
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Creates a binding handle from a canonical representation of a 
**  protocol tower.  After the caller is finished with the binding, the
**  rpc_binding_free routine must be called to release the memory used
**  by the binding.
**
**  Note, this is an SPI routine - available to outside the runtime,
**  but only to other DCE components.
**
**  INPUTS:             
**
**      prot_tower          A single protocol tower (DNA$Tower attribute value)
**                          to convert to a binding handle.
**
**  INPUT/OUTPUTS:          none
**
**  OUTPUTS:            
**
**      binding             Returns a binding handle.
**      status              Returns the status code from the tower-to-binding
**                          operation. This status code indicates whether the 
**                          routine completed successfully and, if not, why.
**                          Returns
**                              rpc_s_ok
**                          or status from a called routine.
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**  
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC void rpc_tower_to_binding 
(
    byte_p_t                prot_tower,
    rpc_binding_handle_t    *binding,
    unsigned32              *status
)
{
    rpc_binding_rep_p_t     binding_rep;
    rpc_protocol_id_t       prot_id;
    rpc_addr_p_t            rpc_addr;


    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * Null binding in case of error before finishing.
     */
    *binding = NULL;

    /*
     * Obtain an RPC address for the tower.
     */
    rpc__naf_tower_flrs_to_addr (prot_tower, &rpc_addr, status);
    if (*status != rpc_s_ok)
    {
        return;
    }

    prot_id = RPC_PROTSEQ_INQ_PROT_ID(rpc_addr->rpc_protseq_id);

    /*
     * Allocate and initialize a binding rep.
     */
    binding_rep = rpc__binding_alloc 
                    (false, &uuid_g_nil_uuid, prot_id, rpc_addr, status);

    /*
     * Return binding handle to user.
     */
    *binding = (rpc_binding_handle_t) binding_rep;

    /*
     * Return status from rpc__binding_alloc
     */
    return;

}


/*
**++
**  ROUTINE NAME:       rpc_tower_vector_from_binding
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Creates a vector of twr_t's from a binding handle. After the caller is
**  finished with the tower vector, the rpc_tower_vector_free routine must be
**  called to release the memory used by the vector. 
**
**  Note, this is an SPI routine - available to outside the runtime,
**  but only to other DCE components.
**
**  INPUTS:             
**
**      if_spec             Interface spec to combine with a binding
**                          handle to form a tower vector.
**
**      binding             Binding handle to combine with an interface 
**                          spec to form a tower vector.
**
**  INPUT/OUTPUTS:          none
**
**  OUTPUTS:            
**
**      tower_vector        Returns an allocated tower vector.
**
**      status              Returns the status code from the 
**                          tower-vector-from-binding operation. 
**                          This status code indicates whether the 
**                          routine completed successfully and, if not, why.
**                          Returns
**                              rpc_s_ok
**                              rpc_s_no_interfaces
**                          or status from a called routine.
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**  
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC void rpc_tower_vector_from_binding 
(
    rpc_if_handle_t         if_spec,
    rpc_binding_handle_t    binding,
    rpc_tower_vector_p_t    *twr_vector,
    unsigned32              *status
)
{
    rpc_tower_ref_vector_t  *tower_ref_vector;
    unsigned int                      i;
    unsigned32               temp_status;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    /*
     * Null the twr_vector in case of error before finishing.
     */
    *twr_vector = NULL;

    if (if_spec == NULL)
    {
        *status = rpc_s_no_interfaces;
        return;
    }

    /*
     * Convert the binding to a vector of tower refs.
     */

    rpc__tower_ref_vec_from_binding ((rpc_if_rep_p_t)if_spec, binding, 
        &tower_ref_vector, status);

    if (*status != rpc_s_ok)
    {
        /*
         * No need to goto CLEANUP; since a tower_ref_vector wasn't
         * returned to us.
         */
        return;
    }

    /*
     * Allocate a rpc_tower_vector_t based on the number of returned
     * tower refs.
     */
    RPC_MEM_ALLOC ( 
        *twr_vector, 
        rpc_tower_vector_p_t, 
        sizeof (rpc_tower_vector_t) + (tower_ref_vector->count - 1) * 
            sizeof (twr_p_t),
        RPC_C_MEM_TOWER_VECTOR,
        RPC_C_MEM_WAITOK );

    (*twr_vector)->count = tower_ref_vector->count;

    /*
     * For each returned tower ref convert the tower ref to a twr_t and
     * store the twr_t in the rpc_tower_vector_t.
     */
    for (i = 0; i < tower_ref_vector->count; i++)
    {
        rpc__tower_from_tower_ref (tower_ref_vector->tower[i],
            &(*twr_vector)->tower[i], status);

        if (*status != rpc_s_ok)
        {
            RPC_MEM_FREE (*twr_vector, RPC_C_MEM_TOWER_VECTOR);

            goto CLEANUP;
        }
    }
   
CLEANUP:
    /*
     * Free the tower_ref_vector returned from 
     * rpc__tower_ref_vec_from_binding().
     */
    rpc__tower_ref_vec_free (&tower_ref_vector, &temp_status);

    /*
     * If we got this far successfully, return whatever the result from 
     * rpc__tower_ref_vec_free(). Otherwise, return the previous error
     * in status.
     */
    if (*status == rpc_s_ok)
    {
        *status = temp_status;
    }

    return;
}

/*
**++
**  ROUTINE NAME:       rpc_tower_vector_free
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Releases memory associated with a tower vector, 
**  including the towers as well as the vector.
**
**  INPUTS:             none
**
**  INPUT/OUTPUTS:          
**
**      twr_vector      The tower vector to free. Nulled on return.
**
**  OUTPUTS:
**
**      status          Returns the status code from the tower free
**                      operation. This status code is a value that
**                      indicates whether the routine completed
**                      successfully and, if not, why.
**                      Returns 
**                          rpc_s_ok 
**                          or status from a called routine.
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
*/

PRIVATE void rpc_tower_vector_free 
(
    rpc_tower_vector_p_t    *twr_vector,
    unsigned32              *status
)
{
    unsigned32      i;


    CODING_ERROR (status);

    /*
     * Free each tower reference in the vector.  
     */
    for (i=0; i < (*twr_vector)->count; i++)
    {
        rpc__tower_free (&((*twr_vector)->tower[i]), status);
        if (*status != rpc_s_ok)
        {
            return;
        }
    }

    /*
     * Free the tower vector structure and set pointer to NULL.
     */
    RPC_MEM_FREE (*twr_vector, RPC_C_MEM_TOWER_VECTOR);

    *twr_vector = NULL;

    *status = rpc_s_ok;
    return;
}

