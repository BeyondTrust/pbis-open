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
DWORD
LwTaskSrvTreeFind(
    PCSTR         pszTaskName,
    PLW_SRV_TASK* ppTask
    );

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
LwTaskBuildExecArgs(
    PLW_SRV_TASK  pTask,
    PLW_TASK_ARG  pArgArray,
    DWORD         dwNumArgs,
    PLW_TASK_ARG* ppArgArray,
    PDWORD        pdwNumArgs
    );

static
DWORD
LwTaskSrvCreateMigrateTask(
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs,
    PSTR*        ppszTaskname
    );

static
DWORD
LwTaskBuildMigrateArgArray(
    PLW_TASK_ARG_INFO pArgInfoArray,
    DWORD             dwNumArgInfos,
    PLW_TASK_ARG      pArgArray,
    DWORD             dwNumArgs,
    PLW_TASK_ARG*     ppArgArray,
    PDWORD            pdwNumArgs
    );

static
NTSTATUS
LwTaskSrvCountCandidateTasks(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
LwTaskSrvEnumCandidateTasks(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
DWORD
LwTaskSrvCreateInternal(
    PCSTR         pszTaskName,
    DWORD         dwTaskId,
    LW_TASK_TYPE  taskType,
    PLW_TASK_ARG* ppArgArray,
    PDWORD        pdwNumArgs,
    PLW_SRV_TASK* ppTask
    );

static
VOID
LwTaskSrvFree(
    PLW_SRV_TASK pTask
    );

static
VOID
LwTaskUnblockOneWorker(
    IN PLW_TASK_PROD_CONS_QUEUE pWorkQueue
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
    PLW_SRV_TASK    pSrvTask   = NULL;
    DWORD           dwNumTasks = 0;
    DWORD           iTask = 0;
    DWORD           iWorker = 0;

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

        if (pSrvTask)
        {
            LwTaskSrvRelease(pSrvTask);
            pSrvTask = NULL;
        }

        dwError = LwTaskSrvCreateInternal(
                    pTask->pszTaskName,
                    pTask->dwTaskId,
                    pTask->taskType,
                    &pTask->pArgArray,
                    &pTask->dwNumArgs,
                    &pSrvTask);
        BAIL_ON_LW_TASK_ERROR(dwError);

        dwError = LwNtStatusToWin32Error(
                        LwRtlRBTreeAdd(
                                gLwTaskSrvGlobals.pTaskCollection,
                                &pSrvTask->uuid,
                                pSrvTask));
        BAIL_ON_LW_TASK_ERROR(dwError);

        pSrvTask = NULL;
    }

    dwError = LwAllocateMemory(
                    gLwTaskSrvGlobals.dwNumWorkers * sizeof(LW_TASK_SRV_WORKER),
                    (PVOID*)&gLwTaskSrvGlobals.pWorkerArray);
    BAIL_ON_LW_TASK_ERROR(dwError);

    for (; iWorker < gLwTaskSrvGlobals.dwNumWorkers; iWorker++)
    {
        PLW_TASK_SRV_WORKER pWorker = &gLwTaskSrvGlobals.pWorkerArray[iWorker];

        pWorker->dwWorkerId = iWorker + 1;

        dwError = LwTaskWorkerInit(pWorker);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

cleanup:

    if (pSrvTask)
    {
        LwTaskSrvRelease(pSrvTask);
    }

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
DWORD
LwTaskSrvTreeFind(
    PCSTR         pszTaskName,
    PLW_SRV_TASK* ppTask
    )
{
    DWORD        dwError = 0;
    BOOLEAN      bInLock = FALSE;
    PLW_SRV_TASK pTask   = NULL;
    uuid_t       uuid;

    if (uuid_parse((PSTR)pszTaskName, uuid) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, &gLwTaskSrvGlobals.mutex);

    dwError = LwNtStatusToWin32Error(
                    LwRtlRBTreeFind(
                            gLwTaskSrvGlobals.pTaskCollection,
                            &uuid,
                            (PVOID*)&pTask));
    BAIL_ON_LW_TASK_ERROR(dwError);

    InterlockedIncrement(&pTask->refCount);

    *ppTask = pTask;

cleanup:

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);

    return dwError;

error:

    *ppTask = NULL;

    if (pTask)
    {
        LwTaskSrvRelease(pTask);
    }

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
    PCSTR        pszTaskname,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    )
{
    DWORD dwError = 0;
    PLW_SRV_TASK pTask = NULL;
    BOOLEAN bInLock = FALSE;
    PLW_TASK_CONTEXT pContext = NULL;
    PLW_TASK_ARG pArgArray2 = NULL;
    DWORD        dwNumArgs2 = 0;

    dwError = LwTaskSrvTreeFind(pszTaskname, &pTask);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskBuildExecArgs(
                    pTask,
                    pArgArray,
                    dwNumArgs,
                    &pArgArray2,
                    &dwNumArgs2);
    BAIL_ON_LW_TASK_ERROR(dwError);

    LW_TASK_LOCK_MUTEX(bInLock, &pTask->mutex);

    if (pTask->execStatus == LW_TASK_EXECUTION_STATUS_RUNNING)
    {
        dwError = ERROR_PROCESS_IN_JOB;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    pTask->bCancel = FALSE;

    LW_TASK_UNLOCK_MUTEX(bInLock, &pTask->mutex);

    dwError = LwTaskCreateContext(
                    pTask,
                    &pArgArray2,
                    &dwNumArgs2,
                    &pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskProdConsEnqueue(&gLwTaskSrvGlobals.workQueue, pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pContext = NULL;

cleanup:

    if (pTask)
    {
        LW_TASK_UNLOCK_MUTEX(bInLock, &pTask->mutex);

        LwTaskSrvRelease(pTask);
    }

    if (pContext)
    {
        LwTaskReleaseContext(pContext);
    }

    return dwError;

error:

    if (pArgArray2)
    {
        LwTaskFreeArgArray(pArgArray2, dwNumArgs2);
    }

    goto cleanup;
}

static
DWORD
LwTaskBuildExecArgs(
    PLW_SRV_TASK  pTask,
    PLW_TASK_ARG  pArgArray,
    DWORD         dwNumArgs,
    PLW_TASK_ARG* ppArgArray,
    PDWORD        pdwNumArgs
    )
{
    DWORD dwError = 0;
    PLW_TASK_DB_CONTEXT pDbContext = NULL;
    PLW_TASK_ARG_INFO pArgInfoArray = NULL;
    DWORD             dwNumArgInfos = 0;
    PLW_TASK_ARG      pExecArgArray = NULL;
    DWORD             dwNumExecArgs = 0;
    DWORD             iArgInfo = 0;
    DWORD             iArg = 0;

    dwError = LwTaskDbOpen(&pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbGetSchema(
                    pDbContext,
                    pTask->taskType,
                    &pArgInfoArray,
                    &dwNumArgInfos);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_ARG) * dwNumArgInfos,
                    (PVOID*)&pExecArgArray);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwNumExecArgs = dwNumArgInfos;

    for (; iArgInfo < dwNumArgInfos; iArgInfo++)
    {
        PLW_TASK_ARG_INFO pArgInfo = &pArgInfoArray[iArgInfo];
        PLW_TASK_ARG pCandidate = NULL;

        pCandidate = LwTaskFindArg(
                            pArgInfo->pszArgName,
                            pArgInfo->argType,
                            pTask->pArgArray,
                            pTask->dwNumArgs);

        if (!pCandidate)
        {
            pCandidate = LwTaskFindArg(
                                pArgInfo->pszArgName,
                                pArgInfo->argType,
                                pArgArray,
                                dwNumArgs);
        }

        if (pCandidate)
        {
            PLW_TASK_ARG pExecArg = &pExecArgArray[iArg++];

            dwError = LwAllocateString(
                            pArgInfo->pszArgName,
                            &pExecArg->pszArgName);
            BAIL_ON_LW_TASK_ERROR(dwError);

            pExecArg->dwArgType = pArgInfo->argType;

            if (pCandidate->pszArgValue)
            {
                dwError = LwAllocateString(
                                pCandidate->pszArgValue,
                                &pExecArg->pszArgValue);
                BAIL_ON_LW_TASK_ERROR(dwError);
            }
        }
        else if (LwIsSetFlag(pArgInfo->dwFlags, LW_TASK_ARG_FLAG_MANDATORY))
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }
    }

    if (!iArg)
    {
        if (pExecArgArray)
        {
            LwTaskFreeArgArray(pExecArgArray, dwNumExecArgs);
            pExecArgArray = NULL;
            dwNumExecArgs = 0;
        }
    }
    else if (iArg < dwNumExecArgs)
    {
        PLW_TASK_ARG pArgArrayNew = NULL;

        dwError = LwAllocateMemory(
                        sizeof(LW_TASK_ARG) * iArg,
                        (PVOID*)&pArgArrayNew);
        BAIL_ON_LW_TASK_ERROR(dwError);

        memcpy(pArgArrayNew, pExecArgArray, sizeof(LW_TASK_ARG) * iArg);

        LwFreeMemory(pExecArgArray);

        pExecArgArray = pArgArrayNew;
        dwNumExecArgs = iArg;
    }

    *ppArgArray = pExecArgArray;
    *pdwNumArgs = dwNumExecArgs;

cleanup:

    if (pDbContext)
    {
        LwTaskDbClose(pDbContext);
    }

    if (pArgInfoArray)
    {
        LwTaskFreeArgInfoArray(pArgInfoArray, dwNumArgInfos);
    }

    return dwError;

error:

    *ppArgArray = NULL;
    *pdwNumArgs = 0;

    if (pExecArgArray)
    {
        LwTaskFreeArgArray(pExecArgArray, dwNumExecArgs);
    }

    goto cleanup;
}

DWORD
LwTaskSrvStop(
    PCSTR pszTaskname
    )
{
    DWORD dwError = 0;
    PLW_SRV_TASK pTask = NULL;
    BOOLEAN bInLock = FALSE;

    dwError = LwTaskSrvTreeFind(pszTaskname, &pTask);
    BAIL_ON_LW_TASK_ERROR(dwError);

    LW_TASK_LOCK_MUTEX(bInLock, &pTask->mutex);

    // TODO : Should we wait for the task to stop executing?
    if (pTask->execStatus == LW_TASK_EXECUTION_STATUS_RUNNING)
    {
        pTask->bCancel = TRUE;
    }

cleanup:

    if (pTask)
    {
        LW_TASK_UNLOCK_MUTEX(bInLock, &pTask->mutex);

        LwTaskSrvRelease(pTask);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskSrvDelete(
    PCSTR pszTaskname
    )
{
    DWORD dwError = 0;
    PLW_SRV_TASK pTask = NULL;
    PLW_TASK_DB_CONTEXT pDbContext = NULL;
    BOOLEAN bInLock = FALSE;

    dwError = LwTaskSrvTreeFind(pszTaskname, &pTask);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbOpen(&pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskSrvGlobals.mutex);

    dwError = LwTaskDbDeleteTask(pDbContext, pTask->dwTaskId);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                    LwRtlRBTreeRemove(
                            gLwTaskSrvGlobals.pTaskCollection,
                            &pTask->uuid));
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);

    if (pDbContext)
    {
        LwTaskDbClose(pDbContext);
    }

    if (pTask)
    {
        LwTaskSrvRelease(pTask);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskSrvGetStatus(
    PCSTR           pszTaskname,
    PLW_TASK_STATUS pTaskStatus
    )
{
    DWORD dwError = 0;
    PLW_SRV_TASK pTask = NULL;
    BOOLEAN bInLock = FALSE;

    dwError = LwTaskSrvTreeFind(pszTaskname, &pTask);
    BAIL_ON_LW_TASK_ERROR(dwError);

    LW_TASK_LOCK_MUTEX(bInLock, &pTask->mutex);

    pTaskStatus->dwError = pTask->dwError;
    pTaskStatus->dwPercentComplete = pTask->dwPercentComplete;
    pTaskStatus->startTime = pTask->startTime;
    pTaskStatus->endTime   = pTask->endTime;

cleanup:

    LW_TASK_UNLOCK_MUTEX(bInLock, &pTask->mutex);

    if (pTask)
    {
        LwTaskSrvRelease(pTask);
    }

    return dwError;

error:

    memset(pTaskStatus, 0, sizeof(*pTaskStatus));

    goto cleanup;
}

DWORD
LwTaskSrvCreate(
    LW_TASK_TYPE taskType,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs,
    PSTR*        ppszTaskname
    )
{
    DWORD dwError = 0;
    PSTR  pszTaskname = NULL;

    switch (taskType)
    {
        case LW_TASK_TYPE_MIGRATE:

            dwError = LwTaskSrvCreateMigrateTask(
                                pArgArray,
                                dwNumArgs,
                                &pszTaskname);

            break;

        default:

            dwError = ERROR_NOT_SUPPORTED;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppszTaskname = pszTaskname;

cleanup:

    return dwError;

error:

    *ppszTaskname = NULL;

    if (pszTaskname)
    {
        LwFreeMemory(pszTaskname);
    }

    goto cleanup;
}

static
DWORD
LwTaskSrvCreateMigrateTask(
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs,
    PSTR*        ppszTaskname
    )
{
    DWORD dwError = 0;
    PLW_TASK_DB_CONTEXT pDbContext = NULL;
    PLW_TASK_ARG_INFO   pArgInfoArray = NULL;
    DWORD               dwNumArgInfos = 0;
    uuid_t              uuid;
    CHAR                szUUID[37];
    DWORD               dwTaskId = 0;
    DWORD               iArg = 0;
    PLW_SRV_TASK        pTask = NULL;
    PLW_TASK_ARG        pArgArrayLocal = NULL;
    DWORD               dwNumArgsLocal = 0;
    BOOLEAN             bInLock = FALSE;
    BOOLEAN             bInTransaction = FALSE;
    PSTR                pszTaskname = NULL;

    dwError = LwTaskDbOpen(&pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbGetSchema(
                    pDbContext,
                    LW_TASK_TYPE_MIGRATE,
                    &pArgInfoArray,
                    &dwNumArgInfos);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskBuildMigrateArgArray(
                    pArgInfoArray,
                    dwNumArgInfos,
                    pArgArray,
                    dwNumArgs,
                    &pArgArrayLocal,
                    &dwNumArgsLocal);
    BAIL_ON_LW_TASK_ERROR(dwError);

    memset(szUUID, 0, sizeof(szUUID));

    uuid_generate(uuid);
    uuid_unparse(uuid, szUUID);

    dwError = LwTaskDbBeginTransaction(pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    bInTransaction = TRUE;

    dwError = LwTaskDbCreateTask(
                    pDbContext,
                    &szUUID[0],
                    LW_TASK_TYPE_MIGRATE,
                    &dwTaskId);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvCreateInternal(
                    &szUUID[0],
                    dwTaskId,
                    LW_TASK_TYPE_MIGRATE,
                    &pArgArrayLocal,
                    &dwNumArgsLocal,
                    &pTask);
    BAIL_ON_LW_TASK_ERROR(dwError);

    for (; iArg < pTask->dwNumArgs; iArg++)
    {
        PLW_TASK_ARG pArg = &pTask->pArgArray[iArg];

        dwError = LwTaskDbCreateTaskArg(
                        pDbContext,
                        pTask->dwTaskId,
                        pArg->pszArgName,
                        pArg->pszArgValue,
                        pArg->dwArgType);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskSrvGlobals.mutex);

    dwError = LwNtStatusToWin32Error(
                    LwRtlRBTreeAdd(
                            gLwTaskSrvGlobals.pTaskCollection,
                            &pTask->uuid,
                            pTask));
    BAIL_ON_LW_TASK_ERROR(dwError);

    pTask = NULL;

    dwError = LwAllocateString(szUUID, &pszTaskname);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbCommitTransaction(pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppszTaskname = pszTaskname;

cleanup:

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);

    if (pTask)
    {
        LwTaskSrvRelease(pTask);
    }

    if (pArgInfoArray)
    {
        LwTaskFreeArgInfoArray(pArgInfoArray, dwNumArgInfos);
    }

    if (pArgArrayLocal)
    {
        LwTaskFreeArgArray(pArgArrayLocal, dwNumArgsLocal);
    }

    if (pDbContext)
    {
        LwTaskDbClose(pDbContext);
    }

    return dwError;

error:

    if (bInTransaction)
    {
        LwTaskDbRollbackTransaction(pDbContext);
    }

    *ppszTaskname = NULL;

    LW_SAFE_FREE_MEMORY(pszTaskname);

    goto cleanup;
}

static
DWORD
LwTaskBuildMigrateArgArray(
    PLW_TASK_ARG_INFO pArgInfoArray,
    DWORD             dwNumArgInfos,
    PLW_TASK_ARG      pArgArray,
    DWORD             dwNumArgs,
    PLW_TASK_ARG*     ppArgArray,
    PDWORD            pdwNumArgs
    )
{
    DWORD        dwError    = 0;
    PLW_TASK_ARG pArgArray2 = NULL;
    DWORD        dwNumArgs2 = 0;
    DWORD        iArg2      = 0;
    DWORD        iArgInfo   = 0;

    for(; iArgInfo < dwNumArgInfos; iArgInfo++)
    {
        PLW_TASK_ARG_INFO pArgInfo = &pArgInfoArray[iArgInfo];

        if (LwIsSetFlag(pArgInfo->dwFlags,LW_TASK_ARG_FLAG_MANDATORY) &&
            LwIsSetFlag(pArgInfo->dwFlags, LW_TASK_ARG_FLAG_PERSIST))
        {
            PLW_TASK_ARG pArg = LwTaskFindArg(
                                    pArgInfo->pszArgName,
                                    pArgInfo->argType,
                                    pArgArray,
                                    dwNumArgs);

            // TODO: Check for empty string also?
            if (!pArg || !pArg->pszArgValue)
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_LW_TASK_ERROR(dwError);
            }

            dwNumArgs2++;
        }
        else if (LwIsSetFlag(pArgInfo->dwFlags, LW_TASK_ARG_FLAG_PERSIST))
        {
        	PLW_TASK_ARG pArg = LwTaskFindArg(
									pArgInfo->pszArgName,
									pArgInfo->argType,
									pArgArray,
									dwNumArgs);
        	if (pArg)
        	{
        		dwNumArgs2++;
        	}
        }
    }

    if (dwNumArgs2)
    {
        dwError = LwAllocateMemory(
                        sizeof(LW_TASK_ARG) * dwNumArgs2,
                        (PVOID*)&pArgArray2);
        BAIL_ON_LW_TASK_ERROR(dwError);

        for(iArgInfo = 0; iArgInfo < dwNumArgInfos; iArgInfo++)
        {
            PLW_TASK_ARG_INFO pArgInfo = &pArgInfoArray[iArgInfo];

            if (LwIsSetFlag(pArgInfo->dwFlags,LW_TASK_ARG_FLAG_MANDATORY) &&
                LwIsSetFlag(pArgInfo->dwFlags, LW_TASK_ARG_FLAG_PERSIST))
            {
                PLW_TASK_ARG pArg2 = &pArgArray2[iArg2++];

                PLW_TASK_ARG pArg = LwTaskFindArg(
                                        pArgInfo->pszArgName,
                                        pArgInfo->argType,
                                        pArgArray,
                                        dwNumArgs);

                dwError = LwAllocateString(
                                    pArg->pszArgName,
                                    &pArg2->pszArgName);
                BAIL_ON_LW_TASK_ERROR(dwError);

                if (pArg->pszArgValue)
                {
					dwError = LwAllocateString(
										pArg->pszArgValue,
										&pArg2->pszArgValue);
					BAIL_ON_LW_TASK_ERROR(dwError);
                }

                pArg2->dwArgType = pArg->dwArgType;
            }
            else if (LwIsSetFlag(pArgInfo->dwFlags, LW_TASK_ARG_FLAG_PERSIST))
            {
            	PLW_TASK_ARG pArg2 = &pArgArray2[iArg2++];

            	PLW_TASK_ARG pArg = LwTaskFindArg(
    									pArgInfo->pszArgName,
    									pArgInfo->argType,
    									pArgArray,
    									dwNumArgs);
            	if (pArg)
            	{
                    dwError = LwAllocateString(
                                        pArg->pszArgName,
                                        &pArg2->pszArgName);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    if (pArg->pszArgValue)
                    {
    					dwError = LwAllocateString(
    										pArg->pszArgValue,
    										&pArg2->pszArgValue);
    					BAIL_ON_LW_TASK_ERROR(dwError);
                    }

                    pArg2->dwArgType = pArg->dwArgType;
            	}
            }
        }
    }

    *ppArgArray = pArgArray2;
    *pdwNumArgs = dwNumArgs2;

cleanup:

    return dwError;

error:

    *ppArgArray = NULL;
    *pdwNumArgs = 0;

    if (pArgArray2)
    {
        LwTaskFreeArgArray(pArgArray2, dwNumArgs2);
    }

    goto cleanup;
}

PLW_TASK_ARG
LwTaskFindArg(
    PCSTR        pszArgName,
    DWORD        dwArgType,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    )
{
    DWORD iArg = 0;
    PLW_TASK_ARG pCandidate = NULL;

    for (; iArg < dwNumArgs; iArg++)
    {
        PLW_TASK_ARG pArg = &pArgArray[iArg];

        if (    !strcmp(pArg->pszArgName, pszArgName) &&
                (dwArgType == pArg->dwArgType))
        {
            pCandidate = pArg;
            break;
        }
    }

    return pCandidate;
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
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    LW_SRV_TASK_ENUM_QUERY taskEnumQuery =
    {
        .taskType       = taskType,
        .dwTotalEntries = 0,
        .pTaskInfoArray = NULL,
        .dwNumTaskInfos = 0,
        .iInfo          = 0,
        .dwResume       = *pdwResume
    };

    LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, &gLwTaskSrvGlobals.mutex);

    // TODO: Implement resume support

    dwError = LwNtStatusToWin32Error(
                    LwRtlRBTreeTraverse(
                            gLwTaskSrvGlobals.pTaskCollection,
                            LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                            &LwTaskSrvCountCandidateTasks,
                            &taskEnumQuery));
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!taskEnumQuery.dwTotalEntries)
    {
        goto done;
    }

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_INFO) * taskEnumQuery.dwTotalEntries,
                    (PVOID*)&taskEnumQuery.pTaskInfoArray);
    BAIL_ON_LW_TASK_ERROR(dwError);

    taskEnumQuery.dwNumTaskInfos = taskEnumQuery.dwTotalEntries;

    dwError = LwNtStatusToWin32Error(
                    LwRtlRBTreeTraverse(
                            gLwTaskSrvGlobals.pTaskCollection,
                            LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                            &LwTaskSrvEnumCandidateTasks,
                            &taskEnumQuery));
    BAIL_ON_LW_TASK_ERROR(dwError);

done:

    *ppTaskInfoArray = taskEnumQuery.pTaskInfoArray;
    *pdwNumTaskInfos = taskEnumQuery.dwNumTaskInfos;
    *pdwTotalTaskInfos = taskEnumQuery.dwTotalEntries;
    *pdwResume         = taskEnumQuery.dwResume;

cleanup:

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);

    return dwError;

error:

    *pdwNumTaskInfos = 0;
    *ppTaskInfoArray = NULL;
    *pdwTotalTaskInfos = 0;

    if (taskEnumQuery.pTaskInfoArray)
    {
        LwTaskFreeTaskInfoArray(
                taskEnumQuery.pTaskInfoArray,
                taskEnumQuery.dwNumTaskInfos);
    }

    goto cleanup;
}

static
NTSTATUS
LwTaskSrvCountCandidateTasks(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLW_SRV_TASK pTask = (PLW_SRV_TASK)pData;
    PLW_SRV_TASK_ENUM_QUERY pTaskEnumQuery =
                                    (PLW_SRV_TASK_ENUM_QUERY)pUserData;
    BOOLEAN bContinue = TRUE;

    if (pTaskEnumQuery->taskType == pTask->taskType)
    {
        if (pTaskEnumQuery->dwTotalEntries == UINT32_MAX)
        {
            bContinue = FALSE;
        }
        else
        {
            pTaskEnumQuery->dwTotalEntries++;
        }
    }

    *pbContinue = bContinue;

    return ntStatus;
}

static
NTSTATUS
LwTaskSrvEnumCandidateTasks(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLW_SRV_TASK pTask = (PLW_SRV_TASK)pData;
    PLW_SRV_TASK_ENUM_QUERY pTaskEnumQuery =
                                    (PLW_SRV_TASK_ENUM_QUERY)pUserData;
    PLW_TASK_INFO pTaskInfo = NULL;
    BOOLEAN bInLock = FALSE;
    CHAR    szUUID[37];

    if (pTask->taskType != pTaskEnumQuery->taskType)
    {
        goto cleanup;
    }

    LW_TASK_LOCK_MUTEX(bInLock, &pTask->mutex);

    pTaskInfo = &pTaskEnumQuery->pTaskInfoArray[pTaskEnumQuery->iInfo++];

    uuid_unparse(pTask->uuid, szUUID);

    ntStatus = LwWin32ErrorToNtStatus(
                    LwAllocateString(
                              szUUID,
                              &pTaskInfo->pszTaskId));
    BAIL_ON_NT_STATUS(ntStatus);

    if (pTask->dwNumArgs)
    {
        ntStatus = LwWin32ErrorToNtStatus(
                        LwTaskDuplicateArgList(
                                pTask->pArgArray,
                                pTask->dwNumArgs,
                                &pTaskInfo->pArgArray,
                                &pTaskInfo->dwNumArgs));
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pbContinue = TRUE;

cleanup:

    LW_TASK_UNLOCK_MUTEX(bInLock, &pTask->mutex);

    return ntStatus;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

static
DWORD
LwTaskSrvCreateInternal(
    PCSTR         pszTaskName,
    DWORD         dwTaskId,
    LW_TASK_TYPE  taskType,
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
    pTask->taskType = taskType;

    pTask->pArgArray = *ppArgArray;
    *ppArgArray      = NULL;
    pTask->dwNumArgs = *pdwNumArgs;
    *pdwNumArgs      = 0;

    *ppTask = pTask;

cleanup:

    return dwError;

error:

    if (ppTask)
    {
        *ppTask = NULL;
    }

    if (pTask)
    {
        LwTaskSrvRelease(pTask);
    }

    goto cleanup;
}

VOID
LwTaskSrvInitStatus(
    PLW_SRV_TASK pTask
    )
{
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_MUTEX(bInLock, &pTask->mutex);

    pTask->dwPercentComplete = 0;

    pTask->dwError = 0;

    pTask->startTime = time(NULL);
    pTask->endTime = 0;

    LW_TASK_UNLOCK_MUTEX(bInLock, &pTask->mutex);
}

VOID
LwTaskSrvSetErrorCode(
    PLW_SRV_TASK pTask,
    DWORD        dwError
    )
{
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_MUTEX(bInLock, &pTask->mutex);

    pTask->dwError = dwError;

    pTask->endTime = time(NULL);

    LW_TASK_UNLOCK_MUTEX(bInLock, &pTask->mutex);
}

VOID
LwTaskSrvSetPercentComplete(
    PLW_SRV_TASK pTask,
    DWORD        dwPercentComplete
    )
{
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_MUTEX(bInLock, &pTask->mutex);

    pTask->dwPercentComplete = dwPercentComplete;

    if (dwPercentComplete == 100)
    {
        pTask->endTime = time(NULL);
    }

    LW_TASK_UNLOCK_MUTEX(bInLock, &pTask->mutex);
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

    if (gLwTaskSrvGlobals.pWorkerArray)
    {
        INT iWorker = 0;

        for (iWorker = 0; iWorker < gLwTaskSrvGlobals.dwNumWorkers; iWorker++)
        {
            PLW_TASK_SRV_WORKER pWorker = &gLwTaskSrvGlobals.pWorkerArray[iWorker];

            LwTaskWorkerIndicateStop(pWorker);
        }

        // Must indicate stop for all workers before queueing the
        // unblocks.
        for (iWorker = 0; iWorker < gLwTaskSrvGlobals.dwNumWorkers; iWorker++)
        {
            LwTaskUnblockOneWorker(&gLwTaskSrvGlobals.workQueue);
        }

        for (iWorker = 0; iWorker < gLwTaskSrvGlobals.dwNumWorkers; iWorker++)
        {
            PLW_TASK_SRV_WORKER pWorker = &gLwTaskSrvGlobals.pWorkerArray[iWorker];

            LwTaskWorkerFreeContents(pWorker);
        }

        LwFreeMemory(gLwTaskSrvGlobals.pWorkerArray);
        gLwTaskSrvGlobals.pWorkerArray = NULL;
    }

    LwTaskProdConsFreeContents(&gLwTaskSrvGlobals.workQueue);

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);
}

static
VOID
LwTaskUnblockOneWorker(
    IN PLW_TASK_PROD_CONS_QUEUE pWorkQueue
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PLW_TASK_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(sizeof(LW_TASK_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pContext->refCount = 1;

    dwError = LwTaskProdConsEnqueue(pWorkQueue, pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    return;

error:

    if (pContext)
    {
        LwTaskReleaseContext(pContext);
    }

    goto cleanup;
}
