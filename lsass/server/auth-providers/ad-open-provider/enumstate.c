/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        enum-state.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
