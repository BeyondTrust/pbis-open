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
 *        lwmsg-ipc.c
 *
 * Abstract:
 *
 *        BeyondTrust Eventlog
 *
 * Authors:
 *          Kyle Stemen <kstemen@likewise.com>
 *
 */

#include "includes.h"

#if defined(WORDS_BIGENDIAN)
    #define UCS2_NATIVE "UCS-2BE"
#else
    #define UCS2_NATIVE "UCS-2LE"
#endif


#define LWMSG_MEMBER_PWSTR(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT16(WCHAR),                        \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_ZERO_TERMINATED,                     \
    LWMSG_ATTR_ENCODING(UCS2_NATIVE)

#define LWMSG_PWSTR       \
    LWMSG_POINTER_BEGIN, \
    LWMSG_UINT16(WCHAR),  \
    LWMSG_POINTER_END,   \
    LWMSG_ATTR_STRING


static LWMsgTypeSpec gLwEvtIpcGenericErrorSpec[] =
{
    LWMSG_STRUCT_BEGIN(EVT_IPC_GENERIC_ERROR),
    LWMSG_MEMBER_UINT32(EVT_IPC_GENERIC_ERROR, Error),
    LWMSG_MEMBER_PWSTR(EVT_IPC_GENERIC_ERROR, pErrorMessage),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwEvtIpcFilterSpec[] =
{
    LWMSG_PWSTR,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwEvtIpcRecordCountSpec[] =
{
    LWMSG_UINT32(DWORD),
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gEvtIpcReadRecordsReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(EVT_IPC_READ_RECORDS_REQ),
    LWMSG_MEMBER_UINT32(EVT_IPC_READ_RECORDS_REQ, MaxResults),
    LWMSG_MEMBER_PWSTR(EVT_IPC_READ_RECORDS_REQ, pFilter),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLwEventLogRecordSpec[] =
{
    LWMSG_STRUCT_BEGIN(LW_EVENTLOG_RECORD),
    LWMSG_MEMBER_UINT64(LW_EVENTLOG_RECORD, EventRecordId),
    LWMSG_MEMBER_PWSTR(LW_EVENTLOG_RECORD, pLogname),
    LWMSG_MEMBER_PWSTR(LW_EVENTLOG_RECORD, pEventType),
    LWMSG_MEMBER_UINT64(LW_EVENTLOG_RECORD, EventDateTime),
    LWMSG_MEMBER_PWSTR(LW_EVENTLOG_RECORD, pEventSource),
    LWMSG_MEMBER_PWSTR(LW_EVENTLOG_RECORD, pEventCategory),
    LWMSG_MEMBER_UINT32(LW_EVENTLOG_RECORD, EventSourceId),
    LWMSG_MEMBER_PWSTR(LW_EVENTLOG_RECORD, pUser),
    LWMSG_MEMBER_PWSTR(LW_EVENTLOG_RECORD, pComputer),
    LWMSG_MEMBER_PWSTR(LW_EVENTLOG_RECORD, pDescription),
    LWMSG_MEMBER_UINT32(LW_EVENTLOG_RECORD, DataLen),
    LWMSG_MEMBER_POINTER_BEGIN(LW_EVENTLOG_RECORD, pData),
    LWMSG_UINT8(BYTE),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(LW_EVENTLOG_RECORD, DataLen),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gEvtIpcRecordArraySpec[] =
{
    LWMSG_STRUCT_BEGIN(EVT_IPC_RECORD_ARRAY),
    LWMSG_MEMBER_UINT32(EVT_IPC_RECORD_ARRAY, Count),
    LWMSG_MEMBER_POINTER_BEGIN(EVT_IPC_RECORD_ARRAY, pRecords),
    LWMSG_TYPESPEC(gLwEventLogRecordSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(EVT_IPC_RECORD_ARRAY, Count),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gLwEvtIPCSpec[] =
{
    LWMSG_MESSAGE(EVT_R_GENERIC_ERROR, gLwEvtIpcGenericErrorSpec),
    LWMSG_MESSAGE(EVT_R_GENERIC_SUCCESS, NULL),
    LWMSG_MESSAGE(EVT_Q_GET_RECORD_COUNT, gLwEvtIpcFilterSpec), //PCWSTR
    LWMSG_MESSAGE(EVT_R_GET_RECORD_COUNT, gLwEvtIpcRecordCountSpec), //DWORD
    LWMSG_MESSAGE(EVT_Q_READ_RECORDS, gEvtIpcReadRecordsReqSpec),
    LWMSG_MESSAGE(EVT_R_READ_RECORDS, gEvtIpcRecordArraySpec),
    LWMSG_MESSAGE(EVT_Q_WRITE_RECORDS, gEvtIpcRecordArraySpec),
    // generic success
    LWMSG_MESSAGE(EVT_Q_DELETE_RECORDS, gLwEvtIpcFilterSpec), //PCWSTR
    // generic success
    LWMSG_PROTOCOL_END
};

LWMsgProtocolSpec*
LwEvtIPCGetProtocolSpec(
    void
    )
{
    return gLwEvtIPCSpec;
}
