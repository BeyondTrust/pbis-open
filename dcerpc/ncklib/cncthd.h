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
#ifndef _CNCTHD_H
#define _CNCTHD_H	1
/*
**
**  NAME
**
**      cncthd.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types/constants for the NCA Connection Protocol
**  Service's Call Executor Service.
**
**
*/


/***********************************************************************/
/*
 * R P C _ _ C N _ C A L L _ E X E C U T O R
 */

PRIVATE void rpc__cn_call_executor (
        pointer_t       /* call_r */,
        boolean32       /* call_was_queued */
    );
#endif /* _CNCTHD_H */
