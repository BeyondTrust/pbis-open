/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

#include "includes.h"

#define BAIL_ON_SQLITE3_ERROR(dwError, pszError) \
    do { \
        if (dwError) \
        { \
           EVT_LOG_DEBUG("Sqlite3 error '%s' (code = %u)", \
                         LW_SAFE_LOG_STRING(pszError), dwError); \
           dwError = ERROR_BADDB; \
           BAIL_ON_EVT_ERROR(dwError); \
        } \
    } while (0)

#define DB_QUERY_CREATE_EVENTS_TABLE "CREATE TABLE lwievents               \
                         (EventRecordId integer PRIMARY KEY AUTOINCREMENT, \
                            EventTableCategoryId   varchar(128),            \
                            EventType     varchar(128),                      \
                            EventDateTime     integer,                       \
                            EventSource   varchar(128),                      \
                            EventCategory varchar(128),                      \
                            EventSourceId      integer,                      \
                            User          varchar(128),                      \
                            Computer      varchar(128),                      \
                            Description   TEXT,                              \
                            Data          varchar(128)                       \
                         )"


#define DB_QUERY_CREATE_UNIQUE_INDEX "CREATE UNIQUE INDEX lwindex_%s ON lwievents(%s)"

#define DB_QUERY_CREATE_INDEX "CREATE INDEX lwindex_%s ON lwievents(%s)"

#define DB_QUERY_ALL_WITH_LIMIT L"SELECT EventRecordId,    \
                                    EventTableCategoryId, \
                                    EventType,            \
                                    EventDateTime,        \
                                    EventSource,          \
                                    EventCategory,        \
                                    EventSourceId,        \
                                    User,                 \
                                    Computer,             \
                                    Description,          \
                                    Data                  \
                             FROM     lwievents           \
                             ORDER BY EventRecordId ASC  \
                             LIMIT %ld"



#define DB_QUERY_WITH_LIMIT L"SELECT EventRecordId,        \
                                    EventTableCategoryId, \
                                    EventType,            \
                                    EventDateTime,        \
                                    EventSource,          \
                                    EventCategory,        \
                                    EventSourceId,        \
                                    User,                 \
                                    Computer,             \
                                    Description,          \
                                    Data                  \
                             FROM     lwievents           \
                             WHERE  (%ws)                  \
                             ORDER BY EventRecordId ASC  \
                             LIMIT %ld"

#define DB_QUERY_COUNT_ALL  L"SELECT COUNT(*)  \
                             FROM     lwievents"

#define DB_QUERY_COUNT      L"SELECT COUNT(*)  \
                             FROM     lwievents           \
                             WHERE  (%ws)"

#define DB_QUERY_DROP_EVENTS_TABLE "DROP TABLE lwievents"

#define DB_QUERY_DELETE     L"DELETE FROM     lwievents    \
                             WHERE  (%ws)"

#define DB_QUERY_DELETE_ALL     L"DELETE FROM     lwievents"

#define DB_QUERY_INSERT_EVENT "INSERT INTO lwievents         \
                                     (                         \
                                        EventTableCategoryId,  \
                                        EventType,             \
                                        EventDateTime,         \
                                        EventSource,           \
                                        EventCategory,         \
                                        EventSourceId,         \
                                        User,                  \
                                        Computer,              \
                                        Description,           \
                                        Data                   \
                                     )                         \
                                VALUES                         \
                                     (                         \
                                        ?1,                    \
                                        ?2,                    \
                                        ?3,                    \
                                        ?4,                    \
                                        ?5,                    \
                                        ?6,                    \
                                        ?7,                    \
                                        ?8,                    \
                                        ?9,                    \
                                        ?10)"

//Delete the record over 'n' entries
#define DB_QUERY_DELETE_AT_LIMIT    "DELETE FROM lwievents \
                                    WHERE EventRecordId IN ( \
                                        SELECT EventRecordId \
                                        FROM lwievents \
                                        ORDER BY EventRecordId ASC \
                                        LIMIT max( \
                                            (SELECT count(*) FROM lwievents) - \
                                            %lu, 0 \
                                        ) \
                                    )"

//Delete the first 'n' entries
#define DB_QUERY_DELETE_COUNT       "DELETE FROM     lwievents      \
                                     WHERE  EventRecordId IN (      \
                                       SELECT EventRecordId         \
                                       FROM     lwievents           \
                                       ORDER BY EventRecordId ASC   \
                                       LIMIT %lu                    \
                                     )"

//To delete records 'n' days older than current date.
#define DB_QUERY_DELETE_OLDER_THAN  "DELETE FROM     lwievents    \
                                     WHERE  (EventDateTime < strftime('%%s', 'now', '-%d day'))"

//To get count of records that are older  than 'n' days
#define DB_QUERY_COUNT_OLDER_THAN  "SELECT COUNT (EventRecordId) FROM  lwievents    \
                                     WHERE  (EventDateTime < strftime('%%s', 'now','-%d day'))"

//To sort the record depending upon the date
#define DB_QUERY_SORT_ON_DATE       "SELECT (*) FROM  lwievents    \
                                     ORDER BY EventDateTime desc "

//To get records 'n' days older than current date
#define DB_QUERY_GET_OLDER_THAN  "SELECT (*) FROM  lwievents    \
                                    WHERE  (EventDateTime < strftime('%%s', 'now','-%d day'))"
//Function prototype

static
DWORD
LwEvtDbCheckSqlFilter(
    PCWSTR pFilter
    );

static
DWORD
LwEvtDbMaintainDB_inlock(
    sqlite3 *pDb
    );

static
DWORD
LwEvtDbEventLogCountOlderThan_inlock(
    sqlite3 *pDb,
    DWORD dwOlderThan,
    PDWORD pdwNumMatched
    );

static
DWORD
LwEvtDbLimitDatabaseSize_inlock(
    sqlite3 *pDb,
    DWORD dwMaxLogSize
    );

static
DWORD
LwEvtDbDeleteOlderThanCurDate_inlock(
    sqlite3 *pDb,
    DWORD dwOlderThan
    );

static
DWORD
LwEvtDbDeleteIfCountExceeds_inlock(
    sqlite3 *pDb,
    DWORD dwOlderThan
    );

static
DWORD
LwEvtDbGetRecordCount_inlock(
    sqlite3 *pDb,
    const WCHAR * pSqlFilter,
    DWORD * pNumMatched
    );

//public interface

DWORD
LwEvtDbInitEventDatabase()
{
    pthread_rwlock_init(&g_dbLock, NULL);
    return 0;
}

DWORD
LwEvtDbShutdownEventDatabase()
{
    return 0;
}

DWORD
LwEvtDbOpen(
    sqlite3** ppDb
    )
{
    DWORD dwError = 0;
    sqlite3* pDb = NULL;

    dwError = sqlite3_open(EVENTLOG_DB, &pDb);
    BAIL_ON_EVT_ERROR(dwError);

    *ppDb = pDb;

cleanup:
    return dwError;

error:
    if (pDb)
    {
        sqlite3_close(pDb);
    }
    *ppDb = NULL;
    goto cleanup;
}

DWORD
LwEvtDbClose(
    sqlite3* pDb
    )
{
    DWORD dwError = 0;

    if (pDb)
    {
        dwError = sqlite3_close(pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

static
DWORD
LwEvtDbCheckSqlFilter(
    PCWSTR pFilter
    )
{
    enum {
        COMMAND,
        SINGLE_QUOTE,
        DOUBLE_QUOTE,
        BACK_QUOTE,
    } mode = COMMAND;
    DWORD dwError = 0;
    DWORD dwIndex = 0;

    while (pFilter[dwIndex])
    {
        switch(mode)
        {
            case COMMAND:
                switch (pFilter[dwIndex])
                {
                    case ';':
                        dwError = ERROR_INVALID_PARAMETER;
                        BAIL_ON_EVT_ERROR(dwError);
                        break;
                    case '\'':
                        mode = SINGLE_QUOTE;
                        break;
                    case '\"':
                        mode = DOUBLE_QUOTE;
                        break;
                    case '`':
                        mode = BACK_QUOTE;
                        break;
                }
                break;
            case SINGLE_QUOTE:
                switch (pFilter[dwIndex])
                {
                    case '\'':
                        mode = COMMAND;
                        break;
                }
                break;
            case DOUBLE_QUOTE:
                switch (pFilter[dwIndex])
                {
                    case '\"':
                        mode = COMMAND;
                        break;
                }
                break;
            case BACK_QUOTE:
                switch (pFilter[dwIndex])
                {
                    case '`':
                        mode = COMMAND;
                        break;
                }
                break;
        }
        dwIndex++;
    }

    if (mode != COMMAND)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LwEvtDbGetRecordCount(
    sqlite3 *pDb,
    const WCHAR * pSqlFilter,
    DWORD * pNumMatched
    )
{
    DWORD error = 0;
    BOOLEAN inLock = FALSE;

    ENTER_RW_READER_LOCK(inLock);

    error = LwEvtDbGetRecordCount_inlock(
                pDb,
                pSqlFilter,
                pNumMatched);

    LEAVE_RW_READER_LOCK(inLock);

    return error;
}


static
DWORD
LwEvtDbGetRecordCount_inlock(
    sqlite3 *pDb,
    const WCHAR * pSqlFilter,
    DWORD * pNumMatched
    )
{
    DWORD dwError = 0;
    PWSTR pQuery = NULL;
    sqlite3_stmt *pStatement = NULL;
    sqlite_int64 recordCount = 0;

    if (pSqlFilter == NULL)
    {
        dwError = LwAllocateWc16sPrintfW(
                        &pQuery,
                        DB_QUERY_COUNT_ALL);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        dwError = LwEvtDbCheckSqlFilter(pSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateWc16sPrintfW(
                        &pQuery,
                        DB_QUERY_COUNT,
                        pSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);
    }

    // This statement needs to be in the lock because it can fail with
    // SQLITE_BUSY if some other thread is accessing the database.
    dwError = sqlite3_prepare16_v2(
                    pDb,
                    pQuery,
                    -1,
                    &pStatement,
                    NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));

    dwError = sqlite3_step(pStatement);
    if (dwError == SQLITE_ROW)
    {
        dwError = 0;
        if (sqlite3_column_count(pStatement) != 1)
        {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_EVT_ERROR(dwError);
        }
        recordCount = sqlite3_column_int64(pStatement, 0);
    }
    else if (dwError == SQLITE_DONE || dwError == SQLITE_OK)
    {
        EVT_LOG_VERBOSE("Could not find count of event logs in database");
        dwError = ERROR_BADDB;
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
    }

    if ((DWORD)recordCount != recordCount)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        BAIL_ON_EVT_ERROR(dwError);
    }
    *pNumMatched = (DWORD)recordCount;

cleanup:
    // sqlite3 API docs say passing NULL is okay
    sqlite3_finalize(pStatement);

    LW_SAFE_FREE_MEMORY(pQuery);

    return dwError;

error:
    *pNumMatched = 0;
    goto cleanup;
}

static
DWORD
LwEvtDbReadString(
    IN DWORD (*pAllocate)(DWORD, PVOID*),
    IN VOID (*pFree)(PVOID),
    IN sqlite3_stmt *pStatement,
    IN int ColumnPos,
    IN PCSTR pColumnName,
    OUT PWSTR *ppResult
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    //Do not free
    PCWSTR pColumnValue = (PCWSTR)sqlite3_column_text16(pStatement, ColumnPos);
    PWSTR pResult = NULL;

    if (strcmp(sqlite3_column_name(pStatement, ColumnPos), pColumnName))
    {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (pColumnValue)
    {
        dwError = LwAllocateWc16String(
                        &pResult,
                        pColumnValue);
        BAIL_ON_EVT_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:
    return dwError;

error:
    *ppResult = NULL;
    pFree(pResult);
    goto cleanup;
}

static
DWORD
LwEvtDbReadBlob(
    IN DWORD (*pAllocate)(DWORD, PVOID*),
    IN VOID (*pFree)(PVOID),
    IN sqlite3_stmt *pStatement,
    IN int ColumnPos,
    IN PCSTR pColumnName,
    OUT PDWORD pLen,
    OUT PBYTE* ppData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    //Do not free
    const BYTE *pColumnValue = sqlite3_column_blob(pStatement, ColumnPos);
    PBYTE pResult = NULL;
    int len = 0;

    if (strcmp(sqlite3_column_name(pStatement, ColumnPos), pColumnName))
    {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_EVT_ERROR(dwError);
    }

    len = sqlite3_column_bytes(pStatement, ColumnPos);
    if ((DWORD)len != len)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = pAllocate(
                    sizeof(pResult[0]) * len,
                    (PVOID*)&pResult);
    BAIL_ON_EVT_ERROR(dwError);
    memcpy(pResult, pColumnValue, sizeof(pResult[0]) * len);

    *pLen = len;
    *ppData = pResult;

cleanup:
    return dwError;

error:
    *pLen = 0;
    *ppData = NULL;
    pFree(pResult);
    goto cleanup;
}

VOID
LwEvtDbFreeRecord(
    IN VOID (*pFree)(PVOID),
    IN PLW_EVENTLOG_RECORD pRecord
    )
{
    PVOID* ppPointers[] = {
        (PVOID *)&pRecord->pLogname,
        (PVOID *)&pRecord->pEventType,
        (PVOID *)&pRecord->pEventSource,
        (PVOID *)&pRecord->pEventCategory,
        (PVOID *)&pRecord->pUser,
        (PVOID *)&pRecord->pComputer,
        (PVOID *)&pRecord->pDescription,
        (PVOID *)&pRecord->pData,
    };
    DWORD index = 0;

    for (index = 0; index < sizeof(ppPointers)/sizeof(ppPointers[0]); index++)
    {
        if (*ppPointers[index])
        {
            pFree(*ppPointers[index]);
            *ppPointers[index] = NULL;
        }
    }
}

static
DWORD
LwEvtDbUnpackRecord(
    IN DWORD (*pAllocate)(DWORD, PVOID*),
    IN VOID (*pFree)(PVOID),
    IN sqlite3_stmt *pStatement,
    OUT PLW_EVENTLOG_RECORD pRecord
    )
{
    int column = 0;
    DWORD dwError = LW_ERROR_SUCCESS;

    pRecord->EventRecordId = sqlite3_column_int64(pStatement, column++);

    dwError = LwEvtDbReadString(
                    pAllocate,
                    pFree,
                    pStatement,
                    column++,
                    "EventTableCategoryId",
                    &pRecord->pLogname);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbReadString(
                    pAllocate,
                    pFree,
                    pStatement,
                    column++,
                    "EventType",
                    &pRecord->pEventType);
    BAIL_ON_EVT_ERROR(dwError);

    pRecord->EventDateTime = sqlite3_column_int64(pStatement, column++);

    dwError = LwEvtDbReadString(
                    pAllocate,
                    pFree,
                    pStatement,
                    column++,
                    "EventSource",
                    &pRecord->pEventSource);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbReadString(
                    pAllocate,
                    pFree,
                    pStatement,
                    column++,
                    "EventCategory",
                    &pRecord->pEventCategory);
    BAIL_ON_EVT_ERROR(dwError);

    pRecord->EventSourceId = sqlite3_column_int(pStatement, column++);

    dwError = LwEvtDbReadString(
                    pAllocate,
                    pFree,
                    pStatement,
                    column++,
                    "User",
                    &pRecord->pUser);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbReadString(
                    pAllocate,
                    pFree,
                    pStatement,
                    column++,
                    "Computer",
                    &pRecord->pComputer);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbReadString(
                    pAllocate,
                    pFree,
                    pStatement,
                    column++,
                    "Description",
                    &pRecord->pDescription);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbReadBlob(
                    pAllocate,
                    pFree,
                    pStatement,
                    column++,
                    "Data",
                    &pRecord->DataLen,
                    &pRecord->pData);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    return dwError;

error:
    LwEvtDbFreeRecord(pFree, pRecord);
    goto cleanup;
}

DWORD
LwEvtDbReadRecords(
    DWORD (*pAllocate)(DWORD, PVOID*),
    VOID (*pFree)(PVOID),
    sqlite3 *pDb,
    DWORD MaxResults,
    PCWSTR pSqlFilter,
    PDWORD pCount,
    PLW_EVENTLOG_RECORD* ppRecords
    )
{
    DWORD dwError = 0;
    PLW_EVENTLOG_RECORD pRecords = NULL;
    PLW_EVENTLOG_RECORD pNewRecords = NULL;
    DWORD count = 0;
    DWORD capacity = 0;
    sqlite3_stmt *pStatement = NULL;
    PWSTR pQuery = NULL;
    BOOLEAN inLock = FALSE;

    if (pSqlFilter == NULL)
    {
        dwError = LwAllocateWc16sPrintfW(
                        &pQuery,
                        DB_QUERY_ALL_WITH_LIMIT,
                        MaxResults);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        dwError = LwEvtDbCheckSqlFilter(pSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateWc16sPrintfW(
                        &pQuery,
                        DB_QUERY_WITH_LIMIT,
                        pSqlFilter,
                        MaxResults);
        BAIL_ON_EVT_ERROR(dwError);
    }

    ENTER_RW_READER_LOCK(inLock);

    // This statement needs to be in the lock because it can fail with
    // SQLITE_BUSY if some other thread is accessing the database.
    dwError = sqlite3_prepare16_v2(
                    pDb,
                    pQuery,
                    -1,
                    &pStatement,
                    NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));

    while (1)
    {
        dwError = sqlite3_step(pStatement);
        if (dwError == SQLITE_DONE || dwError == SQLITE_OK)
        {
            dwError = 0;
            break;
        }
        else if (dwError == SQLITE_ROW)
        {
            dwError = 0;
        }
        else
        {
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        }

        // Resize array if necessary
        if (count >= capacity)
        {
            capacity = (count + 10) * 2;
            if (capacity > MaxResults)
            {
                capacity = MaxResults;
            }
            if (count >= capacity)
            {
                dwError = ERROR_ARITHMETIC_OVERFLOW;
                BAIL_ON_EVT_ERROR(dwError);
            }

            dwError = pAllocate(
                            sizeof(pRecords[0]) * capacity,
                            (PVOID*)&pNewRecords);
            BAIL_ON_EVT_ERROR(dwError);

            memcpy(pNewRecords, pRecords, sizeof(pRecords[0]) * count);
            pFree(pRecords);
            pRecords = pNewRecords;
            pNewRecords = NULL;
        }

        dwError = LwEvtDbUnpackRecord(
                        pAllocate,
                        pFree,
                        pStatement,
                        &pRecords[count]);
        BAIL_ON_EVT_ERROR(dwError);
        count++;
    }

    *pCount = count;
    *ppRecords = pRecords;

cleanup:
    // sqlite3 API docs say passing NULL is okay
    sqlite3_finalize(pStatement);
    LEAVE_RW_READER_LOCK(inLock);

    LW_SAFE_FREE_MEMORY(pQuery);
    pFree(pNewRecords);
    return dwError;

error:
    *pCount = 0;
    *ppRecords = NULL;
    while (count)
    {
        count--;
        LwEvtDbFreeRecord(pFree, &pRecords[count]);
    }
    pFree(pRecords);
    goto cleanup;
}


DWORD
LwEvtDbWriteRecords(
    sqlite3 *pDb,
    DWORD Count,
    const LW_EVENTLOG_RECORD *pRecords 
    )
{
    DWORD dwError = 0;
    sqlite3_stmt *pStatement = NULL;
    int iColumnPos = 1;
    const LW_EVENTLOG_RECORD* pRecord = NULL;
    DWORD index = 0;
    PSTR pError = NULL;
    BOOLEAN inLock = FALSE;
    BOOLEAN removeAsNeeded = FALSE;

    EVT_LOG_VERBOSE("server::evtdb.c Writing %u records (pDb=%.16X)\n",
                    Count, pDb);

    ENTER_RW_WRITER_LOCK(inLock);

    // This statement needs to be in the lock because it can fail with
    // SQLITE_BUSY if some other thread is accessing the database.
    dwError = sqlite3_prepare_v2(
                    pDb,
                    DB_QUERY_INSERT_EVENT,
                    -1, //search for null termination in szQuery to get length
                    &pStatement,
                    NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));

    dwError = sqlite3_exec(
                    pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pError);
    BAIL_ON_SQLITE3_ERROR(dwError, pError);

    for (index = 0; index < Count; index++)
    {
        pRecord = &pRecords[index];
        iColumnPos = 1;

        dwError = sqlite3_reset(pStatement);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));

        dwError = sqlite3_bind_text16(
            pStatement,
            iColumnPos,
            pRecord->pLogname,
            -1,
            SQLITE_STATIC);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pStatement,
            iColumnPos,
            pRecord->pEventType,
            -1,
            SQLITE_STATIC);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_int64(
            pStatement,
            iColumnPos,
            pRecord->EventDateTime);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pStatement,
            iColumnPos,
            pRecord->pEventSource,
            -1,
            SQLITE_STATIC);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pStatement,
            iColumnPos,
            pRecord->pEventCategory,
            -1,
            SQLITE_STATIC);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_int64(
            pStatement,
            iColumnPos,
            pRecord->EventSourceId);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pStatement,
            iColumnPos,
            pRecord->pUser,
            -1,
            SQLITE_STATIC);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pStatement,
            iColumnPos,
            pRecord->pComputer,
            -1,
            SQLITE_STATIC);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pStatement,
            iColumnPos,
            pRecord->pDescription,
            -1,
            SQLITE_STATIC);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = sqlite3_bind_blob(
            pStatement,
            iColumnPos,
            pRecord->pData,
            pRecord->DataLen,
            SQLITE_STATIC);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
        iColumnPos++;

        dwError = (DWORD)sqlite3_step(pStatement);
        if (dwError == SQLITE_DONE)
        {
            dwError = 0;
        }
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
    }

    dwError = sqlite3_exec(
                    pDb,
                    "end",
                    NULL,
                    NULL,
                    &pError);
    BAIL_ON_SQLITE3_ERROR(dwError, pError);

    EVT_LOG_VERBOSE("Write finished");

    gdwNewEventCount += Count;
    if (gdwNewEventCount >= EVT_MAINTAIN_EVENT_COUNT)
    {
        dwError = EVTGetRemoveAsNeeded(&removeAsNeeded);
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (removeAsNeeded)
    {
        //Trim the DB
        dwError = LwEvtDbMaintainDB_inlock(pDb);
        BAIL_ON_EVT_ERROR(dwError);

        gdwNewEventCount = 0;
    }

 cleanup:
    if (pStatement)
    {
        sqlite3_finalize(pStatement);
    }
    LEAVE_RW_WRITER_LOCK(inLock);
    return dwError;

 error:

    if (pError)
    {
        sqlite3_free(pError);
    }
    sqlite3_exec(
                    pDb,
                    "rollback",
                    NULL,
                    NULL,
                    NULL);

    goto cleanup;
}

/* A routine to trim the database*/
static
DWORD
LwEvtDbMaintainDB_inlock(
    sqlite3 *pDb
    )
{
    DWORD dwError = 0;
    DWORD dwRecordCount = 0;
    DWORD dwCountOlderThan = 0;
    DWORD dwMaxRecords = 0;
    DWORD dwMaxAge = 0;
    DWORD dwMaxLogSize = 0;
    DWORD dwActualSize = 0;

    EVT_LOG_VERBOSE("In Maintain DB ...............");
    //Get Max records,max age and max log size from the global list
    dwError = EVTGetMaxRecords(&dwMaxRecords);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTGetMaxAge(&dwMaxAge);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTGetMaxLogSize(&dwMaxLogSize);
    BAIL_ON_EVT_ERROR(dwError);

    //Get the count of the records
    dwError = LwEvtDbGetRecordCount_inlock(pDb, NULL, &dwRecordCount);
    BAIL_ON_EVT_ERROR(dwError);

    //Get the count of the records which are older than 'n' days
    dwError = LwEvtDbEventLogCountOlderThan_inlock(
                  pDb,
                  dwMaxAge,
                  &dwCountOlderThan);
    BAIL_ON_EVT_ERROR(dwError);

    //Get the File size
    dwError = EVTGetFileSize(EVENTLOG_DB,&dwActualSize);
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_VERBOSE("Max Records = %d",dwMaxRecords);
    EVT_LOG_VERBOSE("Max Age = %d",dwMaxAge);
    EVT_LOG_VERBOSE("Max Log size = %d",dwMaxLogSize);
    EVT_LOG_VERBOSE("EventLog record count older than curdate = %d",dwCountOlderThan);
    EVT_LOG_VERBOSE("EventLog Record count = %d",dwRecordCount);
    EVT_LOG_VERBOSE("Actual Log size = %d ",dwActualSize);

    //Regular house keeping
    if (dwCountOlderThan) {
        EVT_LOG_VERBOSE("Deleting the record as older than current date");
        dwError = LwEvtDbDeleteOlderThanCurDate_inlock(pDb, dwMaxAge);
        BAIL_ON_EVT_ERROR(dwError);

        //Since already some records got deleted get the latest record count
        dwError = LwEvtDbGetRecordCount_inlock(pDb, NULL, &dwRecordCount);
        BAIL_ON_EVT_ERROR(dwError);
    }

    //If the record count is greater than the Max Records
    if (dwRecordCount >= dwMaxRecords) {

        EVT_LOG_VERBOSE("Record Count = %d which is more than max records set = %d",dwRecordCount,dwMaxRecords);
        EVT_LOG_VERBOSE("DB size exceeds the Max Records Size");
        EVT_LOG_VERBOSE("Going to trim DB.........");

        //If records not aged out,then delete the records above Max Records
        dwError = LwEvtDbDeleteIfCountExceeds_inlock(pDb, dwMaxRecords);
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (dwActualSize >= dwMaxLogSize)
    {
        EVT_LOG_VERBOSE("Log Size is exceeds the maximum limit set");
        dwError = LwEvtDbLimitDatabaseSize_inlock(pDb, dwMaxLogSize);
        BAIL_ON_EVT_ERROR(dwError);
    }
	
    EVT_LOG_VERBOSE("Pruned DB returning");

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LwEvtDbDeleteRecords(
    sqlite3 *pDb,
    PCWSTR pSqlFilter
    )
{
    DWORD dwError = 0;
    PWSTR pQuery = NULL;
    sqlite3_stmt *pStatement = NULL;
    BOOLEAN inLock = FALSE;

    if (pSqlFilter == NULL)
    {
        dwError = LwAllocateWc16sPrintfW(
                        &pQuery,
                        DB_QUERY_DELETE_ALL);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        dwError = LwEvtDbCheckSqlFilter(pSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateWc16sPrintfW(
                        &pQuery,
                        DB_QUERY_DELETE,
                        pSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);
    }

    ENTER_RW_WRITER_LOCK(inLock);

    // This statement needs to be in the lock because it can fail with
    // SQLITE_BUSY if some other thread is accessing the database.
    dwError = sqlite3_prepare16_v2(
                    pDb,
                    pQuery,
                    -1,
                    &pStatement,
                    NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));

    dwError = sqlite3_step(pStatement);
    if (dwError == SQLITE_ROW)
    {
        dwError = ERROR_BADDB;
        BAIL_ON_EVT_ERROR(dwError);
    }
    else if (dwError == SQLITE_DONE || dwError == SQLITE_OK)
    {
        dwError = 0;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb));
    }

cleanup:
    // sqlite3 API docs say passing NULL is okay
    sqlite3_finalize(pStatement);
    LEAVE_RW_WRITER_LOCK(inLock);

    LW_SAFE_FREE_MEMORY(pQuery);

    return dwError;

error:
    goto cleanup;
}

//To get count of records that are older than 'n' days.
static
DWORD
LwEvtDbEventLogCountOlderThan_inlock(
    sqlite3 *pDb,
    DWORD dwOlderThan,
    PDWORD pdwNumMatched
    )
{

    DWORD dwError = 0;
    CHAR  szQuery[8092];
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;

    sprintf(szQuery, DB_QUERY_COUNT_OLDER_THAN, dwOlderThan);

    dwError = LwEvtDbQueryEventLog(pDb, szQuery, &nRows, &nCols, &ppszResult);

    if (nRows == 1) {
        *pdwNumMatched = (DWORD) atoi(ppszResult[1]);
        BAIL_ON_EVT_ERROR(dwError);
    } else {

        EVT_LOG_VERBOSE("Could not find count of event logs in database");
    }

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }

    return dwError;
 error:
    goto cleanup;

}

static
DWORD
LwEvtDbLimitDatabaseSize_inlock(
    sqlite3 *pDb,
    DWORD dwMaxLogSize
    )
{
    DWORD dwError = 0;
    CHAR  szQuery[8092];
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD dwCurrentCount = 0;
    DWORD dwActualSize = 0;
    DWORD dwDeleteCount = 0;

    while (1)
    {
        dwError = EVTGetFileSize(EVENTLOG_DB, &dwActualSize);
        BAIL_ON_EVT_ERROR(dwError);

        if (dwActualSize < dwMaxLogSize)
        {
            // The file size is now acceptable
            break;
        }

        if (ppszResult) {
            sqlite3_free_table(ppszResult);
        }

        dwError = LwEvtDbGetRecordCount_inlock(
                        pDb,
                        NULL,
                        &dwCurrentCount);
        BAIL_ON_EVT_ERROR(dwError);

        if (dwCurrentCount == 0)
        {
            EVT_LOG_ERROR("evtdb: The current database size ( %d ) is larger than the max ( %d ), but since it contains no records, it cannot be further trimmed.", dwActualSize, dwMaxLogSize);
            break;
        }

        // Assume every record takes the same amount of space and figure out
        // how many need to be cleared. Also, clear 10% more than necessary so
        // this delete operation is not run every time.

        dwDeleteCount = dwCurrentCount -
                        9 * dwCurrentCount * dwMaxLogSize / dwActualSize / 10;
        if (dwDeleteCount < 1)
        {
            // This breaks an infinite loop that might be caused from rounding
            // errors
            dwDeleteCount = 1;
        }
        EVT_LOG_INFO("evtdb: Deleting %d record(s) (out of %d) in an attempt to lower the current database size ( %d ), to lower than %d", dwDeleteCount, dwCurrentCount, dwActualSize, dwMaxLogSize);

        if (ppszResult) {
            sqlite3_free_table(ppszResult);
        }
        sprintf(szQuery, DB_QUERY_DELETE_COUNT, (unsigned long)dwDeleteCount);

        dwError = LwEvtDbQueryEventLog(pDb, szQuery, &nRows, &nCols, &ppszResult);
        BAIL_ON_EVT_ERROR(dwError);

        // Run the vacuum command so the filesize actually shrinks
        if (ppszResult) {
            sqlite3_free_table(ppszResult);
        }
        dwError = LwEvtDbQueryEventLog(pDb, "VACUUM", &nRows, &nCols, &ppszResult);
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }

    return dwError;

error:
    goto cleanup;
}

//Delete the record over 'n' entries
static
DWORD
LwEvtDbDeleteIfCountExceeds_inlock(
    sqlite3 *pDb,
    DWORD dwOlderThan
    )
{

    DWORD dwError = 0;
    CHAR  szQuery[8092];
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;

    // Delete 10 extra records so we only need to trim on 1/10 of the runs.
    if (dwOlderThan > 10)
    {
        dwOlderThan -= 10;
    }

    sprintf(szQuery, DB_QUERY_DELETE_AT_LIMIT, (unsigned long)dwOlderThan);

    dwError = LwEvtDbQueryEventLog(pDb, szQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }

    return dwError;
 error:
    goto cleanup;

}

//To delete records 'n' days older than current date.
static
DWORD
LwEvtDbDeleteOlderThanCurDate_inlock(
    sqlite3 *pDb,
    DWORD dwOlderThan
    )
{
    DWORD dwError = 0;
    CHAR  szQuery[8092];
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;

    sprintf(szQuery, DB_QUERY_DELETE_OLDER_THAN, dwOlderThan);

    dwError = LwEvtDbQueryEventLog(pDb, szQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }

    return dwError;
 error:
    goto cleanup;

}

//helper functions

/*
 * Warning: this function should be surrounded by a lock;
 * sqlite3_free_table(ppszResult) should be called after the
 * function call, within the critical section.
 */
DWORD
LwEvtDbQueryEventLog(
    sqlite3* pDb,
    PSTR szQuery,
    PDWORD pdwNumRows,
    PDWORD pdwNumCols,
    PSTR** pppszResult
    )
{
    DWORD dwError = 0;
    PSTR  pszError = NULL;

    INT numRowsLocal = 0;
    INT numColsLocal = 0;

    if (!pDb) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    EVT_LOG_INFO("evtdb: LwEvtDbQueryEventLog: query={%s}\n\n", szQuery);



    dwError = sqlite3_get_table(pDb,
                                szQuery,
                                pppszResult,
                                (INT*) &numRowsLocal,
                                (INT*) &numColsLocal,
                                (PSTR*) &pszError
                                );

    if (dwError) {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszError)) {
            EVT_LOG_ERROR(pszError);
        }
        BAIL_ON_EVT_ERROR(dwError);
    }

    *pdwNumRows = (DWORD)numRowsLocal;
    *pdwNumCols = (DWORD)numColsLocal;

 error:

    return dwError;

}

DWORD
LwEvtDbCreateDB(BOOLEAN replaceDB)
{
    DWORD dwError = 0;
    sqlite3* pSqliteHandle = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;

    CHAR szQuery[1024];

    dwError = LwCheckFileTypeExists(EVENTLOG_DB, LWFILE_REGULAR, &bExists);
    BAIL_ON_EVT_ERROR(dwError);

    if (bExists)
    {
        if (replaceDB)
        {
            dwError = LwRemoveFile(EVENTLOG_DB);
            BAIL_ON_EVT_ERROR(dwError);
        }
        else return 0;
    }

    dwError = LwCheckFileTypeExists(
                    EVENTLOG_DB_DIR,
                    LWFILE_DIRECTORY,
                    &bExists);
    BAIL_ON_EVT_ERROR(dwError);

    if (!bExists)
    {
        dwError = LwCreateDirectory(EVENTLOG_DB_DIR, S_IRWXU);
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = sqlite3_open(EVENTLOG_DB, &pSqliteHandle);
    BAIL_ON_EVT_ERROR(dwError);
   
    dwError = sqlite3_exec(pSqliteHandle,
                            DB_QUERY_CREATE_EVENTS_TABLE,                           
                            NULL,
                            NULL,
                            &pszError);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_UNIQUE_INDEX, "recordId", "EventRecordId");
    dwError = sqlite3_exec(pSqliteHandle,
                            szQuery,
                            NULL,
                            NULL,
                            &pszError);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_INDEX, "tableCategoryId", "EventTableCategoryId");
    dwError = sqlite3_exec(pSqliteHandle,
                            szQuery,
                            NULL,
                            NULL,
                            &pszError);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_INDEX, "dateTime", "EventDateTime");
    dwError = sqlite3_exec(pSqliteHandle,
                            szQuery,
                            NULL,
                            NULL,
                            &pszError);
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_VERBOSE("New database file %s with TABLE lwievents successfully created.\n", EVENTLOG_DB);

cleanup:

    if (pSqliteHandle)
        sqlite3_close(pSqliteHandle);

    return dwError;

error:

    if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
        EVT_LOG_ERROR(pszError);

    goto cleanup;
}
