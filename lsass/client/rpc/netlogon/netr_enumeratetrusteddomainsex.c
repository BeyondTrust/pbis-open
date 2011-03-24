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
 *        netr_enumeratetrusteddomainsex.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetrEnumerateTrustedDomainsEx function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NTSTATUS
NetrEnumerateTrustedDomainsEx(
    IN  NETR_BINDING      hBinding,
    IN  PCWSTR            pwszServerName,
    OUT NetrDomainTrust **ppTrusts,
    OUT PUINT32           pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszName = NULL;
    NetrDomainTrustList TrustList = {0};
    NetrDomainTrust *pTrusts = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServerName, ntStatus);
    BAIL_ON_INVALID_PTR(ppTrusts, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);

    dwError = LwAllocateWc16String(&pwszName, pwszServerName);
    BAIL_ON_WIN_ERROR(dwError);

    DCERPC_CALL(ntStatus, cli_NetrEnumerateTrustedDomainsEx(
                                           hBinding,
                                           pwszName,
                                           &TrustList));
    BAIL_ON_NT_STATUS(ntStatus);

    *pCount  = TrustList.count;

    ntStatus = NetrAllocateDomainTrusts(NULL,
                                        &dwOffset,
                                        NULL,
                                        &TrustList,
                                        &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pTrusts),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateDomainTrusts(pTrusts,
                                        &dwOffset,
                                        &dwSpaceLeft,
                                        &TrustList,
                                        &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppTrusts = pTrusts;

cleanup:
    NetrCleanStubDomainTrustList(&TrustList);
    LW_SAFE_FREE_MEMORY(pwszName);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    if (ppTrusts)
    {
        *ppTrusts = NULL;
    }

    if (pCount)
    {
        *pCount = 0;
    }

    goto cleanup;
}
