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

#include "stdafx.h"

//SPN changing from RPC_CSTR to RPC_WSTR
#define RPC_STRING_BINDING_COMPOSE(protocol, hostname, pBindingString, pStatus) \
    *pStatus = RpcStringBindingComposeW(NULL, (RPC_WSTR)(protocol), (RPC_WSTR)(hostname), NULL, NULL, (RPC_WSTR*)(pBindingString))

#define RPC_BINDING_FROM_STRING_BINDING(bindingString, pBindingHandle, pStatus) \
    *pStatus = RpcBindingFromStringBindingW((RPC_WSTR)(bindingString), (pBindingHandle))

#define RPC_BINDING_SET_AUTH_INFO(BindingHandle, srv_principal, prot_level, \
                  auth_mech, IdHandle, auth_svc, pStatus) \
    *pStatus = RpcBindingSetAuthInfoW((BindingHandle), (srv_principal), (prot_level), \
                  (auth_mech), (IdHandle), (auth_svc))

#define RPC_STRING_FREE(pString, pStatus) \
    *pStatus = RpcStringFreeW((RPC_WSTR*)pString)

#define RPC_BINDING_FREE(pBindingHandle, pStatus) \
    *pStatus = RpcBindingFree(pBindingHandle)


DWORD
CltrCreateEventLogRpcBinding(
    IN const WCHAR * hostname,
    IN OPTIONAL const WCHAR * pwszServicePrincipal,
    IN WCHAR**       ppszBindingString,
    IN handle_t *   event_binding
    )
{
    DWORD dwError = 0;
    WCHAR * pszBindingString = NULL;
    handle_t eventBinding_local = 0;
    WCHAR szProt[] = {'n','c','a','c','n','_','n','p',0};
    WCHAR szEndPoint[] = {'\\', 'p', 'i', 'p', 'e', '\\', 'l', 'w', 'e', 'v', 'e', 'n', 't', 'c', 'l', 't', 'r', 0};
    PWSTR pwszServicePrincipalLocal = NULL;
    size_t sServicePrincipalLocalSize = 0;

    /* we can't create binding without a proper rpc server address */
    if (hostname == NULL)
    {
    return RPC_S_INVALID_NET_ADDR;
    }

    CLTR_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() hostname=%ws, *event_binding=%p\n",
                    hostname, *event_binding);

    dwError = RpcStringBindingComposeW(NULL, szProt, (WCHAR*)hostname, szEndPoint, NULL, &pszBindingString);

    if (dwError != 0 || pszBindingString == NULL || *pszBindingString == '\0')
    goto error;

    CLTR_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() pszBindingString=%s, running rbfsb\n",
            pszBindingString);

    RPC_BINDING_FROM_STRING_BINDING(pszBindingString, &eventBinding_local, &dwError);

    CLTR_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() eventBinding_local=%p, finished rbfsb\n",
            eventBinding_local);

    if (dwError != 0)
        goto error;

    if (hostname != NULL && pwszServicePrincipal == NULL)
    {
        /* Set up authentication if we are connecting to a remote host */
        sServicePrincipalLocalSize = wc16slen(hostname) + 6;
        
        dwError = LwRtlWC16StringAllocatePrintfW(
                        &pwszServicePrincipalLocal,
                        L"host/%ws",
                        hostname);
        BAIL_ON_CLTR_ERROR(dwError);
        
        pwszServicePrincipal = pwszServicePrincipalLocal;
    }

    if (pwszServicePrincipal != NULL)
    {
        CLTR_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() using host principal [%ws]\n",
                        pwszServicePrincipal);
        
        RPC_BINDING_SET_AUTH_INFO(
            eventBinding_local,
            (PWSTR)pwszServicePrincipal,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_AUTHN_GSS_NEGOTIATE,
            NULL,
            RPC_C_AUTHZ_NAME,
            &dwError);
        if (dwError != 0)
            goto error;
        
        CLTR_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() eventBinding_local=%p, auth info set dwError=0x%08x\n",
                        eventBinding_local, dwError);
        
    }

    *ppszBindingString = pszBindingString;

    *event_binding = eventBinding_local;

    CLTR_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() finished successfully\n",
            dwError);

cleanup:

    LwRtlWC16StringFree(&pwszServicePrincipalLocal);

    return dwError;

error:

    CLTR_LOG_VERBOSE("client::eventlogbinding.c: CreateEventLogRpcBinding() label error: dwError=%d\n",
            dwError);

    if (pszBindingString)
    {
        DWORD tempstatus = 0;
        RPC_STRING_FREE(&pszBindingString, &tempstatus);
    }

    goto cleanup;
}

DWORD
CltrFreeEventLogRpcBinding(
    handle_t event_binding,
    WCHAR * pszBindingString
    )
{
    DWORD rpcstatus = 0;

    /* Free the binding itself */
    if (event_binding != NULL) {
        RPC_BINDING_FREE(&event_binding, &rpcstatus);
    }

    /* Free the rpc binding string (created by Compose function) */
    if (pszBindingString != NULL) {
        RPC_STRING_FREE(&pszBindingString, &rpcstatus);
    }

    return rpcstatus;
}

