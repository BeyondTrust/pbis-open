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
 *     lsapstore-main-a.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     A wrappers for main API code
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-includes.h"

DWORD
LsaPstoreGetPasswordInfoA(
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR dnsDomainName = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W passwordInfoW = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A passwordInfo = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                        &dnsDomainName,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LsaPstoreGetPasswordInfoW(
                    dnsDomainName,
                    &passwordInfoW);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepConvertWideToAnsiPasswordInfo(
                    passwordInfoW,
                    &passwordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_A(&passwordInfo);
    }

    LSA_PSTORE_FREE_PASSWORD_INFO_W(&passwordInfoW);
    LW_RTL_FREE(&dnsDomainName);

    *PasswordInfo = passwordInfo;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstoreSetPasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_PASSWORD_INFO_W passwordInfoW = NULL;

    dwError = LsaPstorepConvertAnsiToWidePasswordInfo(
                    PasswordInfo,
                    &passwordInfoW);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstoreSetPasswordInfoW(passwordInfoW);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LSA_PSTORE_FREE_PASSWORD_INFO_W(&passwordInfoW);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstoreDeletePasswordInfoA(
    IN OPTIONAL PCSTR DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR dnsDomainNameW = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                        &dnsDomainNameW,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LsaPstoreDeletePasswordInfoW(dnsDomainNameW);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LW_RTL_FREE(&dnsDomainNameW);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstoreGetDefaultDomainA(
    OUT PSTR* DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR dnsDomainNameW = NULL;
    PSTR dnsDomainName = NULL;

    dwError = LsaPstoreGetDefaultDomainW(&dnsDomainNameW);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &dnsDomainName,
                    dnsDomainNameW));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LW_RTL_FREE(&dnsDomainName);
    }

    LSA_PSTORE_FREE(&dnsDomainNameW);

    *DnsDomainName = dnsDomainName;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstoreSetDefaultDomainA(
    IN OPTIONAL PCSTR DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR dnsDomainNameW = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                        &dnsDomainNameW,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LsaPstoreSetDefaultDomainW(dnsDomainNameW);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LW_RTL_FREE(&dnsDomainNameW);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstoreGetJoinedDomainsA(
    OUT PSTR** DnsDomainNames,
    OUT PDWORD Count
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR* dnsDomainNames = NULL;
    DWORD count = 0;
    PWSTR* dnsDomainNamesW = NULL;
    DWORD countW = 0;

    dwError = LsaPstoreGetJoinedDomainsW(
                    &dnsDomainNamesW,
                    &countW);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (!countW)
    {
        GOTO_CLEANUP_EE(EE);
    }

    dwError = LwNtStatusToWin32Error(LW_RTL_ALLOCATE(
                    &dnsDomainNames,
                    VOID,
                    countW * sizeof(dnsDomainNames[0])));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    for (count = 0; count < countW; count++)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &dnsDomainNames[count],
                        dnsDomainNamesW[count]));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_STRING_ARRAY_A(&dnsDomainNames, &count);
    }

    LSA_PSTORE_FREE_STRING_ARRAY_W(&dnsDomainNamesW, &countW);

    *DnsDomainNames = dnsDomainNames;
    *Count = count;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}
