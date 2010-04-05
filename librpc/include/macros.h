/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        macros.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Common private macros for rpc client library
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#ifndef _LIBRPC_MACROS_H_
#define _LIBRPC_MACROS_H_

#define BAIL_ON_NT_STATUS(err)     \
    if ((err) != STATUS_SUCCESS) { \
        goto error;                \
    }

#define BAIL_ON_WIN_ERROR(err)    \
    if ((err) != ERROR_SUCCESS) { \
        goto error;               \
    }

#define BAIL_ON_RPC_STATUS(st)    \
    if ((st) != RPC_S_OK) {       \
        goto error;               \
    }

#define BAIL_ON_NO_MEMORY_RPCSTATUS(p, status)  \
    if ((p) == NULL) {                          \
        status = RPC_S_OUT_OF_MEMORY;           \
        goto error;                             \
    }

#define BAIL_ON_INVALID_PTR_RPCSTATUS(p, status)    \
    if ((p) == NULL) {                              \
        status = RPC_S_INVALID_ARG;                 \
        goto error;                                 \
    }

#define BAIL_ON_NULL_PTR(p, status)              \
    if ((p) == NULL) {                           \
        status = STATUS_INSUFFICIENT_RESOURCES;  \
        goto error;                              \
    }

#define BAIL_ON_INVALID_PTR(p, status)           \
    if ((p) == NULL) {                           \
        status = STATUS_INVALID_PARAMETER;       \
        goto error;                              \
    }

#define DCERPC_CALL(status, fn_call)             \
    do {                                         \
        dcethread_exc *dceexc;                   \
                                                 \
        DCETHREAD_TRY                            \
        {                                        \
            dceexc = NULL;                       \
            (status) = fn_call;                  \
        }                                        \
        DCETHREAD_CATCH_ALL(dceexc)              \
        {                                        \
            status = LwRpcStatusToNtStatus(dceexc->match.value); \
        }                                        \
        DCETHREAD_ENDTRY;                        \
    } while (0);


#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#define LIBRPC_LOCK_MUTEX(bInLock, pMutex)           \
    if (!bInLock) {                                  \
        int thr_err = pthread_mutex_lock(pMutex);    \
        if (thr_err) {                               \
            abort();                                 \
        }                                            \
        bInLock = TRUE;                              \
    }

#define LIBRPC_UNLOCK_MUTEX(bInLock, pMutex)        \
    if (bInLock) {                                   \
        int thr_err = pthread_mutex_unlock(pMutex);  \
        if (thr_err) {                               \
            abort();                                 \
        }                                            \
        bInLock = FALSE;                             \
    }

#define LIBRPC_LOCK_RWMUTEX_SHARED(bInLock, pMutex) \
    if (!bInLock) {                                 \
        int thr_err = pthread_rwlock_rdlock(mutex); \
        if (thr_err) {                              \
            abort();                                \
        }                                           \
        bInLock = TRUE;                             \
    }

#define LIBRPC_LOCK_RWMUTEX_EXCLUSIVE(bInLock, pMutex)  \
    if (!bInLock) {                                     \
        int thr_err = pthread_rwlock_wrlock(pMutex);    \
        if (thr_err) {                                  \
            abort();                                    \
        }                                               \
        bInLock = TRUE;                                 \
    }

#define LIBRPC_UNLOCK_RWMUTEX(bInLock, pMutex)       \
    if (bInLock) {                                   \
        int thr_err = pthread_rwlock_unlock(pMutex); \
        if (thr_err) {                               \
            abort();                                 \
        }                                            \
        bInLock = FALSE;                             \
    }



#endif /* _LIBRPC_MACROS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
