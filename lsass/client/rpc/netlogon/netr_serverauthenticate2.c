/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
