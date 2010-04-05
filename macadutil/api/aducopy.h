
#ifndef __ADUCOPY_H__
#define __ADUCOPY_H__

#define BUFF_SIZE 1024
#define MAX_BUFFER 4096

#define BAIL_ON_NULL_POINTER(p)                      \
        if (NULL == p) {                             \
           dwError = MAC_AD_ERROR_INVALID_PARAMETER; \
           goto error;                               \
        }

DWORD
ADULocalOpenFile(
    IN PCSTR pszFileName,
    IN INT dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    );


DWORD
ADULocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );


DWORD
ADULocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    );


DWORD
ADULocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );


DWORD
ADULocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    );


DWORD
ADULocalRemoveDir(
    IN PCSTR pszPath
    );


DWORD
ADULocalRemoveFile(
    IN PCSTR pszPath
    );

DWORD
ADURemoteOpenFile(
    IN  PCSTR           pszFileName,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    );

DWORD
ADURemoteWriteFile(
    IN IO_FILE_HANDLE hFile,
    IN PVOID          pBuffer,
    IN DWORD          dwNumBytesToWrite,
    OUT PDWORD        pdwNumBytesWritten
    );


DWORD
ADURemoteReadFile(
    IN IO_FILE_HANDLE hFile,
    OUT PVOID         pBuffer,
    IN DWORD          dwNumberOfBytesToRead,
    OUT PDWORD        pdwBytesRead
    );

DWORD
ADUCopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCopyMultipleFilesFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCopyFileToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCopyDirToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
ADUCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
ADURemoteRemoveFile(
    const char* pszPath
    );

DWORD
ADURemoteRemoveDirRecursive(
    const char* pszPath
    );
 
DWORD
ADURemoteCreateDirectory(
    const char* pszPath
    );

#endif
