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
 *     lsadmengine.h
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) Engine Private Definitions
 *
 * @details
 *
 *     This module discovers trusts and when domains come back online.
 *     It builds on top of the LsaDm state module.
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#ifndef __LSA_DM_ENGINE_H__
#define __LSA_DM_ENGINE_H__

DWORD
LsaDmEngineDiscoverTrusts(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName
    );

DWORD
LsaDmEngineGetDomainNameWithDiscovery(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName,
    IN PCSTR pszDomainName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    );

DWORD
LsaDmEngineGetDomainNameAndSidByObjectSidWithDiscovery(
    IN LSA_DM_STATE_HANDLE hDmState,
    IN PCSTR pszDnsPrimaryDomainName,
    IN PCSTR pszObjectSid,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSTR* ppszDomainSid
    );

#endif /* __LSA_DM_ENGINE_H__ */
