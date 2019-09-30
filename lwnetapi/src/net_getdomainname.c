/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
