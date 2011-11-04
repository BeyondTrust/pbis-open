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
**  NAME
**
**      perf_c.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Common header file used by perf client and server.
**
**
*/

#include <signal.h>
#include <stdio.h>
#include <math.h>

#include <dce/nbase.h>
#include <dce/rpc.h>

#include <perf.h>
#include <perfb.h>
#include <perfc.h>
#include <perfg.h>

#ifndef NO_TASKING
#  include <dce/pthread.h>
#  include <dce/exc_handling.h>
#  ifdef BROKEN_CMA_EXC_HANDLING
#    define pthread_cancel_e  cma_e_alerted
#  endif
#endif

#include <dce/rpcexc.h>

#if defined(vms) || defined(SYS5)
#  define index strchr
#endif

extern char *error_text();

#if defined(vax) && ! (defined(vms) || defined(ultrix))
#  include <vax.h>
#  define MARSHALL_DOUBLE(d) d_to_g(d)
#  define UNMARSHALL_DOUBLE(d) g_to_d(d)
#else
#  define MARSHALL_DOUBLE(d)
#  define UNMARSHALL_DOUBLE(d)
#endif

extern uuid_old_t FooType, BarType, FooObj1, FooObj2, BarObj1, BarObj2;
extern dce_uuid_t NilTypeObj, NilObj, ZotObj, ZotType;

extern char *authn_level_names[];
extern char *authn_names[];
extern char *authz_names[];

#define DEBUG_LEVEL   "0.1"
#define LOSSY_LEVEL   "4.99"

#ifdef CMA_INCLUDE
#define USE_PTHREAD_DELAY_NP
#endif

#ifdef USE_PTHREAD_DELAY_NP

#define SLEEP(secs) \
{ \
    struct timespec delay; \
    delay.tv_sec  = (secs); \
    delay.tv_nsec = 0; \
    pthread_delay_np(&delay); \
}

#else

#define SLEEP(secs) \
    sleep(secs)

#endif

#define VRprintf(level, stuff) \
{ \
    if (verbose >= (level)) \
        printf stuff; \
}
