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
 *        lsa_lookupnames2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaLookupNames2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaLookupNames2(
    IN  LSA_BINDING        hBinding,
    IN  POLICY_HANDLE      hPolicy,
    IN  DWORD              dwNumNames,
    IN  PWSTR             *ppwszNames,
    OUT RefDomainList    **ppDomList,
    OUT TranslatedSid2   **ppSids,
    IN  WORD               swLevel,
    IN OUT PDWORD          pdwCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntRetStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    UINT32 unknown1 = 0;
    UINT32 unknown2 = 0;
    PUNICODE_STRING pLsaNames = NULL;
    DWORD iName = 0;
    RefDomainList *pRefDomains = NULL;
    RefDomainList *pOutDomains = NULL;
    TranslatedSidArray2 SidArray = {0};
    TranslatedSid2* pTransSids = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(ppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppDomList, ntStatus);
    BAIL_ON_INVALID_PTR(ppSids, ntStatus);
    BAIL_ON_INVALID_PTR(pdwCount, ntStatus);

    dwError = LwAllocateMemory(sizeof(pLsaNames[0]) * dwNumNames,
                               OUT_PPVOID(&pLsaNames));
    BAIL_ON_WIN_ERROR(dwError);

    for (iName = 0; iName < dwNumNames; iName++)
    {
        dwError = LwAllocateUnicodeStringFromWc16String(
                                      &pLsaNames[iName],
                                      ppwszNames[iName]);
        BAIL_ON_WIN_ERROR(dwError);
    }
    
    *pdwCount = 0;

    DCERPC_CALL(ntStatus, cli_LsaLookupNames2(
                              (handle_t)hBinding,
                              hPolicy,
                              dwNumNames,
                              pLsaNames,
                              &pRefDomains,
                              &SidArray,
                              swLevel,
                              pdwCount,
                              unknown1,
                              unknown2));
    ntRetStatus = ntStatus;

    /* Status other than success doesn't have to mean failure here */
    if (ntRetStatus != STATUS_SUCCESS &&
        ntRetStatus != LW_STATUS_SOME_NOT_MAPPED)
    {
        BAIL_ON_NT_STATUS(ntRetStatus);
    }

    if (SidArray.count > 0)
    {
        dwSpaceLeft = sizeof(*pTransSids) * SidArray.count;

        ntStatus = LsaRpcAllocateMemory(OUT_PPVOID(&pTransSids),
                                        dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LsaAllocateTranslatedSids2(pTransSids,
                                              &dwOffset,
                                              &dwSpaceLeft,
                                              &SidArray,
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

    *ppSids    = pTransSids;
    *ppDomList = pOutDomains;
    *pdwCount  = SidArray.count;

cleanup:
    if (pLsaNames)
    {
        for (iName = 0; iName < dwNumNames; iName++)
        {
            LwFreeUnicodeString(&(pLsaNames[iName]));
        }

        LW_SAFE_FREE_MEMORY(pLsaNames);
    }

    /* Free pointers returned from stub */
    LsaCleanStubTranslatedSidArray2(&SidArray);

    if (pRefDomains)
    {
        LsaFreeStubRefDomainList(pRefDomains);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    if (ntStatus == STATUS_SUCCESS &&
        (ntRetStatus == STATUS_SUCCESS ||
         ntRetStatus == LW_STATUS_SOME_NOT_MAPPED))
    {
        ntStatus = ntRetStatus;
    }

    return ntStatus;

error:
    if (pTransSids)
    {
        LsaRpcFreeMemory(pTransSids);
    }

    if (pOutDomains)
    {
        LsaRpcFreeMemory(pOutDomains);
    }

    if (ppSids)
    {
        *ppSids = NULL;
    }

    if (ppDomList)
    {
        *ppDomList = NULL;
    }

    if (pdwCount)
    {
        *pdwCount = 0;
    }

    goto cleanup;
}
