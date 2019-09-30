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
 *        dsr_enumeratedomaintrusts.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DsrEnumerateDomainTrusts function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERROR
DsrEnumerateDomainTrusts(
    IN  NETR_BINDING       hBinding,
    IN  PCWSTR             pwszServer,
    IN  UINT32             Flags,
    OUT NetrDomainTrust  **ppTrusts,
    OUT PUINT32            pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszServerName = NULL;
    NetrDomainTrustList TrustList = {0};
    NetrDomainTrust *pTrusts = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(ppTrusts, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);

    dwError = LwAllocateWc16String(&pwszServerName, pwszServer);
    BAIL_ON_WIN_ERROR(dwError);

    DCERPC_CALL_WINERR(dwError, cli_DsrEnumerateDomainTrusts(
                                        (handle_t)hBinding,
                                        pwszServerName,
                                        Flags,
                                        &TrustList));
    BAIL_ON_WIN_ERROR(dwError);

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

    *pCount   = TrustList.count;
    *ppTrusts = pTrusts;

cleanup:
    NetrCleanStubDomainTrustList(&TrustList);
    LW_SAFE_FREE_MEMORY(pwszServerName);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return (WINERROR)dwError;

error:
    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    if (pCount)
    {
        *pCount = 0;
    }

    if (ppTrusts)
    {
        *ppTrusts = NULL;
    }

    goto cleanup;
}
