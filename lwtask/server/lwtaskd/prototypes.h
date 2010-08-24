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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Function prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

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
    PCSTR        pszTaskId,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    );

DWORD
LwTaskSrvStop(
    PCSTR pszTaskId
    );

DWORD
LwTaskSrvDelete(
    PCSTR pszTaskId
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
LwTaskSrvRelease(
    PLW_SRV_TASK pTask
    );

VOID
LwTaskSrvShutdown(
    VOID
    );

