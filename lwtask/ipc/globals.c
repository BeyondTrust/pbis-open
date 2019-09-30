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
 *        globals.c
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
 *
 *        Inter-process Communication
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static LWMsgTypeSpec gLwTaskIdSpec[] =
{
    LWMSG_PSTR,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskStatusReplySpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_STATUS_REPLY),
    /* err - marshal as 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_STATUS_REPLY, dwError),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskLogInfoSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_LOG_INFO),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_LOG_INFO, maxAllowedLogLevel),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_LOG_INFO, logTarget),
    /* path - marshal as pointer to string */
    LWMSG_MEMBER_PSTR(LW_TASK_LOG_INFO, pszPath),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskTypesSpec[] =
{
    LWMSG_STRUCT_BEGIN(LW_TASK_IPC_GET_TYPES),
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_GET_TYPES, dwNumTaskTypes),
    LWMSG_MEMBER_POINTER(LW_TASK_IPC_GET_TYPES, pdwTaskTypeArray, LWMSG_UINT32(ULONG)),
    LWMSG_ATTR_LENGTH_MEMBER(LW_TASK_IPC_GET_TYPES, dwNumTaskTypes),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskPidSpec[] =
{
    LWMSG_INT32(pid_t),
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskStatusSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_STATUS),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_STATUS, dwError),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_STATUS, dwPercentComplete),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_STATUS, startTime),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_STATUS, endTime),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskArgSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_ARG),
    LWMSG_MEMBER_PSTR(LW_TASK_ARG, pszArgName),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_ARG, dwArgType),
    LWMSG_MEMBER_PSTR(LW_TASK_ARG, pszArgValue),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskCreateSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_IPC_CREATE_ARGS),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_CREATE_ARGS, taskType),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_CREATE_ARGS, dwNumArgs),
    LWMSG_MEMBER_POINTER(LW_TASK_IPC_CREATE_ARGS, pArgArray, LWMSG_TYPESPEC(gLwTaskArgSpec)),
    LWMSG_ATTR_LENGTH_MEMBER(LW_TASK_IPC_CREATE_ARGS, dwNumArgs),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskStartSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_IPC_START_ARGS),
    LWMSG_MEMBER_PSTR(LW_TASK_IPC_START_ARGS, pszTaskId),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_START_ARGS, dwNumArgs),
    LWMSG_MEMBER_POINTER(LW_TASK_IPC_START_ARGS, pArgArray, LWMSG_TYPESPEC(gLwTaskArgSpec)),
    LWMSG_ATTR_LENGTH_MEMBER(LW_TASK_IPC_START_ARGS, dwNumArgs),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskArgInfoSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_ARG_INFO),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_PSTR(LW_TASK_ARG_INFO, pszArgName),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_ARG_INFO, argType),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_ARG_INFO, dwFlags),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskSchemaReplySpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_IPC_SCHEMA),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_SCHEMA, dwNumArgInfos),
    LWMSG_MEMBER_POINTER(LW_TASK_IPC_SCHEMA, pArgInfoArray, LWMSG_TYPESPEC(gLwTaskArgInfoSpec)),
    LWMSG_ATTR_LENGTH_MEMBER(LW_TASK_IPC_SCHEMA, dwNumArgInfos),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskGetSchemaSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_IPC_GET_SCHEMA),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_GET_SCHEMA, taskType),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskEnumRequestSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_IPC_ENUM_REQUEST),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_ENUM_REQUEST, taskType),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_ENUM_REQUEST, dwResume),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskInfoSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_INFO),
    LWMSG_MEMBER_PSTR(LW_TASK_INFO, pszTaskId),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_INFO, dwNumArgs),
    LWMSG_MEMBER_POINTER(LW_TASK_INFO, pArgArray, LWMSG_TYPESPEC(gLwTaskArgSpec)),
    LWMSG_ATTR_LENGTH_MEMBER(LW_TASK_INFO, dwNumArgs),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwTaskEnumReplySpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(LW_TASK_IPC_ENUM_RESPONSE),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_ENUM_RESPONSE, dwResume),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_ENUM_RESPONSE, dwTotalTaskInfos),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(LW_TASK_IPC_ENUM_RESPONSE, dwNumTaskInfos),
    LWMSG_MEMBER_POINTER(LW_TASK_IPC_ENUM_RESPONSE, pTaskInfoArray, LWMSG_TYPESPEC(gLwTaskInfoSpec)),
    LWMSG_ATTR_LENGTH_MEMBER(LW_TASK_IPC_ENUM_RESPONSE, dwNumTaskInfos),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgProtocolSpec gLwTaskDaemonProtocolSpec[] =
{
    LWMSG_MESSAGE(LW_TASK_SET_LOG_INFO,                gLwTaskLogInfoSpec),
    LWMSG_MESSAGE(LW_TASK_SET_LOG_INFO_SUCCESS,        gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_SET_LOG_INFO_FAILED,         gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_GET_LOG_INFO,                NULL),
    LWMSG_MESSAGE(LW_TASK_GET_LOG_INFO_SUCCESS,        gLwTaskLogInfoSpec),
    LWMSG_MESSAGE(LW_TASK_GET_LOG_INFO_FAILED,         gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_GET_PID,                     NULL),
    LWMSG_MESSAGE(LW_TASK_GET_PID_SUCCESS,             gLwTaskPidSpec),
    LWMSG_MESSAGE(LW_TASK_GET_PID_FAILED,              gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_START,                       gLwTaskStartSpec),
    LWMSG_MESSAGE(LW_TASK_START_SUCCESS,               gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_START_FAILED,                gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_STOP,                        gLwTaskIdSpec),
    LWMSG_MESSAGE(LW_TASK_STOP_SUCCESS,                gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_STOP_FAILED,                 gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_DELETE,                      gLwTaskIdSpec),
    LWMSG_MESSAGE(LW_TASK_DELETE_SUCCESS,              gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_DELETE_FAILED,               gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_GET_TYPES,                   NULL),
    LWMSG_MESSAGE(LW_TASK_GET_TYPES_SUCCESS,           gLwTaskTypesSpec),
    LWMSG_MESSAGE(LW_TASK_GET_TYPES_FAILED,            gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_GET_STATUS,                  gLwTaskIdSpec),
    LWMSG_MESSAGE(LW_TASK_GET_STATUS_SUCCESS,          gLwTaskStatusSpec),
    LWMSG_MESSAGE(LW_TASK_GET_STATUS_FAILED,           gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_CREATE,                      gLwTaskCreateSpec),
    LWMSG_MESSAGE(LW_TASK_CREATE_SUCCESS,              gLwTaskIdSpec),
    LWMSG_MESSAGE(LW_TASK_CREATE_FAILED,               gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_GET_SCHEMA,                  gLwTaskGetSchemaSpec),
    LWMSG_MESSAGE(LW_TASK_GET_SCHEMA_SUCCESS,          gLwTaskSchemaReplySpec),
    LWMSG_MESSAGE(LW_TASK_GET_SCHEMA_FAILED,           gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_ENUM,                        gLwTaskEnumRequestSpec),
    LWMSG_MESSAGE(LW_TASK_ENUM_SUCCESS,                gLwTaskEnumReplySpec),
    LWMSG_MESSAGE(LW_TASK_ENUM_FAILED,                 gLwTaskStatusReplySpec),
    LWMSG_PROTOCOL_END
};

