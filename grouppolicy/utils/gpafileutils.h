/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

#ifndef __GPA_FILEUTILS_H__
#define __GPA_FILEUTILS_H__

CENTERROR
GPARemoveFiles(
    PSTR pszPath,
    BOOLEAN fDirectory,
    BOOLEAN fRecursive
    );


CENTERROR
GPACheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

CENTERROR
GPACopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );
    
CENTERROR
GPACopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

CENTERROR
GPAGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

CENTERROR
GPAFilePrintf(
    FILE* handle,
    PCSTR format,
    ...
    );

CENTERROR
GPARemoveDirectory(
    PCSTR pszPath
    );

CENTERROR
GPACheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

CENTERROR
GPAGetOwnerUID(
    PCSTR pszFilePath,
    uid_t* pUid
    );

CENTERROR
GPAMoveFileAcrossDevices(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

CENTERROR
GPAChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );


CENTERROR
GPAOpenFile(
    PCSTR path,
    PCSTR mode, 
    FILE** handle
    );

CENTERROR
GPACloseFile(
    FILE* handle
    );
#endif /* __GPA_FILEUTILS_H__ */
