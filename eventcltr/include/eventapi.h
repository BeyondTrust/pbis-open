#ifndef __EVENTAPI_H__
#define __EVENTAPI_H__

#include <cltr-record.h>

#ifdef _WIN32
typedef HANDLE ACCESS_TOKEN;
#else
#include <lwio/io-types.h>
typedef LW_PIO_CREDS ACCESS_TOKEN;
#endif

/*
//IDL_HANDLE_T should be the same size and signage of integer as IDL type handle_t,
//so that it may be safely cast to handle_t, and back again to IDL_HANDLE_T
//
#define ARCH_32BIT

#ifdef ARCH_32BIT
typedef int IDL_HANDLE_T;
#endif

#ifdef ARCH_64BIT
typedef uint64 IDL_HANDLE_T;
#endif

typedef struct _COLLECTOR_HANDLE
{
    IDL_HANDLE_T bindingHandle;
    //char * pszBindingString;
    PWSTR pszBindingString;
    BOOLEAN bDefaultActive;
    EVENT_LOG_RECORD defaultEventLogRecord;

} COLLECTOR_HANDLE, *PCOLLECTOR_HANDLE;
*/

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LWEVENTLIB_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LW_CLTRLIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
#ifdef LWEVENTLIB_EXPORTS
#define LW_CLTRLIB_API __declspec(dllexport)
#else
#define LW_CLTRLIB_API __declspec(dllimport)
#endif
#else
#define LW_CLTRLIB_API
#endif

void
LW_CLTRLIB_API
CltrFreeRecordContents(
    PLWCOLLECTOR_RECORD pRecord
    );

void
LW_CLTRLIB_API
CltrFreeRecordList(
    DWORD cRecords,
    PLWCOLLECTOR_RECORD pEventRecord
    );

DWORD
LW_CLTRLIB_API
CltrOpenCollector(
    PCWSTR pszServerName,
    PCWSTR pszServerSpn,
    ACCESS_TOKEN pIoAccessToken,
    PHANDLE phEventLog
    );

DWORD
LW_CLTRLIB_API
CltrCloseCollector(
    HANDLE hEventLog
    );


DWORD
LW_CLTRLIB_API
CltrReadRecords(
    HANDLE hEventLog,
    UINT64 qwLastRecordId,
    DWORD nRecordsPerPage,
    PCWSTR sqlFilter,
    PDWORD pdwNumReturned,
    PLWCOLLECTOR_RECORD* eventRecords
    );

DWORD
LW_CLTRLIB_API
CltrGetPushRate(
    HANDLE hEventLog,
    DWORD dwEventsWaiting,
    double dPeriodConsumed,
    PDWORD pdwRecordsPerPeriod,
    PDWORD pdwRecordsPerBatch,
    PDWORD pdwPeriodSecs
    );

DWORD
LW_CLTRLIB_API
CltrGetRecordCount(
    HANDLE hEventLog,
    PCWSTR sqlFilter,
    DWORD* pdwNumMatched
    );    

DWORD
LW_CLTRLIB_API
CltrWriteRecords(
    HANDLE hEventLog,
    DWORD cRecords,
      PLWCOLLECTOR_RECORD pEventRecords 
    );

DWORD
LW_CLTRLIB_API
CltrDeleteRecords(
    HANDLE hEventLog,
    PCWSTR sqlFilter
    );

DWORD
LW_CLTRLIB_API
CltrClearRecords(
    HANDLE hEventLog
    );

#endif /* __EVENTAPI_H__ */
