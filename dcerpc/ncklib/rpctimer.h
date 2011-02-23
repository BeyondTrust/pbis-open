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
#ifndef _RPCTIMER_H
#define _RPCTIMER_H
/*
**
**  NAME
**
**      rpctimer.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Interface to NCK timer functions.
**  
**  Suggested uasge:  These routines are tailored for the situation
**  in which a monitor routine is needed to keep track of the state
**  of a data structure.  In this case, one of the fields in the data
**  structure should be a rpc_timer_t (defined below).  The user of the
**  timer service would then call the rpc__timer_set routine with the 
**  following arguments:
**       
**      1) the address of the rpc_timer_t variable
**      2) the address of the routine to run
**      3) a pointer_t value to be sent to the routine when run
**      4) the frequency with which the routine should be run
**
**  The pointer value used will most often be the address of the data
**  structure being monitored.  (However, it also possible to declare
**  a global rpc_timer_t to be used to periodically run a routine not
**  associated with any particular data object.  N.B.  It is necessary 
**  that the rpc_timer_t variable exist during the entire time that the
**  periodic routine is registered to run;  that is, don't declare it on
**  the stack if the current routine will return before unregistering the
**  periodic routine.)
**
**  Users of this service should not keep track of time by the frequency
**  with which their periodic routines are run.  Aside from the lack of 
**  accuracy of user space time functions, it is also possible that the
**  system's idea of the current time may be changed at any time.  When 
**  necessary, the routines currently err in favor of running a periodic 
**  routine too early rather than too late.  For this reason, data 
**  struture monitoring should follow this outline:
**
**      struct {
**          .....
**          rpc_clock_t        timestamp;
**          rpc_timer_t        timer;
**          ..... 
**      } foo;                             
**
**      rpc__clock_stamp( &foo.timestamp );          
**
**      rpc__timer_set( &foo.timer, foo_mon, &foo, rpc_timer_sec(1) );
**          ...
**
**      void foo_mon( parg ) 
**      pointer_t parg;
**      {
**          if( rpc__clock_aged( parg->timestamp, rpc_timer_sec( 1 ) )
**          {
**            ...
**          }
**      }
**  
**
**
*/
           
#include <dce/dce.h>

typedef void (*rpc_timer_proc_p_t) ( pointer_t );

/*
 * This type is used to create a list of periodic functions ordered by
 * trigger time.
 */
typedef struct rpc_timer_t
{
    struct rpc_timer_t   *next;
    rpc_clock_t          trigger;
    rpc_clock_t          frequency;
    rpc_timer_proc_p_t   proc;
    pointer_t            parg;                           
} rpc_timer_t, *rpc_timer_p_t;

       
/* 
 * Initialize the timer package.
 */
PRIVATE void rpc__timer_init (void);
       
/* 
 * Timer package fork handling routine
 */
PRIVATE void rpc__timer_fork_handler (
    rpc_fork_stage_id_t  /*stage*/
);

/* 
 * Shutdown the timer package.
 */
PRIVATE void rpc__timer_shutdown (void);

/* 
 * Register a routine to be run at a specific interval.
 */
PRIVATE void rpc__timer_set (
    rpc_timer_p_t /*t*/,
    rpc_timer_proc_p_t /*proc*/,
    pointer_t /*parg*/, 
    rpc_clock_t  /*freq*/
);

/* 
 * Change one or more of the characteristics of a periodic routine.
 */
PRIVATE void rpc__timer_adjust (
    rpc_timer_p_t /*t*/,
    rpc_clock_t /*freq*/
);

/*
 * Discontinue running a previously registered periodic routine.
 */
PRIVATE void rpc__timer_clear (rpc_timer_p_t /*t*/);

/*
 * Run any periodic routines that are ready.  Return the amount of time
 * until the next scheduled routine should be run.
 */                                          
PRIVATE rpc_clock_t rpc__timer_callout (void);

#endif /* _RPCTIMER_H */
