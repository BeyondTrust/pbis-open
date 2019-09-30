/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        externs_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
