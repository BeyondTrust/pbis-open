#ifndef _LSA_CFG_H_
#define _LSA_CFG_H_


typedef struct samr_srv_config {
    PSTR pszLpcSocketPath;
    PSTR pszSamrLpcSocketPath;

    BOOLEAN bRegisterTcpIp;
} LSA_SRV_CONFIG, *PLSA_SRV_CONFIG;


DWORD
LsaSrvInitialiseConfig(
    PLSA_SRV_CONFIG pConfig
    );


VOID
LsaSrvFreeConfigContents(
    PLSA_SRV_CONFIG pConfig
    );


DWORD
LsaSrvReadRegistry(
    PLSA_SRV_CONFIG pConfig
    );


DWORD
LsaSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
LsaSrvConfigGetSamrLpcSocketPath(
    PSTR *ppszSamrLpcSocketPath
    );

DWORD
LsaSrvConfigShouldRegisterTcpIp(
    BOOLEAN* pbResult
    );

#endif /* _LSA_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
