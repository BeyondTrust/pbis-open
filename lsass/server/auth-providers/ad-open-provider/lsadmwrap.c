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
 *     lsadmwrap.h
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) Wrapper (Helper) API Implementation
 *
 * @details
 *
 *     This module wraps calls to LsaDm for the convenience of the
 *     AD provider code.
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#include "adprovider.h"
#include "lsadm_p.h"

typedef struct _LSA_DM_WRAP_ENUM_ONE_DOMAIN_INFO_CALLBACK_CONTEXT {
    IN PCSTR pszDnsDomainName;
} LSA_DM_WRAP_ENUM_ONE_DOMAIN_INFO_CALLBACK_CONTEXT, *PLSA_DM_WRAP_ENUM_ONE_DOMAIN_INFO_CALLBACK_CONTEXT;

static
BOOLEAN
LsaDmWrappFilterExtraForestDomainsCallback(
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    BOOLEAN bWantThis = FALSE;

    // Find a "two-way across forest trust".  This is two-way trust to an external
    // trust to a domain in another forest or a forest trust.
    // including one-way trusts as well
    if (!(pDomainInfo->dwTrustFlags & NETR_TRUST_FLAG_IN_FOREST) &&
        (pDomainInfo->dwTrustFlags & NETR_TRUST_FLAG_OUTBOUND))
    {
        bWantThis = TRUE;
    }

    return bWantThis;
}

// ISSUE-2008/08/15-dalmeida -- The old code looked for
// two-way trusts across forest boundaries (external or forest trust).
// However, this is not necessarily correct.
DWORD
LsaDmWrapEnumExtraForestTrustDomains(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR** pppszDomainNames,
    OUT PDWORD pdwCount
    )
{
    return LsaDmEnumDomainNames(
                                hDmState,
                                LsaDmWrappFilterExtraForestDomainsCallback,
                                NULL,
                                pppszDomainNames,
                                pdwCount);
}

static
BOOLEAN
LsaDmWrappFilterExtraTwoWayForestDomainsCallback(
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    BOOLEAN bWantThis = FALSE;

    // Find a "two-way across forest trust".  This is two-way trust to an external
    // trust to a domain in another forest or a forest trust.
    // including one-way trusts as well
    if (!(pDomainInfo->dwTrustFlags & NETR_TRUST_FLAG_IN_FOREST) &&
        (pDomainInfo->dwTrustFlags & NETR_TRUST_FLAG_OUTBOUND) &&
        (pDomainInfo->dwTrustFlags & NETR_TRUST_FLAG_INBOUND))
    {
        bWantThis = TRUE;
    }

    return bWantThis;
}

// ISSUE-2008/08/15-dalmeida -- The old code looked for
// two-way trusts across forest boundaries (external or forest trust).
// However, this is not necessarily correct.
DWORD
LsaDmWrapEnumExtraTwoWayForestTrustDomains(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR** pppszDomainNames,
    OUT PDWORD pdwCount
    )
{
    return LsaDmEnumDomainNames(hDmState,
                                LsaDmWrappFilterExtraTwoWayForestDomainsCallback,
                                NULL,
                                pppszDomainNames,
                                pdwCount);
}

static
BOOLEAN
LsaDmWrappFilterInMyForestDomainsCallback(
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    BOOLEAN bWantThis = FALSE;

    // Find a "two-way across forest trust".  This is two-way trust to an external
    // trust to a domain in another forest or a forest trust.
    // including one-way trusts as well
    if (pDomainInfo->dwTrustMode == LSA_TRUST_MODE_MY_FOREST)
    {
        bWantThis = TRUE;
    }

    return bWantThis;
}


DWORD
LsaDmWrapEnumInMyForestTrustDomains(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR** pppszDomainNames,
    OUT PDWORD pdwCount
    )
{
    return LsaDmEnumDomainNames(hDmState,
                                LsaDmWrappFilterInMyForestDomainsCallback,
                                NULL,
                                pppszDomainNames,
                                pdwCount);
}

static
BOOLEAN
LsaDmWrappFilterFindDomainCallback(
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    BOOLEAN bWantThis = FALSE;
    PLSA_DM_WRAP_ENUM_ONE_DOMAIN_INFO_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_ENUM_ONE_DOMAIN_INFO_CALLBACK_CONTEXT) pContext;

    // Find a domain info given a specified domain name   
    if (!strcasecmp(pDomainInfo->pszDnsDomainName, pCtx->pszDnsDomainName))
    {
        bWantThis = TRUE;
    }

    return bWantThis;
}

DWORD
LsaDmWrapGetDomainEnumInfo(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    OUT PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_ENUM_ONE_DOMAIN_INFO_CALLBACK_CONTEXT context = { 0 };
    PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfoList = NULL;

    context.pszDnsDomainName = pszDnsDomainName;

    dwError = LsaDmEnumDomainInfo(
                               hDmState,
                               LsaDmWrappFilterFindDomainCallback,
                               &context,
                               &ppDomainInfoList,
                               NULL);
    BAIL_ON_LSA_ERROR(dwError);

    // In case of NOT found, the above function bails out
    // Double check here again
    if (!ppDomainInfoList || !ppDomainInfoList[0])
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppDomainInfo = ppDomainInfoList[0];
    ppDomainInfoList[0] = NULL;

cleanup:
    LsaDmFreeEnumDomainInfoArray(ppDomainInfoList);

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDmWrapGetDomainName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    )
{
    return LsaDmQueryDomainInfo(hDmState,
                                pszDomainName,
                                ppszDnsDomainName,
                                ppszNetbiosDomainName,
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
                                NULL,
                                NULL,
                                NULL);
}

DWORD
LsaDmWrapGetDomainNameAndSidByObjectSid(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszObjectSid,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSTR* ppszDomainSid
    )
{
    DWORD dwError = 0;
    PSID pObjectSid = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PSID pDomainSid = NULL;
    PSTR pszDomainSid = NULL;

    dwError = LsaAllocateSidFromCString(&pObjectSid, pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmQueryDomainInfoByObjectSid(
                    hDmState,
                    pObjectSid,
                    ppszDnsDomainName ? &pszDnsDomainName : NULL,
                    ppszNetbiosDomainName ? &pszNetbiosDomainName : NULL,
                    ppszDomainSid ? &pDomainSid : NULL,
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
                    NULL,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppszDomainSid)
    {
        dwError = LsaAllocateCStringFromSid(&pszDomainSid, pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pObjectSid);
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
    // set output in cleanup.
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    LW_SAFE_FREE_STRING(pszDomainSid);
    goto cleanup;
}

///
/// Callback contexts
///

typedef struct _LSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT {
    IN DWORD dwFlags;
    OUT HANDLE hDirectory;
} LSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT, *PLSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT {
    IN PLSA_AD_PROVIDER_STATE pState;
    IN PCSTR pszName;
    OUT PSTR pszSid;
    OUT LSA_OBJECT_TYPE ObjectType;
} LSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT, *PLSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT {
    IN PLSA_AD_PROVIDER_STATE pState;
    IN PCSTR pszSid;
    OUT PSTR pszNT4Name;
    OUT LSA_OBJECT_TYPE ObjectType;
} LSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT, *PLSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_LOOKUP_NAMES_BY_SIDS_CALLBACK_CONTEXT {
    IN PLSA_AD_PROVIDER_STATE pState;
    IN DWORD dwSidCounts;
    IN PSTR* ppszSids;
    OUT DWORD dwFoundNamesCount;
    OUT PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedNames;
} LSA_DM_WRAP_LOOKUP_NAMES_BY_SIDS_CALLBACK_CONTEXT, *PLSA_DM_WRAP_LOOKUP_NAMES_BY_SIDS_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_LOOKUP_SIDS_BY_NAMES_CALLBACK_CONTEXT {
    IN PLSA_AD_PROVIDER_STATE pState;
    IN DWORD dwNameCounts;
    IN PSTR* ppszNames;
    OUT DWORD dwFoundSidsCount;
    OUT PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedSids;
} LSA_DM_WRAP_LOOKUP_SIDS_BY_NAMES_CALLBACK_CONTEXT, *PLSA_DM_WRAP_LOOKUP_SIDS_BY_NAMES_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT {
    IN PLSA_AD_PROVIDER_STATE pState;
    IN DWORD dwFlags;
    OUT NetrDomainTrust* pTrusts;
    OUT DWORD dwCount;
} LSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT, *PLSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_GET_DC_NAME_CALLBACK_CONTEXT {
    IN PLSA_AD_PROVIDER_STATE pState;
    IN PCSTR pszDomainName;
    IN BOOLEAN bReturnDnsName;
    OUT PSTR pszDomainDnsOrFlatName;
    OUT PSTR pszDomainForestDnsName;
} LSA_DM_WRAP_GET_DC_NAME_CALLBACK_CONTEXT, *PLSA_DM_WRAP_GET_DC_NAME_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_AUTH_USER_EX_CALLBACK_CONTEXT {
    IN PLSA_AD_PROVIDER_STATE pState;
    IN PLSA_AUTH_USER_PARAMS pUserParams;
    OUT PLSA_AUTH_USER_INFO  *ppUserInfo;
} LSA_DM_WRAP_AUTH_USER_EX_CALLBACK_CONTEXT, *PLSA_DM_WRAP_AUTH_USER_EX_CALLBACK_CONTEXT;

///
/// Callback functions
///

static
DWORD
LsaDmWrappLdapPingTcpCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;

    dwError = LwLdapPingTcp(pDcInfo->pszDomainControllerAddress, 5);
    *pbIsNetworkError = dwError ? TRUE : FALSE;
    return dwError;
}

static
DWORD
LsaDmWrappLookupSidByNameCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT) pContext;

    dwError = AD_NetLookupObjectSidByName(
                    pCtx->pState,
                    pDcInfo->pszDomainControllerName,
                    pCtx->pszName,
                    &pCtx->pszSid,
                    &pCtx->ObjectType,
                    pbIsNetworkError);
    return dwError;
}

static
DWORD
LsaDmWrappLookupNameBySidCallback(
    IN PCSTR     pszDnsDomainOrForestName,
    IN OPTIONAL  PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL  PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    PLSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT) pContext;

    return AD_NetLookupObjectNameBySid(
                    pCtx->pState,
                    pDcInfo->pszDomainControllerName,
                    pCtx->pszSid,
                    &pCtx->pszNT4Name,
                    &pCtx->ObjectType,
                    pbIsNetworkError);
}

static
DWORD
LsaDmWrappLookupNamesBySidsCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    PLSA_DM_WRAP_LOOKUP_NAMES_BY_SIDS_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_LOOKUP_NAMES_BY_SIDS_CALLBACK_CONTEXT) pContext;

    return AD_NetLookupObjectNamesBySids(
                    pCtx->pState,
                    pDcInfo->pszDomainControllerName,
                    pCtx->dwSidCounts,
                    pCtx->ppszSids,
                    &pCtx->ppTranslatedNames,
                    &pCtx->dwFoundNamesCount,
                    pbIsNetworkError);
}

static
DWORD
LsaDmWrappLookupSidsByNamesCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    PLSA_DM_WRAP_LOOKUP_SIDS_BY_NAMES_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_LOOKUP_SIDS_BY_NAMES_CALLBACK_CONTEXT) pContext;

    return AD_NetLookupObjectSidsByNames(
                    pCtx->pState,
                    pDcInfo->pszDomainControllerName,
                    pCtx->dwNameCounts,
                    pCtx->ppszNames,
                    &pCtx->ppTranslatedSids,
                    &pCtx->dwFoundSidsCount,
                    pbIsNetworkError);
}

static
DWORD
LsaDmWrappAddTrailingDot(
    IN PCSTR pszFqdn,
    OUT PSTR* ppszDottedFqdn
    )
{
    DWORD dwError = 0;
    size_t sLen = 0;
    PSTR pszResult = NULL;

    BAIL_ON_INVALID_STRING(pszFqdn);

    sLen = strlen(pszFqdn);
    if (pszFqdn[sLen - 1] == '.')
    {
        dwError = LwAllocateString(
                        pszFqdn,
                        &pszResult);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
                        &pszResult,
                        "%s.",
                        pszFqdn);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszDottedFqdn = pszResult;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszResult);
    *ppszDottedFqdn = NULL;
    goto cleanup;
}

static
DWORD
LsaDmWrappDsEnumerateDomainTrustsCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT) pContext;
    PSTR pszFqdnDC = NULL;

    dwError = LsaDmWrappAddTrailingDot(
                    pDcInfo->pszDomainControllerName,
                    &pszFqdnDC);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_DsEnumerateDomainTrusts(
                    pCtx->pState,
                    pszFqdnDC,
                    pCtx->dwFlags,
                    &pCtx->pTrusts,
                    &pCtx->dwCount,
                    pbIsNetworkError);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszFqdnDC);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaDmWrappDsGetDcNameCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_DM_WRAP_GET_DC_NAME_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_GET_DC_NAME_CALLBACK_CONTEXT) pContext;

    dwError = AD_DsGetDcName(
                    pCtx->pState,
                    pDcInfo->pszDomainControllerName,
                    pCtx->pszDomainName,
                    pCtx->bReturnDnsName,
                    &pCtx->pszDomainDnsOrFlatName,
                    &pCtx->pszDomainForestDnsName,
                    pbIsNetworkError);
    return dwError;
}

///
/// Connect wrap functions
///

DWORD
LsaDmWrapLdapPingTcp(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName
    )
{
    DWORD dwError = 0;

    dwError = LsaDmConnectDomain(hDmState,
                                      pszDnsDomainName,
                                      LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappLdapPingTcpCallback,
                                      NULL);
    return dwError;
}

DWORD
LsaDmWrapNetLookupObjectSidByName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszName,
    OUT PSTR* ppszSid,
    OUT OPTIONAL LSA_OBJECT_TYPE* pAccountType
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT context = { 0 };

    LsaDmpGetProviderState(hDmState, &context.pState);
    context.pszName = pszName;

    dwError = LsaDmConnectDomain(hDmState,
                                      pszDnsDomainName,
                                      LSA_DM_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappLookupSidByNameCallback,
                                      &context);

    *ppszSid = context.pszSid;

    if (pAccountType)
    {
        *pAccountType = context.ObjectType;
    }

    return dwError;
}

DWORD
LsaDmWrapNetLookupNameByObjectSid(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN  PCSTR pszDnsDomainName,
    IN  PCSTR pszSid,
    OUT PSTR* ppszName,
    OUT OPTIONAL LSA_OBJECT_TYPE* pAccountType
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT context = { 0 };

    LsaDmpGetProviderState(hDmState, &context.pState);
    context.pszSid = pszSid;

    dwError = LsaDmConnectDomain(hDmState,
                                      pszDnsDomainName,
                                      LSA_DM_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappLookupNameBySidCallback,
                                      &context);

    *ppszName = context.pszNT4Name;

    if (pAccountType)
    {
        *pAccountType = context.ObjectType;
    }

    return dwError;
}

DWORD
LsaDmWrapNetLookupNamesByObjectSids(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwSidCounts,
    IN PSTR* ppszSids,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedNames,
    OUT PDWORD pdwFoundNamesCount
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_LOOKUP_NAMES_BY_SIDS_CALLBACK_CONTEXT context = { 0 };

    LsaDmpGetProviderState(hDmState, &context.pState);
    context.ppszSids = ppszSids;
    context.dwSidCounts = dwSidCounts;

    dwError = LsaDmConnectDomain(hDmState,
                                      pszDnsDomainName,
                                      LSA_DM_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappLookupNamesBySidsCallback,
                                      &context);

    *pdwFoundNamesCount = context.dwFoundNamesCount;
    *pppTranslatedNames = context.ppTranslatedNames;

    return dwError;
}

DWORD
LsaDmWrapNetLookupObjectSidsByNames(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwNameCounts,
    IN PSTR* ppszNames,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedSids,
    OUT PDWORD pdwFoundSidsCount
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_LOOKUP_SIDS_BY_NAMES_CALLBACK_CONTEXT context = { 0 };

    LsaDmpGetProviderState(hDmState, &context.pState);
    context.ppszNames = ppszNames;
    context.dwNameCounts = dwNameCounts;

    dwError = LsaDmConnectDomain(hDmState,
                                      pszDnsDomainName,
                                      LSA_DM_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappLookupSidsByNamesCallback,
                                      &context);

    *pdwFoundSidsCount = context.dwFoundSidsCount;
    *pppTranslatedSids = context.ppTranslatedSids;

    return dwError;
}

DWORD
LsaDmWrapDsEnumerateDomainTrusts(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwFlags,
    OUT NetrDomainTrust** ppTrusts,
    OUT PDWORD pdwCount
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT context = { 0 };

    LsaDmpGetProviderState(hDmState, &context.pState);
    context.dwFlags = dwFlags;

    dwError = LsaDmConnectDomain(hDmState,
                                      pszDnsDomainName,
                                      LSA_DM_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappDsEnumerateDomainTrustsCallback,
                                      &context);

    *ppTrusts = context.pTrusts;
    *pdwCount = context.dwCount;

    return dwError;
}

DWORD
LsaDmWrapDsGetDcName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszFindDomainName,
    IN BOOLEAN bReturnDnsName,
    OUT PSTR* ppszDomainDnsOrFlatName,
    OUT OPTIONAL PSTR* ppszDomainForestDnsName
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_GET_DC_NAME_CALLBACK_CONTEXT context = { 0 };

    LsaDmpGetProviderState(hDmState, &context.pState);
    context.pszDomainName = pszFindDomainName;
    context.bReturnDnsName = bReturnDnsName;

    dwError = LsaDmConnectDomain(hDmState,
                                 pszDnsDomainName,
                                 LSA_DM_CONNECT_DOMAIN_FLAG_AUTH |
                                 LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO,
                                 NULL,
                                 LsaDmWrappDsGetDcNameCallback,
                                 &context);

    *ppszDomainDnsOrFlatName = context.pszDomainDnsOrFlatName;
    if (ppszDomainForestDnsName)
    {
        *ppszDomainForestDnsName = context.pszDomainForestDnsName;
    }
    else
    {
        LW_SAFE_FREE_STRING(context.pszDomainForestDnsName);
    }

    return dwError;
}

static
DWORD
LsaDmWrappAuthenticateUserExCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN PLWNET_DC_INFO pDcInfo,
    IN PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    PLSA_DM_WRAP_AUTH_USER_EX_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_AUTH_USER_EX_CALLBACK_CONTEXT) pContext;

    return AD_NetlogonAuthenticationUserEx(
                    pCtx->pState,
                    pDcInfo->pszDomainControllerName,
                    pCtx->pUserParams,
                    pCtx->ppUserInfo,
                    pbIsNetworkError);
}

DWORD
LsaDmWrapAuthenticateUserEx(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    OUT PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_AUTH_USER_EX_CALLBACK_CONTEXT context = { 0 };

    LsaDmpGetProviderState(hDmState, &context.pState);
    context.pUserParams = pUserParams;
    context.ppUserInfo = ppUserInfo;

    dwError = LsaDmConnectDomain(hDmState,
                                      pszDnsDomainName,
                                      LSA_DM_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO |
                                      LSA_DM_CONNECT_DOMAIN_FLAG_NETRSAMLOGON,
                                      NULL,
                                      LsaDmWrappAuthenticateUserExCallback,
                                      &context);

    *ppUserInfo = *(context.ppUserInfo);

    return dwError;
}
