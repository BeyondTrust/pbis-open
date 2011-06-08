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
 *        lsaerror.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Error Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LsaGetErrorMessageForLoggingEvent(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg)
{
    DWORD dwErrorBufferSize = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszErrorMsg = NULL;
    PSTR  pszErrorBuffer = NULL;

    dwErrorBufferSize = LwGetErrorString(dwErrCode, NULL, 0);

    if (!dwErrorBufferSize)
        goto cleanup;

    dwError = LwAllocateMemory(
                dwErrorBufferSize,
                (PVOID*)&pszErrorBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwLen = LwGetErrorString(dwErrCode, pszErrorBuffer, dwErrorBufferSize);

    if ((dwLen == dwErrorBufferSize) && !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
    {
        dwError = LwAllocateStringPrintf(
                     &pszErrorMsg,
                     "Error: %s [error code: %u]",
                     pszErrorBuffer,
                     dwErrCode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszErrorMsg = pszErrorMsg;

cleanup:

    LW_SAFE_FREE_STRING(pszErrorBuffer);

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszErrorMsg);

    *ppszErrorMsg = NULL;

    goto cleanup;
}

DWORD
LsaNtStatusToLsaError(
    IN NTSTATUS Status
    )
{
    switch (Status)
    {
        case STATUS_SUCCESS:
            return LW_ERROR_SUCCESS;
        case STATUS_INSUFFICIENT_RESOURCES:
            return LW_ERROR_OUT_OF_MEMORY;
        case STATUS_INVALID_SID:
            return LW_ERROR_INVALID_SID;
        default:
            return LwNtStatusToWin32Error(Status);
    }
}

NTSTATUS
LsaLsaErrorToNtStatus(
    IN DWORD LsaError
    )
{
    switch (LsaError)
    {
        case LW_ERROR_SUCCESS:
            return STATUS_SUCCESS;
        case LW_ERROR_OUT_OF_MEMORY:
            return STATUS_INSUFFICIENT_RESOURCES;
        case LW_ERROR_INVALID_SID:
            return STATUS_INVALID_SID;
        default:
        {
            NTSTATUS status = LwWin32ErrorToNtStatus(LsaError);
            return (status != (NTSTATUS)-1) ? status : STATUS_UNSUCCESSFUL;
        }
    }
}
