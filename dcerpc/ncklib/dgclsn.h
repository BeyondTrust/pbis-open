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
**      dgclsn.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG protocol service routines.  Client-oriented routines that run in the 
**  listener thread.
**
**
*/

#ifndef _DGCLSN_H
#define _DGCLSN_H	1

#include <dce/dce.h>


#ifdef __cplusplus
extern "C" {
#endif


PRIVATE boolean rpc__dg_do_common_response (               
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

PRIVATE boolean rpc__dg_do_reject (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

PRIVATE boolean rpc__dg_do_fault (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

PRIVATE boolean rpc__dg_do_response (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

PRIVATE boolean rpc__dg_do_working (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

PRIVATE boolean rpc__dg_do_nocall (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

PRIVATE boolean rpc__dg_do_quack (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_ccall_p_t  /*ccall*/
    );

#ifdef __cplusplus
}
#endif

#endif /* _DGCLSN_H */
