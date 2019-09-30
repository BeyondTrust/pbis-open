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

#include "includes.h"

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        net_userdel.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetUserDel function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


NET_API_STATUS
NetUserDel(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername
    )
{
    const DWORD dwUserAccess = DELETE;

    NTSTATUS status = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwUserRid = 0;
    PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pwszUsername, err);

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(status);
        
    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            0,
                            0,
                            pCreds);
    BAIL_ON_NT_STATUS(status);

    hSamrBinding = pConn->Rpc.Samr.hBinding;

    status = NetOpenUser(pConn,
                         pwszUsername,
                         dwUserAccess,
                         &hUser,
                         &dwUserRid);
    if (status == STATUS_NONE_MAPPED)
    {
        err = NERR_UserNotFound;
        BAIL_ON_WIN_ERROR(err);
    }
    BAIL_ON_NT_STATUS(status);

    status = SamrDeleteUser(hSamrBinding, hUser);
    BAIL_ON_NT_STATUS(status);

cleanup:
    NetDisconnectSamr(&pConn);

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
