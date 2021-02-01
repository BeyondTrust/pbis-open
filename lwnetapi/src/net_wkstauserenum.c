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
 *        net_wkstauserenum.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetWkstaUserEnum function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetWkstaUserEnum(
    PWSTR   pwszHostname,
    DWORD   dwLevel,
    PVOID  *ppBuffer,
    DWORD   dwPrefMaxLen,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalNumEntries,
    PDWORD  pdwResume
    )
{
    WINERROR winError = ERROR_SUCCESS;
    WINERROR winRetError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNET_CONN pConn = NULL;
    WKSS_BINDING hWkssvcBinding = NULL;
    PVOID pInfo = NULL;
    DWORD dwSize = 0;
    DWORD dwNumEntries = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD dwResume = 0;
    LW_PIO_CREDS pCreds = NULL;
    PVOID pBuffer = NULL;

    BAIL_ON_INVALID_PTR(ppBuffer, winError);
    BAIL_ON_INVALID_PTR(pdwNumEntries, winError);
    BAIL_ON_INVALID_PTR(pdwTotalNumEntries, winError);

    ntStatus = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pdwResume)
    {
        dwResume = *pdwResume;
    }

    winError = NetConnectWkssvc(&pConn,
                                pwszHostname,
                                pCreds);
    BAIL_ON_WIN_ERROR(winError);

    hWkssvcBinding = pConn->Rpc.WksSvc.hBinding;

    winError = NetrWkstaUserEnum(hWkssvcBinding,
                                 pwszHostname,
                                 dwLevel,
                                 dwPrefMaxLen,
                                 &pInfo,
                                 &dwSize,
                                 &dwNumEntries,
                                 &dwTotalNumEntries,
                                 &dwResume);
    if (winError == ERROR_MORE_DATA)
    {
        winRetError = winError;
        winError    = ERROR_SUCCESS;
    }
    BAIL_ON_WIN_ERROR(winError);

    winError = NetAllocateMemory(OUT_PPVOID(&pBuffer),
                                 dwSize);
    BAIL_ON_WIN_ERROR(winError);

    /*
     * Data structures returned from NetrWkstaUserEnum and
     * NetWkstaUserEnum are essentially the same so simply
     * a memcpy will do.
     */
    memcpy(pBuffer, pInfo, dwSize);

cleanup:
    *ppBuffer = pBuffer;
    *pdwNumEntries = dwNumEntries;
    *pdwTotalNumEntries = dwTotalNumEntries;
    
    if (pdwResume)
    {
        *pdwResume = dwResume;
    }

    NetDisconnectWkssvc(&pConn);

    if (pInfo)
    {
        WkssFreeMemory(pInfo);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

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

    return (NET_API_STATUS)winError;

error:
    if (pBuffer)
    {
        NetFreeMemory(pBuffer);
        pBuffer = NULL;
    }

    dwNumEntries      = 0;
    dwTotalNumEntries = 0;
    dwResume          = 0;

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
