#include "api.h"


static
DWORD
RpcSvcCreateDomainSocketPath(
    PCSTR pszPath
    );


static
DWORD
RpcSvcCreateDirectory(
    PSTR pszDirName,
    mode_t DirMode
    );


static
DWORD
RpcSvcInitServerBinding(
    rpc_binding_vector_p_t *ppSrvBinding,
    PENDPOINT pEndPoints
    );


static
DWORD
RpcSvcCreateDomainSocketPath(
    PCSTR pszPath
    )
{
    const mode_t PathMode = 0655;
    const mode_t DirMode = 0755;

    DWORD dwError = 0;
    PSTR pszSocketPath = NULL;
    PSTR pszSocketName = NULL;
    PSTR pszDirName = NULL;

    dwError = LwAllocateString(pszPath, &pszSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    pszSocketName = strrchr(pszSocketPath, '/');

    if (!pszSocketName) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *(pszSocketName++) = '\0';
    pszDirName = pszSocketPath;

    dwError = RpcSvcCreateDirectory(pszDirName, PathMode);
    BAIL_ON_LSA_ERROR(dwError);

    if (chmod(pszDirName, DirMode))
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pszSocketPath) {
        LW_SAFE_FREE_STRING(pszSocketPath);
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
RpcSvcCreateDirectory(
    PSTR pszDirPath,
    mode_t DirMode
    )
{
    DWORD dwError = 0;
    struct stat statbuf;
    PSTR pszSlash = NULL;

    for (pszSlash = strchr(pszDirPath, '/');
         pszSlash != NULL;
         pszSlash = strchr(pszSlash + 1, '/'))
    {
        if (pszSlash == pszDirPath)
        {
            continue;
        }

        *pszSlash = '\0';

        if (stat(pszDirPath, &statbuf) == 0)
        {
            /* Make sure it's a directory */
            if (!S_ISDIR(statbuf.st_mode))
            {
                dwError = ERROR_FILE_NOT_FOUND;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else
        {
            /* Create it */
            if (mkdir(pszDirPath, DirMode))
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        *pszSlash = '/';
    }

    if (stat(pszDirPath, &statbuf) == 0)
    {
        /* Make sure its a directory */
        if (!S_ISDIR(statbuf.st_mode))
        {
            dwError = ERROR_FILE_NOT_FOUND;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        /* Create it */
        if (mkdir(pszDirPath, DirMode))
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }  
    
error:
    if (pszSlash) {
        *pszSlash = '/';
    }

    return dwError;
}


DWORD
RpcSvcRegisterRpcInterface(
    rpc_if_handle_t SrvInterface
    )
{
    DWORD dwError = 0;
    unsigned32 rpcstatus = rpc_s_ok;

    DCETHREAD_TRY
    {
        rpc_server_register_if(SrvInterface,
                               NULL,
                               NULL,
                               &rpcstatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!rpcstatus) {
            rpcstatus = dcethread_exc_getstatus(THIS_CATCH);
        }

        if (!rpcstatus) {
                dwError = LW_ERROR_RPC_SERVER_REGISTRATION_ERROR;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_DCERPC_ERROR(rpcstatus);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RpcSvcBindRpcInterface(
    rpc_binding_vector_p_t *ppSrvBinding,
    rpc_if_handle_t SrvInterface,
    PENDPOINT pEndPoints,
    PCSTR pszSrvDescription
    )
{
    DWORD dwError = 0;
    unsigned32 rpcstatus = rpc_s_ok;

    DCETHREAD_TRY
    {
        dwError = RpcSvcInitServerBinding(ppSrvBinding, pEndPoints);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!dwError) {
            rpcstatus = dcethread_exc_getstatus(THIS_CATCH);
        }

        if (!rpcstatus) {
            dwError = LW_ERROR_RPC_SERVER_REGISTRATION_ERROR;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_DCERPC_ERROR(rpcstatus);
    BAIL_ON_LSA_ERROR(dwError);

    DCETHREAD_TRY
    {
        rpc_ep_register(SrvInterface,
                        *ppSrvBinding,
                        NULL,
                        (unsigned char*)pszSrvDescription,
                        &rpcstatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!dwError) {
            rpcstatus = dcethread_exc_getstatus(THIS_CATCH);
        }

        if (!rpcstatus) {
            dwError = LW_ERROR_RPC_SERVER_REGISTRATION_ERROR;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_DCERPC_ERROR(rpcstatus);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
DWORD
RpcSvcInitServerBinding(
    rpc_binding_vector_p_t *ppSrvBinding,
    PENDPOINT pEndPoints
    )
{
    DWORD dwError = 0;
    DWORD rpcstatus = rpc_s_ok;
    DWORD i = 0;
    BOOLEAN bIsLocalInterface = FALSE;

    for (i = 0; pEndPoints[i].pszProtocol != NULL; i++)
    {
        bIsLocalInterface = (!strcmp(pEndPoints[i].pszProtocol, "ncalrpc")) &&
                            (pEndPoints[i].pszEndpoint[0] == '/');

        if (!pEndPoints[i].pszEndpoint)
        {
            rpc_server_use_protseq((unsigned char*) pEndPoints[i].pszProtocol,
                                   rpc_c_protseq_max_calls_default,
                                   (unsigned32*)&rpcstatus);
            BAIL_ON_DCERPC_ERROR(rpcstatus);
        }
        else
        {
            if (bIsLocalInterface)
            {
                dwError = RpcSvcCreateDomainSocketPath(pEndPoints[i].pszEndpoint);
                BAIL_ON_LSA_ERROR(dwError);

            }

            rpc_server_use_protseq_ep((unsigned char*)pEndPoints[i].pszProtocol,
                                      rpc_c_protseq_max_calls_default,
                                      (unsigned char*)pEndPoints[i].pszEndpoint,
                                      (unsigned32*)&rpcstatus);
            BAIL_ON_DCERPC_ERROR(rpcstatus);
        }
    }

    rpc_server_inq_bindings(ppSrvBinding, (unsigned32*)&rpcstatus);
    BAIL_ON_DCERPC_ERROR(rpcstatus);

error:
    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
