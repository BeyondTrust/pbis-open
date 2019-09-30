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
 *        prototypes.h
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
 *
 *        Function prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

// context.c

DWORD
LwTaskCreateContext(
    PLW_SRV_TASK      pTask,
    PLW_TASK_ARG*     ppArgArray,
    PDWORD            pdwNumArgs,
    PLW_TASK_CONTEXT* ppContext
    );

BOOLEAN
LwTaskIsValidContext(
    PLW_TASK_CONTEXT pContext
    );

VOID
LwTaskReleaseContextHandle(
    HANDLE hContext
    );

VOID
LwTaskReleaseContext(
    PLW_TASK_CONTEXT pContext
    );

// ipc.c

DWORD
LwTaskDaemonIpcAddDispatch(
    LWMsgServer* pServer /* IN OUT */
    );

// listener.c

DWORD
LwTaskSrvStartListenThread(
    void
    );

DWORD
LwTaskSrvStopListenThread(
    void
    );

// main.c

VOID
LwTaskSrvExitHandler(
    VOID
    );

DWORD
LwTaskInitCacheFolders(
    VOID
    );

BOOLEAN
LwTaskSrvShouldStartAsDaemon(
    VOID
    );

DWORD
LwTaskSrvStartAsDaemon(
    VOID
    );

DWORD
LwTaskSrvGetProcessExitCode(
    PDWORD pdwExitCode
    );

VOID
LwTaskSrvSetProcessExitCode(
    DWORD dwExitCode
    );

DWORD
LwTaskBlockSelectedSignals(
    VOID
    );

BOOLEAN
LwTaskSrvShouldProcessExit(
    VOID
    );

VOID
LwTaskSrvSetProcessToExit(
    BOOLEAN bExit
    );

// prodcons.c

DWORD
LwTaskProdConsInit(
    ULONG                                 ulNumMaxItems,
    PFN_LW_TASK_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem,
    PLW_TASK_PROD_CONS_QUEUE*             ppQueue
    );

DWORD
LwTaskProdConsInitContents(
    PLW_TASK_PROD_CONS_QUEUE              pQueue,
    ULONG                                 ulNumMaxItems,
    PFN_LW_TASK_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem
    );

DWORD
LwTaskProdConsEnqueue(
    PLW_TASK_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    );

DWORD
LwTaskProdConsEnqueueFront(
    PLW_TASK_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    );

DWORD
LwTaskProdConsDequeue(
    PLW_TASK_PROD_CONS_QUEUE pQueue,
    PVOID*               ppItem
    );

DWORD
LwTaskProdConsTimedDequeue(
    PLW_TASK_PROD_CONS_QUEUE pQueue,
    struct timespec*         pTimespec,
    PVOID*                   ppItem
    );

VOID
LwTaskProdConsFree(
    PLW_TASK_PROD_CONS_QUEUE pQueue
    );

VOID
LwTaskProdConsFreeContents(
    PLW_TASK_PROD_CONS_QUEUE pQueue
    );

// signalhandler.c

DWORD
LwTaskSrvIgnoreSIGHUP(
    VOID
    );

DWORD
LwTaskSrvHandleSignals(
    VOID
    );

DWORD
LwTaskSrvStopProcess(
    VOID
    );

// task.c

DWORD
LwTaskSrvInit(
    VOID
    );

DWORD
LwTaskSrvGetTypes(
    PDWORD* ppdwTaskTypeArray,
    PDWORD  pdwNumTypes
    );

DWORD
LwTaskSrvStart(
    PCSTR        pszTaskname,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    );

DWORD
LwTaskSrvStop(
    PCSTR pszTaskname
    );

DWORD
LwTaskSrvDelete(
    PCSTR pszTaskname
    );

DWORD
LwTaskSrvGetStatus(
    PCSTR           pszTaskId,
    PLW_TASK_STATUS pTaskStatus
    );

DWORD
LwTaskSrvCreate(
    LW_TASK_TYPE taskType,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs,
    PSTR*        ppszTaskId
    );

PLW_TASK_ARG
LwTaskFindArg(
    PCSTR        pszArgName,
    DWORD        dwArgType,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    );

DWORD
LwTaskSrvGetSchema(
    LW_TASK_TYPE       taskType,
    PLW_TASK_ARG_INFO* ppArgInfoArray,
    PDWORD             pdwNumArgInfos
    );

DWORD
LwTaskSrvEnum(
    LW_TASK_TYPE   taskType,
    PDWORD         pdwTotalTaskInfos,
    PDWORD         pdwNumTaskInfos,
    PLW_TASK_INFO* ppTaskInfoArray,
    PDWORD         pdwResume
    );

VOID
LwTaskSrvInitStatus(
    PLW_SRV_TASK pTask
    );

VOID
LwTaskSrvSetErrorCode(
    PLW_SRV_TASK pTask,
    DWORD        dwError
    );

VOID
LwTaskSrvSetPercentComplete(
    PLW_SRV_TASK pTask,
    DWORD        dwPercentComplete
    );

VOID
LwTaskSrvRelease(
    PLW_SRV_TASK pTask
    );

VOID
LwTaskSrvShutdown(
    VOID
    );

// worker.c

DWORD
LwTaskWorkerInit(
    PLW_TASK_SRV_WORKER pWorker
    );

VOID
LwTaskWorkerIndicateStop(
    PLW_TASK_SRV_WORKER pWorker
    );
VOID
LwTaskWorkerFreeContents(
    PLW_TASK_SRV_WORKER pWorker
    );
