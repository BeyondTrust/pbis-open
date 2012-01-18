/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        net_querydisplayinformation.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetQueryDisplayInformation function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetQueryDisplayInformation(
    PCWSTR   pwszHostname,
    DWORD    dwLevel,
    DWORD    dwIndex,
    DWORD    dwEntriesRequested,
    DWORD    dwPrefMaxBufferSize,
    PDWORD   pdwNumEntries,
    PVOID   *ppBuffer
    )
{
    const DWORD dwDomainAccessFlags = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                      DOMAIN_ACCESS_OPEN_ACCOUNT;

    WINERROR winError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntRetStatus = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    WORD swLevel = (WORD)dwLevel;
    DWORD dwTotalSize = 0;
    DWORD dwReturnedSize = 0;
    SamrDisplayInfo *pInfo = NULL;
    DWORD dwSize = 0;
    DWORD dwSpaceAvailable = 0;
    PVOID pBuffer = NULL;
    NET_VALIDATION_LEVEL eValidation = NET_VALIDATION_NONE;

    BAIL_ON_INVALID_PTR(pdwNumEntries, winError);
    BAIL_ON_INVALID_PTR(ppBuffer, winError);

    if (dwLevel < 1 ||
        dwLevel > 3)
    {
        winError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(winError);
    }

    /*
     * A single call can return up to 100 entries
     */
    if (dwEntriesRequested > 100)
    {
        dwEntriesRequested = 100;
    }

    ntStatus = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetConnectSamr(&pConn,
                              pwszHostname,
                              dwDomainAccessFlags,
                              0,
                              pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    hSamrBinding = pConn->Rpc.Samr.hBinding;
    hDomain      = pConn->Rpc.Samr.hDomain;

    ntStatus = SamrQueryDisplayInfo(hSamrBinding,
                                    hDomain,
                                    swLevel,
                                    dwIndex,
                                    dwEntriesRequested,
                                    dwPrefMaxBufferSize,
                                    &dwTotalSize,
                                    &dwReturnedSize,
                                    &pInfo);
    if (ntStatus == STATUS_MORE_ENTRIES)
    {
        /*
         * Preserve the status code to return
         */
        ntRetStatus = ntStatus;
        ntStatus    = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    winError = NetAllocateDisplayInformation(NULL,
                                             NULL,
                                             dwLevel,
                                             pInfo,
                                             &dwSize,
                                             eValidation);
    BAIL_ON_WIN_ERROR(winError);

    dwSpaceAvailable = dwSize;
    dwSize           = 0;

    ntStatus = NetAllocateMemory(OUT_PPVOID(&pBuffer),
                                 dwSpaceAvailable);
    BAIL_ON_NT_STATUS(ntStatus);

    winError = NetAllocateDisplayInformation(pBuffer,
                                             &dwSpaceAvailable,
                                             dwLevel,
                                             pInfo,
                                             &dwSize,
                                             eValidation);
    BAIL_ON_WIN_ERROR(winError);

    switch (dwLevel)
    {
    case 1:
        *pdwNumEntries = pInfo->info1.count;
        break;

    case 2:
        *pdwNumEntries = pInfo->info2.count;
        break;

    case 3:
        *pdwNumEntries = pInfo->info3.count;
        break;
    }

    *ppBuffer = pBuffer;

cleanup:
    NetDisconnectSamr(&pConn);

    if (pInfo)
    {
        SamrFreeMemory(pInfo);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (ntStatus == STATUS_SUCCESS &&
        ntRetStatus != STATUS_SUCCESS)
    {
        ntStatus = ntRetStatus;
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
    }

    *ppBuffer      = NULL;
    *pdwNumEntries = 0;

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
