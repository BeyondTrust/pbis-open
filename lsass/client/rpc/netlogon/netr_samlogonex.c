/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        netr_samlogonex.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetrSamLogonEx functions.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Gerald Carter (gcarter@likewise.com)
 */

#include "includes.h"


NTSTATUS
NetrSamLogonEx(
    IN  NETR_BINDING          hBinding,
    IN  NetrCredentials      *pCreds,
    IN  PCWSTR                pwszServer,
    IN  PCWSTR                pwszDomain,
    IN  PCWSTR                pwszComputer,
    IN  PCWSTR                pwszUsername,
    IN  PCWSTR                pwszPassword,
    IN  UINT16                LogonLevel,
    IN  UINT16                ValidationLevel,
    OUT NetrValidationInfo  **ppValidationInfo,
    OUT PBYTE                 pAuthoritative
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszServerName = NULL;
    PWSTR pwszComputerName = NULL;
    NetrLogonInfo LogonInfo = {0};
    NetrPasswordInfo *pPassInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    NetrValidationInfo ValidationInfo = {0};
    NetrValidationInfo *pValidationInfo = NULL;
    BYTE Authoritative = 0;
    UINT32 Flags = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    /* pwszDomain can be NULL */
    BAIL_ON_INVALID_PTR(pwszComputer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszUsername, ntStatus);
    BAIL_ON_INVALID_PTR(pwszPassword, ntStatus);
    BAIL_ON_INVALID_PTR(pCreds, ntStatus);
    BAIL_ON_INVALID_PTR(ppValidationInfo, ntStatus);
    BAIL_ON_INVALID_PTR(pAuthoritative, ntStatus);

    if (!(LogonLevel == 1 ||
          LogonLevel == 3 ||
          LogonLevel == 5))
    {
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = LwAllocateWc16String(&pwszServerName,
                                   pwszServer);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszComputerName,
                                   pwszComputer);
    BAIL_ON_WIN_ERROR(dwError);

    ntStatus = NetrAllocateLogonPasswordInfo(NULL,
                                             &dwOffset,
                                             NULL,
                                             pwszDomain,
                                             pwszComputer,
                                             pwszUsername,
                                             pwszPassword,
                                             pCreds,
                                             &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pPassInfo),
                                 dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus)

    ntStatus = NetrAllocateLogonPasswordInfo(pPassInfo,
                                             &dwOffset,
                                             &dwSpaceLeft,
                                             pwszDomain,
                                             pwszComputer,
                                             pwszUsername,
                                             pwszPassword,
                                             pCreds,
                                             &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (LogonLevel)
    {
    case 1:
        LogonInfo.password1 = pPassInfo;
        break;

    case 3:
        LogonInfo.password3 = pPassInfo;
        break;

    case 5:
        LogonInfo.password5 = pPassInfo;
        break;
    }

    DCERPC_CALL(ntStatus, cli_NetrLogonSamLogonEx(hBinding,
                                                  pwszServerName,
                                                  pwszComputerName,
                                                  LogonLevel,
                                                  &LogonInfo,
                                                  ValidationLevel,
                                                  &ValidationInfo,
                                                  &Authoritative,
                                                  &Flags));
    BAIL_ON_NT_STATUS(ntStatus);

    dwSize   = 0;
    dwOffset = 0;

    ntStatus = NetrAllocateValidationInfo(NULL,
                                          &dwOffset,
                                          NULL,
                                          ValidationLevel,
                                          &ValidationInfo,
                                          &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pValidationInfo),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateValidationInfo(pValidationInfo,
                                          &dwOffset,
                                          &dwSpaceLeft,
                                          ValidationLevel,
                                          &ValidationInfo,
                                          &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppValidationInfo = pValidationInfo;
    *pAuthoritative   = Authoritative;

cleanup:
    NetrCleanStubValidationInfo(&ValidationInfo,
                                ValidationLevel);

    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszComputerName);
    LW_SAFE_FREE_MEMORY(pPassInfo);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pValidationInfo)
    {
        NetrFreeMemory(pValidationInfo);
    }

    if (ppValidationInfo)
    {
        *ppValidationInfo = NULL;
    }

    if (pAuthoritative)
    {
        *pAuthoritative = 0;
    }

    goto cleanup;
}


NTSTATUS
NetrSamLogonNetworkEx(
    IN  NETR_BINDING          hBinding,
    IN  PCWSTR                pwszServer,
    IN  PCWSTR                pwszDomain,
    IN  PCWSTR                pwszComputer,
    IN  PCWSTR                pwszUsername,
    IN  PBYTE                 pChallenge,
    IN  PBYTE                 pLmResp,
    IN  UINT32                LmRespLen,
    IN  PBYTE                 pNtResp,
    IN  UINT32                NtRespLen,
    IN  UINT16                LogonLevel,
    IN  UINT16                ValidationLevel,
    OUT NetrValidationInfo  **ppValidationInfo,
    OUT PBYTE                 pAuthoritative
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszServerName = NULL;
    PWSTR pwszComputerName = NULL;
    NetrLogonInfo LogonInfo = {0};
    NetrNetworkInfo *pNetInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    NetrValidationInfo ValidationInfo = {0};
    NetrValidationInfo *pValidationInfo = NULL;
    BYTE Authoritative = 0;
    UINT32 Flags = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    /* pwszDomain can be NULL */
    BAIL_ON_INVALID_PTR(pwszComputer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszUsername, ntStatus);
    BAIL_ON_INVALID_PTR(pChallenge, ntStatus);
    /* LanMan Response could be NULL */
    BAIL_ON_INVALID_PTR(pNtResp, ntStatus);
    BAIL_ON_INVALID_PTR(ppValidationInfo, ntStatus);
    BAIL_ON_INVALID_PTR(pAuthoritative, ntStatus);

    if (!(LogonLevel == 2 ||
          LogonLevel == 6))
    {
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = LwAllocateWc16String(&pwszServerName,
                                   pwszServer);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszComputerName,
                                   pwszComputer);
    BAIL_ON_WIN_ERROR(dwError);

    ntStatus = NetrAllocateLogonNetworkInfo(NULL,
                                            &dwOffset,
                                            NULL,
                                            pwszDomain,
                                            pwszComputer,
                                            pwszUsername,
                                            pChallenge,
                                            pLmResp,
                                            LmRespLen,
                                            pNtResp,
                                            NtRespLen,
                                            &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    dwError = NetrAllocateMemory(OUT_PPVOID(&pNetInfo),
                                 dwSpaceLeft);
    BAIL_ON_WIN_ERROR(dwError)

    ntStatus = NetrAllocateLogonNetworkInfo(pNetInfo,
                                            &dwOffset,
                                            &dwSpaceLeft,
                                            pwszDomain,
                                            pwszComputer,
                                            pwszUsername,
                                            pChallenge,
                                            pLmResp,
                                            LmRespLen,
                                            pNtResp,
                                            NtRespLen,
                                            &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (LogonLevel)
    {
    case 2:
        LogonInfo.network2 = pNetInfo;
        break;

    case 6:
        LogonInfo.network6 = pNetInfo;
        break;
    }

    DCERPC_CALL(ntStatus, cli_NetrLogonSamLogonEx(hBinding,
                                                  pwszServerName,
                                                  pwszComputerName,
                                                  LogonLevel,
                                                  &LogonInfo,
                                                  ValidationLevel,
                                                  &ValidationInfo,
                                                  &Authoritative,
                                                  &Flags));
    BAIL_ON_NT_STATUS(ntStatus);

    dwSize   = 0;
    dwOffset = 0;

    ntStatus = NetrAllocateValidationInfo(NULL,
                                          &dwOffset,
                                          NULL,
                                          ValidationLevel,
                                          &ValidationInfo,
                                          &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pValidationInfo),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateValidationInfo(pValidationInfo,
                                          &dwOffset,
                                          &dwSpaceLeft,
                                          ValidationLevel,
                                          &ValidationInfo,
                                          &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppValidationInfo  = pValidationInfo;
    *pAuthoritative    = Authoritative;

cleanup:
    NetrCleanStubValidationInfo(&ValidationInfo,
                                ValidationLevel);

    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszComputerName);

    if (pNetInfo)
    {
        NetrFreeMemory(pNetInfo);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pValidationInfo)
    {
        NetrFreeMemory(pValidationInfo);
    }

    *ppValidationInfo  = NULL;
    *pAuthoritative    = 0;

    goto cleanup;
}
