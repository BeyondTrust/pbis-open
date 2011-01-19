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
 *     lsapstore-backend-legacy.h
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Legacy Backend
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-includes.h"
#include "lsapstore-backend-legacy.h"

typedef struct _LSA_PSTORE_BACKEND_STATE {
    PLWPS_LEGACY_STATE OldStoreHandle;
} LSA_PSTORE_BACKEND_STATE, *PLSA_PSTORE_BACKEND_STATE;

static LSA_PSTORE_BACKEND_STATE LsaPstoreBackendState = { 0 };

//
// Functions
//

DWORD
LsaPstorepBackendInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_BACKEND_STATE pState = &LsaPstoreBackendState;

    dwError = LwpsLegacyOpenProvider(&pState->OldStoreHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LsaPstorepBackendCleanup();
    }

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

VOID
LsaPstorepBackendCleanup(
    VOID
    )
{
    PLSA_PSTORE_BACKEND_STATE pState = &LsaPstoreBackendState;

    if (pState->OldStoreHandle)
    {
        LwpsLegacyCloseProvider(pState->OldStoreHandle);
        pState->OldStoreHandle = NULL;
    }
}

DWORD
LsaPstorepBackendGetPasswordInfoW(
    IN OPTIONAL PCWSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainName = NULL;
    PLWPS_LEGACY_PASSWORD_INFO legacyPasswordInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W passwordInfo = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &dnsDomainName,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LwpsLegacyReadPassword(
                    LsaPstoreBackendState.OldStoreHandle,
                    dnsDomainName,
                    &legacyPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LSA_PSTORE_ALLOCATE_AUTO(&passwordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsConvertFillMachinePasswordInfoW(
                    legacyPasswordInfo,
                    passwordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_W(&passwordInfo);
    }

    if (legacyPasswordInfo)
    {
        LwpsLegacyFreePassword(legacyPasswordInfo);
    }

    LwRtlCStringFree(&dnsDomainName);

    *PasswordInfo = passwordInfo;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendSetPasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLWPS_LEGACY_PASSWORD_INFO legacyPasswordInfo = NULL;
    PWSTR defaultDnsDomainName = NULL;
    BOOLEAN isDefaultDomain = FALSE;

    dwError = LwpsConvertAllocateFromMachinePasswordInfoW(
                    PasswordInfo,
                    &legacyPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyWritePassword(
                    LsaPstoreBackendState.OldStoreHandle,
                    legacyPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstoreGetDefaultDomainW(&defaultDnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (defaultDnsDomainName)
    {
        isDefaultDomain = LwRtlWC16StringIsEqual(
                                defaultDnsDomainName,
                                PasswordInfo->Account.DnsDomainName,
                                FALSE);
    }

    if (isDefaultDomain)
    {
        dwError = LsaPstorepCallPluginSetPasswordInfo(PasswordInfo);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:
    if (legacyPasswordInfo)
    {
        LwpsLegacyFreePassword(legacyPasswordInfo);
    }

    LwRtlWC16StringFree(&defaultDnsDomainName);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendDeletePasswordInfoW(
    IN OPTIONAL PCWSTR DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainName = NULL;
    PWSTR defaultDnsDomainName = NULL;
    BOOLEAN isDefaultDomain = FALSE;
    PLSA_MACHINE_PASSWORD_INFO_W passwordInfo = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &dnsDomainName,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        dwError = LsaPstoreGetDefaultDomainW(&defaultDnsDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        if (defaultDnsDomainName)
        {
            isDefaultDomain = LwRtlWC16StringIsEqual(
                                    defaultDnsDomainName,
                                    DnsDomainName,
                                    FALSE);
        }
    }
    else
    {
        isDefaultDomain = TRUE;
    }

    if (!defaultDnsDomainName)
    {
        dwError = LsaPstoreGetDefaultDomainW(&defaultDnsDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LsaPstoreGetPasswordInfoW(DnsDomainName, &passwordInfo);
    // Ignore any error
    dwError = 0;

    dwError = LwpsLegacyDeletePassword(
                    LsaPstoreBackendState.OldStoreHandle,
                    dnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (isDefaultDomain)
    {
        dwError = LsaPstorepCallPluginDeletePasswordInfo(
                        passwordInfo ? &passwordInfo->Account : NULL);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:
    LwRtlCStringFree(&dnsDomainName);
    LwRtlWC16StringFree(&defaultDnsDomainName);
    LSA_PSTORE_FREE_PASSWORD_INFO_W(&passwordInfo);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendGetDefaultDomainW(
    OUT PWSTR* DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR dnsDomainName = NULL;
    PLWPS_LEGACY_PASSWORD_INFO legacyPasswordInfo = NULL;

    dwError = LwpsLegacyReadPassword(
                    LsaPstoreBackendState.OldStoreHandle,
                    NULL,
                    &legacyPasswordInfo);
    if (dwError == NERR_SetupNotJoined)
    {
        // TODO - Is this the desired API behavior?
        // no default domain
        dwError = 0;
        GOTO_CLEANUP_EE(EE);
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringDuplicate(
                    &dnsDomainName,
                    legacyPasswordInfo->pwszDnsDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LwRtlWC16StringFree(&dnsDomainName);
    }

    if (legacyPasswordInfo)
    {
        LwpsLegacyFreePassword(legacyPasswordInfo);
    }

    *DnsDomainName = dnsDomainName;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendSetDefaultDomainW(
    IN OPTIONAL PCWSTR DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainName = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &dnsDomainName,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LwpsLegacySetDefaultJoinedDomain(
                    LsaPstoreBackendState.OldStoreHandle,
                    dnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LwRtlCStringFree(&dnsDomainName);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendGetJoinedDomainsW(
    OUT PWSTR** DnsDomainNames,
    OUT PDWORD Count
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR* dnsDomainNames = NULL;
    DWORD count = 0;
    PSTR* internalDnsDomainNames = NULL;
    DWORD internalCount = 0;

    dwError = LwpsLegacyGetJoinedDomains(
                    LsaPstoreBackendState.OldStoreHandle,
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
        dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                        &dnsDomainNames[count],
                        internalDnsDomainNames[count]));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:
    if (dwError)
    {
        if (dnsDomainNames)
        {
            LsaPstoreFreeStringArrayW(dnsDomainNames, count);
            dnsDomainNames = NULL;
        }
        count = 0;
    }

    if (internalDnsDomainNames)
    {
        LwpsLegacyFreeStringArray(internalDnsDomainNames, internalCount);
    }

    *DnsDomainNames = dnsDomainNames;
    *Count = count;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}
