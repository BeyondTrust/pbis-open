/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        table.c
 *
 * Abstract:
 *
 *        Object table logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

#define RESTART_PERIOD 30
#define RESTART_LIMIT 2

static
DWORD
LwSmTablePollEntry(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    );

static
VOID
LwSmTableFreeEntry(
    PSM_TABLE_ENTRY pEntry
    );

static SM_TABLE gServiceTable = 
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .pLock = &gServiceTable.lock,
    .entries = {&gServiceTable.entries, &gServiceTable.entries}
};

static PLW_SERVICE_LOADER_VTBL gLoaderTable[] =
{
    [LW_SERVICE_TYPE_LEGACY_EXECUTABLE] = &gExecutableVtbl,
    [LW_SERVICE_TYPE_EXECUTABLE] = &gExecutableVtbl,
    [LW_SERVICE_TYPE_DRIVER] = &gDriverVtbl,
    [LW_SERVICE_TYPE_MODULE] = &gContainerVtbl,
    [LW_SERVICE_TYPE_STUB] = &gStubVtbl
};

DWORD
LwSmTableGetEntry(
    PCWSTR pwszName,
    PSM_TABLE_ENTRY* ppEntry
    )
{
    DWORD dwError = 0;
    BOOL bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;

    LOCK(bLocked, gServiceTable.pLock);

    for (pLink = LwSmLinkBegin(&gServiceTable.entries);
         LwSmLinkValid(&gServiceTable.entries, pLink);
         pLink = LwSmLinkNext(pLink))
    {
        pEntry = STRUCT_FROM_MEMBER(pLink, SM_TABLE_ENTRY, link);
        
        if (LwRtlWC16StringIsEqual(pEntry->pInfo->pwszName, pwszName, TRUE))
        {
            pEntry->dwRefCount++;
            *ppEntry = pEntry;
            goto cleanup;
        }
    }

    dwError = LW_ERROR_NO_SUCH_SERVICE;
    BAIL_ON_ERROR(dwError);

cleanup:

    UNLOCK(bLocked, gServiceTable.pLock);

    return dwError;

error:

    *ppEntry = NULL;

    goto cleanup;
}

DWORD
LwSmTableEnumerateEntries(
    PWSTR** pppwszServiceNames
    )
{
    DWORD dwError = 0;
    BOOL bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;
    size_t count = 0;
    size_t i = 0;
    PWSTR* ppwszServiceNames = NULL;

    LOCK(bLocked, gServiceTable.pLock);

    for (pLink = NULL; (pLink = SM_LINK_ITERATE(&gServiceTable.entries, pLink));)
    {
        count++;
    }

    dwError = LwAllocateMemory(
        sizeof(*ppwszServiceNames) * (count + 1),
        OUT_PPVOID(&ppwszServiceNames));
    BAIL_ON_ERROR(dwError);

    for (pLink = NULL, i = 0; (pLink = SM_LINK_ITERATE(&gServiceTable.entries, pLink)); i++)
    {
        pEntry = STRUCT_FROM_MEMBER(pLink, SM_TABLE_ENTRY, link);

        dwError = LwAllocateWc16String(&ppwszServiceNames[i], pEntry->pInfo->pwszName);
        BAIL_ON_ERROR(dwError);
    }

    *pppwszServiceNames = ppwszServiceNames;

cleanup:

    UNLOCK(bLocked, gServiceTable.pLock);

    return dwError;

error:

    *pppwszServiceNames = NULL;

    if (ppwszServiceNames)
    {
        LwSmFreeStringList(ppwszServiceNames);
    }

    goto cleanup;
}

static
DWORD
LwSmTableReconstructEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    PWSTR pwszLoaderName = NULL;

    if (pEntry->object.pData)
    {
        pEntry->pVtbl->pfnDestruct(&pEntry->object);
        pEntry->object.pData = NULL;
    }

    pEntry->pVtbl = gLoaderTable[pEntry->pInfo->type];

    dwError = pEntry->pVtbl->pfnConstruct(&pEntry->object, pEntry->pInfo, &pEntry->object.pData);
    BAIL_ON_ERROR(dwError);

    pEntry->bDirty = FALSE;

error:

    LW_SAFE_FREE_MEMORY(pwszLoaderName);

    return dwError;
}

DWORD
LwSmTableAddEntry(
    PLW_SERVICE_INFO pInfo,
    PSM_TABLE_ENTRY* ppEntry
    )
{
    DWORD dwError = 0;
    BOOL bLocked = TRUE;
    PSM_TABLE_ENTRY pEntry = NULL;

    dwError = LwAllocateMemory(sizeof(*pEntry), OUT_PPVOID(&pEntry));
    BAIL_ON_ERROR(dwError);

    LwSmLinkInit(&pEntry->link);
    LwSmLinkInit(&pEntry->waiters);

    pEntry->bValid = TRUE;

    dwError = LwSmCopyServiceInfo(pInfo, &pEntry->pInfo);
    
    dwError = LwMapErrnoToLwError(pthread_mutex_init(&pEntry->lock, NULL));
    BAIL_ON_ERROR(dwError);
    pEntry->pLock = &pEntry->lock;

    dwError = LwMapErrnoToLwError(pthread_cond_init(&pEntry->event, NULL));
    BAIL_ON_ERROR(dwError);
    pEntry->pEvent = &pEntry->event;

    dwError = LwSmTableReconstructEntry(pEntry);
    BAIL_ON_ERROR(dwError);

    LOCK(bLocked, gServiceTable.pLock);

    LwSmLinkInsertBefore(&gServiceTable.entries, &pEntry->link);

    pEntry->dwRefCount++;

    UNLOCK(bLocked, gServiceTable.pLock);

    *ppEntry = pEntry;

cleanup:

    return dwError;

error:

    if (pEntry)
    {
        LwSmTableFreeEntry(pEntry);
    }

    goto cleanup;
}

DWORD
LwSmTableUpdateEntry(
    PSM_TABLE_ENTRY pEntry,
    PCLW_SERVICE_INFO pInfo,
    LW_SERVICE_INFO_MASK mask
    )
{
    DWORD dwError = 0;
    BOOL bLocked = FALSE;
    BOOL bTableLocked = FALSE;
    PLW_SERVICE_INFO pUpdate = NULL;

    LOCK(bLocked, pEntry->pLock);
    /* We must also hold the service table lock to prevent
       concurrent access to pEntry->pInfo by LwSmTableGetEntry */
    LOCK(bTableLocked, gServiceTable.pLock);

    dwError = LwAllocateMemory(sizeof(*pUpdate), OUT_PPVOID(&pUpdate));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(
        mask & LW_SERVICE_INFO_MASK_NAME ? pInfo->pwszName : pEntry->pInfo->pwszName,
        &pUpdate->pwszName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(
        mask & LW_SERVICE_INFO_MASK_PATH ? pInfo->pwszPath : pEntry->pInfo->pwszPath,
        &pUpdate->pwszPath);

    dwError = LwSmCopyString(
        mask & LW_SERVICE_INFO_MASK_DESCRIPTION ? pInfo->pwszDescription : pEntry->pInfo->pwszDescription,
        &pUpdate->pwszDescription);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(
        mask & LW_SERVICE_INFO_MASK_GROUP ? pInfo->pwszGroup : pEntry->pInfo->pwszGroup,
        &pUpdate->pwszGroup);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(
        mask & LW_SERVICE_INFO_MASK_LOG ? pInfo->pDefaultLogTarget : pEntry->pInfo->pDefaultLogTarget,
        &pUpdate->pDefaultLogTarget);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(
        mask & LW_SERVICE_INFO_MASK_ARGS ? pInfo->ppwszArgs : pEntry->pInfo->ppwszArgs,
        &pUpdate->ppwszArgs);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(
        mask & LW_SERVICE_INFO_MASK_ENVIRONMENT ? pInfo->ppwszEnv : pEntry->pInfo->ppwszEnv,
        &pUpdate->ppwszEnv);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(
        mask & LW_SERVICE_INFO_MASK_DEPENDENCIES ? pInfo->ppwszDependencies : pEntry->pInfo->ppwszDependencies,
        &pUpdate->ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    pUpdate->type = mask & LW_SERVICE_INFO_MASK_TYPE ? pInfo->type : pEntry->pInfo->type;
    pUpdate->bAutostart = mask & LW_SERVICE_INFO_MASK_AUTOSTART ? pInfo->bAutostart : pEntry->pInfo->bAutostart;
    pUpdate->dwFdLimit = mask & LW_SERVICE_INFO_MASK_AUTOSTART ? pInfo->dwFdLimit : pEntry->pInfo->dwFdLimit;
    pUpdate->dwCoreSize = mask & LW_SERVICE_INFO_MASK_AUTOSTART ? pInfo->dwCoreSize : pEntry->pInfo->dwCoreSize;
    pUpdate->DefaultLogType = mask & LW_SERVICE_INFO_MASK_LOG ? pInfo->DefaultLogType : pEntry->pInfo->DefaultLogType;
    pUpdate->DefaultLogLevel = mask & LW_SERVICE_INFO_MASK_LOG ? pInfo->DefaultLogLevel : pEntry->pInfo->DefaultLogLevel;

    /* Atomically replace previous info structure */
    LwSmCommonFreeServiceInfo(pEntry->pInfo);
    pEntry->pInfo = pUpdate;
    pUpdate = NULL;

    pEntry->bDirty = TRUE;

cleanup:

    if (pUpdate)
    {
        LwSmCommonFreeServiceInfo(pUpdate);
    }

    UNLOCK(bTableLocked, gServiceTable.pLock);
    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
}

VOID
LwSmTableRetainEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    BOOL bTableLocked = FALSE;

    LOCK(bTableLocked, gServiceTable.pLock);

    ++pEntry->dwRefCount;
    
    UNLOCK(bTableLocked, gServiceTable.pLock);
}

VOID
LwSmTableReleaseEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    BOOL bEntryLocked = FALSE;
    BOOL bTableLocked = FALSE;

    LOCK(bEntryLocked, pEntry->pLock);
    LOCK(bTableLocked, gServiceTable.pLock);

    if (--pEntry->dwRefCount == 0 && !pEntry->bValid)
    {
        UNLOCK(bEntryLocked, pEntry->pLock);
        LwSmTableFreeEntry(pEntry);
    }

    UNLOCK(bTableLocked, gServiceTable.pLock);
    UNLOCK(bEntryLocked, pEntry->pLock);
}

static
VOID
LwSmTableFreeEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    if (pEntry->pVtbl)
    {
        pEntry->pVtbl->pfnDestruct(&pEntry->object);
    }

    if (pEntry->pInfo)
    {
        LwSmCommonFreeServiceInfo(pEntry->pInfo);
    }

    if (pEntry->pLock)
    {
        pthread_mutex_destroy(pEntry->pLock);
    }

    if (pEntry->pEvent)
    {
        pthread_cond_destroy(pEntry->pEvent);
    }

    LwFreeMemory(pEntry);
}

DWORD
LwSmTableStartEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    LW_SERVICE_STATUS status = {.state = LW_SERVICE_STATE_DEAD};
    DWORD dwAttempts = 0;
    PSTR pszServiceName = NULL;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    while (status.state != LW_SERVICE_STATE_RUNNING)
    {
        dwError = LwSmTablePollEntry(pEntry, &status);
        BAIL_ON_ERROR(dwError);

        switch (status.state)
        {
        case LW_SERVICE_STATE_RUNNING:
            pEntry->StartAttempts = 1;
            break;
        case LW_SERVICE_STATE_STOPPED:
        case LW_SERVICE_STATE_DEAD:
            pEntry->StartAttempts = 0;

            if (dwAttempts == 0)
            {
                LW_SAFE_FREE_MEMORY(pszServiceName);

                dwError = LwWc16sToMbs(pEntry->pInfo->pwszName, &pszServiceName);
                BAIL_ON_ERROR(dwError);
                
                SM_LOG_INFO("Starting service: %s", pszServiceName);

                if (pEntry->bDirty)
                {
                    dwError = LwSmTableReconstructEntry(pEntry);
                    BAIL_ON_ERROR(dwError);
                }

                UNLOCK(bLocked, pEntry->pLock);
                dwError = pEntry->pVtbl->pfnStart(&pEntry->object);
                LOCK(bLocked, pEntry->pLock);
                BAIL_ON_ERROR(dwError);
                dwAttempts++;
            }
            else
            {
                dwError = LW_ERROR_SERVICE_UNRESPONSIVE;
                BAIL_ON_ERROR(dwError);
            }
            break;
        case LW_SERVICE_STATE_STARTING:
        case LW_SERVICE_STATE_STOPPING:
            dwError = LwSmTableWaitEntryChanged(pEntry);
            BAIL_ON_ERROR(dwError);
            break;
        case LW_SERVICE_STATE_PAUSED:
            dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
            BAIL_ON_ERROR(dwError);
            break;
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszServiceName);

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableStopEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    LW_SERVICE_STATUS status = {.state = LW_SERVICE_STATE_RUNNING};
    DWORD dwAttempts = 0;
    PSTR pszServiceName = NULL;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    while (status.state != LW_SERVICE_STATE_STOPPED)
    {
        dwError = LwSmTablePollEntry(pEntry, &status);
        BAIL_ON_ERROR(dwError);

        switch (status.state)
        {
        case LW_SERVICE_STATE_RUNNING:
        case LW_SERVICE_STATE_DEAD:
            pEntry->StartAttempts = 0;
            /* A service that is dead should go directly
               to the stop state when requested */
            if (dwAttempts == 0)
            {
                LW_SAFE_FREE_MEMORY(pszServiceName);

                dwError = LwWc16sToMbs(pEntry->pInfo->pwszName, &pszServiceName);
                BAIL_ON_ERROR(dwError);
                
                SM_LOG_INFO("Stopping service: %s", pszServiceName);

                UNLOCK(bLocked, pEntry->pLock);
                dwError = pEntry->pVtbl->pfnStop(&pEntry->object);
                LOCK(bLocked, pEntry->pLock);
                BAIL_ON_ERROR(dwError);
                dwAttempts++;
            }
            else
            {
                dwError = LW_ERROR_SERVICE_UNRESPONSIVE;
                BAIL_ON_ERROR(dwError);
            }
            break;
        case LW_SERVICE_STATE_STOPPED:
            break;
        case LW_SERVICE_STATE_STARTING:
        case LW_SERVICE_STATE_STOPPING:
            dwError = LwSmTableWaitEntryChanged(pEntry);
            BAIL_ON_ERROR(dwError);
            break;
        case LW_SERVICE_STATE_PAUSED:
            dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
            BAIL_ON_ERROR(dwError);
            break;
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszServiceName);

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableRefreshEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    LW_SERVICE_STATUS status = {.state = LW_SERVICE_STATE_DEAD};

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmTablePollEntry(pEntry, &status);
    BAIL_ON_ERROR(dwError);

    switch (status.state)
    {
    case LW_SERVICE_STATE_RUNNING:
        UNLOCK(bLocked, pEntry->pLock);
        dwError = pEntry->pVtbl->pfnRefresh(&pEntry->object);
        LOCK(bLocked, pEntry->pLock);
        BAIL_ON_ERROR(dwError);
        break;
    default:
        break;
    }

cleanup:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableGetEntryStatus(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);
    
    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmTablePollEntry(pEntry, pStatus);
    BAIL_ON_ERROR(dwError);
    
error:
    
    UNLOCK(bLocked, pEntry->pLock);

    return dwError;
}

DWORD
LwSmTableSetEntryLogInfo(
    PSM_TABLE_ENTRY pEntry,
    LW_PCSTR pszFacility,
    LW_SM_LOGGER_TYPE type,
    PCSTR pszTarget
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    if (!pEntry->pVtbl->pfnSetLogInfo)
    {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_ERROR(dwError);
    }

    dwError = pEntry->pVtbl->pfnSetLogInfo(&pEntry->object, pszFacility, type, pszTarget);
    BAIL_ON_ERROR(dwError);

error:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;
}

DWORD
LwSmTableSetEntryLogLevel(
    PSM_TABLE_ENTRY pEntry,
    LW_PCSTR pFacility,
    LW_SM_LOG_LEVEL level
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    if (!pEntry->pVtbl->pfnSetLogLevel)
    {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_ERROR(dwError);
    }

    dwError = pEntry->pVtbl->pfnSetLogLevel(&pEntry->object, pFacility, level);
    BAIL_ON_ERROR(dwError);

error:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;
}

DWORD
LwSmTableGetEntryLogState(
    PSM_TABLE_ENTRY pEntry,
    LW_PCSTR pFacility,
    PLW_SM_LOGGER_TYPE pType,
    LW_PSTR* ppTarget,
    PLW_SM_LOG_LEVEL pLevel
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    if (!pEntry->pVtbl->pfnGetLogState)
    {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_ERROR(dwError);
    }

    dwError = pEntry->pVtbl->pfnGetLogState(&pEntry->object, pFacility, pType, ppTarget, pLevel);
    BAIL_ON_ERROR(dwError);

error:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;
}

DWORD
LwSmTableGetEntryFacilityList(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppFacilities
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    if (!pEntry->pVtbl->pfnGetFacilityList)
    {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_ERROR(dwError);
    }

    dwError = pEntry->pVtbl->pfnGetFacilityList(&pEntry->object, pppFacilities);
    BAIL_ON_ERROR(dwError);

error:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;
}

static
DWORD
LwSmTablePollEntry(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = TRUE;

    UNLOCK(bLocked, pEntry->pLock);
    dwError = pEntry->pVtbl->pfnGetStatus(&pEntry->object, pStatus);
    LOCK(bLocked, pEntry->pLock);
    BAIL_ON_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
LwSmTableStartRecursive(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD error = 0;
    PSM_TABLE_ENTRY pOtherEntry = NULL;
    PWSTR* ppServices = NULL;
    DWORD index = 0;

    /* Start all dependencies */
    error = LwSmTableGetEntryDependencyClosure(pEntry, &ppServices);
    BAIL_ON_ERROR(error);

    for (index = 0; ppServices[index]; index++)
    {
        error = LwSmTableGetEntry(ppServices[index], &pOtherEntry);
        BAIL_ON_ERROR(error);

        error = LwSmTableStartEntry(pOtherEntry);
        BAIL_ON_ERROR(error);

        LwSmTableReleaseEntry(pOtherEntry);
        pOtherEntry = NULL;
    }

    error = LwSmTableStartEntry(pEntry);
    BAIL_ON_ERROR(error);

error:

    if (ppServices)
    {
        LwSmFreeStringList(ppServices);
    }

    if (pOtherEntry)
    {
        LwSmTableReleaseEntry(pOtherEntry);
    }

    return error;
}

static
VOID
LwSmTableWatchdog(
    PVOID pContext
    )
{
    DWORD error = 0;
    PSM_TABLE_ENTRY pEntry = pContext;
    PSM_TABLE_ENTRY pOtherEntry = NULL;
    LW_SERVICE_STATUS status = {0};
    PWSTR* ppServices = NULL;
    DWORD index = 0;

    error = LwSmTableStartRecursive(pEntry);
    BAIL_ON_ERROR(error);

    /* See if any reverse dependencies changed state without announcing it */
    error = LwSmTableGetEntryReverseDependencyClosure(pEntry, &ppServices);
    BAIL_ON_ERROR(error);

    for (index = 0; ppServices[index]; index++)
    {
        error = LwSmTableGetEntry(ppServices[index], &pOtherEntry);
        BAIL_ON_ERROR(error);

        error = LwSmTableGetEntryStatus(pOtherEntry, &status);
        BAIL_ON_ERROR(error);

        if (status.state == LW_SERVICE_STATE_DEAD)
        {
            (void) LwSmTableStartRecursive(pOtherEntry);
        }

        LwSmTableReleaseEntry(pOtherEntry);
        pOtherEntry = NULL;
    }

error:

    if (ppServices)
    {
        LwSmFreeStringList(ppServices);
    }

    if (pOtherEntry)
    {
        LwSmTableReleaseEntry(pOtherEntry);
    }

    LwSmTableReleaseEntry(pEntry);

    return;
}

VOID
LwSmTableNotifyEntryStateChanged(
    PSM_TABLE_ENTRY pEntry,
    LW_SERVICE_STATE state
    )
{
    DWORD error = ERROR_SUCCESS;
    BOOLEAN bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PSM_ENTRY_NOTIFY pNotify = NULL;
    PSTR pServiceName = NULL;
    time_t now = 0;
    
    LOCK(bLocked, pEntry->pLock);

    for (pLink = LwSmLinkBegin(&pEntry->waiters);
         LwSmLinkValid(&pEntry->waiters, pLink);
         pLink = pNext)
    {
        pNext = LwSmLinkNext(pLink);
        pNotify = STRUCT_FROM_MEMBER(pLink, SM_ENTRY_NOTIFY, link);

        LwSmLinkRemove(pLink);

        pNotify->pfnNotifyEntryStateChange(state, pNotify->pData);

        LwFreeMemory(pNotify);
    }

    pthread_cond_broadcast(pEntry->pEvent);

    if (state == LW_SERVICE_STATE_DEAD && gState.bWatchdog)
    {
        now = time(NULL);
        if (now == (time_t) -1)
        {
            error = LwErrnoToWin32Error(errno);
            BAIL_ON_ERROR(error);
        }

        error = LwWc16sToMbs(pEntry->pInfo->pwszName, &pServiceName);
        BAIL_ON_ERROR(error);

        if (pEntry->StartAttempts >= 1 &&
            (now - pEntry->LastRestartPeriod) > RESTART_PERIOD)
        {
            pEntry->StartAttempts = 1;
            pEntry->LastRestartPeriod = now;
        }

        if (pEntry->StartAttempts > 0 && pEntry->StartAttempts < RESTART_LIMIT)
        {
            pEntry->StartAttempts++;
            pEntry->dwRefCount++;

            SM_LOG_WARNING(
                "Restarting dead service: %s (attempt %u/%u)",
                pServiceName,
                (unsigned int) pEntry->StartAttempts,
                (unsigned int) RESTART_LIMIT);


            error = LwNtStatusToWin32Error(LwRtlQueueWorkItem(gpPool, LwSmTableWatchdog, pEntry, 0));
            if (error)
            {
                pEntry->dwRefCount--;
            }
            BAIL_ON_ERROR(error);
        }
        else if (pEntry->StartAttempts >= 1)
        {
            SM_LOG_ERROR(
                "Service died: %s (restarted %u times in %lu seconds)",
                pServiceName,
                (unsigned int) pEntry->StartAttempts,
                (unsigned long) (now - pEntry->LastRestartPeriod));
        }
        else
        {
            SM_LOG_ERROR(
                "Service failed to start: %s",
                pServiceName);
        }
    }

error:

    UNLOCK(bLocked, pEntry->pLock);

    LW_SAFE_FREE_MEMORY(pServiceName);
}

DWORD
LwSmTableWaitEntryChanged(
    PSM_TABLE_ENTRY pEntry
    )
{
    if (!pEntry->bValid)
    {
        return LW_ERROR_INVALID_HANDLE;
    }

    pthread_cond_wait(pEntry->pEvent, pEntry->pLock);

    if (!pEntry->bValid)
    {
        return LW_ERROR_INVALID_HANDLE;
    }

    return 0;
}

DWORD
LwSmTableRegisterEntryNotify(
    PSM_TABLE_ENTRY pEntry,
    LW_SERVICE_STATE currentState,
    VOID (*pfnNotifyEntryStateChange)(LW_SERVICE_STATE state, PVOID pData),
    PVOID pData
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    PSM_ENTRY_NOTIFY pNotify = NULL;
    LW_SERVICE_STATUS status = {0};

    LOCK(bLocked, pEntry->pLock);

    dwError = LwSmTablePollEntry(pEntry, &status);
    BAIL_ON_ERROR(dwError);

    if (status.state != currentState)
    {
        pfnNotifyEntryStateChange(status.state, pData);
    }
    else
    {
        dwError = LwAllocateMemory(sizeof(*pNotify), OUT_PPVOID(&pNotify));
        BAIL_ON_ERROR(dwError);
        
        LwSmLinkInit(&pNotify->link);
        pNotify->pfnNotifyEntryStateChange = pfnNotifyEntryStateChange;
        pNotify->pData = pData;
        
        LwSmLinkInsertBefore(&pEntry->waiters, &pNotify->link);
    }
    
cleanup:
    
    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pNotify);

    goto cleanup;
}

DWORD
LwSmTableUnregisterEntryNotify(
    PSM_TABLE_ENTRY pEntry,
    VOID (*pfnNotifyEntryStateChange)(LW_SERVICE_STATE state, PVOID pData),
    PVOID pData
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PSM_ENTRY_NOTIFY pNotify = NULL;
    
    LOCK(bLocked, pEntry->pLock);

    for (pLink = LwSmLinkBegin(&pEntry->waiters);
         LwSmLinkValid(&pEntry->waiters, pLink);
         pLink = pNext)
    {
        pNext = LwSmLinkNext(pLink);
        pNotify = STRUCT_FROM_MEMBER(pLink, SM_ENTRY_NOTIFY, link);

        if (pNotify->pfnNotifyEntryStateChange == pfnNotifyEntryStateChange &&
            pNotify->pData == pData)
        {
            LwSmLinkRemove(pLink);
            LwFreeMemory(pNotify);
            goto cleanup;
        }
    }

    dwError = LW_ERROR_INVALID_PARAMETER;
    BAIL_ON_ERROR(dwError);

cleanup:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmTableGetEntryDependencyClosureHelper(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    PSM_TABLE_ENTRY pDepEntry = NULL;
    PWSTR pwszDepName = NULL;
    size_t i = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);
    dwError = LwSmCopyServiceInfo(pEntry->pInfo, &pInfo);
    UNLOCK(bLocked, pEntry->pLock);
    BAIL_ON_ERROR(dwError);

    for (i = 0; pInfo->ppwszDependencies[i]; i++)
    {
        dwError = LwSmTableGetEntry(pInfo->ppwszDependencies[i], &pDepEntry);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableGetEntryDependencyClosureHelper(pDepEntry, pppwszServiceList);
        BAIL_ON_ERROR(dwError);

        if (!LwSmStringListContains(*pppwszServiceList, pInfo->ppwszDependencies[i]))
        {
            dwError = LwAllocateWc16String(&pwszDepName,  pInfo->ppwszDependencies[i]);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmStringListAppend(pppwszServiceList, pwszDepName);
            BAIL_ON_ERROR(dwError);

            pwszDepName = NULL;
        }
        
        LwSmTableReleaseEntry(pDepEntry);
        pDepEntry = NULL;
    }

cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszDepName);
    
    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    if (pDepEntry)
    {
        LwSmTableReleaseEntry(pDepEntry);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableGetEntryDependencyClosure(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceList = NULL;

    dwError = LwAllocateMemory(sizeof(*ppwszServiceList) * 1, OUT_PPVOID(&ppwszServiceList));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmTableGetEntryDependencyClosureHelper(pEntry, &ppwszServiceList);
    BAIL_ON_ERROR(dwError);

    *pppwszServiceList = ppwszServiceList;

cleanup:

    return dwError;

error:

    *pppwszServiceList = NULL;

    if (ppwszServiceList)
    {
        LwSmFreeStringList(ppwszServiceList);
    }

    goto cleanup;
}

static
DWORD
LwSmTableGetEntryReverseDependencyClosureHelper(
    PSM_TABLE_ENTRY pEntry,
    PWSTR* ppwszAllServices,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    PLW_SERVICE_INFO pDepInfo = NULL;
    size_t i = 0;
    PSM_TABLE_ENTRY pDepEntry = NULL;
    PWSTR pwszDepName = NULL;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);
    dwError = LwSmCopyServiceInfo(pEntry->pInfo, &pInfo);
    UNLOCK(bLocked, pEntry->pLock);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszAllServices[i]; i++)
    {
        dwError = LwSmTableGetEntry(ppwszAllServices[i], &pDepEntry);
        BAIL_ON_ERROR(dwError);

        LOCK(bLocked, pEntry->pLock);
        dwError = LwSmCopyServiceInfo(pDepEntry->pInfo, &pDepInfo);
        UNLOCK(bLocked, pEntry->pLock);
        BAIL_ON_ERROR(dwError);

        if (LwSmStringListContains(pDepInfo->ppwszDependencies, pInfo->pwszName))
        {
            dwError = LwSmTableGetEntryReverseDependencyClosureHelper(
                pDepEntry,
                ppwszAllServices,
                pppwszServiceList);
            BAIL_ON_ERROR(dwError);
            
            dwError = LwAllocateWc16String(&pwszDepName, pDepInfo->pwszName);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmStringListAppend(pppwszServiceList, pwszDepName);
            BAIL_ON_ERROR(dwError);

            pwszDepName = NULL;
        }

        LwSmCommonFreeServiceInfo(pDepInfo);
        pDepInfo = NULL;

        LwSmTableReleaseEntry(pDepEntry);
        pDepEntry = NULL;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszDepName);

    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    if (pDepInfo)
    {
        LwSmCommonFreeServiceInfo(pDepInfo);
    }

    if (pDepEntry)
    {
        LwSmTableReleaseEntry(pDepEntry);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableGetEntryReverseDependencyClosure(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceList = NULL;
    PWSTR* ppwszAllServices = NULL;

    dwError = LwAllocateMemory(sizeof(*ppwszServiceList) * 1, OUT_PPVOID(&ppwszServiceList));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmTableEnumerateEntries(&ppwszAllServices);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmTableGetEntryReverseDependencyClosureHelper(
        pEntry,
        ppwszAllServices,
        &ppwszServiceList);
    BAIL_ON_ERROR(dwError);

    *pppwszServiceList = ppwszServiceList;

cleanup:

    if (ppwszAllServices)
    {
        LwSmFreeStringList(ppwszAllServices);
    }

    return dwError;

error:

    *pppwszServiceList = NULL;

    if (ppwszServiceList)
    {
        LwSmFreeStringList(ppwszServiceList);
    }

    goto cleanup;
}

PVOID
LwSmGetServiceObjectData(
    PLW_SERVICE_OBJECT pObject
    )
{
    return pObject->pData;
}

VOID
LwSmRetainServiceObject(
    PLW_SERVICE_OBJECT pObject
    )
{
    PSM_TABLE_ENTRY pEntry = STRUCT_FROM_MEMBER(pObject, SM_TABLE_ENTRY, object);

    LwSmTableRetainEntry(pEntry);
}

VOID
LwSmReleaseServiceObject(
    PLW_SERVICE_OBJECT pObject
    )
{
    PSM_TABLE_ENTRY pEntry = STRUCT_FROM_MEMBER(pObject, SM_TABLE_ENTRY, object);

    LwSmTableReleaseEntry(pEntry);
}

VOID
LwSmNotifyServiceObjectStateChange(
    PLW_SERVICE_OBJECT pObject,
    LW_SERVICE_STATE newState
    )
{
    PSM_TABLE_ENTRY pEntry = STRUCT_FROM_MEMBER(pObject, SM_TABLE_ENTRY, object);

    return LwSmTableNotifyEntryStateChanged(pEntry, newState);
}

DWORD
LwSmTableInit(
    VOID
    )
{
    DWORD error = ERROR_SUCCESS;

    return error;
}

VOID
LwSmTableShutdown(
    VOID
    )
{
    BOOL bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;

    LOCK(bLocked, gServiceTable.pLock);

    for (pLink = LwSmLinkBegin(&gServiceTable.entries);
         LwSmLinkValid(&gServiceTable.entries, pLink);
         pLink = pNext)
    {
        pNext = LwSmLinkNext(pLink);
        pEntry = STRUCT_FROM_MEMBER(pLink, SM_TABLE_ENTRY, link);

        LwSmTableFreeEntry(pEntry);
    }

    UNLOCK(bLocked, gServiceTable.pLock);
    pthread_mutex_destroy(gServiceTable.pLock);
}
