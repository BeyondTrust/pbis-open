/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        globals.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Server API Globals
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

time_t gServerStartTime = 0;

PLSA_AUTH_PROVIDER gpAuthProviderList = NULL;
LSA_SRV_RWLOCK gpAuthProviderList_rwlock;

PLSA_RPC_SERVER gpRpcServerList = NULL;
pthread_rwlock_t gpRpcServerList_rwlock;

pthread_t gRpcSrvWorker;

pthread_rwlock_t gPerfCounters_rwlock;
UINT64 gPerfCounters[LsaMetricSentinel];

pthread_mutex_t    gAPIConfigLock     = PTHREAD_MUTEX_INITIALIZER;
LSA_SRV_API_CONFIG gAPIConfig = {0};

PLW_MAP_SECURITY_CONTEXT gpLsaSecCtx;

// Do not directly access this. Use gEventLogState
#ifdef ENABLE_EVENTLOG
EVENT_LOG_RECORD_QUEUE gEventLogQueues[2] = { {0}, {0} };
EVENTLOG_THREAD_STATE gEventLogState = { (pthread_t)(size_t)-1, PTHREAD_COND_INITIALIZER, 0, PTHREAD_MUTEX_INITIALIZER, &gEventLogQueues[0] };
#endif

DWORD
LsaSrvApiInit(
    PLSA_STATIC_PROVIDER pStaticProviders
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_SRV_API_CONFIG apiConfig = {0};

    gServerStartTime = time(NULL);

    pthread_rwlock_init(&gPerfCounters_rwlock, NULL);

    memset(&gPerfCounters[0], 0, sizeof(gPerfCounters));

    LsaSrvInitializeLock(&gpAuthProviderList_rwlock);

    pthread_rwlock_init(&gpRpcServerList_rwlock, NULL);

    dwError = LsaSrvApiInitConfig(&gAPIConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiReadRegistry(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiTransferConfigContents(
                    &apiConfig,
                    &gAPIConfig);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LwMapSecurityCreateContext(&gpLsaSecCtx);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvInitPrivileges();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvInitAuthProviders(pStaticProviders);
    BAIL_ON_LSA_ERROR(dwError);

#ifndef DISABLE_RPC_SERVERS
    dwError = LsaSrvInitRpcServers();
    BAIL_ON_LSA_ERROR(dwError);
#endif

cleanup:

    LsaSrvApiFreeConfigContents(&apiConfig);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvApiShutdown(
    VOID
    )
{
    LsaSrvFreeAuthProviders();

    LsaSrvFreeRpcServers();

    LsaSrvFreePrivileges();

    pthread_mutex_lock(&gAPIConfigLock);

    LsaSrvApiFreeConfigContents(&gAPIConfig);

    pthread_mutex_unlock(&gAPIConfigLock);

    return 0;


}
