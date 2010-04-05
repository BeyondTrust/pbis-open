#include "api.h"


DWORD
RpcSvcUnregisterRpcInterface(
    rpc_if_handle_t SrvInterface
    )
{
    DWORD dwError = 0;
    unsigned32 rpcstatus = rpc_s_ok;

    rpc_server_unregister_if(SrvInterface,
                             NULL,
                             &rpcstatus);
    BAIL_ON_DCERPC_ERROR(rpcstatus);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RpcSvcUnbindRpcInterface(
    rpc_binding_vector_p_t pSrvBinding,
    rpc_if_handle_t SrvInterface
    )
{
    DWORD dwError = 0;
    unsigned32 rpcstatus = rpc_s_ok;

    if (pSrvBinding) {
        rpc_ep_unregister(SrvInterface,
                          pSrvBinding,
                          NULL,
                          &rpcstatus);
        BAIL_ON_DCERPC_ERROR(rpcstatus);

        rpc_binding_vector_free(&pSrvBinding, &rpcstatus);
        pSrvBinding = NULL;

        BAIL_ON_DCERPC_ERROR(rpcstatus);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
