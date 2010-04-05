/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "server.h"


typedef enum
{
    ColEventRecordId = 0,
    EventRecordId,                //1
    EventLogname,               //2
    EventType,                    //3
    EventDateTime,              //4
    EventSource,                //5
    EventCategory,              //6
    EventSourceId,              //7
    User,                       //8
    Computer,                   //9
    Description,                //10
    EVENT_DB_COL_SENTINEL
} EventDbColumnType;

// database schema version. Change this whenever the shape of the database changes
#define SCHEMA_VERSION "-7"

    DWORD dwDataLen;
#if defined(_DCE_IDL_) || defined(__midl)
    [size_is(dwDataLen)]
#endif
    BYTE *pvData;
#define DB_QUERY_CREATE_EVENTS_TABLE                                            \
                            "CREATE TABLE lwievents (                           \
                            ColEventRecordId INTEGER PRIMARY KEY AUTOINCREMENT, \
                            ColDateTime integer,                                \
                            ColComputer text,                                   \
                            ColComputerAddress text,                            \
                            EventRecordId bigint,                               \
                            Logname text,                                       \
                            EventType     text,                                 \
                            EventDateTime     integer,                          \
                            EventSource   text,                                 \
                            EventCategory text,                                 \
                            EventSourceId      integer,                         \
                            User          text,                                 \
                            Computer      text,                                 \
                            Description   text,                                 \
                            Data          blob                                  \
                         )"

#define DB_QUERY_CREATE_CONFIG_TABLE "CREATE TABLE Configuration             \
                           (Keyword       varchar(128),                      \
                            Value         varchar(256)                       \
                         )"

#define DB_QUERY_CREATE_UNIQUE_INDEX "CREATE UNIQUE INDEX lwindex_%hhs ON lwievents(%hhs)"

#define DB_QUERY_CREATE_INDEX "CREATE INDEX lwindex_%hhs ON lwievents(%hhs)"

#define DB_QUERY_ALL_WITH_LIMIT "SELECT                      \
                             ColEventRecordId,               \
                             ColDateTime,                    \
                             ColComputer,                    \
                             ColComputerAddress,             \
                             EventRecordId,                  \
                             Logname,                        \
                             EventType,                      \
                             EventDateTime,                  \
                             EventSource,                    \
                             EventCategory,                  \
                             EventSourceId,                  \
                             User,                           \
                             Computer,                       \
                             Description,                    \
                             Data                            \
                             FROM     lwievents              \
                             WHERE (ColEventRecordId > %llu) \
                             ORDER BY ColEventRecordId ASC   \
                             LIMIT %ld"

#define DB_QUERY_WITH_LIMIT "SELECT                       \
                             ColEventRecordId,               \
                             ColDateTime,                    \
                             ColComputer,                    \
                             ColComputerAddress,             \
                             EventRecordId,                  \
                             Logname,                        \
                             EventType,                      \
                             EventDateTime,                  \
                             EventSource,                    \
                             EventCategory,                  \
                             EventSourceId,                  \
                             User,                           \
                             Computer,                       \
                             Description,                    \
                             Data                            \
                             FROM     lwievents              \
                             WHERE  (%ws) AND                 \
                                    (ColEventRecordId > %llu)\
                             ORDER BY ColEventRecordId ASC   \
                             LIMIT %ld"

#define DB_QUERY_COUNT_ALL  "SELECT COUNT(ColEventRecordId)  \
                             FROM     lwievents"

#define DB_QUERY_COUNT      "SELECT COUNT(ColEventRecordId)  \
                             FROM     lwievents           \
                             WHERE  (%ws)"

#define DB_QUERY_DROP_EVENTS_TABLE "DROP TABLE lwievents"

#define DB_QUERY_DELETE     "DELETE FROM     lwievents    \
                             WHERE  (%ws)"

#define DB_QUERY_INSERT_CONFIG "INSERT INTO Configuration (Keyword, Value) VALUES (\"%hhs\", \"%hhs\")"
#define DB_QUERY_CONFIG_VALUE "SELECT Value FROM Configuration WHERE (Keyword=\"%hhs\")"
#define DB_QUERY_UPDATE_CONFIG "UPDATE Configuration SET Value=\"%hhs\" WHERE Keyword=\"%hhs\""

#define DB_QUERY_INSERT_EVENT "INSERT INTO lwievents (         \
                                        ColDateTime,           \
                                        ColComputer,           \
                                        ColComputerAddress,    \
                                        EventRecordId,         \
                                        Logname,               \
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
                                VALUES( ?1,                    \
                                        ?2,                    \
                                        ?3,                    \
                                        ?4,                    \
                                        ?5,                    \
                                        ?6,                    \
                                        ?7,                    \
                                        ?8,                    \
                                        ?9,                    \
                                        ?10,                   \
                                        ?11,                   \
                                        ?12,                   \
                                        ?13,                   \
                                        ?14                    \
                                     )"

//Delete the record over 'n' entries
#define DB_QUERY_DELETE_ABOVE_LIMIT "DELETE FROM     lwievents    \
                                     WHERE  EventRecordId > (%d)"

//To delete records 'n' days older than current date.
#define DB_QUERY_DELETE_OLDER_THAN  "DELETE FROM     lwievents    \
                                     WHERE  (datetime(EventDateTime,'unixepoch','localtime') < datetime('now','-%d day'))"

//To get count of records that are older  than 'n' days
#define DB_QUERY_COUNT_OLDER_THAN  "SELECT COUNT (ColEventRecordId) FROM  lwievents    \
                                     WHERE  (datetime(EventDateTime,'unixepoch','localtime') < datetime('now','-%d day'))"

//To sort the record depending upon the date
#define DB_QUERY_SORT_ON_DATE       "SELECT (*) FROM  lwievents    \
                                     ORDER BY EventDateTime desc "

//To get records 'n' days older than current date
#define DB_QUERY_GET_OLDER_THAN  "SELECT (*) FROM  lwievents    \
                                    WHERE  (datetime(EventDateTime,'unixepoch','localtime') < datetime('now','-%d day'))"

#define CLTR_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

//Function prototype
static
DWORD
SrvUnpackCollectorRecord(
    IN sqlite3_stmt* pstQuery,
    IN OUT LWCOLLECTOR_RECORD* pRecord
    );

//public interface

void
SrvRPCFreeRecordList(
    DWORD cRecords,
    PLWCOLLECTOR_RECORD pRecordList
    )
{
    DWORD iRecord = 0;
    for (iRecord = 0; iRecord < cRecords; iRecord++)
    {
        SrvRPCFreeRecordContents(&pRecordList[iRecord]);
    }
    RPCFreeMemory(pRecordList);
}

void
SrvRPCFreeRecordContents(
    PLWCOLLECTOR_RECORD pRecord
    )
{
    RPCFreeString(pRecord->pwszColComputer);
    RPCFreeString(pRecord->pwszColComputerAddress);

    RPCFreeString(pRecord->event.pwszLogname);
    RPCFreeString(pRecord->event.pwszEventType);
    RPCFreeString(pRecord->event.pwszEventSource);
    RPCFreeString(pRecord->event.pwszEventCategory);
    RPCFreeString(pRecord->event.pwszUser);
    RPCFreeString(pRecord->event.pwszComputer);
    RPCFreeString(pRecord->event.pwszDescription);
    RPCFreeMemory(pRecord->event.pvData);
}

DWORD
SrvOpenEventDatabase(
    PHANDLE phDB
    )
{
    DWORD dwError = 0;
    PEVENTLOG_CONTEXT pEventLogCtx = NULL;
    sqlite3* pSqliteHandle = NULL;
    PSTR pszQuery = NULL;
    PSTR  pszError = NULL;
    INT numRowsLocal = 0;
    INT numColsLocal = 0;
    PSTR *ppszResult = NULL;
    PWSTR pwszDbPath = NULL;

    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);
    dwError = CltrGetDatabasePath(&pwszDbPath);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = sqlite3_open16(pwszDbPath, &pSqliteHandle);
    BAIL_ON_CLTR_ERROR(dwError);

    // verify the schema version
    dwError = LwRtlCStringAllocatePrintf(
                    &pszQuery,
                    DB_QUERY_CONFIG_VALUE,
                    "SchemaVersion");
    BAIL_ON_CLTR_ERROR(dwError);

    while(1)
    {
        if (pszError != NULL)
        {
            sqlite3_free(pszError);
            pszError = NULL;
        }
        dwError = sqlite3_get_table( pSqliteHandle,
                        pszQuery,
                        &ppszResult,
                        (INT*) &numRowsLocal,
                        (INT*) &numColsLocal,
                        (PSTR*) &pszError);
        if (dwError == SQLITE_BUSY)
        {
            CLTR_LOG_VERBOSE("Collector database busy. Retrying.");
            continue;
        }
        BAIL_ON_SQLITE3_ERROR(dwError, pszError);
        break;
    }
    if (numRowsLocal == 1) {
        if( strcmp(ppszResult[1], SCHEMA_VERSION)!=0)
            CLTR_LOG_VERBOSE("Event database has wrong schema");
    } else {
        CLTR_LOG_VERBOSE("Unable to retrieve database schema version");
    }


    dwError = CltrAllocateMemory(sizeof(EVENTLOG_CONTEXT),
                                (PVOID*)&pEventLogCtx);
    BAIL_ON_CLTR_ERROR(dwError);

    pEventLogCtx->pDbHandle = pSqliteHandle;
    pSqliteHandle = NULL;

    *phDB = (HANDLE)pEventLogCtx;

cleanup:
    LwRtlCStringFree(&pszQuery);
    CLTR_SAFE_FREE_STRING(pwszDbPath);

    if (ppszResult)
        sqlite3_free_table(ppszResult);

    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

error:
    if (pszError != NULL)
    {
        sqlite3_free(pszError);
    }

    if (pSqliteHandle)
        sqlite3_close(pSqliteHandle);

    if (pEventLogCtx)
        CltrFreeMemory(pEventLogCtx);

    goto cleanup;
}

DWORD
SrvCloseEventDatabase(
    HANDLE hDB
    )
{
    DWORD dwError =0;
    PEVENTLOG_CONTEXT pContext = (PEVENTLOG_CONTEXT)(hDB);

    if (pContext) {

        if (pContext->pDbHandle != NULL) {
            sqlite3_close(pContext->pDbHandle);
        }
        CLTR_LOG_VERBOSE("Freeing the context.................");
        CltrFreeMemory(pContext);
    }

    return dwError;
}

DWORD
SrvEventLogCount(
    HANDLE hDB,
    PWSTR sqlFilter,
    PDWORD pdwNumMatched
    )
{

    DWORD dwError = 0;
    PWSTR pwszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    ENTER_RW_READER_LOCK;

    if (sqlFilter == NULL)
    {
        dwError = LwRtlWC16StringAllocatePrintfW(
                        &pwszQuery,
                        TEXT(DB_QUERY_COUNT_ALL));
        BAIL_ON_CLTR_ERROR(dwError);
    }
    else
    {
        dwError = LwRtlWC16StringAllocatePrintfW(
                        &pwszQuery,
                        TEXT(DB_QUERY_COUNT),
                        sqlFilter);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError = SrvQueryEventLog(hDB, pwszQuery, &nRows, &nCols, &ppszResult);

    if (nRows == 1) {
        *pdwNumMatched = (DWORD) atoi((const char*)ppszResult[1]);
        BAIL_ON_CLTR_ERROR(dwError);
    } else {
        CLTR_LOG_VERBOSE("Could not find count of event logs in database");
    }

 cleanup:
    LwRtlWC16StringFree(&pwszQuery);
    if (ppszResult) {
        sqlite3_free_table((PSTR*)ppszResult);
    }
    LEAVE_RW_READER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

static
DWORD
SrvUnpackCollectorRecord(
    IN sqlite3_stmt* pstQuery,
    IN OUT LWCOLLECTOR_RECORD* pRecord
    )
{
    DWORD dwError = 0;
    int iColumnPos = 0;
    PCWSTR pwszText = NULL;
    WCHAR wszBlank[] = { 0 };

    memset(pRecord, 0, sizeof(*pRecord));

    if (sqlite3_column_count(pstQuery) != 15)
    {
        dwError = CLTR_ERROR_DATA_ERROR;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    pRecord->qwColRecordId = sqlite3_column_int64(
                                        pstQuery,
                                        iColumnPos);
    iColumnPos++;
    pRecord->dwColDateTime = (DWORD)sqlite3_column_int64(
                                        pstQuery,
                                        iColumnPos);
    iColumnPos++;

    pwszText = sqlite3_column_text16(pstQuery, iColumnPos);
    if (pwszText == NULL)
    {
        pwszText = wszBlank;
    }
    dwError = RPCAllocateString(
                    pwszText,
                    &pRecord->pwszColComputer);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    pwszText = sqlite3_column_text16(pstQuery, iColumnPos);
    if (pwszText == NULL)
    {
        pwszText = wszBlank;
    }
    dwError = RPCAllocateString(
                    pwszText,
                    &pRecord->pwszColComputerAddress);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    pRecord->event.qwEventRecordId = sqlite3_column_int64(
                                            pstQuery,
                                            iColumnPos);
    iColumnPos++;

    dwError = RPCAllocateString(
                    sqlite3_column_text16(pstQuery, iColumnPos),
                    &pRecord->event.pwszLogname);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    dwError = RPCAllocateString(
                    sqlite3_column_text16(pstQuery, iColumnPos),
                    &pRecord->event.pwszEventType);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    pRecord->event.dwEventDateTime = (DWORD)sqlite3_column_int64(
                                            pstQuery,
                                            iColumnPos);
    iColumnPos++;
    
    dwError = RPCAllocateString(
                    sqlite3_column_text16(pstQuery, iColumnPos),
                    &pRecord->event.pwszEventSource);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    dwError = RPCAllocateString(
                    sqlite3_column_text16(pstQuery, iColumnPos),
                    &pRecord->event.pwszEventCategory);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    pRecord->event.dwEventSourceId = (DWORD)sqlite3_column_int64(
                                            pstQuery,
                                            iColumnPos);
    iColumnPos++;

    dwError = RPCAllocateString(
                    sqlite3_column_text16(pstQuery, iColumnPos),
                    &pRecord->event.pwszUser);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    dwError = RPCAllocateString(
                    sqlite3_column_text16(pstQuery, iColumnPos),
                    &pRecord->event.pwszComputer);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    dwError = RPCAllocateString(
                    sqlite3_column_text16(pstQuery, iColumnPos),
                    &pRecord->event.pwszDescription);
    BAIL_ON_CLTR_ERROR(dwError);
    iColumnPos++;

    // Tell sqlite the next value will definitely be read as a blob so that
    // the number of bytes is correctly calculated.
    sqlite3_column_blob(pstQuery, iColumnPos);

    pRecord->event.dwDataLen = (DWORD)sqlite3_column_bytes(
                                            pstQuery,
                                            iColumnPos);

    dwError = RPCAllocateMemory(
                    pRecord->event.dwDataLen,
                    &pRecord->event.pvData);
    BAIL_ON_CLTR_ERROR(dwError);

    memcpy(
        pRecord->event.pvData,
        sqlite3_column_blob(pstQuery, iColumnPos),
        pRecord->event.dwDataLen);

cleanup:
    return dwError;

error:
    SrvRPCFreeRecordContents(pRecord);
    goto cleanup;
}

DWORD
SrvReadEventLog(
    HANDLE hDB,
    UINT64 qwStartingRowId,
    DWORD  nRecordsPerPage,
    PWSTR   sqlFilter,
    PDWORD  pdwNumReturned,
    LWCOLLECTOR_RECORD** ppEventRecords
    )
{
    DWORD dwError = 0;
    PWSTR pwszQuery = NULL;
    DWORD dwRowNum = 0;
    LWCOLLECTOR_RECORD* pEventRecords = NULL;
    // Do not free
    PEVENTLOG_CONTEXT pContext = NULL;
    sqlite3_stmt *pstQuery = NULL;
    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);

    ENTER_RW_READER_LOCK;
    CLTR_LOG_VERBOSE("%s entered lock", __FUNCTION__);

    pContext = (PEVENTLOG_CONTEXT)(hDB);
    if (!pContext->pDbHandle)
    {
        dwError = CLTR_ERROR_INVALID_DB_HANDLE;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    if (IsNullOrEmptyString(sqlFilter)) {
        dwError = LwRtlWC16StringAllocatePrintfW(
                        &pwszQuery,
                        TEXT(DB_QUERY_ALL_WITH_LIMIT),
                        qwStartingRowId,
                        (ULONG)nRecordsPerPage);
        BAIL_ON_CLTR_ERROR(dwError);
    }
    else {
        dwError = LwRtlWC16StringAllocatePrintfW(
                        &pwszQuery,
                        TEXT(DB_QUERY_WITH_LIMIT),
                        sqlFilter,
                        qwStartingRowId,
                        (ULONG)nRecordsPerPage);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError = sqlite3_prepare16_v2(
                    pContext->pDbHandle,
                    pwszQuery,
                    -1, //search for null termination in szQuery to get length
                    &pstQuery,
                    NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pContext->pDbHandle));

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = 0;
        CLTR_LOG_VERBOSE("No event logs found in the database");
    }
    else if(dwError == SQLITE_ROW)
    {
        dwError = RPCAllocateMemory(
                        sizeof(pEventRecords[0]) * nRecordsPerPage,
                        (PVOID*)&pEventRecords);
        BAIL_ON_CLTR_ERROR(dwError);

        while (1)
        {
            dwError = SrvUnpackCollectorRecord(
                            pstQuery,
                            &pEventRecords[dwRowNum]);
            BAIL_ON_CLTR_ERROR(dwError);
            dwRowNum++;

            dwError = (DWORD)sqlite3_step(pstQuery);
            if (dwError == SQLITE_DONE)
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
                BAIL_ON_CLTR_ERROR(dwError);
            }
        }
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pContext->pDbHandle));
    }

    *ppEventRecords = pEventRecords;
    *pdwNumReturned = dwRowNum;

 cleanup:
    if (pstQuery)
    {
        sqlite3_finalize(pstQuery);
    }
    LwRtlWC16StringFree(&pwszQuery);
    CLTR_LOG_VERBOSE("%s leaving lock", __FUNCTION__);
    LEAVE_RW_READER_LOCK;
    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

 error:
    if (pEventRecords)
    {
        SrvRPCFreeRecordList(
                dwRowNum,
                pEventRecords);
    }
    goto cleanup;
}


DWORD
SrvWriteEventLog(
    HANDLE hDB,
    DWORD cRecords,
    LWCOLLECTOR_RECORD* pEventRecords 
    )
{
    DWORD dwError = 0;
    sqlite3_stmt *pstQuery = NULL;
    PEVENTLOG_CONTEXT pContext = (PEVENTLOG_CONTEXT)hDB;
    int iColumnPos = 1;
    const LWCOLLECTOR_RECORD* pRecord;
    DWORD dwIndex = 0;
    PSTR pszError = NULL;

    ENTER_RW_WRITER_LOCK;

    dwError = sqlite3_prepare16_v2(
                    pContext->pDbHandle,
                    TEXT(DB_QUERY_INSERT_EVENT),
                    -1, //search for null termination in szQuery to get length
                    &pstQuery,
                    NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pContext->pDbHandle));

    dwError = sqlite3_exec(
                    pContext->pDbHandle,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);

    for (dwIndex = 0; dwIndex < cRecords; dwIndex++)
    {
        pRecord = &pEventRecords[dwIndex];
        iColumnPos = 1;

        dwError = sqlite3_reset(pstQuery);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pContext->pDbHandle));

        dwError = sqlite3_bind_int64(
            pstQuery,
            iColumnPos,
            pRecord->dwColDateTime);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->pwszColComputer,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->pwszColComputerAddress,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_int64(
            pstQuery,
            iColumnPos,
            pRecord->event.qwEventRecordId);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->event.pwszLogname,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->event.pwszEventType,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_int64(
            pstQuery,
            iColumnPos,
            pRecord->event.dwEventDateTime);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->event.pwszEventSource,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->event.pwszEventCategory,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_int64(
            pstQuery,
            iColumnPos,
            pRecord->event.dwEventSourceId);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->event.pwszUser,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->event.pwszComputer,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_text16(
            pstQuery,
            iColumnPos,
            pRecord->event.pwszDescription,
            -1,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = sqlite3_bind_blob(
            pstQuery,
            iColumnPos,
            pRecord->event.pvData,
            pRecord->event.dwDataLen,
            SQLITE_STATIC);
        BAIL_ON_CLTR_ERROR(dwError);
        iColumnPos++;

        dwError = (DWORD)sqlite3_step(pstQuery);
        if (dwError == SQLITE_DONE)
        {
            dwError = 0;
        }
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError = sqlite3_exec(
                    pContext->pDbHandle,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);

    CLTR_LOG_VERBOSE("server::evtdb.c SrvWriteEventLog() finished\n");

 cleanup:
    if (pstQuery)
    {
        sqlite3_finalize(pstQuery);
    }
    LEAVE_RW_WRITER_LOCK;
    return dwError;

 error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(
                    pContext->pDbHandle,
                    "rollback",
                    NULL,
                    NULL,
                    NULL);

    goto cleanup;
}

/* A routine to trim the database*/
static
DWORD
SrvMaintainDB(
    HANDLE hDB
    )
{
    DWORD dwError = 0;
    DWORD dwRecordCount = 0;
    DWORD dwCountOlderThan = 0;

    DWORD dwMaxRecords = 0;
    DWORD dwMaxAge = 0;
    DWORD dwMaxLogSize = 0;

    CLTR_LOG_VERBOSE("In Maintain DB ...............");
    //Get Max records,max age and max log size from the global list
    dwError = CltrGetMaxRecords(&dwMaxRecords);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrGetMaxAge(&dwMaxAge);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrGetMaxLogSize(&dwMaxLogSize);
    BAIL_ON_CLTR_ERROR(dwError);

    //Get the count of the records
    dwError = SrvEventLogCount(hDB, NULL, &dwRecordCount);
    BAIL_ON_CLTR_ERROR(dwError);

    //Get the count of the records which are older than 'n' days
    dwError = SrvEventLogCountOlderThan(hDB, dwMaxAge, &dwCountOlderThan);
    BAIL_ON_CLTR_ERROR(dwError);

    CLTR_LOG_VERBOSE("EventLog record count older than curdate = %d",dwCountOlderThan);
    CLTR_LOG_VERBOSE("EventLog Record count = %d",dwRecordCount);

    //Regular house keeping
    if (dwCountOlderThan){
        CLTR_LOG_VERBOSE("Deleting the record as older than current date");
        dwError = SrvDeleteOlderThanCurDate(hDB, dwMaxAge);
        BAIL_ON_CLTR_ERROR(dwError);

        //Since already some records got deleted get the latest record count
        dwError = SrvEventLogCount(hDB, NULL, &dwRecordCount);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    //TODO: to determine the max log size and check the max log size,waiting for server side to complete

    //If the record count is greater than the Max Records
    if (dwRecordCount > dwMaxRecords) {

        CLTR_LOG_VERBOSE("DB size exceeds the Max Records Size");
        CLTR_LOG_VERBOSE("Going to trim DB.........");

        //If records not aged out,then delete the records above Max Records
        dwError = SrvDeleteAboveLimitFromEventLog(hDB, dwMaxRecords);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    CLTR_LOG_VERBOSE("Pruned DB returning");

error:

    return dwError;
}


DWORD
SrvWriteToDB(
    HANDLE hDB,
    DWORD cRecords,
    LWCOLLECTOR_RECORD* pEventRecords 
    )
{
    DWORD dwError = 0;
    BOOLEAN bRemoveAsNeeded = 0;

    //Determine if auto clean up is  enabled?
    dwError = CltrGetRemoveEventsFlag(&bRemoveAsNeeded);
    BAIL_ON_CLTR_ERROR(dwError);

    //we are not going to trim the DB,if the flag is not set
    if (bRemoveAsNeeded) {
        //Trim the DB
        CLTR_LOG_VERBOSE("Going to trim database .....");
        dwError = SrvMaintainDB(hDB);
        BAIL_ON_CLTR_ERROR(dwError);
    }
    
    //Write the eventlog
    dwError = SrvWriteEventLog(hDB,cRecords,pEventRecords);
    BAIL_ON_CLTR_ERROR(dwError);

error:

    return dwError;

}


DWORD
SrvClearEventLog(
    HANDLE hDB
    )
{
    DWORD dwError = 0;
    PWSTR pwszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    ENTER_RW_WRITER_LOCK;


    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszQuery,
                    L"begin;"
                    TEXT(DB_QUERY_DROP_EVENTS_TABLE)
                    L";"
                    TEXT(DB_QUERY_CREATE_EVENTS_TABLE)
                    L";"
                    TEXT(DB_QUERY_CREATE_UNIQUE_INDEX)
                    L";"
                    TEXT(DB_QUERY_CREATE_INDEX)
                    L";"
                    TEXT(DB_QUERY_CREATE_INDEX)
                    L"; end",
                    /* for create unique index */
                    "recordId", "EventRecordId",
                    /* for create index */
                    "logname", "EventLogname"
                    /* for create index */
                    "dateTime", "EventDateTime");
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = SrvQueryEventLog(hDB, pwszQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_CLTR_ERROR(dwError);

 cleanup:
    LwRtlWC16StringFree(&pwszQuery);
    if (ppszResult) {
        sqlite3_free_table((PSTR*)ppszResult);
    }
    LEAVE_RW_WRITER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

DWORD
SrvDeleteFromEventLog(
    HANDLE hDB,
    PWSTR sqlFilter
    )
{
    DWORD dwError = 0;
    PWSTR pwszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;


    if (sqlFilter == NULL) {
        return 0;
    }

    ENTER_RW_WRITER_LOCK;

    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszQuery,
                    TEXT(DB_QUERY_DELETE),
                    sqlFilter);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = SrvQueryEventLog(hDB, pwszQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_CLTR_ERROR(dwError);

 cleanup:
    LwRtlWC16StringFree(&pwszQuery);
    if (ppszResult) {
        sqlite3_free_table((char**)ppszResult);
    }
    LEAVE_RW_WRITER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

//To get count of records that are older  than 'n' days.
DWORD
SrvEventLogCountOlderThan(
    HANDLE hDB,
    DWORD dwOlderThan,
    PDWORD pdwNumMatched
    )
{
    DWORD dwError = 0;
    PWSTR pwszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    ENTER_RW_READER_LOCK;

    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszQuery,
                    TEXT(DB_QUERY_COUNT_OLDER_THAN),
                    dwOlderThan);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = SrvQueryEventLog(hDB, pwszQuery, &nRows, &nCols, &ppszResult);

    if (nRows == 1) {
        *pdwNumMatched = (DWORD) atoi((const char*)ppszResult[1]);
        BAIL_ON_CLTR_ERROR(dwError);
    } else {

        CLTR_LOG_VERBOSE("Could not find count of event logs in database");
    }

 cleanup:
    LwRtlWC16StringFree(&pwszQuery);
    if (ppszResult) {
        sqlite3_free_table((PSTR*)ppszResult);
    }
    LEAVE_RW_READER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

//Delete the record over 'n' entries
DWORD
SrvDeleteAboveLimitFromEventLog(
    HANDLE hDB,
    DWORD dwOlderThan
    )
{

    DWORD dwError = 0;
    PWSTR pwszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;

    ENTER_RW_WRITER_LOCK;

    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszQuery,
                    TEXT(DB_QUERY_DELETE_ABOVE_LIMIT),
                    dwOlderThan);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = SrvQueryEventLog(hDB, pwszQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_CLTR_ERROR(dwError);

 cleanup:
    LwRtlWC16StringFree(&pwszQuery);
    if (ppszResult) {
        sqlite3_free_table((PSTR*)ppszResult);
    }
    LEAVE_RW_WRITER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

//To delete records 'n' days older than current date.
DWORD
SrvDeleteOlderThanCurDate(
    HANDLE hDB,
    DWORD dwOlderThan
    )
{
    DWORD dwError = 0;
    PWSTR pwszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;

    ENTER_RW_WRITER_LOCK;

    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszQuery,
                    TEXT(DB_QUERY_DELETE_OLDER_THAN),
                    dwOlderThan);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = SrvQueryEventLog(hDB, pwszQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_CLTR_ERROR(dwError);

 cleanup:
    LwRtlWC16StringFree(&pwszQuery);
    if (ppszResult) {
        sqlite3_free_table((PSTR*)ppszResult);
    }
    LEAVE_RW_WRITER_LOCK;
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
SrvQueryEventLog(
    HANDLE hDB,
    PWSTR pwszQuery,
    PDWORD pdwNumRows,
    PDWORD pdwNumCols,
    PSTR** pppszResult
    )
{
    DWORD dwError = 0;
    PEVENTLOG_CONTEXT pContext = NULL;
    PSTR  pszError = NULL;
    PSTR pszQuery = NULL;
    size_t size=0;
    INT numRowsLocal = 0;
    INT numColsLocal = 0;

    if (!hDB) {
        dwError = EINVAL;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    pContext = (PEVENTLOG_CONTEXT)(hDB);
    if (!pContext->pDbHandle) {
        dwError = CLTR_ERROR_INVALID_DB_HANDLE;
        BAIL_ON_CLTR_ERROR(dwError);
    }
    
    dwError = LwRtlCStringAllocateFromWC16String(
                    &pszQuery,
                    pwszQuery);
    BAIL_ON_CLTR_ERROR(dwError);
        
    CLTR_LOG_INFO("\nevtdb: SrvQueryEventLog: query=%ws\n\n", pwszQuery);
    
    dwError = sqlite3_get_table( pContext->pDbHandle,
                                 pszQuery,
                                 pppszResult,
                                 (INT*) &numRowsLocal,
                                 (INT*) &numColsLocal,
                                 (PSTR*) &pszError
                                );

    if (dwError) {
        if (!IsNullOrEmptyString(pszError)) {
            CLTR_LOG_ERROR("evtdb: SrvQueryEventLog: Error ={%hhs}\n\n",pszError);
        }
        BAIL_ON_CLTR_ERROR(dwError);
    }

    *pdwNumRows = (DWORD)numRowsLocal;
    *pdwNumCols = (DWORD)numColsLocal;


cleanup:
    LwRtlCStringFree(&pszQuery);

    return dwError;

 error:
    goto cleanup;
}

DWORD
SrvCreateDB(BOOLEAN replaceDB)
{
    DWORD dwError = 0;
    sqlite3* pSqliteHandle = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;
    sqlite3_stmt *pstQuery = NULL;

    PWSTR pwszQuery = NULL;
    PWSTR pwszQueryPos = NULL;
    PWSTR pwszDbPath = NULL;

    dwError = CltrGetDatabasePath(&pwszDbPath);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrCheckFileExists(pwszDbPath, &bExists);
    BAIL_ON_CLTR_ERROR(dwError);

    if (bExists) {
        if (replaceDB) {
            dwError = CltrRemoveFile(pwszDbPath);
            BAIL_ON_CLTR_ERROR(dwError);
        }   
        else return 0;
    }

    // This function sets pSqliteHandle on all error cases except when no
    // memory is available.
    dwError = sqlite3_open16(pwszDbPath, &pSqliteHandle);
    if (pSqliteHandle != NULL)
    {
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pSqliteHandle));
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError, "no memory");
    }

    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszQuery,
                    L"begin;\n"
                    TEXT(DB_QUERY_CREATE_EVENTS_TABLE)
                    L";\n"
                    TEXT(DB_QUERY_CREATE_CONFIG_TABLE)
                    L";\n"
                    TEXT(DB_QUERY_INSERT_CONFIG)
                    L";\n"
                    TEXT(DB_QUERY_CREATE_UNIQUE_INDEX)
                    L";\n"
                    TEXT(DB_QUERY_CREATE_INDEX)
                    L";\n"
                    TEXT(DB_QUERY_CREATE_INDEX)
                    L";\n"
                    TEXT(DB_QUERY_CREATE_INDEX)
                    L";\n end",
                    /* for insert config */
                    "SchemaVersion", SCHEMA_VERSION,
                    /* for create unique index */
                    "ColrecordId", "ColEventRecordId",
                    /* for create index */
                    "recordId", "EventRecordId",
                    /* for create index */
                    "logname", "Logname",
                    /* for create index */
                    "dateTime", "EventDateTime");
    BAIL_ON_CLTR_ERROR(dwError);

    pwszQueryPos = pwszQuery;
    while(*pwszQueryPos != 0)
    {
        dwError = sqlite3_prepare16_v2(
            pSqliteHandle,
            pwszQueryPos,
            -1, //search for null termination in szQuery to get length
            &pstQuery,
            &pwszQueryPos);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pSqliteHandle));

        dwError = (DWORD)sqlite3_step(pstQuery);
        if (dwError == SQLITE_DONE)
        {
            dwError = 0;
        }
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pSqliteHandle));

        dwError = sqlite3_finalize(pstQuery);
        pstQuery = NULL;
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pSqliteHandle));
    }

    CLTR_LOG_VERBOSE("New database file %ws with TABLE lwievents successfully created.\n", pwszDbPath);

cleanup:
    LwRtlWC16StringFree(&pwszQuery);
    if (pstQuery)
    {
        sqlite3_finalize(pstQuery);
    }
    CLTR_SAFE_FREE_STRING(pwszDbPath);

    if (pSqliteHandle)
        sqlite3_close(pSqliteHandle);

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError))
        CLTR_LOG_ERROR("evtdb: SrvCreateDB: Error ={%hhs}\n\n",pszError);
        

    goto cleanup;
}
