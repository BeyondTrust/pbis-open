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
    rpc_binding_vector_p_t *ppSrvBinding,
    rpc_if_handle_t SrvInterface
    )
{
    DWORD dwError = 0;
    unsigned32 rpcstatus = rpc_s_ok;

    if (*ppSrvBinding) {
        rpc_ep_unregister(SrvInterface,
                          *ppSrvBinding,
                          NULL,
                          &rpcstatus);
        if (rpcstatus == ept_s_not_registered)
        {
            // The end point mapper does not have this binding registered.
            // Someone may have cleared the end point mapper database, or
            // another RPC server with the same end point name already
            // unregistered itself.
            rpcstatus = 0;
        }
        BAIL_ON_DCERPC_ERROR(rpcstatus);

        rpc_binding_vector_free(ppSrvBinding, &rpcstatus);

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
