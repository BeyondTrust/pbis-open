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
 *        traceinfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
