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
 *        lsa_enumaccounts.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaEnumAccounts function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaEnumAccounts(
    IN LSA_BINDING    hBinding,
    IN POLICY_HANDLE  hPolicy,
    IN OUT PDWORD     pResume,
    OUT PSID        **pppAccounts,
    OUT PDWORD        pNumAccounts,
    IN DWORD          PreferredMaxSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntEnumStatus = STATUS_SUCCESS;
    LSA_ACCOUNT_ENUM_BUFFER accounts = {0};
    PSID *ppSids = NULL;
    DWORD offset = 0;
    DWORD spaceLeft = 0;
    DWORD size = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(pResume, ntStatus);
    BAIL_ON_INVALID_PTR(pppAccounts, ntStatus);
    BAIL_ON_INVALID_PTR(pNumAccounts, ntStatus);

    DCERPC_CALL(ntStatus, cli_LsaEnumAccounts(
                              (handle_t)hBinding,
                              hPolicy,
                              pResume,
                              &accounts,
                              PreferredMaxSize));
    if (ntStatus == STATUS_MORE_ENTRIES ||
        ntStatus == STATUS_NO_MORE_ENTRIES)
    {
        ntEnumStatus = ntStatus;
        ntStatus = STATUS_SUCCESS;
    }
    else if (ntStatus != STATUS_SUCCESS)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    offset    = 0;
    spaceLeft = 0;
    size      = 0;

    ntStatus = LsaAllocateSids(
                          ppSids,
                          &offset,
                          &spaceLeft,
                          &accounts,
                          &size);
    BAIL_ON_NT_STATUS(ntStatus);

    offset    = 0;
    spaceLeft = size;
    size      = 0;

    ntStatus = LsaRpcAllocateMemory(
                          OUT_PPVOID(&ppSids),
                          spaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaAllocateSids(
                          ppSids,
                          &offset,
                          &spaceLeft,
                          &accounts,
                          &size);
    BAIL_ON_NT_STATUS(ntStatus);

    *pppAccounts  = ppSids;
    *pNumAccounts = accounts.NumAccounts;

error:
    if (ntStatus)
    {
        if (ppSids)
        {
            LsaRpcFreeMemory(ppSids);
        }

        if (pppAccounts)
        {
            *pppAccounts = NULL;
        }

        if (pNumAccounts)
        {
            *pNumAccounts = 0;
        }
    }

    LsaCleanStubAccountBuffer(&accounts);

    if (ntStatus == STATUS_SUCCESS)
    {
        ntStatus = ntEnumStatus;
    }

    return ntStatus;
}
