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
 *        net_unjoindomain.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetUnjoinDomain function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetUnjoinDomain(
    IN  PCWSTR  pwszServerName,
    IN  PCWSTR  pwszAccountName,
    IN  PCWSTR  pwszPassword,
    IN  DWORD   dwUnjoinFlags
    )
{
    WINERROR err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNET_CONN pConn = NULL;
    WKSS_BINDING hWkssBinding = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszAccount = NULL;
    PIO_CREDS pCreds = NULL;
    ENC_JOIN_PASSWORD_BUFFER PasswordBuffer;

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

    err = LwAllocateWc16String(&pwszAccount,
                               pwszAccountName);
    BAIL_ON_WIN_ERROR(err);

    err = NetEncryptJoinPasswordBuffer(pConn,
                                       pwszPassword,
                                       &PasswordBuffer);
    BAIL_ON_WIN_ERROR(err);

    err = NetrUnjoinDomain2(hWkssBinding,
                            pwszServer,
                            pwszAccount,
                            &PasswordBuffer,
                            dwUnjoinFlags);
    BAIL_ON_WIN_ERROR(err);

cleanup:
    if (pConn)
    {
        WkssFreeBinding(&pConn->Rpc.WksSvc.hBinding);
    }

    memset(&PasswordBuffer, 0, sizeof(PasswordBuffer));

    LW_SAFE_FREE_MEMORY(pwszServer);
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
