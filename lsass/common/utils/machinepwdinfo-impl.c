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

#include <lsa/lsapstore-types.h>
#include "machinepwdinfo-impl.h"
#include <lwmem.h>
#include <lwstr.h>
// These includes work around bugs in lwstr.h:
#include <string.h>
#include <wc16str.h>
#include "lsautils.h" // only for bail macro -- sigh

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

    pTargetAccountInfo->AccountFlags = pSourceAccountInfo->AccountFlags;
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

    pTargetAccountInfo->AccountFlags = pSourceAccountInfo->AccountFlags;
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

    pTargetAccountInfo->AccountFlags = pSourceAccountInfo->AccountFlags;
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

    pTargetAccountInfo->AccountFlags = pSourceAccountInfo->AccountFlags;
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
