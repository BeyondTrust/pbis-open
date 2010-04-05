
#ifndef __GPA_UTILS_H__
#define __GPA_UTILS_H__

#include "includes.h"

BOOLEAN
GPAStrEndsWith(
    PCSTR str,
    PCSTR suffix
    );

BOOLEAN
GPAStrStartsWith(
    PCSTR str,
    PCSTR prefix
    );

CENTERROR
GPACheckLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbLinkExists
    );

CENTERROR
GPACheckFileOrLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbExists
    );

CENTERROR
GPAFindFileInPath(
    PCSTR filename,
    PCSTR searchPath,
    PSTR* foundPath
    );

CENTERROR
GPAGetSymLinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

CENTERROR
GPACreateSymLink(
    PCSTR pszOldPath,
    PCSTR pszNewPath
    );

VOID
GPAFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

CENTERROR
GPAGetMatchingDirPathsInFolder(PCSTR pszDirPath,
                              PCSTR pszDirNameRegExp,
                              PSTR** pppszHostDirPaths,
                              PDWORD pdwNPaths
                              );

CENTERROR
GPAGetFileTimeStamps(
    PCSTR pszFilePath,
    time_t *patime,   /* time of last access */
    time_t *pmtime,   /* time of last modification */
    time_t *pctime /* time of last status change */
    );
  
CENTERROR
GPAReadFile(
	PCSTR pszFilePath,
	PSTR *ppBuffer,
	PLONG pSize
    );

CENTERROR
GPACheckFileHoldsPattern(
    PCSTR pszFilePath,
    PCSTR pszPattern,
    PBOOLEAN pbPatternExists
    );

CENTERROR
GPAFileStreamWrite(
    FILE* handle,
    PCSTR data,
    unsigned int size
    );

CENTERROR
GPAGetUserUID(
    PCSTR pszUID,
    uid_t* pUID
    );

CENTERROR
GPAGetUserGID(
    PCSTR pszGID,
    gid_t* pGID
    );

CENTERROR
GPASetCloseOnExec(
    int fd
    );

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
GPACheckIfMountDirExists(
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
GPAOpenFile(
    PCSTR path,
    PCSTR mode, 
    FILE** handle
    );

CENTERROR
GPACloseFile(
    FILE* handle
    );

CENTERROR
GPACheckSockExists(
    PCSTR pszPath,
    PBOOLEAN pbSockExists
    );

VOID
GPARemoveLeadingWhitespacesOnly(
    PSTR pszString
    );

VOID
GPARemoveTrailingWhitespacesOnly(
    PSTR pszString
    );

#endif
