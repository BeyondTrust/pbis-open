/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lpenumstate.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
