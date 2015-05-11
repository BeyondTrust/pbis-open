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
/*
**
**  NAME
**
**      rpcrand.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The support routines for the rpcrand.h abstraction.  These should NOT 
**  be called directly; use the macros defined in rpcrand.h .
**
**
*/

#include <commonp.h>

/* 
 * R P C _ _ R A N D O M _ I N I T
 */

PRIVATE void rpc__random_init
(
    unsigned32 seed
)
{
    srandom ((int) seed);
}

/* 
 * R P C _ _ R A N D O M _ G E T
 */

PRIVATE unsigned32 rpc__random_get
(
    unsigned32 lower ATTRIBUTE_UNUSED,
    unsigned32 upper ATTRIBUTE_UNUSED
)
{
    return (random ());
}
