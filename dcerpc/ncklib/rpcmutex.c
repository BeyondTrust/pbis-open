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
**      rpcmutex.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The support routines for rpcmutex.h abstraction.  These should NOT
**  be called directly; use the macros in rpcmutex.h .
**
**
*/

#include <commonp.h>

#if defined(RPC_MUTEX_DEBUG) || defined(RPC_MUTEX_STATS)

/*
 * All of the routines return true on success, false on failure.
 *
 * There are races present on the modifications of the package wide
 * statistics as well as the per lock "busy", "try_lock" and "*_assert"
 * statistics, but we don't really care.  We don't want to burden these
 * "informative" statistics with some other mutex.  We're only providing
 * these stats so we can track gross trends.
 */


/*
 * !!! Since CMA pthreads doesn't provide a "null" handle, create our own.
 */
PRIVATE dcethread* rpc_g_null_thread_handle;
#define NULL_THREAD rpc_g_null_thread_handle

#define IS_MY_THREAD(t) dcethread_equal((t), my_thread)

/*
 * Some package wide statistics.
 */

INTERNAL rpc_mutex_stats_t mutex_stats = {0};
INTERNAL rpc_cond_stats_t cond_stats = {0};


/*
 * R P C _ _ M U T E X _ I N I T
 */

PRIVATE boolean rpc__mutex_init
(
    rpc_mutex_p_t mp
)
{
    mp->stats.busy = 0;
    mp->stats.lock = 0;
    mp->stats.try_lock = 0;
    mp->stats.unlock = 0;
    mp->stats.init = 1;
    mp->stats.deletes = 0;
    mp->stats.lock_assert = 0;
    mp->stats.unlock_assert = 0;
    mp->is_locked = false;
    mp->owner = NULL_THREAD;
    mp->locker_file = "never_locked";
    mp->locker_line = 0;
    if (dcethread_mutex_init(&mp->m, NULL) != 0) {
	return (false);
    }
    mutex_stats.init++;
    return(true);
}


/*
 * R P C _ _ M U T E X _ D E L E T E
 */

PRIVATE boolean rpc__mutex_delete
(
    rpc_mutex_p_t mp
)
{
    mp->stats.deletes++;
    mutex_stats.deletes++;
    dcethread_mutex_destroy(&mp->m);
    return(true);
}


/*
 * R P C _ _ M U T E X _ L O C K
 */

PRIVATE boolean rpc__mutex_lock
(
    rpc_mutex_p_t mp,
    char *file,
    int line
)
{
    dcethread* my_thread = NULL_THREAD;
    boolean is_locked = mp->is_locked;
    boolean dbg;

    dbg = RPC_DBG(rpc_es_dbg_mutex, 5);
    if (dbg) 
    {
        my_thread = dcethread_self();
        if (is_locked && IS_MY_THREAD(mp->owner))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__mutex_lock) deadlock with self at %s/%d (previous lock at %s/%d)\n",
                file, line, mp->locker_file, mp->locker_line));
            return(false);
        }
    }
    dcethread_mutex_lock(&mp->m);
    mp->is_locked = true; 
    if (dbg)
    {
        mp->owner = my_thread;
        mp->locker_file = file;
        mp->locker_line = line;
    }
    if (is_locked)
    {
        mp->stats.busy++;
        mutex_stats.busy++;
    }
    mp->stats.lock++;
    mutex_stats.lock++;
    return(true);
}


/*
 * R P C _ _ M U T E X _ T R Y _ L O C K
 */

PRIVATE boolean rpc__mutex_try_lock
(
    rpc_mutex_p_t mp,
    boolean *bp,
    char *file,
    int line
)
{
    dcethread* my_thread = NULL_THREAD;
    boolean is_locked = mp->is_locked;
    boolean dbg;

    dbg = RPC_DBG(rpc_es_dbg_mutex, 5);
    if (dbg) 
    {
        my_thread = dcethread_self();
        if (is_locked && IS_MY_THREAD(mp->owner))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__mutex_try_lock) deadlock with self at %s/%d (previous lock at %s/%d)\n",
                file, line, mp->locker_file, mp->locker_line));
            return(false);
        }
    }
    *bp = dcethread_mutex_trylock(&mp->m);
    if (*bp)
    {
        mp->is_locked = true;
        if (dbg)
        {
            mp->owner = my_thread;
            mp->locker_file = file;
            mp->locker_line = line;
        }
    }
    else
    {
        mp->stats.busy++;
        mutex_stats.busy++;
    }
    mp->stats.try_lock++;
    mutex_stats.try_lock++;
    return(true);
}


/*
 * R P C _ _ M U T E X _ U N L O C K
 */

PRIVATE boolean rpc__mutex_unlock
(
    rpc_mutex_p_t mp
)
{
    dcethread* my_thread;
    boolean is_locked = mp->is_locked;
    boolean dbg;

    dbg = RPC_DBG(rpc_es_dbg_mutex, 5);
    if (dbg) 
    {
        if (! is_locked)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__mutex_unlock) not locked\n"));
            return(false);
        }
        my_thread = dcethread_self();
        if (!IS_MY_THREAD(mp->owner))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__mutex_unlock) not owner (owner at %s/%d)\n",
                mp->locker_file, mp->locker_line));
            return(false);
        }
        mp->owner = NULL_THREAD;
    }
    mp->stats.unlock++;
    mutex_stats.unlock++;
    mp->is_locked = false;
    dcethread_mutex_unlock(&mp->m);
    return(true);
}


/*
 * R P C _ _ M U T E X _ L O C K _ A S S E R T
 *
 * assert that we are the owner of the lock.
 */

PRIVATE boolean rpc__mutex_lock_assert
(
    rpc_mutex_p_t mp
)
{
    dcethread* my_thread;
    boolean is_locked = mp->is_locked;
    boolean dbg;

    dbg = RPC_DBG(rpc_es_dbg_mutex, 5);
    mp->stats.lock_assert++;
    mutex_stats.lock_assert++;
    if (dbg)
    {
        if (! is_locked)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__mutex_lock_assert) not locked\n"));
            return(false);
        }
        my_thread = dcethread_self();
        if (!IS_MY_THREAD(mp->owner))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
	       ("(rpc__mutex_lock_assert) not owner\n"));
            return(false);
        }
    }
    return(true);
}


/*
 * R P C _ _ M U T E X _ U N L O C K _ A S S E R T
 *
 * assert that we are not the owner of the lock.
 */

PRIVATE boolean rpc__mutex_unlock_assert
(
    rpc_mutex_p_t mp
)
{
    dcethread* my_thread;
    boolean is_locked = mp->is_locked;
    boolean dbg;

    dbg = RPC_DBG(rpc_es_dbg_mutex, 5);
    mp->stats.unlock_assert++;
    mutex_stats.unlock_assert++;
    if (dbg)
    {
        if (! is_locked)
            return(true);
        my_thread = dcethread_self();
        if (IS_MY_THREAD(mp->owner))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__mutex_unlock_assert) owner\n"));
            return(false);
        }
    }
    return(true);
}


/*
 * R P C _ _ C O N D _ I N I T
 *
 * The "mp" is the mutex that is associated with the cv.
 */

boolean rpc__cond_init
(
    rpc_cond_p_t cp,
    rpc_mutex_p_t mp
)
{
    cp->stats.init = 1;
    cp->stats.deletes = 0;
    cp->stats.wait = 0;
    cp->stats.signals = 0;
    cp->mp = mp;
    dcethread_cond_init(&cp->c, NULL);
    cond_stats.init++;
    return(true);
}


/*
 * R P C _ _ C O N D _ D E L E T E
 */

boolean rpc__cond_delete
(
    rpc_cond_p_t cp,
    rpc_mutex_p_t mp ATTRIBUTE_UNUSED
)
{
    cp->stats.deletes++;
    cond_stats.deletes++;
    dcethread_cond_destroy(&cp->c);
    return(true);
}


/*
 *  R P C _ _ C O N D _ W A I T
 *
 * The mutex is automatically released and reacquired by the wait.
 */

boolean rpc__cond_wait
(
    rpc_cond_p_t cp,
    rpc_mutex_p_t mp,
    char *file,
    int line
)
{
    dcethread* my_thread;
    volatile boolean dbg;

    DO_NOT_CLOBBER(my_thread);

    cp->stats.wait++;
    cond_stats.wait++;
    dbg = RPC_DBG(rpc_es_dbg_mutex, 5);
    if (dbg)
    {
        if (! rpc__mutex_lock_assert(mp))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__cond_wait) mutex usage error\n"));
            return(false);
        }
        if (cp->mp != mp)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__cond_wait) incorrect mutex\n"));
            return(false);
        }
        my_thread = dcethread_self();
        mp->owner = NULL_THREAD;
    }
    mp->is_locked = false;
    TRY
        dcethread_cond_wait_throw(&cp->c, &mp->m);
    CATCH_ALL
        mp->is_locked = true;
        if (dbg)
        {
            mp->owner = my_thread;
            mp->locker_file = file;
            mp->locker_line = line;
        }
        RERAISE;
    ENDTRY
    mp->is_locked = true;
    if (dbg)
    {
        mp->owner = my_thread;
        mp->locker_file = file;
        mp->locker_line = line;
    }
    return(true);
}


/*
 *  R P C _ _ C O N D _ T I M E D _ W A I T
 *
 * The mutex is automatically released and reacquired by the wait.
 */

boolean rpc__cond_timed_wait
(
    rpc_cond_p_t cp,
    rpc_mutex_p_t mp,
    struct timespec *wtime,
    char *file,
    int line
)
{
    dcethread* my_thread;
    volatile boolean dbg;

    DO_NOT_CLOBBER(my_thread);

    cp->stats.wait++;
    cond_stats.wait++;
    dbg = RPC_DBG(rpc_es_dbg_mutex, 5);
    if (dbg)
    {
        if (! rpc__mutex_lock_assert(mp))
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__cond_wait) mutex usage error\n"));
            return(false);
        }
        if (cp->mp != mp)
        {
            RPC_DBG_PRINTF(rpc_e_dbg_mutex, 1,
		("(rpc__cond_wait) incorrect mutex\n"));
            return(false);
        }
        my_thread = dcethread_self();
        mp->owner = NULL_THREAD;
    }
    mp->is_locked = false;
    TRY	{
        dcethread_cond_timedwait_throw(&cp->c, &mp->m, wtime);
	}
    CATCH_ALL
        mp->is_locked = true;
        if (dbg)
        {
            mp->owner = my_thread;
            mp->locker_file = file;
            mp->locker_line = line;
        }
        RERAISE;
    ENDTRY
    mp->is_locked = true;
    if (dbg)
    {
        mp->owner = my_thread;
        mp->locker_file = file;
        mp->locker_line = line;
    }
    return(true);
}


/*
 *  R P C _ _ C O N D _ S I G N A L
 *
 * It's not clear if it's legal to signal w/o holding the lock
 * (in the runtime's context);  CMA clearly doesn't require it.
 */

boolean rpc__cond_signal
(
    rpc_cond_p_t cp,
    rpc_mutex_p_t mp ATTRIBUTE_UNUSED
)
{
    cp->stats.signals++;
    cond_stats.signals++;
    dcethread_cond_signal(&cp->c);
    return(true);
}


/*
 *  R P C _ _ C O N D _ B R O A D C A S T
 *
 * It's not clear if it's legal to broadcast w/o holding the lock
 * (in the runtime's context);  CMA clearly doesn't require it.
 */

boolean rpc__cond_broadcast
(
    rpc_cond_p_t cp,
    rpc_mutex_p_t mp ATTRIBUTE_UNUSED
)
{
    cp->stats.signals++;
    cond_stats.signals++;
    dcethread_cond_broadcast(&cp->c);
    return(true);
}


#else
#ifdef MIREK_NOT_DEFINED
INTERNAL void rpc__mutex_none (void)
{
}
#endif
#endif /* defined(RPC_MUTEX_DEBUG) || defined(RPC_MUTEX_STATS) */
