/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/**
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * @file
 *
 *     lsadmengine.c
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) Engine Implementation
 *
 * @details
 *
 *     This module discovers trusts and when domains come back online.
 *     It builds on top of the LsaDm state module.
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#include "adprovider.h"
#include "lsadm_p.h"

static
DWORD
LsaDmEnginepAddTrust(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN OPTIONAL PCSTR pszTrusteeDomainName,
    IN NetrDomainTrust* pTrustInfo,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN LSA_TRUST_MODE dwTrustMode,
    IN BOOLEAN bIsTransitiveOnewayChild,
    IN OPTIONAL PCSTR pszDnsForestName
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;

    dwError = LwWc16sToMbs(pTrustInfo->dns_name,
                            &pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pTrustInfo->netbios_name,
                            &pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmAddTrustedDomain(hDmState,
                                    pszDnsDomainName,
                                    pszNetbiosDomainName,
                                    pTrustInfo->sid,
                                    &pTrustInfo->guid,
                                    pszTrusteeDomainName,
                                    pTrustInfo->trust_flags,
                                    pTrustInfo->trust_type,
                                    pTrustInfo->trust_attrs,
                                    dwTrustDirection,
                                    dwTrustMode,
                                    bIsTransitiveOnewayChild,
                                    pszDnsForestName,
                                    NULL);
    if (dwError == LW_ERROR_DUPLICATE_DOMAINNAME ||
        dwError == LW_ERROR_NO_SUCH_DOMAIN)
    {
        // We enumerate at the current domain we are joined to
        // And later we might enumerate at the forest root
        // Since we request all members of the forest at each
        // query, we would get duplicates.
        // Ignore the duplicates, and also ignore the domains without domainSid.
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pszNetbiosDomainName);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaDmEnginepDiscoverTrustsForDomain(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszParentTrusteeDomainName,
    IN PCSTR pszDomainName,
    IN PCSTR pszForestName,
    OUT PSTR** ppTrustedForestRootList
    )
{
    DWORD dwError = 0;
    NetrDomainTrust* pTrusts = NULL;
    DWORD dwTrustCount = 0;
    DWORD dwTrustIndex = 0;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosName = NULL;
    NetrDomainTrust* pPrimaryTrust = NULL;
    PSTR* pTrustedForestRootList = NULL;
    DWORD dwTrustedForestRootListCount = 0;
    PLWNET_DC_INFO pDcInfo = NULL;

    // Enumerate all direct and in-forest trusts.
    // ISSUE-2008/08/14-dalmeida -- Why not just direct outbound and in-forest?
    dwError = LsaDmWrapDsEnumerateDomainTrusts(hDmState,
                                               pszDomainName,
                                               (NETR_TRUST_FLAG_OUTBOUND |
                                                NETR_TRUST_FLAG_INBOUND |
                                                NETR_TRUST_FLAG_IN_FOREST),
                                               &pTrusts,
                                               &dwTrustCount);
    BAIL_ON_LSA_ERROR(dwError);

    // There must be at least one trust (primary)
    if (dwTrustCount < 1)
    {
        LSA_LOG_DEBUG("No entries returned from trust enumeration for '%s'",
                      pszDomainName);
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // First, if we are the primary domain find primary trust info
    // so that we can add it first.
    if (!pszParentTrusteeDomainName)
    {
        // Since there is no trustee, this is the machine's domain.
        // So let's find the entry for the primary domain.
        for (dwTrustIndex = 0; dwTrustIndex < dwTrustCount; dwTrustIndex++)
        {
            NetrDomainTrust* pCurrentTrust = &pTrusts[dwTrustIndex];
            if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_PRIMARY)
            {
                if (pPrimaryTrust)
                {
                    // multiple primary trusts are not allowed
                    dwError = LW_ERROR_INTERNAL;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                pPrimaryTrust = pCurrentTrust;
            }
        }
        // If we did not find the primary trust info, become sad.
        if (!pPrimaryTrust)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // double-check that this is the expected domain name
        dwError = LwWc16sToMbs(pPrimaryTrust->dns_name, &pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        if (strcasecmp(pszDnsDomainName, pszDomainName))
        {
            LSA_LOG_DEBUG("Primary domain mismatch: got '%s', wanted '%s'", pszDnsDomainName, pszDomainName);
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Now add the primary trust info.
        dwError = LsaDmEnginepAddTrust(
                        hDmState,
                        NULL,
                        pPrimaryTrust,
                        LSA_TRUST_DIRECTION_SELF,
                        LSA_TRUST_MODE_MY_FOREST,
                        FALSE,
                        pszForestName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // allocate trusted forest root results, if the caller wants it.
    if (ppTrustedForestRootList)
    {
        dwError = LwAllocateMemory(sizeof(pTrustedForestRootList[0]) * dwTrustCount,
                                    (PVOID*)&pTrustedForestRootList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (dwTrustIndex = 0; dwTrustIndex < dwTrustCount; dwTrustIndex++)
    {
        NetrDomainTrust* pCurrentTrust = &pTrusts[dwTrustIndex];
        PCSTR pszCurrentTrustForestRootName = NULL;
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
        LSA_TRUST_MODE dwTrustMode = LSA_TRUST_MODE_UNKNOWN;

        LW_SAFE_FREE_MEMORY(pszDnsDomainName);
        LW_SAFE_FREE_STRING(pszNetbiosName);

        if (pCurrentTrust->dns_name)
        {
            dwError = LwWc16sToMbs(
                            pCurrentTrust->dns_name,
                            &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pCurrentTrust->netbios_name)
        {
            dwError = LwWc16sToMbs(
                            pCurrentTrust->netbios_name,
                            &pszNetbiosName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Ignore DOWNLEVEL trusts.
        // These are trusts with domains that are earlier than WIN2K
        if (pCurrentTrust->trust_type == NETR_TRUST_TYPE_DOWNLEVEL)
        {
            LSA_LOG_WARNING("Ignoring down level trust to domain '%s'",
                            LSA_SAFE_LOG_STRING(pszNetbiosName));

            if (!LW_IS_NULL_OR_EMPTY_STR(pszNetbiosName))
            {
                dwError = LsaDmCacheUnknownDomainName(
                              hDmState,
                              pszNetbiosName);
                BAIL_ON_LSA_ERROR(dwError);
            }

            continue;
        }

        // if we do not want forest root links, we are enumerating a
        // forest transitive trust, in which case we just want in-forest
        // trusts.
        if (!ppTrustedForestRootList &&
            !(pCurrentTrust->trust_flags & NETR_TRUST_FLAG_IN_FOREST))
        {
            continue;
        }

        // skip the primary trust.
        if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_PRIMARY)
        {
            if (pszParentTrusteeDomainName)
            {
                // We try to catch multiple primary trusts here
                // in case we did not check in the non-trustee case
                // earlier.
                if (pPrimaryTrust)
                {
                    // multiple primary trusts are not allowed
                    dwError = LW_ERROR_INTERNAL;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                pPrimaryTrust = pCurrentTrust;
            }
            continue;
        }

        if (LW_IS_NULL_OR_EMPTY_STR(pszDnsDomainName))
        {
            LSA_LOG_WARNING("Skipping trust with an invalid DNS domain name (NetBIOS name is '%s')",
                            LSA_SAFE_LOG_STRING(pszNetbiosName));
            continue;
        }

        // skip back-link.
        if (pszParentTrusteeDomainName &&
            !strcasecmp(pszDnsDomainName, pszParentTrusteeDomainName))
        {
            continue;
        }

        // skip ignored trust.
        if (LsaDmIsIgnoreTrust(hDmState, pszDnsDomainName, pszNetbiosName))
        {
            LSA_LOG_WARNING("Skipping ignored trust '%s' (NetBIOS name is '%s')",
                            LSA_SAFE_LOG_STRING(pszDnsDomainName),
                            LSA_SAFE_LOG_STRING(pszNetbiosName));

            dwError = LsaDmCacheUnknownDomainNameForever(
                          hDmState,
                          pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmCacheUnknownDomainNameForever(
                          hDmState,
                          pszNetbiosName);
            BAIL_ON_LSA_ERROR(dwError);

            continue;
        }

        if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_IN_FOREST)
        {
            pszCurrentTrustForestRootName = pszForestName;
            // check whether we are in other forest or not
            if (ppTrustedForestRootList)
            {
                dwTrustMode = LSA_TRUST_MODE_MY_FOREST;

            }
            else
            {
                dwTrustMode = LSA_TRUST_MODE_OTHER_FOREST;
            }
         }
        else if (pCurrentTrust->trust_attrs & NETR_TRUST_ATTR_FOREST_TRANSITIVE)
        {
            // This is a forest trust, so the forest name is the same as the
            // domain name.
            pszCurrentTrustForestRootName = pszDnsDomainName;
            dwTrustMode = LSA_TRUST_MODE_OTHER_FOREST;
        }
        else
        {
            // This must be an external trust.  So we do not ever
            // need the forest name as we are not supposed to
            // ever talk to it.
            pszCurrentTrustForestRootName = NULL;
            dwTrustMode = LSA_TRUST_MODE_EXTERNAL;
        }

        //Determine trust direction
        if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_IN_FOREST)
        {
            dwTrustDirection = LSA_TRUST_DIRECTION_TWO_WAY;
        }
        else if ((pCurrentTrust->trust_flags & NETR_TRUST_FLAG_OUTBOUND) &&
                (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_INBOUND))
        {
           dwTrustDirection = LSA_TRUST_DIRECTION_TWO_WAY;
        }
        else if ((pCurrentTrust->trust_flags & NETR_TRUST_FLAG_OUTBOUND) &&
                !(pCurrentTrust->trust_flags & NETR_TRUST_FLAG_INBOUND))
        {
           dwTrustDirection = LSA_TRUST_DIRECTION_ONE_WAY;
        }
        else if (!(pCurrentTrust->trust_flags & NETR_TRUST_FLAG_OUTBOUND) &&
                (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_INBOUND))
        {
           dwTrustDirection = LSA_TRUST_DIRECTION_ZERO_WAY;
        }
        else
        {
           dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
           LSA_LOG_WARNING("Trust direction cannot be determined.");
        }

        // Add the trust.
        dwError = LsaDmEnginepAddTrust(
                         hDmState,
                         pszDomainName,
                         pCurrentTrust,
                         dwTrustDirection,
                         dwTrustMode,
                         FALSE,
                         pszCurrentTrustForestRootName);
        BAIL_ON_LSA_ERROR(dwError);

        // If the caller wants trusted forest roots, add them to the
        // output list of trusted forest roots.
        if (ppTrustedForestRootList &&
            (pCurrentTrust->trust_attrs & NETR_TRUST_ATTR_FOREST_TRANSITIVE) &&
            (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_OUTBOUND) &&
            (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_INBOUND))
        {
            // note that we only want 2-way trusts because otherwise
            // we cannot enumerate the trusted forest.
            if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_IN_FOREST)
            {
                LSA_LOG_WARNING("Skipping unexpected in-forest 2-way transitive forest trust '%s' -> '%s'",
                                pszDomainName, pszDnsDomainName);
                continue;
            }

            dwError = LwAllocateString(pszDnsDomainName,
                                        &pTrustedForestRootList[dwTrustedForestRootListCount]);
            BAIL_ON_LSA_ERROR(dwError);

            dwTrustedForestRootListCount++;
        }
    }

    if (!dwTrustedForestRootListCount)
    {
        // Return NULL rather than an empty list
        LW_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);
    }

cleanup:

    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    LW_SAFE_FREE_STRING(pszNetbiosName);
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);

    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    if (ppTrustedForestRootList)
    {
        *ppTrustedForestRootList = pTrustedForestRootList;
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);
    goto cleanup;
}

static
DWORD
LsaDmEnginepDiscoverTrustsInternal(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName,
    IN PCSTR pszDnsPrimaryForestName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR* pTrustedForestRootList = NULL;
    BOOLEAN bIsForestRoot = FALSE;

    // determine whether the primary domain is a forest root
    if (!strcasecmp(pszDnsPrimaryForestName, pszDnsPrimaryDomainName))
    {
        bIsForestRoot = TRUE;
    }

    // enumerate own domain's trusts
    dwError = LsaDmEnginepDiscoverTrustsForDomain(
                  hDmState,
                  NULL,
                  pszDnsPrimaryDomainName,
                  pszDnsPrimaryForestName,
                  &pTrustedForestRootList);
    if (dwError != ERROR_SUCCESS)
    {
        LSA_LOG_ERROR(
            "Fatal error enumerating trusts for domain %s. Error was %s (%u)\n",
            pszDnsPrimaryDomainName,
            LW_SAFE_LOG_STRING(LwWin32ErrorToName(dwError)),
            dwError);
    }
    BAIL_ON_LSA_ERROR(dwError);

    // If own domain is not the forest root, enumerate the forest root
    if (!bIsForestRoot)
    {
        if (pTrustedForestRootList)
        {
            LSA_LOG_ERROR("Unexpected trusted forest root list when "
                          "enumerating trusts for '%s'", pszDnsPrimaryDomainName);
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaDmEnginepDiscoverTrustsForDomain(
                      hDmState,
                      pszDnsPrimaryDomainName,
                      pszDnsPrimaryForestName,
                      pszDnsPrimaryForestName,
                      &pTrustedForestRootList);
        if (dwError != ERROR_SUCCESS)
        {
            LSA_LOG_ERROR(
                "Ignoring failure enumerating trusts for forest %s. "
                "Error was %s (%u)\n",
                pszDnsPrimaryForestName,
                LW_SAFE_LOG_STRING(LwWin32ErrorToName(dwError)),
                dwError);
            dwError = ERROR_SUCCESS;
        }	
    }

    if (pTrustedForestRootList)
    {
        size_t i;
        for (i = 0; pTrustedForestRootList[i]; i++)
        {
            PSTR pszDnsForestName = pTrustedForestRootList[i];

            dwError = LsaDmEnginepDiscoverTrustsForDomain(
                          hDmState,
                          pszDnsPrimaryForestName,
                          pszDnsForestName,
                          pszDnsForestName,
                          NULL);
            if (dwError != ERROR_SUCCESS)
            {
                LSA_LOG_ERROR(
                    "Ignoring failure enumerating trusts for forest %s. "
                    "Error was %s (%u)\n",
                    pszDnsForestName,
                    LW_SAFE_LOG_STRING(LwWin32ErrorToName(dwError)),
                    dwError);
                dwError = ERROR_SUCCESS;
            }
        }
    }

cleanup:
    LW_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaDmEnginepDiscoverIncludeTrusts(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName
    )
{
    DWORD dwError = 0;
    PSTR* ppszTrustsList = NULL;
    DWORD dwTrustsCount = 0;
    DWORD i = 0;

    dwError = LsaDmQueryIncludeTrusts(
                    hDmState,
                    &ppszTrustsList,
                    &dwTrustsCount);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwTrustsCount; i++)
    {
        dwError = LsaDmEngineGetDomainNameWithDiscovery(
                        hDmState,
                        pszDnsPrimaryDomainName,
                        ppszTrustsList[i],
                        NULL,
                        NULL);
        if (LW_ERROR_NO_SUCH_DOMAIN == dwError)
        {
            LSA_LOG_WARNING("Cannot find domain '%s' from include list, "
                            "so skipping it",
                            ppszTrustsList[i]);
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LwFreeStringArray(ppszTrustsList, dwTrustsCount);

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDmEngineDiscoverTrusts(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_AD_PROVIDER_STATE pProviderState = NULL;
    PLWNET_DC_INFO pDcInfo = NULL;

    LsaDmpGetProviderState(hDmState, &pProviderState);

    pProviderState->TrustDiscovery.bIsDiscoveringTrusts = TRUE;

    // ISSUE-2008/10/09-dalmeida -- Perhaps put this in lsadmwrap.
    dwError = LWNetGetDCName(NULL, pszDnsPrimaryDomainName, NULL, 0, &pDcInfo);
    switch (dwError)
    {
        case DNS_ERROR_BAD_PACKET:
        case NERR_DCNotFound:
            // We pinged a DC earlier, so we must have gone offline
            // in the last few seconds.
            dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
            break;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmEnginepDiscoverTrustsInternal(hDmState,
                                                 pszDnsPrimaryDomainName,
                                                 pDcInfo->pszDnsForestName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmEnginepDiscoverIncludeTrusts(
                  hDmState,
                  pszDnsPrimaryDomainName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    pProviderState->TrustDiscovery.bIsDiscoveringTrusts = FALSE;

    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaDmEnginepGetDomainSidFromObjectSidString(
    OUT PSID* ppDomainSid,
    IN PCSTR pszObjectSid
    )
{
    DWORD dwError = 0;
    PSID pSid = NULL;

    dwError = LsaAllocateSidFromCString(&pSid, pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    if (pSid->SubAuthorityCount < 1)
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pSid->SubAuthorityCount--;

cleanup:
    *ppDomainSid = pSid;

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSid);
    goto cleanup;
}

static
DWORD
LsaDmEnginepAddOneWayOtherForestDomain(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PSID pDomainSid,
    IN PCSTR pszDnsForestName
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pProviderState = NULL;
    GUID guid = { 0 };
    PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo = NULL;

    LsaDmpGetProviderState(hDmState, &pProviderState);

    dwError = LsaDmAddTrustedDomain(
                     hDmState,
                     pszDnsDomainName,
                     pszNetbiosDomainName,
                     pDomainSid,
                     &guid,
                     pszDnsPrimaryDomainName,
                     0,
                     0,
                     0,
                     LSA_TRUST_DIRECTION_ONE_WAY,
                     LSA_TRUST_MODE_OTHER_FOREST,
                     TRUE,
                     pszDnsForestName,
                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmWrapGetDomainEnumInfo(
                    hDmState,
                    pszDnsDomainName,
                    &pDomainInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pProviderState->TrustDiscovery.bIsDiscoveringTrusts)
    {
        dwError = ADState_AddDomainTrust(
                        pProviderState->pszDomainName,
                        pDomainInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LsaDmFreeEnumDomainInfo(pDomainInfo);

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDmEngineGetDomainNameWithDiscovery(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName,
    IN PCSTR pszDomainName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pProviderState = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PSTR pszDomainSid = NULL;
    PSTR pszDnsForestName = NULL;
    PSID pDomainSid = NULL;
    LSA_OBJECT_TYPE accountType = LSA_OBJECT_TYPE_UNDEFINED;
    BOOLEAN bIsLocalDomain = FALSE;

    if (LW_IS_NULL_OR_EMPTY_STR(pszDomainName) ||
        AdIsSpecialDomainName(pszDomainName))
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Check whether this is the local domain.

    dwError = LsaSrvProviderServicesDomain(LSA_PROVIDER_TAG_LOCAL,
                                           pszDomainName,
                                           &bIsLocalDomain);
    if (LW_ERROR_INVALID_AUTH_PROVIDER == dwError)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (bIsLocalDomain)
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Check the domain manager

    dwError = LsaDmWrapGetDomainName(hDmState,
                                     pszDomainName,
                                     &pszDnsDomainName,
                                     &pszNetbiosDomainName);
    if (LW_ERROR_NO_SUCH_DOMAIN != dwError)
    {
        BAIL_ON_LSA_ERROR(dwError);
        // On success, we are done.
        goto cleanup;
    }

    // Check unknown domain cache to avoid always hitting the
    // network before deciding to ignore bogus domain names.

    if (LsaDmIsUnknownDomainName(hDmState, pszDomainName))
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Make sure that this name is not supposed to be ignored.
    if (LsaDmIsCertainIgnoreTrust(hDmState, pszDomainName))
    {
        // Ensure all known names and SID for this domain are in
        // unknown cache forever.

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Go to the network

    LsaDmpGetProviderState(hDmState, &pProviderState);

    dwError = LsaDmWrapNetLookupObjectSidByName(
                 hDmState,
                 pszDnsPrimaryDomainName,
                 pszDomainName,
                 &pszDomainSid,
                 &accountType);
    if (dwError)
    {
        // This domain name is not resolvable, so cache it.
        dwError = LsaDmCacheUnknownDomainName(
                      hDmState,
                      pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (LW_IS_NULL_OR_EMPTY_STR(pszDomainSid))
    {
        LSA_LOG_ERROR("Missing SID for name '%s'", pszDomainName);
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (LSA_OBJECT_TYPE_DOMAIN != accountType)
    {
        LSA_LOG_ERROR("Non-domain account type %d for name '%s'",
                accountType, pszDomainName);
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateSidFromCString(&pDomainSid,
                                        pszDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    // Need to add newly discovered domain to trusted domain list.

    dwError = LsaDmWrapDsGetDcName(
                    hDmState,
                    pszDnsPrimaryDomainName,
                    pszDomainName,
                    TRUE,
                    &pszDnsDomainName,
                    &pszDnsForestName);
    if (LW_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        // This domain name is not resolvable, so cache it.
        dwError = LsaDmCacheUnknownDomainName(
                      hDmState,
                      pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainSid(
                      hDmState,
                      pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    BAIL_ON_LSA_ERROR(dwError);

    // Make sure that this name is not supposed to be ignored.
    if (LsaDmIsCertainIgnoreTrust(hDmState, pszDnsDomainName))
    {
        // Ensure all known names and SID for this domain are in
        // unknown cache forever.

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainSidForever(
                      hDmState,
                      pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmWrapDsGetDcName(
                    hDmState,
                    pszDnsPrimaryDomainName,
                    pszDomainName,
                    FALSE,
                    &pszNetbiosDomainName,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    // Make sure that this name is not supposed to be ignored.
    if (LsaDmIsIgnoreTrust(hDmState, pszDnsDomainName, pszNetbiosDomainName))
    {
        // Ensure all known names and SID for this domain are in
        // unknown cache forever.

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainSidForever(
                      hDmState,
                      pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmEnginepAddOneWayOtherForestDomain(
                    hDmState,
                    pszDnsPrimaryDomainName,
                    pszDnsDomainName,
                    pszNetbiosDomainName,
                    pDomainSid,
                    pszDnsForestName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pDomainSid);

    if (!ppszDnsDomainName)
    {
        LW_SAFE_FREE_STRING(pszDnsDomainName);
    }
    if (!ppszNetbiosDomainName)
    {
        LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    }
    LW_SAFE_FREE_STRING(pszDomainSid);
    LW_SAFE_FREE_STRING(pszDnsForestName);

    if (ppszDnsDomainName)
    {
        *ppszDnsDomainName = pszDnsDomainName;
    }

    if (ppszNetbiosDomainName)
    {
        *ppszNetbiosDomainName = pszNetbiosDomainName;
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszNetbiosDomainName);

    goto cleanup;
}

DWORD
LsaDmEngineGetDomainNameAndSidByObjectSidWithDiscovery(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName,
    IN PCSTR pszObjectSid,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSTR* ppszDomainSid
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pProviderState = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PSTR pszDomainSid = NULL;
    PSTR pszDnsForestName = NULL;
    LSA_OBJECT_TYPE accountType = LSA_OBJECT_TYPE_UNDEFINED;
    PSID pDomainSid = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszObjectSid) ||
        AdIsSpecialDomainSidPrefix(pszObjectSid))
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmWrapGetDomainNameAndSidByObjectSid(
                    hDmState,
                    pszObjectSid,
                    &pszDnsDomainName,
                    &pszNetbiosDomainName,
                    &pszDomainSid);
    if (LW_ERROR_NO_SUCH_DOMAIN != dwError)
    {
        BAIL_ON_LSA_ERROR(dwError);
        // On success, we are done.
        goto cleanup;
    }

    dwError = LsaDmEnginepGetDomainSidFromObjectSidString(
                    &pDomainSid,
                    pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    // Check SID history domain SID cache to avoid always hitting the
    // network before deciding to ignore SID history SIDs.

    if (LsaDmIsUnknownDomainSid(hDmState, pDomainSid))
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateCStringFromSid(&pszDomainSid, pDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    // Note that domain objects cannot have SID history as the AD schema
    // does not allow for it.

    LsaDmpGetProviderState(hDmState, &pProviderState);

    dwError = LsaDmWrapNetLookupNameByObjectSid(
                    hDmState,
                    pszDnsPrimaryDomainName,
                    pszDomainSid,
                    &pszNetbiosDomainName,
                    &accountType);
    if (LW_ERROR_NO_SUCH_OBJECT == dwError)
    {
        // This domain SID is not resolvable, so cache it.
        dwError = LsaDmCacheUnknownDomainSid(
                      hDmState,
                      pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwError)
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (LW_IS_NULL_OR_EMPTY_STR(pszNetbiosDomainName))
    {
        LSA_LOG_ERROR("Missing name for SID '%s'", pszDomainSid);
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (LSA_OBJECT_TYPE_DOMAIN != accountType)
    {
        LSA_LOG_ERROR("Non-domain account type %d for SID '%s'",
                accountType, pszDomainSid);
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Make sure that this name is not supposed to be ignored.
    if (LsaDmIsCertainIgnoreTrust(hDmState, pszNetbiosDomainName))
    {
        // Ensure all known names and SID for this domain are in
        // unknown cache forever.

        dwError = LsaDmCacheUnknownDomainSidForever(
                      hDmState,
                      pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmWrapDsGetDcName(
                    hDmState,
                    pszDnsPrimaryDomainName,
                    pszNetbiosDomainName,
                    TRUE,
                    &pszDnsDomainName,
                    &pszDnsForestName);
    if (LW_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        // This domain name is not resolvable, so cache it.
        dwError = LsaDmCacheUnknownDomainName(
                      hDmState,
                      pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainSid(
                      hDmState,
                      pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    BAIL_ON_LSA_ERROR(dwError);

    // Make sure that this name is not supposed to be ignored.
    if (LsaDmIsIgnoreTrust(hDmState, pszDnsDomainName, pszNetbiosDomainName))
    {
        // Ensure all known names and SID for this domain are in
        // unknown cache forever.

        dwError = LsaDmCacheUnknownDomainSidForever(
                      hDmState,
                      pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmCacheUnknownDomainNameForever(
                      hDmState,
                      pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmEnginepAddOneWayOtherForestDomain(
                    hDmState,
                    pszDnsPrimaryDomainName,
                    pszDnsDomainName,
                    pszNetbiosDomainName,
                    pDomainSid,
                    pszDnsForestName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (!ppszDnsDomainName)
    {
        LW_SAFE_FREE_STRING(pszDnsDomainName);
    }
    if (!ppszNetbiosDomainName)
    {
        LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    }
    if (!ppszDomainSid)
    {
        LW_SAFE_FREE_STRING(pszDomainSid);
    }
    LW_SAFE_FREE_STRING(pszDnsForestName);
    LW_SAFE_FREE_MEMORY(pDomainSid);

    if (ppszDnsDomainName)
    {
        *ppszDnsDomainName = pszDnsDomainName;
    }

    if (ppszNetbiosDomainName)
    {
        *ppszNetbiosDomainName = pszNetbiosDomainName;
    }

    if (ppszDomainSid)
    {
        *ppszDomainSid = pszDomainSid;
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    LW_SAFE_FREE_STRING(pszDomainSid);

    goto cleanup;
}
