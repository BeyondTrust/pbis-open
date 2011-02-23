/*
 * 
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
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
#ifndef _COMTWR_H
#define _COMTWR_H 1
/*
**
**
**  NAME:
**
**      comtwr.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**  
**  ABSTRACT:
**
**      Header file containing macros, definitions, typedefs and prototypes 
**      of exported routines from the comtwr.c module.
**  
**
**/

#include <dce/dce.h>

#ifdef __cplusplus
extern "C" {
#endif

PRIVATE void rpc__tower_free ( 
    twr_p_t                 * /*tower*/,
    unsigned32              * /*status*/ );

PRIVATE void rpc__tower_from_tower_ref ( 
    rpc_tower_ref_p_t        /*tower_ref*/,
    twr_p_t                 * /*tower*/,
    unsigned32              * /*status*/ );

PRIVATE void rpc__tower_to_tower_ref ( 
   twr_p_t                  /*tower*/,
   rpc_tower_ref_p_t       * /*tower_ref*/,
   unsigned32              * /*status*/ );

#endif /* _COMTRW_H */
