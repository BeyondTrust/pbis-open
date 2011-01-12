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
// Just for legacy password info type:
#include <lwps/lwps.h>


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
    IN OPTIONAL PCSTR pszQueryDomainName,
    OUT PLWPS_PASSWORD_INFO* ppInfo
    );

DWORD
LwpsLegacyWritePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PLWPS_PASSWORD_INFO pInfo
    );

DWORD
LwpsLegacyDeletePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName
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
LwpsLegacyFreePassword(
    IN PLWPS_PASSWORD_INFO pInfo
    );

VOID
LwpsLegacyFreeStringArray(
    IN PSTR* ppszDomainList,
    IN DWORD dwCount
    );

//
// From lsapstore-backend-legacy-convert.c
//

DWORD
LwpsConvertFillMachinePasswordInfoW(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    );
///<
/// Fill LSA pstore machine password info from legacy pstore password info.
///
/// This also handles any conversions or "cons"-ing up of values that
/// are not stored in the old legacy format.
///
/// @param[in] pLegacyPasswordInfo - Legacy password information.
/// @param[out] pPasswordInfo - Returns machine account information.
///     This is freed with LsaPstoreFreePasswordInfoW().
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on failure
///

DWORD
LwpsConvertAllocateFromMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLWPS_PASSWORD_INFO* ppLegacyPasswordInfo
    );
///<
/// Conver LSA pstore machine password info into legacy pstore password info.
///
/// @param[in] pPasswordInfo - Machine password information.
/// @param[in] pLegacyPasswordInfo - Returns legacy password information.
///     This is freed with LwpsLegacyFreePasswordInfo().
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on failure
///

#endif
