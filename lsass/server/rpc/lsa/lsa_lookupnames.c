/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        lsa_lookupnames.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupNames function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvLookupNames(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UINT32 num_names,
    /* [in] */ UNICODE_STRING *names,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedSidArray *sids,
    /* [in] */ UINT16 level,
    /* [in, out] */ UINT32 *count
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntLookupStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PPOLICY_CONTEXT pPolCtx = NULL;
    PUNICODE_STRING pNames = NULL;
    PWSTR pwszName = NULL;
    DWORD i = 0;
    RefDomainList *pDomains = NULL;
    TranslatedSidArray2 *pSidArray = NULL;
    DWORD dwCount = 0;

    pPolCtx = (PPOLICY_CONTEXT)hPolicy;

    if (pPolCtx == NULL || pPolCtx->Type != LsaContextPolicy)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pPolCtx->dwAccessGranted & LSA_ACCESS_LOOKUP_NAMES_SIDS))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = LsaSrvAllocateMemory((void**)&pNames,
                                    sizeof(*pNames) * num_names);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < num_names; i++) {
        dwError = LwAllocateWc16StringFromUnicodeString(
                                    &pwszName,
                                    &(names[i]));
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = LsaSrvInitUnicodeStringEx(&(pNames[i]), pwszName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        if (pwszName)
        {
            LW_SAFE_FREE_MEMORY(pwszName);
        }
    }

    ntStatus = LsaSrvAllocateMemory((void**)&pSidArray,
                                    sizeof(*pSidArray) * num_names);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvLookupNames2(IDL_handle,
                                hPolicy,
                                num_names,
                                pNames,
                                &pDomains,
                                pSidArray,
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

    sids->count = pSidArray->count;

    ntStatus = LsaSrvAllocateMemory((void**)&(sids->sids),
                                  sizeof(sids->sids[0]) * sids->count);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < sids->count; i++) {
        TranslatedSid2 *pTransSid2 = &(pSidArray->sids[i]);
        TranslatedSid *pTransSid = &(sids->sids[i]);

        pTransSid->type  = pTransSid2->type;
        pTransSid->rid   = pTransSid2->rid;
        pTransSid->index = pTransSid2->index;
    }
                                  
    *domains = pDomains;
    *count   = dwCount;

cleanup:
    if (pSidArray)
    {
        if (pSidArray->sids)
        {
            LsaSrvFreeMemory(pSidArray->sids);
        }

        LsaSrvFreeMemory(pSidArray);
    }

    if (pwszName)
    {
        LW_SAFE_FREE_MEMORY(pwszName);
    }

    if (pNames)
    {
        LsaSrvFreeMemory(pNames);
    }

    if (ntStatus == STATUS_SUCCESS &&
        ntLookupStatus != STATUS_SUCCESS)
    {
        ntStatus = ntLookupStatus;
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pDomains) {
        LsaSrvFreeMemory(pDomains);
    }

    if (sids->sids) {
        LsaSrvFreeMemory(sids->sids);
    }

    *domains    = NULL;
    sids->sids  = NULL;
    sids->count = 0;
    *count      = 0;

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
