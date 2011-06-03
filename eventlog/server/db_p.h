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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Server API Implementation
 *
 */
#ifndef __EVTDB_H__
#define __EVTDB_H__

typedef struct __EVENTLOG_CONTEXT
{
    sqlite3* pDbHandle;
} EVENTLOG_CONTEXT, *PEVENTLOG_CONTEXT;
    
typedef enum
{
    EventRecordId = 0,
    EventTableCategoryId, //1
    EventType, //2
    EventDateTime, //3
    EventSource, //4
    EventCategory, //5
    EventSourceId, //6
    User, //7
    Computer, //8
    Description, //9
    Data, //10
    EVENT_DB_COL_SENTINEL
} EventDbColumnType;

DWORD
LwEvtDbInitEventDatabase();

DWORD
LwEvtDbShutdownEventDatabase();

//database actions
DWORD
LwEvtDbOpen(
    sqlite3** ppDb
    );

DWORD
LwEvtDbClose(
    sqlite3* pDb
    );

DWORD
LwEvtDbGetRecordCount(
    sqlite3 *pDb,
    const WCHAR * pSqlFilter,
    DWORD * pNumMatched
    );

VOID
LwEvtDbFreeRecord(
    IN VOID (*pFree)(PVOID),
    IN PLW_EVENTLOG_RECORD pRecord
    );

DWORD
LwEvtDbReadRecords(
    DWORD (*pAllocate)(DWORD, PVOID*),
    VOID (*pFree)(PVOID),
    sqlite3 *pDb,
    DWORD MaxResults,
    PCWSTR pSqlFilter,
    PDWORD pCount,
    PLW_EVENTLOG_RECORD* ppRecords
    );

DWORD
LwEvtDbWriteRecords(
    sqlite3 *pDb,
    DWORD Count,
    const LW_EVENTLOG_RECORD* pRecords 
    );

DWORD
LwEvtDbDeleteRecords(
    sqlite3 *pDb,
    PCWSTR pSqlFilter
    );

//helper functions
DWORD
LwEvtDbQueryEventLog(
    sqlite3 *pDb,
    PSTR   szQuery,
    PDWORD pdwNumRows,
    PDWORD pdwNumCols,
    PSTR** pppszResult
    );

DWORD
LwEvtDbCreateDB(
    BOOLEAN replaceDB
    );

#endif /* __EVTDB_H__ */
