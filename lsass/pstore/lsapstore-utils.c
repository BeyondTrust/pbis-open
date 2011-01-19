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
 *     lsapstore-utils.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Utility code
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-includes.h"
#include <wc16str.h>
#include <ctype.h>
#include <assert.h>
#include <dlfcn.h>

VOID
LsaPstoreFreeMemory(
    IN PVOID Pointer
    )
{
    LwRtlMemoryFree(Pointer);
}

VOID
LsaPstoreFreeStringArrayA(
    IN PSTR* StringArray,
    IN DWORD Count
    )
{
    if (StringArray)
    {
        DWORD i = 0;
        for (i = 0; i < Count; i++)
        {
            if (StringArray[i])
            {
                LsaPstoreFreeMemory(StringArray[i]);
            }
        }
        LsaPstoreFreeMemory(StringArray);
    }
}

VOID
LsaPstoreFreeStringArrayW(
    IN PWSTR* StringArray,
    IN DWORD Count
    )
{
    if (StringArray)
    {
        DWORD i = 0;
        for (i = 0; i < Count; i++)
        {
            if (StringArray[i])
            {
                LsaPstoreFreeMemory(StringArray[i]);
            }
        }
        LsaPstoreFreeMemory(StringArray);
    }
}

static
VOID
LsaPstorepFreeAccountInfoContentsA(
    IN OUT PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    LSA_PSTORE_FREE(&pAccountInfo->DnsDomainName);
    LSA_PSTORE_FREE(&pAccountInfo->NetbiosDomainName);
    LSA_PSTORE_FREE(&pAccountInfo->DomainSid);
    LSA_PSTORE_FREE(&pAccountInfo->SamAccountName);
    LSA_PSTORE_FREE(&pAccountInfo->Fqdn);
}

static
VOID
LsaPstorepFreeAccountInfoContentsW(
    IN OUT PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    )
{
    LSA_PSTORE_FREE(&pAccountInfo->DnsDomainName);
    LSA_PSTORE_FREE(&pAccountInfo->NetbiosDomainName);
    LSA_PSTORE_FREE(&pAccountInfo->DomainSid);
    LSA_PSTORE_FREE(&pAccountInfo->SamAccountName);
    LSA_PSTORE_FREE(&pAccountInfo->Fqdn);
}

static
VOID
LsaPstorepFreePasswordInfoContentsA(
    IN OUT PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    LsaPstorepFreeAccountInfoContentsA(&pPasswordInfo->Account);
    LSA_PSTORE_FREE_SECURE_CSTRING(&pPasswordInfo->Password);
}

VOID
LsaPstorepFreePasswordInfoContentsW(
    IN OUT PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    LsaPstorepFreeAccountInfoContentsW(&pPasswordInfo->Account);
    LSA_PSTORE_FREE_SECURE_WC16STRING(&pPasswordInfo->Password);
}

VOID
LsaPstorepFreeAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    if (pAccountInfo)
    {
        LsaPstorepFreeAccountInfoContentsA(pAccountInfo);
        LsaPstoreFreeMemory(pAccountInfo);
    }
}

VOID
LsaPstoreFreePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        LsaPstorepFreePasswordInfoContentsA(pPasswordInfo);
        LsaPstoreFreeMemory(pPasswordInfo);
    }
}

VOID
LsaPstoreFreePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        LsaPstorepFreePasswordInfoContentsW(pPasswordInfo);
        LsaPstoreFreeMemory(pPasswordInfo);
    }
}

DWORD
LsaPstorepConvertAnsiToWidePasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_PASSWORD_INFO_W passwordInfo = NULL;

    dwError = LSA_PSTORE_ALLOCATE_AUTO(&passwordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                    &passwordInfo->Account.DnsDomainName,
                    pPasswordInfo->Account.DnsDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                    &passwordInfo->Account.NetbiosDomainName,
                    pPasswordInfo->Account.NetbiosDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                    &passwordInfo->Account.DomainSid,
                    pPasswordInfo->Account.DomainSid));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                    &passwordInfo->Account.SamAccountName,
                    pPasswordInfo->Account.SamAccountName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                    &passwordInfo->Account.Fqdn,
                    pPasswordInfo->Account.Fqdn));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    passwordInfo->Account.Type = pPasswordInfo->Account.Type;
    passwordInfo->Account.KeyVersionNumber = pPasswordInfo->Account.KeyVersionNumber;
    passwordInfo->Account.LastChangeTime = pPasswordInfo->Account.LastChangeTime;

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                    &passwordInfo->Password,
                    pPasswordInfo->Password));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_W(&passwordInfo);
    }

    *ppPasswordInfo = passwordInfo;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepConvertWideToAnsiPasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_PASSWORD_INFO_A passwordInfo = NULL;

    dwError = LSA_PSTORE_ALLOCATE_AUTO(&passwordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &passwordInfo->Account.DnsDomainName,
                    pPasswordInfo->Account.DnsDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &passwordInfo->Account.NetbiosDomainName,
                    pPasswordInfo->Account.NetbiosDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &passwordInfo->Account.DomainSid,
                    pPasswordInfo->Account.DomainSid));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &passwordInfo->Account.SamAccountName,
                    pPasswordInfo->Account.SamAccountName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &passwordInfo->Account.Fqdn,
                    pPasswordInfo->Account.Fqdn));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    passwordInfo->Account.Type = pPasswordInfo->Account.Type;
    passwordInfo->Account.KeyVersionNumber = pPasswordInfo->Account.KeyVersionNumber;
    passwordInfo->Account.LastChangeTime = pPasswordInfo->Account.LastChangeTime;

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &passwordInfo->Password,
                    pPasswordInfo->Password));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_A(&passwordInfo);
    }

    *ppPasswordInfo = passwordInfo;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepConvertWideToAnsiAccountInfo(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A accountInfo = NULL;

    dwError = LSA_PSTORE_ALLOCATE_AUTO(&accountInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &accountInfo->DnsDomainName,
                    pAccountInfo->DnsDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &accountInfo->NetbiosDomainName,
                    pAccountInfo->NetbiosDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &accountInfo->DomainSid,
                    pAccountInfo->DomainSid));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &accountInfo->SamAccountName,
                    pAccountInfo->SamAccountName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &accountInfo->Fqdn,
                    pAccountInfo->Fqdn));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    accountInfo->Type = pAccountInfo->Type;
    accountInfo->KeyVersionNumber = pAccountInfo->KeyVersionNumber;
    accountInfo->LastChangeTime = pAccountInfo->LastChangeTime;

cleanup:
    if (dwError)
    {
        LSA_PSTOREP_FREE_ACCOUNT_INFO_A(&accountInfo);
    }

    *ppAccountInfo = accountInfo;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

PSTR
LsaPstorepCStringDowncase(
    IN OUT PSTR String
    )
{
    if (String)
    {
        PSTR Current;
        for (Current = String; *Current; Current++)
        {
            *Current = tolower((int)*Current);
        }
    }

    return String;
}

PSTR
LsaPstorepCStringUpcase(
    IN OUT PSTR String
    )
{
    if (String)
    {
        PSTR Current;
        for (Current = String; *Current; Current++)
        {
            *Current = toupper((int)*Current);
        }
    }

    return String;
}

PWSTR
LsaPstorepWC16StringDowncase(
    IN OUT PWSTR String
    )
{
    if (String)
    {
        wc16slower(String);
    }

    return String;
}

PWSTR
LsaPstorepWC16StringUpcase(
    IN OUT PWSTR String
    )
{
    if (String)
    {
        wc16supper(String);
    }

    return String;
}

DWORD
LsaPstorepRegGetDword(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    OUT PDWORD ValueData
    )
{
    DWORD dwError = 0;
    int EE = 0;
    DWORD size = 0;
    DWORD valueData = 0;

    size = sizeof(valueData);

    dwError = LwRegGetValueA(
                    RegistryConnection,
                    KeyHandle,
                    NULL,
                    ValueName,
                    RRF_RT_REG_DWORD,
                    NULL,
                    &valueData,
                    &size);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    assert(size == sizeof(valueData));

cleanup:
    if (dwError)
    {
        valueData = 0;
    }

    *ValueData = valueData;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepRegGetQword(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    OUT PULONG64 ValueData
    )
{
    DWORD dwError = 0;
    int EE = 0;
    DWORD size = 0;
    DWORD valueData = 0;

    // TODO: Fix to use RRF_RT_REG_QWORD

    size = sizeof(valueData);

    dwError = LwRegGetValueA(
                    RegistryConnection,
                    KeyHandle,
                    NULL,
                    ValueName,
                    RRF_RT_REG_DWORD,
                    NULL,
                    &valueData,
                    &size);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    assert(size == sizeof(valueData));

cleanup:
    if (dwError)
    {
        valueData = 0;
    }

    *ValueData = valueData;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepRegGetStringA(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    OUT PSTR* ValueData
    )
{
    DWORD dwError = 0;
    int EE = 0;
    DWORD size = 0;
    PSTR valueData = NULL;

    dwError = LwRegGetValueA(
                    RegistryConnection,
                    KeyHandle,
                    NULL,
                    ValueName,
                    RRF_RT_REG_SZ,
                    NULL,
                    NULL,
                    &size);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    assert(size > 0);

    dwError = LSA_PSTORE_ALLOCATE(&valueData, size);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegGetValueA(
                    RegistryConnection,
                    KeyHandle,
                    NULL,
                    ValueName,
                    RRF_RT_REG_SZ,
                    NULL,
                    valueData,
                    &size);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE(&valueData);
    }

    *ValueData = valueData;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}


DWORD
LsaPstorepOpenPlugin(
    IN PCSTR Path,
    IN PCSTR InitFunctionName,
    OUT PVOID* Handle,
    OUT PVOID* InitFunction
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PCSTR errorMessage = NULL;
    PVOID handle = NULL;
    PVOID initFunction = NULL;

    dlerror();

    handle = dlopen(Path, RTLD_NOW | RTLD_LOCAL);
    if (!handle)
    {
        errorMessage = dlerror();

        dwError = ERROR_DLL_INIT_FAILED;
        GOTO_CLEANUP_EE(EE);
    }

    dlerror();

    initFunction = dlsym(handle, InitFunctionName);
    if (!initFunction)
    {
        errorMessage = dlerror();

        dwError = ERROR_DLL_INIT_FAILED;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (dwError)
    {
        if (handle)
        {
            LsaPstorepClosePlugin(handle);
        }
        handle = NULL;
        initFunction = NULL;
    }

    *Handle = handle;
    *InitFunction = initFunction;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE_FMT(dwError, EE, "dlerror = '%s' loading '%s'",
            LW_RTL_LOG_SAFE_STRING(errorMessage),
            LW_RTL_LOG_SAFE_STRING(Path));
    return dwError;
}

VOID
LsaPstorepClosePlugin(
    IN PVOID Handle
    )
{
    if (Handle)
    {
        dlclose(Handle);
    }
}
