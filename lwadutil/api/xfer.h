#ifndef __GPAXFER_H__
#define __GPAXFER_H__

#define LWDS_ADMIN_CACHE_DIR "/var/lib/likewise/lwadutil"
#define LWDS_GPO_CACHE_DIR   "/var/lib/likewise/grouppolicy"
#define LWDSKRB5CC           "/var/lib/likewise/lwadutil/krb5cc_lwadutil"


DWORD
ADUGetPolicyFiles(
    DWORD   dwPolicyType,
    PSTR    pszgGpSysVolPath,
    PSTR    pszgCseIdentifier,
    PSTR    pszDestFolderRootPath,
    PSTR    *ppszDestFolder,
    BOOLEAN *pbPolicyExists
    );

DWORD
ADUPutPolicyFiles(
    PSTR    pszSourceFolderRootPath,
    BOOLEAN fReplace,
    DWORD   dwPolicyType,
    PSTR    pszgGpSysVolPath,
    PSTR    pszgCseIdentifier
    );

#endif /* __GPAXFER_H__ */
