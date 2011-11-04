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
 *     LSASS Domain Manager (LsaDm) Wrapper (Helper) API
 *
 * @details
 *
 *     This module wraps calls to LsaDm for the convenience of the
 *     AD provider code.
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#ifndef __LSA_DM_WRAP_H__
#define __LSA_DM_WRAP_H__

DWORD
LsaDmWrapEnumExtraForestTrustDomains(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR** pppszDomainNames,
    OUT PDWORD pdwCount
    );

DWORD
LsaDmWrapEnumExtraTwoWayForestTrustDomains(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR** pppszDomainNames,
    OUT PDWORD pdwCount
    );

DWORD
LsaDmWrapEnumInMyForestTrustDomains(
    IN LSA_DM_STATE_HANDLE hDmState,
    OUT PSTR** pppszDomainNames,
    OUT PDWORD pdwCount
    );

DWORD
LsaDmWrapGetDomainEnumInfo(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    OUT PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo
    );

DWORD
LsaDmWrapGetDomainName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDomainName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    );

DWORD
LsaDmWrapGetDomainNameAndSidByObjectSid(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszObjectSid,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSTR* ppszDomainSid
    );

//
// Connectivity-oriented calls
//

DWORD
LsaDmWrapLdapPingTcp(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName
    );

DWORD
LsaDmWrapNetLookupObjectSidByName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszName,
    OUT PSTR* ppszSid,
    OUT OPTIONAL LSA_OBJECT_TYPE* pAccountType
    );

DWORD
LsaDmWrapNetLookupNamesByObjectSids(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwSidCounts,
    IN PSTR* ppszSids,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedNames,
    OUT PDWORD pdwFoundNamesCount
    );

DWORD
LsaDmWrapNetLookupObjectSidsByNames(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwNameCounts,
    IN PSTR* ppszNames,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedSids,
    OUT PDWORD pdwFoundSidsCount
    );

DWORD
LsaDmWrapNetLookupNameByObjectSid(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN  PCSTR pszDnsDomainName,
    IN  PCSTR pszSid,
    OUT PSTR* ppszName,
    OUT OPTIONAL LSA_OBJECT_TYPE* pAccountType
    );

DWORD
LsaDmWrapDsEnumerateDomainTrusts(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwFlags,
    OUT NetrDomainTrust** ppTrusts,
    OUT PDWORD pdwCount
    );

DWORD
LsaDmWrapDsGetDcName(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszFindDomainName,
    IN BOOLEAN bReturnDnsName,
    OUT PSTR* ppszDomainDnsOrFlatName,
    OUT OPTIONAL PSTR* ppszDomainForestDnsName
    );

DWORD
LsaDmWrapAuthenticateUserEx(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsDomainName,
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    OUT PLSA_AUTH_USER_INFO *ppUserInfo
    );

#endif /* __LSA_DM_WRAP_H__ */
