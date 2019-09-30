/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
