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
 *        lwtaskclient.h
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
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

