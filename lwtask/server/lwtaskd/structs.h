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
 *        structs.h
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

typedef enum
{
    LW_TASK_EXECUTION_STATUS_STOPPED = 0,
    LW_TASK_EXECUTION_STATUS_RUNNING

} LW_TASK_EXECUTION_STATUS;

typedef struct _LW_SRV_TASK
{
    LONG   refCount;

    uuid_t uuid;

    DWORD  dwTaskId;

    LW_TASK_TYPE taskType;

    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    PLW_TASK_ARG pArgArray;
    DWORD        dwNumArgs;

    LW_TASK_EXECUTION_STATUS execStatus;

    BOOLEAN      bCancel;

    DWORD        dwError;

    DWORD        dwPercentComplete;

    time_t       startTime;
    time_t       endTime;

} LW_SRV_TASK, *PLW_SRV_TASK;

typedef struct _LW_SRV_TASK_ENUM_QUERY
{
    LW_TASK_TYPE  taskType;

    DWORD         dwTotalEntries;

    PLW_TASK_INFO pTaskInfoArray;
    DWORD         dwNumTaskInfos;

    DWORD         dwResume;

    DWORD         iInfo;

} LW_SRV_TASK_ENUM_QUERY, *PLW_SRV_TASK_ENUM_QUERY;

typedef struct _LW_TASK_CONTEXT
{
    LONG         refCount;

    PLW_SRV_TASK pTask;
    PLW_TASK_ARG pArgArray;
    DWORD        dwNumArgs;

} LW_TASK_CONTEXT, *PLW_TASK_CONTEXT;

typedef VOID (*PFN_LW_TASK_PROD_CONS_QUEUE_FREE_ITEM)(PVOID pItem);

typedef struct _LW_TASK_PROD_CONS_QUEUE
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    LW_TASK_QUEUE    queue;

    ULONG            ulNumMaxItems;
    ULONG            ulNumItems;

    PFN_LW_TASK_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem;

    pthread_cond_t  event;
    pthread_cond_t* pEvent;

} LW_TASK_PROD_CONS_QUEUE, *PLW_TASK_PROD_CONS_QUEUE;

typedef struct _LW_TASK_SRV_WORKER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    BOOLEAN bStop;

    DWORD   dwWorkerId;

} LW_TASK_SRV_WORKER_CONTEXT, *PLW_TASK_SRV_WORKER_CONTEXT;

typedef struct _LW_TASK_SRV_WORKER
{
    pthread_t  worker;
    pthread_t* pWorker;

    DWORD      dwWorkerId;

    LW_TASK_SRV_WORKER_CONTEXT context;

} LW_TASK_SRV_WORKER, *PLW_TASK_SRV_WORKER;

typedef struct _LW_TASKD_GLOBALS
{
    pthread_rwlock_t   mutex;                      /* MT safety               */
    pthread_rwlock_t*  pMutex;

    DWORD              dwStartAsDaemon;            /* Should start as daemon  */
    LW_TASK_LOG_TARGET logTarget;                  /* where are we logging    */
    LW_TASK_LOG_LEVEL  maxAllowedLogLevel;         /* How much logging ?      */
    CHAR               szLogFilePath[PATH_MAX + 1];/* log file path           */
    BOOLEAN            bProcessShouldExit;         /* Process termination flag*/
    DWORD              dwExitCode;                 /* Process exit Code       */

    LWMsgContext*      pContext;
    LWMsgProtocol*     pProtocol;
    LWMsgServer*       pServer;


    PLWRTL_RB_TREE     pTaskCollection;

    LW_TASK_PROD_CONS_QUEUE   workQueue;
    DWORD                     dwMaxNumWorkItemsInQueue;

    PLW_TASK_SRV_WORKER       pWorkerArray;
    DWORD                     dwNumWorkers;

} LW_TASKD_GLOBALS, *PLW_TASKD_GLOBALS;
