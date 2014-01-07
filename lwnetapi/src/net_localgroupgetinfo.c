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
