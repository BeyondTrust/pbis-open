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
**      rpcclock.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Routines for manipulating the time entities understood by the rpc_timer
**  services.
**
**
*/

#include <commonp.h>


/* ========================================================================= */

GLOBAL rpc_clock_t rpc_g_clock_curr;
GLOBAL rpc_clock_unix_t rpc_g_clock_unix_curr;

EXTERNAL boolean32 rpc_g_long_sleep;

INTERNAL struct timeval  start_time = { 0, 0 };
INTERNAL rpc_clock_t     rpc_clock_skew = 0;

/* ========================================================================= */

/*
 * R P C _ _ C L O C K _ S T A M P
 * 
 * Timestamp a data structure with the current approximate tick count.
 * The tick count returned is only updated by the rpc_timer routines
 * once each time through the select listen loop.  This degree of accuracy
 * should be adequate for the purpose of tracking the age of a data
 * structure.                                          
 */

PRIVATE rpc_clock_t rpc__clock_stamp(void)
{
    if (rpc_g_long_sleep)
	rpc__clock_update();

    return( rpc_g_clock_curr );
}


/*
 * R P C _ _ C L O C K _ A G E D
 *
 * A routine to determine whether a specified timestamp has aged.
 */

PRIVATE boolean rpc__clock_aged
(
    rpc_clock_t      time,
    rpc_clock_t      interval
)
{
    return( rpc_g_clock_curr >= (time + interval) );
}


/*
 * R P C _ _ C L O C K _ U N I X _ E X P I R E D
 *
 * Determine whether a specified UNIX timestamp is in the past.
 */

PRIVATE boolean rpc__clock_unix_expired 
(
    rpc_clock_unix_t time
)
{
    return( rpc_g_clock_unix_curr >= time );
}

/*
 * R P C _ _ C L O C K _ U P D A T E 
 *
 * Update the current tick count.  This routine is the only one that
 * actually makes system calls to obtain the time.
 */

PRIVATE void rpc__clock_update(void)
{

    struct timeval         tp;
    unsigned long          ticks;
    
    /*
     * On startup, just initialize start time.  Arrange for the initial
     * time to be '1' tick (0 can be confusing in some cases).
     */                             
    if (start_time.tv_sec == 0)
    {
        gettimeofday( &start_time, (struct timezone *) 0 );
        rpc_g_clock_unix_curr = start_time.tv_sec;
        start_time.tv_usec -= (1000000L/RPC_C_CLOCK_HZ);
        if (start_time.tv_usec < 0)
        {
            start_time.tv_usec += 1000000L;
            start_time.tv_sec --;
        }
        rpc_g_clock_curr = (rpc_clock_t) 1;
    }
    else
    {             
        /*
         * Get the time of day from the system, and convert it to the
         * tick count format we're using (RPC_C_CLOCK_HZ ticks per second).
         * For now, just use 1 second accuracy.
         */
        gettimeofday(&tp, (struct timezone *) 0 );
        rpc_g_clock_unix_curr = tp.tv_sec;      

        ticks = (tp.tv_sec - start_time.tv_sec) * RPC_C_CLOCK_HZ +
                 rpc_clock_skew;

        if (tp.tv_usec < start_time.tv_usec)
        {
            ticks -= RPC_C_CLOCK_HZ;
            tp.tv_usec += 1000000L;
        }
        ticks += (tp.tv_usec - start_time.tv_usec) / (1000000L/RPC_C_CLOCK_HZ);
           
        /*
         * We need to watch out for changes to the system time after we
         * have stored away our starting time value.  This situation is 
         * handled by maintaining a static variable containing the amount of
         * RPC_C_CLOCK_HZ's we believe that the current time has been altered.
         * This variable gets updated each time we detect that the system time
         * has been modified.
         *
         * This scheme takes into account the fact that there are data 
         * structures that have been timestamped with tick counts;  it is 
         * not possible to simply start the tick count over, of to just
         * update the trigger counts in the list of periodic funtions.
         *
         * We determine that the system time has been modified if 1) the
         * tick count has gone backward, or 2) if we notice that we haven't
         * been called in 60 seconds.  This last condition is based on the
         * assumption that the listen loop will never intentionally wait
         * that long before calling us. It would also be possible (tho more
         * complicated) to look at the trigger count of the next periodic
         * routine and assume that if we've gone a couple of seconds past
         * that then something's wrong.
         */
        if ( (ticks < rpc_g_clock_curr) ||
            (ticks > (rpc_g_clock_curr + RPC_CLOCK_SEC(60))))
        {
            rpc_clock_skew += rpc_g_clock_curr - ticks + RPC_C_CLOCK_HZ;
            rpc_g_clock_curr += RPC_C_CLOCK_HZ;
        }
        else
        {
            rpc_g_clock_curr = (rpc_clock_t) ticks;
        }
    }
}   

/*
 * R P C _ _ C L O C K _ T I M E S P E C
 * 
 * Return a "struct timespec" equivalent to a given rpc_clock_t.
 */

PRIVATE void rpc__clock_timespec 
(
        rpc_clock_t clock,
        struct timespec *ts
)
{
    int whole_secs;
    int remaining_ticks;

    clock -= rpc_clock_skew;

    whole_secs = (int)clock / RPC_C_CLOCK_HZ;
    remaining_ticks = (int)clock % RPC_C_CLOCK_HZ;
    if (remaining_ticks < 0)
    {
        whole_secs--;
        remaining_ticks += RPC_C_CLOCK_HZ;
    }

    ts->tv_sec = start_time.tv_sec + whole_secs;
    ts->tv_nsec = (1000 * start_time.tv_usec) + 
        remaining_ticks * (1000000000 / RPC_C_CLOCK_HZ);
    if (ts->tv_nsec >= 1000000000) 
    {
	ts->tv_nsec -= 1000000000;
	ts->tv_sec += 1;
    }
}
