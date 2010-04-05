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

void
SrvRPCFreeRecordList(
    DWORD cRecords,
    PLWCOLLECTOR_RECORD pRecordList
    );

void
SrvRPCFreeRecordContents(
    PLWCOLLECTOR_RECORD pRecord
    );

//database actions

DWORD
SrvOpenEventDatabase(
    PHANDLE phDB
    );

DWORD
SrvCloseEventDatabase(
    HANDLE hDB
    );


DWORD
SrvEventLogCount(
    HANDLE hDB,
    PWSTR  sqlFilter,
    PDWORD pdwNumMatched
    );

DWORD
SrvReadEventLog(
    HANDLE hDB,
    UINT64  qwStartingRowId,
    DWORD  nRecordsPerPage,
    PWSTR  sqlFilter,
    PDWORD pdwNumReturned,
    LWCOLLECTOR_RECORD** eventRecords
    );


DWORD
SrvWriteEventLog(
    HANDLE hDB,
    DWORD cRecords,
    LWCOLLECTOR_RECORD* pEventRecords 
    );


DWORD
SrvWriteToDB(
    HANDLE hDB,
    DWORD cRecords,
    LWCOLLECTOR_RECORD* pEventRecords 
    );

DWORD
SrvClearEventLog(
    HANDLE hDB
    );

DWORD
SrvDeleteFromEventLog(
    HANDLE hDB,
    PWSTR   sqlFilter
    );

DWORD
SrvEventLogCountOlderThan(
    HANDLE hDB,
    DWORD  dwOlderThan,
    PDWORD pdwNumMatched
    );
DWORD
SrvDeleteAboveLimitFromEventLog(
    HANDLE hDB,
    DWORD  dwOlderThan
    ) ;
DWORD
SrvDeleteOlderThanCurDate(
    HANDLE hDB,
    DWORD  dwOlderThan
    ); 

//helper functions
DWORD
SrvQueryEventLog(
    HANDLE hDB,
    PWSTR   szQuery,
    PDWORD pdwNumRows,
    PDWORD pdwNumCols,
    PSTR** pppszResult
    );


DWORD
SrvCreateDB(BOOLEAN replaceDB);

#endif /* __EVTDB_H__ */
