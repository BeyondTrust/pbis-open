/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        server.h
 *
 * Abstract:
 *
 *        Server private header file
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWSM_SERVER_H__
#define __LWSM_SERVER_H__

#include "common.h"

#include <pthread.h>
#include <time.h>

struct _LW_SERVICE_OBJECT
{
    PVOID pData;
};

typedef struct _SM_LOADER_CALLS
{
    PVOID (*pfnGetServiceObjectData) (PLW_SERVICE_OBJECT pObject);
    VOID (*pfnRetainServiceObject) (PLW_SERVICE_OBJECT pObject);
    VOID (*pfnReleaseServiceObject) (PLW_SERVICE_OBJECT pObject);
    VOID (*pfnNotifyServiceObjectStateChange) (PLW_SERVICE_OBJECT pObject, LW_SERVICE_STATE newState);
} SM_LOADER_CALLS, *PSM_LOADER_CALLS;

typedef struct _SM_ENTRY_NOTIFY
{
    VOID (*pfnNotifyEntryStateChange)(LW_SERVICE_STATE state, PVOID pData);
    PVOID pData;
    SM_LINK link;
} SM_ENTRY_NOTIFY, *PSM_ENTRY_NOTIFY;

/* Entry in the running object table */
typedef struct _SM_TABLE_ENTRY
{
    /* Details */
    PLW_SERVICE_INFO pInfo;
    /* Is entry still valid? */
    BOOL volatile bValid;
    /* Are dependencies marked? */
    BOOL volatile bDepsMarked;
    /* Has pInfo been changed since the service was last constructed? */
    BOOL volatile bDirty;
    /* How many automatic restarts have we attempted? */
    DWORD RestartAttempts;
    /* When did we begin the last restart period? */
    time_t LastRestartPeriod;
    /* Lock controlling access to entry */
    pthread_mutex_t lock;
    pthread_mutex_t* pLock;
    /* State change event */
    pthread_cond_t event;
    pthread_cond_t* pEvent;
    /* State change waiters */
    SM_LINK waiters;
    /* Pointer to vtbl */
    PLW_SERVICE_LOADER_VTBL pVtbl;
    /* Loader handle */
    LW_SERVICE_OBJECT object;
    /* Data */
    void* pData;
    /* Reverse dependency count
       
       This is the number of other running services
       which depend directly on us.  When it is > 0,
       the service cannot be safely stopped */
    DWORD volatile dwDepCount;
    /* Reference count
       
       This is the number of holders of a reference to
       this entry -- in particular, by service handles.
       
       The reference count is protected by the table lock
       and not the entry lock */
    DWORD volatile dwRefCount;
    /* Links to siblings (protected by table lock) */
    SM_LINK link;
} SM_TABLE_ENTRY, *PSM_TABLE_ENTRY;

/* Global running object table */
typedef struct _SM_TABLE
{
    pthread_mutex_t  lock;
    pthread_mutex_t* pLock; 
    SM_LINK entries;
} SM_TABLE;

/* API handle */
struct _LW_SERVICE_HANDLE
{
     /* Pointer to table entry */
    PSM_TABLE_ENTRY pEntry;
};

/* Bootstrap service definition */
typedef struct _SM_BOOTSTRAP_SERVICE
{
    PCSTR pszName;
    LW_SERVICE_TYPE type;
    PCSTR pszPath;
    CHAR const * const ppszArgs[];
} SM_BOOTSTRAP_SERVICE, *PSM_BOOTSTRAP_SERVICE;

typedef struct _SM_LOGGER
{
    LW_SM_LOGGER_TYPE type;

    DWORD
    (*pfnOpen) (
        PVOID pData
        );

    DWORD
    (*pfnLog) (
        LW_SM_LOG_LEVEL level,
        PCSTR pszFacility,
        PCSTR pszFunctionName,
        PCSTR pszSourceFile,
        DWORD dwLineNumber,
        PCSTR pszMessage,
        PVOID pData
        );

    VOID
    (*pfnClose) (
        PVOID pData
        );

    DWORD
    (*pfnGetTargetName) (
        PSTR* ppszTargetName,
        PVOID pData
        );
} SM_LOGGER, *PSM_LOGGER;

DWORD
LwSmSrvAcquireServiceHandle(
    PCWSTR pwszName,
    PLW_SERVICE_HANDLE phHandle
    );

VOID
LwSmSrvReleaseHandle(
    LW_SERVICE_HANDLE hHandle
    );

DWORD
LwSmSrvEnumerateServices(
    PWSTR** pppwszServiceNames
    );

DWORD
LwSmSrvGetServiceStatus(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_STATUS pStatus
    );

DWORD
LwSmSrvStartService(
    LW_SERVICE_HANDLE hHandle
    );

DWORD
LwSmSrvStopService(
    LW_SERVICE_HANDLE hHandle
    );

DWORD
LwSmSrvGetServiceInfo(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_INFO* ppInfo
    );

LWMsgDispatchSpec*
LwSmGetDispatchSpec(
    VOID
    );

DWORD
LwSmTableGetEntry(
    PCWSTR pwszName,
    PSM_TABLE_ENTRY* ppEntry
    );

DWORD
LwSmTableEnumerateEntries(
    PWSTR** pppwszServiceNames
    );

DWORD
LwSmTableAddEntry(
    PLW_SERVICE_INFO pInfo,
    PSM_TABLE_ENTRY* ppEntry
    );

DWORD
LwSmTableUpdateEntry(
    PSM_TABLE_ENTRY pEntry,
    PCLW_SERVICE_INFO pInfo,
    LW_SERVICE_INFO_MASK mask
    );

VOID
LwSmTableRetainEntry(
    PSM_TABLE_ENTRY pEntry
    );

VOID
LwSmTableReleaseEntry(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableStartEntry(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableStopEntry(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableRefreshEntry(
    PSM_TABLE_ENTRY pEntry
    );

VOID
LwSmTableNotifyEntryStateChanged(
    PSM_TABLE_ENTRY pEntry,
    LW_SERVICE_STATE state
    );

DWORD
LwSmTableRegisterEntryNotify(
    PSM_TABLE_ENTRY pEntry,
    LW_SERVICE_STATE currentState,
    VOID (*pfnNotifyEntryStateChange)(LW_SERVICE_STATE state, PVOID pData),
    PVOID pData
    );

DWORD
LwSmTableUnregisterEntryNotify(
    PSM_TABLE_ENTRY pEntry,
    VOID (*pfnNotifyEntryStateChange)(LW_SERVICE_STATE state, PVOID pData),
    PVOID pData
    );

DWORD
LwSmTableWaitEntryChanged(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableGetEntryStatus(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    );

DWORD
LwSmTableGetEntryDependencyClosure(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    );

DWORD
LwSmTableGetEntryReverseDependencyClosure(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    );

DWORD
LwSmTableInit(
    VOID
    );

VOID
LwSmTableShutdown(
    VOID
    );

DWORD
LwSmRegistryEnumServices(
    HANDLE hReg,
    PWSTR** pppwszNames
    );

    
DWORD
LwSmRegistryReadServiceInfo(
    HANDLE hReg,
    PCWSTR pwszName,
    PLW_SERVICE_INFO* ppInfo
    );

DWORD
LwSmBootstrap(
    VOID
    );

DWORD
LwSmPopulateTable(
    VOID
    );

DWORD
LwSmLoaderInitialize(
    PSM_LOADER_CALLS pCalls
    );

VOID
LwSmLoaderShutdown(
    VOID
    );

DWORD
LwSmLoaderGetVtbl(
    PCWSTR pwszLoaderName,
    PLW_SERVICE_LOADER_VTBL* ppVtbl
    );

VOID
LwSmLogInit(
    VOID
    );

DWORD
LwSmLogMessage(
    LW_SM_LOG_LEVEL level,
    PCSTR pszFacility,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszMessage
    );

DWORD
LwSmLogPrintfv(
    LW_SM_LOG_LEVEL level,
    PCSTR pszFacility,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszFormat,
    va_list ap
    );

DWORD
LwSmLogPrintf(
    LW_SM_LOG_LEVEL level,
    PCSTR pszFacility,
    PCSTR pszFunctionName,
    PCSTR pszSourceFile,
    DWORD dwLineNumber,
    PCSTR pszFormat,
    ...
    );

DWORD
LwSmSetLogger(
    PSM_LOGGER pLogger,
    PVOID pData
    );

VOID
LwSmSetMaxLogLevel(
    LW_SM_LOG_LEVEL level
    );

LW_SM_LOG_LEVEL
LwSmGetMaxLogLevel(
    VOID
    );

DWORD
LwSmSetLoggerToFile(
    FILE* file
    );

DWORD
LwSmSetLoggerToPath(
    PCSTR pszPath
    );

DWORD
LwSmSetLoggerToSyslog(
    PCSTR pszProgramName
    );

DWORD
LwSmLogLevelNameToLogLevel(
    PCSTR pszName,
    PLW_SM_LOG_LEVEL pLevel
    );

DWORD
LwSmSrvGetLogInfo(
    LW_SM_LOGGER_TYPE* pType,
    PSTR* ppszTargetName
    );

#define SM_LOG(level, ...) LwSmLogPrintf((level), NULL, __func__, __FILE__, __LINE__, __VA_ARGS__)
#define SM_LOG_ALWAYS(...) SM_LOG(LW_SM_LOG_LEVEL_ALWAYS, __VA_ARGS__)
#define SM_LOG_ERROR(...) SM_LOG(LW_SM_LOG_LEVEL_ERROR, __VA_ARGS__)
#define SM_LOG_WARNING(...) SM_LOG(LW_SM_LOG_LEVEL_WARNING, __VA_ARGS__)
#define SM_LOG_INFO(...) SM_LOG(LW_SM_LOG_LEVEL_INFO, __VA_ARGS__)
#define SM_LOG_VERBOSE(...) SM_LOG(LW_SM_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define SM_LOG_DEBUG(...) SM_LOG(LW_SM_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define SM_LOG_TRACE(...) SM_LOG(LW_SM_LOG_LEVEL_TRACE, __VA_ARGS__)

extern SM_LOADER_CALLS gTableCalls;

#endif
