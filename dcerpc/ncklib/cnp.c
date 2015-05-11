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
**      cnp.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Definitions of global variables.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */

GLOBAL rpc_cond_t               rpc_g_cn_lookaside_cond;
GLOBAL rpc_list_desc_t          rpc_g_cn_syntax_lookaside_list;
GLOBAL rpc_list_desc_t          rpc_g_cn_sec_lookaside_list;
GLOBAL rpc_list_desc_t          rpc_g_cn_assoc_lookaside_list;
GLOBAL rpc_list_desc_t          rpc_g_cn_binding_lookaside_list;
GLOBAL rpc_list_desc_t          rpc_g_cn_lg_fbuf_lookaside_list;
GLOBAL rpc_list_desc_t          rpc_g_cn_sm_fbuf_lookaside_list;
GLOBAL rpc_list_desc_t          rpc_g_cn_call_lookaside_list;
GLOBAL rpc_cn_assoc_grp_tbl_t   rpc_g_cn_assoc_grp_tbl;
GLOBAL unsigned32               rpc_g_cn_call_id;
GLOBAL rpc_cn_mgmt_t            rpc_g_cn_mgmt;
