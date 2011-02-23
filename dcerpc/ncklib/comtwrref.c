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
**    COMTWRREF.C
**
**  FACILITY:
**
**    Remote procedure call (RPC) 
**  
**  ABSTRACT:
**
**  Set of routines supporting operations on the runtime reference 
**  represention of protocol towers.
**
**  
**
**/

#include <commonp.h>    /* Common internals for RPC Runtime system   */
#include <com.h>        /* Externals for Common Services component   */
#include <comp.h>       /* Privates for Common Services component    */
#include <comtwrref.h>  /* Private prototypes of this module         */
#include <comtwr.h>     /* tower defs for other RPC components       */
#include <comtwrflr.h>  /* Privates for the Tower floor services     */


/*
**++
**  ROUTINE NAME:       rpc__tower_ref_add_floor
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Adds a floor to a tower reference.
**
**  INPUTS:             
**      floor_number    The number of the floor to add to the tower.
**      floor           The tower floor to add to the tower.
**      tower_ref       The tower to which the floor is added.
**      
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**
**      status          Returns the status code from the tower add floor
**                      operation. This status code is a value that
**                      indicates whether the routine completed
**                      successfully and, if not, why.
**                          Returns rpc_s_ok.
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

PRIVATE void rpc__tower_ref_add_floor 
(
    unsigned32          floor_number,
    rpc_tower_floor_p_t floor,
    rpc_tower_ref_t     *tower_ref,
    unsigned32          *status
)
{

    CODING_ERROR (status);

    /*
     * If we're replacing the floor, free it first.
     */
    if (tower_ref->floor[floor_number-1] != NULL)
    {
        rpc__tower_flr_free (&(tower_ref->floor[floor_number-1]), status);
        if (*status != rpc_s_ok)
        {
            return;
        }
    }

    /*
     * Add the new floor.
     */
    tower_ref->floor[floor_number-1] = floor;

    *status = rpc_s_ok;

    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_ref_alloc
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Allocates memory for a RPC tower reference structure and initializes
**  the structure to point to the individual floors in the tower.
**  The returned referenced structure contains NULL floor pointers
**  for any unused floor.
**
**  INPUTS:             
**  
**      tower_octet_string  Canonical tower to reference using a tower
**                          reference structure.
**
**      num_flrs            Specifies the total number of floors to be stored
**                          in the returned tower reference argument.
**
**      start_flr           Tower floor number of the first floor in
**                          tower_octet_string.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**      tower_ref       The returned uninitialized RPC tower reference.
**
**      status          Returns the status code from the tower ref allocate
**                      operation.  This status code is a value that indicates
**                      whether the routine completed successfully and,
**                      if not, why.  Returns:
**                          rpc_s_ok
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

PRIVATE void rpc__tower_ref_alloc 
(
    byte_p_t            tower_octet_string,
    unsigned32          num_flrs,
    unsigned32          start_flr,
    rpc_tower_ref_p_t   *tower_ref,
    unsigned32          *status
)
{

    byte_p_t        tower_floor;
    unsigned32      i;


    CODING_ERROR (status);

    /*
     * Allocate the tower reference structure allowing 
     * for the number of tower floors desired.
     *
     * Note, the first floor is allocated via the "floor[1] 
     * member of rpc_tower_ref_t.
     */
    RPC_MEM_ALLOC (
        *tower_ref,
        rpc_tower_ref_p_t,
        sizeof (rpc_tower_ref_t) + 
        (sizeof (rpc_tower_floor_p_t) * (num_flrs-1)),
        RPC_C_MEM_TOWER_REF,
        RPC_C_MEM_WAITOK);

    /*
     * Initialize floor count to the number of floors and 
     * floor pointers to NULL.
     */
    (*tower_ref)->count = num_flrs;

    for (i=0; i < num_flrs; i++)
    {
        (*tower_ref)->floor[i] = NULL;
    }

    /*
     * Initialize local to point to beginning of
     * the canonical tower floor.
     */
    tower_floor =  tower_octet_string + RPC_C_TOWER_FLR_COUNT_SIZE;

    /*
     * For each tower floor, starting at the desired floor,
     * allocate the floor and initialize with the information 
     * in the tower structure.
     */
    for (i= start_flr-1; i < num_flrs; i++)
    {
        /*
         * Allocate the tower floor
         */
        RPC_MEM_ALLOC ( 
            (*tower_ref)->floor[i],
            rpc_tower_floor_p_t,
            sizeof (rpc_tower_floor_t),
            RPC_C_MEM_TOWER_FLOOR,
            RPC_C_MEM_WAITOK );

        /*
         * Initialize the floor's tower octet free flag to not 
         * free the tower floor octet string, since they will be 
         * freed by the caller who created them.
         */
        (*tower_ref)->floor[i]->free_twr_octet_flag = false;

        /*
         * Get the pointer to the octet string.
         */
        (*tower_ref)->floor[i]->octet_string = tower_floor;

        /*
         * Get the protocol identifier count.
         */
        memcpy ((char *) &((*tower_ref)->floor[i]->prot_id_count),
                (char *) tower_floor,
                RPC_C_TOWER_FLR_LHS_COUNT_SIZE);

        /*
         * Convert prot id count to host's endian representation.
         */
        RPC_RESOLVE_ENDIAN_INT16 ((*tower_ref)->floor[i]->prot_id_count);

        /*
         * Get the additional information count.
         */
        memcpy ((char *) &((*tower_ref)->floor[i]->address_count),
                (char *) tower_floor + RPC_C_TOWER_FLR_LHS_COUNT_SIZE + 
                (*tower_ref)->floor[i]->prot_id_count,
                RPC_C_TOWER_FLR_RHS_COUNT_SIZE);

        /*
         * Convert address count to host's endian representation.
         */
        RPC_RESOLVE_ENDIAN_INT16 ((*tower_ref)->floor[i]->address_count);

        /*
         * Point to the next tower floor in the tower octet string.
         */
        tower_floor += RPC_C_TOWER_FLR_LHS_COUNT_SIZE +
                       (*tower_ref)->floor[i]->prot_id_count +
                       RPC_C_TOWER_FLR_RHS_COUNT_SIZE +
                       (*tower_ref)->floor[i]->address_count;
    }


    *status = rpc_s_ok;
    return;
}

PRIVATE void rpc__tower_verify
(
    byte_p_t            tower_octet_string,
    unsigned32          length,
    unsigned32          num_flrs,
    unsigned32          *status
)
{
    byte_p_t        bound = tower_octet_string + length;
    byte_p_t        tower_floor;
    unsigned32      i;
    unsigned16      prot_count;
    unsigned16      addr_count;

    CODING_ERROR (status);

    if (bound < tower_octet_string)
    {
        *status = rpc_s_not_rpc_tower;
        return;
    }

    if (tower_octet_string + RPC_C_TOWER_FLR_COUNT_SIZE >= bound)
    {
        *status = rpc_s_not_rpc_tower;
        return;
    }

    /*
     * Initialize local to point to beginning of
     * the canonical tower floor.
     */
    tower_floor = tower_octet_string + RPC_C_TOWER_FLR_COUNT_SIZE;

    /*
     * For each tower floor, starting at the desired floor,
     * allocate the floor and initialize with the information 
     * in the tower structure.
     */
    for (i = 0; i < num_flrs; i++)
    {
        if (tower_floor + RPC_C_TOWER_FLR_LHS_COUNT_SIZE >= bound)
        {    
            *status = rpc_s_not_rpc_tower;
            return;
        }

        memcpy ((char *) &prot_count,
                (char *) tower_floor,
                RPC_C_TOWER_FLR_LHS_COUNT_SIZE);

        /*
         * Convert prot id count to host's endian representation.
         */
        RPC_RESOLVE_ENDIAN_INT16 (prot_count);

        if (tower_floor + RPC_C_TOWER_FLR_LHS_COUNT_SIZE + prot_count >= bound)
        {
            *status = rpc_s_not_rpc_tower;
            return;
        }

        /*
         * Get the additional information count.
         */
        memcpy ((char *) &addr_count,
                (char *) tower_floor + RPC_C_TOWER_FLR_LHS_COUNT_SIZE + prot_count,
                RPC_C_TOWER_FLR_RHS_COUNT_SIZE);

        /*
         * Convert address count to host's endian representation.
         */
        RPC_RESOLVE_ENDIAN_INT16 (addr_count);

        /*
         * Point to the next tower floor in the tower octet string.
         */
        tower_floor += RPC_C_TOWER_FLR_LHS_COUNT_SIZE + prot_count +
            RPC_C_TOWER_FLR_RHS_COUNT_SIZE + addr_count;
    }

    *status = rpc_s_ok;
    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_ref_copy
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Allocates memory for a RPC tower reference structure and
**  copies the contents of the source tower reference into it.
**  The new tower reference points to the same tower floor as
**  the source data structure.  The new tower reference free floor 
**  flag is set to false so that a call to free the tower reference
**  will not attempt to deallocate the tower floor twice.
**
**  INPUTS:             
**
**      source_tower    RPC tower reference to copy.
**      
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**
**      dest_tower      Returns a tower reference which is a copy of
**                      source_tower.
**
**      status          Returns the status code from the tower ref copy
**                      operation. This status code is a value that
**                      indicates whether the routine completed
**                      successfully and, if not, why.
**                          Returns rpc_s_ok.
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

PRIVATE void rpc__tower_ref_copy 
(
    rpc_tower_ref_p_t   source_tower,
    rpc_tower_ref_p_t   *dest_tower,
    unsigned32          *status
)
{
    unsigned32      i,
                    tower_ref_size;

    CODING_ERROR (status);


    tower_ref_size = sizeof (rpc_tower_ref_t) + 
                     (sizeof (rpc_tower_floor_p_t) * ((source_tower->count)-1));

    /*
     * Allocate the destination tower structure and
     * copy the source tower into it.
     */
    RPC_MEM_ALLOC (
        *dest_tower,
        rpc_tower_ref_p_t,
        tower_ref_size,
        RPC_C_MEM_TOWER_REF,
        RPC_C_MEM_WAITOK);
        
    /*
     * Copy the floor count to the destination tower ref.
     */
    (*dest_tower)->count = source_tower->count;


    /*
     * For each floor in the source tower ref, allocate a new floor 
     * for the destination tower ref and copy the floor contents.
     * Set the destination free flag to false since the 
     * floors' tower octet string will be freed when freeing 
     * the source_tower.
     */
    for (i=0; i < source_tower->count; i++)
    {
        RPC_MEM_ALLOC (
            (*dest_tower)->floor[i],
            rpc_tower_floor_p_t,
            sizeof (rpc_tower_floor_t),
            RPC_C_MEM_TOWER_FLOOR,
            RPC_C_MEM_WAITOK);

        memcpy ((*dest_tower)->floor[i], source_tower->floor[i],
                sizeof (rpc_tower_floor_t));

        (*dest_tower)->floor[i]->free_twr_octet_flag = false;
    }

    *status = rpc_s_ok;
    return;

}


/*
**++
**  ROUTINE NAME:       rpc__tower_ref_free
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Releases memory associated with a tower reference, including the 
**  tower floor as well as the tower reference.
**
**  INPUTS:             none
**
**      
**  INPUT/OUTPUTS:          
**
**      tower_ref       The tower reference to free. Nulled on return.
**
**  OUTPUTS:
**
**      status          Returns the status code from the tower ref free
**                      operation. This status code is a value that
**                      indicates whether the routine completed
**                      successfully and, if not, why.
**                          Returns rpc_s_ok.
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

PRIVATE void rpc__tower_ref_free 
(
    rpc_tower_ref_p_t       *tower_ref,
    unsigned32              *status
)
{
    unsigned32      i;


    CODING_ERROR (status);

    /*
     * Free the tower floors in the tower, freeing the octet string
     * associated with the each floor only if the free floor flag is set.
     */
    for (i=0; i < (*tower_ref)->count; i++)
    {
        rpc__tower_flr_free (&((*tower_ref)->floor[i]), status);
        if (*status != rpc_s_ok)
        {
            return;
        }
    }

    /*
     * Free the tower reference and set the pointer to NULL.
     */
    RPC_MEM_FREE (*tower_ref, RPC_C_MEM_TOWER_REF);
    
    *tower_ref = NULL;

    *status = rpc_s_ok;

    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_ref_inq_protseq_id
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Returns the RPC protocol sequence from a tower reference.
**
**  INPUTS:
**
**      tower_ref       Protocol tower reference from which to 
**                      return the RPC protocol sequence id.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**
**      protseq_id      Returns the protocol sequence id for tower_ref.
**
**      status          Returns the status code from the inquire protocol
**                      sequence operation. This status code is a value that
**                      indicates whether the routine completed
**                      successfully and, if not, why.
**                      Returns:
**                          rpc_s_ok
**                          rpc_s_not_rpc_tower
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

PRIVATE void rpc__tower_ref_inq_protseq_id 
(
    rpc_tower_ref_p_t   tower_ref,
    rpc_protseq_id_t    *protseq_id,
    unsigned32          *status
)
{
    boolean             match;
    rpc_flr_prot_id_t   *tower_prot_ids,
                        master_prot_ids[RPC_C_MAX_NUM_NETWORK_FLOORS + 1];
    byte_p_t            tp;
    unsigned32          floors_to_search,
                        start_floor,
                        i, j, k;            
    rpc_protocol_id_t   rpc_protocol_id;
    unsigned32          version_major;
    unsigned32          version_minor;

    CODING_ERROR (status);

    /*  
     * Initialize return protseq in case of failure.
     */
    *protseq_id = RPC_C_INVALID_PROTSEQ_ID;

    /*
     * Let's find out if this is a tower without floors 1 & 2 (CDS
     * has been known to use towers like this). So long as the tower
     * contains an RPC protocol (floor 3) and recognized addressing
     * floors, we'll try to figure out its protseq id.
     */

    /*
     * Check to see if this is a full tower.
     */
    if (tower_ref->count >= RPC_C_FULL_TOWER_MIN_FLR_COUNT)
    {
        /*
         * Calculate the number of floors we are searching.
         * This is the number of lower tower floors plus one for floor 3.
         */
        floors_to_search = (tower_ref->count - RPC_C_NUM_RPC_FLOORS)+ 1;

        /*
         * For a full rpc tower, identifying the protseq begins at the
         * rpc protocol id floor.
         */
        start_floor = RPC_C_NUM_RPC_FLOORS - 1;
    }
    else
    {
        /*
         * Check to see if this is a minimal tower.
         */
        if (tower_ref->count >= RPC_C_MIN_TOWER_MIN_FLR_COUNT)
        {
            /*
             * We might have a minimal rpc tower. Let's make sure the 1st floor
             * in the tower contains a valid rpc protocol.
             */
            rpc__tower_flr_to_rpc_prot_id (tower_ref->floor[0],
                &rpc_protocol_id, &version_major, &version_minor, status);

            /*
             * If the floor contains a valid rpc protocol id, we have a minimal
             * rpc tower and need to process all of the floors.
             */
            if (*status == rpc_s_ok)
            {
                floors_to_search = tower_ref->count;
                start_floor = 0;
            }
            else
            {
                /*
                 * We don't have even a minimal rpc tower.
                 */
                *status = rpc_s_not_rpc_tower;
                return;
            }
        }
        else
        {
            *status = rpc_s_not_rpc_tower;
            return;
        }
    }

    /*
     * Allocate the array to hold the tower's protocol ids.
     * This is one element for each of the lower tower floors plus
     * an element for the the RPC protocol id floor (in a minimal tower 
     * floor 1; in a full tower floor 3).
     */
    RPC_MEM_ALLOC (
        tower_prot_ids, 
        rpc_flr_prot_id_p_t, 
        floors_to_search * sizeof (rpc_flr_prot_id_t),
        RPC_C_MEM_TOWER_PROT_IDS,
        RPC_C_MEM_WAITOK);

    /*
     * Copy the tower floors' protocol id, starting at the
     * RPC protocol id, into the tower_prot_ids array.
     */
    for (i= 0, j= start_floor; i < floors_to_search; i++)
    {
        
        /*
         * Copy the floor's protocol id prefix.
         */
        memcpy ((char *) &(tower_prot_ids[i].prefix),
                (char *) RPC_PROT_ID_START(tower_ref->floor[i+j]),
                RPC_C_TOWER_PROT_ID_SIZE);
        /*
         * If the floor's protocol id also has an uuid,
         * copy it.
         */
        if (tower_ref->floor[i+j]->prot_id_count > RPC_C_TOWER_PROT_ID_SIZE)
        { 
            tp = (byte_p_t) RPC_PROT_ID_START(tower_ref->floor[i+j]);

            memcpy ((char *) &(tower_prot_ids[i].uuid),
                    (char *) (tp + RPC_C_TOWER_PROT_ID_SIZE),
                    RPC_C_TOWER_UUID_SIZE);

            RPC_RESOLVE_ENDIAN_UUID (tower_prot_ids[i].uuid);
        }
        else
        {
            tower_prot_ids[i].uuid = uuid_g_nil_uuid;
        }
    }

    /*
     * For each protocol sequence supported by RPC,
     * see if the tower protocol ids match.
     *
     * Note, we use RPC_C_PROTSEQ_ID_MAX+1 since 
     * there are two entries in our table for 
     * RPC_C_PROTSEQ_ID_NCACN_OSI_DNA - one for nsp 
     * and the other for tp4.
     */
    for (i = 0; i < rpc_g_tower_prot_id_number; i++)
    {
        /*
         * If the number of floors to process does not
         * match the number of floors for this protocol
         * sequence, skip it.
         */
        if (floors_to_search != rpc_g_tower_prot_ids[i].num_floors)
        {
            continue;
        }

        /*
         * Copy the protocol id for the current
         * protocol sequence being matched into
         * a local array.  Do this for the number
         * of floors in this protocol sequence.
         */
        for (k = 0; k < rpc_g_tower_prot_ids[i].num_floors; k++)
        {
            master_prot_ids[k].prefix = 
                rpc_g_tower_prot_ids[i].floor_prot_ids[k].prefix;

            master_prot_ids[k].uuid = 
                rpc_g_tower_prot_ids[i].floor_prot_ids[k].uuid;
        }

        /*
         * For each protocol id in the master array,
         * see if a match is found with the tower floors' protocol ids.
         * We only compare to the number of significant floors.
         */

        /*
         * Assume success. This way we can check the status
         * of dce_uuid_equal when it is called.
         */
        *status = rpc_s_ok;

        for (k = 0; k < rpc_g_tower_prot_ids[i].num_floors; k++)
        {
            for (j = 0; j < floors_to_search; j++)
            {
                if ((master_prot_ids[k].prefix == tower_prot_ids[j].prefix) &&
                    (dce_uuid_equal (&(master_prot_ids[k].uuid), 
                     &(tower_prot_ids[j].uuid), status)))
                {
                    master_prot_ids[k].prefix = 0;
                    break;
                }
                
                /*
                 * Check status from dce_uuid_equal.
                 * Return if failure.
                 */
                if (*status != rpc_s_ok)
                {
                    goto CLEANUP;
                }
            }
        }

        /*
         * See if a match was found
         */
        for (k = 0, match = true; k < rpc_g_tower_prot_ids[i].num_floors; k++)
        {
            if (master_prot_ids[k].prefix != 0)
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            *protseq_id = rpc_g_tower_prot_ids[i].rpc_protseq_id;

            /*
             * Status is already set above.
             */
            goto CLEANUP;
        }

    }

    /*
     * If we get here, then we couldn't find a match and must
     * assume that the tower does not belong to RPC.
     */
    *status = rpc_s_not_rpc_tower;

CLEANUP:
    /*
     * Free the allocated array of the tower floors
     * protocol identifier.
     */
    RPC_MEM_FREE (tower_prot_ids, RPC_C_MEM_TOWER_PROT_IDS);

    /*
     * Return with the protocol id sequence and status.
     */
    return;

}


/*
**++
**  ROUTINE NAME:       rpc__tower_ref_is_compatible
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Tells whether the specified protocol tower is compatible with 
**  the client's protocols.
**
**  INPUTS:
**
**      if_spec         The interface specification for the client.
**                      If NULL, the if_spec compatibility check is bypassed.
**
**      tower_ref       Protocol tower reference to be checked for 
**                      compatibility with the client.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**
**      status          Returns the status code from the tower-is-compatible
**                      operation. This status code is a value that
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
**      true            The specified protocol tower is compatible
**                      with the client's protocols.
**      false           The specified protocol tower is NOT compatible
**                      with the client's protocols.
**
**  SIDE EFFECTS:       none
**
**--
*/

PRIVATE boolean rpc__tower_ref_is_compatible 
(
    rpc_if_rep_p_t          if_spec,
    rpc_tower_ref_p_t       tower_ref,
    unsigned32              *status
)
{
    boolean                 match;
    unsigned32              if_spec_syntax_count,
                            tower_vers_major,
                            tower_vers_minor,
                            version_major,
                            version_minor;
    rpc_if_id_t             if_id,
                            tower_if_id;
    rpc_syntax_id_t         *if_syntax_id,
                            tower_syntax_id;
    rpc_protseq_id_t        tower_protseq_id;
    rpc_protocol_id_t       tower_prot_id;
    unsigned8               temp_id;


    CODING_ERROR (status);


    /*
     * Obtain the protocol sequence from this tower.
     */
    rpc__tower_ref_inq_protseq_id (tower_ref, &tower_protseq_id, status);
    if (*status != rpc_s_ok)
    {
        *status = rpc_s_ok;	/* ignore towers we don't understand */
        return (false);
    }

    /*
     * Ensure the protocol sequence from the tower 
     * is supported by this client.  
     */
    if (!(RPC_PROTSEQ_INQ_SUPPORTED (tower_protseq_id)))
    {
        /*
         * Invalid protocol sequence, return status from call.
         */
        return (false);
    }

    /*
     * We have a valid protocol sequence.
     * Check the interface for compatibility if one is specified.
     */
    if (if_spec != NULL)
    {

        /*
         * Get the interface identifier.
         */
        rpc_if_inq_id ((rpc_if_handle_t) if_spec, &if_id, status);
        if (*status != rpc_s_ok)
        {
            return (false);
        }
        
        /*
         * Get the interface identifier from the tower
         */
        rpc__tower_flr_to_if_id (tower_ref->floor[0], &tower_if_id, status);
        if (*status != rpc_s_ok)
        {
            return (false);
        }

        /*
         * Compare the client's interface identifier to the
         * tower's interface id.  (Checks both the uuid and version.)
         */
        if (!(rpc__if_id_compare 
            (&if_id, &tower_if_id, rpc_c_vers_compatible, status)))
        {
            return (false);
        }

        /* 
         * See if any of the if_spec transfer syntaxes matches 
         * the tower transfer syntaxes. 
         *
         * Note an interface transfer syntax count of 0 is an internal error.
         */
         
         /*
          * Obtain the tower transfer syntax.
          */
        rpc__tower_flr_to_drep (tower_ref->floor[1], &tower_syntax_id, status);
        if (*status != rpc_s_ok)
        {
            return (false);
        }

        for (if_spec_syntax_count = 0, 
             match = false,
             if_syntax_id = if_spec->syntax_vector.syntax_id;
             ((match == false) && 
              (if_spec_syntax_count < if_spec->syntax_vector.count ));
             if_spec_syntax_count++,
             if_syntax_id++ )
        {
            /*
             * Check if a syntax id and version match.
             */
            match = dce_uuid_equal 
                    (&(tower_syntax_id.id), &(if_syntax_id->id), status);

            if ((match == true) &&
                (tower_syntax_id.version != if_syntax_id->version))
            {
                match = false;
            }
        }

        /* 
         * if no match occurred, binding is not compatible - return false
         */
        if (match == false) 
        {
            *status = rpc_s_ok;
            return (false);
        }
    }

    /*
     * Obtain the RPC protocol id and version numbers of the tower.
     */
    rpc__tower_flr_to_rpc_prot_id (tower_ref->floor[2], &tower_prot_id,
        &tower_vers_major, &tower_vers_minor, status);
    if (*status != rpc_s_ok)
    {
        return (false);
    }

    /*
     * Obtain the clients' version numbers for the protocol sequence
     * specified in the tower.
     */
    rpc__network_inq_prot_version (
        tower_protseq_id, &temp_id, &version_major, 
        &version_minor, status);
    if (*status != rpc_s_ok)
    {
        return (false);
    }

    /*
     * Compare protocol versions, they are only 1 byte each.
     */
    if ((unsigned8) version_major != (unsigned8) tower_vers_major)
       /*
        * We don't do this so we can rev the minor protocol version.
        *  || ((unsigned8) version_minor >  (unsigned8) tower_vers_minor)
        */
    {
        return (false);
    }

    /*
     * Tower is compatible with the client's.
     */
    return (true);
}



/*
**++
**  ROUTINE NAME:       rpc__tower_ref_vec_free
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Releases memory associated with a tower reference vector, 
**  including the towers as well as the vector.
**
**  INPUTS:             none
**
**  INPUT/OUTPUTS:          
**
**      tower_vector    The tower vector to free. Nulled on return.
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

PRIVATE void rpc__tower_ref_vec_free 
(
    rpc_tower_ref_vector_p_t    *tower_vector,
    unsigned32                  *status
)
{
    unsigned32      i;


    CODING_ERROR (status);

    /*
     * Free the tower vector's lower network floors.
     */
    rpc__tower_free (&((*tower_vector)->lower_flrs), status);
    if (*status != rpc_s_ok)
    {
        return;
    }

    /*
     * Free each tower reference in the vector.  
     */
    for (i=0; i < (*tower_vector)->count; i++)
    {
        rpc__tower_ref_free (&((*tower_vector)->tower[i]), status);
        if (*status != rpc_s_ok)
        {
            return;
        }
    }

    /*
     * Free the tower vector structure and set pointer to NULL.
     */
    RPC_MEM_FREE (*tower_vector, RPC_C_MEM_TOWER_REF_VECTOR);
    *tower_vector = NULL;

    *status = rpc_s_ok;
    return;

}


/*
**++
**  ROUTINE NAME:       rpc__tower_ref_vec_from_binding
**
**  SCOPE:              PRIVATE - declared in comtwrref.h
**
**  DESCRIPTION:
**
**  Creates a vector of tower references from a single binding handle.
**  A separate tower reference is created for each data representation 
**  (transfer syntax) found in the interface specification.  
**  With the exception of the data representation floor, each of 
**  the returned towers in the vector is identical.
**
**  INPUTS:             
**
**      if_spec         Interface specification.
**      binding         Binding handle.
**
**  INPUT/OUTPUTS:      none.
**
**
**  OUTPUTS:
**
**      tower_vector    Returns a pointer to a tower reference vector.
**
**      status          Returns the status code from the tower-ref vector
**                      from binding operation. This status code is a value 
**                      that indicates whether the routine completed
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

PRIVATE void rpc__tower_ref_vec_from_binding 
(
    rpc_if_rep_p_t              if_spec,
    rpc_binding_handle_t        binding,
    rpc_tower_ref_vector_p_t    *tower_vector,
    unsigned32                  *status
)
{
    unsigned16              lower_flr_count;
    unsigned32              i,
                            temp_status;
    twr_p_t                 lower_floors;
    rpc_tower_ref_p_t       tower_copy;
    rpc_tower_floor_p_t     tower_floor;
    rpc_if_id_t             if_id;
    rpc_binding_rep_p_t     binding_rep;
    rpc_syntax_id_t         *if_syntax_id;


    CODING_ERROR (status);

    /*
     * Create the tower vector.
     */
    RPC_MEM_ALLOC ( 
        *tower_vector,
        rpc_tower_ref_vector_p_t,
        sizeof (rpc_tower_ref_vector_t) + 
        ((if_spec->syntax_vector.count -1) * (sizeof (rpc_tower_ref_p_t))) ,
        RPC_C_MEM_TOWER_REF_VECTOR,
        RPC_C_MEM_WAITOK );

    /*
     * Initialize tower count.
     */
    (*tower_vector)->count = 0;

    /*
     * Create the tower for the first transfer syntax 
     * in the interface specification.
     */

    /*
     * Obtain the rpc address of this binding.
     */
    binding_rep = (rpc_binding_rep_p_t) binding;

    /*
     * Create the lower tower floors.
     */
    rpc__naf_tower_flrs_from_addr (
        binding_rep->rpc_addr, &lower_floors, status);
    if (*status != rpc_s_ok)
    {
        RPC_MEM_FREE (*tower_vector, RPC_C_MEM_TOWER_REF_VECTOR);
        return;
    }

    /*
     * Initialize the tower vector with the pointer
     * to the lower floors.
     */
    (*tower_vector)->lower_flrs = lower_floors;

    /*
     * Get the number of lower tower floors returned and
     * convert to host's representation.
     */
    memcpy ((char *)&lower_flr_count, 
            (char *)lower_floors->tower_octet_string,
            RPC_C_TOWER_FLR_COUNT_SIZE);

    RPC_RESOLVE_ENDIAN_INT16 (lower_flr_count);

    /*
     * Allocate the tower reference structure to the first tower.
     * The number of floors is equal to the number of RPC (upper) floors
     * plus the number of network (lower) floors.
     */
    rpc__tower_ref_alloc (lower_floors->tower_octet_string, 
        RPC_C_NUM_RPC_FLOORS + lower_flr_count, 
        RPC_C_NUM_RPC_FLOORS+1, &((*tower_vector)->tower[0]), status);

    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    /*
     * Get the interface identifier and create tower floor 1.
     */
    rpc_if_inq_id ((rpc_if_handle_t) if_spec, &if_id, status);
    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    rpc__tower_flr_from_if_id (&if_id, &tower_floor, status);
    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    /*
     * Add floor 1 to the tower.
     */
    rpc__tower_ref_add_floor (1, tower_floor, (*tower_vector)->tower[0], status);
    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    /*
     * Create tower floor 2 from the transfer syntax from the ifspec.
     */
    if_syntax_id = if_spec->syntax_vector.syntax_id;
    rpc__tower_flr_from_drep (if_syntax_id, &tower_floor, status);
    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    /*
     * Add floor 2 to the tower.
     */
    rpc__tower_ref_add_floor (2, tower_floor, (*tower_vector)->tower[0], status);
    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    /*
     * Create tower floor 3 from the RPC protocol id.
     */
    rpc__tower_flr_from_rpc_prot_id (binding_rep->rpc_addr->rpc_protseq_id, 
        binding_rep->protocol_version, &tower_floor, status);
    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    /*
     * Add floor 3 to the tower.
     */
    rpc__tower_ref_add_floor (3, tower_floor, (*tower_vector)->tower[0], status);
    if (*status != rpc_s_ok)
    {
        goto CLEANUP;
    }

    /*
     * Increment the number of towers in the vector.
     */
    (*tower_vector)->count++;

    /*
     * Create the towers for the remaining transfer syntaxes
     * in the interface specification.
     */
    if_syntax_id++;
    for (i=1; i < if_spec->syntax_vector.count; 
         i++, if_syntax_id++, (*tower_vector)->count++)
    {
        /*
         * Copy the initial tower created.
         */
        rpc__tower_ref_copy ((*tower_vector)->tower[0], &tower_copy, status);
        if (*status != rpc_s_ok)
        {
            goto CLEANUP;
        }

        /*
         * Create floor 2 for this tower from the next transfer syntax.
         */
        rpc__tower_flr_from_drep (if_syntax_id, &tower_floor, status);
        if (*status != rpc_s_ok)
        {
            goto CLEANUP;
        }

        /*
         * Add floor 2 to the tower.
         */
        rpc__tower_ref_add_floor (2, tower_floor, tower_copy, status);
        if (*status != rpc_s_ok)
        {
            goto CLEANUP;
        }

        (*tower_vector)->tower[i] = tower_copy;
        

    }
        
CLEANUP:
    /*
     * If status is anything other than successful,
     * free the tower vector.
     */
    if (*status != rpc_s_ok)
    {
        rpc__tower_ref_vec_free (tower_vector, &temp_status);
    }

    return;

}
