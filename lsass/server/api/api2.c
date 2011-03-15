/*
 * Copyright Likewise Software
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
 *
 * Module Name:
 *
 *        api2.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Server API (version 2)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#include "api.h"

typedef struct _LSA_SRV_ENUM_HANDLE
{
    enum
    {
        LSA_SRV_ENUM_OBJECTS,
        LSA_SRV_ENUM_MEMBERS
    } Type;
    LSA_FIND_FLAGS FindFlags;
    LSA_OBJECT_TYPE ObjectType;
    PSTR pszDomainName;
    PSTR pszSid;
    PSTR pszTargetInstance;
    PLSA_AUTH_PROVIDER pProvider;
    HANDLE hProvider;
    HANDLE hEnum;
    BOOLEAN bMergeResults;
    BOOLEAN bReleaseLock;
} LSA_SRV_ENUM_HANDLE, *PLSA_SRV_ENUM_HANDLE;

typedef struct _LSA_SRV_MEMBER_OF_PASS
{
    DWORD dwNumProviders;
    DWORD dwTotalSidCount;
    struct
    {
        DWORD dwSidCount;
        PSTR* ppszSids;
    } *pResults;
} LSA_SRV_MEMBER_OF_PASS, *PLSA_SRV_MEMBER_OF_PASS;

VOID
LsaSrvInitializeLock(
     PLSA_SRV_RWLOCK pLock
     )
{
    int localError = 0;

    localError = pthread_mutex_init(&pLock->stateMutex, NULL);
    LSA_ASSERT(localError == 0);

    localError = pthread_cond_init(&pLock->stateCond, NULL);
    LSA_ASSERT(localError == 0);

    pLock->readers = 0;
}

VOID
LsaSrvAcquireRead(
     PLSA_SRV_RWLOCK pLock
     )
{
    int status = 0;

    status = pthread_mutex_lock(&pLock->stateMutex);
    LW_ASSERT(status == 0);
    pLock->readers++;
    status = pthread_mutex_unlock(&pLock->stateMutex);
    LW_ASSERT(status == 0);
}

VOID
LsaSrvAcquireWrite(
     PLSA_SRV_RWLOCK pLock
     )
{
    int status = 0;

    status = pthread_mutex_lock(&pLock->stateMutex);
    LW_ASSERT(status == 0);
    while (pLock->readers)
    {
        pthread_cond_wait(&pLock->stateCond, &pLock->stateMutex);
    }
}

VOID
LsaSrvReleaseRead(
     PLSA_SRV_RWLOCK pLock
     )
{
    int status = 0;

    status = pthread_mutex_lock(&pLock->stateMutex);
    LW_ASSERT(status == 0);
    pLock->readers--;
    if (pLock->readers == 0)
    {
        pthread_cond_broadcast(&pLock->stateCond);
    }
    status = pthread_mutex_unlock(&pLock->stateMutex);
    LW_ASSERT(status == 0);
}


VOID
LsaSrvReleaseWrite(
     PLSA_SRV_RWLOCK pLock
     )
{
    int status = 0;

    LW_ASSERT(pLock->readers == 0);
    status = pthread_mutex_unlock(&pLock->stateMutex);
    LW_ASSERT(status == 0);
}

static
DWORD
LsaSrvInitMemberOfPass(
    DWORD dwNumProviders,
    PLSA_SRV_MEMBER_OF_PASS pPass
    )
{
    DWORD dwError = 0;

    pPass->dwNumProviders = dwNumProviders;
    pPass->dwTotalSidCount = 0;

    dwError = LwAllocateMemory(
        dwNumProviders * sizeof(*pPass->pResults),
        OUT_PPVOID(&pPass->pResults));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
VOID
LsaSrvDestroyMemberOfPass(
    PLSA_SRV_MEMBER_OF_PASS pPass
    )
{
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < pPass->dwNumProviders; dwIndex++)
    {
        if (pPass->pResults[dwIndex].ppszSids)
        {
            LwFreeStringArray(pPass->pResults[dwIndex].ppszSids, pPass->pResults[dwIndex].dwSidCount);
        }
    }

    LW_SAFE_FREE_MEMORY(pPass->pResults);
    memset(pPass, 0, sizeof(*pPass));
}

static
DWORD
LsaSrvConcatenateSidLists(
    IN DWORD dwSidCount,
    IN OUT PSTR** pppszSidList,
    IN DWORD dwAppendCount,
    IN PSTR* ppszAppend
    )
{
    DWORD dwError = 0;
    PSTR* ppszNewList = NULL;

    dwError = LwReallocMemory(*pppszSidList, OUT_PPVOID(&ppszNewList),
                              sizeof(*ppszNewList) * (dwSidCount + dwAppendCount));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(ppszNewList + dwSidCount, ppszAppend, sizeof(*ppszNewList) * dwAppendCount);

    *pppszSidList = ppszNewList;

error:

    return dwError;
}

static
DWORD
LsaSrvMergeMemberOfPass(
    IN PLSA_SRV_MEMBER_OF_PASS pPass,
    IN OUT PLW_HASH_TABLE pSidHash
    )
{
    DWORD dwError = 0;
    DWORD dwResultIndex = 0;
    DWORD dwSidIndex = 0;

    for (dwResultIndex = 0; dwResultIndex < pPass->dwNumProviders; dwResultIndex++)
    {
        for (dwSidIndex = 0; dwSidIndex < pPass->pResults[dwResultIndex].dwSidCount; dwSidIndex++)
        {
            dwError = LwHashSetValue(pSidHash,
                                      pPass->pResults[dwResultIndex].ppszSids[dwSidIndex],
                                      pPass->pResults[dwResultIndex].ppszSids[dwSidIndex]);
            BAIL_ON_LSA_ERROR(dwError);
            pPass->pResults[dwResultIndex].ppszSids[dwSidIndex] = NULL;
        }
    }

error:

    return dwError;
}

static
DWORD
LsaSrvMakeMemberOfFirstPass(
    HANDLE hServer,
    PCSTR pszTargetInstance,
    LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PLSA_SRV_MEMBER_OF_PASS pPass
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = NULL;
    DWORD dwProviderCount = 0;
    DWORD dwIndex = 0;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwProviderCount++;
    }

    dwError = LsaSrvInitMemberOfPass(dwProviderCount, pPass);
    BAIL_ON_LSA_ERROR(dwError);

    for (pProvider = gpAuthProviderList, dwIndex = 0;
         pProvider;
         pProvider = pProvider->pNext, dwIndex++)
    {
        if (pProvider->pFnTable == NULL)
        {
            continue;
        }

        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnQueryMemberOf(
            hProvider,
            FindFlags,
            dwSidCount,
            ppszSids,
            &pPass->pResults[dwIndex].dwSidCount,
            &pPass->pResults[dwIndex].ppszSids);
        switch (dwError)
        {
        case LW_ERROR_NOT_HANDLED:
        case LW_ERROR_NO_SUCH_OBJECT:
        case LW_ERROR_NO_SUCH_USER:
        case LW_ERROR_NO_SUCH_GROUP:
            dwError = LW_ERROR_SUCCESS;
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
        }

        pPass->dwTotalSidCount += pPass->pResults[dwIndex].dwSidCount;

        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = NULL;
    }

cleanup:

    if (hProvider != NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LsaSrvMakeMemberOfTransferPass(
    HANDLE hServer,
    PCSTR pszTargetInstance,
    LSA_FIND_FLAGS FindFlags,
    IN PLSA_SRV_MEMBER_OF_PASS pIn,
    OUT PLSA_SRV_MEMBER_OF_PASS pOut
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PLSA_AUTH_PROVIDER pSourceProvider = NULL;
    HANDLE hProvider = NULL;
    PSTR* ppszBatchedSids = NULL;
    DWORD dwBatchedSidCount = 0;
    DWORD dwIndex = 0;
    DWORD dwSourceIndex = 0;

    LsaSrvDestroyMemberOfPass(pOut);

    dwError = LsaSrvInitMemberOfPass(pIn->dwNumProviders, pOut);
    BAIL_ON_LSA_ERROR(dwError);

    /* Iterate each provider that we will query */
    for (pProvider = gpAuthProviderList, dwIndex = 0;
         pProvider;
         pProvider = pProvider->pNext, dwIndex++)
    {
        if (pProvider->pFnTable == NULL)
        {
            continue;
        }

        /* Iterate each provider from previous pass */
        for (pSourceProvider = gpAuthProviderList, dwSourceIndex = 0;
             pSourceProvider;
             pSourceProvider = pSourceProvider->pNext, dwSourceIndex++)
        {
            /* If the source provider is not the provider we are querying,
               go ahead and add it to the query.  This test can be refined
               to take into account relationships between providers to
               avoid making unnecessary queries */
            if (strcmp(pSourceProvider->pszName, pProvider->pszName) &&
                pIn->pResults[dwSourceIndex].dwSidCount)
            {
                dwError = LsaSrvConcatenateSidLists(
                    dwBatchedSidCount,
                    &ppszBatchedSids,
                    pIn->pResults[dwSourceIndex].dwSidCount,
                    pIn->pResults[dwSourceIndex].ppszSids);
                BAIL_ON_LSA_ERROR(dwError);

                dwBatchedSidCount += pIn->pResults[dwSourceIndex].dwSidCount;
            }
        }
        
        if (dwBatchedSidCount)
        {
            dwError = LsaSrvOpenProvider(
                          hServer,
                          pProvider,
                          pszTargetInstance,
                          &hProvider);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = pProvider->pFnTable->pfnQueryMemberOf(
                hProvider,
                FindFlags,
                dwBatchedSidCount,
                ppszBatchedSids,
                &pOut->pResults[dwIndex].dwSidCount,
                &pOut->pResults[dwIndex].ppszSids);
            switch (dwError)
            {
            case LW_ERROR_NOT_HANDLED:
            case LW_ERROR_NO_SUCH_OBJECT:
            case LW_ERROR_NO_SUCH_USER:
            case LW_ERROR_NO_SUCH_GROUP:
                dwError = LW_ERROR_SUCCESS;
                break;
            default:
                BAIL_ON_LSA_ERROR(dwError);
            }

            pOut->dwTotalSidCount += pOut->pResults[dwIndex].dwSidCount;

            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = NULL;
        }
        
        LW_SAFE_FREE_MEMORY(ppszBatchedSids);
        dwBatchedSidCount = 0;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(ppszBatchedSids);

    if (hProvider != NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    return dwError;

error:

    goto cleanup;
}

static
VOID
LsaSrvConstructPartialQuery(
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    IN PLSA_SECURITY_OBJECT* ppCombinedObjects,
    OUT PDWORD pdwPartialCount,
    OUT LSA_QUERY_LIST PartialQueryList
    )
{
    DWORD dwPartialIndex = 0;
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppCombinedObjects[dwIndex] == NULL)
        {
            switch (QueryType)
            {
            case LSA_QUERY_TYPE_BY_UNIX_ID:
                PartialQueryList.pdwIds[dwPartialIndex++] = QueryList.pdwIds[dwIndex];
                break;
            default:
                PartialQueryList.ppszStrings[dwPartialIndex++] = QueryList.ppszStrings[dwIndex];
                break;
            }
        }
    }

    *pdwPartialCount = dwPartialIndex;

    return;
}

static
VOID
LsaSrvMergePartialQueryResult(
    IN DWORD dwCount,
    IN PLSA_SECURITY_OBJECT* ppPartialObjects,
    OUT PLSA_SECURITY_OBJECT* ppCombinedObjects
    )
{
    DWORD dwIndex = 0;
    DWORD dwPartialIndex = 0;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppCombinedObjects[dwIndex] == NULL)
        {
            ppCombinedObjects[dwIndex] = ppPartialObjects[dwPartialIndex++];
        }
    }
}

static
DWORD
LsaSrvFindObjectsInternal(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    IN OUT PLSA_SECURITY_OBJECT* ppCombinedObjects
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = NULL;
    PLSA_SECURITY_OBJECT* ppPartialObjects = NULL;
    LSA_QUERY_LIST PartialQueryList;
    DWORD dwPartialCount = 0;
    BOOLEAN bFoundProvider = FALSE;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    memset(&PartialQueryList, 0, sizeof(PartialQueryList));

    switch (QueryType)
    {
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        dwError = LwAllocateMemory(
            sizeof(*PartialQueryList.pdwIds) * dwCount,
            OUT_PPVOID(&PartialQueryList.pdwIds));
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LwAllocateMemory(
            sizeof(*PartialQueryList.ppszStrings) * dwCount,
            OUT_PPVOID(&PartialQueryList.ppszStrings));
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        if (pszTargetProviderName)
        {
            if (!strcmp(pszTargetProviderName, pProvider->pszName))
            {
                bFoundProvider = TRUE;
            }
            else
            {
                continue;
            }
        }

        if (pProvider->pFnTable == NULL)
        {
            continue;
        }

        LsaSrvConstructPartialQuery(
            QueryType,
            dwCount,
            QueryList,
            ppCombinedObjects,
            &dwPartialCount,
            PartialQueryList);

        /* Stop iterating if all keys now have results */
        if (dwPartialCount == 0)
        {
            break;
        }

        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnFindObjects(
            hProvider,
            FindFlags,
            ObjectType,
            QueryType,
            dwPartialCount,
            PartialQueryList,
            &ppPartialObjects);

        if (dwError == LW_ERROR_NOT_HANDLED)
        {
            dwError = LW_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
            
            LsaSrvMergePartialQueryResult(
                dwCount,
                ppPartialObjects,
                ppCombinedObjects);
        }   

        LW_SAFE_FREE_MEMORY(ppPartialObjects);
        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = NULL;
    }
        
    if (pszTargetProviderName && !bFoundProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    /* All objects inside the partial result list were moved into
       the combined list, so we do a shallow free */
    LW_SAFE_FREE_MEMORY(ppPartialObjects);
    /* Note that this is safe regardless of the type of the query */
    LW_SAFE_FREE_MEMORY(PartialQueryList.ppszStrings);

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (hProvider != NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvFindObjects(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppCombinedObjects = NULL;
    DWORD dwIndex = 0;
    LSA_QUERY_LIST SingleList;
    LSA_QUERY_TYPE SingleType = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LwAllocateMemory(
        sizeof(*ppCombinedObjects) * dwCount,
        OUT_PPVOID(&ppCombinedObjects));
    BAIL_ON_LSA_ERROR(dwError);

    switch (QueryType)
    {
    default:
        dwError = LsaSrvFindObjectsInternal(
            hServer,
            pszTargetProvider,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            ppCombinedObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_QUERY_TYPE_BY_NAME:
        /* Each name in the query list could end up being
           a different type (alias, NT4, UPN, etc.), so break
           the query up into multiple single queries,
           using LsaCrackDomainQualifiedName() to determine
           the appropriate query type for each name */
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            dwError = LsaSrvCrackDomainQualifiedName(
                QueryList.ppszStrings[dwIndex],
                &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);

            switch (pLoginInfo->nameType)
            {
            case NameType_NT4:
                SingleType = LSA_QUERY_TYPE_BY_NT4;
                break;
            case NameType_UPN:
                SingleType = LSA_QUERY_TYPE_BY_UPN;
                break;
            case NameType_Alias:
                SingleType = LSA_QUERY_TYPE_BY_ALIAS;
                break;
            default:
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            SingleList.ppszStrings = &QueryList.ppszStrings[dwIndex];

            dwError = LsaSrvFindObjectsInternal(
                hServer,
                pszTargetProvider,
                FindFlags,
                ObjectType,
                SingleType,
                1,
                SingleList,
                &ppCombinedObjects[dwIndex]);
            BAIL_ON_LSA_ERROR(dwError);

            LsaSrvFreeNameInfo(pLoginInfo);
            pLoginInfo = NULL;
        }
        break;
    }

    *pppObjects = ppCombinedObjects;

cleanup:

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
        pLoginInfo = NULL;
    }

    return dwError;

error:

    *pppObjects = NULL;

    if (ppCombinedObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppCombinedObjects);
    }

    goto cleanup;
}

DWORD
LsaSrvOpenEnumObjects(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_HANDLE pEnum = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);

    if (pszDomainName)
    {
        dwError = LwAllocateString(pszDomainName, &pEnum->pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEnum->Type = LSA_SRV_ENUM_OBJECTS;
    pEnum->FindFlags = FindFlags;
    pEnum->ObjectType = ObjectType;

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);

        pEnum->pszTargetInstance = pszTargetInstance;
        pszTargetInstance = NULL;
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(pEnum->bReleaseLock);
    
    if (pszTargetProviderName)
    {
        for (pProvider = gpAuthProviderList;
             pProvider;
             pProvider = pProvider->pNext)
        {
            if (!strcmp(pszTargetProviderName, pProvider->pszName))
            {
                pEnum->pProvider = pProvider;
                break;
            }
        }

        if (!pEnum->pProvider)
        {
            dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        pEnum->pProvider = gpAuthProviderList;
        pEnum->bMergeResults = TRUE;
    }

    *phEnum = pEnum;

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    return dwError;

error:

    if (pEnum)
    {
        LsaSrvCloseEnum(hServer, pEnum);
    }

    goto cleanup;
}

DWORD
LsaSrvEnumObjects(
    IN HANDLE hServer,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_HANDLE pEnum = hEnum;
    PLSA_SECURITY_OBJECT* ppCombinedObjects = NULL;
    PLSA_SECURITY_OBJECT* ppPartialObjects = NULL;
    DWORD dwCombinedObjectCount = 0;
    DWORD dwPartialObjectCount = 0;

    if (pEnum->Type != LSA_SRV_ENUM_OBJECTS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        sizeof(*ppCombinedObjects) * dwMaxObjectsCount,
        OUT_PPVOID(&ppCombinedObjects));
    BAIL_ON_LSA_ERROR(dwError);
    
    while (dwCombinedObjectCount < dwMaxObjectsCount && pEnum->pProvider != NULL)
    {
        if (pEnum->pProvider->pFnTable == NULL)
        {
            pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
            continue;
        }

        if (!pEnum->hProvider)
        {
            dwError = LsaSrvOpenProvider(
                          hServer,
                          pEnum->pProvider,
                          pEnum->pszTargetInstance,
                          &pEnum->hProvider);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (!pEnum->hEnum)
        {
            dwError = pEnum->pProvider->pFnTable->pfnOpenEnumObjects(
                pEnum->hProvider,
                &pEnum->hEnum,
                pEnum->FindFlags,
                pEnum->ObjectType,
                pEnum->pszDomainName);
            if (dwError == LW_ERROR_NOT_HANDLED)
            {
                dwError = LW_ERROR_SUCCESS;
                pEnum->pProvider->pFnTable->pfnCloseHandle(pEnum->hProvider);
                pEnum->hProvider = NULL;
                pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
                continue;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        dwError = pEnum->pProvider->pFnTable->pfnEnumObjects(
            pEnum->hEnum,
            dwMaxObjectsCount - dwCombinedObjectCount,
            &dwPartialObjectCount,
            &ppPartialObjects);
        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            dwError = LW_ERROR_SUCCESS;
            pEnum->pProvider->pFnTable->pfnCloseEnum(pEnum->hEnum);
            pEnum->hEnum = NULL;
            pEnum->pProvider->pFnTable->pfnCloseHandle(pEnum->hProvider);
            pEnum->hProvider = NULL;
            pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
            continue;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        memcpy(ppCombinedObjects + dwCombinedObjectCount,
               ppPartialObjects,
               sizeof(*ppPartialObjects) * dwPartialObjectCount);
        dwCombinedObjectCount += dwPartialObjectCount;

        LW_SAFE_FREE_MEMORY(ppPartialObjects);
    }

    if (dwCombinedObjectCount == 0)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        *pppObjects = ppCombinedObjects;
        *pdwObjectsCount = dwCombinedObjectCount;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(ppPartialObjects);

    return dwError;

error:

    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    if (ppCombinedObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCombinedObjectCount, ppCombinedObjects);
    }

    goto cleanup;
}

DWORD
LsaSrvOpenEnumMembers(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_HANDLE pEnum = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);
   
    dwError = LwAllocateString(pszSid, &pEnum->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    pEnum->Type = LSA_SRV_ENUM_MEMBERS;
    pEnum->FindFlags = FindFlags;

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);

        pEnum->pszTargetInstance = pszTargetInstance;
        pszTargetInstance = NULL;
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(pEnum->bReleaseLock);
    
    if (pszTargetProviderName)
    {
        for (pProvider = gpAuthProviderList;
             pProvider;
             pProvider = pProvider->pNext)
        {
            if (!strcmp(pszTargetProviderName, pProvider->pszName))
            {
                pEnum->pProvider = pProvider;
                break;
            }
        }

        if (!pEnum->pProvider)
        {
            dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        pEnum->pProvider = gpAuthProviderList;
        pEnum->bMergeResults = TRUE;
    }

    *phEnum = pEnum;

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    return dwError;

error:

    if (pEnum)
    {
        LsaSrvCloseEnum(hServer, pEnum);
    }

    goto cleanup;
}

DWORD
LsaSrvEnumMembers(
    IN HANDLE hServer,
    IN HANDLE hEnum,
    IN DWORD dwMaxSidCount,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSids
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_HANDLE pEnum = hEnum;
    PSTR* ppszCombinedSids = NULL;
    PSTR* ppszPartialSids = NULL;
    DWORD dwCombinedSidCount = 0;
    DWORD dwPartialSidCount = 0;

    if (pEnum->Type != LSA_SRV_ENUM_MEMBERS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        sizeof(*ppszCombinedSids) * dwMaxSidCount,
        OUT_PPVOID(&ppszCombinedSids));
    BAIL_ON_LSA_ERROR(dwError);
    
    while (dwCombinedSidCount < dwMaxSidCount && pEnum->pProvider != NULL)
    {
        if (pEnum->pProvider->pFnTable == NULL)
        {
            pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
            continue;
        }

        if (!pEnum->hProvider)
        {
            dwError = LsaSrvOpenProvider(
                          hServer,
                          pEnum->pProvider,
                          pEnum->pszTargetInstance,
                          &pEnum->hProvider);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (!pEnum->hEnum)
        {
            dwError = pEnum->pProvider->pFnTable->pfnOpenEnumGroupMembers(
                pEnum->hProvider,
                &pEnum->hEnum,
                pEnum->FindFlags,
                pEnum->pszSid);
            switch (dwError)
            {
            case LW_ERROR_NOT_HANDLED:
            case LW_ERROR_NO_SUCH_OBJECT:
            case LW_ERROR_NO_SUCH_GROUP:
                dwError = LW_ERROR_SUCCESS;
                pEnum->pProvider->pFnTable->pfnCloseHandle(pEnum->hProvider);
                pEnum->hProvider = NULL;
                pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
                continue;
            default:
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        dwError = pEnum->pProvider->pFnTable->pfnEnumGroupMembers(
            pEnum->hEnum,
            dwMaxSidCount - dwCombinedSidCount,
            &dwPartialSidCount,
            &ppszPartialSids);
        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            dwError = LW_ERROR_SUCCESS;
            pEnum->pProvider->pFnTable->pfnCloseEnum(pEnum->hEnum);
            pEnum->hEnum = NULL;
            pEnum->pProvider->pFnTable->pfnCloseHandle(pEnum->hProvider);
            pEnum->hProvider = NULL;
            pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
            continue;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        memcpy(ppszCombinedSids + dwCombinedSidCount,
               ppszPartialSids,
               sizeof(*ppszPartialSids) * dwPartialSidCount);
        dwCombinedSidCount += dwPartialSidCount;

        LW_SAFE_FREE_MEMORY(ppszPartialSids);
    }

    if (dwCombinedSidCount == 0)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        *pppszSids = ppszCombinedSids;
        *pdwSidCount = dwCombinedSidCount;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(ppszPartialSids);

    return dwError;

error:

    *pdwSidCount = 0;
    *pppszSids = NULL;

    if (ppszCombinedSids)
    {
        LwFreeStringArray(ppszCombinedSids, dwCombinedSidCount);
    }

    goto cleanup;
}

static
VOID
LsaSrvFreeMemberOfHashEntry(
    const LW_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        LwFreeMemory(pEntry->pValue);
    }
}

static
DWORD
LsaSrvQueryMemberOfMerged(
    IN HANDLE hServer,
    IN OPTIONAL PCSTR pszTargetInstance,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR* ppszCombinedSids = NULL;
    DWORD dwCombinedCount = 0;
    LSA_SRV_MEMBER_OF_PASS pass1 = {0};
    LSA_SRV_MEMBER_OF_PASS pass2 = {0};
    PLSA_SRV_MEMBER_OF_PASS pSourcePass = &pass1;
    PLSA_SRV_MEMBER_OF_PASS pDestPass = &pass2;
    PLSA_SRV_MEMBER_OF_PASS pTempPass = NULL;
    PLW_HASH_TABLE pHash = NULL;
    LW_HASH_ITERATOR hashIterator = {0};
    LW_HASH_ENTRY* pHashEntry = NULL;
    DWORD dwIndex = 0;
    
    dwError = LwHashCreate(
        29,
        LwHashCaselessStringCompare,
        LwHashCaselessStringHash,
        LsaSrvFreeMemberOfHashEntry,
        NULL,
        &pHash);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LsaSrvMakeMemberOfFirstPass(
        hServer,
        pszTargetInstance,
        FindFlags,
        dwSidCount,
        ppszSids,
        pSourcePass);
    BAIL_ON_LSA_ERROR(dwError);

    while (pSourcePass->dwTotalSidCount)
    {
        dwError = LsaSrvMakeMemberOfTransferPass(
            hServer,
            pszTargetInstance,
            FindFlags,
            pSourcePass,
            pDestPass);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaSrvMergeMemberOfPass(
            pSourcePass,
            pHash);
        BAIL_ON_LSA_ERROR(dwError);

        pTempPass = pSourcePass;
        pSourcePass = pDestPass;
        pDestPass =  pTempPass;
    }

    dwCombinedCount = (DWORD) LwHashGetKeyCount(pHash);
    
    if (dwCombinedCount)
    {
        dwError = LwAllocateMemory(
            sizeof(*ppszCombinedSids) * dwCombinedCount,
            OUT_PPVOID(&ppszCombinedSids));
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = LwHashGetIterator(pHash, &hashIterator);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (dwIndex = 0; (pHashEntry = LwHashNext(&hashIterator)) != NULL; dwIndex++)
        {
            ppszCombinedSids[dwIndex] = (PSTR) pHashEntry->pValue;
            pHashEntry->pValue = NULL;
        }
    }

    *pppszGroupSids = ppszCombinedSids;
    *pdwGroupSidCount = dwCombinedCount;

cleanup:

    LsaSrvDestroyMemberOfPass(&pass1);
    LsaSrvDestroyMemberOfPass(&pass2);
    LwHashSafeFree(&pHash);

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return dwError;

error:

    *pppszGroupSids = NULL;
    *pdwGroupSidCount = 0;

    if (ppszCombinedSids)
    {
        LwFreeStringArray(ppszCombinedSids, dwCombinedCount);
    }

    goto cleanup;
}

static
DWORD
LsaSrvQueryMemberOfSingle(
    IN HANDLE hServer,
    IN OPTIONAL PCSTR pszTargetProviderName,
    IN OPTIONAL PCSTR pszTargetInstance,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = NULL;
    BOOLEAN bInLock = FALSE;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        if (pProvider->pFnTable == NULL)
        {
            continue;
        }

        if (!strcmp(pProvider->pszName, pszTargetProviderName))
        {
            dwError = LsaSrvOpenProvider(
                          hServer,
                          pProvider,
                          pszTargetInstance,
                          &hProvider);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = pProvider->pFnTable->pfnQueryMemberOf(
                hProvider,
                FindFlags,
                dwSidCount,
                ppszSids,
                pdwGroupSidCount,
                pppszGroupSids);
            BAIL_ON_LSA_ERROR(dwError);

            goto cleanup;
        }
    }

    dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hProvider != NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvQueryMemberOf(
    IN HANDLE hServer,
    IN OPTIONAL PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    
    if (dwSidCount == 0)
    {
        *pdwGroupSidCount = 0;
        *pppszGroupSids = NULL;
        goto cleanup;
    }

    if (pszTargetProviderName)
    {
        dwError = LsaSrvQueryMemberOfSingle(
            hServer,
            pszTargetProviderName,
            pszTargetInstance,
            FindFlags,
            dwSidCount,
            ppszSids,
            pdwGroupSidCount,
            pppszGroupSids);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaSrvQueryMemberOfMerged(
            hServer,
            pszTargetInstance,
            FindFlags,
            dwSidCount,
            ppszSids,
            pdwGroupSidCount,
            pppszGroupSids);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    return dwError;

error:

    *pppszGroupSids = NULL;
    *pdwGroupSidCount = 0;

    goto cleanup;
}

static
DWORD
LsaSrvQueryExpandedGroupMembersInternal(
    IN HANDLE hServer,
    PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN PCSTR pszSid,
    IN OUT PLW_HASH_TABLE pHash
    )
{
    DWORD dwError = 0;
    HANDLE hEnum = NULL;
    static const DWORD dwMaxEnumCount = 128;
    DWORD dwEnumCount = 0;
    PSTR* ppszSids = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    DWORD dwIndex = 0;

    dwError = LsaSrvOpenEnumMembers(
        hServer,
        pszTargetProvider,
        &hEnum,
        FindFlags,
        pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    for (;;)
    {
        dwError = LsaSrvEnumMembers(
            hServer,
            hEnum,
            dwMaxEnumCount,
            &dwEnumCount,
            &ppszSids);
        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            dwError = 0;
            break;
        }
        BAIL_ON_LSA_ERROR(dwError);

        QueryList.ppszStrings = (PCSTR*) ppszSids;

        dwError = LsaSrvFindObjects(
            hServer,
            pszTargetProvider,
            FindFlags,
            LSA_OBJECT_TYPE_UNDEFINED,
            LSA_QUERY_TYPE_BY_SID,
            dwEnumCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        for (dwIndex = 0; dwIndex < dwEnumCount; dwIndex++)
        {
            if (ppObjects[dwIndex] && !LwHashExists(pHash, ppObjects[dwIndex]->pszObjectSid))
            {
                dwError = LwHashSetValue(
                    pHash,
                    ppObjects[dwIndex]->pszObjectSid,
                    ppObjects[dwIndex]);
                BAIL_ON_LSA_ERROR(dwError);

                if (ppObjects[dwIndex]->type == LSA_OBJECT_TYPE_GROUP)
                {
                    dwError = LsaSrvQueryExpandedGroupMembersInternal(
                        hServer,
                        pszTargetProvider,
                        FindFlags,
                        ObjectType,
                        ppObjects[dwIndex]->pszObjectSid,
                        pHash);
                    ppObjects[dwIndex] = NULL;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                else
                {
                    ppObjects[dwIndex] = NULL;
                }
            }
        }

        if (ppszSids)
        {
            LwFreeStringArray(ppszSids, dwEnumCount);
        }

        if (ppObjects)
        {
            LsaUtilFreeSecurityObjectList(dwEnumCount, ppObjects);
            ppObjects = NULL;
        }
    }

cleanup:

    if (ppszSids)
    {
        LwFreeStringArray(ppszSids, dwEnumCount);
    }

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwEnumCount, ppObjects);
        ppObjects = NULL;
    }

    if (hEnum)
    {
        LsaSrvCloseEnum(hServer, hEnum);
    }

    return dwError;

error:

    goto cleanup;
}

static
VOID
LsaFreeMemberHashEntry(
    const LW_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        LsaUtilFreeSecurityObject(pEntry->pValue);
    }
}

static
DWORD
LsaSrvQueryExpandedGroupMembers(
    IN HANDLE hServer,
    PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN PCSTR pszSid,
    OUT PDWORD pdwMemberCount,
    OUT PLSA_SECURITY_OBJECT** pppMembers
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PLW_HASH_TABLE pHash = NULL;
    LW_HASH_ITERATOR hashIterator = {0};
    LW_HASH_ENTRY* pHashEntry = NULL;
    DWORD dwMemberCount = 0;
    DWORD dwFilteredMemberCount = 0;
    PLSA_SECURITY_OBJECT* ppMembers = NULL;
    PLSA_SECURITY_OBJECT pMember = NULL;

    dwError = LwHashCreate(
        29,
        LwHashCaselessStringCompare,
        LwHashCaselessStringHash,
        LsaFreeMemberHashEntry,
        NULL,
        &pHash);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvQueryExpandedGroupMembersInternal(
        hServer,
        pszTargetProvider,
        FindFlags,
        ObjectType,
        pszSid,
        pHash);

    dwMemberCount = (DWORD) LwHashGetKeyCount(pHash);
    
    if (dwMemberCount)
    {
        dwError = LwAllocateMemory(
            sizeof(*ppMembers) * dwMemberCount,
            OUT_PPVOID(&ppMembers));
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = LwHashGetIterator(pHash, &hashIterator);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (dwIndex = 0; (pHashEntry = LwHashNext(&hashIterator)) != NULL; dwIndex++)
        {
            pMember = pHashEntry->pValue;

            if (ObjectType == LSA_OBJECT_TYPE_UNDEFINED ||
                pMember->type == ObjectType)
            {
                ppMembers[dwFilteredMemberCount++] = pMember;
                pHashEntry->pValue = NULL;
            }
        }
    }

    *pppMembers = ppMembers;
    *pdwMemberCount = dwFilteredMemberCount;

cleanup:

    LwHashSafeFree(&pHash);

    return dwError;

error:

    *pppMembers = NULL;
    *pdwMemberCount = 0;

    if (ppMembers)
    {
        LsaUtilFreeSecurityObjectList(dwMemberCount, ppMembers);
    }

    goto cleanup;
}

LW_DWORD
LsaSrvFindGroupAndExpandedMembers(
    LW_IN LW_HANDLE hServer,
    LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LSA_QUERY_TYPE QueryType,
    LW_IN LSA_QUERY_ITEM QueryItem,
    LW_OUT PLSA_SECURITY_OBJECT* ppGroupObject,
    LW_OUT LW_PDWORD pdwMemberObjectCount,
    LW_OUT PLSA_SECURITY_OBJECT** pppMemberObjects
    )
{
    DWORD dwError = 0;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    switch(QueryType)
    {
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        QueryList.pdwIds = &QueryItem.dwId;
        break;
    default:
        QueryList.ppszStrings = &QueryItem.pszString;
        break;
    }

    dwError = LsaSrvFindObjects(
        hServer,
        pszTargetProvider,
        FindFlags,
        LSA_OBJECT_TYPE_GROUP,
        QueryType,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvQueryExpandedGroupMembers(
        hServer,
        pszTargetProvider,
        FindFlags,
        LSA_OBJECT_TYPE_USER,
        ppObjects[0]->pszObjectSid,
        pdwMemberObjectCount,
        pppMemberObjects);
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupObject = ppObjects[0];
    ppObjects[0] = NULL;

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    return dwError;

error:

    *ppGroupObject = NULL;
    *pdwMemberObjectCount = 0;
    *pppMemberObjects = NULL;

    goto cleanup;
}

VOID
LsaSrvCloseEnum(
    IN HANDLE hServer,
    IN OUT HANDLE hEnum
    )
{
    PLSA_SRV_ENUM_HANDLE pEnum = hEnum;

    if (pEnum)
    {
        if (pEnum->hEnum)
        {
            pEnum->pProvider->pFnTable->pfnCloseEnum(pEnum->hEnum);
        }

        if (pEnum->hProvider)
        {
            pEnum->pProvider->pFnTable->pfnCloseHandle(pEnum->hProvider);
        }

        LW_SAFE_FREE_STRING(pEnum->pszDomainName);
        LW_SAFE_FREE_STRING(pEnum->pszSid);
        LW_SAFE_FREE_STRING(pEnum->pszTargetInstance);
        LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(pEnum->bReleaseLock);
        LwFreeMemory(pEnum);
    }
}

DWORD
LsaSrvAddUser2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_USER_ADD_INFO pUserAddInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    BOOLEAN bInLock = FALSE;
    BOOLEAN bFoundProvider = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));;

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        if (pszTargetProviderName &&
            strcmp(pProvider->pszName, pszTargetProviderName))
        {
            continue;
        }

        bFoundProvider = TRUE;

        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnAddUser(
                                        hProvider,
                                        pUserAddInfo);
        if (dwError == LW_ERROR_SUCCESS)
        {
            break;
        }
        else if (dwError == LW_ERROR_NOT_HANDLED && !pszTargetProvider)
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = NULL;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pszTargetProviderName && !bFoundProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "add user");

    goto cleanup;
}

DWORD
LsaSrvModifyUser2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    BOOLEAN bInLock = FALSE;
    BOOLEAN bFoundProvider = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        if (pszTargetProviderName &&
            strcmp(pProvider->pszName, pszTargetProviderName))
        {
            continue;
        }

        bFoundProvider = TRUE;

        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnModifyUser(
                                        hProvider,
                                        pUserModInfo);
        if (dwError == LW_ERROR_SUCCESS)
        {
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED ||
                  dwError == LW_ERROR_NO_SUCH_USER) &&
                 !pszTargetProviderName)
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = NULL;
        } else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pszTargetProviderName && !bFoundProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "modify user (sid %s)", pUserModInfo->pszSid);

    goto cleanup;
}

DWORD
LsaSrvAddGroup2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bFoundProvider = FALSE;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        if (pszTargetProviderName &&
            strcmp(pProvider->pszName, pszTargetProviderName))
        {
            continue;
        }

        bFoundProvider = TRUE;

        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnAddGroup(
                                        hProvider,
                                        pGroupAddInfo);
        if (dwError == LW_ERROR_SUCCESS)
        {
            break;
        }
        else if (dwError == LW_ERROR_NOT_HANDLED && !pszTargetProviderName)
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = NULL;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pszTargetProviderName && !bFoundProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (hProvider != NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "add group");

    goto cleanup;
}

DWORD
LsaSrvModifyGroup2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    BOOLEAN bInLock = FALSE;
    BOOLEAN bFoundProvider = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        if (pszTargetProviderName &&
            strcmp(pProvider->pszName, pszTargetProviderName))
        {
            continue;
        }

        bFoundProvider = TRUE;

        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnModifyGroup(
                                        hProvider,
                                        pGroupModInfo);
        if (dwError == LW_ERROR_SUCCESS)
        {
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED ||
             dwError == LW_ERROR_NO_SUCH_GROUP) &&
            !pszTargetProviderName)
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = NULL;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pszTargetProviderName && !bFoundProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (hProvider != (HANDLE)NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:
    
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "modify group (sid %s)", pGroupModInfo->pszSid);

    goto cleanup;
}

DWORD
LsaSrvDeleteObject(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PCSTR pszSid
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bFoundProvider = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        if (pszTargetProviderName &&
            strcmp(pProvider->pszName, pszTargetProviderName))
        {
            continue;
        }

        bFoundProvider = TRUE;

        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnDeleteObject(hProvider, pszSid);
        if (dwError == LW_ERROR_SUCCESS)
        {
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED ||
                  dwError == LW_ERROR_NO_SUCH_OBJECT ||
                  dwError == LW_ERROR_NO_SUCH_USER ||
                  dwError == LW_ERROR_NO_SUCH_GROUP) &&
                 !pszTargetProviderName)
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pszTargetProviderName && !bFoundProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "delete object (sid %s)", pszSid);

    goto cleanup;
}

DWORD
LsaSrvGetSmartCardUserObject(
    IN HANDLE hServer,
    OUT PLSA_SECURITY_OBJECT* ppObject,
    OUT PSTR* ppszSmartCardReader
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnGetSmartCardUserObject(
                                        hProvider,
                                        ppObject,
                                        ppszSmartCardReader);
        if (dwError == LW_ERROR_SUCCESS)
        {
            break;
        }
        else if (dwError == LW_ERROR_NOT_HANDLED)
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = NULL;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:

    goto cleanup;
}
