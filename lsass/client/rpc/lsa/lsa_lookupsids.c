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
 *        lsa_lookupsids.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaLookupSids function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaLookupSids(
    IN  LSA_BINDING        hBinding,
    IN  POLICY_HANDLE      hPolicy,
    IN  PSID_ARRAY         pSids,
    OUT RefDomainList    **ppRefDomList,
    OUT TranslatedName   **ppTransNames,
    IN  WORD               swLevel,
    IN OUT PDWORD          pdwCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntRetStatus = STATUS_SUCCESS;
    TranslatedNameArray NameArray = {0};
    RefDomainList *pRefDomains = NULL;
    TranslatedName *pTransNames = NULL;
    RefDomainList *pOutDomains = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(pSids, ntStatus);
    BAIL_ON_INVALID_PTR(ppRefDomList, ntStatus);
    BAIL_ON_INVALID_PTR(ppTransNames, ntStatus);
    BAIL_ON_INVALID_PTR(pdwCount, ntStatus);

    /* windows allows Level to be in range 1-6 */

    *pdwCount = 0;

    DCERPC_CALL(ntStatus, cli_LsaLookupSids(
                              (handle_t)hBinding,
                              hPolicy,
                              pSids,
                              &pRefDomains,
                              &NameArray,
                              swLevel,
                              pdwCount));
    ntRetStatus = ntStatus;

    /* Status other than success doesn't have to mean failure here */
    if (ntRetStatus != STATUS_SUCCESS &&
        ntRetStatus != LW_STATUS_SOME_NOT_MAPPED)
    {
        BAIL_ON_NT_STATUS(ntRetStatus);
    }

    if (NameArray.count)
    {
        ntStatus = LsaAllocateTranslatedNames(NULL,
                                              &dwOffset,
                                              NULL,
                                              &NameArray,
                                              &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = LsaRpcAllocateMemory(OUT_PPVOID(&pTransNames),
                                        dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LsaAllocateTranslatedNames(pTransNames,
                                              &dwOffset,
                                              &dwSpaceLeft,
                                              &NameArray,
                                              &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pRefDomains)
    {
        dwSize   = 0;
        dwOffset = 0;

        ntStatus = LsaAllocateRefDomainList(NULL,
                                            &dwOffset,
                                            NULL,
                                            pRefDomains,
                                            &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = LsaRpcAllocateMemory(OUT_PPVOID(&pOutDomains),
                                        dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LsaAllocateRefDomainList(pOutDomains,
                                            &dwOffset,
                                            &dwSpaceLeft,
                                            pRefDomains,
                                            &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppTransNames = pTransNames;
    *ppRefDomList = pOutDomains;
    *pdwCount     = NameArray.count;

cleanup:
    /* Free pointers returned from stub */
    if (pRefDomains)
    {
        LsaFreeStubRefDomainList(pRefDomains);
    }

    LsaCleanStubTranslatedNameArray(&NameArray);

    if (ntStatus == STATUS_SUCCESS &&
        (ntRetStatus == STATUS_SUCCESS ||
         ntRetStatus == LW_STATUS_SOME_NOT_MAPPED))
    {
        ntStatus = ntRetStatus;
    }

    return ntStatus;

error:
    if (pTransNames)
    {
        LsaRpcFreeMemory(pTransNames);
    }

    if (pOutDomains)
    {
        LsaRpcFreeMemory(pOutDomains);
    }

    if (ppTransNames)
    {
        *ppTransNames = NULL;
    }

    if (ppRefDomList)
    {
        *ppRefDomList = NULL;
    }

    if (pdwCount)
    {
        *pdwCount = 0;
    }

    goto cleanup;
}
