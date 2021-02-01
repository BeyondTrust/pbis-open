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
 *        lwtaskipc.h
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
 *
 *        Inter-process Communication
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#define MAP_LWMSG_ERROR(_e_) (LwMapLwmsgStatusToLwError(_e_))

#define LW_TASK_SERVER_FILENAME    ".lwtaskd"

/*
 *  Generic status code reply
 */
typedef struct _LW_TASK_STATUS_REPLY
{
    DWORD dwError;

} LW_TASK_STATUS_REPLY, *PLW_TASK_STATUS_REPLY;

typedef struct _LW_TASK_IPC_GET_TYPES
{
    DWORD  dwNumTaskTypes;
    PDWORD pdwTaskTypeArray;
} LW_TASK_IPC_GET_TYPES, *PLW_TASK_IPC_GET_TYPES;

typedef struct _LW_TASK_IPC_CREATE_ARGS
{
    LW_TASK_TYPE taskType;
    DWORD        dwNumArgs;
    PLW_TASK_ARG pArgArray;
} LW_TASK_IPC_CREATE_ARGS, *PLW_TASK_IPC_CREATE_ARGS;

typedef struct _LW_TASK_IPC_START_ARGS
{
    PCSTR        pszTaskId;
    DWORD        dwNumArgs;
    PLW_TASK_ARG pArgArray;
} LW_TASK_IPC_START_ARGS, *PLW_TASK_IPC_START_ARGS;

typedef struct _LW_TASK_IPC_GET_SCHEMA
{
    LW_TASK_TYPE taskType;

} LW_TASK_IPC_GET_SCHEMA, *PLW_TASK_IPC_GET_SCHEMA;

typedef struct _LW_TASK_IPC_SCHEMA
{
    DWORD             dwNumArgInfos;
    PLW_TASK_ARG_INFO pArgInfoArray;

} LW_TASK_IPC_SCHEMA, *PLW_TASK_IPC_SCHEMA;

typedef struct _LW_TASK_IPC_ENUM_REQUEST
{
    LW_TASK_TYPE taskType;
    DWORD        dwResume;

} LW_TASK_IPC_ENUM_REQUEST, *PLW_TASK_IPC_ENUM_REQUEST;

typedef struct _LW_TASK_IPC_ENUM_RESPONSE
{
    DWORD         dwResume;
    DWORD         dwTotalTaskInfos;
    DWORD         dwNumTaskInfos;
    PLW_TASK_INFO pTaskInfoArray;

} LW_TASK_IPC_ENUM_RESPONSE, *PLW_TASK_IPC_ENUM_RESPONSE;

/*
 *  Protocol message enumeration
 */
typedef enum _LW_TASK_IPC_MESSAGE_TAG
{
    LW_TASK_SET_LOG_INFO,                     // LW_TASK_LOG_INFO
    LW_TASK_SET_LOG_INFO_SUCCESS,             // LW_TASK_STATUS_REPLY
    LW_TASK_SET_LOG_INFO_FAILED,              // LW_TASK_STATUS_REPLY
    LW_TASK_GET_LOG_INFO,                     // LW_TASK_LOG_INFO
    LW_TASK_GET_LOG_INFO_SUCCESS,             // LW_TASK_STATUS_REPLY
    LW_TASK_GET_LOG_INFO_FAILED,              // LW_TASK_STATUS_REPLY
    LW_TASK_GET_PID,
    LW_TASK_GET_PID_SUCCESS,
    LW_TASK_GET_PID_FAILED,
    LW_TASK_START,
    LW_TASK_START_SUCCESS,
    LW_TASK_START_FAILED,
    LW_TASK_STOP,
    LW_TASK_STOP_SUCCESS,
    LW_TASK_STOP_FAILED,
    LW_TASK_DELETE,
    LW_TASK_DELETE_SUCCESS,
    LW_TASK_DELETE_FAILED,
    LW_TASK_GET_TYPES,
    LW_TASK_GET_TYPES_SUCCESS,
    LW_TASK_GET_TYPES_FAILED,
    LW_TASK_GET_STATUS,
    LW_TASK_GET_STATUS_SUCCESS,
    LW_TASK_GET_STATUS_FAILED,
    LW_TASK_CREATE,
    LW_TASK_CREATE_SUCCESS,
    LW_TASK_CREATE_FAILED,
    LW_TASK_GET_SCHEMA,
    LW_TASK_GET_SCHEMA_SUCCESS,
    LW_TASK_GET_SCHEMA_FAILED,
    LW_TASK_ENUM,
    LW_TASK_ENUM_SUCCESS,
    LW_TASK_ENUM_FAILED
} LW_TASK_IPC_MESSAGE_TAG;

DWORD
LwTaskIpcAddProtocolSpec(
    LWMsgProtocol* pProtocol /* IN OUT */
    );

