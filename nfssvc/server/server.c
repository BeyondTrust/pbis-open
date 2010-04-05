/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 * Likewise Server Service
 *
 */
#include "includes.h"

typedef const struct
{
    PCSTR protocol;
    PCSTR endpoint;
} ENDPOINT, *PENDPOINT;


static
DWORD
mkdir_recursive(PSTR pszPath, mode_t mode)
{
    DWORD dwError = 0;
    struct stat statbuf;
    PSTR pszSlash = NULL;

    for(pszSlash = strchr(pszPath, '/'); pszSlash; pszSlash = strchr(pszSlash + 1, '/'))
    {
        if (pszSlash == pszPath)
        {
            continue;
        }

        *pszSlash = '\0';

        if (stat(pszPath, &statbuf) == 0)
        {
            /* Make sure its a directory */
            if (!S_ISDIR(statbuf.st_mode))
            {
                dwError = ENOENT;
                BAIL_ON_NFSSVC_ERROR(dwError);
            }
        }
        else
        {
            /* Create it */
            if (mkdir(pszPath, mode))
            {
                dwError = errno;
                BAIL_ON_NFSSVC_ERROR(dwError);
            }
        }

        *pszSlash = '/';
    }

    if (stat(pszPath, &statbuf) == 0)
    {
        /* Make sure its a directory */
        if (!S_ISDIR(statbuf.st_mode))
        {
            dwError = ENOENT;
            BAIL_ON_NFSSVC_ERROR(dwError);
            }
    }
    else
    {
        /* Create it */
        if (mkdir(pszPath, mode))
        {
                dwError = errno;
                BAIL_ON_NFSSVC_ERROR(dwError);
        }
    }  
    
error:
    if (pszSlash)
        *pszSlash = '/';

    return dwError;
}

static
DWORD
prepare_domain_socket(PCSTR pszPath)
{
    DWORD dwError = 0;
    PSTR pszPathCopy = NULL;
    PSTR pszDirname = NULL;
    PSTR pszBasename = NULL;

    pszPathCopy = strdup(pszPath);
    if (!pszPathCopy)
    {
        dwError = ENOMEM;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }
    
    pszBasename = strrchr(pszPathCopy, '/');
    
    if (!pszBasename)
    {
        dwError = EINVAL;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

    *(pszBasename++) = '\0';

    pszDirname = pszPathCopy;
    
    dwError = mkdir_recursive(pszDirname, 0655);
    BAIL_ON_NFSSVC_ERROR(dwError);
    
    /* Ensure directory is only accessible by root */
    if (chmod(pszDirname, 0600))
    {
        dwError = errno;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

error:
    
    if (pszPathCopy)
        free(pszPathCopy);

    return dwError;
}

static
DWORD
bind_server(
    rpc_binding_vector_p_t * server_binding,
    rpc_if_handle_t interface_spec,
    PENDPOINT pEndPoints
    )
{
    DWORD dwError = 0;
    DWORD dwRpcStatus = 0;
    DWORD i;

    /*
     * Prepare the server binding handle
     * use all avail protocols (UDP and TCP). This basically allocates
     * new sockets for us and associates the interface UUID and
     * object UUID of with those communications endpoints.
     */
    for (i = 0; pEndPoints[i].protocol != NULL; i++)
    {
        if (!pEndPoints[i].endpoint)
        {
            rpc_server_use_protseq((unsigned char*) pEndPoints[i].protocol, 
                                   rpc_c_protseq_max_calls_default,
                                   (unsigned32*)&dwRpcStatus);
            BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
        }
        else
        {
            if (!strcmp(pEndPoints[i].protocol, "ncalrpc") &&
                pEndPoints[i].endpoint[0] == '/')
            {
                dwError = prepare_domain_socket(pEndPoints[i].endpoint);
                BAIL_ON_NFSSVC_ERROR(dwError);
            }

            rpc_server_use_protseq_ep((unsigned char*) pEndPoints[i].protocol,
                                      rpc_c_protseq_max_calls_default,
                                      (unsigned char*) pEndPoints[i].endpoint,
                                      (unsigned32*)&dwRpcStatus);
            BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
        }
    }

    rpc_server_inq_bindings(server_binding, (unsigned32*)&dwRpcStatus);
    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);

error:

    return dwError;
}

DWORD
NfsSvcRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    )
{
    volatile DWORD dwError = 0;
    volatile DWORD dwRpcStatus = 0;
    rpc_binding_vector_p_t pServerBinding = NULL;
    BOOLEAN bRegistered = FALSE;
    BOOLEAN bBound = FALSE;
    BOOLEAN bEPRegistered = FALSE;
    static ENDPOINT endpoints[] =
    {
        {"ncacn_ip_tcp", NULL},
        {"ncacn_np"    , "\\\\pipe\\\\nfssvc"},
        {"ncalrpc", "nfssvc"},
        {NULL, NULL}
    };

    DCETHREAD_TRY
    {
        rpc_server_register_if (nfssvc_v3_0_s_ifspec,
                                NULL,
                                NULL,
                                (unsigned32*)&dwRpcStatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if ( dwRpcStatus == RPC_S_OK )
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
            if(!dwError)
            {
                dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_REGISTER;
            }
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_NFSSVC_ERROR(dwError);

    bRegistered = TRUE;
    NFSSVC_LOG_INFO("RPC Service registered successfully.");

    DCETHREAD_TRY
    {
        dwError = bind_server(&pServerBinding,
                              nfssvc_v3_0_s_ifspec,
                              endpoints);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if(!dwError)
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
        }
        if(!dwError)
        {
            dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_REGISTER;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_NFSSVC_ERROR(dwError);

    bBound = TRUE;

    DCETHREAD_TRY
    {
        rpc_ep_register(nfssvc_v3_0_s_ifspec,
                        pServerBinding,
                        NULL,
                        (idl_char*)pszServiceName,
                        (unsigned32*)&dwRpcStatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if ( dwRpcStatus == RPC_S_OK )
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
            if(!dwError)
            {
                dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_REGISTER;
            }
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_NFSSVC_ERROR(dwError);

    bEPRegistered = TRUE;
    NFSSVC_LOG_INFO("RPC Endpoint registered successfully.");

    *ppServerBinding = pServerBinding;

cleanup:

    return dwError;

error:

    NFSSVC_LOG_ERROR("Failed to register RPC endpoint.  Error Code: [%u]\n", dwError);

    if (bEPRegistered)
    {
        DCETHREAD_TRY
        {
            DWORD tmpStatus = 0;
            rpc_ep_unregister(nfssvc_v3_0_s_ifspec,
                              pServerBinding,
                              NULL,
                              (unsigned32*)&tmpStatus);
        }
        DCETHREAD_CATCH_ALL(THIS_CATCH)
        DCETHREAD_ENDTRY;
    }

    if (bBound) {
        DCETHREAD_TRY
        {
            DWORD tmpStatus = 0;
            rpc_binding_vector_free(&pServerBinding,
                                    (unsigned32*)&tmpStatus);
        }
        DCETHREAD_CATCH_ALL(THIS_CATCH)
        DCETHREAD_ENDTRY;
    }

    if (bRegistered)
    {
        DCETHREAD_TRY
        {
            DWORD tmpStatus = 0;
            rpc_server_unregister_if (nfssvc_v3_0_s_ifspec,
                                      NULL,
                                      (unsigned32*)&tmpStatus);
        }
        DCETHREAD_CATCH_ALL(THIS_CATCH)
        DCETHREAD_ENDTRY;
    }

    *ppServerBinding = NULL;

    goto cleanup;
}

PVOID
NfsSvcListenForRPC(
    PVOID pArg
    )
{
    volatile DWORD dwError = 0;

    DCETHREAD_TRY
    {
        rpc_server_listen(rpc_c_listen_max_calls_default, (unsigned32*)&dwError);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!dwError)
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
        }
        if(!dwError)
        {
            dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_LISTEN;
        }
    }
    DCETHREAD_ENDTRY

    BAIL_ON_NFSSVC_ERROR(dwError);

cleanup:

    raise(SIGTERM);

    return;

error:

    NFSSVC_LOG_ERROR("Failed to begin RPC listening.  Error code [%d]\n", dwError);

    goto cleanup;
}

DWORD
NfsSvcUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    )
{
    volatile DWORD dwError = 0;
    volatile DWORD dwRpcStatus = 0;

    DCETHREAD_TRY
    {
        NFSSVC_LOG_INFO("Unregistering server from the endpoint mapper...");
        rpc_ep_unregister(nfssvc_v3_0_s_ifspec,
                            pServerBinding,
                            NULL,
                            (unsigned32*)&dwRpcStatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if ( dwRpcStatus == RPC_S_OK )
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
            if(!dwError)
            {
                dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_UNREGISTER;
            }
        }
    }
    DCETHREAD_ENDTRY

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_NFSSVC_ERROR(dwError);

    DCETHREAD_TRY
    {
        rpc_binding_vector_free(&pServerBinding, (unsigned32*)&dwRpcStatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if ( dwRpcStatus == RPC_S_OK )
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
            if(!dwError)
            {
                dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_UNREGISTER;
            }
        }
    }
    DCETHREAD_ENDTRY
    
    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_NFSSVC_ERROR(dwError);

    DCETHREAD_TRY
    {
        NFSSVC_LOG_INFO("Cleaning up the communications endpoints...");
        rpc_server_unregister_if (nfssvc_v3_0_s_ifspec,
                                 NULL,
                                 (unsigned32*)&dwRpcStatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if ( dwRpcStatus == RPC_S_OK )
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
            if(!dwError)
            {
                dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_UNREGISTER;
            }
        }
    }
    DCETHREAD_ENDTRY

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_NFSSVC_ERROR(dwError);

cleanup:
    return dwError;

error:
    NFSSVC_LOG_ERROR("Failed to unregister RPC endpoint.  Error code [%d]\n", dwError);
    goto cleanup;
}

BOOLEAN
NfsSvcRpcIsListening(
    VOID
    )
{
    volatile DWORD dwError = 0;
    BOOLEAN bIsListening = FALSE;

    DCETHREAD_TRY
    {
        bIsListening = rpc_mgmt_is_server_listening(NULL, (unsigned32*)&dwError);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!dwError)
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
        }
        if (!dwError)
        {
            dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_LISTEN;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_NFSSVC_ERROR(dwError);

cleanup:

    return bIsListening;

error:

    bIsListening = FALSE;

    goto cleanup;
}

DWORD
NfsSvcRpcStopListening(
    VOID
    )
{
    volatile DWORD dwError = 0;

    DCETHREAD_TRY
    {
        rpc_mgmt_stop_server_listening(NULL, (unsigned32*)&dwError);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!dwError)
        {
            dwError = dcethread_exc_getstatus (THIS_CATCH);
        }
        if(!dwError)
        {
            dwError = NFSSVC_ERROR_RPC_EXCEPTION_UPON_LISTEN;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_NFSSVC_ERROR(dwError);

cleanup:

    return dwError;

error:

    NFSSVC_LOG_ERROR("Failed to stop RPC listening.  Error code [%d]\n", dwError);

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
