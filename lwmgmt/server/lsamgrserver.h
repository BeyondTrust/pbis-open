/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * LSA Management API
 *
 */
#ifndef __LSAMGRSERVER_H__
#define __LSAMGRSERVER_H__

DWORD
LsaServerBuildDomainInfoArray(
    DWORD                    dwNumTrustedDomains,
    PLSA_TRUSTED_DOMAIN_INFO pSrcDomainInfoArray,
    LWMGMTLSADOMAININFO**    ppDestDomainInfoArray
    );

DWORD
LsaServerBuildDCInfo(
    PLSA_DC_INFO      pSrcDCInfo,
    LWMGMTLSADCINFO** ppDCInfo
    );

VOID
LsaServerFreeDomainInfoArray(
    DWORD                dwNumDomains,
    LWMGMTLSADOMAININFO* pDomainInfoArray
    );

VOID
LsaServerFreeDCInfo(
    LWMGMTLSADCINFO* pDCInfo
    );

DWORD
LsaServerRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    );

DWORD
LsaServerBind(
    rpc_binding_vector_p_t * server_binding,
    rpc_if_handle_t interface_spec,
    unsigned_char_p_t  protocol,
    unsigned_char_p_t  endpoint
    );

DWORD
LsaServerUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    );

#endif /* __LSAMGRSERVER_H__ */
