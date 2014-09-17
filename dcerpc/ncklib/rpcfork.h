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
#ifndef _RPCFORK_H
#define _RPCFORK_H	1
/*
**
**  NAME:
**
**      rpcfork.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Various macros and data to assist with fork handling.
**
**
*/

/*
 * Define constants to be passed to fork handling routines.  The value passed 
 * indicates at which stage of the fork we are.
 */

#include <commonp.h>
#define RPC_C_PREFORK          1
#define RPC_C_POSTFORK_PARENT  2
#define RPC_C_POSTFORK_CHILD   3

typedef unsigned32       rpc_fork_stage_id_t;

PRIVATE void rpc__atfork ( void *handler);

#endif /* RCPFORK_H */
