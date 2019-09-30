/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        netr_wkstagetinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetrWkstaGetInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERROR
NetrWkstaGetInfo(
    IN WKSS_BINDING       hBinding,
    IN PWSTR              pwszServerName,
    IN DWORD              dwLevel,
    OUT PNETR_WKSTA_INFO  pWkstaInfo
    )
{
    WINERROR winError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NETR_WKSTA_INFO WkstaInfo = {0};
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pWkstaInfo, ntStatus);

    memset(pWkstaInfo, 0, sizeof(*pWkstaInfo));
    memset(&WkstaInfo, 0, sizeof(WkstaInfo));

    DCERPC_CALL(winError, cli_NetrWkstaGetInfo(
                              (handle_t)hBinding,
                              pwszServerName,
                              dwLevel,
                              &WkstaInfo));
    BAIL_ON_WIN_ERROR(winError);

    winError = WkssAllocateNetrWkstaInfo(
                              pWkstaInfo,
                              &dwOffset,
                              NULL,
                              dwLevel,
                              &WkstaInfo,
                              &dwSize);
    BAIL_ON_WIN_ERROR(winError);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    /*
     * Allocating pInfo100 is essentially the same as allocating
     * any other infolevels since this is a union of pointers
     */
    winError = WkssAllocateMemory(OUT_PPVOID(&pWkstaInfo->pInfo100),
                                  dwSpaceLeft);
    BAIL_ON_WIN_ERROR(winError);

    winError = WkssAllocateNetrWkstaInfo(
                              pWkstaInfo,
                              &dwOffset,
                              &dwSpaceLeft,
                              dwLevel,
                              &WkstaInfo,
                              &dwSize);
    BAIL_ON_WIN_ERROR(winError);

cleanup:
    WkssCleanStubNetrWkstaInfo(&WkstaInfo, dwLevel);

    if (winError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        winError = LwNtStatusToWin32Error(ntStatus);
    }

    return winError;

error:
    if (pWkstaInfo)
    {
        /* See the above comment about allocating */
        WkssFreeMemory(pWkstaInfo->pInfo100);
        memset(pWkstaInfo, 0, sizeof(*pWkstaInfo));
    }

    goto cleanup;
}
