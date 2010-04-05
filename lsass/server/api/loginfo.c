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
 *        loginfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Log Info Settings (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvGetLogInfo(
    HANDLE hServer,
    PLSA_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLSA_LOG_INFO pLogInfo = NULL;

    BAIL_ON_INVALID_POINTER(ppLogInfo);

    dwError = LsaLogGetInfo_r(&pLogInfo);
    BAIL_ON_LSA_ERROR(dwError);

    *ppLogInfo = pLogInfo;

cleanup:

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "get log info");

    *ppLogInfo = NULL;

    if (pLogInfo)
    {
        LsaFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
LsaSrvSetLogInfo(
    HANDLE hServer,
    PLSA_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    BAIL_ON_INVALID_POINTER(pLogInfo);

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLogSetInfo_r(pLogInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "set log info (level = %d)", pLogInfo ? pLogInfo->maxAllowedLogLevel : -1);

    goto cleanup;
}
