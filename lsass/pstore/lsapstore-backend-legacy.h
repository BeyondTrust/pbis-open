/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *  
 * Module Name:
 *
 *     lsapstore-backend-legacy.h
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Legacy Backend Common Includes
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LSA_PSTORE_BACKEND_LEGACY__
#define __LSA_PSTORE_BACKEND_LEGACY__

#include <lw/types.h>
#include <lw/attrs.h>

#include <lsa/lsapstore-types.h>

//
// From lsapstore-backend-legacy-internal.c
//

typedef struct _LWPS_LEGACY_STATE *PLWPS_LEGACY_STATE;

DWORD
LwpsLegacyOpenProvider(
    OUT PLWPS_LEGACY_STATE* ppContext
    );

VOID
LwpsLegacyCloseProvider(
    IN PLWPS_LEGACY_STATE pContext
    );

DWORD
LwpsLegacyReadPassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PCSTR pszDnsDomainName,
    OUT OPTIONAL PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    );

DWORD
LwpsLegacyWritePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    );

DWORD
LwpsLegacyDeletePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PCSTR pszDomainName
    );

DWORD
LwpsLegacyGetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR* ppszDomainName
    );

DWORD
LwpsLegacySetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName
    );

DWORD
LwpsLegacyGetJoinedDomains(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR** pppszDomainList,
    OUT PDWORD pdwDomainCount
    );

VOID
LwpsLegacyFreeStringArray(
    IN PSTR* ppszDomainList,
    IN DWORD dwCount
    );

#endif
