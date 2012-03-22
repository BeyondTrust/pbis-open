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
#ifndef _COMP_H
#define _COMP_H
/*
**
**  NAME
**
**      comp.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types/constants internal to the Common Communications 
**  Service component of the RPC runtime.
**
**
*/


/***********************************************************************/

#include <dce/dce.h>

#include <comprot.h>    /* Externals for Protocol Services sub-component*/
#include <comnaf.h>     /* Externals for NAF Services sub-component     */
#include <comauth.h>    /* Externals for Auth. Services sub-component   */

/*
 * Accessor macros for the RPC Protocol Sequence ID table.
 */


#define RPC_PROTSEQ_INQ_SUPPORTED(id) \
       (boolean)rpc_g_protseq_id[id].supported
#define RPC_PROTSEQ_INQ_PROTSEQ_ID(id)      rpc_g_protseq_id[id].rpc_protseq_id
#define RPC_PROTSEQ_INQ_PROT_ID(id)         rpc_g_protseq_id[id].rpc_protocol_id
#define RPC_PROTSEQ_INQ_NAF_ID(id)          rpc_g_protseq_id[id].naf_id
#define RPC_PROTSEQ_INQ_PROTSEQ(id)         rpc_g_protseq_id[id].rpc_protseq
#define RPC_PROTSEQ_INQ_NET_IF_ID(id)       rpc_g_protseq_id[id].network_if_id


/***********************************************************************/
/*
 * R P C _ P R O T O C O L _ I D _ E L T _ T
 *
 * The RPC Protocol ID table element structure.  An element describes
 * a single RPC Protocol.
 * 
 * The fields are:
 *
 *      prot_init       The address of an initialization routine in the
 *                      Protocol Service that will be called by rpc__init.
 * 
 *      prot_fork_handler  The address of a routine to call to handle 
 *                      protocol specific, fork-related processing.
 *
 *      rpc_protocol_id A constant identifier for this RPC Protocol.
 *      
 *      call_epv        An entry point vector for the Call Services in
 *                      the Protocol Service.
 *      
 *      mgmt_epv        An entry point vector for the Management Services in 
 *                      the Protocol Service.
 *      
 *      binding_epv     An entry point vector for the Binding Services
 *                      in the Protocol Service.
 *      
 *      network_epv     An entry point vector for the Network Services
 *                      in the Protocol Service.
 */
typedef struct
{
    rpc_prot_init_fn_t          prot_init;
    rpc_prot_fork_handler_fn_t  prot_fork_handler;
    rpc_protocol_id_t           rpc_protocol_id;
    rpc_prot_call_epv_t         *call_epv;
    rpc_prot_mgmt_epv_t         *mgmt_epv;
    rpc_prot_binding_epv_t      *binding_epv;
    rpc_prot_network_epv_t      *network_epv;
} rpc_protocol_id_elt_t, *rpc_protocol_id_elt_p_t;

/*
 * Accessor macros for the RPC Protocol ID table.
 */
#define RPC_PROTOCOL_INQ_SUPPORTED(id) \
      (rpc_g_protocol_id[id].prot_init != NULL)
#define RPC_PROTOCOL_INQ_CALL_EPV(id)       rpc_g_protocol_id[id].call_epv
#define RPC_PROTOCOL_INQ_MGMT_EPV(id)       rpc_g_protocol_id[id].mgmt_epv
#define RPC_PROTOCOL_INQ_BINDING_EPV(id)    rpc_g_protocol_id[id].binding_epv
#define RPC_PROTOCOL_INQ_NETWORK_EPV(id)    rpc_g_protocol_id[id].network_epv

/***********************************************************************/
/*
 * R P C _ G _ P R O T O C O L _ I D
 *
 * The RPC Protocol ID table.  Each RPC Protocol has an entry in this
 * table.  This table is index by RPC Protocol ID.
 *
 * Note that the ".rpc_protocol_id" field of i'th element in the table
 * is always "i".  While redundant, this is useful so that you can pass
 * pointers to individual table elements.
 */
EXTERNAL rpc_protocol_id_elt_t   rpc_g_protocol_id[];

/*
 * Protocol Sequence ID Table
 * 
 * This table contains the valid combination of protocol ids
 * for upper floor 3 and the lower tower floors.
 * This table maps each combination to the appropriate 
 * RPC protocol id sequence.  
 *
 * The field num_floors provides for the number of significant
 * floors comprising the RPC protocol sequence.
 * This table is used only by comtwrref.c to validate towers, but we need
 * it accessible by the dynamic loading code so that new modules can register
 * with the runtime.
 * We allocate extra space, since there may be mulitple entries: eg
 * RPC_C_PROTSEQ_ID_NCACN_OSI_DNA
 * */
typedef struct
{
    unsigned8               prefix;
    dce_uuid_t                  uuid;
} rpc_flr_prot_id_t, *rpc_flr_prot_id_p_t;

typedef struct
{
    rpc_protseq_id_t        rpc_protseq_id;
    unsigned8               num_floors;
    rpc_flr_prot_id_t       floor_prot_ids[RPC_C_MAX_NUM_NETWORK_FLOORS + 1];
} rpc_tower_prot_ids_t, *rpc_tower_prot_ids_p_t;
EXTERNAL rpc_tower_prot_ids_t rpc_g_tower_prot_ids[RPC_C_PROTSEQ_ID_MAX*2];
EXTERNAL unsigned32 rpc_g_tower_prot_id_number;	/* number of elts in rpc_g_tower_prot_ids */



/***********************************************************************/
/*
 * R P C _ N A F _ I D _ E L T _ T
 *
 * The Network Address Family ID table element structure.  An element
 * describes a single Network Address Family Extension.
 * 
 * The fields are:
 *
 *      naf_init        The address of an initialization routine in the
 *                      NAF Service that will be called by rpc__init
 *
 *      naf_id          A constant identifier for this NAF.
 *
 *      net_if_id       A constant identifier for the network interface
 *                      type used in the NAF initialization routine (when
 *                      determining if this NAF is supported).
 *      
 *      naf_epv         An entry point vector for the NAF Service.
 */
typedef struct
{
    rpc_naf_init_fn_t        naf_init;
    rpc_naf_id_t             naf_id;
    rpc_network_if_id_t      network_if_id;
    rpc_naf_epv_t            *epv;
} rpc_naf_id_elt_t, *rpc_naf_id_elt_p_t;

/*
 * Accessor macros for the Network Address Family ID table.
 */
#define RPC_NAF_INQ_SUPPORTED(id)       (rpc_g_naf_id[id].naf_init != NULL)
#define RPC_NAF_INQ_EPV(id)             rpc_g_naf_id[id].epv

/***********************************************************************/
/*
 * R P C _ G _ N A F _ I D
 *
 * The Network Address Family ID table.  This table is indexed by a NAF
 * ID.
 *
 * Each Network Address Family Extension has an entry in this table.
 * Note that this is a sparse table because it uses the Unix Address
 * Family ID's as NAF ID's.
 *
 * Note that the ".naf_id" field of i'th element in the table is always
 * "i".  While redundant, this is useful so that you can pass pointers
 * to individual table elements.
 */
EXTERNAL rpc_naf_id_elt_t   rpc_g_naf_id[RPC_C_NAF_ID_MAX];

/***********************************************************************/
/*
 * R P C _ A U T H N _ P R O T O C O L _ I D _ E L T _ T
 *
 * The RPC Authentication Protocol ID table element structure.
 * 
 * The fields are:
 *
 *      auth_init           The address of an initialization routine in the
 *                          Authentication Service that will be called by 
 *                          rpc__init.
 *
 *      authn_protocol_id   A constant identifier for this Authentication Service.
 *
 *      dce_rpc_authn_protocol_id_t
 *                          The value that goes into RPC protocol messages to
 *                          identify which authentication protocol is in use.
 *
 *      epv                 An entry point vector for the Authentication Service
 *                          functions.
 *      rpc_prot_epv_tbl    A table, indexed by RPC protocol ID,
 *                          containing that RPC protocol's specific
 *                          interface to the Authentication Service.
 *
 * Note that the ".auth_protocol_id" contains API values (see
 * "rpc_c_authn_..." constants in "rpc.idl").
 * "dce_rpc_authn_protocol_id_t" contains architectural values that appear
 * in network messages (see "dce_c_rpc_authn_protocol_..." constants in
 * "nbase.idl").
 */
typedef struct
{
    rpc_auth_init_fn_t      auth_init;
    rpc_authn_protocol_id_t authn_protocol_id;
    dce_rpc_authn_protocol_id_t 
                            dce_rpc_authn_protocol_id;
    rpc_auth_epv_t          *epv;
    rpc_auth_rpc_prot_epv_tbl_t
                            rpc_prot_epv_tbl;
} rpc_authn_protocol_id_elt_t, *rpc_authn_protocol_id_elt_p_t;


/***********************************************************************/
/*
 * R P C _ G _ A U T H N _ P R O T O C O L _ I D
 *
 * The RPC Authentication Protocol ID table.  This table is indexed by
 * auth protocol ID.
 *
 * Note that the ".auth_protocol_id" field of i'th element in the table
 * is always "i".  While redundant, this is useful so that you can pass
 * pointers to individual table elements.
 */
EXTERNAL rpc_authn_protocol_id_elt_t  rpc_g_authn_protocol_id[RPC_C_AUTHN_PROTOCOL_ID_MAX];

/*
 * Accessor macros for the Auth Protocol ID table.
 */
#define RPC_AUTHN_INQ_SUPPORTED(id)             (rpc_g_authn_protocol_id[id].auth_init != NULL)
#define RPC_AUTHN_INQ_EPV(id)                   rpc_g_authn_protocol_id[id].epv
#define RPC_AUTHN_INQ_RPC_PROT_EPV_TBL(id)      rpc_g_authn_protocol_id[id].rpc_prot_epv_tbl
#define RPC_AUTHN_INQ_RPC_PROT_EPV(id, rpc_id)  (RPC_AUTHN_INQ_RPC_PROT_EPV_TBL(id))[rpc_id]
#define RPC_AUTHN_INQ_SUPPORTED_RPC_PROT(id, rpc_id) (RPC_AUTHN_INQ_RPC_PROT_EPV(id, rpc_id) != NULL)

/*
 * Make sure the specified authentication protocol is valid.  If we're
 * asked for the "default" authentication protocol, use DCE private
 * authentication.  (Obviously this will have to be generalized in the
 * future.)
 */

static inline int RPC_AUTHN_IN_RANGE(unsigned32 id)
{
	return (id > 0) ? (id < RPC_C_AUTHN_PROTOCOL_ID_MAX) : 0;
}
	
#define RPC_AUTHN_CHECK_SUPPORTED(id, st) \
{ \
    if ((id) == (typeof(id))(rpc_c_authn_default)) \
    { \
        id = rpc_c_authn_dce_private; \
    } \
    else if (! RPC_AUTHN_IN_RANGE(id) || ! RPC_AUTHN_INQ_SUPPORTED(id)) \
    { \
        *(st) = rpc_s_unknown_auth_protocol; \
        return; \
    } \
}

#define RPC_AUTHN_CHECK_SUPPORTED_RPC_PROT(id, rpc_id, st) \
{ \
    RPC_AUTHN_CHECK_SUPPORTED(id, st); \
    if (! RPC_AUTHN_INQ_SUPPORTED_RPC_PROT(id, rpc_id)) \
    { \
        *(st) = rpc_s_proto_unsupp_by_auth; \
        return; \
    } \
}



/***********************************************************************/
/*
 * R P C _ G _ S E R V E R _ P T H R E A D _ A T T R
 *
 * A dcethread attribute for server thread creation.
 * Initialized by rpc_init().
 */
EXTERNAL dcethread_attr     rpc_g_server_dcethread_attr;


/***********************************************************************/
/*
 * Macros for checking the validity of bindings.  Note that these live
 * here instead of "com.h" because they use RPC_PROTOCOL_INQ_SUPPORTED
 * which is defined here.
 */

/*
 * The following macro is used to determine the validity of a binding
 * handle passed into the runtime.  Along with the normal sanity checks,
 * it will also check whether the handle is being used for the first
 * time in the child of a fork.  In the case of a handle crossing a fork,
 * a protocol specific routine is called to clean up any dangling state
 * that might have been carried across the fork.  Any other failed checks
 * result in a bad status being set.
 */

#define RPC_BINDING_VALIDATE(binding_rep, st) \
{ \
    if ((binding_rep) == NULL || \
        (binding_rep)->protocol_id >= RPC_C_PROTOCOL_ID_MAX || \
        ! RPC_PROTOCOL_INQ_SUPPORTED((binding_rep)->protocol_id)) \
    { \
        *(st) = rpc_s_invalid_binding; \
    } \
    else if ((binding_rep)->fork_count != rpc_g_fork_count) \
    { \
        rpc__binding_cross_fork(binding_rep, st); \
    } \
    else \
        *(st) = rpc_s_ok; \
} 

/*
 * The following macros are for use by callers that want to verify
 * that, along with being valid, the handle in question is of the
 * right type.  Note that the check for the correct type is only
 * done if the sanity checks carried out by the VALIDATE macro pass.
 */
#define RPC_BINDING_VALIDATE_SERVER(binding_rep, st) \
{ \
    RPC_BINDING_VALIDATE(binding_rep, st) \
    if (*(st) == rpc_s_ok && RPC_BINDING_IS_CLIENT(binding_rep)) \
        *(st) = rpc_s_wrong_kind_of_binding; \
}

#define RPC_BINDING_VALIDATE_CLIENT(binding_rep, st) \
{ \
    RPC_BINDING_VALIDATE(binding_rep, st) \
    if (*(st) == rpc_s_ok && RPC_BINDING_IS_SERVER(binding_rep)) \
        *(st) = rpc_s_wrong_kind_of_binding; \
}

/***********************************************************************/
/*
 * Prototypes for Common Communications Services routines that are used
 * across sub-components of the service.
 */

#ifdef __cplusplus
extern "C" {
#endif

PRIVATE void rpc__if_inq_endpoint (
        rpc_if_rep_p_t          /*ifspec*/,
        rpc_protseq_id_t        /*protseq_id*/,
        unsigned_char_t         ** /*endpoint*/,
        unsigned32              * /*status*/
    );

#include <comimage.h>

#ifdef __cplusplus
}
#endif


#endif /* _COMP_H */
