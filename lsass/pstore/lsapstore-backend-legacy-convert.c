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
 *     lsapstore-backend-legacy-convert.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Legacy Backend Conversion Implementation
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-backend-legacy-convert.h"
#include "lsapstore-backend-legacy-internal.h"
#include "lsapstore-includes.h"
#include <lwstr.h>
#include <lwmem.h>
// These includes work around bugs in lwstr.h:
#include <string.h>
#include <wc16str.h>
// Needed for SIZEOF_TIME_T:
#include "config.h"

static
inline
PCWSTR
LocalStringOrEmptyW(
    IN OPTIONAL PCWSTR String
    )
{
    static PCWSTR empty = { 0 };
    return String ? String : empty;
}

static
inline
DWORD
LwpsConvertWC16StringDuplicateOrNull(
    OUT PWSTR* ppszNewString,
    IN PCWSTR pszOriginalString
)
{
    DWORD dwError = 0;

    if (pszOriginalString)
    {
        dwError = LwNtStatusToWin32Error(LwRtlWC16StringDuplicate(
                        ppszNewString,
                        pszOriginalString));
    }
    else
    {
        *ppszNewString = NULL;
    }

    return dwError;
}

static
inline
DWORD
LwpsConvertWc16StrDupOrNull(
    IN OPTIONAL PCWSTR InputString,
    OUT PWSTR* OutputString
    )
{
    DWORD dwError = 0;

    if (InputString)
    {
        dwError = LwAllocateWc16String(OutputString, InputString);
    }
    else
    {
        *OutputString = NULL;
    }

    return dwError;
}

static
inline
DWORD
LwpsConvertTimeWindowsToUnix(
    IN LONG64 WindowsTime,
    OUT time_t* pUnixTime
    )
{
    DWORD dwError = 0;
    LONG64 unixTime = 0;

    if (WindowsTime < 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

    unixTime = WindowsTime/10000000LL - 11644473600LL;

    // Ensure the range is within time_t.
    if (unixTime != (time_t) unixTime)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

cleanup:
    if (dwError)
    {
        unixTime = 0;
    }

    *pUnixTime = (time_t) unixTime;

    return 0;
}

static
inline
DWORD
LwpsConvertTimeUnixToWindows(
    IN time_t UnixTime,
    OUT PLONG64 pWindowsTime
    )
{
    DWORD dwError = 0;
    LONG64 windowsTime = 0;

#if SIZEOF_TIME_T > 4
    if ((LONG64) UnixTime < - 11644473600LL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }
#endif

    windowsTime = (LONG64) UnixTime + 11644473600LL;
    if (windowsTime < 0)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }
    windowsTime *= 10000000LL;
    if (windowsTime < 0)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

cleanup:
    if (dwError)
    {
        windowsTime = 0;
    }

    *pWindowsTime = windowsTime;

    return 0;
}

static
LSA_MACHINE_ACCOUNT_TYPE
LwpsConvertSchannelToAccountType(
    DWORD SchannelType
    )
{
    LSA_MACHINE_ACCOUNT_TYPE accountType = 0;

    switch (SchannelType)
    {
        case 2: // SCHANNEL_WKSTA
            accountType = LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION;
            break;
        case 4: // SCHANNEL_DOMAIN
            accountType = LSA_MACHINE_ACCOUNT_TYPE_DC;
            break;
        case 6: // SCHANNEL_BDC
            accountType = LSA_MACHINE_ACCOUNT_TYPE_BDC;
            break;
        default:
            // Default to workstation
            accountType = LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION;
            break;
    }

    return accountType;
}

static
DWORD
LwpsConvertAccountTypeToSchannel(
    LSA_MACHINE_ACCOUNT_TYPE Accountype
    )
{
    DWORD schannelType = 0;

    switch (Accountype)
    {
        case LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION:
            schannelType = 2; // SCHANNEL_WKSTA
            break;
        case LSA_MACHINE_ACCOUNT_TYPE_DC:
            schannelType = 4; // SCHANNEL_DOMAIN
            break;
        case LSA_MACHINE_ACCOUNT_TYPE_BDC:
            schannelType = 6; // SCHANNEL_BDC
            break;
        default:
            // Default to workstation
            schannelType = 2; // SCHANNEL_WKSTA
            break;
    }

    return schannelType;
}

DWORD
LwpsConvertFillMachinePasswordInfoW(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = &pPasswordInfo->Account;

    //
    // Account Portion
    //

    dwError = LwpsConvertWC16StringDuplicateOrNull(
                    &pAccountInfo->DnsDomainName,
                    pLegacyPasswordInfo->pwszDnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LsaPstorepWC16StringUpcase(pAccountInfo->DnsDomainName);

    dwError = LwpsConvertWC16StringDuplicateOrNull(
                    &pAccountInfo->NetbiosDomainName,
                    pLegacyPasswordInfo->pwszDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LsaPstorepWC16StringUpcase(pAccountInfo->NetbiosDomainName);

    dwError = LwpsConvertWC16StringDuplicateOrNull(
                    &pAccountInfo->DomainSid,
                    pLegacyPasswordInfo->pwszSID);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsConvertWC16StringDuplicateOrNull(
                    &pAccountInfo->SamAccountName,
                    pLegacyPasswordInfo->pwszMachineAccount);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LsaPstorepWC16StringUpcase(pAccountInfo->SamAccountName);

    pAccountInfo->Type = LwpsConvertSchannelToAccountType(pLegacyPasswordInfo->dwSchannelType);

    // TODO-2010/12/03-dalmeida - Hard-coded for now.
    pAccountInfo->KeyVersionNumber = 0;

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocatePrintfW(
                    &pAccountInfo->Fqdn,
                    L"%ws.%ws",
                    LocalStringOrEmptyW(pLegacyPasswordInfo->pwszHostname),
                    LocalStringOrEmptyW(pLegacyPasswordInfo->pwszHostDnsDomain)));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LsaPstorepWC16StringDowncase(pAccountInfo->Fqdn);

    dwError = LwpsConvertTimeUnixToWindows(
                    pLegacyPasswordInfo->last_change_time,
                    &pAccountInfo->LastChangeTime);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Password portion
    //

    dwError = LwpsConvertWC16StringDuplicateOrNull(
                    &pPasswordInfo->Password,
                    pLegacyPasswordInfo->pwszMachinePassword);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LsaPstorepFreePasswordInfoContentsW(pPasswordInfo);
    }

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;    
}

DWORD
LwpsConvertAllocateFromMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLWPS_PASSWORD_INFO* ppLegacyPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLWPS_PASSWORD_INFO pLegacyPasswordInfo = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = &pPasswordInfo->Account;

    dwError = LwAllocateMemory(sizeof(*pLegacyPasswordInfo), OUT_PPVOID(&pLegacyPasswordInfo));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsConvertWc16StrDupOrNull(
                    pAccountInfo->DnsDomainName,
                    &pLegacyPasswordInfo->pwszDnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwWc16sToUpper(pLegacyPasswordInfo->pwszDnsDomainName);

    dwError = LwpsConvertWc16StrDupOrNull(
                    pAccountInfo->NetbiosDomainName,
                    &pLegacyPasswordInfo->pwszDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwWc16sToUpper(pLegacyPasswordInfo->pwszDomainName);

    dwError = LwpsConvertWc16StrDupOrNull(
                    pAccountInfo->DomainSid,
                    &pLegacyPasswordInfo->pwszSID);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsConvertWc16StrDupOrNull(
                    pAccountInfo->SamAccountName,
                    &pLegacyPasswordInfo->pwszMachineAccount);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwWc16sToUpper(pLegacyPasswordInfo->pwszMachineAccount);

    pLegacyPasswordInfo->dwSchannelType = LwpsConvertAccountTypeToSchannel(pAccountInfo->Type);

    dwError = LwpsConvertWc16StrDupOrNull(
                    pAccountInfo->Fqdn,
                    &pLegacyPasswordInfo->pwszHostname);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwWc16sToLower(pLegacyPasswordInfo->pwszHostname);

    // Split FQDN
    if (pLegacyPasswordInfo->pwszHostname)
    {
        PWSTR cursor = pLegacyPasswordInfo->pwszHostname;
        while (*cursor && (*cursor != '.'))
        {
            cursor++;
        }
        if (*cursor)
        {
            *cursor = 0;
            cursor++;

            dwError = LwpsConvertWc16StrDupOrNull(
                            cursor,
                            &pLegacyPasswordInfo->pwszHostDnsDomain);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

            LwWc16sToLower(pLegacyPasswordInfo->pwszHostDnsDomain);
        }
    }

    dwError = LwpsConvertTimeWindowsToUnix(
                    pAccountInfo->LastChangeTime,
                    &pLegacyPasswordInfo->last_change_time);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsConvertWc16StrDupOrNull(
                    pPasswordInfo->Password,
                    &pLegacyPasswordInfo->pwszMachinePassword);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        if (pLegacyPasswordInfo)
        {
            LwpsLegacyFreePassword(pLegacyPasswordInfo);
            pLegacyPasswordInfo = NULL;
        }
    }

    *ppLegacyPasswordInfo = pLegacyPasswordInfo;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;    
}
