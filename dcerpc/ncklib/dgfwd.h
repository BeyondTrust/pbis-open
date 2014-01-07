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
#ifndef _DGFWD_H
#define _DGFWD_H

/*
**
**  NAME:
**
**      dgfwd.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG Packet Forwarding handler
**
**
*/

#include <dce/dce.h>

/*
 * R P C _ _ D G _ F W D _ P K T
 *
 * Forwarding Service.
 */

PRIVATE unsigned32 rpc__dg_fwd_pkt    (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/
    );

/*
 * Can return three values:
 *     FWD_PKT_NOTDONE  - caller should handle packet
 *     FWD_PKT_DONE     - we handled the packet, ok to free it
 *     FWD_PKT_DELAYED  - we saved it, don't handle it, don't free it.
 */
#define	FWD_PKT_NOTDONE		0
#define FWD_PKT_DONE		1
#define FWD_PKT_DELAYED		2

/*
 * R P C _ _ D G _ F W D _ I N I T
 *
 * Initialize forwarding service private mutex.
 */

PRIVATE void rpc__dg_fwd_init (void);

#endif /* _DGFWD_H */
