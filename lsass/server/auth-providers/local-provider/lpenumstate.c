/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpenumstate.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Enumeration State Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LocalCreateUserState(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PLOCAL_PROVIDER_ENUM_STATE* ppEnumState
    )
{
    return LocalCreateEnumState(dwInfoLevel, ppEnumState);
}

VOID
LocalFreeUserState(
    HANDLE hProvider,
    PLOCAL_PROVIDER_ENUM_STATE  pEnumState
    )
{
    return LocalFreeEnumState(pEnumState);
}

DWORD
LocalCreateGroupState(
    HANDLE hProvider,
    DWORD  dwInfoLevel,
    PLOCAL_PROVIDER_ENUM_STATE* ppEnumState
    )
{
    return LocalCreateEnumState(
                    dwInfoLevel,
                    ppEnumState);
}

DWORD
LocalCreateEnumState(
    DWORD dwInfoLevel,
    PLOCAL_PROVIDER_ENUM_STATE* ppNewEnumState
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LOCAL_PROVIDER_ENUM_STATE),
                    (PVOID*)&pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_init(&pEnumState->mutex, NULL);
    pEnumState->pMutex = &pEnumState->mutex;

    pEnumState->dwInfoLevel = dwInfoLevel;

    *ppNewEnumState = pEnumState;

cleanup:

    return dwError;

error:

    *ppNewEnumState = NULL;

    if (pEnumState)
    {
       LocalFreeEnumState(pEnumState);
    }

    goto cleanup;
}

VOID
LocalFreeGroupState(
    HANDLE hProvider,
    PLOCAL_PROVIDER_ENUM_STATE  pEnumState
    )
{
    return LocalFreeEnumState(pEnumState);
}

VOID
LocalFreeEnumState(
    HANDLE hResume
    )
{
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = (PLOCAL_PROVIDER_ENUM_STATE)hResume;

    if (pEnumState->pMutex)
    {
        pthread_mutex_destroy(&pEnumState->mutex);
    }

    if (pEnumState->pEntries)
    {
        DirectoryFreeEntries(pEnumState->pEntries, pEnumState->dwNumEntries);
    }

    LwFreeMemory(pEnumState);
}
