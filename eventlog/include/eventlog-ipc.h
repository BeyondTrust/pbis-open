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
 *        eventlog-ipc.h
 *
 * Abstract:
 *
 *        Likewise event log
 *
 *        Interprocess Communication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen <kstemen@likewise.com>
 * 
 */
#ifndef __EVENTLOG_IPC_H__
#define __EVENTLOG_IPC_H__

#include <lwmsg/lwmsg.h>
#include <lwerror.h>

#define EVT_SERVER_FILENAME    ".eventlog"

typedef struct _EVT_IPC_GENERIC_ERROR
{
    DWORD Error;
    PCWSTR pErrorMessage;
} EVT_IPC_GENERIC_ERROR, *PEVT_IPC_GENERIC_ERROR;

typedef struct _EVT_IPC_READ_RECORDS_REQ {
    DWORD MaxResults;
    PCWSTR pFilter;
} EVT_IPC_READ_RECORDS_REQ, *PEVT_IPC_READ_RECORDS_REQ;

typedef struct _EVT_IPC_RECORD_ARRAY {
    DWORD Count;
    PLW_EVENTLOG_RECORD pRecords;
} EVT_IPC_RECORD_ARRAY, *PEVT_IPC_RECORD_ARRAY;

typedef enum _EVT_IPC_TAG
{
    EVT_R_GENERIC_ERROR,
    EVT_R_GENERIC_SUCCESS,
    EVT_Q_GET_RECORD_COUNT,
    EVT_R_GET_RECORD_COUNT,
    EVT_Q_READ_RECORDS,
    EVT_R_READ_RECORDS,
    EVT_Q_WRITE_RECORDS,
    // generic success or error
    EVT_Q_DELETE_RECORDS,
    // generic success or error
} EVT_IPC_TAG;

LWMsgProtocolSpec*
LwEvtIPCGetProtocolSpec(
    void
    );

#define MAP_LWMSG_ERROR(_e_) ((_e_) ? LwMapLwmsgStatusToLwError(_e_) : 0)
#define MAP_LW_ERROR_IPC(_e_) ((_e_) ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS)

#endif /*__EVENTLOG_IPC_H__*/
