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
 *        lwtaskipc.h
 *
 * Abstract:
 *
 *        Likewise Task System (LWTASK)
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
    LW_TASK_GET_PID_FAILED
} LW_TASK_IPC_MESSAGE_TAG;

DWORD
LwTaskIpcAddProtocolSpec(
    LWMsgProtocol* pProtocol /* IN OUT */
    );

