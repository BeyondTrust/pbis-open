#ifndef _DSSETUP_CFG_H_
#define _DSSETUP_CFG_H_


typedef struct dssetup_srv_config {
    PSTR pszLpcSocketPath;
    PSTR pszLsaLpcSocketPath;
    BOOLEAN bRegisterTcpIp;
} DSSETUP_SRV_CONFIG, *PDSSETUP_SRV_CONFIG;


DWORD
DsrSrvInitialiseConfig(
    PDSSETUP_SRV_CONFIG pConfig
    );


DWORD
DsrSrvReadRegistry(
    PDSSETUP_SRV_CONFIG pConfig
    );


DWORD
DsrSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
DsrSrvConfigGetLsaLpcSocketPath(
    PSTR *ppszSamrLpcSocketPath
    );

DWORD
DsrSrvConfigShouldRegisterTcpIp(
    BOOLEAN* pbResult
    );

#endif /* _DSSETUP_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
