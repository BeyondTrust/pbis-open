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
#ifndef _COMINIT_H
#define _COMINIT_H
/*
**
**  NAME
**
**      cominit.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Interface to the Common Communications Service Initialization Service.
**
**
*/

#include <dce/dce.h>

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************/
/*
 * R P C _ _ I N I T
 *
 */

PRIVATE void rpc__init ( void );


PRIVATE void rpc__fork_handler (
        rpc_fork_stage_id_t   /*stage*/
        
    );

PRIVATE void rpc__set_port_restriction_from_string (
        unsigned_char_p_t  /*input_string*/,
        unsigned32         * /*status*/
    );

#ifdef __cplusplus
}
#endif

#endif /* _COMINIT_H */

