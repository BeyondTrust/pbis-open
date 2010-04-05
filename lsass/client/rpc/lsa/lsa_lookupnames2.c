/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
    IN  handle_t         hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  UINT32           NumNames,
    IN  PWSTR           *ppNames,
    OUT RefDomainList  **ppDomList,
    OUT TranslatedSid2 **ppSids,
    IN  UINT16           Level,
    IN OUT UINT32       *Count
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntRetStatus = STATUS_SUCCESS;
    UINT32 unknown1 = 0;
    UINT32 unknown2 = 0;
    UnicodeStringEx *pLsaNames = NULL;
    RefDomainList *pRefDomains = NULL;
    RefDomainList *pOutDomains = NULL;
    TranslatedSidArray2 SidArray = {0};
    TranslatedSid2* pTransSids = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(ppNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppDomList, ntStatus);
    BAIL_ON_INVALID_PTR(ppSids, ntStatus);
    BAIL_ON_INVALID_PTR(Count, ntStatus);

    pLsaNames = InitUnicodeStringExArray(ppNames, NumNames);
    BAIL_ON_NULL_PTR(pLsaNames, ntStatus);

    *Count = 0;

    DCERPC_CALL(ntStatus, cli_LsaLookupNames2(
                              hBinding,
                              hPolicy,
                              NumNames,
                              pLsaNames,
                              &pRefDomains,
                              &SidArray,
                              Level,
                              Count,
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
    *Count     = SidArray.count;

cleanup:
    FreeUnicodeStringExArray(pLsaNames, NumNames);

    /* Free pointers returned from stub */
    LsaCleanStubTranslatedSidArray2(&SidArray);

    if (pRefDomains)
    {
        LsaFreeStubRefDomainList(pRefDomains);
    }

    if (ntStatus == STATUS_SUCCESS &&
        (ntRetStatus == STATUS_SUCCESS ||
         ntRetStatus == LW_STATUS_SOME_NOT_MAPPED))
    {
        ntStatus = ntRetStatus;
    }

    return ntStatus;

error:
    LsaRpcFreeMemory(pTransSids);
    LsaRpcFreeMemory(pOutDomains);

    *ppSids    = NULL;
    *ppDomList = NULL;
    *Count     = 0;

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
