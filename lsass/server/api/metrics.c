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
 *        metrics.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Metrics (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "api.h"

DWORD
LsaSrvGetMetrics(
    HANDLE hServer,
    DWORD  dwInfoLevel,
    PVOID* ppMetricPack
    )
{
    DWORD dwError = 0;
    PVOID pMetricPack = NULL;

    BAIL_ON_INVALID_POINTER(ppMetricPack);

    switch(dwInfoLevel)
    {
        case 0:

            dwError = LsaSrvGetMetrics_0(
                            &pMetricPack);
            break;

        case 1:

            dwError = LsaSrvGetMetrics_1(
                            &pMetricPack);
            break;

        default:

            dwError = LW_ERROR_INVALID_METRIC_INFO_LEVEL;
            break;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppMetricPack = pMetricPack;

cleanup:

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "get metrics (level=%u)", dwInfoLevel);

    *ppMetricPack = NULL;

    LW_SAFE_FREE_MEMORY(pMetricPack);

    goto cleanup;
}

DWORD
LsaSrvGetMetrics_0(
    PVOID* ppMetricPack
    )
{
    DWORD dwError = 0;
    PLSA_METRIC_PACK_0 pMetricPack = NULL;

    pthread_rwlock_rdlock(&gPerfCounters_rwlock);

    dwError = LwAllocateMemory(
                  sizeof(LSA_METRIC_PACK_0),
                  (PVOID*)&pMetricPack);
    BAIL_ON_LSA_ERROR(dwError);

    pMetricPack->failedAuthentications =
                 gPerfCounters[LsaMetricFailedAuthentications] ;
    pMetricPack->failedUserLookupsByName =
                 gPerfCounters[LsaMetricFailedUserLookupsByName];
    pMetricPack->failedUserLookupsById =
                 gPerfCounters[LsaMetricFailedUserLookupsById];
    pMetricPack->failedGroupLookupsByName =
                 gPerfCounters[LsaMetricFailedGroupLookupsByName];
    pMetricPack->failedGroupLookupsById =
                 gPerfCounters[LsaMetricFailedGroupLookupsById];
    pMetricPack->failedOpenSession =
                 gPerfCounters[LsaMetricFailedOpenSession];
    pMetricPack->failedCloseSession =
                 gPerfCounters[LsaMetricFailedCloseSession];
    pMetricPack->failedChangePassword =
                 gPerfCounters[LsaMetricFailedChangePassword];

    *ppMetricPack = pMetricPack;

cleanup:

    pthread_rwlock_unlock(&gPerfCounters_rwlock);

    return dwError;

error:

    *ppMetricPack = NULL;

    LW_SAFE_FREE_MEMORY(pMetricPack);

    goto cleanup;
}

DWORD
LsaSrvGetMetrics_1(
    PVOID* ppMetricPack
    )
{
    DWORD dwError = 0;
    PLSA_METRIC_PACK_1 pMetricPack = NULL;

    pthread_rwlock_rdlock(&gPerfCounters_rwlock);

    dwError = LwAllocateMemory(
                  sizeof(LSA_METRIC_PACK_1),
                  (PVOID*)&pMetricPack);
    BAIL_ON_LSA_ERROR(dwError);

    pMetricPack->successfulAuthentications =
                 gPerfCounters[LsaMetricSuccessfulAuthentications];
    pMetricPack->failedAuthentications =
                 gPerfCounters[LsaMetricFailedAuthentications];
    pMetricPack->rootUserAuthentications =
                 gPerfCounters[LsaMetricRootUserAuthentications];
    pMetricPack->successfulUserLookupsByName =
                 gPerfCounters[LsaMetricSuccessfulUserLookupsByName];
    pMetricPack->failedUserLookupsByName =
                 gPerfCounters[LsaMetricFailedUserLookupsByName];
    pMetricPack->successfulUserLookupsById =
                 gPerfCounters[LsaMetricSuccessfulUserLookupsById];
    pMetricPack->failedUserLookupsById =
                 gPerfCounters[LsaMetricFailedUserLookupsById];
    pMetricPack->successfulGroupLookupsByName =
                 gPerfCounters[LsaMetricSuccessfulGroupLookupsByName];
    pMetricPack->failedGroupLookupsByName =
                 gPerfCounters[LsaMetricFailedGroupLookupsByName];
    pMetricPack->successfulGroupLookupsById =
                 gPerfCounters[LsaMetricSuccessfulGroupLookupsById];
    pMetricPack->failedGroupLookupsById =
                 gPerfCounters[LsaMetricFailedGroupLookupsById];
    pMetricPack->successfulOpenSession =
                 gPerfCounters[LsaMetricSuccessfulOpenSession];
    pMetricPack->failedOpenSession =
                 gPerfCounters[LsaMetricFailedOpenSession];
    pMetricPack->successfulCloseSession =
                 gPerfCounters[LsaMetricSuccessfulCloseSession];
    pMetricPack->failedCloseSession =
                 gPerfCounters[LsaMetricFailedCloseSession];
    pMetricPack->successfulChangePassword =
                 gPerfCounters[LsaMetricSuccessfulChangePassword];
    pMetricPack->failedChangePassword =
                 gPerfCounters[LsaMetricFailedChangePassword];

    *ppMetricPack = pMetricPack;

cleanup:

    pthread_rwlock_unlock(&gPerfCounters_rwlock);

    return dwError;

error:

    *ppMetricPack = NULL;

    LW_SAFE_FREE_MEMORY(pMetricPack);

    goto cleanup;
}

VOID
LsaSrvIncrementMetricValue(
    LsaMetricType metricType
    )
{
    pthread_rwlock_wrlock(&gPerfCounters_rwlock);

    gPerfCounters[metricType]++;

    pthread_rwlock_unlock(&gPerfCounters_rwlock);
}

VOID
LsaSrvFreeIpcMetriPack(
    PLSA_METRIC_PACK pMetricPack
    )
{
    if (pMetricPack)
    {
        switch (pMetricPack->dwInfoLevel)
        {
            case 0:
                LW_SAFE_FREE_MEMORY(pMetricPack->pMetricPack.pMetricPack0);
                break;
            case 1:
                LW_SAFE_FREE_MEMORY(pMetricPack->pMetricPack.pMetricPack1);
                break;
            default:
                {
                    LSA_LOG_ERROR("Unsupported Metric Pack Info Level [%u]", pMetricPack->dwInfoLevel);
                }
        }
        LwFreeMemory(pMetricPack);
    }
}
