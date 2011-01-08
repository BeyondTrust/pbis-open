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
 *     lsapstore-backend-wrap-a.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Backend wrapper to automatically provide A functions on top of W.
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-includes.h"
#include "machinepwdinfo-impl.h"

//
// Functions
//

DWORD
LsaPstorepBackendGetPasswordInfoA(
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR dnsDomainName = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W internalPasswordInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A passwordInfo = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                        &dnsDomainName,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LsaPstorepBackendGetPasswordInfoW(
                    dnsDomainName,
                    &internalPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaImplConvertMachinePasswordInfoWideToMultiByte(
                    internalPasswordInfo,
                    &passwordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        // TODO - Mem alloc/free mismatch?
        LSA_PSTORE_FREE_PASSWORD_INFO_A(&passwordInfo);
    }

    LSA_PSTORE_FREE_PASSWORD_INFO_W(&internalPasswordInfo);
    LwRtlWC16StringFree(&dnsDomainName);

    *PasswordInfo = passwordInfo;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendSetPasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_PASSWORD_INFO_W internalPasswordInfo = NULL;

    dwError = LsaImplConvertMachinePasswordInfoMultiByteToWide(
                    PasswordInfo,
                    &internalPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepBackendSetPasswordInfoW(internalPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    // TODO - Mem alloc/free mismatch?
    LSA_PSTORE_FREE_PASSWORD_INFO_W(&internalPasswordInfo);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendDeletePasswordInfoA(
    IN OPTIONAL PCSTR DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR dnsDomainName = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                        &dnsDomainName,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LsaPstorepBackendDeletePasswordInfoW(dnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LwRtlWC16StringFree(&dnsDomainName);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendGetDefaultDomainA(
    OUT PSTR* DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR internalDnsDomainName = NULL;
    PSTR dnsDomainName = NULL;

    dwError = LsaPstorepBackendGetDefaultDomainW(&internalDnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &dnsDomainName,
                    internalDnsDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LwRtlCStringFree(&dnsDomainName);
    }

    LSA_PSTORE_FREE(&internalDnsDomainName);

    *DnsDomainName = dnsDomainName;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendSetDefaultDomainA(
    IN OPTIONAL PCSTR DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR dnsDomainName = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                        &dnsDomainName,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LsaPstorepBackendSetDefaultDomainW(dnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LwRtlWC16StringFree(&dnsDomainName);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendGetJoinedDomainsA(
    OUT PSTR** DnsDomainNames,
    OUT PDWORD Count
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR* dnsDomainNames = NULL;
    DWORD count = 0;
    PWSTR* internalDnsDomainNames = NULL;
    DWORD internalCount = 0;

    dwError = LsaPstorepBackendGetJoinedDomainsW(
                    &internalDnsDomainNames,
                    &internalCount);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (!internalCount)
    {
        GOTO_CLEANUP_EE(EE);
    }

    dwError = LwNtStatusToWin32Error(LW_RTL_ALLOCATE(
                    &dnsDomainNames,
                    VOID,
                    internalCount * sizeof(dnsDomainNames[0])));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    for (count = 0; count < internalCount; count++)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &dnsDomainNames[count],
                        internalDnsDomainNames[count]));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:
    if (dwError)
    {
        if (dnsDomainNames)
        {
            LsaPstoreFreeStringArrayA(dnsDomainNames, count);
            dnsDomainNames = NULL;
        }
        count = 0;
    }

    if (internalDnsDomainNames)
    {
        LsaPstoreFreeStringArrayW(internalDnsDomainNames, internalCount);
    }

    *DnsDomainNames = dnsDomainNames;
    *Count = count;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}
