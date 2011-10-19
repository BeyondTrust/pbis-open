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
**      dgglob.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Defining instances of DG-specific runtime global variables.
**
**
*/

#include <dg.h>

/* ======= */

    /*
     * The following variables are the "defining instance" and static
     * initialization of variables mentioned (and documented) in
     * "dgglob.h".
     */

GLOBAL rpc_dg_cct_t rpc_g_dg_cct;

GLOBAL rpc_dg_ccall_p_t rpc_g_dg_ccallt[RPC_DG_CCALLT_SIZE];

GLOBAL rpc_dg_sct_elt_p_t rpc_g_dg_sct[RPC_DG_SCT_SIZE];

GLOBAL rpc_dg_stats_t rpc_g_dg_stats = RPC_DG_STATS_INITIALIZER;

GLOBAL unsigned32 rpc_g_dg_server_boot_time;

GLOBAL dce_uuid_t rpc_g_dg_my_cas_uuid;

GLOBAL rpc_dg_sock_pool_t rpc_g_dg_sock_pool;
/* ======= */

