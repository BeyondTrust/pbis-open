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
 *        lwmsg-ipc.c
 *
 * Abstract:
 *
 *        Likewise Eventlog
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
