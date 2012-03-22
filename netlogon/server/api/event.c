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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        event.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Eventlog API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

static HANDLE ghEventLogItf = NULL;
static pthread_rwlock_t ghEventLogItf_rwlock;

#define ENTER_EVENTLOG_READER_LOCK(bInLock) \
    do { \
        if (!bInLock) \
        { \
            pthread_rwlock_rdlock(&ghEventLogItf_rwlock); \
            bInLock = TRUE; \
        } \
    } while (0)

#define LEAVE_EVENTLOG_READER_LOCK(bReleaseLock) \
    do { \
        if (bReleaseLock) \
        { \
            pthread_rwlock_unlock(&ghEventLogItf_rwlock); \
            bReleaseLock = FALSE; \
        } \
    } while (0)

#define ENTER_EVENTLOG_WRITER_LOCK(bInLock) \
    do { \
        if (!bInLock) \
        { \
            pthread_rwlock_wrlock(&ghEventLogItf_rwlock); \
            bInLock = TRUE; \
        } \
    } while (0)

#define LEAVE_EVENTLOG_WRITER_LOCK(bReleaseLock) \
    do { \
        if (bReleaseLock) \
        { \
            pthread_rwlock_unlock(&ghEventLogItf_rwlock); \
            bReleaseLock = FALSE;                         \
        } \
    } while (0)

#define SUCCESS_AUDIT_EVENT_TYPE    "Success Audit"
#define FAILURE_AUDIT_EVENT_TYPE    "Failure Audit"
#define INFORMATION_EVENT_TYPE      "Information"
#define WARNING_EVENT_TYPE          "Warning"
#define ERROR_EVENT_TYPE            "Error"


VOID
LWNetSrvInitEventlogInterface(
    VOID
    )
{
    DWORD dwError = 0;
    PEVENTLOG_INTERFACE pEventLogItf = NULL;
    PVOID pLibHandle = NULL;
    PCSTR pszError = NULL;
    PSTR pszLibDirPath = NULL;
    CHAR szEventLogLibPath[PATH_MAX+1];
    BOOLEAN bExists = FALSE;
    PEVENTAPIFUNCTIONTABLE  pFuncTable = NULL;
    BOOLEAN bInLock = FALSE;

    pthread_rwlock_init(&ghEventLogItf_rwlock, NULL);
    
    ENTER_EVENTLOG_WRITER_LOCK(bInLock);
    
    dwError = LWNetGetLibDirPath(&pszLibDirPath);
    BAIL_ON_LWNET_ERROR(dwError);
 
#if defined (__LWI_DARWIN__)
    sprintf(szEventLogLibPath, "%s/libeventlog-mac.so", pszLibDirPath);
#else
    sprintf(szEventLogLibPath, "%s/libeventlog%s", pszLibDirPath, MOD_EXT);
#endif
    
    dwError = LwCheckFileTypeExists(
                    szEventLogLibPath,
                    LWFILE_REGULAR,
                    &bExists);
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (!bExists) {
       ghEventLogItf = (HANDLE)NULL;
       goto cleanup;
    }
    
    dlerror();
    pLibHandle = dlopen(szEventLogLibPath, RTLD_NOW | RTLD_GLOBAL);
    if (pLibHandle == NULL) {
        
        pszError = dlerror();
                
        LWNET_LOG_ERROR("Error: Failed to load Likewise Eventlog Module [%s]",
             IsNullOrEmptyString(pszError) ? "" : pszError);
        
        dwError = ERROR_DLL_INIT_FAILED;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateMemory(
                    sizeof(EVENTAPIFUNCTIONTABLE),
                    (PVOID*)&pFuncTable);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dlerror();
    pFuncTable->pfnFreeEventRecord = (PFN_FREE_EVENT_RECORD)dlsym(
                                pLibHandle, 
                                EVENTAPI_FREE_EVENT_RECORD_FUNCTION);
    if (pFuncTable->pfnFreeEventRecord == NULL) {
        pszError = dlerror();
        
        LWNET_LOG_ERROR("Error: Failed to lookup symbol %s in Eventlog Module [%s]",
                      EVENTAPI_FREE_EVENT_RECORD_FUNCTION,
                      IsNullOrEmptyString(pszError) ? "" : pszError);
        
        dwError = ERROR_BAD_DLL_ENTRYPOINT;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dlerror();
    pFuncTable->pfnOpenEventLog = (PFN_OPEN_EVENT_LOG)dlsym(
                                pLibHandle, 
                                EVENTAPI_OPEN_EVENT_LOG_FUNCTION);
    if (pFuncTable->pfnOpenEventLog == NULL) {
        pszError = dlerror();
        
        LWNET_LOG_ERROR("Error: Failed to lookup symbol %s in Eventlog Module [%s]",
                      EVENTAPI_OPEN_EVENT_LOG_FUNCTION,
                      IsNullOrEmptyString(pszError) ? "" : pszError);
        
        dwError = ERROR_BAD_DLL_ENTRYPOINT;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dlerror();
    pFuncTable->pfnCloseEventLog = (PFN_CLOSE_EVENT_LOG)dlsym(
                                pLibHandle, 
                                EVENTAPI_CLOSE_EVENT_LOG_FUNCTION);
    if (pFuncTable->pfnCloseEventLog == NULL) {
        pszError = dlerror();
        
        LWNET_LOG_ERROR("Error: Failed to lookup symbol %s in Eventlog Module [%s]",
                      EVENTAPI_CLOSE_EVENT_LOG_FUNCTION,
                      IsNullOrEmptyString(pszError) ? "" : pszError);
        
        dwError = ERROR_BAD_DLL_ENTRYPOINT;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dlerror();
    pFuncTable->pfnWriteEventLogBase = (PFN_WRITE_EVENT_LOG_BASE)dlsym(
                                pLibHandle, 
                                EVENTAPI_WRITE_EVENT_LOG_BASE_FUNCTION);
    if (pFuncTable->pfnWriteEventLogBase == NULL) {
        pszError = dlerror();
        
        LWNET_LOG_ERROR("Error: Failed to lookup symbol %s in Eventlog Module [%s]",
                      EVENTAPI_WRITE_EVENT_LOG_BASE_FUNCTION,
                      IsNullOrEmptyString(pszError) ? "" : pszError);
        
        dwError = ERROR_BAD_DLL_ENTRYPOINT;
        BAIL_ON_LWNET_ERROR(dwError);
    }
 
    dwError = LWNetSrvValidateEventlogInterface(pFuncTable);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetAllocateMemory(
                    sizeof(EVENTLOG_INTERFACE),
                    (PVOID*)&pEventLogItf);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pEventLogItf->pFuncTable = pFuncTable;
    pEventLogItf->pLibHandle = pLibHandle;
    
    ghEventLogItf = (HANDLE)pEventLogItf;
    
cleanup:

    LEAVE_EVENTLOG_WRITER_LOCK(bInLock);

    LWNET_SAFE_FREE_STRING(pszLibDirPath);

    return;
    
error:

    if (pLibHandle) {
        dlclose(pLibHandle);
    }
    
    ghEventLogItf = (HANDLE)NULL;
    
    LWNET_LOG_ERROR("Failed to load eventlog module [Error code: %d]", dwError);

    goto cleanup;
}

DWORD
LWNetSrvValidateEventlogInterface(
    PEVENTAPIFUNCTIONTABLE pFuncTable
    )
{
    DWORD dwError = 0;
    
    if (!pFuncTable ||
        !pFuncTable->pfnCloseEventLog ||
        !pFuncTable->pfnFreeEventRecord ||
        !pFuncTable->pfnOpenEventLog ||
        !pFuncTable->pfnWriteEventLogBase)
    {
       dwError = ERROR_EVENTLOG_CANT_START;
    }
    
    return dwError;
}

VOID
LWNetSrvShutdownEventlogInterface(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;
    
    ENTER_EVENTLOG_WRITER_LOCK(bInLock);
    
    PEVENTLOG_INTERFACE pEventLogItf = (PEVENTLOG_INTERFACE)ghEventLogItf;
    if (pEventLogItf) {
       if (pEventLogItf->pLibHandle) {
           dlclose(pEventLogItf->pLibHandle);
           pEventLogItf->pLibHandle = NULL;
       }
       LWNetFreeMemory(pEventLogItf);
    }

    ghEventLogItf = NULL;
    
    LEAVE_EVENTLOG_WRITER_LOCK(bInLock);
}

DWORD
LWNetSrvOpenEventLog(
    PHANDLE phEventLog
    )
{
    DWORD dwError = 0;
    PEVENTLOG_INTERFACE pEventLogItf = NULL;
    BOOLEAN bInLock = FALSE;
    
    ENTER_EVENTLOG_READER_LOCK(bInLock);
    
    if (ghEventLogItf == (HANDLE)NULL) {
       *phEventLog = (HANDLE)NULL;
       goto cleanup;
    }
    
    pEventLogItf = (PEVENTLOG_INTERFACE)ghEventLogItf;

    dwError = pEventLogItf->pFuncTable->pfnOpenEventLog(
                   NULL,            // Server name (defaults to local computer eventlogd)
                   phEventLog);
    BAIL_ON_LWNET_ERROR(dwError);

cleanup:

    LEAVE_EVENTLOG_READER_LOCK(bInLock);

    return dwError; 

error:

    *phEventLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LWNetSrvCloseEventLog(
    HANDLE hEventLog
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    
    ENTER_EVENTLOG_READER_LOCK(bInLock);
    
    if (ghEventLogItf != (HANDLE)NULL) {
       PEVENTLOG_INTERFACE pEventLogItf = 
           (PEVENTLOG_INTERFACE)(ghEventLogItf);
       dwError = pEventLogItf->pFuncTable->pfnCloseEventLog(hEventLog);
       BAIL_ON_LWNET_ERROR(dwError);
    }
    
cleanup:

    LEAVE_EVENTLOG_READER_LOCK(bInLock);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWNetSrvLogEvent(
    EVENT_LOG_RECORD event
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;
    PEVENTLOG_INTERFACE pEventLogItf = NULL;
    
    ENTER_EVENTLOG_READER_LOCK(bInLock);
    
    if (ghEventLogItf == (HANDLE)NULL) {
       goto cleanup;
    }
    
    pEventLogItf = (PEVENTLOG_INTERFACE)ghEventLogItf;

    dwError = LWNetSrvOpenEventLog(&hEventLog);
    BAIL_ON_LWNET_ERROR(dwError); 

    dwError = pEventLogItf->pFuncTable->pfnWriteEventLogBase(
                   hEventLog,
                   event);
    BAIL_ON_LWNET_ERROR(dwError);

cleanup:

    LWNetSrvCloseEventLog(hEventLog);

    LEAVE_EVENTLOG_READER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNetSrvLogInformationEvent(
    DWORD  dwEventID,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = "System";
    event.pszEventType = INFORMATION_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = "Likewise NETLOGON";
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = "SYSTEM";
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LWNetSrvLogEvent(event);
}

DWORD
LWNetSrvLogWarningEvent(
    DWORD  dwEventID,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = "System";
    event.pszEventType = WARNING_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = "Likewise NETLOGON";
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = "SYSTEM";
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LWNetSrvLogEvent(event);
}

DWORD
LWNetSrvLogErrorEvent(
    DWORD  dwEventID,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = "System";
    event.pszEventType = ERROR_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = "Likewise NETLOGON";
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = "SYSTEM";
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LWNetSrvLogEvent(event);
}


