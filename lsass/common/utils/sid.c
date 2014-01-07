/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        sid.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        SID Allocation Wrappers that take care of allocation
 *        and error mapping.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"
#include <lw/base.h>

DWORD
LsaAllocateCStringFromSid(
    OUT PSTR* ppszStringSid,
    IN PSID pSid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszStringSid = NULL;
    PSTR pszResultStringSid = NULL;

    status = RtlAllocateCStringFromSid(&pszStringSid, pSid);
    dwError = LsaNtStatusToLsaError(status);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszStringSid, &pszResultStringSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LW_ERROR_SUCCESS;

cleanup:
    RTL_FREE(&pszStringSid);

    *ppszStringSid = pszResultStringSid;

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszResultStringSid);
    goto cleanup;
}

DWORD
LsaAllocateSidFromCString(
    OUT PSID* ppSid,
    IN PCSTR pszStringSid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PSID pSid = NULL;
    PSID pResultSid = NULL;
    ULONG size = 0;

    status = RtlAllocateSidFromCString(&pSid, pszStringSid);
    dwError = LsaNtStatusToLsaError(status);
    BAIL_ON_LSA_ERROR(dwError);

    size = RtlLengthSid(pSid);

    dwError = LwAllocateMemory(size, OUT_PPVOID(&pResultSid));
    BAIL_ON_LSA_ERROR(dwError);

    RtlCopyMemory(pResultSid, pSid, size);

    dwError = LW_ERROR_SUCCESS;

cleanup:
    RTL_FREE(&pSid);

    *ppSid = pResultSid;

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pResultSid);
    goto cleanup;
}

DWORD
LsaAllocateSidAppendRid(
    OUT PSID* ppSid,
    IN PSID pDomainSid,
    IN ULONG Rid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PSID pResultSid = NULL;
    ULONG size = RtlLengthRequiredSid(pDomainSid->SubAuthorityCount + 1);

    dwError = LwAllocateMemory(size, OUT_PPVOID(&pResultSid));
    BAIL_ON_LSA_ERROR(dwError);

    status = RtlCopySid(size, pResultSid, pDomainSid);
    dwError = LsaNtStatusToLsaError(status);
    BAIL_ON_LSA_ERROR(dwError);

    status = RtlAppendRidSid(size, pResultSid, Rid);
    dwError = LsaNtStatusToLsaError(status);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppSid = pResultSid;

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pResultSid);
    goto cleanup;
}
