/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Management Services
 * 
 * Client Private Header
 *
 */
#ifndef __CLIENT_P_H__
#define __CLIENT_P_H__

DWORD
LWMGMTBuildDomainInfoArray(
    DWORD                     dwNumTrustedDomains,
    LWMGMTLSADOMAININFO*      pSrcDomainInfoArray,
    PLSA_TRUSTED_DOMAIN_INFO* ppDomainInfoArray
    );

DWORD
LWMGMTBuildDCInfo(
    LWMGMTLSADCINFO* pSrcDCInfo,
    PLSA_DC_INFO*    ppDCInfo
    );

VOID
LWMGMTFreeDomainInfoArray(
    DWORD dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray
    );

VOID
LWMGMTFreeDCInfo(
    PLSA_DC_INFO pDCInfo
    );

VOID
LWMGMTRpcFreeDomainInfoArray(
    DWORD                dwNumDomains,
    LWMGMTLSADOMAININFO* pDomainInfoArray
    );

VOID
LWMGMTRpcFreeDCInfo(
    LWMGMTLSADCINFO* pDCInfo
    );
DWORD
LWMGMTGetRpcError(
    dcethread_exc* exCatch,
    DWORD          dwEVTError
    );

VOID
LWMGMTFreeRpcLsaStatus(
    LWMGMTLSASTATUS* pLsaStatus
    );

VOID
LWMGMTFreeRpcKeyTabEntries(
    LSA_KEYTAB_ENTRIES * pKeyTabEntries
    );

#endif /* __CLIENT_P_H__ */

