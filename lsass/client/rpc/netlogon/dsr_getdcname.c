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
