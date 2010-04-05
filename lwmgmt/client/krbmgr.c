/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Client API
 *
 */
#include "includes.h"

DWORD
LWIOpenKeyTabServer(
    PCSTR      pszServerName,
    char **    ppszBindingString,
    handle_t * phKeyTabServer
    )
{
    DWORD    dwError = 0;
    handle_t hKeyTabServer;
    char *   bindingStringLocal = NULL;

    TRY
    {
        dwError = LWMGMTCreateRpcBinding(
                      (IsNullOrEmptyString(pszServerName) ?
                      "127.0.0.1" : pszServerName),
                      &bindingStringLocal,
                      &hKeyTabServer);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    CATCH_ALL
    {
        dwError = LWMGMTGetRpcError(
                      THIS_CATCH,
                      LWMGMT_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING);
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

    *phKeyTabServer = hKeyTabServer;
    *ppszBindingString = bindingStringLocal;

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to connect to keytab server. Error code [%d]\n", dwError);

    goto cleanup;
}

DWORD
LWIReadKeyTab(
    handle_t             hKeyTabServer,
    PCSTR                pszKeyTabPath,
    DWORD                dwLastRecordId,
    DWORD                nRecordsPerPage,
    LSA_KEYTAB_ENTRIES * pKeyTabEntries
    )
{
    DWORD        dwError = 0;

    TRY
    {
        dwError = RpcLWIReadKeyTab(
                      hKeyTabServer,
                      (idl_char *)(pszKeyTabPath ? pszKeyTabPath : ""),
                      dwLastRecordId, 
                      nRecordsPerPage,
                      pKeyTabEntries);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    CATCH_ALL
    {
        dwError = LWMGMTGetRpcError(
                      THIS_CATCH,
                      LWMGMT_ERROR_RPC_EXCEPTION_UPON_READ);
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to read keytab. Error code [%d]\n", dwError);

    goto cleanup;
}

DWORD
LWICountKeyTabEntries(
    handle_t hKeyTabServer,
    PCSTR    pszKeyTabPath,
    DWORD*   pdwCount
    )
{
    DWORD        dwError = 0;
    idl_long_int nCount = 0;

    TRY
    {
        dwError = RpcLWIKeyTabCount(
                      hKeyTabServer,
                      (idl_char *)(pszKeyTabPath ? pszKeyTabPath : ""),
                      &nCount); BAIL_ON_LWMGMT_ERROR(dwError);
    }
    CATCH_ALL
    {
        dwError = LWMGMTGetRpcError(
                      THIS_CATCH,
                      LWMGMT_ERROR_RPC_EXCEPTION_UPON_COUNT);
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

    *pdwCount = nCount;

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to count keytab. Error code [%d]\n", dwError);

    goto cleanup;
}

DWORD
LWIWriteKeyTabEntry(
    handle_t         hKeyTabServer,
    PCSTR            pszKeyTabPath,
    LSA_KEYTAB_ENTRY KeyTabEntry
    )
{
    DWORD  dwError = 0;

    TRY
    {
        dwError = RpcLWIWriteKeyTabEntry(
                      hKeyTabServer,
                      (idl_char *)(pszKeyTabPath ? pszKeyTabPath : ""),
                      KeyTabEntry);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    CATCH_ALL
    {
        dwError = LWMGMTGetRpcError(
                      THIS_CATCH,
                      LWMGMT_ERROR_RPC_EXCEPTION_UPON_WRITE);        
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    ENDTRY

cleanup:

    return dwError;
    
error:

    LWMGMT_LOG_ERROR("Failed to write to keytab. Error code [%d]\n", dwError);

    goto cleanup;
}

DWORD
LWIDeleteFromKeyTab(
    handle_t         hKeyTabServer,
    PCSTR            pszKeyTabPath,
    LSA_KEYTAB_ENTRY KeyTabEntry
    )
{
    DWORD  dwError = 0;

    TRY
    {
        dwError = RpcLWIDeleteFromKeyTab(
                      hKeyTabServer,
                      (idl_char *)(pszKeyTabPath ? pszKeyTabPath : ""),
                      KeyTabEntry);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    CATCH_ALL
    {
        dwError = LWMGMTGetRpcError(
                      THIS_CATCH,
                      LWMGMT_ERROR_RPC_EXCEPTION_UPON_DELETE);
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to delete entry from keytab. Error code [%d]\n", dwError);

    goto cleanup;
}

DWORD
LWIClearKeyTab(
    handle_t hKeyTabServer,
    PCSTR    pszKeyTabPath
    )
{
    DWORD  dwError = 0;

    TRY
    {
        dwError = RpcLWIClearKeyTab(
                      hKeyTabServer,
                      (idl_char *)(pszKeyTabPath ? pszKeyTabPath : ""));
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    CATCH_ALL
    {
        dwError = LWMGMTGetRpcError(
                      THIS_CATCH,
                      LWMGMT_ERROR_RPC_EXCEPTION_UPON_CLEAR);
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to clear keytab. Error code [%d]\n", dwError);

    goto cleanup;

}

DWORD
LWICloseKeyTabServer(
    handle_t hKeyTabServer,
    char *   pszBindingString
    )
{
    DWORD dwError = 0;

    TRY
    {
        dwError = LWMGMTFreeRpcBinding(
                      hKeyTabServer,
                      pszBindingString);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    CATCH_ALL
    {
        dwError = LWMGMTGetRpcError(
                      THIS_CATCH,
                      LWMGMT_ERROR_RPC_EXCEPTION_UPON_CLOSE);
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to close keytab servver. Error code [%d]\n", dwError);

    goto cleanup;
}

