/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwtaskclient.h
 *
 * Abstract:
 *
 *        Likewise Task System (LWTASK)
 *
 *        Client API
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#ifndef __LW_TASK_CLIENT_H__
#define __LW_TASK_CLIENT_H__

typedef struct _LW_TASK_CLIENT_CONNECTION* PLW_TASK_CLIENT_CONNECTION;

DWORD
LwTaskOpenServer(
    PLW_TASK_CLIENT_CONNECTION* ppConnection
    );

DWORD
LwTaskGetTypes(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE**             ppTaskTypes,
    PDWORD                     pdwNumTypes
    );

DWORD
LwTaskGetSchema(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE               taskType,
    PLW_TASK_ARG_INFO*         ppArgInfoArray,
    PDWORD                     pdwNumArgInfo
    );

DWORD
LwTaskEnum(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE               taskType,
    PLW_TASK_INFO*             ppTaskInfoArray,
    PDWORD                     pdwNumTaskInfos,
    PDWORD                     pdwNumTotalTaskInfos,
    PDWORD                     pdwResume
    );

DWORD
LwTaskCreate(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE               taskType,
    PLW_TASK_ARG               pArgArray,
    DWORD                      dwNumArgs,
    PSTR*                      ppszTaskId
    );

DWORD
LwTaskStart(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId,
    PLW_TASK_ARG               pArgArray,
    DWORD                      dwNumArgs
    );

DWORD
LwTaskGetStatus(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId,
    PLW_TASK_STATUS            pStatus
    );

DWORD
LwTaskStop(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId
    );

DWORD
LwTaskDelete(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId
    );

VOID
LwTaskFreeTaskInfoArray(
    PLW_TASK_INFO pTaskInfoArray,
    DWORD         dwNumTaskInfos
    );

VOID
LwTaskFreeArgInfoArray(
    PLW_TASK_ARG_INFO pArgInfoArray,
    DWORD             dwNumArgInfos
    );

VOID
LwTaskFreeArgArray(
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    );

VOID
LwTaskFreeMemory(
    PVOID pMemory
    );

DWORD
LwTaskSetLogLevel(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_LOG_LEVEL          logLevel
    );

DWORD
LwTaskSetLogInfo(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PLW_TASK_LOG_INFO          pLogInfo
    );

DWORD
LwTaskGetLogInfo(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PLW_TASK_LOG_INFO*         ppLogInfo
    );

DWORD
LwTaskGetPid(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    pid_t*                     pPid
    );

DWORD
LwTaskCloseServer(
    PLW_TASK_CLIENT_CONNECTION pConnection
    );

#endif /* __LW_TASK_CLIENT_H__ */

