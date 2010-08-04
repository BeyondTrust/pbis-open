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
 *        eventlog.h
 *
 * Abstract:
 *
 *        Likewise Eventlog Service (LWEVT)
 *
 *        Public header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EVENTLOG_H__
#define __EVENTLOG_H__

#ifdef UNICODE
#undef UNICODE
#endif

#ifndef _WIN32
#include <lw/types.h>
#include <lw/attrs.h>
#endif

/* ERRORS */
#define EVT_ERROR_SUCCESS                   0x0000
#define EVT_ERROR_INVALID_CONFIG_PATH       0x9001 // 36865
#define EVT_ERROR_INVALID_PREFIX_PATH       0x9002 // 36866
#define EVT_ERROR_INSUFFICIENT_BUFFER       0x9003 // 36867
#define EVT_ERROR_OUT_OF_MEMORY             0x9004 // 36868
#define EVT_ERROR_INVALID_MESSAGE           0x9005 // 36869
#define EVT_ERROR_UNEXPECTED_MESSAGE        0x9006 // 36870
#define EVT_ERROR_NO_SUCH_USER              0x9007 // 36871
#define EVT_ERROR_DATA_ERROR                0x9008 // 36872
#define EVT_ERROR_NOT_IMPLEMENTED           0x9009 // 36873
#define EVT_ERROR_NO_CONTEXT_ITEM           0x900A // 36874
#define EVT_ERROR_NO_SUCH_GROUP             0x900B // 36875
#define EVT_ERROR_REGEX_COMPILE_FAILED      0x900C // 36876
#define EVT_ERROR_NSS_EDIT_FAILED           0x900D // 36877
#define EVT_ERROR_NO_HANDLER                0x900E // 36878
#define EVT_ERROR_INTERNAL                  0x900F // 36879
#define EVT_ERROR_NOT_HANDLED               0x9010 // 36880
#define EVT_ERROR_UNEXPECTED_DB_RESULT      0x9011 // 36881
#define EVT_ERROR_INVALID_PARAMETER         0x9012 // 36882
#define EVT_ERROR_LOAD_LIBRARY_FAILED       0x9013 // 36883
#define EVT_ERROR_LOOKUP_SYMBOL_FAILED      0x9014 // 36884
#define EVT_ERROR_INVALID_EVENTLOG          0x9015 // 36885
#define EVT_ERROR_INVALID_CONFIG            0x9016 // 36886
#define EVT_ERROR_STRING_CONV_FAILED        0x9017 // 36887
#define EVT_ERROR_INVALID_DB_HANDLE         0x9018 // 36888
#define EVT_ERROR_FAILED_CONVERT_TIME       0x9019 // 36889
#define EVT_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING 0x901A // 36890
#define EVT_ERROR_RPC_EXCEPTION_UPON_OPEN   0x901B // 36891
#define EVT_ERROR_RPC_EXCEPTION_UPON_CLOSE  0x901C // 36892
#define EVT_ERROR_RPC_EXCEPTION_UPON_COUNT  0x901D // 36893
#define EVT_ERROR_RPC_EXCEPTION_UPON_READ   0x901E // 36894
#define EVT_ERROR_RPC_EXCEPTION_UPON_WRITE  0x901F // 36895
#define EVT_ERROR_RPC_EXCEPTION_UPON_CLEAR  0x9020 // 36896
#define EVT_ERROR_RPC_EXCEPTION_UPON_DELETE 0x9021 // 36897
#define EVT_ERROR_RPC_EXCEPTION_UPON_REGISTER 0x9022 // 36898
#define EVT_ERROR_RPC_EXCEPTION_UPON_UNREGISTER 0x9023 // 36899
#define EVT_ERROR_RPC_EXCEPTION_UPON_LISTEN 0x9024 // 36900
#define EVT_ERROR_RPC_EXCEPTION             0x9025 // 36901
#define EVT_ERROR_ACCESS_DENIED             0x9026 // 36902
#define EVT_ERROR_SENTINEL                  0x9027 // 36903

#define EVT_ERROR_MASK(_e_)             (_e_ & 0x9000)

#ifndef EINVAL
#define EINVAL 22
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef ENOMEM
#define ENOMEM 12
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#ifndef EVENT_LOG_RECORD_DEFINED
#define EVENT_LOG_RECORD_DEFINED 1

typedef struct _EVENT_LOG_RECORD
{
    DWORD   dwEventRecordId;
    PSTR    pszEventTableCategoryId;
    PSTR    pszEventType;
    DWORD   dwEventDateTime;
    PSTR    pszEventSource;
    PSTR    pszEventCategory;
    DWORD   dwEventSourceId;
    PSTR    pszUser;
    PSTR    pszComputer;
    PSTR    pszDescription;
    PSTR    pszData;

} EVENT_LOG_RECORD, *PEVENT_LOG_RECORD;

#endif /* EVENT_LOG_RECORD_DEFINED */

#ifndef EVENT_LOG_HANDLE_DEFINED
#define EVENT_LOG_HANDLE_DEFINED 1

typedef struct _EVENT_LOG_HANDLE
{
    HANDLE bindingHandle;

    short  bDefaultActive;

    EVENT_LOG_RECORD defaultEventLogRecord;

} EVENT_LOG_HANDLE, *PEVENT_LOG_HANDLE;

#endif /* EVENT_LOG_HANDLE_DEFINED */

DWORD
LWIOpenEventLog(
    PCSTR pszServerName,
    PHANDLE phEventLog
    );

DWORD
LWIOpenEventLogEx(
    PCSTR pszServerName,
    PCSTR pszEventTableCategoryId,
    PCSTR pszSource,
    DWORD dwEventSourceId,
    PCSTR pszUser,
    PCSTR pszComputer,
    PHANDLE phEventLog
    );

DWORD
LWIReadEventLog(
    HANDLE hEventLog,
    DWORD dwLastRecordId,
    DWORD nRecordsPerPage,
    PCWSTR sqlFilter,
    PDWORD pdwNumReturned,
    EVENT_LOG_RECORD** eventRecords
    );

DWORD
LWICountEventLog(
    HANDLE hEventLog,
    PCWSTR sqlFilter,
    DWORD* pdwNumMatched
    );

DWORD
LWISetEventLogTableCategoryId(
    HANDLE hEventLog,
    PCSTR pszEventTableCategoryId
    );

DWORD
LWISetEventLogType(
    HANDLE hEventLog,
    PCSTR pszEventType
    );

DWORD
LWISetEventLogSource(
    HANDLE hEventLog,
    PCSTR pszEventSource,
    DWORD dwEventSourceId
    );

DWORD
LWISetEventLogTableCategory(
    HANDLE hEventLog,
    PCSTR pszEventCategory
    );

DWORD
LWISetEventLogUser(
    HANDLE hEventLog,
    PCSTR pszUser
    );

DWORD
LWISetEventLogComputer(
    HANDLE hEventLog,
    PCSTR pszComputer
    );

DWORD
LWIWriteEventLogBase(
    HANDLE hEventLog,
    EVENT_LOG_RECORD eventRecord
    );

DWORD
LWIWriteEventLogRecords(
    HANDLE hEventLog,
    DWORD cRecords,
    PEVENT_LOG_RECORD pEventRecords 
    );

DWORD
LWIWriteEventLog(
    HANDLE hEventLog,
    PCSTR eventType,
    PCSTR eventCategory,
    PCSTR eventDescription,
    PCSTR eventData
    );

DWORD
LWIDeleteFromEventLog(
    HANDLE hEventLog,
    PCWSTR sqlFilter
    );

DWORD
LWIClearEventLog(
    HANDLE hEventLog
    );

DWORD
LWICloseEventLog(
    HANDLE hEventLog
    );

VOID
LWIFreeEventRecord(
    PEVENT_LOG_RECORD pEventRecord
    );

VOID
LWIFreeEventRecordContents(
    PEVENT_LOG_RECORD pEventRecord
    );

VOID
LWIFreeEventRecordList(
    DWORD dwRecords,
    PEVENT_LOG_RECORD pEventRecordList
    );

VOID
LWIFreeEventLogHandle(
    HANDLE hEventLog
    );

#endif /* __EVENTLOG_H__ */

