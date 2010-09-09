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
 *     lsadm_p.h
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) Private Definitions
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

#ifndef __LSA_DM_P_H__
#define __LSA_DM_P_H__

struct _LSA_DM_STATE;
typedef struct _LSA_DM_STATE *LSA_DM_STATE_HANDLE, **PLSA_DM_STATE_HANDLE;

typedef BOOLEAN (*PLSA_DM_ENUM_DOMAIN_CALLBACK)(
    IN OPTIONAL PCSTR pszEnumDomainName,
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    );

DWORD
LsaDmpStateCreate(
    OUT PLSA_DM_STATE_HANDLE pHandle,
    IN BOOLEAN bIsOfflineBehaviorEnabled,
    IN DWORD dwCheckOnlineSeconds,
    IN DWORD dwUnknownCacheTimeoutSeconds,
    IN BOOLEAN bIgnoreAllTrusts,
    IN PSTR* ppszTrustExceptionList,
    IN DWORD dwTrustExceptionCount
    );

VOID
LsaDmpStateDestroy(
    IN OUT LSA_DM_STATE_HANDLE Handle
    );

DWORD
LsaDmpQueryState(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT OPTIONAL PLSA_DM_STATE_FLAGS pStateFlags,
    OUT OPTIONAL PDWORD pdwCheckOnlineSeconds,
    OUT OPTIONAL PDWORD pdwUnknownDomainCacheTimeoutSeconds
    );

DWORD
LsaDmpSetState(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PBOOLEAN pbIsOfflineBehaviorEnabled,
    IN OPTIONAL PDWORD pdwCheckOnlineSeconds,
    IN OPTIONAL PDWORD pdwUnknownDomainCacheTimeoutSeconds
    );

VOID
LsaDmpMediaSenseOffline(
    IN LSA_DM_STATE_HANDLE Handle
    );

VOID
LsaDmpMediaSenseOnline(
    IN LSA_DM_STATE_HANDLE Handle
    );

DWORD
LsaDmpAddTrustedDomain(
    IN LSA_DM_STATE_HANDLE Handle,
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
    IN OPTIONAL PCSTR pDnsForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo
    );

BOOLEAN
LsaDmpIsDomainPresent(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    );

VOID
LsaDmpEnumDomains(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName,
    IN PLSA_DM_ENUM_DOMAIN_CALLBACK pfCallback,
    IN OPTIONAL PVOID pContext
    );

DWORD
LsaDmpEnumDomainNames(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PSTR** pppszDomainNames,
    OUT OPTIONAL PDWORD pdwCount
    );

DWORD
LsaDmpEnumDomainInfo(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PLSA_DM_ENUM_DOMAIN_INFO** pppDomainInfo,
    OUT OPTIONAL PDWORD pdwCount
    );

DWORD
LsaDmpQueryDomainInfo(
    IN LSA_DM_STATE_HANDLE Handle,
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
    );

DWORD
LsaDmpQueryDomainInfoByObjectSid(
    IN LSA_DM_STATE_HANDLE Handle,
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
    );

DWORD
LsaDmpDomainSetDcInfoByName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LsaDmpDomainSetGcInfoByName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LsaDmpSetForceOfflineState(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsSet
    );

DWORD
LsaDmpTransitionOffline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN BOOLEAN bIsGc
    );

DWORD
LsaDmpTransitionOnline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    );

BOOLEAN
LsaDmpIsDomainOffline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsGC
    );

DWORD
LsaDmpGetPrimaryDomainName(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT PSTR* ppszPrimaryDomain
    );

DWORD
LsaDmpDetectTransitionOnline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName
    );

VOID
LsaDmpTriggerOnlindeDetectionThread(
    IN LSA_DM_STATE_HANDLE Handle
    );

DWORD
LsaDmpDuplicateSid(
    OUT PSID* ppSid,
    IN PSID pSid
    );

HANDLE
LsaDmpGetLdapHandle(
    IN PLSA_DM_LDAP_CONNECTION pConn
    );

DWORD
LsaDmpLdapReconnect(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OUT PLSA_DM_LDAP_CONNECTION pLdap
    );

DWORD
LsaDmpLdapOpen(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bUseGc,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
    );

VOID
LsaDmpLdapClose(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PLSA_DM_LDAP_CONNECTION pConn
    );

BOOLEAN
LsaDmpLdapIsRetryError(
    DWORD dwError
    );

DWORD
LsaDmpQueryForestNameFromNetlogon(
    IN PCSTR pszDnsDomainName,
    OUT PSTR* ppszDnsForestName
    );

BOOLEAN
LsaDmpIsNetworkError(
    IN DWORD dwError
    );

BOOLEAN
LsaDmpIsUnknownDomainSid(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pDomainSid
    );

BOOLEAN
LsaDmpIsUnknownDomainName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    );

DWORD
LsaDmpCacheUnknownDomainSid(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pDomainSid
    );

DWORD
LsaDmpCacheUnknownDomainName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    );

DWORD
LsaDmpCacheUnknownDomainSidForever(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pDomainSid
    );

DWORD
LsaDmpCacheUnknownDomainNameForever(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    );

BOOLEAN
LsaDmpIsCertainIgnoreTrust(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    );

BOOLEAN
LsaDmpIsIgnoreTrust(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName
    );

DWORD
LsaDmpQueryExcludeTrusts(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT PSTR** pppszTrustList,
    OUT PDWORD pdwTrustCount
    );

DWORD
LsaDmpQueryIncludeTrusts(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT PSTR** pppszTrustList,
    OUT PDWORD pdwTrustCount
    );

VOID
ADLogMediaSenseOnlineEvent(
    VOID
    );

VOID
ADLogMediaSenseOfflineEvent(
    VOID
    );

VOID
ADLogDomainOnlineEvent(
    PCSTR pszDomainName
    );

VOID
ADLogDomainOfflineEvent(
    IN PCSTR pszDomainName,
    IN BOOLEAN bIsGc
    );

#endif /* __LSA_DM_P_H__ */
