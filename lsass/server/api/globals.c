/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
