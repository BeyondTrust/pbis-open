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
