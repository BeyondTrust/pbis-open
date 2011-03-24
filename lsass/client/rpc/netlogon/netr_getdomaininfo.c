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
 *        netr_getdomaininfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetrGetDomainInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
NetrGetDomainInfo(
    IN  NETR_BINDING      hBinding,
    IN  NetrCredentials  *pCreds,
    IN  PCWSTR            pwszServer,
    IN  PCWSTR            pwszComputer,
    IN  UINT32            Level,
    IN  NetrDomainQuery  *pQuery,
    OUT NetrDomainInfo  **ppDomainInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszServerName = NULL;
    PWSTR pwszComputerName = NULL;
    NetrAuth *pAuth = NULL;
    NetrAuth *pReturnedAuth = NULL;
    NetrDomainInfo DomainInfo = {0};
    NetrDomainInfo *pDomainInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pCreds, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszComputer, ntStatus);
    BAIL_ON_INVALID_PTR(pQuery, ntStatus);
    BAIL_ON_INVALID_PTR(ppDomainInfo, ntStatus);

    dwError = LwAllocateWc16String(&pwszServerName,
                                   pwszServer);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszComputerName,
                                   pwszComputer);
    BAIL_ON_WIN_ERROR(dwError);


    /* Create authenticator info with credentials chain */
    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pAuth),
                                  sizeof(NetrAuth));
    BAIL_ON_NT_STATUS(ntStatus);

    pCreds->sequence += 2;
    NetrCredentialsCliStep(pCreds);

    pAuth->timestamp = pCreds->sequence;
    memcpy(pAuth->cred.data,
           pCreds->cli_chal.data,
           sizeof(pAuth->cred.data));

    /* Allocate returned authenticator */
    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pReturnedAuth),
                                  sizeof(NetrAuth));
    BAIL_ON_NT_STATUS(ntStatus);

    DCERPC_CALL(ntStatus, cli_NetrLogonGetDomainInfo(hBinding,
                                                     pwszServerName,
                                                     pwszComputerName,
                                                     pAuth,
                                                     pReturnedAuth,
                                                     Level,
                                                     pQuery,
                                                     &DomainInfo));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateDomainInfo(NULL,
                                      &dwOffset,
                                      NULL,
                                      Level,
                                      &DomainInfo,
                                      &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pDomainInfo),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateDomainInfo(pDomainInfo,
                                      &dwOffset,
                                      &dwSpaceLeft,
                                      Level,
                                      &DomainInfo,
                                      &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppDomainInfo = pDomainInfo;

cleanup:
    NetrCleanStubDomainInfo(&DomainInfo, Level);

    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszComputerName);

    if (pAuth)
    {
        NetrFreeMemory(pAuth);
    }

    if (pReturnedAuth)
    {
        NetrFreeMemory(pReturnedAuth);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pDomainInfo)
    {
        NetrFreeMemory(pDomainInfo);
    }

    if (ppDomainInfo)
    {
        *ppDomainInfo = NULL;
    }

    goto cleanup;
}
