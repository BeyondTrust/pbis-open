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
 * Eventlog Server API
 *
 */
#include "server.h"

DWORD
RpcCltrOpen(
    handle_t bindingHandle,
    RPC_LWCOLLECTOR_HANDLE *phCollector
    )
{
    DWORD dwError = 0;
    PRPC_LWCOLLECTOR_CONNECTION pConnection = NULL;
    PWSTR pwszStringBinding = NULL;
    PWSTR pwszAddress = NULL;
    ULONG luClientSize = 0;
    BOOLEAN bImpersonating = FALSE;
    RPC_BINDING_HANDLE serverBinding = NULL;
    PSECURITY_DESCRIPTOR pDescriptor = NULL;
    HANDLE hImpersonation = NULL;
    DWORD dwAllowedAccess = 0;
    char chPrivilegeBuffer[1000];
    GENERIC_MAPPING mapping = {0};
    DWORD dwPrivilegeLen = sizeof(chPrivilegeBuffer);
    BOOL bAccessStatus = FALSE;

    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);

    mapping.GenericAll = ADS_RIGHT_DS_READ_PROP | 
                        ADS_RIGHT_DS_CREATE_CHILD | 
                        ADS_RIGHT_DS_DELETE_CHILD;
    mapping.GenericExecute = 
                        ADS_RIGHT_DS_CREATE_CHILD;
    mapping.GenericRead = 
                        ADS_RIGHT_DS_READ_PROP;
    mapping.GenericWrite = 
                        ADS_RIGHT_DS_CREATE_CHILD |
                        ADS_RIGHT_DS_DELETE_CHILD;

    dwError = CltrGetRemoteSecurityDescriptor(&pDescriptor);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrAllocateMemory(sizeof(RPC_LWCOLLECTOR_CONNECTION),
                                (PVOID*)&pConnection);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = RpcImpersonateClient(bindingHandle);
    BAIL_ON_CLTR_ERROR(dwError);
    bImpersonating = TRUE;

    if (!OpenThreadToken(
            GetCurrentThread(),
            TOKEN_READ,
            FALSE,
            &hImpersonation))
    {
        dwError = GetLastError();   
        BAIL_ON_CLTR_ERROR(dwError);
    }

   if (!GetUserNameEx(
            NameSamCompatible,
            NULL,
            &luClientSize) &&
        GetLastError() != ERROR_MORE_DATA)
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError = CltrAllocateMemory(sizeof(WCHAR) * luClientSize,
                                (PVOID*)&pConnection->pwszClientName);
    BAIL_ON_CLTR_ERROR(dwError);

    if (!GetUserNameEx(
            NameSamCompatible, 
            pConnection->pwszClientName, 
            &luClientSize))
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }

    if (!AccessCheck(
                pDescriptor,
                hImpersonation,
                ADS_RIGHT_DS_READ_PROP,
                &mapping,
                (PPRIVILEGE_SET)chPrivilegeBuffer,
                &dwPrivilegeLen,
                &dwAllowedAccess,
                &bAccessStatus))
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (bAccessStatus)
    {
        pConnection->bReadAllowed = TRUE;
    }
    if (!AccessCheck(
                pDescriptor,
                hImpersonation,
                ADS_RIGHT_DS_CREATE_CHILD,
                &mapping,
                (PPRIVILEGE_SET)chPrivilegeBuffer,
                &dwPrivilegeLen,
                &dwAllowedAccess,
                &bAccessStatus))
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (bAccessStatus)
    {
        pConnection->bWriteAllowed = TRUE;
    }
    if (!AccessCheck(
                pDescriptor,
                hImpersonation,
                ADS_RIGHT_DS_DELETE_CHILD,
                &mapping,
                (PPRIVILEGE_SET)chPrivilegeBuffer,
                &dwPrivilegeLen,
                &dwAllowedAccess,
                &bAccessStatus))
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (bAccessStatus)
    {
        pConnection->bDeleteAllowed = TRUE;
    }

    dwError = RpcRevertToSelf();
    bImpersonating = FALSE;
    BAIL_ON_CLTR_ERROR(dwError);

    /* figure out who's calling us.
       first, get the binding string */

    dwError = RpcBindingServerFromClient(bindingHandle, &serverBinding);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = RpcBindingToStringBinding(serverBinding, &pwszStringBinding);
    BAIL_ON_CLTR_ERROR(dwError);

    /* now parse out the address */
    dwError = RpcStringBindingParse(
                    pwszStringBinding,
                    NULL,
                    NULL,
                    &pwszAddress,
                    NULL,
                    NULL);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrAllocateString(
                    pwszAddress,
                    &pConnection->pwszFromAddress);
    BAIL_ON_CLTR_ERROR(dwError);

    *phCollector = pConnection;

cleanup:
    RpcStringFree(&pwszStringBinding);
    RpcStringFree(&pwszAddress);
    if (bImpersonating)
    {
        RpcRevertToSelf();
    }
    if (serverBinding)
    {
        RpcBindingFree(serverBinding);
    }
    if (pDescriptor)
    {
        LocalFree(pDescriptor);
    }
    if (hImpersonation)
    {
        CloseHandle(hImpersonation);
    }

    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

error:
    if (pConnection)
    {
        CLTR_SAFE_FREE_STRING(pConnection->pwszFromAddress);
        CLTR_SAFE_FREE_STRING(pConnection->pwszClientName);
    }
    CLTR_SAFE_FREE_MEMORY(pConnection);
    goto cleanup;
}

DWORD
RpcCltrClose(
    RPC_LWCOLLECTOR_HANDLE *phCollector
    )
{
    DWORD     dwError = 0;
    PRPC_LWCOLLECTOR_CONNECTION pConnection = NULL;
    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);

    if (phCollector == NULL) 
    {
        dwError = EINVAL;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    pConnection = *phCollector;
    if (pConnection != NULL)
    {
        CLTR_SAFE_FREE_STRING(pConnection->pwszFromAddress);
        CLTR_SAFE_FREE_STRING(pConnection->pwszClientName);
        CLTR_SAFE_FREE_MEMORY(pConnection);
    }

    *phCollector = NULL;

cleanup:
    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

error:
    goto cleanup;
}

void
__RPC_USER
RPC_LWCOLLECTOR_HANDLE_rundown(
    RPC_LWCOLLECTOR_HANDLE hCollector
    )
{
    RpcCltrClose(&hCollector);
}

DWORD
RpcCltrGetRecordCount(
    RPC_LWCOLLECTOR_HANDLE hCollector,
    WCHAR * sqlFilter,
    DWORD * pdwNumMatched
    )
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;

    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);
    if (!hCollector->bReadAllowed)
    {
        dwError = LW_STATUS_ACCESS_DENIED;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError =  SrvOpenEventDatabase(&hDB);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError =  SrvEventLogCount(
                    hDB,
                    sqlFilter,
                    pdwNumMatched);
    BAIL_ON_CLTR_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

error:

    goto cleanup;
}

DWORD
RpcCltrReadRecords(
    RPC_LWCOLLECTOR_HANDLE hCollector,
    UINT64 qwIndex,
    DWORD nRecordsPerPage,
    WCHAR * sqlFilter,
    PLWCOLLECTOR_RECORD_LIST pEventRecordList
    )
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;

    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);
    if (!hCollector->bReadAllowed)
    {
        dwError = LW_STATUS_ACCESS_DENIED;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError =  SrvOpenEventDatabase(&hDB);
    BAIL_ON_CLTR_ERROR(dwError);

    pEventRecordList->pRecords = NULL;
    dwError = SrvReadEventLog(
                    hDB,
                    qwIndex,
                    nRecordsPerPage,
                    sqlFilter,
                    &pEventRecordList->dwCount,
                    &pEventRecordList->pRecords);

    BAIL_ON_CLTR_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

error:

    pEventRecordList->dwCount = 0;

    goto cleanup;
}

DWORD
RpcCltrWriteRecords(
    RPC_LWCOLLECTOR_HANDLE hCollector,
    DWORD cRecords,
    LWCOLLECTOR_RECORD* pEventRecords 
    )
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;
    DWORD dwCurrentTime = (DWORD)time(NULL);
    DWORD dwIndex = 0;

    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);
    if (!hCollector->bWriteAllowed)
    {
        dwError = LW_STATUS_ACCESS_DENIED;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    for (dwIndex = 0; dwIndex < cRecords; dwIndex++)
    {
        dwError = RPCAllocateString(
                        hCollector->pwszFromAddress,
                        &pEventRecords[dwIndex].pwszColComputerAddress);
        BAIL_ON_CLTR_ERROR(dwError);

        dwError = RPCAllocateString(
                        hCollector->pwszClientName,
                        &pEventRecords[dwIndex].pwszColComputer);
        BAIL_ON_CLTR_ERROR(dwError);

        pEventRecords[dwIndex].dwColDateTime = dwCurrentTime;
    }

    dwError = SrvOpenEventDatabase(&hDB);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = SrvWriteToDB(
                    hDB,
                    cRecords,
                    pEventRecords);
    BAIL_ON_CLTR_ERROR(dwError);

cleanup:
    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

error:
    goto cleanup;
}

DWORD
RpcCltrDeleteRecords(
    RPC_LWCOLLECTOR_HANDLE hCollector,
    WCHAR * sqlFilter
    )
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;

    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);
    if (!hCollector->bDeleteAllowed)
    {
        dwError = LW_STATUS_ACCESS_DENIED;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError =  SrvOpenEventDatabase(&hDB);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = SrvDeleteFromEventLog(
                    hDB,
                    (PWSTR)sqlFilter);
    BAIL_ON_CLTR_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

error:

    goto cleanup;
}

DWORD
RpcCltrGetPushRate(
    RPC_LWCOLLECTOR_HANDLE hCollector,
    DWORD dwEventsWaiting,
    double dPeriodConsumed,
    DWORD *pdwRecordsPerPeriod,
    DWORD *pdwRecordsPerBatch,
    DWORD *pdwPeriodSecs
    )
{
    DWORD dwError = 0;

    CLTR_LOG_VERBOSE("%s called", __FUNCTION__);
    dwError = CltrGetRecordsPerPeriod(pdwRecordsPerPeriod);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrGetRecordsPerBatch(pdwRecordsPerBatch);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrGetPeriodSeconds(pdwPeriodSecs);
    BAIL_ON_CLTR_ERROR(dwError);

cleanup:
    CLTR_LOG_VERBOSE("%s finished (result = %d)", __FUNCTION__, dwError);
    return dwError;

error:
    *pdwRecordsPerPeriod = 0;
    *pdwRecordsPerBatch = 0;
    *pdwPeriodSecs = 0;

    goto cleanup;
}

DWORD
CltrRegisterForRPC(
    PWSTR pszServiceName,
    RPC_BINDING_VECTOR ** ppServerBinding
    )
{
    DWORD status = 0;
    RPC_BINDING_VECTOR * pServerBinding = NULL;
    BOOLEAN bRegistered = FALSE;
    BOOLEAN bBound = FALSE;
    BOOLEAN bEPRegistered = FALSE;
    
    status = RpcServerRegisterIf (LWCollector_v1_0_s_ifspec,
                            NULL,
                            NULL
                            );
    BAIL_ON_CLTR_ERROR(status);

    bRegistered = TRUE;

    CLTR_LOG_INFO("RPC Service registered successfully.");

    status = RpcServerUseProtseq(TEXT("ncacn_ip_tcp"), RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL);
    BAIL_ON_DCE_ERROR(status);
    status = RpcServerUseProtseqEp(TEXT("ncacn_np"), 0, TEXT("\\pipe\\lweventcltr"), NULL);
    BAIL_ON_DCE_ERROR(status);

    status = RpcServerInqBindings(&pServerBinding);
    BAIL_ON_DCE_ERROR(status);
    
    bBound = TRUE;

    RpcEpRegister(LWCollector_v1_0_s_ifspec,
                    pServerBinding,
                    NULL,
                    pszServiceName
                    );
    BAIL_ON_CLTR_ERROR(status);

    bEPRegistered = TRUE;

    CLTR_LOG_INFO("RPC Endpoint registered successfully.");

    *ppServerBinding = pServerBinding;

cleanup:

    return status;

error:
    
    if (bBound) {
        DWORD tmpStatus = 0;
        tmpStatus = RpcEpUnregister(LWCollector_v1_0_s_ifspec,
                         pServerBinding,
                         NULL
                         );
    }

    if (bEPRegistered) {
        DWORD tmpStatus = 0;
        tmpStatus  = RpcServerUnregisterIf (LWCollector_v1_0_s_ifspec,
                                NULL,
                                0
                                );
    }

    *ppServerBinding = NULL;

    goto cleanup;
}

DWORD
CltrListenForRPC()
{
    DWORD status = 0;

    status = RpcServerListen(1, 20, 0);
    
    return status;
}

DWORD
CltrUnregisterForRPC(
    RPC_BINDING_VECTOR * pServerBinding
    
    )
{
    DWORD status = 0;

    CLTR_LOG_INFO("Unregistering server from the endpoint mapper...");
    status = RpcEpUnregister(LWCollector_v1_0_s_ifspec,
                        pServerBinding,
                        NULL
                        );
    BAIL_ON_DCE_ERROR(status);

    CLTR_LOG_INFO("Cleaning up the communications endpoints...");
    status = RpcServerUnregisterIf (LWCollector_v1_0_s_ifspec,
                             NULL,
                             0
                             );
    BAIL_ON_DCE_ERROR(status);

error:

    return status;
}
