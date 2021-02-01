/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

/**
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
