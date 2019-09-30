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
 *        net_joindomain.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetJoinDomain function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetJoinDomain(
    IN  PCWSTR   pwszServerName,
    IN  PCWSTR   pwszDomainName,
    IN  PCWSTR   pwszAccountOu,
    IN  PCWSTR   pwszAccountName,
    IN  PCWSTR   pwszPassword,
    IN  DWORD    dwJoinFlags
    )
{
    WINERROR err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNET_CONN pConn = NULL;
    WKSS_BINDING hWkssBinding = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszAccount = NULL;
    PWSTR pwszOu = NULL;
    PIO_CREDS pCreds = NULL;
    ENC_JOIN_PASSWORD_BUFFER PasswordBuffer;

    BAIL_ON_INVALID_PTR(pwszDomainName, err);
    BAIL_ON_INVALID_PTR(pwszAccountName, err);
    BAIL_ON_INVALID_PTR(pwszPassword, err);

    memset(&PasswordBuffer, 0, sizeof(PasswordBuffer));

    ntStatus = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetConnectWkssvc(&pConn,
                                pwszServerName,
                                pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    hWkssBinding = pConn->Rpc.WksSvc.hBinding;

    if (pwszServerName)
    {
        err = LwAllocateWc16String(&pwszServer,
                                   pwszServerName);
        BAIL_ON_WIN_ERROR(err);
    }

    err = LwAllocateWc16String(&pwszDomain,
                               pwszDomainName);
    BAIL_ON_WIN_ERROR(err);

    err = LwAllocateWc16String(&pwszAccount,
                               pwszAccountName);
    BAIL_ON_WIN_ERROR(err);

    if (pwszAccountOu)
    {
        err = LwAllocateWc16String(&pwszOu,
                                   pwszAccountOu);
        BAIL_ON_WIN_ERROR(err);
    }

    err = NetEncryptJoinPasswordBuffer(pConn,
                                       pwszPassword,
                                       &PasswordBuffer);
    BAIL_ON_WIN_ERROR(err);

    err = NetrJoinDomain2(hWkssBinding,
                          pwszServer,
                          pwszDomain,
                          pwszOu,
                          pwszAccount,
                          &PasswordBuffer,
                          dwJoinFlags);
    BAIL_ON_WIN_ERROR(err);

cleanup:
    if (pConn)
    {
        NetDisconnectWkssvc(&pConn);
    }

    memset(&PasswordBuffer, 0, sizeof(PasswordBuffer));

    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszOu);
    LW_SAFE_FREE_MEMORY(pwszAccount);

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return (NET_API_STATUS)err;

error:
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
