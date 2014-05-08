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
 *        lsa_lookupnames3.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaLookupNames3 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaLookupNames3(
    IN  LSA_BINDING        hBinding,
    IN  POLICY_HANDLE      hPolicy,
    IN  UINT32             dwNumNames,
    IN  PWSTR             *ppwszNames,
    OUT RefDomainList    **ppDomList,
    OUT TranslatedSid3   **ppSids,
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
    TranslatedSidArray3 SidArray = {0};
    TranslatedSid3* pTransSids = NULL;
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

    DCERPC_CALL(ntStatus, cli_LsaLookupNames3(
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

    /* Status other than success doesn't have
       to mean failure here */

    if (ntRetStatus != STATUS_SUCCESS &&
        ntRetStatus != LW_STATUS_SOME_NOT_MAPPED)
    {
        BAIL_ON_NT_STATUS(ntRetStatus);
    }

    if (SidArray.count > 0)
    {
        ntStatus = LsaAllocateTranslatedSids3(NULL,
                                              &dwOffset,
                                              NULL,
                                              &SidArray,
                                              &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = LsaRpcAllocateMemory(OUT_PPVOID(&pTransSids),
                                        dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LsaAllocateTranslatedSids3(pTransSids,
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
    LsaCleanStubTranslatedSidArray3(&SidArray);

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
