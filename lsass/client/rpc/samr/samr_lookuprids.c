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
 *        samr_lookuprids.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrLookupRids function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrLookupRids(
    IN  SAMR_BINDING    hBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  UINT32          NumRids,
    IN  UINT32         *pRids,
    OUT PWSTR         **pppwszNames,
    OUT UINT32        **ppTypes
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntLookupStatus = STATUS_SUCCESS;
    UNICODE_STRING_ARRAY Names = {0};
    IDS Types = {0};
    PWSTR *ppwszNames = NULL;
    UINT32 *pTypes = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pRids, ntStatus);
    BAIL_ON_INVALID_PTR(pppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppTypes, ntStatus);

    DCERPC_CALL(ntStatus, cli_SamrLookupRids((handle_t)hBinding,
                                             hDomain,
                                             NumRids,
                                             pRids,
                                             &Names,
                                             &Types));
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != STATUS_SOME_NOT_MAPPED)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntLookupStatus = ntStatus;

    if (Names.dwCount > 0)
    {
        ntStatus = SamrAllocateNamesFromUnicodeStringArray(
                                      NULL,
                                      &dwOffset,
                                      NULL,
                                      &Names,
                                      &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&ppwszNames),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateNamesFromUnicodeStringArray(
                                      ppwszNames,
                                      &dwOffset,
                                      &dwSpaceLeft,
                                      &Names,
                                      &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = sizeof(pTypes[0]) * Types.dwCount;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&pTypes),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateIds(pTypes,
                                   &dwOffset,
                                   &dwSpaceLeft,
                                   &Types,
                                   &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pppwszNames = ppwszNames;
    *ppTypes     = pTypes;

cleanup:
    SamrCleanStubIds(&Types);
    SamrCleanStubUnicodeStringArray(&Names);

    if (ntStatus == STATUS_SUCCESS &&
        ntLookupStatus != STATUS_SUCCESS)
    {
        ntStatus = ntLookupStatus;
    }

    return ntStatus;

error:
    if (ppwszNames)
    {
        SamrFreeMemory(ppwszNames);
    }

    if (pTypes)
    {
        SamrFreeMemory(pTypes);
    }

    if (pppwszNames)
    {
        *pppwszNames = NULL;
    }

    if (ppTypes)
    {
        *ppTypes = NULL;
    }

    goto cleanup;
}
