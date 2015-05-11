/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
