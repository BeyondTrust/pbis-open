/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#ifndef __CTFILEUTILS_H__
#define __CTFILEUTILS_H__

DWORD
LWRemoveFile(
    PCSTR pszPath
    );

DWORD
LWRemoveDirectory(
    PCSTR pszPath
    );

DWORD
LWCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LWCheckLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbLinkExists
    );

DWORD
LWCheckFileOrLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbExists
    );

DWORD
LWGetSymLinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

DWORD
LWCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LWCheckSockExists(
    PCSTR pszPath,
    PBOOLEAN pbSockExists
    );

DWORD
LWCreateSymLink(
    PCSTR pszOldPath,
    PCSTR pszNewPath
    );

DWORD
LWMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LWMoveFileEx(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    BOOLEAN bCrossDevice
    );

DWORD
LWCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );
    
DWORD
LWGetOwnerUID(
    PCSTR pszFilePath,
    uid_t* pUid
    );

DWORD
LWChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LWGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
LWChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
LWChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
LWCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

/* Safely creates a temporary directory that is only accessible by the current
 * user and root.
 */
DWORD
LWCreateTempDirectory(
    PSTR *pszPath
    );

DWORD
LWGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    );

DWORD
LWGetMatchingDirPathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszDirNameRegExp,
    PSTR** pppszHostDirPaths,
    PDWORD pdwNPaths
    );

DWORD
LWCheckFileHoldsPattern(
    PCSTR pszFilePath,
    PCSTR pszPattern,
    PBOOLEAN pbPatternExists
    );

DWORD
LWGetAbsolutePath(
    PSTR pszRelativePath,
    PSTR* ppszAbsolutePath
    );

DWORD
LWRemoveFiles(
    PSTR pszPath,
    BOOLEAN fDirectory,
    BOOLEAN fRecursive
    );
	
DWORD
LWOpenFile(
    PCSTR path,
    PCSTR mode, 
    FILE** handle);

DWORD
LWSetCloseOnExec(
    int fd);

DWORD
LWFileStreamWrite(
    FILE* handle,
    PCSTR data,
    unsigned int size);

DWORD
LWFilePrintf(
    FILE* handle,
    PCSTR format,
    ...
    );

//Use LWSafeCloseFile instead
DWORD
LWCloseFile(
    FILE* handle);

/* This function calls fclose on the handle and returns a DWORD if
 * something goes wrong.
 *
 * One way this function could fail is if an IO error occurs while the file
 * buffers are being flushed, in which case the previous writes the file would
 * be incomplete.
 *
 * Even if fclose fails, the man fclose man page says that the handle is
 * invalid afterwards. This means that is an error to ever attempt to call
 * fclose on the same handle twice. That could result in a memory double free,
 * or segfault.
 *
 * To prevent the user from calling close twice, this function sets the handle
 * to NULL whether or not the close is successful.
 *
 * If this function is called with *handle == NULL, it will simply return
 * success.
 */
DWORD
LWSafeCloseFile(
    FILE** handle);

DWORD
LWMoveFileAcrossDevices(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LWGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
LWReadNextLine(
    FILE* fp,
    PSTR *output,
    PBOOLEAN pbEndOfFile
    );

/* Reads a file and separates into individual lines
 * The dynamic array must be initialized (at least zeroed out) beforehand
 */
DWORD
LWReadLines(FILE *file, DynamicArray *dest);

DWORD
LWWriteLines(FILE *file, const DynamicArray *lines);

VOID
LWFreeLines(DynamicArray *lines);

#endif /* __CTFILEUTILS_H__ */
