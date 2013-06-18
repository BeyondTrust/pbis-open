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
#ifndef _RPCCLOCK_H
#define _RPCCLOCK_H	1
/*
**
**  NAME:
**
**      rpcclock.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**
*/

#include <dce/dce.h>

/* 
 * Number of times per second the clock ticks (LSB weight for time values).
 */

#define  RPC_C_CLOCK_HZ             5      
           
#define  RPC_CLOCK_SEC(sec)         ((sec)*RPC_C_CLOCK_HZ)
#define  RPC_CLOCK_MS(ms)           ((ms)/(1000/RPC_C_CLOCK_HZ))

typedef unsigned32  rpc_clock_t, *rpc_clock_p_t;
                                      
/*
 * An absolute time, UNIX time(2) format (i.e. time since 00:00:00 GMT, 
 * Jan. 1, 1970, measured in seconds.
 */

typedef unsigned32  rpc_clock_unix_t, *rpc_clock_unix_p_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get the current approximate tick count.  This routine is used to
 * timestamp data structures.  The tick count returned is only updated
 * by the rpc_timer routines once each time through the select listen
 * loop.  This degree of accuracy should be adequate for the purpose
 * of tracking the age of a data structure.
 */                                          

PRIVATE rpc_clock_t rpc__clock_stamp (void);

/*
 * A routine to determine whether a specified time interval has passed.
 */
PRIVATE boolean rpc__clock_aged    (
        rpc_clock_t          /*time*/,
        rpc_clock_t          /*interval*/
    );

/*
 * Update the current tick count.  This routine is the only one that
 * actually makes system calls to obtain the time, and should only be
 * called from within the rpc_timer routines themselves.  Everyone else
 * should use the  routine rpc_timer_get_current_time which returns an
 * approximate tick count, or rpc_timer_aged which uses the approximate
 * tick count.  The value returned is the current tick count just
 * calculated.
 */
PRIVATE void rpc__clock_update ( void );

/*
 * Determine if a UNIX absolute time has expired
 * (relative to the system's current time).
 */

PRIVATE boolean rpc__clock_unix_expired (
        rpc_clock_unix_t    /*time*/
    );

/*
 * Convert an rpc_clock_t back to a "struct timespec".
 */

PRIVATE void rpc__clock_timespec (
	rpc_clock_t  /*clock*/,
	struct timespec * /*ts*/
    );
      
#ifdef __cplusplus
}
#endif

#endif /* _RPCCLOCK_H */
