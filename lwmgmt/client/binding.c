/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Client RPC Binding
 *
 */
#include "includes.h"

#define HOST_PREFIX "host/"

DWORD
LWMGMTCreateRpcBinding(
    const char * pszHostname,
    char**       ppszBindingString,
    handle_t *   ppBinding
    )
{
    DWORD rpcstatus = 0;
    DWORD dwError = 0;
    char * protocol = "ncacn_ip_tcp";
    char * pszBindingString = NULL;
    char * hostPrincipal = NULL;
    size_t hostPrincipalSize = 0;
    int ret = 0;
    handle_t pBinding = 0;
    char * pszHostname2 = NULL;

    /* we can't create binding without a proper rpc server address */
    if (pszHostname == NULL) {
        rpcstatus = rpc_s_inval_net_addr;
        goto error;
    }

    dwError = LWMGMTAllocateString(
                  pszHostname,
                  &pszHostname2);
    BAIL_ON_LWMGMT_ERROR(dwError);

    LWMGMTStrToUpper(pszHostname2);

    RPC_STRING_BINDING_COMPOSE(
                    protocol, 
                    (char*)pszHostname2, 
                    &pszBindingString, 
                    &rpcstatus);
    if (rpcstatus != 0 ||
        pszBindingString == NULL ||
        *pszBindingString == '\0')
    {
        goto error;
    }

    RPC_BINDING_FROM_STRING_BINDING(
                    pszBindingString,
                    &pBinding,
                    &rpcstatus);
    if (rpcstatus != 0) {
        goto error;
    }

    hostPrincipalSize = strlen(pszHostname2) + sizeof(HOST_PREFIX);

    dwError = LWMGMTAllocateMemory(
                    hostPrincipalSize, 
                    (PVOID*)&hostPrincipal);
    BAIL_ON_LWMGMT_ERROR(dwError);

    ret = snprintf(hostPrincipal,
                   hostPrincipalSize, 
                   "%s%s", 
                   HOST_PREFIX,
                   pszHostname2);
    if (ret < 0 || ret >= hostPrincipalSize) {
        goto error;
    }

    RPC_BINDING_SET_AUTH_INFO(
                  pBinding,
			      (unsigned char*)hostPrincipal,
			      rpc_c_protect_level_pkt_privacy,
			      rpc_c_authn_gss_negotiate,
			      NULL,
			      rpc_c_authz_name,
			      &rpcstatus);
    if (rpcstatus != 0) {
        goto error;
    }

    *ppszBindingString = pszBindingString;
    *ppBinding = pBinding;

cleanup:

    if (hostPrincipal) {
        LWMGMTFreeMemory(hostPrincipal);
    }

    if (pszHostname2)
    {
       LWMGMTFreeString(pszHostname2);
    }

    return rpcstatus;

error:

    LWMGMT_LOG_ERROR(
            "Failed to create RPC binding [rpcstatus=%d]",
            rpcstatus);

    if (pszBindingString) {
        DWORD tempstatus = 0;
        RPC_STRING_FREE(&pszBindingString, &tempstatus);
    }
    
    *ppszBindingString = NULL;
    *ppBinding = NULL;

    goto cleanup;
}

DWORD
LWMGMTFreeRpcBinding(
    handle_t pBinding,
    char * pszBindingString
    )
{
    DWORD rpcstatus = 0;

    /* Free the binding itself */
    if (pBinding != NULL) {
        RPC_BINDING_FREE(&pBinding, &rpcstatus);
    }

    /* Free the rpc binding string (created by Compose function) */
    if (pszBindingString != NULL) {
        RPC_STRING_FREE(&pszBindingString, &rpcstatus);
    }

    return rpcstatus;
}

