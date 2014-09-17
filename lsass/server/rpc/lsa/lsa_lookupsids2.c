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
 *        lsa_lookupsids2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupSids2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
LsaSrvLookupDomainSids(
    PPOLICY_CONTEXT        pPolCtx,
    PACCOUNT_SIDS          pAccountSids,
    DWORD                  dwLevel,
    RefDomainList         *pDomains,
    TranslatedNameArray2  *pSidArray
    );


static
NTSTATUS
LsaSrvLookupForeignDomainSids(
    PPOLICY_CONTEXT        pPolCtx,
    PACCOUNT_SIDS          pAccountSids,
    DWORD                  dwLevel,
    RefDomainList         *pDomains,
    TranslatedNameArray2  *pSidArray
    );


static
NTSTATUS
LsaSrvLookupLocalSids(
    PPOLICY_CONTEXT        pPolCtx,
    PACCOUNT_SIDS          pAccountSids,
    RefDomainList         *pDomains,
    TranslatedNameArray2  *pSidArray,
    PDWORD                 pdwLocalDomainIndex
    );


static
NTSTATUS
LsaSrvLookupBuiltinSids(
    PPOLICY_CONTEXT        pPolCtx,
    PACCOUNT_SIDS          pAccountSids,
    RefDomainList         *pDomains,
    TranslatedNameArray2  *pSidArray,
    PDWORD                 pdwBuiltinDomainIndex
    );


NTSTATUS
LsaSrvLookupSids2(
    handle_t hBinding,
    POLICY_HANDLE hPolicy,
    SID_ARRAY *pSids,
    RefDomainList **ppDomains,
    TranslatedNameArray2 *pNamesArray,
    UINT16 level,
    UINT32 *pdwCount,
    UINT32 unknown1,
    UINT32 unknown2
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLevel = level;
    PPOLICY_CONTEXT pPolCtx = NULL;
    PACCOUNT_SIDS pAccountSids = NULL;
    DWORD dwSidsNum = 0;
    DWORD i = 0;
    DWORD dwLocalDomainIndex = 0;
    DWORD dwBuiltinDomainIndex = 0;
    DWORD dwUnknownSidsNum = 0;
    RefDomainList *pDomains = NULL;
    TranslatedNameArray2 NamesArray = {0};

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

    dwSidsNum = pSids->dwNumSids;

    ntStatus = LsaSrvAllocateMemory(OUT_PPVOID(&(NamesArray.names)),
                                    sizeof(NamesArray.names[0]) * dwSidsNum);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(OUT_PPVOID(&pDomains),
                                    sizeof(*pDomains));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(
                             OUT_PPVOID(&pDomains->domains),
                             sizeof(*pDomains->domains) * (dwSidsNum + 2));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Fresh start - we'll see which SIDs are going to be resolved
     */
    for (i = 0; i < dwSidsNum; i++)
    {
        TranslatedName2 *pTransName = &(NamesArray.names[i]);

        pTransName->type      = SID_TYPE_UNKNOWN;
        pTransName->sid_index = 0;
        pTransName->unknown1  = 0;
    }

    ntStatus = LsaSrvSelectAccountsByDomainSid(pPolCtx,
                                               pSids,
                                               pSids->dwNumSids,
                                               &pAccountSids);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Check remote (resolving to DOMAIN\name) SIDs first.
     * This means asking the DC.
     */
    if (pAccountSids[LSA_DOMAIN_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupDomainSids(pPolCtx,
                                          &(pAccountSids[LSA_DOMAIN_ACCOUNTS]),
                                          dwLevel,
                                          pDomains,
                                          &NamesArray);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Check SIDs from foreign domains.
     */
    if (pAccountSids[LSA_FOREIGN_DOMAIN_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupForeignDomainSids(
                                 pPolCtx,
                                 &(pAccountSids[LSA_FOREIGN_DOMAIN_ACCOUNTS]),
                                 dwLevel,
                                 pDomains,
                                 &NamesArray);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Check local (resolving to MACHINE\name) SIDs.
     * Call our local \samr server to lookup in MACHINE domain.
     */
    if (pAccountSids[LSA_LOCAL_DOMAIN_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupLocalSids(
                               pPolCtx,
                               &(pAccountSids[LSA_LOCAL_DOMAIN_ACCOUNTS]),
                               pDomains,
                               &NamesArray,
                               &dwLocalDomainIndex);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (pAccountSids[LSA_BUILTIN_DOMAIN_ACCOUNTS].dwCount)
    {
        ntStatus = LsaSrvLookupBuiltinSids(
                               pPolCtx,
                               &(pAccountSids[LSA_BUILTIN_DOMAIN_ACCOUNTS]),
                               pDomains,
                               &NamesArray,
                               &dwBuiltinDomainIndex);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    NamesArray.count = dwSidsNum;

    /* Check if all SIDs have been mapped to decide about
       returned status */
    for (i = 0; i < NamesArray.count; i++)
    {
        if (NamesArray.names[i].type == SID_TYPE_UNKNOWN)
        {
             dwUnknownSidsNum++;
        }
    }

    ntStatus = STATUS_SUCCESS;

    if (dwUnknownSidsNum > 0)
    {
        if (dwUnknownSidsNum < NamesArray.count)
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

    *ppDomains         = pDomains;
    pNamesArray->count = NamesArray.count;
    pNamesArray->names = NamesArray.names;
    *pdwCount          = NamesArray.count - dwUnknownSidsNum;

cleanup:
    if (pAccountSids)
    {
        LsaSrvFreeAccountSids(pAccountSids);
    }

    return ntStatus;

error:
    if (pDomains)
    {
        LsaSrvFreeMemory(pDomains);
    }

    if (NamesArray.names)
    {
        LsaSrvFreeMemory(NamesArray.names);
    }

    *ppDomains         = NULL;
    pNamesArray->count = 0;
    pNamesArray->names = NULL;
    *pdwCount          = 0;
    goto cleanup;
}


static
NTSTATUS
LsaSrvLookupDomainSids(
    PPOLICY_CONTEXT        pPolCtx,
    PACCOUNT_SIDS          pAccountSids,
    DWORD                  dwLevel,
    RefDomainList         *pDomains,
    TranslatedNameArray2  *pNamesArray
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_ENTRY pDomEntry = NULL;
    handle_t hLsaBinding = NULL;
    POLICY_HANDLE hDcPolicy = NULL;
    DWORD dwDomIndex = 0;
    SID_ARRAY Sids = {0};
    RefDomainList *pDomain = NULL;
    TranslatedName *pDomainNames = NULL;
    DWORD dwDomainNamesCount = 0;
    DWORD iDomain = 0;
    DWORD iSid = 0;
    DWORD i = 0;

    ntStatus = LsaSrvGetDomainBySid(pPolCtx,
                                    pPolCtx->pDomainSid,
                                    &pDomEntry);
    if (ntStatus == STATUS_NO_SUCH_DOMAIN)
    {
        ntStatus = LsaSrvConnectDomainBySid(pPolCtx,
                                           pPolCtx->pDomainSid,
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

    Sids.dwNumSids = pAccountSids->dwCount;

    dwError = LwAllocateMemory(sizeof(Sids.pSids[0]) * Sids.dwNumSids,
                               OUT_PPVOID(&Sids.pSids));
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < Sids.dwNumSids; i++)
    {
        Sids.pSids[i].pSid = pAccountSids->ppSids[i];
    }

    ntStatus = LsaLookupSids(hLsaBinding,
                             hDcPolicy,
                             &Sids,
                             &pDomain,
                             &pDomainNames,
                             dwLevel,
                             &dwDomainNamesCount);
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

            for (iSid = 0; iSid < dwDomainNamesCount; iSid++)
            {
                DWORD iTransName = pAccountSids->pdwIndices[iSid];
                TranslatedName *pSrcName = &(pDomainNames[iSid]);
                TranslatedName2 *pDstName = &(pNamesArray->names[iTransName]);

                if (iDomain != pSrcName->sid_index)
                {
                    continue;
                }

                ntStatus = LsaSrvDuplicateUnicodeString(&pDstName->name,
                                                        &pSrcName->name);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);

                pDstName->type      = pSrcName->type;
                pDstName->sid_index = dwDomIndex;
                pDstName->unknown1  = 0;
            }

            pDomains->count  = (++dwDomIndex);
        }

        pNamesArray->count += dwDomainNamesCount;
    }
    else if (ntStatus == STATUS_NONE_MAPPED)
    {
        for (i = 0; i < pAccountSids->dwCount; i++)
        {
            DWORD iTransName = pAccountSids->pdwIndices[i];
            TranslatedName2 *pDstName = &(pNamesArray->names[iTransName]);

            pDstName->type      = SID_TYPE_UNKNOWN;
            pDstName->sid_index = 0;
            pDstName->unknown1  = 0;
        }

        pNamesArray->count += pAccountSids->dwCount;
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
    LW_SAFE_FREE_MEMORY(Sids.pSids);

    if (pDomEntry)
    {
        LsaSrvDomainEntryFree(&pDomEntry);
    }

    if (pDomain)
    {
        LsaRpcFreeMemory(pDomain);
    }

    if (pDomainNames)
    {
        LsaRpcFreeMemory(pDomainNames);
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
LsaSrvLookupForeignDomainSids(
    PPOLICY_CONTEXT        pPolCtx,
    PACCOUNT_SIDS          pAccountSids,
    DWORD                  dwLevel,
    RefDomainList         *pDomains,
    TranslatedNameArray2  *pNamesArray
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_ENTRY pDomEntry = NULL;
    handle_t hLsaBinding = NULL;
    POLICY_HANDLE hDcPolicy = NULL;
    PSID pDomSid = NULL;
    DWORD iDomSid = 0;
    DWORD iResolvSid = 0;
    DWORD iSid = 0;
    DWORD dwDomIndex = 0;
    ACCOUNT_SIDS ForeignSids = {0};
    SID_ARRAY Sids = {0};
    RefDomainList *pForeignDomains = NULL;
    TranslatedName *pForeignNames = NULL;
    DWORD dwForeignNamesCount = 0;
    DWORD iDomain = 0;
    DWORD i = 0;
    PSID pSid = NULL;
    DWORD dwSidIndex = 0;

    /*
     * Allocate enough space for potential sequence of sid lookups
     */
    dwError = LwAllocateMemory(
                    sizeof(ForeignSids.ppSids[0]) * pAccountSids->dwCount,
                    OUT_PPVOID(&ForeignSids.ppSids));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(ForeignSids.pdwIndices[0]) * pAccountSids->dwCount,
                    OUT_PPVOID(&ForeignSids.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(Sids.pSids[0]) * pAccountSids->dwCount,
                    OUT_PPVOID(&Sids.pSids));
    BAIL_ON_LSA_ERROR(dwError);

    while (iDomSid < pAccountSids->dwCount)
    {
        ntStatus = RtlDuplicateSid(&pDomSid,
                                   pAccountSids->ppSids[iDomSid]);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        pDomSid->SubAuthorityCount--;
        pDomSid->SubAuthority[pDomSid->SubAuthorityCount] = 0;

        iResolvSid = 0;

        for (iSid = iDomSid; iSid < pAccountSids->dwCount; iSid++)
        {
            pSid       = pAccountSids->ppSids[iSid];
            dwSidIndex = pAccountSids->pdwIndices[iSid];

            if (RtlIsPrefixSid(pDomSid, pSid))
            {
                ntStatus = RtlDuplicateSid(
                                    &(ForeignSids.ppSids[iResolvSid]),
                                    pSid);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);

                ForeignSids.pdwIndices[iResolvSid] = dwSidIndex;

                ForeignSids.dwCount = ++iResolvSid;

                /*
                 * Free the sid so it's not looked up again
                 */
                RTL_FREE(&pAccountSids->ppSids[iSid]);
                pAccountSids->ppSids[iSid] = NULL;
                pSid = NULL;
            }
        }

        for (i = 0; i < ForeignSids.dwCount; i++)
        {
            Sids.pSids[i].pSid = ForeignSids.ppSids[i];
        }
        Sids.dwNumSids = ForeignSids.dwCount;

        ntStatus = LsaSrvConnectDomainBySid(pPolCtx,
                                            pDomSid,
                                            &pDomEntry);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        hLsaBinding = pDomEntry->hLsaBinding;
        hDcPolicy   = pDomEntry->hPolicy;

        ntStatus = LsaLookupSids(hLsaBinding,
                                 hDcPolicy,
                                 &Sids,
                                 &pForeignDomains,
                                 &pForeignNames,
                                 dwLevel,
                                 &dwForeignNamesCount);
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

                for (iSid = 0; iSid < dwForeignNamesCount; iSid++)
                {
                    DWORD iTransName          = ForeignSids.pdwIndices[iSid];
                    TranslatedName *pSrcName  = &(pForeignNames[iSid]);
                    TranslatedName2 *pDstName = &(pNamesArray->names[iTransName]);

                    if (iDomain != pSrcName->sid_index)
                    {
                        continue;
                    }

                    ntStatus = LsaSrvDuplicateUnicodeString(&pDstName->name,
                                                            &pSrcName->name);
                    BAIL_ON_NTSTATUS_ERROR(ntStatus);

                    pDstName->type      = pSrcName->type;
                    pDstName->sid_index = dwDomIndex;
                    pDstName->unknown1  = 0;
                }

                pDomains->count  = (++dwDomIndex);
            }

            pNamesArray->count += dwForeignNamesCount;
        }
        else if (ntStatus == STATUS_NONE_MAPPED)
        {
            for (i = 0; i < ForeignSids.dwCount; i++)
            {
                DWORD iTransName = ForeignSids.pdwIndices[i];
                TranslatedName2 *pDstName = &(pNamesArray->names[iTransName]);

                pDstName->type      = SID_TYPE_UNKNOWN;
                pDstName->sid_index = 0;
                pDstName->unknown1  = 0;
            }

            pNamesArray->count += ForeignSids.dwCount;
        }
        else
        {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        /*
         * Free the sids array before the next round of lookup.
         * The entire allocated space (i.e. as big as pAccountSids)
         * is cleaned up.
         */
        for (i = 0; i < pAccountSids->dwCount; i++)
        {
            RTL_FREE(&(ForeignSids.ppSids[i]));
            ForeignSids.ppSids[i]     = NULL;
            Sids.pSids[i].pSid        = NULL;
            ForeignSids.pdwIndices[i] = -1;
        }
        Sids.dwNumSids = 0;

        if (pDomEntry)
        {
            LsaSrvDomainEntryFree(&pDomEntry);
        }

        if (pForeignDomains)
        {
            LsaRpcFreeMemory(pForeignDomains);
            pForeignDomains = NULL;
        }

        if (pForeignNames)
        {
            LsaRpcFreeMemory(pForeignNames);
            pForeignNames = NULL;
        }

        RTL_FREE(&pDomSid);

        /*
         * Find another domain name (first name that hasn't
         * been nulled out)
         */
        iDomSid = 0;
        while (iDomSid < pAccountSids->dwCount &&
               pAccountSids->ppSids[iDomSid] == NULL)
        {
            iDomSid++;
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
    for (i = 0; i < pAccountSids->dwCount; i++)
    {
        RTL_FREE(&(ForeignSids.ppSids[i]));
    }

    LW_SAFE_FREE_MEMORY(ForeignSids.ppSids);
    LW_SAFE_FREE_MEMORY(ForeignSids.pdwIndices);
    LW_SAFE_FREE_MEMORY(Sids.pSids);

    if (pDomEntry)
    {
        LsaSrvDomainEntryFree(&pDomEntry);
    }

    if (pForeignDomains)
    {
        LsaRpcFreeMemory(pForeignDomains);
    }

    if (pForeignNames)
    {
        LsaRpcFreeMemory(pForeignNames);
    }

    RTL_FREE(&pDomSid);

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
LsaSrvLookupLocalSids(
    PPOLICY_CONTEXT        pPolCtx,
    PACCOUNT_SIDS          pAccountSids,
    RefDomainList         *pDomains,
    TranslatedNameArray2  *pNamesArray,
    PDWORD                 pdwLocalDomainIndex
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLocalDomIndex = 0;
    LsaDomainInfo *pLocalDomainInfo = NULL;
    PDWORD pdwLocalRids = NULL;
    PWSTR *ppwszLocalNames = NULL;
    PDWORD pdwLocalTypes = NULL;
    DWORD i = 0;

    dwLocalDomIndex  = pDomains->count;
    pLocalDomainInfo = &(pDomains->domains[dwLocalDomIndex]);

    ntStatus = LsaSrvInitUnicodeStringEx(&pLocalDomainInfo->name,
                                         pPolCtx->pwszLocalDomainName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvDuplicateSid(&pLocalDomainInfo->sid,
                                  pPolCtx->pLocalDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateMemory(
                      sizeof(pdwLocalRids[0]) * pAccountSids->dwCount,
                      OUT_PPVOID(&pdwLocalRids));
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < pAccountSids->dwCount; i++)
    {
        PSID pSid = pAccountSids->ppSids[i];
        DWORD iSubAuthority = pSid->SubAuthorityCount - 1;

        if (pSid->SubAuthorityCount == 5)
        {
            pdwLocalRids[i] = pSid->SubAuthority[iSubAuthority];
        }
        else
        {
            /* This could be local machine SID to be resolved so
               just avoid accidental match with an existing RID */
            pdwLocalRids[i] = 0;
        }
    }

    ntStatus = SamrLookupRids(pPolCtx->hSamrBinding,
                              pPolCtx->hLocalDomain,
                              pAccountSids->dwCount,
                              pdwLocalRids,
                              &ppwszLocalNames,
                              &pdwLocalTypes);
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
        ntStatus != STATUS_NONE_MAPPED)
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    for (i = 0; i < pAccountSids->dwCount; i++)
    {
        DWORD iTransName = pAccountSids->pdwIndices[i];
        TranslatedName2 *pDstLocalName = &(pNamesArray->names[iTransName]);

        if ((ntStatus == STATUS_SUCCESS ||
             ntStatus == LW_STATUS_SOME_NOT_MAPPED) &&
            ppwszLocalNames[i] != NULL)
        {
            /* RID (and thus SID) has been resolved to a name */
            ntStatus = LsaSrvInitUnicodeString(&pDstLocalName->name,
                                               ppwszLocalNames[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pDstLocalName->type      = pdwLocalTypes[i];
            pDstLocalName->sid_index = dwLocalDomIndex;
        }
        else if (pAccountSids->ppSids[i]->SubAuthorityCount == 4 &&
                 RtlIsPrefixSid(pAccountSids->ppSids[i],
                                pLocalDomainInfo->sid))
        {
            /* RID is unknown because SID turns out to be
               the local machine SID, not an account SID */
            pDstLocalName->type      = SID_TYPE_COMPUTER;
            pDstLocalName->sid_index = dwLocalDomIndex;
        }
        else
        {
            /* RID is unknown */
            pDstLocalName->type      = SID_TYPE_UNKNOWN;
            pDstLocalName->sid_index = 0;
        }

        pDstLocalName->unknown1 = 0;
    }

    pDomains->count      = dwLocalDomIndex + 1;
    pNamesArray->count  += pAccountSids->dwCount;
    *pdwLocalDomainIndex = dwLocalDomIndex;

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
    LW_SAFE_FREE_MEMORY(pdwLocalRids);

    if (ppwszLocalNames)
    {
        SamrFreeMemory(ppwszLocalNames);
    }

    if (pdwLocalTypes)
    {
        SamrFreeMemory(pdwLocalTypes);
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
LsaSrvLookupBuiltinSids(
    PPOLICY_CONTEXT        pPolCtx,
    PACCOUNT_SIDS          pAccountSids,
    RefDomainList         *pDomains,
    TranslatedNameArray2  *pNamesArray,
    PDWORD                 pdwBuiltinDomainIndex
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwBuiltinDomIndex = 0;
    LsaDomainInfo *pBuiltinDomainInfo = NULL;
    WCHAR wszBuiltinDomainName[] = LSA_BUILTIN_DOMAIN_NAME;
    PSID pBuiltinDomainSid = NULL;
    PDWORD pdwBuiltinRids = NULL;
    PWSTR *ppwszBuiltinNames = NULL;
    PDWORD pdwBuiltinTypes = NULL;
    DWORD i = 0;

    dwBuiltinDomIndex  = pDomains->count;
    pBuiltinDomainInfo = &(pDomains->domains[dwBuiltinDomIndex]);

    dwError = LwAllocateWellKnownSid(WinBuiltinDomainSid,
                                     NULL,
                                     &pBuiltinDomainSid,
                                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaSrvInitUnicodeStringEx(&pBuiltinDomainInfo->name,
                                         wszBuiltinDomainName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvDuplicateSid(&pBuiltinDomainInfo->sid,
                                  pBuiltinDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateMemory(
                      sizeof(pdwBuiltinRids[0]) * pAccountSids->dwCount,
                      OUT_PPVOID(&pdwBuiltinRids));
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < pAccountSids->dwCount; i++)
    {
        PSID pSid = pAccountSids->ppSids[i];
        DWORD iSubAuthority = pSid->SubAuthorityCount - 1;

        if (pSid->SubAuthorityCount == 2)
        {
            pdwBuiltinRids[i] = pSid->SubAuthority[iSubAuthority];
        }
        else
        {
            /* This could be builtin domain SID to be resolved so
               just avoid accidental match with an existing RID */
            pdwBuiltinRids[i] = 0;
        }
    }

    ntStatus = SamrLookupRids(pPolCtx->hSamrBinding,
                              pPolCtx->hBuiltinDomain,
                              pAccountSids->dwCount,
                              pdwBuiltinRids,
                              &ppwszBuiltinNames,
                              &pdwBuiltinTypes);
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
        ntStatus != STATUS_NONE_MAPPED)
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    for (i = 0; i < pAccountSids->dwCount; i++)
    {
        DWORD iTransName = pAccountSids->pdwIndices[i];
        TranslatedName2 *pDstBuiltinName = &(pNamesArray->names[iTransName]);

        if ((ntStatus == STATUS_SUCCESS ||
             ntStatus == LW_STATUS_SOME_NOT_MAPPED) &&
            ppwszBuiltinNames[i] != NULL)
        {
            /* RID (and thus SID) has been resolved to a name */
            ntStatus = LsaSrvInitUnicodeString(&pDstBuiltinName->name,
                                               ppwszBuiltinNames[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pDstBuiltinName->type      = pdwBuiltinTypes[i];
            pDstBuiltinName->sid_index = dwBuiltinDomIndex;
        }
        else if (pAccountSids->ppSids[i]->SubAuthorityCount == 1 &&
                 RtlIsPrefixSid(pAccountSids->ppSids[i],
                                pBuiltinDomainInfo->sid))
        {
            /* RID is unknown because SID turns out to be
               the builtin domain SID, not an account SID */
            pDstBuiltinName->type      = SID_TYPE_DOMAIN;
            pDstBuiltinName->sid_index = dwBuiltinDomIndex;
        }
        else
        {
            /* RID is unknown */
            pDstBuiltinName->type      = SID_TYPE_UNKNOWN;
            pDstBuiltinName->sid_index = 0;
        }

        pDstBuiltinName->unknown1 = 0;
    }

    pDomains->count         = dwBuiltinDomIndex + 1;
    pNamesArray->count     += pAccountSids->dwCount;
    *pdwBuiltinDomainIndex  = dwBuiltinDomIndex;

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
    LW_SAFE_FREE_MEMORY(pdwBuiltinRids);

    if (ppwszBuiltinNames)
    {
        SamrFreeMemory(ppwszBuiltinNames);
    }

    if (pdwBuiltinTypes)
    {
        SamrFreeMemory(pdwBuiltinTypes);
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
