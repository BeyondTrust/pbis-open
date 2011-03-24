/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2009
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
 *        dsr_getdcname.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DsrGetDcName function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERROR
DsrGetDcName(
    IN  NETR_BINDING    hBinding,
    IN  PCWSTR          pwszServerName,
    IN  PCWSTR          pwszDomainName,
    IN  const PGUID     pDomainGuid,
    IN  const PGUID     pSiteGuid,
    IN  DWORD           dwGetDcFlags,
    OUT DsrDcNameInfo **ppInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszServer = NULL;
    PWSTR pwszDomain = NULL;
    PGUID pDomainGuidCopy = NULL;
    PGUID pSiteGuidCopy = NULL;
    DsrDcNameInfo *pDcInfo = NULL;
    DsrDcNameInfo *pDcRetInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServerName, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomainName, ntStatus);
    BAIL_ON_INVALID_PTR(ppInfo, ntStatus);

    dwError = LwAllocateWc16String(&pwszServer,
                                   pwszServerName);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszDomain,
                                   pwszDomainName);
    BAIL_ON_WIN_ERROR(dwError);

    if (pDomainGuid)
    {
        ntStatus = NetrAllocateMemory(OUT_PPVOID(&pDomainGuidCopy),
                                      sizeof(*pDomainGuidCopy));
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pDomainGuidCopy,
               pDomainGuid,
               sizeof(*pDomainGuidCopy));
    }

    if (pSiteGuid)
    {
        ntStatus = NetrAllocateMemory(OUT_PPVOID(&pSiteGuidCopy),
                                      sizeof(*pSiteGuidCopy));
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pSiteGuidCopy,
               pSiteGuid,
               sizeof(*pSiteGuid));
    }

    DCERPC_CALL_WINERR(dwError, cli_DsrGetDcName(hBinding,
                                          pwszServer,
                                          pwszDomain,
                                          pDomainGuidCopy,
                                          pSiteGuidCopy,
                                          dwGetDcFlags,
                                          &pDcInfo));
    BAIL_ON_WIN_ERROR(dwError);

    ntStatus = NetrAllocateDcNameInfo(NULL,
                                      &dwOffset,
                                      NULL,
                                      pDcInfo,
                                      &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pDcRetInfo),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateDcNameInfo(pDcRetInfo,
                                      &dwOffset,
                                      &dwSpaceLeft,
                                      pDcInfo,
                                      &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppInfo = pDcRetInfo;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszDomain);

    if (pDomainGuidCopy)
    {
        NetrFreeMemory(pDomainGuidCopy);
    }

    if (pSiteGuidCopy)
    {
        NetrFreeMemory(pSiteGuidCopy);
    }

    if (pDcInfo)
    {
        NetrFreeStubDcNameInfo(pDcInfo);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return (WINERROR)dwError;

error:
    if (pDcRetInfo)
    {
        NetrFreeMemory(pDcRetInfo);
    }

    if (ppInfo)
    {
        *ppInfo = NULL;
    }

    goto cleanup;
}
