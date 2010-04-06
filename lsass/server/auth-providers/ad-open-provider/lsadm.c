/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *     lsadm.c
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) Implementation
 *
 * @details
 *
 *     This module keeps track of the state of each domain.  In addition
 *     to keeping track of domain names, SIDs, trust info, and affinity,
 *     it also keeps track of which domains are considered unreachable
 *     (and thus "offline").  A thread will try to transition each offline
 *     domain back to online by periodically checking the reachability
 *     of offline domains.
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#include "adprovider.h"
#include "lsadm_p.h"

///
/// LSASS offline state.
///
static LSA_DM_STATE_HANDLE gLsaDmState;

DWORD
LsaDmInitialize(
    IN BOOLEAN bIsOfflineBehaviorEnabled,
    IN DWORD dwCheckOnlineSeconds,
    IN DWORD dwUnknownDomainCacheTimeoutSeconds
    )
{
    DWORD dwError = 0;
    LSA_DM_STATE_HANDLE pState = NULL;

    dwError = LsaDmpStateCreate(&pState,
                                bIsOfflineBehaviorEnabled,
                                dwCheckOnlineSeconds,
                                dwUnknownDomainCacheTimeoutSeconds);
    BAIL_ON_LSA_ERROR(dwError);

    if (gLsaDmState)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    gLsaDmState = pState;
    pState = NULL;
    dwError = 0;

cleanup:
    if (pState)
    {
        LsaDmpStateDestroy(pState);
    }

    return dwError;

error:
    goto cleanup;
}

VOID
LsaDmCleanup(
    VOID
    )
{
    if (gLsaDmState)
    {
        LsaDmpStateDestroy(gLsaDmState);
        gLsaDmState = NULL;
    }
}

DWORD
LsaDmQueryState(
    OUT OPTIONAL PLSA_DM_STATE_FLAGS pStateFlags,
    OUT OPTIONAL PDWORD pdwCheckOnlineSeconds,
    OUT OPTIONAL PDWORD pdwUnknownDomainCacheTimeoutSeconds
    )
{
    return LsaDmpQueryState(
                gLsaDmState,
                pStateFlags,
                pdwCheckOnlineSeconds,
                pdwUnknownDomainCacheTimeoutSeconds);
}

DWORD
LsaDmSetState(
    IN OPTIONAL PBOOLEAN pbIsOfflineBehaviorEnabled,
    IN OPTIONAL PDWORD pdwCheckOnlineSeconds,
    IN OPTIONAL PDWORD pdwUnknownDomainCacheTimeoutSeconds
    )
{
    DWORD dwError = 0;

    if (gLsaDmState)
    {
        dwError = LsaDmpSetState(
                        gLsaDmState,
                        pbIsOfflineBehaviorEnabled,
                        pdwCheckOnlineSeconds,
                        pdwUnknownDomainCacheTimeoutSeconds);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
LsaDmMediaSenseOffline(
    VOID
    )
{
    LsaDmpMediaSenseOffline(gLsaDmState);
}

VOID
LsaDmMediaSenseOnline(
    VOID
    )
{
    LsaDmpMediaSenseOnline(gLsaDmState);
}

DWORD
LsaDmAddTrustedDomain(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PSID pDomainSid,
    IN PGUID pDomainGuid,
    IN PCSTR pszTrusteeDnsDomainName,
    IN DWORD dwTrustFlags,
    IN DWORD dwTrustType,
    IN DWORD dwTrustAttributes,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN LSA_TRUST_MODE dwTrustMode,
    IN BOOLEAN bIsTransitiveOnewayChild,
    IN OPTIONAL PCSTR pszDnsForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpAddTrustedDomain(gLsaDmState,
                                  pszDnsDomainName,
                                  pszNetbiosDomainName,
                                  pDomainSid,
                                  pDomainGuid,
                                  pszTrusteeDnsDomainName,
                                  dwTrustFlags,
                                  dwTrustType,
                                  dwTrustAttributes,
                                  dwTrustDirection,
                                  dwTrustMode,
                                  bIsTransitiveOnewayChild,
                                  pszDnsForestName,
                                  pDcInfo);
}

BOOLEAN
LsaDmIsDomainPresent(
    IN PCSTR pszDomainName
    )
{
    return LsaDmpIsDomainPresent(gLsaDmState, pszDomainName);
}

DWORD
LsaDmEnumDomainNames(
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PSTR** pppszDomainNames,
    OUT OPTIONAL PDWORD pdwCount
    )
{
    return LsaDmpEnumDomainNames(gLsaDmState,
                                 pfFilterCallback,
                                 pFilterContext,
                                 pppszDomainNames,
                                 pdwCount);
}

DWORD
LsaDmEnumDomainInfo(
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PLSA_DM_ENUM_DOMAIN_INFO** pppDomainInfo,
    OUT OPTIONAL PDWORD pdwCount
    )
{
    return LsaDmpEnumDomainInfo(gLsaDmState,
                                pfFilterCallback,
                                pFilterContext,
                                pppDomainInfo,
                                pdwCount);
}

DWORD
LsaDmQueryDomainInfo(
    IN PCSTR pszDomainName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSID* ppSid,
    OUT OPTIONAL uuid_t* pGuid,
    OUT OPTIONAL PSTR* ppszTrusteeDnsDomainName,
    OUT OPTIONAL PDWORD pdwTrustFlags,
    OUT OPTIONAL PDWORD pdwTrustType,
    OUT OPTIONAL PDWORD pdwTrustAttributes,
    OUT OPTIONAL LSA_TRUST_DIRECTION* pdwTrustDirection,
    OUT OPTIONAL LSA_TRUST_MODE* pdwTrustMode,
    OUT OPTIONAL PSTR* ppszForestName,
    OUT OPTIONAL PSTR* ppszClientSiteName,
    OUT OPTIONAL PLSA_DM_DOMAIN_FLAGS pFlags,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppDcInfo,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppGcInfo
    )
{
    return LsaDmpQueryDomainInfo(gLsaDmState,
                                 pszDomainName,
                                 ppszDnsDomainName,
                                 ppszNetbiosDomainName,
                                 ppSid,
                                 pGuid,
                                 ppszTrusteeDnsDomainName,
                                 pdwTrustFlags,
                                 pdwTrustType,
                                 pdwTrustAttributes,
                                 pdwTrustDirection,
                                 pdwTrustMode,
                                 ppszForestName,
                                 ppszClientSiteName,
                                 pFlags,
                                 ppDcInfo,
                                 ppGcInfo);
}

DWORD
LsaDmQueryDomainInfoByObjectSid(
    IN PSID pObjectSid,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSID* ppSid,
    OUT OPTIONAL uuid_t* pGuid,
    OUT OPTIONAL PSTR* ppszTrusteeDnsDomainName,
    OUT OPTIONAL PDWORD pdwTrustFlags,
    OUT OPTIONAL PDWORD pdwTrustType,
    OUT OPTIONAL PDWORD pdwTrustAttributes,
    OUT OPTIONAL LSA_TRUST_DIRECTION* pdwTrustDirection,
    OUT OPTIONAL LSA_TRUST_MODE* pdwTrustMode,
    OUT OPTIONAL PSTR* ppszForestName,
    OUT OPTIONAL PSTR* ppszClientSiteName,
    OUT OPTIONAL PLSA_DM_DOMAIN_FLAGS pFlags,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppDcInfo,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppGcInfo
    )
{
    return LsaDmpQueryDomainInfoByObjectSid(gLsaDmState,
                                 pObjectSid,
                                 ppszDnsDomainName,
                                 ppszNetbiosDomainName,
                                 ppSid,
                                 pGuid,
                                 ppszTrusteeDnsDomainName,
                                 pdwTrustFlags,
                                 pdwTrustType,
                                 pdwTrustAttributes,
                                 pdwTrustDirection,
                                 pdwTrustMode,
                                 ppszForestName,
                                 ppszClientSiteName,
                                 pFlags,
                                 ppDcInfo,
                                 ppGcInfo);
}

VOID
LsaDmFreeDcInfo(
    IN OUT PLSA_DM_DC_INFO pDcInfo
    )
{
    if (pDcInfo)
    {
        LW_SAFE_FREE_STRING(pDcInfo->pszName);
        LW_SAFE_FREE_STRING(pDcInfo->pszAddress);
        LW_SAFE_FREE_STRING(pDcInfo->pszSiteName);
        LwFreeMemory(pDcInfo);
    }
}

DWORD
LsaDmSetDomainDcInfo(
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpDomainSetDcInfoByName(gLsaDmState, pszDomainName, pDcInfo);
}

DWORD
LsaDmSetDomainGcInfo(
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpDomainSetGcInfoByName(gLsaDmState, pszDomainName, pDcInfo);
}

DWORD
LsaDmSetForceOfflineState(
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsSet
    )
{
    return LsaDmpSetForceOfflineState(gLsaDmState, pszDomainName, bIsSet);
}

DWORD
LsaDmTransitionOffline(
    IN PCSTR pszDomainName,
    IN BOOLEAN bIsGc
    )
{
    return LsaDmpTransitionOffline(gLsaDmState, pszDomainName, bIsGc);
}

DWORD
LsaDmTransitionOnline(
    IN PCSTR pszDomainName
    )
{
    return LsaDmpTransitionOnline(gLsaDmState, pszDomainName);
}

BOOLEAN
LsaDmIsDomainOffline(
    IN OPTIONAL PCSTR pszDomainName
    )
{
    return LsaDmpIsDomainOffline(gLsaDmState, pszDomainName, FALSE);
}

DWORD
LsaDmGetPrimaryDomainName(
    OUT PSTR* ppszPrimaryDomain
    )
{
    return LsaDmpGetPrimaryDomainName(gLsaDmState, ppszPrimaryDomain);
}

static
BOOLEAN
LsaDmIsForestGcOffline(
    IN OPTIONAL PCSTR pszForestName
    )
{
    return LsaDmpIsDomainOffline(gLsaDmState, pszForestName, TRUE);
}

DWORD
LsaDmDetectTransitionOnline(
    IN OPTIONAL PCSTR pszDomainName
    )
{
    return LsaDmpDetectTransitionOnline(gLsaDmState, pszDomainName);
}

VOID
LsaDmTriggerOnlindeDetectionThread(
    VOID
    )
{
    LsaDmpTriggerOnlindeDetectionThread(gLsaDmState);
}

BOOLEAN
LsaDmIsSpecificDomainNameMatch(
    IN PCSTR pszDomainNameQuery,
    IN PCSTR pszDomainName
    )
{
    BOOLEAN bIsMatch = FALSE;

    if (pszDomainName &&
        !strcasecmp(pszDomainNameQuery, pszDomainName))
    {
        bIsMatch = TRUE;
    }

    return bIsMatch;
}

BOOLEAN
LsaDmIsEitherDomainNameMatch(
    IN PCSTR pszDomainNameQuery,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName
    )
{
    BOOLEAN bIsMatch = FALSE;

    if (LsaDmIsSpecificDomainNameMatch(pszDomainNameQuery, pszDnsDomainName) ||
        LsaDmIsSpecificDomainNameMatch(pszDomainNameQuery, pszNetbiosDomainName))
    {
        bIsMatch = TRUE;
    }

    return bIsMatch;
}

BOOLEAN
LsaDmIsValidNetbiosDomainName(
    IN PCSTR pszDomainName
    )
{
    BOOLEAN bIsValid = FALSE;
    // 15-char is the limit as per http://support.microsoft.com/kb/226144,
    // but we fdo 16 to be extra safe.
    if (strlen(pszDomainName) <= 16)
    {
        bIsValid = TRUE;
    }
    return bIsValid;
}

DWORD
LsaDmDuplicateConstEnumDomainInfo(
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pSrc,
    OUT PLSA_DM_ENUM_DOMAIN_INFO* ppDest
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_DM_ENUM_DOMAIN_INFO pDest = NULL;

    dwError = LwAllocateMemory(sizeof(*pDest), (PVOID*)&pDest);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                pSrc->pszDnsDomainName,
                &pDest->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                pSrc->pszNetbiosDomainName,
                &pDest->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmpDuplicateSid(
                &pDest->pSid,
                pSrc->pSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                sizeof(*pDest->pGuid),
                (PVOID *)&pDest->pGuid);
    BAIL_ON_LSA_ERROR(dwError);
    memcpy(pDest->pGuid, pSrc->pGuid, sizeof(*pSrc->pGuid));

    dwError = LwStrDupOrNull(
                pSrc->pszTrusteeDnsDomainName,
                &pDest->pszTrusteeDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    pDest->dwTrustFlags = pSrc->dwTrustFlags;
    pDest->dwTrustType = pSrc->dwTrustType;
    pDest->dwTrustAttributes = pSrc->dwTrustAttributes;
    pDest->dwTrustDirection = pSrc->dwTrustDirection;
    pDest->dwTrustMode = pSrc->dwTrustMode;

    dwError = LwStrDupOrNull(
                pSrc->pszForestName,
                &pDest->pszForestName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                pSrc->pszClientSiteName,
                &pDest->pszClientSiteName);
    BAIL_ON_LSA_ERROR(dwError);

    pDest->Flags = pSrc->Flags;

    // ISSUE-2008/09/10-dalmeida -- Never duplicate DC info (for now, at least).
    // We currently never populate this information.
    pDest->DcInfo = NULL;
    pDest->GcInfo = NULL;

    *ppDest = pDest;

cleanup:
    return dwError;

error:
    if (pDest != NULL)
    {
        LsaDmFreeEnumDomainInfo(pDest);
    }

    *ppDest = NULL;
    goto cleanup;
}

VOID
LsaDmFreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    if (pDomainInfo)
    {
        LW_SAFE_FREE_STRING(pDomainInfo->pszDnsDomainName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszNetbiosDomainName);
        LW_SAFE_FREE_MEMORY(pDomainInfo->pSid);
        LW_SAFE_FREE_MEMORY(pDomainInfo->pGuid);
        LW_SAFE_FREE_STRING(pDomainInfo->pszTrusteeDnsDomainName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszForestName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszClientSiteName);
        if (pDomainInfo->DcInfo)
        {
            LsaDmFreeDcInfo(pDomainInfo->DcInfo);
        }
        if (pDomainInfo->GcInfo)
        {
            LsaDmFreeDcInfo(pDomainInfo->GcInfo);
        }
        LwFreeMemory(pDomainInfo);
    }
}

VOID
LsaDmFreeEnumDomainInfoArray(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo
    )
{
    if (ppDomainInfo)
    {
        DWORD dwIndex;
        for (dwIndex = 0; ppDomainInfo[dwIndex]; dwIndex++)
        {
            LsaDmFreeEnumDomainInfo(ppDomainInfo[dwIndex]);
        }
        LwFreeMemory(ppDomainInfo);
    }
}

DWORD
LsaDmLdapOpenDc(
    IN PCSTR pszDnsDomainName,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
    )
{
    return LsaDmpLdapOpen(
            gLsaDmState,
            pszDnsDomainName,
            FALSE,
            ppConn);
}

DWORD
LsaDmLdapOpenGc(
    IN PCSTR pszDnsDomainName,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
    )
{
    return LsaDmpLdapOpen(
            gLsaDmState,
            pszDnsDomainName,
            TRUE,
            ppConn);
}

DWORD
LsaDmLdapDirectorySearch(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszObjectDN,
    IN int scope,
    IN PCSTR pszQuery,
    IN PSTR* ppszAttributeList,
    OUT HANDLE* phDirectory,
    OUT LDAPMessage** ppMessage
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = NULL;
    DWORD dwTry = 0;

    while (TRUE)
    {
        hDirectory = LsaDmpGetLdapHandle(pConn);
        dwError = LwLdapDirectorySearch(
                    hDirectory,
                    pszObjectDN,
                    scope,
                    pszQuery,
                    ppszAttributeList,
                    ppMessage);
        if (LsaDmpLdapIsRetryError(dwError) && dwTry < 3)
        {
            if (dwTry > 0)
            {
                LSA_LOG_ERROR("Error code %d occurred during attempt %d of a ldap search. Retrying.", dwError, dwTry);
            }
            dwError = LsaDmpLdapReconnect(
                    gLsaDmState,
                    pConn);
            BAIL_ON_LSA_ERROR(dwError);
            dwTry++;
        }
        else if(dwError)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            break;
        }
    }

    *phDirectory = hDirectory;

cleanup:

    return dwError;

error:

    *phDirectory = NULL;
    goto cleanup;
}

DWORD
LsaDmLdapDirectoryExtendedDNSearch(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszObjectDN,
    IN PCSTR pszQuery,
    IN PSTR* ppszAttributeList,
    IN int scope,
    OUT HANDLE* phDirectory,
    OUT LDAPMessage** ppMessage
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = NULL;
    DWORD dwTry = 0;

    while (TRUE)
    {
        hDirectory = LsaDmpGetLdapHandle(pConn);
        dwError = LwLdapDirectoryExtendedDNSearch(
                        hDirectory,
                        pszObjectDN,
                        pszQuery,
                        ppszAttributeList,
                        scope,
                        ppMessage);
        if (LsaDmpLdapIsRetryError(dwError) && dwTry < 3)
        {
            LSA_LOG_ERROR("Error code %d occurred during attempt %d of a ldap search. Retrying.", dwError, dwTry);
            dwError = LsaDmpLdapReconnect(
                            gLsaDmState,
                            pConn);
            BAIL_ON_LSA_ERROR(dwError);
            dwTry++;
        }
        else if(dwError)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            break;
        }
    }

    LW_ASSERT(*ppMessage != NULL);
    *phDirectory = hDirectory;

cleanup:

    return dwError;

error:

    *phDirectory = NULL;
    goto cleanup;
}

DWORD
LsaDmLdapDirectoryOnePagedSearch(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszObjectDN,
    IN PCSTR pszQuery,
    IN PSTR* ppszAttributeList,
    IN DWORD dwPageSize,
    IN OUT PLW_SEARCH_COOKIE pCookie,
    IN int scope,
    OUT HANDLE* phDirectory,
    OUT LDAPMessage** ppMessage
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = NULL;
    DWORD dwTry = 0;

    while (TRUE)
    {
        hDirectory = LsaDmpGetLdapHandle(pConn);
        dwError = LwLdapDirectoryOnePagedSearch(
                        hDirectory,
                        pszObjectDN,
                        pszQuery,
                        ppszAttributeList,
                        dwPageSize,
                        pCookie,
                        scope,
                        ppMessage);
        if (LsaDmpLdapIsRetryError(dwError) && dwTry < 3)
        {
            // When pCookie->pfnFree is null, the cookie has not been used yet,
            // which means the cookie points to the first item in the search.
            // If it is non-null, the cookie points somewhere in the middle of
            // a ldap search, and the cookie value most likely will not be
            // valid in a new ldap connection. It is better to fail in that
            // case.
            if (pCookie->pfnFree != NULL)
            {
                LSA_LOG_ERROR("Error code %d occurred during attempt %d of a ldap search. The search cannot be retried, because a cookie was already received from the connection.", dwError, dwTry);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                LSA_LOG_ERROR("Error code %d occurred during attempt %d of a ldap search. Retrying.", dwError, dwTry);
                dwError = LsaDmpLdapReconnect(
                            gLsaDmState,
                            pConn);
                BAIL_ON_LSA_ERROR(dwError);
                dwTry++;
            }
        }
        else if(dwError)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            break;
        }
    }

    *phDirectory = hDirectory;

cleanup:

    return dwError;

error:

    *phDirectory = NULL;
    goto cleanup;
}

VOID
LsaDmLdapClose(
    IN PLSA_DM_LDAP_CONNECTION pConn
    )
{
    return LsaDmpLdapClose(
                gLsaDmState,
                pConn);
}

DWORD
LsaDmConnectDomain(
    IN PCSTR pszDnsDomainName,
    IN LSA_DM_CONNECT_DOMAIN_FLAGS dwConnectFlags,
    IN PLWNET_DC_INFO pDcInfo,
    IN PFLSA_DM_CONNECT_CALLBACK pfConnectCallback,
    IN OPTIONAL PVOID pContext
    )
{
    DWORD dwError = 0;
    PSTR pszDnsForestName = NULL;
    PCSTR pszDnsDomainOrForestName = pszDnsDomainName;
    PLWNET_DC_INFO pLocalDcInfo = NULL;
    PLWNET_DC_INFO pActualDcInfo = pDcInfo;
    DWORD dwGetDcNameFlags = 0;
    BOOLEAN bIsNetworkError = FALSE;
    BOOLEAN bUseGc = IsSetFlag(dwConnectFlags, LSA_DM_CONNECT_DOMAIN_FLAG_GC);
    BOOLEAN bUseDcInfo = IsSetFlag(dwConnectFlags, LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO);
    PSTR pszPrimaryDomain = NULL;

    if (bUseGc)
    {
        dwError = LsaDmGetForestName(pszDnsDomainName,
                                         &pszDnsForestName);
        BAIL_ON_LSA_ERROR(dwError);
        if (!pszDnsForestName)
        {
            // This is the case where there is an external trust such
            // that we do not have forest root information.
            // So let's do what we can.

            // ISSUE-2008/09/22-dalmeida -- It is likely never correct to
            // access the GC for an external trust.  We should check the
            // trust attributes here and ASSERT some invariants.
            // For now, however, we will log and try our best to comply
            // with the caller.  This should help identify whether
            // there are any mis-uses.
            LSA_LOG_WARNING("Trying to access forest root for probable external trust (%s).",
                            pszDnsDomainName);
            dwError = LsaDmpQueryForestNameFromNetlogon(
                        pszDnsDomainName,
                        &pszDnsForestName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        pszDnsDomainOrForestName = pszDnsForestName;
        dwGetDcNameFlags |= DS_GC_SERVER_REQUIRED;
     
        dwError = LsaDmGetPrimaryDomainName(&pszPrimaryDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ( (!bUseGc && LsaDmIsDomainOffline(pszDnsDomainOrForestName)) ||
         (bUseGc && LsaDmIsForestGcOffline(pszDnsDomainOrForestName)))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (IsSetFlag(dwConnectFlags, LSA_DM_CONNECT_DOMAIN_FLAG_AUTH))
    {
        dwError = AD_MachineCredentialsCacheInitialize();
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (bUseDcInfo && !pActualDcInfo)
    {
        dwError = LWNetGetDCNameExt(
            NULL,
            pszDnsDomainOrForestName,
            NULL,
            pszPrimaryDomain,
            dwGetDcNameFlags,
            0,
            NULL,
            &pLocalDcInfo);
        bIsNetworkError = LsaDmpIsNetworkError(dwError);
        BAIL_ON_LSA_ERROR(dwError);
        pActualDcInfo = pLocalDcInfo;
    }

    dwError = pfConnectCallback(pszDnsDomainOrForestName,
                                pActualDcInfo,
                                pContext,
                                &bIsNetworkError);
    if (!dwError)
    {
        goto cleanup;
    }
    if (!bIsNetworkError)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (!bUseDcInfo)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    LWNET_SAFE_FREE_DC_INFO(pLocalDcInfo);
    pActualDcInfo = NULL;
    dwError = LWNetGetDCNameExt(
        NULL,
        pszDnsDomainOrForestName,
        NULL,
        pszPrimaryDomain,
        dwGetDcNameFlags | DS_FORCE_REDISCOVERY,
        0,
        NULL,
        &pLocalDcInfo);
    bIsNetworkError = LsaDmpIsNetworkError(dwError);
    BAIL_ON_LSA_ERROR(dwError);
    pActualDcInfo = pLocalDcInfo;

    dwError = pfConnectCallback(pszDnsDomainOrForestName,
                                pActualDcInfo,
                                pContext,
                                &bIsNetworkError);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pLocalDcInfo);
    LW_SAFE_FREE_STRING(pszDnsForestName);
    LW_SAFE_FREE_MEMORY(pszPrimaryDomain);
    return dwError;

error:
    if (bIsNetworkError)
    {
        DWORD dwLocalError = LsaDmTransitionOffline(
                pszDnsDomainOrForestName,
                bUseGc);
        if (dwLocalError)
        {
            LSA_LOG_DEBUG("Error %d transitioning %s offline",
                          dwLocalError, pszDnsDomainOrForestName);
        }
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    goto cleanup;
}

DWORD
LsaDmGetForestName(
    IN PCSTR pszDomainName,
    OUT PSTR* ppszDnsForestName
    )
{
    return LsaDmQueryDomainInfo(pszDomainName,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                ppszDnsForestName,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
}

BOOLEAN
LsaDmIsUnknownDomainSid(
    IN PSID pDomainSid
    )
{
    return LsaDmpIsUnknownDomainSid(gLsaDmState, pDomainSid);
}

BOOLEAN
LsaDmIsUnknownDomainName(
    IN PCSTR pszDomainName
    )
{
    return LsaDmpIsUnknownDomainName(gLsaDmState, pszDomainName);
}

DWORD
LsaDmCacheUnknownDomainSid(
    IN PSID pDomainSid
    )
{
    return LsaDmpCacheUnknownDomainSid(gLsaDmState, pDomainSid);
}

DWORD
LsaDmCacheUnknownDomainName(
    IN PCSTR pszDomainName
    )
{
    return LsaDmpCacheUnknownDomainName(gLsaDmState, pszDomainName);
}
