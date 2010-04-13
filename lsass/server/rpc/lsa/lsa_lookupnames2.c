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
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupNames2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvLookupNames2(
    handle_t hBinding,
    POLICY_HANDLE hPolicy,
    UINT32 num_names,
    UNICODE_STRING *names,
    RefDomainList **domains,
    TranslatedSidArray2 *pSids,
    UINT16 level,
    UINT32 *count,
    UINT32 unknown1,
    UINT32 unknown2
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntLookupStatus = STATUS_SUCCESS;
    RefDomainList *pDomains = NULL;
    TranslatedSidArray2 Sids2;
    TranslatedSidArray3 Sids3;
    DWORD dwCount = 0;
    DWORD i = 0;

    memset(&Sids2, 0, sizeof(Sids2));
    memset(&Sids3, 0, sizeof(Sids3));

    ntStatus = LsaSrvLookupNames3(hBinding,
                                  hPolicy,
                                  num_names,
                                  names,
                                  &pDomains,
                                  &Sids3,
                                  level,
                                  &dwCount,
                                  unknown1,
                                  unknown2);
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != STATUS_SOME_NOT_MAPPED &&
        ntStatus != STATUS_NONE_MAPPED)
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntLookupStatus = ntStatus;

    Sids2.count = Sids3.count;
    ntStatus = LsaSrvAllocateMemory((void**)&Sids2.sids,
                                    sizeof(*Sids2.sids) * Sids2.count);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < Sids2.count; i++) {
        TranslatedSid2 *pOut = &(Sids2.sids[i]);
        TranslatedSid3 *pIn = &(Sids3.sids[i]);
        DWORD iRid = 0;

        pOut->type     = pIn->type;
        pOut->index    = pIn->index;
        pOut->unknown1 = pIn->unknown1;

        if (pIn->sid)
        {
            iRid      = pIn->sid->SubAuthorityCount - 1;
            pOut->rid = pIn->sid->SubAuthority[iRid];
        }
    }

    pSids->count = Sids2.count;
    pSids->sids  = Sids2.sids;
    *domains     = pDomains;
    *count       = dwCount;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        ntLookupStatus != STATUS_SUCCESS)
    {
        ntStatus = ntLookupStatus;
    }

    return ntStatus;

error:
    if (Sids2.sids)
    {
        LsaSrvFreeMemory(Sids2.sids);
    }

    pSids->count = 0;
    pSids->sids  = NULL;
    *domains     = NULL;
    *count       = 0;

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
