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
#ifndef _RPCRAND_H
#define _RPCRAND_H
/*
**
**  NAME:
**
**      rpcrand.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
** 
**  Random number generator abstraction to isolate random number generation
**  routines and allow alternate implementations to be substituted more
**  easily.
**
**  This package provides the following PRIVATE operations:
**  
**      void       RPC_RANDOM_INIT(seed)
**      unsigned32 RPC_RANDOM_GET(lower, upper)
**
**
*/   


/* 
 * R P C _ R A N D O M _ I N I T
 *
 * Used for random number 'seed' routines or any other one time
 * initialization required.
 */

#define RPC_RANDOM_INIT(seed) \
        rpc__random_init(seed)

/* 
 * R P C _ R A N D O M _ G E T
 *
 * Get a random number in the range lower - upper (inclusive)
 */ 

#define RPC_RANDOM_GET(lower, upper) \
        (((rpc__random_get(lower, upper)) % (upper - lower + 1)) + lower) 

/*
 * Prototype for the private 'c' routines used by the RPC_RANDOM macros.
 */

#include <dce/dce.h>


PRIVATE void rpc__random_init ( unsigned32  /*seed*/ );

PRIVATE unsigned32 rpc__random_get (
        unsigned32  /*lower*/,
        unsigned32  /*upper*/
    );

#endif /* _RPCRAND_H */
        
