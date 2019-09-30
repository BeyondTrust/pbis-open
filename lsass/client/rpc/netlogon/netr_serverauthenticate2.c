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
 *        netr_serverauthenticate2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetrServerAuthenticate2 function.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NTSTATUS
NetrServerAuthenticate2(
    IN  NETR_BINDING   hBinding,
    IN  PCWSTR         pwszServer,
    IN  PCWSTR         pwszAccount,
    IN  UINT16         SchannelType,
    IN  PCWSTR         pwszComputer,
    IN  BYTE           CliCreds[8],
    IN  BYTE           SrvCreds[8],
    IN OUT PUINT32     pNegFlags
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    NetrCred Creds;
    PWSTR pwszServerName = NULL;
    PWSTR pwszAccountName = NULL;
    PWSTR pwszComputerName = NULL;
    UINT32 Flags = 0;

    memset(&Creds, 0, sizeof(Creds));

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccount, ntStatus);
    BAIL_ON_INVALID_PTR(pwszComputer, ntStatus);
    BAIL_ON_INVALID_PTR(CliCreds, ntStatus);
    BAIL_ON_INVALID_PTR(SrvCreds, ntStatus);
    BAIL_ON_INVALID_PTR(pNegFlags, ntStatus);

    memcpy(Creds.data, CliCreds, sizeof(Creds.data));

    dwError = LwAllocateWc16String(&pwszServerName,
                                   pwszServer);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszAccountName,
                                   pwszAccount);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszComputerName,
                                   pwszComputer);
    BAIL_ON_WIN_ERROR(dwError);

    Flags = *pNegFlags;

    DCERPC_CALL(ntStatus, cli_NetrServerAuthenticate2(hBinding,
                                                      pwszServerName,
                                                      pwszAccountName,
                                                      SchannelType,
                                                      pwszComputerName,
                                                      &Creds,
                                                      &Flags));
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(SrvCreds, Creds.data, sizeof(Creds.data));

    *pNegFlags = Flags;

cleanup:
    memset(&Creds, 0, sizeof(Creds));

    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszAccountName);
    LW_SAFE_FREE_MEMORY(pwszComputerName);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (SrvCreds)
    {
        memset(SrvCreds, 0, sizeof(Creds.data));
    }

    if (pNegFlags)
    {
        *pNegFlags = 0;
    }

    goto cleanup;
}
