#include "api.h"


static
PVOID
RpcSvcWorkerMain(
    void* pCtx
    );


DWORD
RpcSvcStartWorker(
    void
    )
{
    DWORD dwError = 0;
    int ret = 0;

    ret = pthread_create(&gRpcSrvWorker,
                         NULL,
                         RpcSvcWorkerMain,
                         NULL);
    if (ret) {
        dwError = LW_ERROR_INVALID_RPC_SERVER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
PVOID
RpcSvcWorkerMain(
    void* pCtx
    )
{
    unsigned32 dwRpcStatus = rpc_s_ok;

    DCETHREAD_TRY
    {
        rpc_server_listen(rpc_c_listen_max_calls_default, &dwRpcStatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!dwRpcStatus) {
            dwRpcStatus = dcethread_exc_getstatus(THIS_CATCH);
        }
    }
    DCETHREAD_ENDTRY;

    return NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
