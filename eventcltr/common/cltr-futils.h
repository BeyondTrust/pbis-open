#ifndef __EVTFUTILS_H__
#define __EVTFUTILS_H__

#ifndef WIN32
FILE *
_wfopen(
    PCWSTR pwszPath,
    PCWSTR pwszMode
    );
#endif

DWORD
CltrRemoveFile(
    PWSTR pszPath
    );

DWORD
CltrCheckFileExists(
    PWSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
CltrCheckDirectoryExists(
    PWSTR pszPath,
    PBOOLEAN pbDirExists
    );


DWORD
CltrCreateDirectory(
    PCWSTR pszPath
    );

#endif
