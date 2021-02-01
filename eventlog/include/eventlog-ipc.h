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
 *        eventlog-ipc.h
 *
 * Abstract:
 *
 *        BeyondTrust event log
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
