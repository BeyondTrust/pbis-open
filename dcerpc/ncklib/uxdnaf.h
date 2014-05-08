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
#ifndef _UXDNAF_H
#define _UXDNAF_H	1
/*
**
**  NAME
**
**      uxdnaf.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions and Data Type declarations
**  used by Unix Domain Socket Network Address Family Extension
**  service.
**
**
*/

#include <dce/dce.h>

/***********************************************************************
 *
 *  Include the Internet specific socket address 
 */


#ifdef VMS
#include <un.h>
#else
#include <sys/un.h>
#endif

#ifndef RPC_C_UXD_DIR
#define RPC_C_UXD_DIR	"/var/opt/novell/xad/rpc"
#endif

#ifndef RPC_C_UXD_DIR_LEN
#define RPC_C_UXD_DIR_LEN (sizeof(RPC_C_UXD_DIR) - 1)
#endif

/***********************************************************************
 *
 *  The representation of an RPC Address that holds an UXD address.
 */

typedef struct rpc_addr_uxd_t
{
    rpc_protseq_id_t        rpc_protseq_id;
    unsigned32              len;
    struct sockaddr_un      sa;
} rpc_uxd_addr_t, *rpc_uxd_addr_p_t;


/*
 * Max Local DG Fragment Size:
 *
 *   The size in bytes of the largest DG fragment that can be sent to
 *   a "local" address. The data won't be transmitted over the "wire"
 *   by the transport service, i.e., the loopback is done on the local
 *   host. This is determined when the socket is created and won't
 *   change in the life of the socket.
 *
 * The constant defined here is based on experimentation.
 *
 * Caution: This must be less than RPC_C_DG_MAX_FRAG_SIZE::dg.h!
 */

#ifndef RPC_C_UXD_MAX_LOCAL_FRAG_SIZE
#define RPC_C_UXD_MAX_LOCAL_FRAG_SIZE (8 * 1024)
#endif


/***********************************************************************
 *
 *  Routine Prototypes for the Internet Extension service routines.
 */
    
#ifdef __cplusplus
extern "C" {
#endif


PRIVATE void rpc__uxd_init (  
        rpc_naf_epv_p_t             * /*naf_epv*/,
        unsigned32                  * /*status*/
    );

PRIVATE void rpc__uxd_desc_inq_addr (
        rpc_protseq_id_t             /*protseq_id*/,
        rpc_socket_t                 /*desc*/,
        rpc_addr_vector_p_t         * /*rpc_addr_vec*/,
        unsigned32                  * /*st*/
    );

PRIVATE void rpc__uxd_get_broadcast (
        rpc_naf_id_t                 /*naf_id*/,
        rpc_protseq_id_t             /*rpc_protseq_id*/,
        rpc_addr_vector_p_t         * /*rpc_addrs*/,
        unsigned32                  * /*status*/
    );

PRIVATE void rpc__uxd_init_local_addr_vec (
        unsigned32                  * /*status*/
    );

PRIVATE boolean32 rpc__uxd_is_local_network (
        rpc_addr_p_t                 /*rpc_addr*/,
        unsigned32                  * /*status*/
    );

PRIVATE boolean32 rpc__uxd_is_local_addr (
        rpc_addr_p_t                 /*rpc_addr*/,
        unsigned32                  * /*status*/
    );

#ifdef __cplusplus
}
#endif

              
#endif /* _UXDNAF_H */
