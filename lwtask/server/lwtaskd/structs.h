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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
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
