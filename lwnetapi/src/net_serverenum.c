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
 *        net_serverenum.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetServerEnum function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"
#include <lsa/ad.h>


NET_API_STATUS
NetServerEnum(
    PCWSTR   pwszHostname,
    DWORD    dwLevel,
    PVOID   *ppBuffer,
    DWORD    dwPrefMaxLen,
    PDWORD   pdwNumEntries,
    PDWORD   pdwTotalNumEntries,
    DWORD    dwServerType,
    PCWSTR   pwszDomain,
    PDWORD   pdwResume
    )
{
    WINERROR winError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pBuffer = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD dwResume = 0;
    PSTR pszDomain = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszSiteName = NULL;
    DWORD dwFlags = 0;
    PLWNET_DC_ADDRESS pDcList = NULL;
    DWORD dwDcCount = 0;
    PVOID *ppInfo = NULL;
    DWORD iDc = 0;
    PWSTR pwszDcName = NULL;
    PSRVSVC_CONTEXT pSrvsCtx = NULL;
    DWORD dwQueryInfoLevel = 101;
    PVOID pSrvInfo = NULL;
    DWORD dwInfoLevelSize = 0;
    DWORD dwSize = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwTotalSize = 0;
    PVOID pBufferCursor = NULL;
    NET_VALIDATION_LEVEL eValidation = NET_VALIDATION_NONE;
    HANDLE hLsaConnection = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    BAIL_ON_INVALID_PTR(ppBuffer, winError);
    BAIL_ON_INVALID_PTR(pdwNumEntries, winError);
    BAIL_ON_INVALID_PTR(pdwTotalNumEntries, winError);

    /*
     * We can only handle a call to the local server
     */
    if (pwszHostname != NULL)
    {
        winError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(winError);
    }

    if (!(dwLevel == 100 ||
          dwLevel == 101))
    {
        winError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(winError);
    }

    /*
     * We can type of servers we can look for is a domain controller
     */
    if (!(dwServerType & SV_TYPE_DOMAIN_CTRL))
    {
        winError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(winError);
    }

    if (pwszDomain)
    {
        winError = LwWc16sToMbs(pwszDomain,
                                &pszDomain);
        BAIL_ON_WIN_ERROR(winError);

        pszDomainName = pszDomain;
    }
    else
    {
        winError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_WIN_ERROR(winError);

        winError = LsaAdGetMachineAccountInfo(hLsaConnection,
                                              NULL,
                                              &pAccountInfo);
        if (winError == NERR_SetupNotJoined)
        {
            winError = ERROR_BAD_NETPATH;
        }
        BAIL_ON_WIN_ERROR(winError);

        pszDomainName = pAccountInfo->DnsDomainName;
    }

    if (pdwResume)
    {
        dwResume = *pdwResume;
    }

    dwFlags = DS_DIRECTORY_SERVICE_REQUIRED |
              DS_RETURN_DNS_NAME;

    winError = LWNetGetDCList(pszDomainName,
                              pszSiteName,
                              dwFlags,
                              &pDcList,
                              &dwDcCount);
    if (winError == DNS_ERROR_BAD_PACKET)
    {
        winError = ERROR_NO_BROWSER_SERVERS_FOUND;
    }
    BAIL_ON_WIN_ERROR(winError);

    winError = LwAllocateMemory(sizeof(pSrvInfo) * dwDcCount,
                                OUT_PPVOID(&ppInfo));
    BAIL_ON_WIN_ERROR(winError);

    for (iDc = 0; iDc < dwDcCount; iDc++)
    {
        winError = LwMbsToWc16s(pDcList[iDc].pszDomainControllerName,
                                &pwszDcName);
        BAIL_ON_WIN_ERROR(winError);

        winError = SrvSvcCreateContext(pwszDcName,
                                       &pSrvsCtx);
        BAIL_ON_WIN_ERROR(winError);

        /*
         * Querying infolevel 101 (regarless of requested level)
         * because it allows checking the server type flags field
         */
        winError = NetrServerGetInfo(pSrvsCtx,
                                     pwszDcName,
                                     dwQueryInfoLevel,
                                     (PBYTE*)&pSrvInfo);
        BAIL_ON_WIN_ERROR(winError);

        if (((PSERVER_INFO_101)pSrvInfo)->sv101_type & dwServerType)
        {
            ppInfo[iDc] = pSrvInfo;
            dwTotalNumEntries++;
        }

        LW_SAFE_FREE_MEMORY(pwszDcName);
        SrvSvcCloseContext(pSrvsCtx);

        pSrvsCtx   = NULL;
        pwszDcName = NULL;
    }

    for (iDc = 0; iDc + dwResume < dwTotalNumEntries; iDc++)
    {
        dwSize = 0;
        winError = NetAllocateServerInfo(NULL,
                                         NULL,
                                         dwLevel,
                                         ppInfo[iDc + dwResume],
                                         &dwSize,
                                         eValidation);
        BAIL_ON_WIN_ERROR(winError);

        dwTotalSize += dwSize;
        dwNumEntries++;

        if (dwTotalSize > dwPrefMaxLen)
        {
            dwTotalSize -= dwSize;
            dwNumEntries--;
            break;
        }
    }

    if (dwTotalNumEntries > 0 && dwNumEntries == 0)
    {
        winError = NERR_BufTooSmall;
        BAIL_ON_WIN_ERROR(winError);
    }

    ntStatus = NetAllocateMemory(OUT_PPVOID(&pBuffer),
                                 dwTotalSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSize           = 0;
    dwSpaceLeft      = dwTotalSize;
    pBufferCursor    = pBuffer;

    switch (dwLevel)
    {
    case 100:
        dwInfoLevelSize = sizeof(SERVER_INFO_100);
        break;

    case 101:
        dwInfoLevelSize = sizeof(SERVER_INFO_101);
        break;

    default:
        winError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(winError);
        break;
    }

    for (iDc = 0; iDc < dwNumEntries; iDc++)
    {
        pBufferCursor = pBuffer + (iDc * dwInfoLevelSize);

        winError = NetAllocateServerInfo(pBufferCursor,
                                         &dwSpaceLeft,
                                         dwLevel,
                                         ppInfo[iDc + dwResume],
                                         &dwSize,
                                         eValidation);
        BAIL_ON_WIN_ERROR(winError);
    }

    if (dwResume + dwNumEntries < dwTotalNumEntries)
    {
        winError = ERROR_MORE_DATA;
    }

    *ppBuffer           = pBuffer;
    *pdwNumEntries      = dwNumEntries;
    *pdwTotalNumEntries = dwTotalNumEntries;
    
    if (pdwResume)
    {
        *pdwResume = dwResume;
    }

cleanup:
    if (pSrvsCtx)
    {
        SrvSvcCloseContext(pSrvsCtx);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    for (iDc = 0; iDc < dwDcCount; iDc++)
    {
        if (ppInfo[iDc])
        {
            SrvSvcFreeMemory(ppInfo[iDc]);
        }
    }
    LW_SAFE_FREE_MEMORY(ppInfo);

    LW_SAFE_FREE_MEMORY(pszDomain);
    LsaAdFreeMachineAccountInfo(pAccountInfo);
    pszDomainName = NULL;

    LWNetFreeDCList(pDcList, dwDcCount);

    LW_SAFE_FREE_MEMORY(pwszDcName);

    if (winError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        winError = LwNtStatusToWin32Error(ntStatus);
    }

    return (NET_API_STATUS)winError;

error:
    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pdwNumEntries)
    {
        *pdwNumEntries = 0;
    }

    if (pdwTotalNumEntries)
    {
        *pdwTotalNumEntries = 0;
    }

    if (pdwResume)
    {
        *pdwResume = 0;
    }

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
