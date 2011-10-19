/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"


NET_API_STATUS
NetGetDomainName(
    IN  PCWSTR    pwszHostname,
    OUT PWSTR    *ppwszDomainName
    )
{
    const DWORD dwConnAccess = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PNET_CONN pConn = NULL;
    size_t sDomainNameLen = 0;
    PWSTR pwszDomainName = NULL;
    PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(ppwszDomainName, err);

    ntStatus = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetConnectSamr(&pConn, pwszHostname, dwConnAccess, 0, pCreds);
    BAIL_ON_NT_STATUS(ntStatus);
    
    err = LwWc16sLen(pConn->Rpc.Samr.pwszDomainName, &sDomainNameLen);
    BAIL_ON_WIN_ERROR(err);

    ntStatus = NetAllocateMemory(
                    OUT_PPVOID(&pwszDomainName),
                    sizeof(pwszDomainName[0]) * (sDomainNameLen + 1));
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwWc16snCpy(pwszDomainName,
                      pConn->Rpc.Samr.pwszDomainName,
                      sDomainNameLen);
    BAIL_ON_WIN_ERROR(err);

    *ppwszDomainName = pwszDomainName;

cleanup:
    NetDisconnectSamr(&pConn);
 
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(ntStatus);
    }

    return err;

error:
    if (pwszDomainName)
    {
        NetFreeMemory(pwszDomainName);
    }

    *ppwszDomainName = NULL;

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
