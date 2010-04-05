#ifndef __GPCACHE_H__
#define __GPCACHE_H__

DWORD
GetCachedPolicyFiles(
    DWORD    dwPolicyType,
    PCSTR    pszgGpSysVolPath,
    PCSTR    pszgCseIdentifier,
    PCSTR    pszDestFolderRootPath,
    PSTR *   ppszDestFolder,
    PBOOLEAN pbPolicyExists
    );

DWORD
IsCacheDataCurrentForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    PDWORD   pdwVersion,
    PDWORD   pdwFileVersion,
    PBOOLEAN pbCurrent
    );

#endif /* __GPCACHE_H__ */
