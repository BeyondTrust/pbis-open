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
 *        lsa_lookupnames3.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupNames3 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
LsaSrvLookupDomainNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    DWORD                 dwLevel,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray
    );


static
NTSTATUS
LsaSrvLookupForeignDomainNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    DWORD                 dwLevel,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray
    );


static
NTSTATUS
LsaSrvLookupLocalNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray,
    PDWORD                pdwLocalDomainIndex
    );


static
NTSTATUS
LsaSrvLookupBuiltinNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray,
    PDWORD                pdwBuiltinDomainIndex
    );


static
NTSTATUS
LsaSrvLookupOtherNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    DWORD                 dwLocalDomIndex,
    DWORD                 dwBuiltinDomIndex,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray
    );


NTSTATUS
LsaSrvLookupNames3(
    IN  handle_t               hBinding,
    IN  POLICY_HANDLE          hPolicy,
    IN  DWORD                  dwNumNames,
    IN  UNICODE_STRING        *pNames,
    OUT RefDomainList        **ppDomains,
    OUT TranslatedSidArray3   *pSids,
    IN  UINT16                 level,
    OUT PDWORD                 pdwCount,
    IN  DWORD                  dwUnknown1,
    IN  DWORD                  dwUnknown2
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPOLICY_CONTEXT pPolCtx = NULL;
    PACCOUNT_NAMES pAccountNames = NULL;
    DWORD dwUnknownNamesNum = 0;
    DWORD i = 0;
    DWORD dwLocalDomainIndex = 0;
    DWORD dwBuiltinDomainIndex = 0;
    TranslatedSidArray3 SidArray = {0};
    RefDomainList *pDomains = NULL;

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

    ntStatus = LsaSrvAllocateMemory((void**)&(SidArray.sids),
                                    sizeof(*SidArray.sids) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory((void**)&pDomains,
                                    sizeof(*pDomains));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(
                               (void**)&(pDomains->domains),
                               sizeof(*pDomains->domains) * (dwNumNames + 2));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Fresh start - we'll see which names are going to be resolved
     */
    for (i = 0; i < dwNumNames; i++)
    {
        TranslatedSid3 *pTransSid = &(SidArray.sids[i]);

        pTransSid->type     = SID_TYPE_UNKNOWN;
        pTransSid->index    = 0;
        pTransSid->unknown1 = 0;
        pTransSid->sid      = NULL;
    }

    ntStatus = LsaSrvSelectAccountsByDomainName(pPolCtx,
                                                pNames,
                                                dwNumNames,
                                                &pAccountNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Check domain (DOMAIN\name) names first.
     * This means asking our DC.
     */
    if (pAccountNames[LSA_DOMAIN_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupDomainNames(
                                       pPolCtx,
                                       &(pAccountNames[LSA_DOMAIN_ACCOUNTS]),
                                       level,
                                       pDomains,
                                       &SidArray);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Check foreign domain names.
     * Asking our DC since we don't have a list of trusted
     * domains' DCs yet.
     */
    if (pAccountNames[LSA_FOREIGN_DOMAIN_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupForeignDomainNames(
                                 pPolCtx,
                                 &(pAccountNames[LSA_FOREIGN_DOMAIN_ACCOUNTS]),
                                 level,
                                 pDomains,
                                 &SidArray);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Check local (MACHINE\name) names.
     * Call our local \samr server to lookup in MACHINE domain.
     */
    if (pAccountNames[LSA_LOCAL_DOMAIN_ACCOUNTS].dwCount ||
        pAccountNames[LSA_OTHER_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupLocalNames(
                                 pPolCtx,
                                 &(pAccountNames[LSA_LOCAL_DOMAIN_ACCOUNTS]),
                                 pDomains,
                                 &SidArray,
                                 &dwLocalDomainIndex);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Check builtin (BUILTIN\name) names.
     * Call our local \samr server to lookup in BUILTIN domain.
     */
    if (pAccountNames[LSA_BUILTIN_DOMAIN_ACCOUNTS].dwCount ||
        pAccountNames[LSA_OTHER_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupBuiltinNames(
                                 pPolCtx,
                                 &(pAccountNames[LSA_BUILTIN_DOMAIN_ACCOUNTS]),
                                 pDomains,
                                 &SidArray,
                                 &dwBuiltinDomainIndex);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Check names we're not sure about.
     * Call our local \samr server to lookup in MACHINE domain first.
     * If a name can't be found there, try BUILTIN domain before
     * considering it unknown.
     */
    if (pAccountNames[LSA_OTHER_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupOtherNames(
                                 pPolCtx,
                                 &(pAccountNames[LSA_OTHER_ACCOUNTS]),
                                 dwLocalDomainIndex,
                                 dwBuiltinDomainIndex,
                                 pDomains,
                                 &SidArray);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    SidArray.count = dwNumNames;

    /* Check if all names have been mapped to decide about
       returned status */
    for (i = 0; i < SidArray.count; i++)
    {
        if (SidArray.sids[i].type == SID_TYPE_UNKNOWN)
        {
             dwUnknownNamesNum++;
        }
    }

    if (dwUnknownNamesNum > 0)
    {
        if (dwUnknownNamesNum < SidArray.count)
        {
            ntStatus = STATUS_SOME_NOT_MAPPED;
        }
        else
        {
            ntStatus = STATUS_NONE_MAPPED;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
    }

    /* windows seems to set max_size to multiple of 32 */
    pDomains->max_size = ((pDomains->count / 32) + 1) * 32;

    *ppDomains   = pDomains;
    pSids->count = SidArray.count;
    pSids->sids  = SidArray.sids;
    *pdwCount    = SidArray.count - dwUnknownNamesNum;

cleanup:
    LsaSrvFreeAccountNames(pAccountNames);

    return ntStatus;

error:
    if (pDomains)
    {
        LsaSrvFreeMemory(pDomains);
    }

    if (SidArray.sids)
    {
        LsaSrvFreeMemory(SidArray.sids);
    }

    *ppDomains   = NULL;
    pSids->count = 0;
    pSids->sids  = NULL;
    *pdwCount    = 0;
    goto cleanup;

}


static
NTSTATUS
LsaSrvLookupDomainNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    DWORD                 dwLevel,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDOMAIN_ENTRY pDomEntry = NULL;
    handle_t hLsaBinding = NULL;
    POLICY_HANDLE hDcPolicy = NULL;
    DWORD dwDomIndex = 0;
    RefDomainList *pDomain = NULL;
    TranslatedSid3 *pSids = NULL;
    DWORD dwRemoteSidsCount = 0;
    DWORD iDomain = 0;
    DWORD iSid = 0;
    DWORD i = 0;

    ntStatus = LsaSrvGetDomainByName(pPolCtx,
                                     pPolCtx->pwszDomainName,
                                     &pDomEntry);
    if (ntStatus == STATUS_NO_SUCH_DOMAIN)
    {
        ntStatus = LsaSrvConnectDomainByName(pPolCtx,
                                            pPolCtx->pwszDomainName,
                                            &pDomEntry);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        hLsaBinding = pDomEntry->hLsaBinding;
        hDcPolicy   = pDomEntry->hPolicy;
    }
    else if (ntStatus == STATUS_SUCCESS)
    {
        hLsaBinding = pDomEntry->hLsaBinding;
        hDcPolicy   = pDomEntry->hPolicy;
    }
    else
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = LsaLookupNames3(hLsaBinding,
                               hDcPolicy,
                               pAccountNames->dwCount,
                               pAccountNames->ppwszNames,
                               &pDomain,
                               &pSids,
                               dwLevel,
                               &dwRemoteSidsCount);
    if (ntStatus == STATUS_SUCCESS ||
        ntStatus == STATUS_SOME_NOT_MAPPED)
    {
        for (iDomain = 0; iDomain < pDomain->count; iDomain++)
        {
            LsaDomainInfo *pSrcDomInfo = NULL;
            LsaDomainInfo *pDstDomInfo = NULL;

            dwDomIndex  = pDomains->count;
            pSrcDomInfo = &(pDomain->domains[iDomain]);
            pDstDomInfo = &(pDomains->domains[dwDomIndex]);

            ntStatus = LsaSrvDuplicateUnicodeStringEx(&pDstDomInfo->name,
                                                      &pSrcDomInfo->name);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ntStatus = LsaSrvDuplicateSid(&pDstDomInfo->sid,
                                          pSrcDomInfo->sid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            for (iSid = 0; iSid < dwRemoteSidsCount; iSid++)
            {
                TranslatedSid3 *pSrcSid = &(pSids[iSid]);
                DWORD iTransSid = pAccountNames->pdwIndices[iSid];
                TranslatedSid3 *pDstSid = &(pSidArray->sids[iTransSid]);
                PSID pAcctSid = NULL;

                if (iDomain != pSrcSid->index)
                {
                    continue;
                }

                if (pSrcSid->sid)
                {
                    ntStatus = LsaSrvDuplicateSid(&pAcctSid, pSrcSid->sid);
                    BAIL_ON_NTSTATUS_ERROR(ntStatus);
                }

                pDstSid->type     = pSrcSid->type;
                pDstSid->index    = dwDomIndex;
                pDstSid->unknown1 = pSrcSid->unknown1;
                pDstSid->sid      = pAcctSid;
            }

            pDomains->count  = (++dwDomIndex);
        }

        pSidArray->count += dwRemoteSidsCount;
    }
    else if (ntStatus == STATUS_NONE_MAPPED)
    {
        for (i = 0; i < pAccountNames->dwCount; i++)
        {
            DWORD iTransSid = pAccountNames->pdwIndices[i];
            TranslatedSid3 *pDstSid = &(pSidArray->sids[iTransSid]);

            pDstSid->type     = SID_TYPE_UNKNOWN;
            pDstSid->index    = 0;
            pDstSid->unknown1 = 0;
            pDstSid->sid      = NULL;
        }

        pSidArray->count += pAccountNames->dwCount;
    }
    else
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Lookup status is checked later by the caller
     * so avoid bailing accidentally because other lookups
     * may be successful
     */
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

cleanup:
    if (pDomEntry)
    {
        LsaSrvDomainEntryFree(&pDomEntry);
    }

    if (pDomain)
    {
        LsaRpcFreeMemory(pDomain);
    }

    if (pSids)
    {
        LsaRpcFreeMemory(pSids);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaSrvLookupForeignDomainNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    DWORD                 dwLevel,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_ENTRY pDomain = NULL;
    handle_t hLsaBinding = NULL;
    POLICY_HANDLE hDcPolicy = NULL;
    DWORD iDomName = 0;
    DWORD iResName = 0;
    DWORD iName = 0;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszAcctName = NULL;
    PWSTR pwszName = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszAcct = NULL;
    DWORD dwNameIndex = 0;
    DWORD dwDomIndex = 0;
    ACCOUNT_NAMES ForeignNames = {0};
    RefDomainList *pForeignDomains = NULL;
    TranslatedSid3 *pForeignSids = NULL;
    DWORD dwForeignSidsCount = 0;
    DWORD iDomain = 0;
    DWORD iSid = 0;
    DWORD i = 0;

    /*
     * Allocate enough space for potential sequence of name lookups
     */
    dwError = LwAllocateMemory(
                    sizeof(ForeignNames.ppwszNames[0]) * pAccountNames->dwCount,
                    OUT_PPVOID(&ForeignNames.ppwszNames));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(ForeignNames.pdwIndices[0]) * pAccountNames->dwCount,
                    OUT_PPVOID(&ForeignNames.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(ForeignNames.pdwIndices, -1, sizeof(DWORD) * pAccountNames->dwCount);

    while (iDomName < pAccountNames->dwCount)
    {
        ntStatus = LsaSrvParseAccountName(
                              pAccountNames->ppwszNames[iDomName],
                              &pwszDomainName,
                              &pwszAcctName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        iResName = 0;

        for (iName = iDomName; iName < pAccountNames->dwCount; iName++)
        {
            pwszName    = pAccountNames->ppwszNames[iName];
            dwNameIndex = pAccountNames->pdwIndices[iName];
            pwszDomain  = NULL;
            pwszAcct    = NULL;

            ntStatus = LsaSrvParseAccountName(pwszName,
                                              &pwszDomain,
                                              &pwszAcct);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            if (!wc16scasecmp(pwszDomainName, pwszDomain))
            {
                dwError = LwAllocateWc16String(
                                    &(ForeignNames.ppwszNames[iResName]),
                                    pwszName);
                BAIL_ON_LSA_ERROR(dwError);

                ForeignNames.pdwIndices[iResName] = dwNameIndex;

                ForeignNames.dwCount = ++iResName;

                /*
                 * Free the name so it's not looked up again
                 */
                LW_SAFE_FREE_MEMORY(pAccountNames->ppwszNames[iName]);
                pAccountNames->ppwszNames[iName] = NULL;
                pwszName = NULL;
            }

            LW_SAFE_FREE_MEMORY(pwszDomain);
            LW_SAFE_FREE_MEMORY(pwszAcct);
        }

        ntStatus = LsaSrvConnectDomainByName(pPolCtx,
                                             pwszDomainName,
                                             &pDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        hLsaBinding = pDomain->hLsaBinding;
        hDcPolicy   = pDomain->hPolicy;

        ntStatus = LsaLookupNames3(hLsaBinding,
                                   hDcPolicy,
                                   ForeignNames.dwCount,
                                   ForeignNames.ppwszNames,
                                   &pForeignDomains,
                                   &pForeignSids,
                                   dwLevel,
                                   &dwForeignSidsCount);
        if (ntStatus == STATUS_SUCCESS ||
            ntStatus == STATUS_SOME_NOT_MAPPED)
        {
            for (iDomain = 0; iDomain < pForeignDomains->count; iDomain++)
            {
                LsaDomainInfo *pSrcDomInfo = NULL;
                LsaDomainInfo *pDstDomInfo = NULL;

                dwDomIndex  = pDomains->count;
                pSrcDomInfo = &(pForeignDomains->domains[iDomain]);
                pDstDomInfo = &(pDomains->domains[dwDomIndex]);

                ntStatus = LsaSrvDuplicateUnicodeStringEx(&pDstDomInfo->name,
                                                          &pSrcDomInfo->name);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);

                ntStatus = LsaSrvDuplicateSid(&pDstDomInfo->sid,
                                              pSrcDomInfo->sid);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);

                for (iSid = 0; iSid < dwForeignSidsCount ; iSid++)
                {
                    DWORD iTransSid = pAccountNames->pdwIndices[iSid];
                    TranslatedSid3 *pSrcSid = &(pForeignSids[iSid]);
                    TranslatedSid3 *pDstSid = &(pSidArray->sids[iTransSid]);
                    PSID pAcctSid = NULL;

                    if (iDomain != pSrcSid->index)
                    {
                        continue;
                    }

                    if (pSrcSid->sid)
                    {
                        ntStatus = LsaSrvDuplicateSid(&pAcctSid, pSrcSid->sid);
                        BAIL_ON_NTSTATUS_ERROR(ntStatus);
                    }

                    pDstSid->type     = pSrcSid->type;
                    pDstSid->index    = dwDomIndex;
                    pDstSid->unknown1 = pSrcSid->unknown1;
                    pDstSid->sid      = pAcctSid;
                }

                pDomains->count  = (++dwDomIndex);
            }

            pSidArray->count += dwForeignSidsCount;
        }
        else if (ntStatus == STATUS_NONE_MAPPED)
        {
            for (i = 0; i < ForeignNames.dwCount; i++)
            {
                DWORD iTransSid = ForeignNames.pdwIndices[i];
                TranslatedSid3 *pDstSid = &(pSidArray->sids[iTransSid]);

                pDstSid->type     = SID_TYPE_UNKNOWN;
                pDstSid->index    = 0;
                pDstSid->unknown1 = 0;
                pDstSid->sid      = NULL;
            }

            pSidArray->count += ForeignNames.dwCount;
        }
        else
        {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        /*
         * Free the names array before the next round of lookup
         * The entire allocated space (i.e. as big as pAccountNames)
         * is cleaned up.
         */
        for (i = 0; i < pAccountNames->dwCount; i++)
        {
            LW_SAFE_FREE_MEMORY(ForeignNames.ppwszNames[i]);
            ForeignNames.ppwszNames[i] = NULL;
            ForeignNames.pdwIndices[i] = -1;
        }

        if (pForeignDomains)
        {
            LsaRpcFreeMemory(pForeignDomains);
            pForeignDomains = NULL;
        }

        if (pForeignSids)
        {
            LsaRpcFreeMemory(pForeignSids);
            pForeignSids = NULL;
        }

        LW_SAFE_FREE_MEMORY(pwszDomainName);
        LW_SAFE_FREE_MEMORY(pwszAcctName);
        pwszDomainName = NULL;
        pwszAcctName = NULL;

        /*
         * Find another domain name (first name that hasn't
         * been nulled out)
         */
        iDomName = 0;
        while (iDomName < pAccountNames->dwCount &&
               pAccountNames->ppwszNames[iDomName] == NULL)
        {
            iDomName++;
        }
    }

    /*
     * Lookup status is checked later by the caller
     * so avoid bailing accidentally because other lookups
     * may be successful
     */
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

cleanup:
    for (i = 0; i < pAccountNames->dwCount; i++)
    {
        LW_SAFE_FREE_MEMORY(ForeignNames.ppwszNames[i]);
    }

    LW_SAFE_FREE_MEMORY(ForeignNames.ppwszNames);
    LW_SAFE_FREE_MEMORY(ForeignNames.pdwIndices);

    if (pForeignDomains)
    {
        LsaRpcFreeMemory(pForeignDomains);
    }

    if (pForeignSids)
    {
        LsaRpcFreeMemory(pForeignSids);
    }

    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszAcct);
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszAcctName);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaSrvLookupLocalNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray,
    PDWORD                pdwDomIndex
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLocalDomIndex = 0;
    LsaDomainInfo *pLocalDomainInfo = NULL;
    PDWORD pdwRids = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwLocalDomIndex  = pDomains->count;
    pLocalDomainInfo = &(pDomains->domains[dwLocalDomIndex]);

    ntStatus = LsaSrvInitUnicodeStringEx(&pLocalDomainInfo->name,
                                         pPolCtx->pwszLocalDomainName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvDuplicateSid(&pLocalDomainInfo->sid,
                                  pPolCtx->pLocalDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrLookupNames(pPolCtx->hSamrBinding,
                               pPolCtx->hLocalDomain,
                               pAccountNames->dwCount,
                               pAccountNames->ppwszNames,
                               &pdwRids,
                               &pdwTypes,
                               &dwCount);
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
        ntStatus != STATUS_NONE_MAPPED)
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    for (i = 0; i < pAccountNames->dwCount; i++)
    {
        DWORD iTransSid = pAccountNames->pdwIndices[i];
        TranslatedSid3 *pSid = &(pSidArray->sids[iTransSid]);
        PSID pDomainSid = pLocalDomainInfo->sid;
        PSID pAcctSid = NULL;

        if (ntStatus == STATUS_SUCCESS ||
            ntStatus == LW_STATUS_SOME_NOT_MAPPED)
        {
            ntStatus = LsaSrvSidAppendRid(&pAcctSid, pDomainSid, pdwRids[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pSid->type = pdwTypes[i];
            pSid->sid  = pAcctSid;
        }
        else
        {
            /* STATUS_NONE_MAPPED is returned along with NULL
               results so we have to set translated sids to
               invalid one by one */
            pSid->type = SID_TYPE_UNKNOWN;
            pSid->sid  = NULL;
        }

        pSid->index    = dwLocalDomIndex;
        pSid->unknown1 = 0;
    }

    pDomains->count   = dwLocalDomIndex + 1;
    pSidArray->count += pAccountNames->dwCount;
    *pdwDomIndex      = dwLocalDomIndex;

    /*
     * Lookup status is checked later by the caller
     * so avoid bailing accidentally because other lookups
     * may be successful
     */
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

cleanup:
    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
    }

    if (pdwTypes)
    {
        SamrFreeMemory(pdwTypes);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaSrvLookupBuiltinNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray,
    PDWORD                pdwDomIndex
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwBuiltinDomIndex = 0;
    LsaDomainInfo *pBuiltinDomainInfo = NULL;
    WCHAR wszBuiltinDomainName[] = LSA_BUILTIN_DOMAIN_NAME;
    PSID pBuiltinDomainSid = NULL;
    PDWORD pdwRids = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwBuiltinDomIndex  = pDomains->count;
    pBuiltinDomainInfo = &(pDomains->domains[dwBuiltinDomIndex]);

    ntStatus = LsaSrvInitUnicodeStringEx(&pBuiltinDomainInfo->name,
                                         wszBuiltinDomainName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateWellKnownSid(WinBuiltinDomainSid,
                                     NULL,
                                     &pBuiltinDomainSid,
                                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaSrvDuplicateSid(&pBuiltinDomainInfo->sid,
                                  pBuiltinDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrLookupNames(pPolCtx->hSamrBinding,
                               pPolCtx->hBuiltinDomain,
                               pAccountNames->dwCount,
                               pAccountNames->ppwszNames,
                               &pdwRids,
                               &pdwTypes,
                               &dwCount);
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
        ntStatus != STATUS_NONE_MAPPED)
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    for (i = 0; i < pAccountNames->dwCount; i++)
    {
        DWORD iTransSid = pAccountNames->pdwIndices[i];
        TranslatedSid3 *pSid = &(pSidArray->sids[iTransSid]);
        PSID pDomainSid = pBuiltinDomainInfo->sid;
        PSID pAcctSid = NULL;

        if (ntStatus == STATUS_SUCCESS ||
            ntStatus == LW_STATUS_SOME_NOT_MAPPED)
        {
            ntStatus = LsaSrvSidAppendRid(&pAcctSid, pDomainSid, pdwRids[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pSid->type = pdwTypes[i];
            pSid->sid  = pAcctSid;
        }
        else
        {
            /* STATUS_NONE_MAPPED is returned along with NULL
               results so we have to set translated sids to
               invalid one by one */
            pSid->type = SID_TYPE_UNKNOWN;
            pSid->sid  = NULL;
        }

        pSid->index    = dwBuiltinDomIndex;
        pSid->unknown1 = 0;
    }

    pDomains->count   = dwBuiltinDomIndex + 1;
    pSidArray->count += pAccountNames->dwCount;
    *pdwDomIndex      = dwBuiltinDomIndex;

    /*
     * Lookup status is checked later by the caller
     * so avoid bailing accidentally because other lookups
     * may be successful
     */
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

cleanup:
    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
    }

    if (pdwTypes)
    {
        SamrFreeMemory(pdwTypes);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaSrvLookupOtherNames(
    PPOLICY_CONTEXT       pPolCtx,
    PACCOUNT_NAMES        pAccountNames,
    DWORD                 dwLocalDomIndex,
    DWORD                 dwBuiltinDomIndex,
    RefDomainList        *pDomains,
    TranslatedSidArray3  *pSidArray
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LsaDomainInfo *pLocalDomainInfo = NULL;
    LsaDomainInfo *pBuiltinDomainInfo = NULL;
    PDWORD pdwLocalRids = NULL;
    PDWORD pdwLocalTypes = NULL;
    DWORD dwLocalCount = 0;
    PDWORD pdwBuiltinRids = NULL;
    PDWORD pdwBuiltinTypes = NULL;
    DWORD dwBuiltinCount = 0;
    DWORD dwCount = 0;
    DWORD i = 0;
    WCHAR wszBuiltinDomainName[] = LSA_BUILTIN_DOMAIN_NAME;

    pLocalDomainInfo   = &(pDomains->domains[dwLocalDomIndex]);
    pBuiltinDomainInfo = &(pDomains->domains[dwBuiltinDomIndex]);

    ntStatus = SamrLookupNames(pPolCtx->hSamrBinding,
                               pPolCtx->hLocalDomain,
                               pAccountNames->dwCount,
                               pAccountNames->ppwszNames,
                               &pdwLocalRids,
                               &pdwLocalTypes,
                               &dwLocalCount);
    if (ntStatus == LW_STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = SamrLookupNames(pPolCtx->hSamrBinding,
                                   pPolCtx->hBuiltinDomain,
                                   pAccountNames->dwCount,
                                   pAccountNames->ppwszNames,
                                   &pdwBuiltinRids,
                                   &pdwBuiltinTypes,
                                   &dwBuiltinCount);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
            ntStatus != STATUS_NONE_MAPPED)
        {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

    }
    else if (ntStatus != STATUS_SUCCESS)
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    for (i = 0; i < pAccountNames->dwCount; i++)
    {
        PWSTR pwszName         = pAccountNames->ppwszNames[i];
        DWORD iTransSid        = pAccountNames->pdwIndices[i];
        TranslatedSid3 *pSid   = &(pSidArray->sids[iTransSid]);
        PSID pLocalDomainSid   = pLocalDomainInfo->sid;
        PSID pBuiltinDomainSid = pBuiltinDomainInfo->sid;
        PSID pAcctSid          = NULL;

        if (pdwLocalTypes &&
            pdwLocalTypes[i] != SID_TYPE_UNKNOWN)
        {
            /*
             * This is a name from local domain
             */
            ntStatus = LsaSrvSidAppendRid(&pAcctSid,
                                          pLocalDomainSid,
                                          pdwLocalRids[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pSid->type   = pdwLocalTypes[i];
            pSid->sid    = pAcctSid;
            pSid->index  = dwLocalDomIndex;
        }
        else if (!wc16scasecmp(pwszName, pPolCtx->pwszLocalDomainName))
        {
            /*
             * It turns out to be a local domain (machine) name itself
             */
            pSid->type   = SID_TYPE_COMPUTER;
            pSid->sid    = NULL;
            pSid->index  = dwLocalDomIndex;
        }
        else if (pdwBuiltinTypes &&
                 pdwBuiltinTypes[i] != SID_TYPE_UNKNOWN)
        {
            /*
             * This is a name from builtin domain
             */
            ntStatus = LsaSrvSidAppendRid(&pAcctSid,
                                          pBuiltinDomainSid,
                                          pdwBuiltinRids[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pSid->type   = pdwBuiltinTypes[i];
            pSid->sid    = pAcctSid;
            pSid->index  = dwBuiltinDomIndex;
        }
        else if (!wc16scasecmp(pwszName, wszBuiltinDomainName))
        {
            /*
             * It turns out to be the builtin domain name
             */
            pSid->type   = SID_TYPE_DOMAIN;
            pSid->sid    = NULL;
            pSid->index  = dwBuiltinDomIndex;
        }
        else
        {
            /*
             * We have run out of ideas...
             */
            pSid->type   = SID_TYPE_UNKNOWN;
            pSid->sid    = NULL;
            pSid->index  = 0;
        }

        pSid->unknown1  = 0;
        dwCount++;
    }

    pSidArray->count += dwCount;

    /*
     * Lookup status is checked later by the caller
     * so avoid bailing accidentally because other lookups
     * may be successful
     */
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

cleanup:
    if (pdwLocalRids)
    {
        SamrFreeMemory(pdwLocalRids);
    }

    if (pdwLocalTypes)
    {
        SamrFreeMemory(pdwLocalTypes);
    }

    if (pdwBuiltinRids)
    {
        SamrFreeMemory(pdwBuiltinRids);
    }

    if (pdwBuiltinTypes)
    {
        SamrFreeMemory(pdwBuiltinTypes);
    }

    return ntStatus;

error:
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
