/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Kerberos Keytab Management API
 *
 */
#ifndef __KRBMGRSERVER_H__
#define __KRBMGRSERVER_H__

#define BAIL_ON_KRB_ERROR(ctx, ret)                                     \
    if (ret) {                                                          \
        if (ctx)  {                                                     \
            PCSTR pszKrb5Error = krb5_get_error_message(ctx, ret);      \
            if (pszKrb5Error) {                                         \
                LWMGMT_LOG_ERROR("KRB5 Error: %s", pszKrb5Error);       \
                krb5_free_error_message(ctx, pszKrb5Error);             \
            }                                                           \
        } else {                                                        \
            LWMGMT_LOG_ERROR("KRB5 Error [Code:%d]", ret);              \
        }                                                               \
        ret = KrbMgrMapError(ret);                                      \
        goto error;                                                     \
    }

DWORD
KrbMgrRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    );

idl_long_int
KrbMgrGetKeyTab(
    idl_char *     pszKeyTabPath,
    krb5_context * pKrb5context,
    krb5_keytab *  pKeyTab
    );

void
KrbMgrFreeKeyTab(
    krb5_context krb5context,
    krb5_keytab  keytab
    );

DWORD
KrbMgrBindServer(
    rpc_binding_vector_p_t * server_binding,
    rpc_if_handle_t interface_spec,
    unsigned_char_p_t  protocol,
    unsigned_char_p_t  endpoint
    );

DWORD
KrbMgrUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    );

DWORD
KrbMgrMapError(
    DWORD dwKrbError
    );

#endif /* __KRBMGRSERVER_H__ */
