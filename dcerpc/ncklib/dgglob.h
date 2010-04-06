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
**      dgglob.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG-specific runtime global variable (external) declarations.
**
**
*/

#ifndef _DGGLOB_H
#define _DGGLOB_H

/* ========================================================================= */

/*
 * Client connection table (CCT).
 */
EXTERNAL rpc_dg_cct_t rpc_g_dg_cct;

/*
 * Client call control block table (CCALLT).
 */
EXTERNAL rpc_dg_ccall_p_t rpc_g_dg_ccallt[];

/*
 * Server connection table (SCT)
 */
EXTERNAL rpc_dg_sct_elt_p_t rpc_g_dg_sct[];

#ifndef NO_STATS

/*
 * RPC statistics
 */
EXTERNAL rpc_dg_stats_t rpc_g_dg_stats;

#endif /* NO_STATS */

/*
 * The server boot time 
 */
EXTERNAL unsigned32 rpc_g_dg_server_boot_time;

/*
 * The following UUID will be used to uniquely identify a particular instance
 * of a client process.  It is periodically sent to all servers with which we
 * need to maintain liveness.
 */

EXTERNAL dce_uuid_t rpc_g_dg_my_cas_uuid;

/*
 * The DG socket pool
 */  
EXTERNAL rpc_dg_sock_pool_t rpc_g_dg_sock_pool;

#endif /* _DGGLOB_H */
