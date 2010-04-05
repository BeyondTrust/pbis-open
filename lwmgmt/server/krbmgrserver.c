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
#include "includes.h"

idl_long_int
RpcLWIReadKeyTab(
    handle_t             bindingHandle,
    idl_char *           pszKeyTabPath,
    idl_long_int         nRecordsToSkip,
    idl_long_int         nRecordsPerPage,
    LSA_KEYTAB_ENTRIES * pEntries
    )
{
    idl_long_int       dwError = 0;
    idl_long_int       dwNumFound = 0;
    idl_long_int       dwCount = 0;
    krb5_context       krb5context = NULL;
    krb5_keytab        keytab = NULL;
    krb5_kt_cursor     ktcursor = NULL;
    krb5_keytab_entry  ktentry;
    char *             pszName = NULL;
    LSA_KEYTAB_ENTRY * pEntryArray = NULL;

    memset(&ktentry, 0, sizeof(ktentry));
    pEntries->count = 0;

    dwError = RPCAllocateMemory(
                  nRecordsPerPage * sizeof(LSA_KEYTAB_ENTRY),
                  (PVOID*)&pEntryArray);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = KrbMgrGetKeyTab(
                  pszKeyTabPath,
                  &krb5context,
                  &keytab);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = krb5_kt_start_seq_get(
                  krb5context,
                  keytab,
                  &ktcursor);
    BAIL_ON_KRB_ERROR(krb5context, dwError);

    while( dwNumFound < nRecordsPerPage )
    {
        dwError = krb5_kt_next_entry(
                      krb5context,
                      keytab,
                      &ktentry,
                      &ktcursor);
        if ( dwError == KRB5_KT_END )
        {
            dwError = 0;
            goto done;
        }
        BAIL_ON_KRB_ERROR(krb5context, dwError);

        if ( ! (++dwCount > nRecordsToSkip) )
        {
            dwError = krb5_free_keytab_entry_contents(
                          krb5context,
                          &ktentry);
            memset(&ktentry, 0, sizeof(ktentry));
            BAIL_ON_KRB_ERROR(krb5context, dwError);

            continue;
        }

        dwError = krb5_unparse_name(
                      krb5context,
                      ktentry.principal,
                      &pszName);
        BAIL_ON_KRB_ERROR(krb5context, dwError);

        if ( !dwError )
        {
            pEntryArray[dwNumFound].timestamp = ktentry.timestamp;
            pEntryArray[dwNumFound].kvno = ktentry.vno;
            pEntryArray[dwNumFound].enctype = ktentry.key.enctype;

            dwError = RPCAllocateString(
                          pszName,
                          (PSTR*)&pEntryArray[dwNumFound].pszPrincipal);
            BAIL_ON_LWMGMT_ERROR(dwError);

            dwNumFound++;
        }

        if ( pszName )
        {
            krb5_free_unparsed_name(
                krb5context,
                pszName);
            pszName = NULL;
        }
        dwError = krb5_free_keytab_entry_contents(
                      krb5context,
                      &ktentry);
        memset(&ktentry, 0, sizeof(ktentry));
        BAIL_ON_KRB_ERROR(krb5context, dwError);
    }

done:
    pEntries->count = dwNumFound;
    pEntries->pLsaKeyTabEntryArray = pEntryArray;

cleanup:
    if ( pszName )
    {
        krb5_free_unparsed_name(
            krb5context,
            pszName);
    }
    if ( ktcursor )
    {
        krb5_free_keytab_entry_contents(
            krb5context,
            &ktentry);
        krb5_kt_end_seq_get(
            krb5context,
            keytab,
            &ktcursor);
    }
    KrbMgrFreeKeyTab(
        krb5context,
        keytab);

    return dwError;

error:
    if (pEntryArray )
    {
        for ( dwCount = 0 ; dwCount < dwNumFound ; dwCount++ )
        {
            if ( pEntryArray[dwCount].pszPrincipal )
            {
                RPCFreeMemory( pEntryArray[dwCount].pszPrincipal );
            }
        }

        RPCFreeMemory(pEntryArray);
    }

    goto cleanup;
}

idl_long_int
RpcLWIKeyTabCount(
    handle_t       bindingHandle,
    idl_char *     pszKeyTabPath,
    idl_long_int * pdwCount
    )
{
    idl_long_int      dwError = 0;
    idl_long_int      dwNumFound = 0;
    krb5_context      krb5context = NULL;
    krb5_keytab       keytab = NULL;
    krb5_kt_cursor    ktcursor = NULL;
    krb5_keytab_entry ktentry;

    memset(&ktentry, 0, sizeof(ktentry));
    *pdwCount = 0;

    dwError = KrbMgrGetKeyTab(
                  pszKeyTabPath,
                  &krb5context,
                  &keytab);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = krb5_kt_start_seq_get(
                  krb5context,
                  keytab,
                  &ktcursor);
    BAIL_ON_KRB_ERROR(krb5context, dwError);

    while( 1 )
    {
        dwError = krb5_kt_next_entry(
                      krb5context,
                      keytab,
                      &ktentry,
                      &ktcursor);
        if ( dwError == KRB5_KT_END )
        {
            dwError = 0;
            goto done;
        }
        BAIL_ON_KRB_ERROR(krb5context, dwError);

        dwNumFound++;
        dwError = krb5_free_keytab_entry_contents(
                      krb5context,
                      &ktentry);
        memset(&ktentry, 0, sizeof(ktentry));
        BAIL_ON_KRB_ERROR(krb5context, dwError);
    }

done:

    *pdwCount = dwNumFound;

cleanup:

    if ( ktcursor )
    {
        krb5_free_keytab_entry_contents(
            krb5context,
            &ktentry);
        krb5_kt_end_seq_get(
            krb5context,
            keytab,
            &ktcursor);
    }
    KrbMgrFreeKeyTab(
        krb5context,
        keytab);

    return dwError;

error:

    goto cleanup;
}

idl_long_int
RpcLWIWriteKeyTabEntry(
    handle_t         bindingHandle,
    idl_char *       pszKeyTabPath,
    LSA_KEYTAB_ENTRY KeyTabEntry
    )
{
    idl_long_int     dwError = 0;

    /* This one would requirexs updating Active Directory.
     * Need to make sure the password is passed through
     * RPC securely and is overwritten as soon as we're
     * done with it.
     */

    return dwError;
}

idl_long_int
RpcLWIDeleteFromKeyTab(
    handle_t         bindingHandle,
    idl_char *       pszKeyTabPath,
    LSA_KEYTAB_ENTRY KeyTabEntry
    )
{
    idl_long_int      dwError = 0;
    krb5_context      krb5context = NULL;
    krb5_keytab       keytab = NULL;
    PSTR              writeName = NULL;
    krb5_keytab_entry ktentry;

    memset(&ktentry, 0, sizeof(ktentry));

    if ( pszKeyTabPath )
    {
        dwError = LWMGMTAllocateStringPrintf(
                      &writeName,
                      "WRFILE:%s",
                      pszKeyTabPath);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    dwError = KrbMgrGetKeyTab(
                  (idl_char *)(pszKeyTabPath ? writeName : "WRFILE:"),
                  &krb5context,
                  &keytab);
    BAIL_ON_LWMGMT_ERROR(dwError);

    ktentry.vno = KeyTabEntry.kvno;
    ktentry.key.enctype = KeyTabEntry.enctype;

    dwError = krb5_parse_name(
                  krb5context,
                  (char *)KeyTabEntry.pszPrincipal,
                  &ktentry.principal);
    BAIL_ON_KRB_ERROR(krb5context, dwError);

    dwError = krb5_kt_remove_entry(
                  krb5context,
                  keytab,
                  &ktentry);
    BAIL_ON_KRB_ERROR(krb5context, dwError);

cleanup:

    LWMGMT_SAFE_FREE_STRING(writeName);

    krb5_free_principal(
        krb5context,
        ktentry.principal);

    KrbMgrFreeKeyTab(
        krb5context,
        keytab);

    return dwError;

error:

    goto cleanup;
}

idl_long_int
RpcLWIClearKeyTab(
    handle_t     bindingHandle,
    idl_char *   pszKeyTabPath
    )
{
    idl_long_int      dwError = 0;
    krb5_context      krb5context = NULL;
    krb5_keytab       keytab = NULL;
    krb5_kt_cursor    ktcursor = NULL;
    krb5_keytab_entry ktentry;
    PSTR              writeName = NULL;

    memset(&ktentry, 0, sizeof(ktentry));

    if ( pszKeyTabPath )
    {
        dwError = LWMGMTAllocateStringPrintf(
                      &writeName,
                      "WRFILE:%s",
                      pszKeyTabPath);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    dwError = KrbMgrGetKeyTab(
                  (idl_char *)(pszKeyTabPath ? writeName : "WRFILE:"),
                  &krb5context,
                  &keytab);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = krb5_kt_start_seq_get(
                  krb5context,
                  keytab,
                  &ktcursor);
    BAIL_ON_KRB_ERROR(krb5context, dwError);

    while( 1 )
    {
        dwError = krb5_kt_next_entry(
                      krb5context,
                      keytab,
                      &ktentry,
                      &ktcursor);
        if ( dwError == KRB5_KT_END ) {
            dwError = 0;
            goto cleanup;
        }
        BAIL_ON_KRB_ERROR(krb5context, dwError);

        dwError = krb5_kt_remove_entry(
                      krb5context,
                      keytab,
                      &ktentry);
        BAIL_ON_KRB_ERROR(krb5context, dwError);

        dwError = krb5_free_keytab_entry_contents(
                      krb5context,
                      &ktentry);
        memset(&ktentry, 0, sizeof(ktentry));
        BAIL_ON_KRB_ERROR(krb5context, dwError);

    }

cleanup:

    LWMGMT_SAFE_FREE_STRING(writeName);

    if ( ktcursor )
    {
        krb5_free_keytab_entry_contents(
            krb5context,
            &ktentry);
        krb5_kt_end_seq_get(
            krb5context,
            keytab,
            &ktcursor);
    }
    KrbMgrFreeKeyTab(
        krb5context,
        keytab);

    return dwError;

error:

    goto cleanup;
}

idl_long_int
KrbMgrGetKeyTab(
    idl_char *     pszKeyTabPath,
    krb5_context * pKrb5context,
    krb5_keytab *  pKeyTab
    )
{
    idl_long_int       dwError = 0;

    dwError = krb5_init_context( pKrb5context );
    BAIL_ON_KRB_ERROR(*pKrb5context, dwError);

    if ( IsNullOrEmptyString(pszKeyTabPath) )
    {
        dwError = krb5_kt_default(
                      *pKrb5context,
                      pKeyTab);
        BAIL_ON_KRB_ERROR(*pKrb5context, dwError);
    }
    else
    {
        dwError = krb5_kt_resolve(
                      *pKrb5context,
                      (char *)pszKeyTabPath,
                      pKeyTab);
        BAIL_ON_KRB_ERROR(*pKrb5context, dwError);
    }

cleanup:

    return dwError;

error:

    KrbMgrFreeKeyTab(
        *pKrb5context,
        *pKeyTab);

    *pKrb5context = NULL;
    *pKeyTab = NULL;

    goto cleanup;
}

void
KrbMgrFreeKeyTab(
    krb5_context krb5context,
    krb5_keytab  keytab
    )
{
    if ( keytab )
    {
        krb5_kt_close(
            krb5context,
            keytab);
    }
    if ( krb5context )
    {
        krb5_free_context(
            krb5context);
    }

    return;
}

DWORD
KrbMgrRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    )
{
    unsigned32 dwError = 0;
    rpc_binding_vector_p_t pServerBinding = NULL;
    BOOLEAN bRegistered = FALSE;
    BOOLEAN bBound = FALSE;
    BOOLEAN bEPRegistered = FALSE;
    TRY
    {
        rpc_server_register_if(
            krbmgr_v1_0_s_ifspec,
            NULL,
            NULL,
            &dwError);
        BAIL_ON_DCE_ERROR(dwError);

        bRegistered = TRUE;
        LWMGMT_LOG_INFO("RPC Service registered successfully.");

        dwError = KrbMgrBindServer(
                      &pServerBinding,
                      krbmgr_v1_0_s_ifspec,
                      (unsigned_char_p_t)"ncacn_ip_tcp",
                      NULL /* endpoint */);
        BAIL_ON_DCE_ERROR(dwError);

        bBound = TRUE;

        rpc_ep_register(
            krbmgr_v1_0_s_ifspec,
            pServerBinding,
            NULL,
            (idl_char*)pszServiceName,
            &dwError);
        BAIL_ON_DCE_ERROR(dwError);

        bEPRegistered = TRUE;
        LWMGMT_LOG_INFO("RPC Endpoint registered successfully.");
    }
    CATCH_ALL
    {
        dwError = dcethread_exc_getstatus (THIS_CATCH);
    	if(!dwError)
    	{
                dwError = LWMGMT_ERROR_RPC_EXCEPTION_UPON_REGISTER;
    	}
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

    *ppServerBinding = pServerBinding;

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to register RPC endpoint.  Error Code: [%u]\n", dwError);

    if (bBound) {
        unsigned32 tmpStatus = 0;
        rpc_ep_unregister(
            krbmgr_v1_0_s_ifspec,
            pServerBinding,
            NULL,
            &tmpStatus);
    }

    if (bEPRegistered) {
        unsigned32 tmpStatus = 0;
        rpc_server_unregister_if(
            krbmgr_v1_0_s_ifspec,
            NULL,
            &tmpStatus);
    }

    *ppServerBinding = NULL;

    goto cleanup;
}

DWORD
KrbMgrBindServer(
    rpc_binding_vector_p_t * server_binding,
    rpc_if_handle_t interface_spec,
    unsigned_char_p_t  protocol,
    unsigned_char_p_t  endpoint
    )
{
    unsigned32 status = 0;

    /*
     * Prepare the server binding handle
     * use all avail protocols (UDP and TCP). This basically allocates
     * new sockets for us and associates the interface UUID and
     * object UUID of with those communications endpoints.
     */

    if (!endpoint)
    {
        rpc_server_use_protseq(
            protocol,
            rpc_c_protseq_max_calls_default,
            &status);
        BAIL_ON_DCE_ERROR(status);
    }
    else
    {
        rpc_server_use_protseq_ep(
            protocol,
            rpc_c_protseq_max_calls_default,
            endpoint,
            &status);
        BAIL_ON_DCE_ERROR(status);
    }

    rpc_server_inq_bindings(
        server_binding,
        &status);
    BAIL_ON_DCE_ERROR(status);

error:

    return status;
}


DWORD
KrbMgrUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    )
{
    unsigned32 dwError = 0;

    TRY
    {      
        LWMGMT_LOG_INFO("Unregistering server from the endpoint mapper...");
        rpc_ep_unregister(
            krbmgr_v1_0_s_ifspec,
            pServerBinding,
            NULL,
            &dwError);
        BAIL_ON_DCE_ERROR(dwError);

        LWMGMT_LOG_INFO("Cleaning up the communications endpoints...");
        rpc_server_unregister_if(
            krbmgr_v1_0_s_ifspec,
            NULL,
            &dwError);
        BAIL_ON_DCE_ERROR(dwError);
    }

    CATCH_ALL
    {
        dwError = dcethread_exc_getstatus (THIS_CATCH);
    	if(!dwError)
    	{
            dwError = LWMGMT_ERROR_RPC_EXCEPTION_UPON_UNREGISTER;
    	}
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to unregister RPC endpoing.  Error code [%d]\n", dwError);
    goto cleanup;
}

DWORD
KrbMgrMapError(
    DWORD dwKrbError
    )
{
    DWORD dwError = 0;

    switch (dwKrbError)
    {
        case KRB5_KT_BADNAME:
            dwError = LWMGMT_ERROR_KRB5_KT_BADNAME;
            break;
        case KRB5_KT_NOTFOUND:
            dwError = LWMGMT_ERROR_KRB5_KT_NOTFOUND;
            break;
        case KRB5_KT_END:
            dwError = LWMGMT_ERROR_KRB5_KT_END;
            break;
        case KRB5_KT_NOWRITE:
            dwError = LWMGMT_ERROR_KRB5_KT_NOWRITE;
            break;
        case KRB5_KT_IOERR:
            dwError = LWMGMT_ERROR_KRB5_KT_IOERR;
            break;
        case KRB5_KT_NAME_TOOLONG:
            dwError = LWMGMT_ERROR_KRB5_KT_NAME_TOOLONG;
            break;
        case KRB5_KT_KVNONOTFOUND:
            dwError = LWMGMT_ERROR_KRB5_KT_KVNONOTFOUND;
            break;
        case KRB5_KT_FORMAT:
            dwError = LWMGMT_ERROR_KRB5_KT_FORMAT;
            break;
        case KRB5_KEYTAB_BADVNO:
            dwError = LWMGMT_ERROR_KRB5_KT_BADVNO;
            break;
        case EACCES:
            dwError = LWMGMT_ERROR_KRB5_KT_EACCES;
            break;
        case ENOENT:
            dwError = LWMGMT_ERROR_KRB5_KT_ENOENT;
            break;
        default :
            dwError = LWMGMT_ERROR_KRB5_CALL_FAILED;
            break;
    }

    return dwError;
}
