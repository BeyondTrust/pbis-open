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
