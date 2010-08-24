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
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
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

static LWMsgTypeSpec gLwTaskPidSpec[] =
{
    LWMSG_INT32(pid_t),
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
    LWMSG_MESSAGE(LW_TASK_START,                       gLwTaskIdSpec),
    LWMSG_MESSAGE(LW_TASK_START_SUCCESS,               gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_START_FAILED,                gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_STOP,                        gLwTaskIdSpec),
    LWMSG_MESSAGE(LW_TASK_STOP_SUCCESS,                gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_STOP_FAILED,                 gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_DELETE,                      gLwTaskIdSpec),
    LWMSG_MESSAGE(LW_TASK_DELETE_SUCCESS,              gLwTaskStatusReplySpec),
    LWMSG_MESSAGE(LW_TASK_DELETE_FAILED,               gLwTaskStatusReplySpec),
    LWMSG_PROTOCOL_END
};

