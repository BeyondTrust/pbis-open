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
#ifndef _RPCDBG_H
#define _RPCDBG_H	1
/*
**
**  NAME:
**
**      rpcdbg.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Various macros and data to assist with debugging related code.
**
**
*/

/*
 * Debug has been separated into messaging
 * code and "other" ... messaging now largely
 * implemented by rpcsvc.{c,h}
 */
#  include <rpcsvc.h>

/*
 * A few macros for dealing with debugging code / printfs.  In general,
 * code is generated only if DEBUG is defined.
 *
 * The model here is that there are a number of debug "switches", each one
 * of which can be set to some debug "level".  The switches are represented
 * as an enumeration.
 */

/*
 * The debug switches.
 */

typedef enum {
    rpc_es_dbg_general = 0,              /*  0 */
    rpc_es_dbg_mutex,                    /*  1 */
    rpc_es_dbg_xmit,                     /*  2 */
    rpc_es_dbg_recv,                     /*  3 */
    rpc_es_dbg_dg_lossy,                 /*  4 */
    rpc_es_dbg_dg_state,                 /*  5 */
    rpc_es_dbg_ip_max_pth_unfrag_tpdu,   /*  6 */
    rpc_es_dbg_ip_max_loc_unfrag_tpdu,   /*  7 */
    rpc_es_dbg_dds_max_pth_unfrag_tpdu,  /*  8 */
    rpc_es_dbg_dds_max_loc_unfrag_tpdu,  /*  9 */
    rpc_es_dbg_dg_rq_qsize,              /* 10 */
    rpc_es_dbg_cancel,                   /* 11 */
    rpc_es_dbg_orphan,                   /* 12 */
    rpc_es_dbg_cn_state,                 /* 13 */
    rpc_es_dbg_cn_pkt,                   /* 14 */
    rpc_es_dbg_pkt_quotas,               /* 15 */
    rpc_es_dbg_auth,                     /* 16 */
    rpc_es_dbg_source,                   /* 17 */
    rpc_es_dbg_pkt_quota_size,           /* 18 */
    rpc_es_dbg_stats,                    /* 19 */
    rpc_es_dbg_mem,                      /* 20 */
    rpc_es_dbg_mem_type,                 /* 21 */
    rpc_es_dbg_dg_pktlog,                /* 22 */
    rpc_es_dbg_thread_id,                /* 23 */
    rpc_es_dbg_timestamp,                /* 24 */
    rpc_es_dbg_cn_errors,                /* 25 */
    rpc_es_dbg_conv_thread,              /* 26 */
    rpc_es_dbg_pid,                      /* 27 */
    rpc_es_dbg_atfork,                   /* 28 */
    rpc_es_dbg_cma_thread,               /* 29 */
    rpc_es_dbg_inherit,                  /* 30 */
    rpc_es_dbg_dg_sockets,               /* 31 */
    rpc_es_dbg_ip_max_tsdu,              /* 32 */
    rpc_es_dbg_dg_max_psock,             /* 33 */
    rpc_es_dbg_dg_max_window_size,       /* 34 */
    rpc_es_dbg_threads,       	 	 /* 35 */
    rpc_es_dbg_uxd_max_pth_unfrag_tpdu,   /*  36 */
    rpc_es_dbg_uxd_max_loc_unfrag_tpdu,   /*  37 */
    rpc_es_dbg_uxd_max_tsdu,              /* 38 */
    rpc_es_dbg_np_max_pth_unfrag_tpdu,   /*  39 */
    rpc_es_dbg_np_max_loc_unfrag_tpdu,   /*  40 */
    rpc_es_dbg_np_max_tsdu,              /* 41 */
 
    /* 
     * Add new switches above this comment and adjust the
     * "last_switch" value if necessary.  We keep a few
     * empty slots to allow for easy temporary additions.
     */
    rpc_es_dbg_last_switch       = 42    /* 42 */
} rpc_dbg_switch_t;

#define RPC_DBG_N_SWITCHES (((int) rpc_es_dbg_last_switch) + 1)

#define RPC_C_DBG_SWITCHES      43      /* size of rpc_g_dbg_switches[] */

#ifdef DEBUG

/*
 * Debug table
 *
 * A vector of "debug levels", one level per "debug switch".
 */
EXTERNAL unsigned8 rpc_g_dbg_switches[];

#endif


/*
 * R P C _ D B G
 *
 * Tests whether a particular debug switch is set at a particular level (or
 * higher).
 */
#ifdef DEBUG 

#define RPC_DBG(switch, level) (rpc_g_dbg_switches[(int) (switch)] >= (level))

#else /* DEBUG */

#define RPC_DBG(switch, level) (0)

#endif /* DEBUG */

/*
 * R P C _ D B G _ E X A C T
 *
 * Tests whether a particular debug switch is set at exactly a particular level
 */
#ifdef DEBUG 

#define RPC_DBG_EXACT(switch, level) (rpc_g_dbg_switches[(int) (switch)] == (level))

#else /* DEBUG */

#define RPC_DBG_EXACT(switch, level) (0)

#endif /* DEBUG */


/*
 * a macro to set *status rpc_s_coding_error
 */
#ifdef DEBUG

#define CODING_ERROR(status)        *(status) = rpc_s_coding_error

#else /* DEBUG */

#define CODING_ERROR(status)

#endif /* DEBUG */

#ifndef DCE_RPC_SVC
/*
 * R P C _ D B G _ P R I N T F
 *
 * A macro that prints debug info based on a debug switch's level.  Note
 * that this macro is intended to be used as follows:
 *
 *      RPC_DBG_PRINTF(rpc_es_dbg_xmit, 3, ("Sent pkt %d", pkt_count));
 *
 * I.e. the third parameter is the argument list to "printf" and must be
 * enclosed in parens.  The macro is designed this way to allow us to 
 * eliminate all debug code when DEBUG is not defined.
 *
 */
#ifdef DEBUG

#define RPC_DBG_ADD_PRINTF(switch, level, pargs) \
( \
    ! RPC_DBG((switch), (level)) ? \
        0 : (rpc__printf pargs, 0) \
)

#define RPC_DBG_PRINTF(switch, level, pargs) \
( \
    ! RPC_DBG((switch), (level)) ? \
        0 : (rpc__printf pargs, rpc__print_source(__FILE__, __LINE__), 0) \
)

#define RPC_DBG_GPRINTF(pargs) \
    RPC_DBG_PRINTF(rpc_es_dbg_general, 1, pargs)

#else /* DEBUG */

#define RPC_DBG_PRINTF(switch, level, pargs) do {;} while(0)
#define RPC_DBG_ADD_PRINTF(switch, level, pargs) do {;} while(0)
#define RPC_DBG_GPRINTF(pargs) do {;} while(0)

#endif /* DEBUG */

#else	/* !DCE_RPC_SVC */

/*
 * R P C _ D B G _ G P R I N T F
 *
 * A macro on top of RPC_DBG_PRINTF that's used for printing random general
 * debug info.  Sample usage:
 *
 *      RPC_DBG_GPRINTF(("Sent pkt %d", pkt_count));
 *
 */

#define RPC_DBG_GPRINTF(pargs) \
    RPC_DBG_PRINTF(rpc_es_dbg_general, 1, pargs)

#endif	/* !DCE_RPC_SVC */

/*
 * R P C _ _ D B G _ S E T _ S W I T C H E S
 */

PUBLIC void rpc__dbg_set_switches    (
        char            * /*s*/,
        unsigned32      * /*st*/
    );


#ifndef	DCE_RPC_SVC
/*
 * R P C _ _ P R I N T F
 *
 * Note: This function uses a variable-length argument list. The "right"
 * way to handle this is using the ANSI C notation (listed below under
 * #ifdef STDARG_PRINTF). However, not all of the compilers support this,
 * so it's here just for future reference purposes.
 *
 * An alternative is to use the "varargs" convention (listed below under
 * #ifndef NO_VARARGS_PRINTF). Most compilers support this convention,
 * however you can't use prototypes with this.
 *
 * The next to last choice is to use the "old" notation. In this case
 * also you can't use prototypes.
 *
 * The last choice is to abandon this all together, define NO_RPC_PRINTF
 * and just use "printf" (from commonp.h).
 */
#if defined(VMS) || (defined(ultrix) && defined(vaxc))
#define NO_RPC_PRINTF
#endif

#ifndef NO_RPC_PRINTF

#include <stdarg.h>

PRIVATE int rpc__printf ( char * /*format*/, ...);

#endif /* NO_RPC_PRINTF */

#endif	/* !DCE_RPC_SVC */

/*
 * R P C _ _ D I E
 */

PRIVATE void rpc__die (
        char            * /*text*/,
        char            * /*file*/,
        int              /*line*/
    );

/*
 * R P C _ _ U U I D _ S T R I N G
 */

PRIVATE char *rpc__uuid_string ( dce_uuid_t */*uuid*/);

/*
 * R P C _ _ P R I N T _ S O U R C E
 */

PRIVATE void rpc__print_source (
        char            * /*file*/,
        int             /*line*/
    );

#endif /* _RPCDBG_H */
