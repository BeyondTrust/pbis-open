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
 *        taskrepository.h
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Task Repository
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#ifndef __TASKREPOSITORY_H__
#define __TASKREPOSITORY_H__

#define LW_TASK_MIGRATE_ARG_REMOTE_SERVER_NAME    "remote-server-name"
#define LW_TASK_MIGRATE_ARG_REMOTE_SHARE_NAME     "remote-share-name"
#define LW_TASK_MIGRATE_ARG_REMOTE_USER_PRINCIPAL "remote-user-principal"
#define LW_TASK_MIGRATE_ARG_REMOTE_PASSWORD       "remote-password"

typedef struct _LW_TASK_DB_CONTEXT *PLW_TASK_DB_CONTEXT;

typedef struct _LW_SRV_DB_TASK
{
    PSTR         pszTaskName;

    DWORD        dwTaskId;

    LW_TASK_TYPE taskType;

    PLW_TASK_ARG pArgArray;
    DWORD        dwNumArgs;

} LW_SRV_DB_TASK, *PLW_SRV_DB_TASK;

DWORD
LwTaskRepositoryInit(
    VOID
    );

DWORD
LwTaskDbOpen(
    PLW_TASK_DB_CONTEXT* ppDbContext
    );

DWORD
LwTaskDbBeginTransaction(
    PLW_TASK_DB_CONTEXT pDbContext
    );

DWORD
LwTaskDbRollbackTransaction(
    PLW_TASK_DB_CONTEXT pDbContext
    );

DWORD
LwTaskDbCommitTransaction(
    PLW_TASK_DB_CONTEXT pDbContext
    );

DWORD
LwTaskDbGetTypes(
    PLW_TASK_DB_CONTEXT pDbContext,
    PDWORD*             ppdwTaskTypeArray,
    PDWORD              pdwNumTaskTypes
    );

DWORD
LwTaskDbGetSchema(
    PLW_TASK_DB_CONTEXT pDbContext,
    LW_TASK_TYPE        taskType,
    PLW_TASK_ARG_INFO*  ppArgInfoArray,
    PDWORD              pdwNumArgInfos
    );

DWORD
LwTaskDbCreateTask(
    PLW_TASK_DB_CONTEXT pDbContext,
    PCSTR               pszTaskname,
    LW_TASK_TYPE        taskType,
    PDWORD              pdwTaskId
    );

DWORD
LwTaskDbCreateTaskArg(
    PLW_TASK_DB_CONTEXT pDbContext,
    DWORD               dwTaskId,
    PCSTR               pszArgName,
    PCSTR               pszArgValue,
    DWORD               dwArgType
    );

DWORD
LwTaskDbGetTasks(
    PLW_TASK_DB_CONTEXT pDbContext,
    PLW_SRV_DB_TASK*    ppTaskArray,
    PDWORD              pdwNumTasks
    );

DWORD
LwTaskDbDeleteTask(
    PLW_TASK_DB_CONTEXT pDbContext,
    DWORD               dwTaskId
    );

VOID
LwTaskDbFreeTaskArray(
    PLW_SRV_DB_TASK pTaskArray,
    DWORD           dwNumTasks
    );

VOID
LwTaskDbClose(
    PLW_TASK_DB_CONTEXT pDbContext
    );

VOID
LwTaskRepositoryShutdown(
    VOID
    );

#endif /* __TASKREPOSITORY_H__ */
