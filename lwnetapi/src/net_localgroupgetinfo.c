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
 *        net_localgroupgetinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetLocalGroupGetInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetLocalGroupGetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszAliasname,
    DWORD   dwLevel,
    PVOID  *ppBuffer
    )
{
    const DWORD dwAliasAccessFlags = ALIAS_ACCESS_LOOKUP_INFO;
    const WORD swInfoLevel = ALIAS_INFO_ALL;

    NTSTATUS status = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    ACCOUNT_HANDLE hAlias = NULL;
    DWORD dwAliasRid = 0;
    AliasInfo *pInfo = NULL;
    PVOID pSourceBuffer = NULL;
    DWORD dwSize = 0;
    DWORD dwSpaceAvailable = 0;
    PVOID pBuffer = NULL;
    PIO_CREDS pCreds = NULL;
    NET_VALIDATION_LEVEL eValidation = NET_VALIDATION_NONE;

    BAIL_ON_INVALID_PTR(pwszAliasname, err);
    BAIL_ON_INVALID_PTR(ppBuffer, err);

    if (dwLevel != 1)
    {
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
    }

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            0,
                            0,
                            pCreds);
    BAIL_ON_NT_STATUS(status);

    hSamrBinding = pConn->Rpc.Samr.hBinding;

    status = NetOpenAlias(pConn,
                          pwszAliasname,
                          dwAliasAccessFlags,
                          &hAlias,
                          &dwAliasRid);
    BAIL_ON_NT_STATUS(status);

    status = SamrQueryAliasInfo(hSamrBinding,
                                hAlias,
                                swInfoLevel,
                                &pInfo);
    BAIL_ON_NT_STATUS(status);

    pSourceBuffer = &pInfo->all;

    err = NetAllocateLocalGroupInfo(NULL,
                                    NULL,
                                    dwLevel,
                                    pSourceBuffer,
                                    &dwSize,
                                    eValidation);
    BAIL_ON_WIN_ERROR(err);

    dwSpaceAvailable = dwSize;
    dwSize           = 0;

    status = NetAllocateMemory((void**)&pBuffer,
                               dwSpaceAvailable);
    BAIL_ON_NT_STATUS(status);

    err = NetAllocateLocalGroupInfo(pBuffer,
                                    &dwSpaceAvailable,
                                    dwLevel,
                                    pSourceBuffer,
                                    &dwSize,
                                    eValidation);
    BAIL_ON_WIN_ERROR(err);

    status = SamrClose(hSamrBinding, hAlias);
    BAIL_ON_NT_STATUS(status);

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

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (pBuffer)
    {
        NetFreeMemory(pBuffer);
    }

    *ppBuffer = NULL;
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
