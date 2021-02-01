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

typedef BOOLEAN (*PLSA_DM_ENUM_DOMAIN_CALLBACK)(
    IN OPTIONAL PCSTR pszEnumDomainName,
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    );

DWORD
LsaDmpStateCreate(
    OUT PLSA_DM_STATE_HANDLE pHandle,
    IN PLSA_AD_PROVIDER_STATE pProviderState,
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

VOID
LsaDmpGetProviderState(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT PLSA_AD_PROVIDER_STATE *ppProviderState
    );

BOOLEAN
LsaDmpIsDomainOffline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsGC
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
    IN OUT PLSA_DM_LDAP_CONNECTION pLdap
    );

DWORD
LsaDmpLdapOpen(
    IN PAD_PROVIDER_CONTEXT pProvider,
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bUseGc,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
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
