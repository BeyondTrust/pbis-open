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
 *        dsr_rolegetprimarydomaininfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DsrRoleGetPrimaryDomainInformation function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERROR
DsrRoleGetPrimaryDomainInformation(
    IN  DSR_BINDING   hBinding,
    IN  WORD          swLevel,
    PDSR_ROLE_INFO   *ppInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDSR_ROLE_INFO pStubInfo = NULL;
    PDSR_ROLE_INFO pInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(ppInfo, ntStatus);

    DCERPC_CALL(dwError, cli_DsrRoleGetPrimaryDomainInformation(
                              (handle_t)hBinding,
                              swLevel,
                              &pStubInfo));
    BAIL_ON_WIN_ERROR(dwError);

    if (pStubInfo)
    {
        dwError = DsrAllocateDsRoleInfo(NULL,
                                        &dwOffset,
                                        NULL,
                                        pStubInfo,
                                        swLevel,
                                        &dwSize);
        BAIL_ON_WIN_ERROR(dwError);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        dwError = DsrAllocateMemory(OUT_PPVOID(&pInfo),
                                    dwSpaceLeft);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = DsrAllocateDsRoleInfo(pInfo,
                                        &dwOffset,
                                        &dwSpaceLeft,
                                        pStubInfo,
                                        swLevel,
                                        &dwSize);
        BAIL_ON_WIN_ERROR(dwError);
    }

    *ppInfo = pInfo;
    pInfo = NULL;

cleanup:
    if (pStubInfo)
    {
        DsrFreeStubDsRoleInfo(pStubInfo, swLevel);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return (WINERROR)dwError;

error:
    if (pInfo)
    {
        DsrFreeMemory(pInfo);
    }

    if (ppInfo)
    {
        *ppInfo = NULL;
    }

    goto cleanup;
}
