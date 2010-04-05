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
LWMGMTOpenLsaServer(
    PCSTR     pszServerName,
    char**    ppszBindingString,
    handle_t* phLsaServer
    )
{
    DWORD dwError = 0;
    handle_t hLsaServer = NULL;
    char* bindingStringLocal = NULL;

    TRY
    {
        dwError = LWMGMTCreateRpcBinding(
                    (IsNullOrEmptyString(pszServerName) ? "127.0.0.1" :
                                                          pszServerName),
                    &bindingStringLocal,
                    &hLsaServer);
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

    *phLsaServer = hLsaServer;
    *ppszBindingString = bindingStringLocal;

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to connect to lsa server. Error code [%d]\n", dwError);

    goto cleanup;
}

DWORD
LWMGMTGetLsaMetrics(
    handle_t       hLsaServer,
    idl_usmall_int infoLevel,
    LSAMETRICPACK* pMetricPack
    )
{
    DWORD dwError = 0;

    TRY
    {
        dwError = RpcLWMgmtEnumPerformanceMetrics(
                      hLsaServer,
                      infoLevel,
                      pMetricPack);
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

cleanup:

    return dwError;

error:

    goto cleanup;
} 

DWORD
LWMGMTGetLsaStatus(
    handle_t         hLsaServer,
    LWMGMTLSASTATUS* pLsaStatus
    )
{
    DWORD dwError = 0;

    TRY
    {
        dwError = RpcLWMgmtGetStatus(
                      hLsaServer,
                      pLsaStatus);
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

cleanup:

    return dwError;

error:

    goto cleanup;
} 

DWORD
LWMGMTCloseLsaServer(
    handle_t hLsaServer,
    char*    pszBindingString
    )
{
    DWORD dwError = 0;

    TRY
    {
        dwError = LWMGMTFreeRpcBinding(
                      hLsaServer,
                      pszBindingString);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    CATCH_ALL
    {
        dwError = LWMGMTGetRpcError(THIS_CATCH, LWMGMT_ERROR_RPC_EXCEPTION_UPON_CLOSE);
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to close lsa server. Error code [%d]\n", dwError);
    goto cleanup;
}

