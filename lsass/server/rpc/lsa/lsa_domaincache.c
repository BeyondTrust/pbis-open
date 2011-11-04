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
 *        lsa_domaincache.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SAM domains cache for use with samr rpc client calls
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
VOID
LsaSrvDomainEntryDestroy(
    PDOMAIN_ENTRY *ppEntry,
    BOOLEAN        bCleanClose
    );


static
int
LsaSrvDomainKeyCompare(
    PCVOID pEntry1,
    PCVOID pEntry2
    );


static
size_t
LsaSrvDomainKeyHash(
    PCVOID pKey
    );


static
void
LsaSrvDomainHashEntryFree(
    const LW_HASH_ENTRY *pEntry
    );


static
NTSTATUS
LsaSrvDomainEntryCopy(
    PDOMAIN_ENTRY *ppOut,
    const PDOMAIN_ENTRY pIn
    );


static
NTSTATUS
LsaSrvCreateDomainKey(
    PDOMAIN_KEY *ppKey,
    PCWSTR pwszName,
    const PSID pSid
    );


static
void
LsaSrvDomainKeyFree(
    PDOMAIN_KEY *ppKey
    );


NTSTATUS
LsaSrvCreateDomainsTable(
    PLW_HASH_TABLE *ppDomains
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PLW_HASH_TABLE pDomains = NULL;

    dwError = LwHashCreate(20,
                            LsaSrvDomainKeyCompare,
                            LsaSrvDomainKeyHash,
                            LsaSrvDomainHashEntryFree,
                            NULL,
                            &pDomains);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDomains = pDomains;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    *ppDomains = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvGetDomainByName(
    PPOLICY_CONTEXT pPolCtx,
    PCWSTR pwszDomainName,
    PDOMAIN_ENTRY *ppDomain
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_KEY pKey = NULL;
    PVOID pEntry = NULL;
    PDOMAIN_ENTRY pDomain = NULL;

    BAIL_ON_INVALID_PTR(pPolCtx);
    BAIL_ON_INVALID_PTR(pwszDomainName);
    BAIL_ON_INVALID_PTR(ppDomain);

    ntStatus = LsaSrvCreateDomainKey(&pKey,
                                     pwszDomainName,
                                     NULL);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwHashGetValue(pPolCtx->pDomains,
                              (PVOID)pKey,
                              OUT_PPVOID(&pEntry));
    if (dwError == ERROR_NOT_FOUND)
    {
        ntStatus = STATUS_NO_SUCH_DOMAIN;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }
    else if (dwError != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    ntStatus = LsaSrvDomainEntryCopy(&pDomain,
                                     (PDOMAIN_ENTRY)pEntry);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppDomain = pDomain;

cleanup:
    LsaSrvDomainKeyFree(&pKey);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LsaSrvDomainEntryFree(&pDomain);

    *ppDomain = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvGetDomainBySid(
    PPOLICY_CONTEXT pPolCtx,
    const PSID pSid,
    PDOMAIN_ENTRY *ppDomain
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_KEY pKey = NULL;
    PVOID pEntry = NULL;
    PDOMAIN_ENTRY pDomain = NULL;

    BAIL_ON_INVALID_PTR(pPolCtx);
    BAIL_ON_INVALID_PTR(pSid);
    BAIL_ON_INVALID_PTR(ppDomain);

    ntStatus = LsaSrvCreateDomainKey(&pKey,
                                     NULL,
                                     pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwHashGetValue(pPolCtx->pDomains,
                              (PVOID)pKey,
                              OUT_PPVOID(&pEntry));
    if (dwError == ERROR_NOT_FOUND)
    {
        ntStatus = STATUS_NO_SUCH_DOMAIN;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }
    else if (dwError != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    ntStatus = LsaSrvDomainEntryCopy(&pDomain,
                                     (PDOMAIN_ENTRY)pEntry);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppDomain = pDomain;

cleanup:
    LsaSrvDomainKeyFree(&pKey);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LsaSrvDomainEntryFree(&pDomain);

    *ppDomain = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvSetDomain(
    PPOLICY_CONTEXT pPolCtx,
    const PDOMAIN_ENTRY pDomain
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_KEY pKeyName = NULL;
    PDOMAIN_ENTRY pEntryByName = NULL;
    PDOMAIN_KEY pKeySid = NULL;
    PDOMAIN_ENTRY pEntryBySid = NULL;

    BAIL_ON_INVALID_PTR(pPolCtx);
    BAIL_ON_INVALID_PTR(pDomain);

    if (pDomain->pwszName)
    {
        ntStatus = LsaSrvCreateDomainKey(&pKeyName,
                                         pDomain->pwszName,
                                         NULL);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvDomainEntryCopy(&pEntryByName,
                                         pDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwHashSetValue(pPolCtx->pDomains,
                                  pKeyName,
                                  pEntryByName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pDomain->pSid)
    {
        ntStatus = LsaSrvCreateDomainKey(&pKeySid,
                                         NULL,
                                         pDomain->pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvDomainEntryCopy(&pEntryBySid,
                                         pDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwHashSetValue(pPolCtx->pDomains,
                                  pKeySid,
                                  pEntryBySid);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pKeyName)
    {
        LsaSrvDomainKeyFree(&pKeyName);
    }

    if (pKeySid)
    {
        LsaSrvDomainKeyFree(&pKeySid);
    }

    if (pEntryByName)
    {
        LsaSrvDomainEntryFree(&pEntryByName);
    }

    if (pEntryBySid)
    {
        LsaSrvDomainEntryFree(&pEntryBySid);
    }

    goto cleanup;
}


VOID
LsaSrvDomainEntryFree(
    PDOMAIN_ENTRY *ppEntry
    )
{
    PDOMAIN_ENTRY pEntry = *ppEntry;

    if (!pEntry) return;

    RTL_FREE(&pEntry->pSid);
    LW_SAFE_FREE_MEMORY(pEntry->pwszName);
    LW_SAFE_FREE_MEMORY(pEntry);

    *ppEntry = NULL;
}


static
VOID
LsaSrvDomainEntryDestroy(
    PDOMAIN_ENTRY *ppEntry,
    BOOLEAN        bCleanClose
    )
{
    PDOMAIN_ENTRY pEntry = *ppEntry;

    if (!pEntry) return;

    if (bCleanClose &&
        pEntry->hLsaBinding &&
        pEntry->hPolicy)
    {
        LsaClose(pEntry->hLsaBinding, pEntry->hPolicy);
        LsaFreeBinding(&pEntry->hLsaBinding);
    }

    LsaSrvDomainEntryFree(&pEntry);

    *ppEntry = NULL;
}


static
int
LsaSrvDomainKeyCompare(
    PCVOID pK1,
    PCVOID pK2
    )
{
    int ret = 0;
    PDOMAIN_KEY pKey1 = (PDOMAIN_KEY)pK1;
    PDOMAIN_KEY pKey2 = (PDOMAIN_KEY)pK2;

    if (pKey1->eType != pKey2->eType)
    {
        ret = 1;
        goto cleanup;
    }

    switch (pKey1->eType)
    {
    case eDomainSid:
        if (!RtlEqualSid(pKey1->pSid, pKey2->pSid))
        {
            ret = 1;
        }
        break;

    case eDomainName:
        ret = wc16scmp(pKey1->pwszName, pKey2->pwszName);
        break;
    }

cleanup:
    return ret;
}


static
size_t
LsaSrvDomainKeyHash(
    PCVOID pK
    )
{
    PDOMAIN_KEY pKey = (PDOMAIN_KEY)pK;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszKeyStr = NULL;
    size_t hash = 0;

    switch (pKey->eType)
    {
    case eDomainSid:
        ntStatus = RtlAllocateCStringFromSid(
                                  &pszKeyStr,
                                  pKey->pSid);
        break;

    case eDomainName:
        ntStatus = LwRtlCStringAllocateFromWC16String(
                                  &pszKeyStr,
                                  pKey->pwszName);
        break;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    hash = LwHashCaselessStringHash(pszKeyStr);

cleanup:
    RTL_FREE(&pszKeyStr);
    return hash;

error:
    hash = 0;
    goto cleanup;
}


static
void
LsaSrvDomainHashEntryFree(
    const LW_HASH_ENTRY *pEntry
    )
{
    LsaSrvDomainKeyFree((PDOMAIN_KEY*)&pEntry->pKey);
    LsaSrvDomainEntryFree((PDOMAIN_ENTRY*)&pEntry->pValue);
}


static
NTSTATUS
LsaSrvDomainEntryCopy(
    PDOMAIN_ENTRY *ppOut,
    const PDOMAIN_ENTRY pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_ENTRY pOut = NULL;

    BAIL_ON_INVALID_PTR(ppOut);
    BAIL_ON_INVALID_PTR(pIn);

    dwError = LwAllocateMemory(sizeof(*pOut),
                               OUT_PPVOID(&pOut));
    BAIL_ON_LSA_ERROR(dwError);

    if (pIn->pwszName)
    {
        dwError = LwAllocateWc16String(&pOut->pwszName,
                                       pIn->pwszName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pIn->pSid)
    {
        ntStatus = RtlDuplicateSid(&pOut->pSid,
                                   pIn->pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pOut->hPolicy     = pIn->hPolicy;
    pOut->hLsaBinding = pIn->hLsaBinding;

    *ppOut = pOut;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pOut)
    {
        LW_SAFE_FREE_MEMORY(pOut->pwszName);
        RTL_FREE(&pOut->pSid);

        LW_SAFE_FREE_MEMORY(pOut);
    }

    if (ppOut)
    {
        *ppOut = NULL;
    }

    goto cleanup;
}


static
NTSTATUS
LsaSrvCreateDomainKey(
    PDOMAIN_KEY *ppKey,
    PCWSTR pwszName,
    const PSID pSid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_KEY pKey = NULL;

    BAIL_ON_INVALID_PTR(ppKey);

    dwError = LwAllocateMemory(sizeof(*pKey),
                               OUT_PPVOID(&pKey));
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszName)
    {
        pKey->eType = eDomainName;

        dwError = LwAllocateWc16String(&pKey->pwszName,
                                       pwszName);
        BAIL_ON_LSA_ERROR(dwError);

    }
    else if (pSid)
    {
        pKey->eType = eDomainSid;

        ntStatus = RtlDuplicateSid(&pKey->pSid,
                                   pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    *ppKey = pKey;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LsaSrvDomainKeyFree(&pKey);
    goto cleanup;
}


static
void
LsaSrvDomainKeyFree(
    PDOMAIN_KEY *ppKey
    )
{
    PDOMAIN_KEY pKey = *ppKey;

    if (!pKey) return;

    switch (pKey->eType)
    {
    case eDomainName:
        LW_SAFE_FREE_MEMORY(pKey->pwszName);
        break;

    case eDomainSid:
        RTL_FREE(&pKey->pSid);
        break;
    }

    LW_SAFE_FREE_MEMORY(pKey);
    *ppKey = NULL;
}


VOID
LsaSrvDestroyDomainsTable(
    PLW_HASH_TABLE  pDomains,
    BOOLEAN          bCleanClose
    )
{
    DWORD dwError = ERROR_SUCCESS;
    LW_HASH_ITERATOR Iter = {0};
    LW_HASH_ENTRY *pEntry = NULL;

    dwError = LwHashGetIterator(pDomains, &Iter);
    BAIL_ON_LSA_ERROR(dwError);

    while ((pEntry = LwHashNext(&Iter)) != NULL)
    {
        LsaSrvDomainKeyFree((PDOMAIN_KEY*)&pEntry->pKey);
        LsaSrvDomainEntryDestroy((PDOMAIN_ENTRY*)&pEntry->pValue,
                                 bCleanClose);
    }

error:
    return;
}


NTSTATUS
LsaSrvConnectDomainByName(
    PPOLICY_CONTEXT   pPolCtx,
    PCWSTR            pwszDomainName,
    PDOMAIN_ENTRY    *ppDomEntry
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const DWORD dwTrustFlags = NETR_TRUST_FLAG_IN_FOREST;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PIO_CREDS pCreds = NULL;
    LSA_BINDING hLsaBinding = NULL;
    POLICY_HANDLE hDcPolicy = NULL;
    NETR_BINDING hNetrBinding = NULL;
    NetrDomainTrust *pTrusts = NULL;
    DWORD dwNumTrusts = 0;
    DWORD i = 0;
    PSTR pszTrustedDomainName = NULL;
    PSTR pszTrustedDcName = NULL;
    POLICY_HANDLE hTrustedDcPolicy = NULL;
    DOMAIN_ENTRY DomEntry = {0};
    PDOMAIN_ENTRY pDomEntry = NULL;

    ntStatus = LsaSrvGetSystemCreds(&pCreds);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (pwszDomainName == NULL ||
        !wc16scasecmp(pwszDomainName, pPolCtx->pwszDomainName))
    {
        ntStatus = LsaSrvGetDomainByName(pPolCtx,
                                         pPolCtx->pwszDomainName,
                                         &pDomEntry);
        if (ntStatus == STATUS_SUCCESS)
        {
            goto cleanup;
        }
        else if (ntStatus != STATUS_NO_SUCH_DOMAIN)
        {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        /*
         * Connect to a DC of the domain we're member of
         */
        ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                         pPolCtx->pwszDcName,
                                         pCreds);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaOpenPolicy2(hLsaBinding,
                                  pPolCtx->pwszDcName,
                                  NULL,
                                  dwPolicyAccessMask,
                                  &hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwAllocateMemory(sizeof(*pDomEntry),
                                   OUT_PPVOID(&pDomEntry));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateWc16String(&pDomEntry->pwszName,
                                       pPolCtx->pwszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        pDomEntry->pSid        = NULL;
        pDomEntry->hLsaBinding = hLsaBinding;
        pDomEntry->hPolicy     = hDcPolicy;

        dwError = LsaSrvSetDomain(pPolCtx, pDomEntry);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (pwszDomainName != NULL)
    {
        /*
         * Check if requested domain is one of our trusted
         * domains and make a connection if so.
         */

        ntStatus = NetrInitBindingDefault(&hNetrBinding,
                                          pPolCtx->pwszDcName,
                                          pCreds);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = DsrEnumerateDomainTrusts(hNetrBinding,
                                            pPolCtx->pwszDcName,
                                            dwTrustFlags,
                                            &pTrusts,
                                            &dwNumTrusts);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        for (i = 0; i < dwNumTrusts; i++)
        {
            NetrDomainTrust *pTrust = &(pTrusts[i]);

            /*
             * If this trusted domain is already in the cache
             * don't try opening another connection and caching
             * it once again
             */
            ntStatus = LsaSrvGetDomainByName(pPolCtx,
                                             pTrust->netbios_name,
                                             &pDomEntry);
            if (ntStatus == STATUS_SUCCESS)
            {
                LsaSrvDomainEntryFree(&pDomEntry);
                continue;
            }
            else if (ntStatus != STATUS_NO_SUCH_DOMAIN)
            {
                BAIL_ON_NTSTATUS_ERROR(ntStatus);
            }

            dwError = LwWc16sToMbs(pTrust->dns_name,
                                   &pszTrustedDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LWNetGetDomainController(pszTrustedDomainName,
                                               &pszTrustedDcName);
            BAIL_ON_LSA_ERROR(dwError);

            ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                             pTrust->dns_name,
                                             pCreds);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ntStatus = LsaOpenPolicy2(hLsaBinding,
                                      pTrust->dns_name,
                                      NULL,
                                      dwPolicyAccessMask,
                                      &hTrustedDcPolicy);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            DomEntry.pwszName    = pTrust->netbios_name;
            DomEntry.pSid        = NULL;
            DomEntry.hLsaBinding = hLsaBinding;
            DomEntry.hPolicy     = hTrustedDcPolicy;

            dwError = LsaSrvSetDomain(pPolCtx, &DomEntry);
            BAIL_ON_LSA_ERROR(dwError);

            LW_SAFE_FREE_MEMORY(pszTrustedDomainName);
            LWNetFreeString(pszTrustedDcName);
            pszTrustedDomainName = NULL;
            pszTrustedDcName     = NULL;
            hLsaBinding          = NULL;
            hTrustedDcPolicy     = NULL;
        }

        /*
         * If the domain we're looking for is none of the trusted
         * ones try to leave it up to the our DC.
         * If we still don't have the connection to our DC at this
         * point it means an error.
         */
        ntStatus = LsaSrvGetDomainByName(pPolCtx,
                                         pwszDomainName,
                                         &pDomEntry);
        if (ntStatus == STATUS_NO_SUCH_DOMAIN)
        {
            ntStatus = LsaSrvGetDomainByName(pPolCtx,
                                             pPolCtx->pwszDomainName,
                                             &pDomEntry);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
    }

    *ppDomEntry = pDomEntry;

cleanup:
    NetrFreeBinding(&hNetrBinding);

    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (pszTrustedDcName)
    {
        LWNetFreeString(pszTrustedDcName);
    }


    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pDomEntry)
    {
        LsaSrvDomainEntryFree(&pDomEntry);
    }

    if (ppDomEntry)
    {
        *ppDomEntry = NULL;
    }

    goto cleanup;
}


NTSTATUS
LsaSrvConnectDomainBySid(
    PPOLICY_CONTEXT   pPolCtx,
    PSID              pDomainSid,
    PDOMAIN_ENTRY    *ppDomEntry
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const DWORD dwTrustFlags = NETR_TRUST_FLAG_IN_FOREST;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_CREDS pCreds = NULL;
    LSA_BINDING hLsaBinding= NULL;
    POLICY_HANDLE hDcPolicy = NULL;
    NETR_BINDING hNetrBinding = NULL;
    NetrDomainTrust *pTrusts = NULL;
    DWORD dwNumTrusts = 0;
    DWORD i = 0;
    PSTR pszTrustedDomainName = NULL;
    PSTR pszTrustedDcName = NULL;
    POLICY_HANDLE hTrustedDcPolicy = NULL;
    DOMAIN_ENTRY DomEntry = {0};
    PDOMAIN_ENTRY pDomEntry = NULL;

    ntStatus = LsaSrvGetSystemCreds(&pCreds);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (pDomainSid == NULL ||
        RtlEqualSid(pDomainSid, pPolCtx->pDomainSid))
    {
        ntStatus = LsaSrvGetDomainBySid(pPolCtx,
                                        pPolCtx->pDomainSid,
                                        &pDomEntry);
        if (ntStatus == STATUS_SUCCESS)
        {
            goto cleanup;
        }
        else if (ntStatus != STATUS_NO_SUCH_DOMAIN)
        {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        /*
         * Connect to a DC of the domain we're member of
         */
        ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                         pPolCtx->pwszDcName,
                                         pCreds);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaOpenPolicy2(hLsaBinding,
                                  pPolCtx->pwszDcName,
                                  NULL,
                                  dwPolicyAccessMask,
                                  &hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwAllocateMemory(sizeof(*pDomEntry),
                                   OUT_PPVOID(&pDomEntry));
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = RtlDuplicateSid(&pDomEntry->pSid,
                                   pPolCtx->pDomainSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        pDomEntry->pwszName    = NULL;
        pDomEntry->hLsaBinding = hLsaBinding;
        pDomEntry->hPolicy     = hDcPolicy;

        dwError = LsaSrvSetDomain(pPolCtx, pDomEntry);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (pDomainSid != NULL)
    {
        /*
         * Check if requested domain is one of our trusted
         * domains and make a connection if so.
         */

        ntStatus = NetrInitBindingDefault(&hNetrBinding,
                                          pPolCtx->pwszDcName,
                                          pCreds);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = DsrEnumerateDomainTrusts(hNetrBinding,
                                            pPolCtx->pwszDcName,
                                            dwTrustFlags,
                                            &pTrusts,
                                            &dwNumTrusts);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        for (i = 0; i < dwNumTrusts; i++)
        {
            NetrDomainTrust *pTrust = &(pTrusts[i]);

            /*
             * If this trusted domain is already in the cache
             * don't try opening another connection and caching
             * it once again
             */
            ntStatus = LsaSrvGetDomainBySid(pPolCtx,
                                            pTrust->sid,
                                            &pDomEntry);
            if (ntStatus == STATUS_SUCCESS)
            {
                LsaSrvDomainEntryFree(&pDomEntry);
                continue;
            }
            else if (ntStatus != STATUS_NO_SUCH_DOMAIN)
            {
                BAIL_ON_NTSTATUS_ERROR(ntStatus);
            }

            dwError = LwWc16sToMbs(pTrust->dns_name,
                                   &pszTrustedDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LWNetGetDomainController(pszTrustedDomainName,
                                               &pszTrustedDcName);
            BAIL_ON_LSA_ERROR(dwError);

            ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                             pTrust->dns_name,
                                             pCreds);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ntStatus = LsaOpenPolicy2(hLsaBinding,
                                      pTrust->dns_name,
                                      NULL,
                                      dwPolicyAccessMask,
                                      &hTrustedDcPolicy);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            DomEntry.pwszName    = NULL;
            DomEntry.pSid        = pTrust->sid;
            DomEntry.hLsaBinding = hLsaBinding;
            DomEntry.hPolicy     = hTrustedDcPolicy;

            dwError = LsaSrvSetDomain(pPolCtx, &DomEntry);
            BAIL_ON_LSA_ERROR(dwError);

            LW_SAFE_FREE_MEMORY(pszTrustedDomainName);
            LWNetFreeString(pszTrustedDcName);
            pszTrustedDomainName = NULL;
            pszTrustedDcName     = NULL;
            hLsaBinding          = NULL;
            hTrustedDcPolicy     = NULL;
        }

        /*
         * If the domain we're looking for is none of the trusted
         * ones try to leave it up to the our DC.
         * If we still don't have the connection to our DC at this
         * point it means an error.
         */
        ntStatus = LsaSrvGetDomainBySid(pPolCtx,
                                        pDomainSid,
                                        &pDomEntry);
        if (ntStatus == STATUS_NO_SUCH_DOMAIN)
        {
            ntStatus = LsaSrvGetDomainBySid(pPolCtx,
                                            pPolCtx->pDomainSid,
                                            &pDomEntry);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
    }

    *ppDomEntry = pDomEntry;

cleanup:
    NetrFreeBinding(&hNetrBinding);

    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (pszTrustedDcName)
    {
        LWNetFreeString(pszTrustedDcName);
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
