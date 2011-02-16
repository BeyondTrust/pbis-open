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
 *        state.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Server State Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvGetTargetElements(
    IN PCSTR pszTargetProvider,
    OUT PSTR* ppszTargetProviderName,
    OUT PSTR* ppszTargetInstance
    )
{
    DWORD dwError = 0;
    int idx = 0;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;
    PSTR pszIndex = NULL;

    if ((pszIndex = strchr(pszTargetProvider, ':')) != NULL)
    {
        idx = pszIndex-pszTargetProvider;

        if (idx)
        {
            dwError = LwStrndup(
                          pszTargetProvider,
                          idx,
                          &pszTargetProviderName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if ((++pszIndex)[0])
        {
            dwError = LwAllocateString(
                          pszIndex,
                          &pszTargetInstance);
            BAIL_ON_LSA_ERROR(dwError);
        }
    } 
    else if (pszTargetProvider)
    {
        dwError = LwAllocateString(
                      pszTargetProvider,
                      &pszTargetProviderName);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    *ppszTargetProviderName = pszTargetProviderName;
    *ppszTargetInstance = pszTargetInstance;

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    goto cleanup;
}

DWORD
LsaSrvFindProviderByName(
    IN PCSTR pszProvider,
    OUT PLSA_AUTH_PROVIDER* ppProvider
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        if (!strcmp(pProvider->pszId, pszProvider))
        {
            break;
        }
    }

    if (!pProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppProvider = pProvider;

    return dwError;

error:
    pProvider = NULL;

    goto cleanup;

}

void
LsaSrvCloseServer(
    HANDLE hServer
    )
{
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    LwFreeMemory(pServerState);
}

VOID
LsaSrvCloseProvider(
    PLSA_AUTH_PROVIDER pProvider,
    HANDLE hProvider
    )
{
    if (pProvider) {
        pProvider->pFnTable->pfnCloseHandle(hProvider);
    }
}

DWORD
LsaSrvOpenProvider(
    HANDLE  hServer,
    PLSA_AUTH_PROVIDER pProvider,
    PCSTR pszInstance,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    HANDLE hProvider = (HANDLE)NULL;

    dwError = pProvider->pFnTable->pfnOpenHandle(hServer, pszInstance, &hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    *phProvider = hProvider;

cleanup:

    return dwError;

error:

    *phProvider = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LsaSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    pid_t peerPID,
    PHANDLE phServer
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pServerState),
                    (PVOID*)&pServerState);
    BAIL_ON_LSA_ERROR(dwError);

    pServerState->peerUID = peerUID;
    pServerState->peerGID = peerGID;
    pServerState->peerPID = peerPID;

    *phServer = (HANDLE)pServerState;

cleanup:

    return dwError;

error:

    *phServer = (HANDLE)NULL;

    if (pServerState) {
        LsaSrvCloseServer((HANDLE)pServerState);
    }

    goto cleanup;
}

VOID
LsaSrvGetClientId(
    HANDLE hServer,
    uid_t* pUid,
    gid_t* pGid,
    pid_t* pPid
    )
{
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    if (pUid)
    {
        *pUid = pServerState->peerUID;
    }
    
    if (pGid)
    {
        *pGid = pServerState->peerGID;
    }

    if (pPid)
    {
        *pPid = pServerState->peerPID;
    }
}

DWORD
LsaSrvCreateNSSArtefactEnumState(
    HANDLE  hServer,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD   dwNSSArtefactInfoLevel,
    DWORD   dwMaxNumArtefacts,
    PLSA_SRV_ENUM_STATE* ppEnumState
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderStateList = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    dwError = LwAllocateMemory(
                       sizeof(LSA_SRV_ENUM_STATE),
                       (PVOID*)&pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    pEnumState->dwInfoLevel = dwNSSArtefactInfoLevel;
    pEnumState->dwNumMaxRecords = dwMaxNumArtefacts;
    pEnumState->dwMapFlags = dwFlags;

    dwError = LwAllocateString(
                    pszMapName,
                    &pEnumState->pszMapName);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(pEnumState->bInLock);

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LwAllocateMemory(
                            sizeof(LSA_SRV_PROVIDER_STATE),
                            (PVOID*)&pProviderState);
        BAIL_ON_LSA_ERROR(dwError);

        pProviderState->pProvider = pProvider;

        dwError = LsaSrvOpenProvider(
                            hServer,
                            pProvider,
                            NULL,
                            &pProviderState->hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnBeginEnumNSSArtefacts(
                                            pProviderState->hProvider,
                                            pEnumState->dwInfoLevel,
                                            pEnumState->pszMapName,
                                            pEnumState->dwMapFlags,
                                            &pProviderState->hResume);
        if (!dwError) {

           pProviderState->pNext = pProviderStateList;
           pProviderStateList = pProviderState;
           pProviderState = NULL;

        } else if (dwError == LW_ERROR_NOT_HANDLED) {

           LsaSrvFreeProviderStateList(pProviderState);
           pProviderState = NULL;

           dwError = 0;

           continue;

        } else {

           BAIL_ON_LSA_ERROR(dwError);

        }
    }

    pEnumState->pProviderStateList =
               LsaSrvReverseProviderStateList(pProviderStateList);
    pProviderStateList = NULL;
    pEnumState->pCurProviderState = pEnumState->pProviderStateList;

    *ppEnumState = pEnumState;

cleanup:

    return dwError;

error:

    *ppEnumState = NULL;

    if (pProviderState) {
        LsaSrvFreeProviderStateList(pProviderState);
    }
    if (pProviderStateList)
    {
        pEnumState->pProviderStateList = pProviderStateList;
        pProviderStateList = NULL;
        LsaSrvEndEnumNSSArtefacts(hServer, pEnumState);
        pEnumState = NULL;
    }
    else
    {
        if (pEnumState)
        {
            LsaSrvFreeEnumState(pEnumState);
        }
    }

    goto cleanup;
}

VOID
LsaSrvFreeProviderStateList(
    PLSA_SRV_PROVIDER_STATE pStateList
    )
{
    while (pStateList) {
        PLSA_SRV_PROVIDER_STATE pState = pStateList;
        pStateList = pStateList->pNext;

        if (pState->pProvider && (pState->hProvider != (HANDLE)NULL)) {
            pState->pProvider->pFnTable->pfnCloseHandle(pState->hProvider);
        }
        LwFreeMemory(pState);
    }
}

PLSA_SRV_PROVIDER_STATE
LsaSrvReverseProviderStateList(
    PLSA_SRV_PROVIDER_STATE pStateList
    )
{
    PLSA_SRV_PROVIDER_STATE pP = NULL;
    PLSA_SRV_PROVIDER_STATE pQ  = pStateList;
    PLSA_SRV_PROVIDER_STATE pR = NULL;

    while (pQ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

VOID
LsaSrvFreeEnumState(
    PLSA_SRV_ENUM_STATE pState
    )
{
    if (pState)
    {
        LW_SAFE_FREE_MEMORY(pState->pszMapName);
        if (pState->pProviderStateList)
        {
            LsaSrvFreeProviderStateList(pState->pProviderStateList);
        }

        LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(pState->bInLock);

        LwFreeMemory(pState);
    }
}
