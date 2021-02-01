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
 *        enum-state.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Enumeration State Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *
 */
#include "adprovider.h"

static
DWORD
AD_CreateEnumState(
    PAD_PROVIDER_CONTEXT pContext,
    DWORD dwInfoLevel,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PCSTR pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PAD_ENUM_STATE* ppNewEnumState
    );

static
VOID
AD_FreeEnumState(
    PAD_ENUM_STATE pEnumState
    );

DWORD
AD_CreateNSSArtefactState(
    PAD_PROVIDER_CONTEXT pContext,
    DWORD  dwInfoLevel,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PAD_ENUM_STATE* ppEnumState
    )
{
    return AD_CreateEnumState(
                    pContext,
                    dwInfoLevel,
                    FALSE,
                    0,
                    pszMapName,
                    dwFlags,
                    ppEnumState);
}

VOID
AD_FreeNSSArtefactState(
    PAD_PROVIDER_CONTEXT pContext,
    PAD_ENUM_STATE  pEnumState
    )
{
    return AD_FreeEnumState(pEnumState);
}

static
DWORD
AD_CreateEnumState(
    PAD_PROVIDER_CONTEXT pContext,
    DWORD dwInfoLevel,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PCSTR pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PAD_ENUM_STATE* ppNewEnumState
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = NULL;

    BAIL_ON_INVALID_POINTER(ppNewEnumState);

    dwError = LwAllocateMemory(sizeof(AD_ENUM_STATE), (PVOID*)&pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    pEnumState->dwInfoLevel = dwInfoLevel;
    pEnumState->dwMapFlags = dwFlags;
    pEnumState->bCheckGroupMembersOnline = bCheckGroupMembersOnline;
    pEnumState->FindFlags = FindFlags;

    AD_ReferenceProviderContext(pContext);
    pEnumState->pProviderContext = pContext;

    if (pszMapName)
    {
        dwError = LwAllocateString(pszMapName, &pEnumState->pszMapName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppNewEnumState = pEnumState;

cleanup:

    return dwError;

error:

    if (ppNewEnumState)
    {
        *ppNewEnumState = NULL;
    }

    if (pEnumState)
    {
       AD_FreeEnumState(pEnumState);
    }

    goto cleanup;
}

static
VOID
AD_FreeEnumState(
    PAD_ENUM_STATE pState
    )
{
    if (pState)
    {
        LwFreeCookieContents(&pState->Cookie);

        LW_SAFE_FREE_STRING(pState->pszMapName);

        AD_ClearProviderState(pState->pProviderContext);
        AD_DereferenceProviderContext(pState->pProviderContext);

        LwFreeMemory(pState);
    }
}
