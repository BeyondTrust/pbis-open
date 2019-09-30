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
#include "lsapstore-backend.h"
#include "lsapstore-backend-legacy.h"

//
// Functions
//

DWORD
LsaPstorepBackendInitialize(
    OUT PLSA_PSTORE_BACKEND_STATE* State
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_BACKEND_STATE state = NULL;

    dwError = LSA_PSTORE_ALLOCATE_AUTO(&state);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyOpenProvider(&state->OldStoreHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LsaPstorepBackendCleanup(state);
        state = NULL;
    }

    *State = state;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

VOID
LsaPstorepBackendCleanup(
    IN PLSA_PSTORE_BACKEND_STATE State
    )
{
    if (State)
    {
        if (State->OldStoreHandle)
        {
            LwpsLegacyCloseProvider(State->OldStoreHandle);
            State->OldStoreHandle = NULL;
        }
        LSA_PSTORE_FREE(&State);
    }
}

DWORD
LsaPstorepBackendGetPasswordInfoW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN PCWSTR DnsDomainName,
    OUT OPTIONAL PLSA_MACHINE_PASSWORD_INFO_W* PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainNameA = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W passwordInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A passwordInfoA = NULL;

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &dnsDomainNameA,
                    DnsDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyReadPassword(
                    State->OldStoreHandle,
                    dnsDomainNameA,
                    &passwordInfoA);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepConvertAnsiToWidePasswordInfo(
                    passwordInfoA,
                    &passwordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_W(&passwordInfo);
    }

    LSA_PSTORE_FREE_PASSWORD_INFO_A(&passwordInfoA);
    LW_RTL_FREE(&dnsDomainNameA);

    if (PasswordInfo)
    {
        *PasswordInfo = passwordInfo;
    }
    else
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_W(&passwordInfo);
    }

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendSetPasswordInfoW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN PLSA_MACHINE_PASSWORD_INFO_W PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_PASSWORD_INFO_A passwordInfoA = NULL;

    dwError = LsaPstorepConvertWideToAnsiPasswordInfo(
                    PasswordInfo,
                    &passwordInfoA);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyWritePassword(
                    State->OldStoreHandle,
                    passwordInfoA);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LSA_PSTORE_FREE_PASSWORD_INFO_A(&passwordInfoA);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendDeletePasswordInfoW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN PCWSTR DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainNameA = NULL;

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                    &dnsDomainNameA,
                    DnsDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyDeletePassword(
                    State->OldStoreHandle,
                    dnsDomainNameA);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LW_RTL_FREE(&dnsDomainNameA);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendGetDefaultDomainW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    OUT PWSTR* DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainNameA = NULL;
    PWSTR dnsDomainName = NULL;

    dwError = LwpsLegacyGetDefaultJoinedDomain(
                    State->OldStoreHandle,
                    &dnsDomainNameA);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwNtStatusToWin32Error(LwRtlWC16StringAllocateFromCString(
                    &dnsDomainName,
                    dnsDomainNameA));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LW_RTL_FREE(&dnsDomainName);
    }

    LW_RTL_FREE(&dnsDomainNameA);

    *DnsDomainName = dnsDomainName;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendSetDefaultDomainW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN OPTIONAL PCWSTR DnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainNameA = NULL;

    if (DnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &dnsDomainNameA,
                        DnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LwpsLegacySetDefaultJoinedDomain(
                    State->OldStoreHandle,
                    dnsDomainNameA);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LW_RTL_FREE(&dnsDomainNameA);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendGetJoinedDomainsW(
    IN PLSA_PSTORE_BACKEND_STATE State,
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
                    State->OldStoreHandle,
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

DWORD
LsaPstoreBackendSetDomainWTrustEnumerationWaitTime(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN OPTIONAL PCWSTR pwszDnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszdnsDomainNameA = NULL;

    if (pwszDnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &pszdnsDomainNameA,
                        pwszDnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LwpsLegacySetJoinedDomainTrustEnumerationWaitTime(
                    State->OldStoreHandle,
                    pszdnsDomainNameA);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LW_RTL_FREE(&pszdnsDomainNameA);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepBackendGetDomainTrustEnumerationWaitTime(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN OPTIONAL PCWSTR pwszDnsDomainName,
    OUT PDWORD* ppdwTrustEnumerationWaitSeconds,
    OUT PDWORD* ppdwTrustEnumerationWaitEnabled
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PDWORD pdwTrustEnumerationWaitSecondsValue = NULL;
    PDWORD pdwTrustEnumerationWaitEnabledValue = NULL;
    PSTR pszdnsDomainNameA = NULL;
 
    if (pwszDnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &pszdnsDomainNameA,
                        pwszDnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        dwError = LwpsLegacyGetJoinedDomainTrustEnumerationWaitTime(
                        State->OldStoreHandle,
                        pszdnsDomainNameA,
                        &pdwTrustEnumerationWaitSecondsValue, &pdwTrustEnumerationWaitEnabledValue);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    *ppdwTrustEnumerationWaitSeconds = (PDWORD) pdwTrustEnumerationWaitSecondsValue;
    *ppdwTrustEnumerationWaitEnabled = (PDWORD) pdwTrustEnumerationWaitEnabledValue;

cleanup:
    LW_RTL_FREE(&pszdnsDomainNameA);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstoreBackendDeleteTrustEnumerationWaitInfo(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN PCWSTR pwszDnsDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszdnsDomainNameA = NULL;

    if(pwszDnsDomainName)
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &pszdnsDomainNameA,
                        pwszDnsDomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    
        dwError = LwpsLegacyDeleteTrustEnumerationWaitInfo(
                        State->OldStoreHandle,
                        pszdnsDomainNameA);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:

    LW_RTL_FREE(&pszdnsDomainNameA);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}
