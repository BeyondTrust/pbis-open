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
