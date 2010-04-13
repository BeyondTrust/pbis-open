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
 *        lsa_lookupsids.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupSids function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvLookupSids(
    handle_t hBinding,
    POLICY_HANDLE hPolicy,
    SID_ARRAY *sids,
    RefDomainList **ppDomains,
    TranslatedNameArray *pNames,
    UINT16 level,
    UINT32 *count
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntLookupStatus = STATUS_SUCCESS;
    RefDomainList *pDomains = NULL;
    TranslatedNameArray2 Names = {0};
    DWORD dwCount = 0;
    DWORD i = 0;

    ntStatus = LsaSrvLookupSids2(hBinding,
                                 hPolicy,
                                 sids,
                                 &pDomains,
                                 &Names,
                                 level,
                                 &dwCount,
                                 0, 0);
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != STATUS_SOME_NOT_MAPPED &&
        ntStatus != STATUS_NONE_MAPPED)
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntLookupStatus = ntStatus;

    pNames->count = Names.count;
    ntStatus = LsaSrvAllocateMemory(OUT_PPVOID(&pNames->names),
                                    sizeof(pNames->names[0]) * pNames->count);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < pNames->count; i++)
    {
        UNICODE_STRING *pIn  = &(Names.names[i].name);
        UNICODE_STRING *pOut = &(pNames->names[i].name);

        pNames->names[i].type      = Names.names[i].type;
        pNames->names[i].sid_index = Names.names[i].sid_index;
        pOut->Buffer               = pIn->Buffer;
        pOut->Length               = pIn->Length;
        pOut->MaximumLength        = pIn->MaximumLength;
    }

    *ppDomains = pDomains;
    *count     = dwCount;

cleanup:
    if (Names.names)
    {
        LsaSrvFreeMemory(Names.names);
    }

    if (ntStatus == STATUS_SUCCESS &&
        ntLookupStatus != STATUS_SUCCESS)
    {
        ntStatus = ntLookupStatus;
    }

    return ntStatus;

error:
    pNames->names = NULL;
    pNames->count = 0;
    *ppDomains    = NULL;
    *count        = 0;

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
