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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        sid.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
