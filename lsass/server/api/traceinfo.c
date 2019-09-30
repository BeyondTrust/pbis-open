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
 *        traceinfo.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Trace Info Settings (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvSetTraceFlags(
    HANDLE          hServer,
    PLSA_TRACE_INFO pTraceFlagArray,
    DWORD           dwNumFlags
    )
{
    DWORD dwError = 0;
    DWORD iFlag = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (; iFlag < dwNumFlags; iFlag++)
    {
        PLSA_TRACE_INFO pTraceInfo = &pTraceFlagArray[iFlag];

        dwError = LsaTraceSetFlag_r(
                        pTraceInfo->dwTraceFlag,
                        pTraceInfo->bStatus);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvGetTraceInfo(
    HANDLE hServer,
    DWORD  dwTraceFlag,
    PLSA_TRACE_INFO* ppTraceInfo
    )
{
    DWORD dwError = 0;
    PLSA_TRACE_INFO pTraceInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LSA_TRACE_INFO),
                    (PVOID*)&pTraceInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaTraceGetInfo_r(
                    dwTraceFlag,
                    &pTraceInfo->bStatus);
    BAIL_ON_LSA_ERROR(dwError);

    pTraceInfo->dwTraceFlag = dwTraceFlag;

    *ppTraceInfo = pTraceInfo;

cleanup:

    return dwError;

error:

    *ppTraceInfo = NULL;

    LW_SAFE_FREE_MEMORY(pTraceInfo);

    goto cleanup;
}

DWORD
LsaSrvEnumTraceFlags(
    HANDLE hServer,
    PLSA_TRACE_INFO* ppTraceFlagArray,
    PDWORD pdwNumFlags
    )
{
    DWORD dwError = 0;
    DWORD iFlag = 0;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;
    DWORD dwNumFlags = LSA_TRACE_FLAG_SENTINEL - 1;

    dwError = LwAllocateMemory(
                    sizeof(LSA_TRACE_INFO) * dwNumFlags,
                    (PVOID*)&pTraceFlagArray);
    BAIL_ON_LSA_ERROR(dwError);

    for (iFlag = 1; iFlag < LSA_TRACE_FLAG_SENTINEL; iFlag++)
    {
        PLSA_TRACE_INFO pTraceInfo = &pTraceFlagArray[iFlag-1];

        pTraceInfo->dwTraceFlag = iFlag;

        dwError = LsaTraceGetInfo_r(
                        pTraceInfo->dwTraceFlag,
                        &pTraceInfo->bStatus
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppTraceFlagArray = pTraceFlagArray;
    *pdwNumFlags = dwNumFlags;

cleanup:

    return dwError;

error:

    *ppTraceFlagArray = NULL;
    *pdwNumFlags = 0;

    LW_SAFE_FREE_MEMORY(pTraceFlagArray);

    goto cleanup;
}
