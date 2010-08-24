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
 *        task.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Server side Task Handlers
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
int
LwTaskSrvTreeCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
LwTaskSrvTreeRelease(
    PVOID pTask
    );

static
DWORD
LwTaskSrvCreateInternal(
    PCSTR         pszTaskName,
    DWORD         dwTaskId,
    PLW_TASK_ARG* ppArgArray,
    PDWORD        pdwNumArgs,
    PLW_SRV_TASK* ppTask
    );

static
VOID
LwTaskSrvFree(
    PLW_SRV_TASK pTask
    );

DWORD
LwTaskSrvInit(
    VOID
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLW_TASK_DB_CONTEXT pDbContext = NULL;
    PLW_SRV_DB_TASK pTaskArray = NULL;
    DWORD           dwNumTasks = 0;
    DWORD           iTask = 0;

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskSrvGlobals.mutex);

    dwError = LwNtStatusToWin32Error(
                    LwRtlRBTreeCreate(
                        &LwTaskSrvTreeCompare,
                        NULL,
                        &LwTaskSrvTreeRelease,
                        &gLwTaskSrvGlobals.pTaskCollection));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbOpen(&pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbGetTasks(pDbContext, &pTaskArray, &dwNumTasks);
    BAIL_ON_LW_TASK_ERROR(dwError);

    for (; iTask < dwNumTasks; iTask++)
    {
        PLW_SRV_DB_TASK pTask = &pTaskArray[iTask];

        dwError = LwTaskSrvCreateInternal(
                    pTask->pszTaskName,
                    pTask->dwTaskId,
                    &pTask->pArgArray,
                    &pTask->dwNumArgs,
                    NULL);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

cleanup:

    if (pTaskArray)
    {
        LwTaskDbFreeTaskArray(pTaskArray, dwNumTasks);
    }

    if (pDbContext)
    {
        LwTaskDbClose(pDbContext);
    }

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);

    return dwError;

error:

    goto cleanup;
}

static
int
LwTaskSrvTreeCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    uuid_t* pUUID1 = (uuid_t*)pKey1;
    uuid_t* pUUID2 = (uuid_t*)pKey2;

    return uuid_compare(*pUUID1, *pUUID2);
}

static
VOID
LwTaskSrvTreeRelease(
    PVOID pTask
    )
{
    LwTaskSrvRelease((PLW_SRV_TASK)pTask);
}

DWORD
LwTaskSrvGetTypes(
    PDWORD* ppdwTaskTypeArray,
    PDWORD  pdwNumTypes
    )
{
    DWORD  dwError = 0;
    PDWORD pdwTaskTypeArray = NULL;
    DWORD  dwNumTypes = 0;
    PLW_TASK_DB_CONTEXT pDbContext = NULL;

    dwError = LwTaskDbOpen(&pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbGetTypes(pDbContext, &pdwTaskTypeArray, &dwNumTypes);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppdwTaskTypeArray = pdwTaskTypeArray;
    *pdwNumTypes = dwNumTypes;

cleanup:

    if (pDbContext)
    {
        LwTaskDbClose(pDbContext);
    }

    return dwError;

error:

    *ppdwTaskTypeArray = NULL;
    *pdwNumTypes = 0;

    LW_SAFE_FREE_MEMORY(pdwTaskTypeArray);

    goto cleanup;
}

DWORD
LwTaskSrvStart(
    PCSTR        pszTaskId,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
LwTaskSrvStop(
    PCSTR pszTaskId
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
LwTaskSrvDelete(
    PCSTR pszTaskId
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
LwTaskSrvGetStatus(
    PCSTR           pszTaskId,
    PLW_TASK_STATUS pTaskStatus
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
LwTaskSrvCreate(
    LW_TASK_TYPE taskType,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs,
    PSTR*        ppszTaskId
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
LwTaskSrvGetSchema(
    LW_TASK_TYPE       taskType,
    PLW_TASK_ARG_INFO* ppArgInfoArray,
    PDWORD             pdwNumArgInfos
    )
{
    DWORD  dwError = 0;
    PLW_TASK_ARG_INFO pArgInfoArray = NULL;
    DWORD  dwNumArgInfos = 0;
    PLW_TASK_DB_CONTEXT pDbContext = NULL;

    dwError = LwTaskDbOpen(&pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbGetSchema(
                    pDbContext,
                    taskType,
                    &pArgInfoArray,
                    &dwNumArgInfos);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppArgInfoArray = pArgInfoArray;
    *pdwNumArgInfos = dwNumArgInfos;

cleanup:

    if (pDbContext)
    {
        LwTaskDbClose(pDbContext);
    }

    return dwError;

error:

    *ppArgInfoArray = NULL;
    *pdwNumArgInfos = 0;

    if (pArgInfoArray)
    {
        LwTaskFreeArgInfoArray(pArgInfoArray, dwNumArgInfos);
    }

    goto cleanup;
}

DWORD
LwTaskSrvEnum(
    LW_TASK_TYPE   taskType,
    PDWORD         pdwTotalTaskInfos,
    PDWORD         pdwNumTaskInfos,
    PLW_TASK_INFO* ppTaskInfoArray,
    PDWORD         pdwResume
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static
DWORD
LwTaskSrvCreateInternal(
    PCSTR         pszTaskName,
    DWORD         dwTaskId,
    PLW_TASK_ARG* ppArgArray,
    PDWORD        pdwNumArgs,
    PLW_SRV_TASK* ppTask
    )
{
    DWORD dwError = 0;
    PLW_SRV_TASK pTask = NULL;

    dwError = LwAllocateMemory(sizeof(LW_SRV_TASK), (PVOID*)&pTask);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pTask->refCount = 1;

    pthread_mutex_init(&pTask->mutex, NULL);
    pTask->pMutex = &pTask->mutex;

    if (uuid_parse((PSTR)pszTaskName, pTask->uuid) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    pTask->dwTaskId = dwTaskId;

    pTask->pArgArray = *ppArgArray;
    *ppArgArray      = NULL;
    pTask->dwNumArgs = *pdwNumArgs;
    *pdwNumArgs      = 0;

    if (ppTask)
    {
        *ppTask = pTask;
        pTask = NULL;
    }

cleanup:

    if (pTask)
    {
        LwTaskSrvRelease(pTask);
    }

    return dwError;

error:

    if (ppTask)
    {
        *ppTask = NULL;
    }

    goto cleanup;
}

VOID
LwTaskSrvRelease(
    PLW_SRV_TASK pTask
    )
{
    if (InterlockedDecrement(&pTask->refCount) == 0)
    {
        LwTaskSrvFree(pTask);
    }
}

static
VOID
LwTaskSrvFree(
    PLW_SRV_TASK pTask
    )
{
    if (pTask->pMutex)
    {
        pthread_mutex_destroy(&pTask->mutex);
        pTask->pMutex = NULL;
    }

    if (pTask->pArgArray)
    {
        LwTaskFreeArgArray(pTask->pArgArray, pTask->dwNumArgs);
    }

    LwFreeMemory(pTask);
}

VOID
LwTaskSrvShutdown(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskSrvGlobals.mutex);

    if (gLwTaskSrvGlobals.pTaskCollection)
    {
        LwRtlRBTreeFree(gLwTaskSrvGlobals.pTaskCollection);
    }

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);
}
