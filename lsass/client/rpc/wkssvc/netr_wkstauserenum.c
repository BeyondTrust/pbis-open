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
 *        netr_wkstauserenum.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetrWkstaUserEnum function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERROR
NetrWkstaUserEnum(
    IN  WKSS_BINDING    hBinding,
    IN  PWSTR           pwszServerName,
    IN  DWORD           dwLevel,
    IN  DWORD           dwPrefMaxLen,
    OUT PVOID          *ppInfo,
    OUT PDWORD          pdwSize,
    OUT PDWORD          pdwNumEntries,
    OUT PDWORD          pdwTotalNumEntries,
    IN OUT PDWORD       pdwResume
    )
{
    WINERROR winError = ERROR_SUCCESS;
    WINERROR winRetError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NETR_WKSTA_USER_INFO WkstaUserInfo = {0};
    NETR_WKSTA_USER_INFO_CTR_0 InfoCtr0 = {0};
    NETR_WKSTA_USER_INFO_CTR_1 InfoCtr1 = {0};
    DWORD dwNumEntries = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD dwResume = 0;
    PVOID pInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    memset(&WkstaUserInfo, 0, sizeof(WkstaUserInfo));

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(ppInfo, ntStatus);

    WkstaUserInfo.dwLevel = dwLevel;

    switch (dwLevel)
    {
    case 0:
        WkstaUserInfo.Ctr.pInfo0 = &InfoCtr0;
        break;

    case 1:
        WkstaUserInfo.Ctr.pInfo1 = &InfoCtr1;
        break;

    default:
        winError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(winError);
    }

    if (pdwResume)
    {
        dwResume = *pdwResume;
    }

    DCERPC_CALL(winError, cli_NetrWkstaUserEnum(
                              (handle_t)hBinding,
                              pwszServerName,
                              &WkstaUserInfo,
                              dwPrefMaxLen,
                              &dwTotalNumEntries,
                              &dwResume));
    if (winError == ERROR_MORE_DATA)
    {
        /*
         * Preserve the status code to return
         */
        winRetError = winError;
        winError    = ERROR_SUCCESS;
    }
    BAIL_ON_WIN_ERROR(winError);

    winError = WkssAllocateNetrWkstaUserInfo(NULL,
                                             &dwOffset,
                                             NULL,
                                             WkstaUserInfo.dwLevel,
                                             &WkstaUserInfo.Ctr,
                                             &dwSize);
    BAIL_ON_WIN_ERROR(winError);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    winError = WkssAllocateMemory(OUT_PPVOID(&pInfo),
                                  dwSpaceLeft);
    BAIL_ON_WIN_ERROR(winError);

    winError = WkssAllocateNetrWkstaUserInfo(pInfo,
                                             &dwOffset,
                                             &dwSpaceLeft,
                                             WkstaUserInfo.dwLevel,
                                             &WkstaUserInfo.Ctr,
                                             &dwSize);
    BAIL_ON_WIN_ERROR(winError);

    switch (dwLevel)
    {
    case 0:
        dwNumEntries = WkstaUserInfo.Ctr.pInfo0->dwCount;
        break;

    case 1:
        dwNumEntries = WkstaUserInfo.Ctr.pInfo1->dwCount;
        break;

    default:
        winError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(winError);
        break;
    }

    *ppInfo             = pInfo;
    *pdwNumEntries      = dwNumEntries;
    *pdwTotalNumEntries = dwTotalNumEntries;

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

    if (pdwResume)
    {
        *pdwResume = dwResume;
    }

cleanup:
    WkssCleanStubNetrWkstaUserInfo(&WkstaUserInfo);

    if (winError == ERROR_SUCCESS &&
        winRetError != ERROR_SUCCESS)
    {
        winError = winRetError;
    }

    if (winError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        winError = LwNtStatusToWin32Error(ntStatus);
    }

    return winError;

error:
    if (pInfo)
    {
        WkssFreeMemory(pInfo);
        pInfo = NULL;
    }

    if (ppInfo)
    {
        *ppInfo = NULL;
    }

    if (pdwSize)
    {
        *pdwSize = 0;
    }

    if (pdwNumEntries)
    {
        *pdwNumEntries = 0;
    }

    if (pdwTotalNumEntries)
    {
        *pdwTotalNumEntries = 0;
    }

    goto cleanup;
}
