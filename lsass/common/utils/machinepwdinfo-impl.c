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
 *        machinepwdinfo-impl.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Machine Account/Password Info Memory Managent Implementation Helper
 *
 *        These functions should not be exposed across any API boundaries.
 *        Rather, they are intended to help implement memory management
 *        related to machine account/password within a single
 *        component/module.
 *
 * Authors: Danilo Almeida <dalmeida@likewise.com>
 *
 */

#include <lwstr.h>
#include <lwmem.h>
#include <lsa/lsapstore-types.h>
#include "machinepwdinfo-impl.h"
#include "lsautils.h" // only for bail macro -- sigh

// For legacy conversion support
#include <lwps/lwps.h>
#include <lwpwdinfo.h>

// These includes work around bugs in lwstr.h:
#include <string.h>
#include <wc16str.h>


static
inline
DWORD
LocalWc16StrDupOrNull(
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
LocalWc16sToMbsOrNull(
    IN OPTIONAL PCWSTR InputString,
    OUT PSTR* OutputString
    )
{
    DWORD dwError = 0;

    if (InputString)
    {
        dwError = LwWc16sToMbs(InputString, OutputString);
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
LocalMbsToWc16sOrNull(
    IN OPTIONAL PCSTR InputString,
    OUT PWSTR* OutputString
    )
{
    DWORD dwError = 0;

    if (InputString)
    {
        dwError = LwMbsToWc16s(InputString, OutputString);
    }
    else
    {
        *OutputString = NULL;
    }

    return dwError;
}

static
inline
PCSTR
LocalStringOrEmpty(
    IN OPTIONAL PCSTR String
    )
{
    return String ? String : "";
}

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
LocalConvertTimeWindowsToUnix(
    IN LONG64 WindowsTime,
    OUT time_t* pUnixTime
    )
{
    DWORD dwError = 0;
    LONG64 unixTime = 0;

    if (WindowsTime < 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    unixTime = WindowsTime/10000000LL - 11644473600LL;

    // Ensure the range is within time_t.
    if (unixTime != (time_t) unixTime)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:
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
LocalConvertTimeUnixToWindows(
    IN time_t UnixTime,
    OUT PLONG64 pWindowsTime
    )
{
    DWORD dwError = 0;
    LONG64 windowsTime = 0;

    if ((LONG64) UnixTime < - 11644473600LL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    windowsTime = (LONG64) UnixTime + 11644473600LL;
    if (windowsTime < 0)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        BAIL_ON_LSA_ERROR(dwError);
    }
    windowsTime *= 10000000LL;
    if (windowsTime < 0)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:
    if (dwError)
    {
        windowsTime = 0;
    }

    *pWindowsTime = windowsTime;

    return 0;
}


static
VOID
LsaImplFreeMachineAccountInfoContentsA(
    IN OUT PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    LW_SAFE_FREE_MEMORY(pAccountInfo->DnsDomainName);
    LW_SAFE_FREE_MEMORY(pAccountInfo->NetbiosDomainName);
    LW_SAFE_FREE_MEMORY(pAccountInfo->DomainSid);
    LW_SAFE_FREE_MEMORY(pAccountInfo->SamAccountName);
    LW_SAFE_FREE_MEMORY(pAccountInfo->Fqdn);
}

static
VOID
LsaImplFreeMachineAccountInfoContentsW(
    IN OUT PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    )
{
    LW_SAFE_FREE_MEMORY(pAccountInfo->DnsDomainName);
    LW_SAFE_FREE_MEMORY(pAccountInfo->NetbiosDomainName);
    LW_SAFE_FREE_MEMORY(pAccountInfo->DomainSid);
    LW_SAFE_FREE_MEMORY(pAccountInfo->SamAccountName);
    LW_SAFE_FREE_MEMORY(pAccountInfo->Fqdn);
}

VOID
LsaImplFreeMachinePasswordInfoContentsA(
    IN OUT PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    LsaImplFreeMachineAccountInfoContentsA(&pPasswordInfo->Account);
    LW_SECURE_FREE_STRING(pPasswordInfo->Password);
}

VOID
LsaImplFreeMachinePasswordInfoContentsW(
    IN OUT PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    LsaImplFreeMachineAccountInfoContentsW(&pPasswordInfo->Account);
    LW_SECURE_FREE_WSTRING(pPasswordInfo->Password);
}

static
DWORD
LsaImplFillMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pSourceAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A pTargetAccountInfo
    )
{
    DWORD dwError = 0;

    dwError = LwStrDupOrNull(
                    pSourceAccountInfo->DnsDomainName,
                    &pTargetAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pSourceAccountInfo->NetbiosDomainName,
                    &pTargetAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pSourceAccountInfo->DomainSid,
                    &pTargetAccountInfo->DomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pSourceAccountInfo->SamAccountName,
                    &pTargetAccountInfo->SamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    pTargetAccountInfo->Type = pSourceAccountInfo->Type;
    pTargetAccountInfo->KeyVersionNumber = pSourceAccountInfo->KeyVersionNumber;

    dwError = LwStrDupOrNull(
                    pSourceAccountInfo->Fqdn,
                    &pTargetAccountInfo->Fqdn);
    BAIL_ON_LSA_ERROR(dwError);

    pTargetAccountInfo->LastChangeTime = pSourceAccountInfo->LastChangeTime;

error:
    if (dwError)
    {
        LsaImplFreeMachineAccountInfoContentsA(pTargetAccountInfo);
    }

    return dwError;    
}

static
DWORD
LsaImplFillMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pSourceAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W pTargetAccountInfo
    )
{
    DWORD dwError = 0;

    dwError = LocalWc16StrDupOrNull(
                    pSourceAccountInfo->DnsDomainName,
                    &pTargetAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pSourceAccountInfo->NetbiosDomainName,
                    &pTargetAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pSourceAccountInfo->DomainSid,
                    &pTargetAccountInfo->DomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pSourceAccountInfo->SamAccountName,
                    &pTargetAccountInfo->SamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    pTargetAccountInfo->Type = pSourceAccountInfo->Type;
    pTargetAccountInfo->KeyVersionNumber = pSourceAccountInfo->KeyVersionNumber;

    dwError = LocalWc16StrDupOrNull(
                    pSourceAccountInfo->Fqdn,
                    &pTargetAccountInfo->Fqdn);
    BAIL_ON_LSA_ERROR(dwError);

    pTargetAccountInfo->LastChangeTime = pSourceAccountInfo->LastChangeTime;

error:
    if (dwError)
    {
        LsaImplFreeMachineAccountInfoContentsW(pTargetAccountInfo);
    }

    return dwError;    
}

DWORD
LsaImplFillMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pSourcePasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A pTargetPasswordInfo
    )
{
    DWORD dwError = 0;

    dwError = LsaImplFillMachineAccountInfoA(
                    &pSourcePasswordInfo->Account,
                    &pTargetPasswordInfo->Account);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pSourcePasswordInfo->Password,
                    &pTargetPasswordInfo->Password);
    BAIL_ON_LSA_ERROR(dwError);                 

error:
    if (dwError)
    {
        LsaImplFreeMachinePasswordInfoContentsA(pTargetPasswordInfo);
    }

    return dwError;
}

DWORD
LsaImplFillMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pSourcePasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W pTargetPasswordInfo
    )
{
    DWORD dwError = 0;

    dwError = LsaImplFillMachineAccountInfoW(
                    &pSourcePasswordInfo->Account,
                    &pTargetPasswordInfo->Account);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pSourcePasswordInfo->Password,
                    &pTargetPasswordInfo->Password);
    BAIL_ON_LSA_ERROR(dwError);                 

error:
    if (dwError)
    {
        LsaImplFreeMachinePasswordInfoContentsW(pTargetPasswordInfo);
    }

    return dwError;
}

VOID
LsaImplFreeMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    if (pAccountInfo)
    {
        LsaImplFreeMachineAccountInfoContentsA(pAccountInfo);
        LwFreeMemory(pAccountInfo);
    }
}

VOID
LsaImplFreeMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    )
{
    if (pAccountInfo)
    {
        LsaImplFreeMachineAccountInfoContentsW(pAccountInfo);
        LwFreeMemory(pAccountInfo);
    }
}

VOID
LsaImplFreeMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        LsaImplFreeMachinePasswordInfoContentsA(pPasswordInfo);
        LwFreeMemory(pPasswordInfo);
    }
}

VOID
LsaImplFreeMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        LsaImplFreeMachinePasswordInfoContentsW(pPasswordInfo);
        LwFreeMemory(pPasswordInfo);
    }
}

DWORD
LsaImplDuplicateMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppNewAccountInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A pNewAccountInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pNewAccountInfo), OUT_PPVOID(&pNewAccountInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplFillMachineAccountInfoA(pAccountInfo, pNewAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pNewAccountInfo)
        {
            LsaImplFreeMachineAccountInfoA(pNewAccountInfo);
            pNewAccountInfo = NULL;
        }
    }

    *ppNewAccountInfo = pNewAccountInfo;

    return dwError;    
}

DWORD
LsaImplDuplicateMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppNewAccountInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_ACCOUNT_INFO_W pNewAccountInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pNewAccountInfo), OUT_PPVOID(&pNewAccountInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplFillMachineAccountInfoW(pAccountInfo, pNewAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pNewAccountInfo)
        {
            LsaImplFreeMachineAccountInfoW(pNewAccountInfo);
            pNewAccountInfo = NULL;
        }
    }

    *ppNewAccountInfo = pNewAccountInfo;

    return dwError;    
}

DWORD
LsaImplDuplicateMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppNewPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pNewPasswordInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pNewPasswordInfo), OUT_PPVOID(&pNewPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplFillMachinePasswordInfoA(pPasswordInfo, pNewPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pNewPasswordInfo)
        {
            LsaImplFreeMachinePasswordInfoA(pNewPasswordInfo);
            pNewPasswordInfo = NULL;
        }
    }

    *ppNewPasswordInfo = pNewPasswordInfo;

    return dwError;    
}

DWORD
LsaImplDuplicateMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppNewPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_W pNewPasswordInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pNewPasswordInfo), OUT_PPVOID(&pNewPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplFillMachinePasswordInfoW(pPasswordInfo, pNewPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pNewPasswordInfo)
        {
            LsaImplFreeMachinePasswordInfoW(pNewPasswordInfo);
            pNewPasswordInfo = NULL;
        }
    }

    *ppNewPasswordInfo = pNewPasswordInfo;

    return dwError;    
}

static
DWORD
LsaImplFillMachineAccountInfoMultiByteToWide(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pSourceAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W pTargetAccountInfo
    )
{
    DWORD dwError = 0;

    dwError = LocalMbsToWc16sOrNull(
                    pSourceAccountInfo->DnsDomainName,
                    &pTargetAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMbsToWc16sOrNull(
                    pSourceAccountInfo->NetbiosDomainName,
                    &pTargetAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMbsToWc16sOrNull(
                    pSourceAccountInfo->DomainSid,
                    &pTargetAccountInfo->DomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMbsToWc16sOrNull(
                    pSourceAccountInfo->SamAccountName,
                    &pTargetAccountInfo->SamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    pTargetAccountInfo->Type = pSourceAccountInfo->Type;
    pTargetAccountInfo->KeyVersionNumber = pSourceAccountInfo->KeyVersionNumber;

    dwError = LocalMbsToWc16sOrNull(
                    pSourceAccountInfo->Fqdn,
                    &pTargetAccountInfo->Fqdn);
    BAIL_ON_LSA_ERROR(dwError);

    pTargetAccountInfo->LastChangeTime = pSourceAccountInfo->LastChangeTime;

error:
    if (dwError)
    {
        LsaImplFreeMachineAccountInfoContentsW(pTargetAccountInfo);
    }

    return dwError;
}

static
DWORD
LsaImplFillMachineAccountInfoWideToMultiByte(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pSourceAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A pTargetAccountInfo
    )
{
    DWORD dwError = 0;

    dwError = LocalWc16sToMbsOrNull(
                    pSourceAccountInfo->DnsDomainName,
                    &pTargetAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16sToMbsOrNull(
                    pSourceAccountInfo->NetbiosDomainName,
                    &pTargetAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16sToMbsOrNull(
                    pSourceAccountInfo->DomainSid,
                    &pTargetAccountInfo->DomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16sToMbsOrNull(
                    pSourceAccountInfo->SamAccountName,
                    &pTargetAccountInfo->SamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    pTargetAccountInfo->Type = pSourceAccountInfo->Type;
    pTargetAccountInfo->KeyVersionNumber = pSourceAccountInfo->KeyVersionNumber;

    dwError = LocalWc16sToMbsOrNull(
                    pSourceAccountInfo->Fqdn,
                    &pTargetAccountInfo->Fqdn);
    BAIL_ON_LSA_ERROR(dwError);

    pTargetAccountInfo->LastChangeTime = pSourceAccountInfo->LastChangeTime;

error:
    if (dwError)
    {
        LsaImplFreeMachineAccountInfoContentsA(pTargetAccountInfo);
    }

    return dwError;}

static
DWORD
LsaImplFillMachinePasswordInfoMultiByteToWide(
    IN PLSA_MACHINE_PASSWORD_INFO_A pSourcePasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W pTargetPasswordInfo
    )
{
    DWORD dwError = 0;

    dwError = LsaImplFillMachineAccountInfoMultiByteToWide(
                    &pSourcePasswordInfo->Account,
                    &pTargetPasswordInfo->Account);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMbsToWc16sOrNull(
                    pSourcePasswordInfo->Password,
                    &pTargetPasswordInfo->Password);
    BAIL_ON_LSA_ERROR(dwError);                 

error:
    if (dwError)
    {
        LsaImplFreeMachinePasswordInfoContentsW(pTargetPasswordInfo);
    }

    return dwError;
}

static
DWORD
LsaImplFillMachinePasswordInfoWideToMultiByte(
    IN PLSA_MACHINE_PASSWORD_INFO_W pSourcePasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A pTargetPasswordInfo
    )
{
    DWORD dwError = 0;

    dwError = LsaImplFillMachineAccountInfoWideToMultiByte(
                    &pSourcePasswordInfo->Account,
                    &pTargetPasswordInfo->Account);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16sToMbsOrNull(
                    pSourcePasswordInfo->Password,
                    &pTargetPasswordInfo->Password);
    BAIL_ON_LSA_ERROR(dwError);                 

error:
    if (dwError)
    {
        LsaImplFreeMachinePasswordInfoContentsA(pTargetPasswordInfo);
    }

    return dwError;
}

DWORD
LsaImplConvertMachinePasswordInfoMultiByteToWide(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppNewPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_W pNewPasswordInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pNewPasswordInfo), OUT_PPVOID(&pNewPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplFillMachinePasswordInfoMultiByteToWide(pPasswordInfo, pNewPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pNewPasswordInfo)
        {
            LsaImplFreeMachinePasswordInfoW(pNewPasswordInfo);
            pNewPasswordInfo = NULL;
        }
    }

    *ppNewPasswordInfo = pNewPasswordInfo;

    return dwError;
}

DWORD
LsaImplConvertMachinePasswordInfoWideToMultiByte(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppNewPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pNewPasswordInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pNewPasswordInfo), OUT_PPVOID(&pNewPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplFillMachinePasswordInfoWideToMultiByte(pPasswordInfo, pNewPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pNewPasswordInfo)
        {
            LsaImplFreeMachinePasswordInfoA(pNewPasswordInfo);
            pNewPasswordInfo = NULL;
        }
    }

    *ppNewPasswordInfo = pNewPasswordInfo;

    return dwError;
}

static
LSA_MACHINE_ACCOUNT_TYPE
LsaImplLegacySchannelToAccountType(
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
LsaImplLegacyAccountTypeToSchannel(
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

static
DWORD
LsaImplLegacyFillMachineAccountInfoA(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
///<
/// Fill LSA pstore machine account info from legacy pstore password info.
///
/// This also handles any conversions or "cons"-ing up of values that
/// are not stored in the old legacy format.
///
/// @param[in] pLegacyPasswordInfo - Legacy password information.
/// @param[out] pAccountInfo - Returns machine account information.
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on failure
///
/// @note This function will go away once the new pstore changes
///       are complete.
///
{
    DWORD dwError = 0;
    PSTR hostname = NULL;
    PSTR dnsSuffix = NULL;

    dwError = LocalWc16sToMbsOrNull(
                    pLegacyPasswordInfo->pwszDnsDomainName,
                    &pAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pAccountInfo->DnsDomainName);

    dwError = LocalWc16sToMbsOrNull(
                    pLegacyPasswordInfo->pwszDomainName,
                    &pAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pAccountInfo->NetbiosDomainName);

    dwError = LocalWc16sToMbsOrNull(
                    pLegacyPasswordInfo->pwszSID,
                    &pAccountInfo->DomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16sToMbsOrNull(
                    pLegacyPasswordInfo->pwszMachineAccount,
                    &pAccountInfo->SamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pAccountInfo->SamAccountName);

    pAccountInfo->Type = LsaImplLegacySchannelToAccountType(pLegacyPasswordInfo->dwSchannelType);

    // TODO-2010/12/03-dalmeida - Hard-coded for now.
    pAccountInfo->KeyVersionNumber = 0;

    dwError = LocalWc16sToMbsOrNull(
                    pLegacyPasswordInfo->pwszHostname,
                    &hostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16sToMbsOrNull(
                    pLegacyPasswordInfo->pwszHostDnsDomain,
                    &dnsSuffix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pAccountInfo->Fqdn,
                    "%s.%s",
                    LocalStringOrEmpty(hostname),
                    LocalStringOrEmpty(dnsSuffix));
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToLower(pAccountInfo->Fqdn);

    dwError = LocalConvertTimeUnixToWindows(
                    pLegacyPasswordInfo->last_change_time,
                    &pAccountInfo->LastChangeTime);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        LsaImplFreeMachineAccountInfoContentsA(pAccountInfo);
    }

    LW_SAFE_FREE_MEMORY(hostname);
    LW_SAFE_FREE_MEMORY(dnsSuffix);

    return dwError;    
}

static
DWORD
LsaImplLegacyFillMachineAccountInfoW(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    )
///<
/// Fill LSA pstore machine account info from legacy pstore password info.
///
/// This also handles any conversions or "cons"-ing up of values that
/// are not stored in the old legacy format.
///
/// @param[in] pLegacyPasswordInfo - Legacy password information.
/// @param[out] pAccountInfo - Returns machine account information.
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on failure
///
/// @note This function will go away once the new pstore changes
///       are complete.
///
{
    DWORD dwError = 0;

    dwError = LocalWc16StrDupOrNull(
                    pLegacyPasswordInfo->pwszDnsDomainName,
                    &pAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pAccountInfo->DnsDomainName);

    dwError = LocalWc16StrDupOrNull(
                    pLegacyPasswordInfo->pwszDomainName,
                    &pAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pAccountInfo->NetbiosDomainName);

    dwError = LocalWc16StrDupOrNull(
                    pLegacyPasswordInfo->pwszSID,
                    &pAccountInfo->DomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pLegacyPasswordInfo->pwszMachineAccount,
                    &pAccountInfo->SamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pAccountInfo->SamAccountName);

    pAccountInfo->Type = LsaImplLegacySchannelToAccountType(pLegacyPasswordInfo->dwSchannelType);

    // TODO-2010/12/03-dalmeida - Hard-coded for now.
    pAccountInfo->KeyVersionNumber = 0;

    dwError = LwAllocateWc16sPrintfW(
                    &pAccountInfo->Fqdn,
                    L"%ws.%ws",
                    LocalStringOrEmptyW(pLegacyPasswordInfo->pwszHostname),
                    LocalStringOrEmptyW(pLegacyPasswordInfo->pwszHostDnsDomain));
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToLower(pAccountInfo->Fqdn);

    dwError = LocalConvertTimeUnixToWindows(
                    pLegacyPasswordInfo->last_change_time,
                    &pAccountInfo->LastChangeTime);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        LsaImplFreeMachineAccountInfoContentsW(pAccountInfo);
    }

    return dwError;    
}

DWORD
LsaImplLegacyFillMachinePasswordInfoA(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    DWORD dwError = 0;

    dwError = LsaImplLegacyFillMachineAccountInfoA(
                    pLegacyPasswordInfo,
                    &pPasswordInfo->Account);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16sToMbsOrNull(
                    pLegacyPasswordInfo->pwszMachinePassword,
                    &pPasswordInfo->Password);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        LsaImplFreeMachinePasswordInfoContentsA(pPasswordInfo);
    }

    return dwError;
}

DWORD
LsaImplLegacyFillMachinePasswordInfoW(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    DWORD dwError = 0;

    dwError = LsaImplLegacyFillMachineAccountInfoW(
                    pLegacyPasswordInfo,
                    &pPasswordInfo->Account);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pLegacyPasswordInfo->pwszMachinePassword,
                    &pPasswordInfo->Password);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        LsaImplFreeMachinePasswordInfoContentsW(pPasswordInfo);
    }

    return dwError;
}

DWORD
LsaImpLegacyDuplicatePasswordInfoA(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pPasswordInfo), OUT_PPVOID(&pPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplLegacyFillMachinePasswordInfoA(
                    pLegacyPasswordInfo,
                    pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pPasswordInfo)
        {
            LsaImplFreeMachinePasswordInfoA(pPasswordInfo);
            pPasswordInfo = NULL;
        }
    }

    *ppPasswordInfo = pPasswordInfo;

    return dwError;    
}

DWORD
LsaImpLegacyDuplicatePasswordInfoW(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pPasswordInfo), OUT_PPVOID(&pPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplLegacyFillMachinePasswordInfoW(
                    pLegacyPasswordInfo,
                    pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pPasswordInfo)
        {
            LsaImplFreeMachinePasswordInfoW(pPasswordInfo);
            pPasswordInfo = NULL;
        }
    }

    *ppPasswordInfo = pPasswordInfo;

    return dwError;    
}

DWORD
LsaImplLegacyConvertFromMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLWPS_PASSWORD_INFO* ppLegacyPasswordInfo
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pLegacyPasswordInfo = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = &pPasswordInfo->Account;

    dwError = LwAllocateMemory(sizeof(*pLegacyPasswordInfo), OUT_PPVOID(&pLegacyPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pAccountInfo->DnsDomainName,
                    &pLegacyPasswordInfo->pwszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pLegacyPasswordInfo->pwszDnsDomainName);

    dwError = LocalWc16StrDupOrNull(
                    pAccountInfo->NetbiosDomainName,
                    &pLegacyPasswordInfo->pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pLegacyPasswordInfo->pwszDomainName);

    dwError = LocalWc16StrDupOrNull(
                    pAccountInfo->DomainSid,
                    &pLegacyPasswordInfo->pwszSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pAccountInfo->SamAccountName,
                    &pLegacyPasswordInfo->pwszMachineAccount);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pLegacyPasswordInfo->pwszMachineAccount);

    pLegacyPasswordInfo->dwSchannelType = LsaImplLegacyAccountTypeToSchannel(pAccountInfo->Type);

    dwError = LocalWc16StrDupOrNull(
                    pAccountInfo->Fqdn,
                    &pLegacyPasswordInfo->pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);

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

            dwError = LocalWc16StrDupOrNull(
                            cursor,
                            &pLegacyPasswordInfo->pwszHostDnsDomain);
            BAIL_ON_LSA_ERROR(dwError);

            LwWc16sToLower(pLegacyPasswordInfo->pwszHostDnsDomain);
        }
    }

    dwError = LocalConvertTimeWindowsToUnix(
                    pAccountInfo->LastChangeTime,
                    &pLegacyPasswordInfo->last_change_time);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalWc16StrDupOrNull(
                    pPasswordInfo->Password,
                    &pLegacyPasswordInfo->pwszMachinePassword);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pLegacyPasswordInfo)
        {
            LsaImpLegacyFreePasswordInfo(pLegacyPasswordInfo);
            pLegacyPasswordInfo = NULL;
        }
    }

    *ppLegacyPasswordInfo = pLegacyPasswordInfo;

    return dwError;    
}

VOID
LsaImpLegacyFreePasswordInfo(
    IN PLWPS_PASSWORD_INFO pLegacyPasswordInfo
    )
{
    if (pLegacyPasswordInfo)
    {
        LW_SAFE_FREE_MEMORY(pLegacyPasswordInfo->pwszDomainName);
        LW_SAFE_FREE_MEMORY(pLegacyPasswordInfo->pwszDnsDomainName);
        LW_SAFE_FREE_MEMORY(pLegacyPasswordInfo->pwszSID);
        LW_SAFE_FREE_MEMORY(pLegacyPasswordInfo->pwszHostname);
        LW_SAFE_FREE_MEMORY(pLegacyPasswordInfo->pwszHostDnsDomain);
        LW_SAFE_FREE_MEMORY(pLegacyPasswordInfo->pwszMachineAccount);
        LW_SECURE_FREE_WSTRING(pLegacyPasswordInfo->pwszMachinePassword);
        LwFreeMemory(pLegacyPasswordInfo);
    }
}
