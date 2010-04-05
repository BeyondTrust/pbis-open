#ifndef __GPAXFER_H__
#define __GPAXFER_H__

CENTERROR
GPOLwioCopyFile(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszPreferredServerIP, 
    PSTR pszSourcePath,
    PSTR pszDestPath
    );

CENTERROR
GPOLwioCopyFileMultiple(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszPreferredServerIP, /* Optional */
    PSTR pszSourceFolder,
    PSTR pszDestFolder
    );

CENTERROR
GPOLwioCopyFolder(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszPreferredServerIP, 
    PSTR pszSourceFolder,
    PSTR pszDestFolder
    );

#endif
