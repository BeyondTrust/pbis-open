/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
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
**      COMTWRFLR.C
**
**  FACILITY:
**
**     Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**     Contains the routines necessary to create the individual tower
**     floors that formulate a protocol tower that is stored in the namespace.
**
**
*/

/*
 * Include files
 */
#include <commonp.h>    /* Private defs for Common component            */
#include <com.h>        /* Private COM defs for other RPC components    */
#include <comp.h>       /* Privates for COM Services component          */
#include <comtwrflr.h>  /* Private defs for this module                 */


/*
**++
**  ROUTINE NAME:           rpc__tower_flr_free
**
**  SCOPE:                  PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Frees the tower floor.
**
**  INPUTS:                 none
**
**  INPUT/OUTPUTS:          
**
**      floor               The tower floor to be freed.
**                          Nulled on output.
**  OUTPUTS:
**
**      status              Returns the status code from the floor free
**                          operation.  This status code is a value 
**                          that inidicates whether the routine completed 
**                          successfully and, if not,
**                          why.  Returns:
**                              rpc_s_ok
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

PRIVATE void rpc__tower_flr_free 
(
    rpc_tower_floor_p_t     *floor,
    unsigned32              *status
)
{
    CODING_ERROR (status);

    /*
     * Free the tower floor, freeing the octet string associated
     * with the each floor only if the free flag is set.
     */
    if ((*floor)->free_twr_octet_flag)
    {
        RPC_MEM_FREE ((*floor)->octet_string, 
                      RPC_C_MEM_TOWER_FLOOR_OCTET);
    }

    RPC_MEM_FREE (*floor, RPC_C_MEM_TOWER_FLOOR);
    *floor = NULL;

    /*
     * Return the status of the floor free routine.
     */
    *status = rpc_s_ok;
    return;
}


/*
**++
**  ROUTINE NAME:           rpc__tower_flr_from_drep
**
**  SCOPE:                  PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Creates the RPC-specific tower floor for data representation (floor 2).
**
**  INPUTS:
**
**      transfer_syntax     Data representation (transfer syntax) to 
**                          convert to a tower floor.
**
**  INPUT/OUTPUTS:          none
**
**  OUTPUTS:
**
**      floor               The returned tower floor.
**  
**      status              Returns the status code from the data
**                          representation to floor operation.  This
**                          status code is a value that inidicates whether
**                          the routine completed successfully and, if not,
**                          why.  Returns:
**                              status of the rpc__tower_flr_from_uuid routine.
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

PRIVATE void rpc__tower_flr_from_drep 
(
    rpc_syntax_id_p_t       transfer_syntax,
    rpc_tower_floor_p_t     *floor,
    unsigned32              *status
)
{
    CODING_ERROR (status);

    /*
     * Get the floor from the tower floor from uuid routine, passing 
     * it the uuid and version number of the transfer syntax.
     */
    rpc__tower_flr_from_uuid (&(transfer_syntax->id), 
                              (transfer_syntax->version) & 0x0000FFFF,
                              (transfer_syntax->version) >> 16,
                              floor,
                              status);
    /*
     * Return the status of the tower floor from uuid routine.
     */
    return;
}


/*
**++
**  ROUTINE NAME:           rpc__tower_flr_from_if_id
**
**  SCOPE:                  PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Creates the RPC-specific tower floor for the interface id (floor 1).
**
**  INPUTS:
**
**      if_id               The interface identifier to convert to
**                          a tower floor.
**
**  INPUT/OUTPUTS:          none
**
**  OUTPUTS:
**
**      floor               The returned tower floor.
**  
**      status              Returns the status code from the interface id
**                          to floor operation.  This status code is a 
**                          value that inidicates whether the routine 
**                          completed successfully and, if not, why.
**                          Returns:
**                              status of the rpc__tower_flr_from_uuid routine.
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

PRIVATE void rpc__tower_flr_from_if_id 
(
    rpc_if_id_p_t           if_id,
    rpc_tower_floor_p_t     *floor,
    unsigned32              *status
)
{

    CODING_ERROR (status);

    /*
     * Get the floor from tower floor from uuid routine, passing 
     * it the uuid and version number of the interface identifier.
     */
    rpc__tower_flr_from_uuid (&(if_id->uuid), 
                              if_id->vers_major,
                              if_id->vers_minor,
                              floor,
                              status);
    /*
     * Return the status of the tower floor from uuid routine.
     */
    return;

}


/*
**++
**  ROUTINE NAME:           rpc__tower_flr_from_rpc_prot_id
**
**  SCOPE:                  PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Creates the RPC-specific tower floor for the RPC Protocol (floor 3).
**  The returned floor is encoded for storage in DNS Towers.
**
**  INPUTS:
**
**      rpc_protseq_id      The RPC protocol sequence id from which the
**                          tower protocol id and version is obtained 
**                          to place in the tower floor.
**
**      protocol_version    The RPC protocol version to place in the tower.
**
**  INPUT/OUTPUTS:          none
**
**  OUTPUTS:
**
**      floor               The returned tower floor.
**  
**      status              Returns the status code from the protocol id
**                          to floor operation.  This status code is a 
**                          value that indicates whether the routine 
**                          completed successfully and, if not, why.
**                          Returns:
**                              rpc_s_ok
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

PRIVATE void rpc__tower_flr_from_rpc_prot_id 
(
    rpc_protseq_id_t        rpc_protseq_id,
    rpc_protocol_version_p_t protocol_version,
    rpc_tower_floor_p_t     *floor,
    unsigned32              *status
)
{
    unsigned8       tower_protocol_id;
    unsigned16      prot_id_size,
                    address_size,
                    tower_vers_minor,
                    twr_rep_16;
    unsigned32      floor_size,
                    version_major,
                    version_minor;

    CODING_ERROR (status);

    /*
     * Allocate the tower floor structure.
     */
    RPC_MEM_ALLOC (
        *floor, rpc_tower_floor_p_t, sizeof (rpc_tower_floor_t),
       RPC_C_MEM_TOWER_FLOOR, RPC_C_MEM_WAITOK);

    /*
     * Calculate the actual size of the tower floor
     * and allocate.
     */
    prot_id_size = RPC_C_TOWER_PROT_ID_SIZE;
    address_size = RPC_C_TOWER_VERSION_SIZE;
    floor_size = RPC_C_TOWER_FLR_LHS_COUNT_SIZE +       /* lhs count */
                 prot_id_size                   +       /* protocol id */
                 RPC_C_TOWER_FLR_RHS_COUNT_SIZE +       /* rhs count */
                 address_size;                          /* minor version */

    RPC_MEM_ALLOC (
        (*floor)->octet_string, byte_p_t, floor_size,
        RPC_C_MEM_TOWER_FLOOR_OCTET, RPC_C_MEM_WAITOK);

    /*
     * Initialize the tower floor fields.
     */
    (*floor)->free_twr_octet_flag = true;
    (*floor)->prot_id_count = prot_id_size;
    (*floor)->address_count = address_size;

    /*
     * Initialize the tower floor's octet string.
     */

    /*
     * Convert the prot_id_count to little endian 
     * representation and copy to the octet string.
     */
    twr_rep_16 = (*floor)->prot_id_count;
    RPC_RESOLVE_ENDIAN_INT16 (twr_rep_16);
    memcpy ((char *)RPC_PROT_ID_COUNT (*floor), (char *)&twr_rep_16, 
            RPC_C_TOWER_FLR_LHS_COUNT_SIZE);

    
    /*
     * Obtain the RPC tower protocol id and minor version number
     */
    rpc__network_inq_prot_version (rpc_protseq_id,  &tower_protocol_id,
        &version_major, &version_minor, status);

    if (*status != rpc_s_ok)
    {
        RPC_MEM_FREE ((*floor)->octet_string, RPC_C_MEM_TOWER_FLOOR_OCTET);
        RPC_MEM_FREE (*floor, RPC_C_MEM_TOWER_FLOOR);
        return;
    }

    /*
     * Copy the RPC tower protocol id to the octet string.
     */
    memcpy ((char *)RPC_PROT_ID_START (*floor), 
            (char *)&tower_protocol_id, (*floor)->prot_id_count);

    /*
     * Convert the address_count (rhs) to little endian 
     * representation and copy to the octet string.
     */
    twr_rep_16 = (*floor)->address_count;
    RPC_RESOLVE_ENDIAN_INT16 (twr_rep_16);
    memcpy ((char *)RPC_ADDRESS_COUNT (*floor), (char *)&twr_rep_16, 
            RPC_C_TOWER_FLR_RHS_COUNT_SIZE);


    /*
     * Copy the RPC protocol minor version to the octet string,
     * after converting to little endian representation.
     *
     * Note, we do not need to store the major version of the
     * protocol in the tower.  If a major revision is made to the
     * protocol, the architecture will define a new tower protocol id,
     * since it is a new protocol in itself.
     *
     * If we are provided with a minor version, use it instead of
     * our hardwired value.
     */
    if (protocol_version != NULL)
    {
        tower_vers_minor = (unsigned16) protocol_version->minor_version;
    }
    else
    {
        tower_vers_minor = (unsigned16) version_minor;
    }
    RPC_RESOLVE_ENDIAN_INT16 (tower_vers_minor);
    memcpy ((char *)RPC_ADDRESS_START (*floor), (char *)&tower_vers_minor,
            (*floor)->address_count);

    *status = rpc_s_ok;
    return;
}


/*
**++
**  ROUTINE NAME:           rpc__tower_flr_from_uuid
**
**  SCOPE:                  PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Creates the RPC-specific tower floor for UUIDs (floors 1 and 2).
**
**  INPUTS:
**
**      uuid                The uuid to place in the protocol
**                          identifier field of the tower floor.
**
**      version_major       The major version number to place in the 
**                          protocol identifier field of the tower floor.
**
**      version_minor       The minor version number to place in the 
**                          protocol identifier field of the tower floor.
**
**  INPUT/OUTPUTS:          none
**
**  OUTPUTS:
**
**      floor               The returned tower floor.
**  
**      status              Returns the status code from the UUID 
**                          to floor operation.  This status code is a 
**                          value that indicates whether the routine 
**                          completed successfully and, if not, why.
**                          Returns:
**                              rpc_s_ok
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

PRIVATE void rpc__tower_flr_from_uuid 
(
    dce_uuid_p_t                uuid,
    unsigned32              version_major,
    unsigned32              version_minor,
    rpc_tower_floor_p_t     *floor,
    unsigned32              *status
)
{
    byte_p_t        prot_id;
    unsigned16      address_size,
                    tower_vers_minor,
                    twr_rep_16;
    unsigned32      floor_size,
                    prot_id_len;

    CODING_ERROR (status);

    /*
     * Encode the uuid and major version number into the 
     * protocol identifier (lhs) for storage in the tower floor.
     */
    rpc__tower_flr_id_from_uuid 
        (uuid, version_major, &prot_id_len, &prot_id, status);

    if (*status != rpc_s_ok)
    {
        return;
    }

    /*
     * Allocate the tower floor structure.
     */
    RPC_MEM_ALLOC (
        *floor, rpc_tower_floor_p_t, sizeof (rpc_tower_floor_t),
       RPC_C_MEM_TOWER_FLOOR, RPC_C_MEM_WAITOK);

    /*
     * Calculate the actual size of the tower floor
     * and allocate.
     */
    address_size = RPC_C_TOWER_VERSION_SIZE;
    floor_size = RPC_C_TOWER_FLR_LHS_COUNT_SIZE +       /* lhs count */
                 prot_id_len                    +       /* protocol id */
                 RPC_C_TOWER_FLR_RHS_COUNT_SIZE +       /* rhs count */
                 address_size;                          /* minor version */

    RPC_MEM_ALLOC (
        (*floor)->octet_string, byte_p_t, floor_size,
        RPC_C_MEM_TOWER_FLOOR_OCTET, RPC_C_MEM_WAITOK);

    /*
     * Initialize the tower floor fields.
     */
    (*floor)->free_twr_octet_flag = true;
    (*floor)->prot_id_count = (unsigned16) prot_id_len;
    (*floor)->address_count = address_size;

    /*
     * Initialize the tower floor's octet string.
     */

    /*
     * Convert the prot_id count to to little endian 
     * representationt and copy to the octet string.
     */
    twr_rep_16 = (*floor)->prot_id_count;
    RPC_RESOLVE_ENDIAN_INT16 (twr_rep_16);
    memcpy ((char *) RPC_PROT_ID_COUNT (*floor), 
            (char *) &twr_rep_16, 
            RPC_C_TOWER_FLR_LHS_COUNT_SIZE);


    /*
     * Copy the encoded protocol identifier to the octet string.
     * It's already in little endian frm rpc__tower_flr_id_from_uuid.
     */
    memcpy ((char *)RPC_PROT_ID_START (*floor), (char *)prot_id,
            prot_id_len);

    /*
     * Free the protocol identifier now that we are done with it.
     */
    RPC_MEM_FREE (prot_id, RPC_C_MEM_TOWER_FLOOR_ID);

    /*
     * Convert the address count to little endian 
     * representation and copy to the octet string.
     */
    twr_rep_16 = (*floor)->address_count;
    RPC_RESOLVE_ENDIAN_INT16 (twr_rep_16);
    memcpy ((char *)RPC_ADDRESS_COUNT (*floor), (char *)&twr_rep_16, 
            RPC_C_TOWER_FLR_RHS_COUNT_SIZE);


    /*
     * Copy the RPC protocol minor version to the octet string,
     * after converting to little endian representation.
     */
    tower_vers_minor = (unsigned16) version_minor;
    RPC_RESOLVE_ENDIAN_INT16 (tower_vers_minor);
    memcpy ((char *)RPC_ADDRESS_START (*floor), (char *)&tower_vers_minor,
            (*floor)->address_count);

    *status = rpc_s_ok;
    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_flr_id_from_uuid
**
**  SCOPE:              PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Creates a tower floor protocol identifier from a UUID 
**  and major version for storage in a tower floor.
**
**  INPUTS:
**
**      uuid            Pointer to the UUID to encode into the 
**                      tower floor protocol identifier.
**
**      version_major   Major version number to encode into the 
**                      tower floor protocol identifier.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**      
**      prot_id_len     Returns the length of the tower floor protocol id.
**
**      prot_id         Returns the tower floor protocol identifier.
**
**      status          Returns the status code from the protocol id
**                      from uuid operation.  This status code is a 
**                      value that indicates whether the routine 
**                      completed successfully and, if not, why.
**                      Returns:
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

PRIVATE void rpc__tower_flr_id_from_uuid 
(
    dce_uuid_p_t        uuid,
    unsigned32      version_major,
    unsigned32      *prot_id_len,
    byte_p_t        *prot_id,
    unsigned32      *status
)
{

    byte_t          prot_id_prefix = RPC_C_PROT_ID_PREFIX,
                    *prot_id_p;
    unsigned16      tower_vers_major;
    dce_uuid_t          tower_uuid;


    CODING_ERROR (status);

    /*
     * Calculate the length of the returned tower floor
     * protocol id.
     */
    *prot_id_len = 
        RPC_C_TOWER_PROT_ID_SIZE +        /* protocol id prefix (0x0D) */
        RPC_C_TOWER_UUID_SIZE +           /* UUID of interest          */
        RPC_C_TOWER_VERSION_SIZE;         /* major version number      */

    /*
     * Allocate the protocol identifier.
     */
    RPC_MEM_ALLOC (
        *prot_id, byte_p_t, *prot_id_len,
        RPC_C_MEM_TOWER_FLOOR_ID, RPC_C_MEM_WAITOK);

    prot_id_p = *prot_id;

    /*
     * Copy the RPC protocol id prefix.
     * One byte, no need to convert.
     */
    memcpy ((char *)prot_id_p, (char *)&prot_id_prefix, 
        RPC_C_TOWER_PROT_ID_SIZE);
    prot_id_p++;

    /*
     * Convert the UUID of interest to little endian and copy it.
     */
    tower_uuid = *uuid;
    RPC_RESOLVE_ENDIAN_UUID (tower_uuid);

    memcpy ((char *)prot_id_p, (char *)&tower_uuid, RPC_C_TOWER_UUID_SIZE);
    prot_id_p += RPC_C_TOWER_UUID_SIZE;

    /*
     * Copy the major version and convert to little endian.
     */
    tower_vers_major = (unsigned16) version_major;
    RPC_RESOLVE_ENDIAN_INT16 (tower_vers_major);
    memcpy ((char *)prot_id_p, (char *)&tower_vers_major, 
        RPC_C_TOWER_VERSION_SIZE);

    *status = rpc_s_ok;
    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_flr_id_to_uuid
**
**  SCOPE:              PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Converts a tower floor protocol identifier to a UUID 
**  and major version number.
**
**  INPUTS:
**
**      prot_id         Protocol identifier to decode into a UUID
**                      a major version number.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**      
**      uuid            Returned UUID.
**
**      version_major   Returned major version number.
**
**      status          Returns the status code from the protocol id
**                      to uuid operation.  This status code is a 
**                      value that indicates whether the routine 
**                      completed successfully and, if not, why.
**                      Returns:
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

PRIVATE void rpc__tower_flr_id_to_uuid 
(
    byte_p_t        prot_id,
    dce_uuid_t          *uuid,
    unsigned32      *version_major,
    unsigned32      *status
)
{
    byte_t          prot_id_prefix,
                    *prot_id_p;
    unsigned16      flr_vers_major;

    CODING_ERROR (status);

    /*
     * Copy the pointer to the protocol identifier.
     */
    prot_id_p = prot_id;

    /*
     * Copy the protocol id prefix and compare with the expected value.
     * One byte, no conversion necessary.
     */
    memcpy ((char *)&prot_id_prefix, (char *)prot_id_p, 
        RPC_C_TOWER_PROT_ID_SIZE);

    if (prot_id_prefix != RPC_C_PROT_ID_PREFIX)
    {
        *status = rpc_s_invalid_rpc_floor;
    }

    prot_id_p++;

    /*
     * Copy the uuid and resolve to host's endian.
     */
    memcpy ((char *)uuid, (char *)prot_id_p, RPC_C_TOWER_UUID_SIZE);
    RPC_RESOLVE_ENDIAN_UUID (*uuid);

    prot_id_p += RPC_C_TOWER_UUID_SIZE;

    /*
     * Copy the major version and resolve to host's endian.
     */
    memcpy ((char *)&flr_vers_major, (char *)prot_id_p, 
        RPC_C_TOWER_VERSION_SIZE);
    RPC_RESOLVE_ENDIAN_INT16 (flr_vers_major);

    *version_major = (unsigned32) flr_vers_major;

    *status = rpc_s_ok;
    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_flr_to_drep
**
**  SCOPE:              PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Converts a tower floor to a data representation (transfer syntax)
**  compatible with the caller's host architecture.
**
**  INPUTS:
**
**      floor           The tower floor to convert to a data representation
**                      (transfer syntax).
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**
**      transfer_syntax The returned transfer syntax.
**
**      status          Returns the status code from the floor to data 
**                      representation operation.  This status code is a
**                      value that indicates whether the routine completed
**                      successfully and, if not, why.  Returns:
**                        status of the tower floor to uuid routine.
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

PRIVATE void rpc__tower_flr_to_drep 
(
    rpc_tower_floor_p_t     floor,
    rpc_syntax_id_t         *transfer_syntax,
    unsigned32              *status
)
{
    unsigned32      version_major,
                    version_minor;


    CODING_ERROR (status);

    /*
     * Convert the floor to a data representation by calling the
     * floor to uuid operation.
     */
    rpc__tower_flr_to_uuid (floor, &(transfer_syntax->id),
                            &version_major, &version_minor, status);

    /*
     * Place the two version numbers into a single unsigned32 
     * version for the transfer syntax.
     */
     transfer_syntax->version = version_minor << 16;
     transfer_syntax->version |= version_major;

    /*
     * Return the status of the tower floor to uuid operation.
     */

    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_flr_to_if_id
**
**  SCOPE:              PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Converts a tower floor to an interface identifer compatible
**  with the caller's host architecture.
**
**  INPUTS:
**
**      floor           The tower floor to convert to an interface id.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**
**      if_id           The returned interface identifier.
**      status          Returns the status code from the floor to if id
**                      operation.  This status code is a value that
**                      indicates whether the routine completed successfully
**                      and, if not, why.  Returns:
**                        status from the tower floor to uuid routine.
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

PRIVATE void rpc__tower_flr_to_if_id 
(
    rpc_tower_floor_p_t     floor,
    rpc_if_id_t             *if_id,
    unsigned32              *status
)
{
    unsigned32          version_major,
                        version_minor;


    CODING_ERROR (status);

    /*
     * Call the tower floor to uuid routine to convert the
     * this floor to an if-id.
     */
    rpc__tower_flr_to_uuid (floor, &(if_id->uuid), &version_major,
                            &version_minor, status);

    if_id->vers_major = (unsigned16) version_major;
    if_id->vers_minor = (unsigned16) version_minor;

    /*
     * Return the status of the tower floor to uuid operation.
     */
    return;

}


/*
**++
**  ROUTINE NAME:       rpc__tower_flr_to_rpc_prot_id
**
**  SCOPE:              PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Converts a tower floor to an RPC protocol identifier compatible
**  with the caller's host architecture.
**
**  INPUTS:
**
**      floor           The tower floor to convert to a RPC protocol id.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**
**      rpc_protocol_id The returned RPC protocol identifier.
**
**      version_major   Returned RPC protocol major version number.
**
**      version_minor   Returned RPC protocol minor version number.
**
**      status          Returns the status code from the floor to protocol id
**                      operation.  This status code is a value that
**                      indicates whether the routine completed successfully
**                      and, if not, why.  Returns:
**                        rpc_s_ok
**                        rpc_s_invalid_rpc_protid
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

PRIVATE void rpc__tower_flr_to_rpc_prot_id 
(
    rpc_tower_floor_p_t     floor,
    rpc_protocol_id_t       *rpc_protocol_id,
    unsigned32              *version_major,
    unsigned32              *version_minor,
    unsigned32              *status
)
{
    boolean             match;
    rpc_protseq_id_t    protseq_id;
    unsigned32          i,
                        temp_vers_minor;
    unsigned16          prot_vers_minor;
    unsigned8           network_prot_id;


    CODING_ERROR (status);

    /*
     * For each possible RPC protocol sequence
     * find the protocol id that matches the tower's.
     */
    for (i=0, match = false; i < RPC_C_PROTSEQ_ID_MAX; i++)
    {

        /*
         * Get the RPC protocol id for this RPC
         * protocol sequence.
         */
        protseq_id = RPC_PROTSEQ_INQ_PROTSEQ_ID (i);

        /*
         * Get the network's 'tower' protocol id for this 
         * protocol sequence and return its major version.
         */
        rpc__network_inq_prot_version (protseq_id, &network_prot_id, 
            version_major, &temp_vers_minor, status);

        /*
         * Ignore protocol sequences not supported by this host
         * and continue with next protocol sequence.
         */
        if (*status == rpc_s_protseq_not_supported)
        {
            continue;
        }

        /*
         * Return on any other error.
         */
        if (*status != rpc_s_ok)
        {
            return;
        }

        /*
         * Obtain the tower's protocol id and see if it 
         * matches the network's tower protocol id for this
         * protocol sequence.  
         *
         * Note: protocol id is only one byte so no need to 
         * memcpy or convert to host representation.
         */
        if ((unsigned8) *(RPC_PROT_ID_START(floor)) == 
            (unsigned8) network_prot_id)
        {
            /*
             * Return the protocol identifier.
             */
            *rpc_protocol_id = RPC_PROTSEQ_INQ_PROT_ID (i);

            match = true;
            break;
        }
    }

    if (match)
    {
        /*
         * Obtain the protocol id's minor version
         * from the tower and convert to the host's
         * endian representation.
         */
        memcpy ((char *)&prot_vers_minor, (char *)RPC_ADDRESS_START (floor),
                floor->address_count);
        RPC_RESOLVE_ENDIAN_INT16 (prot_vers_minor);

        /*
         * Return the protocol id's minor version and
         * set status to success.
         */
        *version_minor = (unsigned32) prot_vers_minor;

        *status = rpc_s_ok;
    }
    else
    {
        /*
         * No match found on the tower's protocol id.
         */
        *status = rpc_s_invalid_rpc_protid;
    }

    return;
}


/*
**++
**  ROUTINE NAME:       rpc__tower_flr_to_uuid
**
**  SCOPE:              PRIVATE - declared in comtwrflr.h
**
**  DESCRIPTION:
**
**  Converts a tower floor to UUID and version numbers.
**
**  INPUTS:
**
**      floor           The tower floor to convert to a UUID and 
**                      version numbers.
**
**  INPUT/OUTPUTS:      none
**
**  OUTPUTS:
**
**      uuid            Returned UUID.
**
**      version_major   Returned major version number.
**
**      version_minor   Returned minor version number.
**
**      status          Returns the status code from the floor to protocol id
**                      operation.  This status code is a value that
**                      indicates whether the routine completed successfully
**                      and, if not, why.  Returns:
**                        rpc_s_ok
**                        status from a called routine
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

PRIVATE void rpc__tower_flr_to_uuid 
(
    rpc_tower_floor_p_t     floor,
    dce_uuid_t                  *uuid,
    unsigned32              *version_major,
    unsigned32              *version_minor,
    unsigned32              *status
)
{
    unsigned16          flr_vers_minor;

    CODING_ERROR (status);

    /*
     * Decode the protocol identifier information into the uuid
     * and major version number compatible with the host architecture.
     */
    rpc__tower_flr_id_to_uuid ((byte_p_t)RPC_PROT_ID_START (floor), 
        uuid, version_major, status);

    if (*status != rpc_s_ok)
    {
        return;
    }

    /*
     * Obtain the address information (right hand side) from the 
     * tower floor and convert to host's endian.
     */
    memcpy ((char *)&flr_vers_minor, (char *)RPC_ADDRESS_START (floor),
            floor->address_count);

    RPC_RESOLVE_ENDIAN_INT16 (flr_vers_minor);

    *version_minor = (unsigned32) flr_vers_minor;

    *status = rpc_s_ok;

    return;
}

