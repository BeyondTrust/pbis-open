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
 * Copyright (C) BeyondTrust Corporation 2004-2007
 * Copyright (C) BeyondTrust Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Client Private Header
 *
 */
#ifndef __LWMSG_CLIENT_H__
#define __LWMSG_CLIENT_H__

typedef struct __LW_EVT_CLIENT_CONNECTION_CONTEXT
{
    LWMsgProtocol* pProtocol;
    LWMsgPeer* pClient;
    LWMsgSession* pSession;
} LW_EVT_CLIENT_CONNECTION_CONTEXT, *PLW_EVT_CLIENT_CONNECTION_CONTEXT;

DWORD
LwmEvtOpenServer(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT* ppConn
    );

DWORD
LwmEvtCloseServer(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn
    );

DWORD
LwmEvtAcquireCall(
    HANDLE hConnection,
    LWMsgCall** ppCall
    );

DWORD
LwmEvtGetRecordCount(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn,
    IN PCWSTR pSqlFilter,
    OUT PDWORD pNumMatched
    );

DWORD
LwmEvtReadRecords(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn,
    IN DWORD MaxResults,
    IN PCWSTR pSqlFilter,
    OUT PDWORD pCount,
    OUT PLW_EVENTLOG_RECORD* ppRecords
    );

DWORD
LwmEvtWriteRecords(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn,
    IN DWORD Count,
    IN PLW_EVENTLOG_RECORD pRecords 
    );

DWORD
LwmEvtDeleteRecords(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn,
    IN PCWSTR pSqlFilter
    );

#endif /* __LWMSG_CLIENT_H__ */
