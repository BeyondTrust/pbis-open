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
 *     lsadm.h
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) API
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

#ifndef __LSA_DM_H__
#define __LSA_DM_H__

//////////////////////////////////////////////////////////////////////
///
/// @name LSASS Domain Manager (LsaDm)
///
/// @details These functions control the LSASS domain state.
///
/// @{

struct _LSA_DM_LDAP_CONNECTION;
typedef struct _LSA_DM_LDAP_CONNECTION
    LSA_DM_LDAP_CONNECTION, *PLSA_DM_LDAP_CONNECTION;

typedef struct _LSA_DM_DC_INFO {
    DWORD dwDsFlags;
    PSTR pszName;
    PSTR pszAddress;
    PSTR pszSiteName;
} LSA_DM_DC_INFO, *PLSA_DM_DC_INFO;

typedef struct _LSA_DM_ENUM_DOMAIN_INFO {
    // Can be NULL for downlevel domain
    PSTR pszDnsDomainName;
    PSTR pszNetbiosDomainName;
    PSID pSid;
    PGUID pGuid;
    PSTR pszTrusteeDnsDomainName;
    DWORD dwTrustFlags;
    DWORD dwTrustType;
    DWORD dwTrustAttributes;
    LSA_TRUST_DIRECTION dwTrustDirection;
    LSA_TRUST_MODE dwTrustMode;
    // Can be NULL (e.g. external trust)
    PSTR pszForestName;
    PSTR pszClientSiteName;
    LSA_DM_DOMAIN_FLAGS Flags;
    PLSA_DM_DC_INFO DcInfo;
    PLSA_DM_DC_INFO GcInfo;
} LSA_DM_ENUM_DOMAIN_INFO, *PLSA_DM_ENUM_DOMAIN_INFO;

typedef struct _LSA_DM_CONST_DC_INFO {
    DWORD dwDsFlags;
    PCSTR pszName;
    PCSTR pszAddress;
    PCSTR pszSiteName;
} LSA_DM_CONST_DC_INFO, *PLSA_DM_CONST_DC_INFO;

typedef struct _LSA_DM_CONST_ENUM_DOMAIN_INFO {
    // Can be NULL for downlevel domain
    PCSTR pszDnsDomainName;
    PCSTR pszNetbiosDomainName;
    PSID pSid;
    uuid_t* pGuid;
    PCSTR pszTrusteeDnsDomainName;
    DWORD dwTrustFlags;
    DWORD dwTrustType;
    DWORD dwTrustAttributes;
    LSA_TRUST_DIRECTION dwTrustDirection;
    LSA_TRUST_MODE dwTrustMode;
    // Can be NULL (e.g. external trust)
    PCSTR pszForestName;
    PCSTR pszClientSiteName;
    LSA_DM_DOMAIN_FLAGS Flags;
    PLSA_DM_CONST_DC_INFO DcInfo;
    PLSA_DM_CONST_DC_INFO GcInfo;
} LSA_DM_CONST_ENUM_DOMAIN_INFO, *PLSA_DM_CONST_ENUM_DOMAIN_INFO;

typedef BOOLEAN (*PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK)(
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    );

typedef DWORD LSA_DM_CONNECT_DOMAIN_FLAGS;

#define LSA_DM_CONNECT_DOMAIN_FLAG_GC           0x00000001
#define LSA_DM_CONNECT_DOMAIN_FLAG_DC_INFO      0x00000002
#define LSA_DM_CONNECT_DOMAIN_FLAG_AUTH         0x00000004
#define LSA_DM_CONNECT_DOMAIN_FLAG_NETRSAMLOGON 0x00000008

typedef DWORD (*PFLSA_DM_CONNECT_CALLBACK)(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    );

DWORD
LsaDmInitialize(
    IN PLSA_AD_PROVIDER_STATE pProviderState,
    IN BOOLEAN bIsOfflineBehaviorEnabled,
    IN DWORD dwCheckOnlineSeconds,
    IN DWORD dwUnknownDomainCacheTimeoutSeconds,
    IN BOOLEAN bIgnoreAllTrusts,
    IN PSTR* ppszTrustExceptionList,
    IN DWORD dwTrustExceptionCount
    );
///<
/// Initialize state for domain manager.
///
/// This includes starting up the online detection thread.
///
/// @param[in] bIsOfflineBehaviorEnabled - Whether to enable offline behavior.
///
/// @param[in] dwCheckOnlineSeconds - How often to check whether an offline
///     domain is back online. A setting of zero disables these checks.
///
/// @param[in] dwUnknownDomainCacheTimeoutSeconds - Number of seconds to keep
///     entries in the unknown domain cache.
///
/// @param[in] bIgnoreAllTrusts - Whether to ignore all trusts (except for
///     those in ppszTrustExceptionList).
///
/// @param[in] ppszTrustExceptionList - Specific trusts to exclude/include.
///     If bIgnoreAllTrusts is not set, this is an exclusion list.
///     If bIgnoreAllTrusts is set, this is an inclusion list.
///
/// @param[in] dwTrustExceptionCount - Count of entries in
///     ppszTrustExceptionList.
///
/// @return LSA status code.
///  @arg LW_ERROR_SUCCESS on success
///  @arg !LW_ERROR_SUCCESS on failure
///
/// @note This function must be called in a race-free context.
///

VOID
LsaDmCleanup(
    IN LSA_DM_STATE_HANDLE hDmState
    );
///<
/// Cleanup state for domain manager.
///
/// This includes terminating the online detection thread.
///
/// @note This function must be called in a race-free context.
///

VOID
LsaDmResetTrusts(
    IN LSA_DM_STATE_HANDLE hDmState
    );
///<
/// Remove trusts for domain manager.
///
/// This resets trusts when switching from online to offline
/// initialization.
///

DWORD
LsaDmQueryState(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT OPTIONAL PLSA_DM_STATE_FLAGS pStateFlags,
    OUT OPTIONAL PDWORD pdwCheckOnlineSeconds,
    OUT OPTIONAL PDWORD pdwUnknownDomainCacheTimeoutSeconds
    );

DWORD
LsaDmSetState(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN OPTIONAL PBOOLEAN pbIsOfflineBehaviorEnabled,
    IN OPTIONAL PDWORD pdwCheckOnlineSeconds,
    IN OPTIONAL PDWORD pdwUnknownDomainCacheTimeoutSeconds
    );

VOID
LsaDmMediaSenseOffline(
    IN LSA_DM_STATE_HANDLE hDmState
    );

VOID
LsaDmMediaSenseOnline(
    IN LSA_DM_STATE_HANDLE hDmState
    );

// When adding a normally discovered trust
DWORD
LsaDmAddTrustedDomain(
    IN LSA_DM_STATE_HANDLE hDmState,
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
    );

// TODO-One way transitive trust
#if 0
// After calling name to SID
DWORD
LsaDmAddTransitiveOneWayDomain(
    IN PCSTR pszNetbiosDomainName,
    IN PSID pDomainSid
    );

// After figuring out the DNS name of the domain
DWORD
LsaDmSetDnsDomainNameForOneWayTransitiveDomain(
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo
    );
#endif

BOOLEAN
LsaDmIsDomainPresent(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName
    );

DWORD
LsaDmEnumDomainNames(
IN LSA_DM_STATE_HANDLE hDmState,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PSTR** pppszDomainNames,
    OUT OPTIONAL PDWORD pdwCount
    );

DWORD
LsaDmEnumDomainInfo(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PLSA_DM_ENUM_DOMAIN_INFO** pppDomainInfo,
    OUT OPTIONAL PDWORD pdwCount
    );

DWORD
LsaDmQueryDomainInfo(
    IN LSA_DM_STATE_HANDLE hDmState,
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
LsaDmQueryDomainInfoByObjectSid(
    IN LSA_DM_STATE_HANDLE hDmState,
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

VOID
LsaDmFreeDcInfo(
    IN OUT PLSA_DM_DC_INFO pDcInfo
    );

DWORD
LsaDmSetDomainDcInfo(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LsaDmSetDomainGcInfo(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LsaDmSetForceOfflineState(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsSet
    );
///<
/// Set/unset force offline state of a specific domain.
///
/// This sets/unsets whether all or a specfic domain should be forced
/// offline.  This condition is trigerred manually.  We could also do
/// the global trigger via an external (media sense) event.  Setting
/// force offline will transition a domain offline and will prevent it
/// from being transitioned online.  Unsetting the bit allows a domain
/// to be subsequently transitioned online.  If the force offline
/// setting is already in the desired state, this function succeeds.
///
/// @param[in] pszDomainName - Optional Name of domain to set/unset force
///     offline.  If NULL, operations on global force state.
///
/// @param[in] bIsSet - Whether to set/unset.
///
/// @return LSA status code.
///  @arg LW_ERROR_SUCCESS on success
///  @arg !LW_ERROR_SUCCESS on failure
///

DWORD
LsaDmTransitionOffline(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName,
    IN BOOLEAN bIsGc
    );
///<
/// Transition a domain to offline mode.
///
/// This adds the domain to the offline domains list, if it is not
/// already there.
///
/// @param[in] pszDomainName Name of domain to transition.
///
/// @return LSA status code.
///  @arg SUCCESS on success
///  @arg !SUCCESS on failure
///

DWORD
LsaDmTransitionOnline(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName
    );
///<
/// Transition a domain to online mode.
///
/// This removes a domain from the offline domains list, if it
/// is in the list and not forced offline.
///
/// @param[in] pszDomainName Name of domain to transition.
///
/// @return LSA status code.
///  @arg SUCCESS on success
///  @arg NOT_FOUND if not in list
///  @arg FORCED_OFFLINE if forced offline
///

BOOLEAN
LsaDmIsDomainOffline(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN OPTIONAL PCSTR pszDomainName
    );
///<
/// @brief Checks whether a domain is in offline mode.
///
/// @param[in] pszDomainName
///     Name of the domain to check or NULL for just global state.
///
/// @return Whether DomainName is offline (forced or otherwise).
///

DWORD
LsaDmGetPrimaryDomainName(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR* ppszPrimaryDomain
    );

DWORD
LsaDmDetectTransitionOnline(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN OPTIONAL PCSTR pszDomainName
    );
///<
///
/// Transition a domain online if it is reachable.
///
/// @param[in] pszDomainName
///     Optional name of domain to try to transition online.  If NULL,
///     tries to transition all domains.
///
/// @return LSA status code.
///  @arg SUCCESS on success
///  @arg !SUCCESS on failure
///

VOID
LsaDmTriggerOnlineDetectionThread(
    IN LSA_DM_STATE_HANDLE hDmState
    );

// TODO-inline these?

BOOLEAN
LsaDmIsSpecificDomainNameMatch(
    IN PCSTR pszDomainNameQuery,
    IN PCSTR pszDomainName
    );

BOOLEAN
LsaDmIsEitherDomainNameMatch(
    IN PCSTR pszDomainNameQuery,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName
    );

BOOLEAN
LsaDmIsValidNetbiosDomainName(
    IN PCSTR pszDomainName
    );

DWORD
LsaDmDuplicateConstEnumDomainInfo(
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pSrc,
    OUT PLSA_DM_ENUM_DOMAIN_INFO* ppDest
    );

VOID
LsaDmFreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    );

VOID
LsaDmFreeEnumDomainInfoArray(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo
    );

DWORD
LsaDmLdapOpenDc(
    IN PAD_PROVIDER_CONTEXT pProvider,
    IN PCSTR pszDnsDomainName,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
    );

DWORD
LsaDmLdapOpenGc(
    IN PAD_PROVIDER_CONTEXT pProvider,
    IN PCSTR pszDnsDomainName,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
    );

VOID
LsaDmLdapClose(
    IN PLSA_DM_LDAP_CONNECTION pConn
    );

DWORD
LsaDmLdapDirectorySearch(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszObjectDN,
    IN int scope,
    IN PCSTR pszQuery,
    IN PSTR* ppszAttributeList,
    OUT HANDLE* phDirectory,
    OUT LDAPMessage** ppMessage
    );

DWORD
LsaDmLdapDirectoryExtendedDNSearch(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszObjectDN,
    IN PCSTR pszQuery,
    IN PSTR* ppszAttributeList,
    IN int scope,
    OUT HANDLE* phDirectory,
    OUT LDAPMessage** ppMessage
    );

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
    );

DWORD
LsaDmConnectDomain(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN LSA_DM_CONNECT_DOMAIN_FLAGS dwConnectFlags,
    IN PLWNET_DC_INFO pDcInfo,
    IN PFLSA_DM_CONNECT_CALLBACK pfConnectCallback,
    IN OPTIONAL PVOID pContext
    );

DWORD
LsaDmGetForestName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName,
    OUT PSTR* ppszDnsForestName
    );

BOOLEAN
LsaDmIsUnknownDomainSid(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PSID pDomainSid
    );

BOOLEAN
LsaDmIsUnknownDomainName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName
    );

DWORD
LsaDmCacheUnknownDomainSid(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PSID pDomainSid
    );

DWORD
LsaDmCacheUnknownDomainName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName
    );

DWORD
LsaDmCacheUnknownDomainSidForever(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PSID pDomainSid
    );

DWORD
LsaDmCacheUnknownDomainNameForever(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName
    );

BOOLEAN
LsaDmIsCertainIgnoreTrust(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName
    );

BOOLEAN
LsaDmIsIgnoreTrust(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName
    );

DWORD
LsaDmQueryExcludeTrusts(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR** pppszTrustList,
    OUT PDWORD pdwTrustCount
    );

DWORD
LsaDmQueryIncludeTrusts(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR** pppszTrustList,
    OUT PDWORD pdwTrustCount
    );

/// @} lsa_om

#endif /* __LSA_DM_H__ */
