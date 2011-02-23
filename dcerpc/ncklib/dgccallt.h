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
**      dgccallt.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines
**
**
*/

#ifndef _DGCCALLT_H
#define _DGCCALLT_H	1	

#include <dce/dce.h>


#ifdef __cplusplus
extern "C" {
#endif

PRIVATE void rpc__dg_ccallt_insert (
        rpc_dg_ccall_p_t /*ccall*/
    );

PRIVATE void rpc__dg_ccallt_remove (
        rpc_dg_ccall_p_t /*ccall*/
    );

PRIVATE rpc_dg_ccall_p_t rpc__dg_ccallt_lookup (
        dce_uuid_p_t /*actid*/,
        unsigned32 /*probe_hint*/
    );

PRIVATE void rpc__dg_ccallt_fork_handler (
        rpc_fork_stage_id_t /*stage*/
   );

#ifdef __cplusplus
}
#endif


#endif /* _DGCCALLT_H */
