#ifndef __LSAMGR_P_H__
#define __LSAMGR_P_H__

DWORD
LWMGMTOpenLsaServer(
    PCSTR     pszServerName,
    char**    ppszBindingString,
    handle_t* phLsaServer
    );

DWORD
LWMGMTGetLsaMetrics(
    handle_t       hLsaServer,
    idl_usmall_int infoLevel,
    LSAMETRICPACK* pMetricPack
    );

DWORD
LWMGMTGetLsaStatus(
    handle_t         hLsaServer,
    LWMGMTLSASTATUS* pLsaStatus
    );

DWORD
LWMGMTCloseLsaServer(
    handle_t hLsaServer,
    char*    pszBindingString
    );

#endif

