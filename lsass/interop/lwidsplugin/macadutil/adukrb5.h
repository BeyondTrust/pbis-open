#ifndef __ADUKRB5_H__
#define __ADUKRB5_H__

DWORD
ADUInitKrb5(
    PCSTR pszDomainName
    );

DWORD
ADUKerb5DestroyCache(
    PSTR pszCachePath
    );

DWORD
ADUKrb5GetSystemCachePath(
    PSTR* ppszCachePath
    );

DWORD
ADUKrb5GetUserCachePathAndSID(
    PCSTR pszUserUPN,
    PSTR* ppszCachePath,
    PSTR* ppszSID,
    uid_t* pUid
    );

DWORD
ADUKrb5SetDefaultCachePath(
    PSTR  pszCachePath,
    PSTR* ppszOrigCachePath
    );

DWORD
ADUKrb5GetDefaultCachePath(
    PSTR* ppszPath
    );

DWORD
ADUKrb5GetPrincipalName(
    PCSTR pszCachePath,
    PSTR* ppszPrincipalName
    );

#endif /* __ADUKRB5_H__ */

