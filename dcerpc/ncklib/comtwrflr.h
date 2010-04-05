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
#ifndef _COMTWRFLR_H
#define _COMTWRFLR_H 1

/*
**
**  NAME
**
**      comtwrflr.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Contains private definitions and prototypes of the 
**      comtwrflr.c module.
**
**
*/

/*
 * Constants
 */

/*
 * The architecturally defined tower floor protocol identifier 
 * prefix to signify the succeeding data as an RPC uuid.
 */
#define RPC_C_PROT_ID_PREFIX    (0x0D)

/*
 * Prototypes
 */

#ifndef _DCE_PROTOTYPE_
#include <dce/dce.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

PRIVATE void rpc__tower_flr_free _DCE_PROTOTYPE_ ((
    rpc_tower_floor_p_t     * /*floor*/,
    unsigned32              * /*status*/ 
));

PRIVATE void rpc__tower_flr_from_drep _DCE_PROTOTYPE_ ((
    rpc_syntax_id_p_t        /*transfer_syntax*/,
    rpc_tower_floor_p_t     * /*floor*/,
    unsigned32              * /*status*/ 
));

PRIVATE void rpc__tower_flr_from_if_id _DCE_PROTOTYPE_ ((
    rpc_if_id_p_t            /*if_id*/,
    rpc_tower_floor_p_t     * /*floor*/,
    unsigned32              * /*status*/ 
));

PRIVATE void rpc__tower_flr_from_rpc_prot_id _DCE_PROTOTYPE_ ((
    rpc_protseq_id_t         /*rpc_protseq_id*/,
    rpc_protocol_version_p_t /*protocol_version*/,
    rpc_tower_floor_p_t     * /*floor*/,
    unsigned32              * /*status*/ 
));

PRIVATE void rpc__tower_flr_from_uuid _DCE_PROTOTYPE_ ((
    uuid_p_t                 /*uuid*/,
    unsigned32               /*version_major*/,
    unsigned32               /*version_minor*/,
    rpc_tower_floor_p_t     * /*floor*/,
    unsigned32              * /*status*/ 
));

PRIVATE void rpc__tower_flr_id_from_uuid _DCE_PROTOTYPE_ ((
    uuid_p_t         /*uuid*/,
    unsigned32       /*version_major*/,
    unsigned32      * /*prot_id_len*/,
    unsigned8       ** /*prot_id*/,
    unsigned32      * /*status*/ 
));

PRIVATE void rpc__tower_flr_id_to_uuid _DCE_PROTOTYPE_ ((
    unsigned8       * /*prot_id*/,
    uuid_t          * /*uuid*/,
    unsigned32      * /*version_major*/,
    unsigned32      * /*status*/ 
));

PRIVATE void rpc__tower_flr_to_drep _DCE_PROTOTYPE_ ((
    rpc_tower_floor_p_t      /*floor*/,
    rpc_syntax_id_t         * /*transfer_syntax*/,
    unsigned32              * /*status*/ 
));

PRIVATE void rpc__tower_flr_to_if_id _DCE_PROTOTYPE_ ((
    rpc_tower_floor_p_t      /*floor*/,
    rpc_if_id_t             * /*if_id*/,
    unsigned32              * /*status*/ 
));

PRIVATE void rpc__tower_flr_to_rpc_prot_id _DCE_PROTOTYPE_ ((
    rpc_tower_floor_p_t      /*floor*/,
    rpc_protocol_id_t       * /*rpc_protocol_id*/,
    unsigned32              * /*version_major*/,
    unsigned32              * /*version_minor*/,
    unsigned32              * /*status*/ 
));

PRIVATE void rpc__tower_flr_to_uuid _DCE_PROTOTYPE_ ((
    rpc_tower_floor_p_t      /*floor*/,
    uuid_t                  * /*uuid*/,
    unsigned32              * /*version_major*/,
    unsigned32              * /*version_minor*/,
    unsigned32              * /*status*/ 
));


#endif /* _COMTWRFLR_H */
