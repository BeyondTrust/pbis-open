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
