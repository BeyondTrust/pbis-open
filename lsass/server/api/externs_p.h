/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        externs_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Server API Externals (Library)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EXTERNS_P_H__
#define __EXTERNS_P_H__

extern time_t gServerStartTime;

/*
 * Auth Provider List
 */

extern LSA_SRV_RWLOCK gpAuthProviderList_rwlock;

#define ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock)         \
        if (!bInLock) {                                       \
           LsaSrvAcquireRead(&gpAuthProviderList_rwlock);     \
           bInLock = TRUE;                                    \
        }

#define LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bReleaseLock)    \
        if (bReleaseLock) {                                   \
           LsaSrvReleaseRead(&gpAuthProviderList_rwlock);     \
           bReleaseLock = FALSE;                              \
        }

#define ENTER_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock)         \
        if (!bInLock) {                                       \
           LsaSrvAcquireWrite(&gpAuthProviderList_rwlock);    \
           bInLock = TRUE;                                    \
        }

#define LEAVE_AUTH_PROVIDER_LIST_WRITER_LOCK(bReleaseLock)    \
        if (bReleaseLock) {                                   \
           LsaSrvReleaseWrite(&gpAuthProviderList_rwlock);    \
           bReleaseLock = FALSE;                              \
        }

extern PLSA_AUTH_PROVIDER gpAuthProviderList;


/*
 * RPC Server List
 */

extern pthread_rwlock_t gpRpcServerList_rwlock;

#define ENTER_RPC_SERVER_LIST_READER_LOCK(bInLock)            \
    if (!(bInLock)) {                                         \
        pthread_rwlock_rdlock(&gpRpcServerList_rwlock);       \
        (bInLock) = TRUE;                                     \
    }

#define LEAVE_RPC_SERVER_LIST_READER_LOCK(bInLock)            \
    if (!(bInLock)) {                                         \
        pthread_rwlock_unlock(&gpRpcServerList_rwlock);       \
        bInLock = FALSE;                                      \
    }

#define ENTER_RPC_SERVER_LIST_WRITER_LOCK(bInLock)            \
    if (!(bInLock)) {                                         \
        pthread_rwlock_wrlock(&gpRpcServerList_rwlock);       \
        (bInLock) = TRUE;                                     \
    }

#define LEAVE_RPC_SERVER_LIST_WRITER_LOCK(bInLock)            \
    if (!(bInLock)) {                                         \
        pthread_rwlock_unlock(&gpRpcServerList_rwlock);       \
        bInLock = FALSE;                                      \
    }

extern PLSA_RPC_SERVER gpRpcServerList;


extern pthread_rwlock_t gPerfCounters_rwlock;
extern UINT64 gPerfCounters[LsaMetricSentinel];

extern pthread_mutex_t    gAPIConfigLock;

extern LSA_SRV_API_CONFIG gAPIConfig;

#ifdef ENABLE_EVENTLOG
extern EVENTLOG_THREAD_STATE gEventLogState;
extern EVENT_LOG_RECORD_QUEUE gEventLogQueues[2];
#endif

#endif /* __EXTERNS_P_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
