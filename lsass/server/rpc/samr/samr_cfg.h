#ifndef _SAMR_CFG_H_
#define _SAMR_CFG_H_


typedef struct samr_srv_config
{
    PSTR pszLpcSocketPath;
    PSTR pszDefaultLoginShell;
    PSTR pszHomedirPrefix;
    PSTR pszHomedirTemplate;
    BOOLEAN bRegisterTcpIp;
} SAMR_SRV_CONFIG, *PSAMR_SRV_CONFIG;


DWORD
SamrSrvInitialiseConfig(
    PSAMR_SRV_CONFIG pConfig
    );


VOID
SamrSrvFreeConfigContents(
    PSAMR_SRV_CONFIG pConfig
    );


DWORD
SamrSrvReadRegistry(
    PSAMR_SRV_CONFIG pConfig
    );


DWORD
SamrSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
SamrSrvConfigGetDefaultLoginShell(
    PSTR *ppszDefaultLoginShell
    );


DWORD
SamrSrvConfigGetHomedirPrefix(
    PSTR *ppszLpcSocketPath
    );


DWORD
SamrSrvConfigGetHomedirTemplate(
    PSTR *ppszHomedirTemplate
    );

DWORD
SamrSrvConfigShouldRegisterTcpIp(
    BOOLEAN* pbResult
    );

#endif /* _SAMR_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
