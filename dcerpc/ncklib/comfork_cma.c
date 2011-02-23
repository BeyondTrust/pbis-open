/*
 * 
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
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
**      rpc/runtime/comfork_cma.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Include CMA atfork grodiness.
**
**
*/

#include <commonp.h>
#include <com.h>        /* Externals for Common Services component  */

#if defined(ATFORK_SUPPORTED)

#if defined(CMA_INCLUDE)

/*
 * This really doesn't belong here, but is here for pragmatic reasons.
 */

typedef void (*atfork_handler_ptr_t) (rpc_fork_stage_id_t /*stage*/);
/*
 * Save the address of the fork handler registered through atfork.  
 * This pointer should be saved ONLY once by rpc__cma_atfork(). If
 * rpc__cma_atfork() is called more than once, which happens in the
 * child context when rpc__init() is called, do not register the
 * fork handler.
 */
INTERNAL atfork_handler_ptr_t  atfork_handler_ptr = NULL;

INTERNAL struct atfork_user_data_t
{
    unsigned32  pid;    /* who registered */
    unsigned32  pre;    /* How many times _pre_fork called */
    unsigned32  post_p; /*                _post_fork_parent called */
    unsigned32  post_c; /*                _post_fork_child called */
} atfork_user_data;

/* ========================================================================= */

INTERNAL void _pre_fork (
        cma_t_address   /*arg*/
    );

INTERNAL void _post_fork_child (
        cma_t_address   /*arg*/
    );

INTERNAL void _post_fork_parent (
        cma_t_address   /*arg*/
    );

/* ========================================================================= */

/*
 * _ P R E _ F O R K
 *
 * This procedure is called by the Pthreads library prior to calling
 * the fork/vfork system call.
 */         
INTERNAL void _pre_fork
(
  cma_t_address arg 
)
{                      
    RPC_DBG_PRINTF(rpc_e_dbg_atfork, 1,
               ("(_pre_fork) entering, pid %d, pre %d, post_p %d, post_c %d\n",
                    ((struct atfork_user_data_t *) arg)->pid,
                    ((struct atfork_user_data_t *) arg)->pre,
                    ((struct atfork_user_data_t *) arg)->post_p,
                    ((struct atfork_user_data_t *) arg)->post_c));

    ((struct atfork_user_data_t *) arg)->pre++;

    if (rpc_g_initialized == true)
        (*atfork_handler_ptr)(RPC_C_PREFORK);

    RPC_DBG_PRINTF(rpc_e_dbg_atfork, 1,
              ("(_pre_fork) returning, pid %d, pre %d, post_p %d, post_c %d\n",
                    ((struct atfork_user_data_t *) arg)->pid,
                    ((struct atfork_user_data_t *) arg)->pre,
                    ((struct atfork_user_data_t *) arg)->post_p,
                    ((struct atfork_user_data_t *) arg)->post_c));
}

/*
 * _ P O S T _ F O R K _ C H I L D
 *
 * This procedure is called in the child of a forked process immediately
 * after the fork is performed.
 */         

INTERNAL void _post_fork_child
(
  cma_t_address arg
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_atfork, 1,
        ("(_post_fork_child) entering, pid %d, pre %d, post_p %d, post_c %d\n",
                    ((struct atfork_user_data_t *) arg)->pid,
                    ((struct atfork_user_data_t *) arg)->pre,
                    ((struct atfork_user_data_t *) arg)->post_p,
                    ((struct atfork_user_data_t *) arg)->post_c));

    ((struct atfork_user_data_t *) arg)->post_c++;

    if (rpc_g_initialized == true)
        (*atfork_handler_ptr)(RPC_C_POSTFORK_CHILD);

    RPC_DBG_PRINTF(rpc_e_dbg_atfork, 1,
       ("(_post_fork_child) returning, pid %d, pre %d, post_p %d, post_c %d\n",
                    ((struct atfork_user_data_t *) arg)->pid,
                    ((struct atfork_user_data_t *) arg)->pre,
                    ((struct atfork_user_data_t *) arg)->post_p,
                    ((struct atfork_user_data_t *) arg)->post_c));
}

/*
 * _ P O S T _ F O R K _ P A R E N T
 *
 * This procedure is called in the parent of a forked process immediately
 * after the fork is performed.
 */         


INTERNAL void _post_fork_parent
(
  cma_t_address arg
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_atfork, 1,
       ("(_post_fork_parent) entering, pid %d, pre %d, post_p %d, post_c %d\n",
                    ((struct atfork_user_data_t *) arg)->pid,
                    ((struct atfork_user_data_t *) arg)->pre,
                    ((struct atfork_user_data_t *) arg)->post_p,
                    ((struct atfork_user_data_t *) arg)->post_c));

    ((struct atfork_user_data_t *) arg)->post_p++;

    if (rpc_g_initialized == true)
        (*atfork_handler_ptr)(RPC_C_POSTFORK_PARENT);

    RPC_DBG_PRINTF(rpc_e_dbg_atfork, 1,
      ("(_post_fork_parent) returning, pid %d, pre %d, post_p %d, post_c %d\n",
                    ((struct atfork_user_data_t *) arg)->pid,
                    ((struct atfork_user_data_t *) arg)->pre,
                    ((struct atfork_user_data_t *) arg)->post_p,
                    ((struct atfork_user_data_t *) arg)->post_c));
}


PRIVATE void rpc__cma_atfork
(
 void *handler
)
{
    if (handler == NULL)
        return;

    /*
     * Don't register it again!
     */
    if (atfork_handler_ptr != NULL)
        return;

    /*
     * Save the address of the handler routine, and register our own
     * handlers. (see note above)
     */
    atfork_handler_ptr = (atfork_handler_ptr_t) handler;
    atfork_user_data.pid = (unsigned32) getpid();
    atfork_user_data.pre = 0;
    atfork_user_data.post_p = 0;
    atfork_user_data.post_c = 0;

    cma_atfork((cma_t_address)(&atfork_user_data), _pre_fork,
               _post_fork_parent, _post_fork_child);
}
#endif  /* defined(CMA_INCLUDE) */

#endif  /* defined(ATFORK_SUPPORTED) */
