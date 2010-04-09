#ifndef _WKSS_CFG_H_
#define _WKSS_CFG_H_


typedef struct _WKSS_SRV_CONFIG
{
    PSTR pszLpcSocketPath;
    PSTR pszLsaLpcSocketPath;

} WKSS_SRV_CONFIG, *PWKSS_SRV_CONFIG;


DWORD
WkssSrvInitialiseConfig(
    PWKSS_SRV_CONFIG pConfig
    );


VOID
WkssSrvFreeConfigContents(
    PWKSS_SRV_CONFIG pConfig
    );


DWORD
WkssSrvReadRegistry(
    PWKSS_SRV_CONFIG pConfig
    );


DWORD
WkssSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
WkssSrvConfigGetLsaLpcSocketPath(
    PSTR *ppszSamrLpcSocketPath
    );


#endif /* _WKSS_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
