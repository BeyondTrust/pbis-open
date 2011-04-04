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

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#include <eventlog-record.h>

struct _LW_EVENTLOG_CONNECTION;
typedef struct _LW_EVENTLOG_CONNECTION
    LW_EVENTLOG_CONNECTION, *PLW_EVENTLOG_CONNECTION;

DWORD
LwEvtOpenEventlog(
    IN OPTIONAL PCSTR pServerName,
    OUT PLW_EVENTLOG_CONNECTION* ppConn
    );

// Returns an error if there was a problem safely closing the connection, but
// always frees the handle (meaning do not use the handle anymore).
DWORD
LwEvtCloseEventlog(
    IN PLW_EVENTLOG_CONNECTION pConn
    );

DWORD
LwEvtGetRecordCount(
    IN PLW_EVENTLOG_CONNECTION pConn,
    IN PCWSTR pSqlFilter,
    OUT PDWORD pNumMatched
    );

DWORD
LwEvtReadRecords(
    IN PLW_EVENTLOG_CONNECTION pConn,
    IN DWORD MaxResults,
    IN PCWSTR pSqlFilter,
    OUT PDWORD pCount,
    OUT PLW_EVENTLOG_RECORD* ppRecords
    );

DWORD
LwEvtWriteRecords(
    IN PLW_EVENTLOG_CONNECTION pConn,
    IN DWORD Count,
    IN PLW_EVENTLOG_RECORD pRecords 
    );

DWORD
LwEvtDeleteRecords(
    IN PLW_EVENTLOG_CONNECTION pConn,
    IN OPTIONAL PCWSTR pSqlFilter
    );

VOID
LwEvtFreeRecord(
    IN PLW_EVENTLOG_RECORD pRecord
    );

VOID
LwEvtFreeRecordArray(
    IN DWORD Count,
    IN PLW_EVENTLOG_RECORD pRecords
    );

// Legacy API. Do not use this for new applications
#if 1
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

DWORD
LWIOpenEventLog(
    PCSTR pszServerName,
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
LWIDeleteFromEventLog(
    HANDLE hEventLog,
    PCWSTR sqlFilter
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
#endif

#endif /* __EVENTLOG_H__ */
