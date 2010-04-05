
#ifndef __GPALWIOCOPY_H__
#define __GPALWIOCOPY_H__

#define BUFF_SIZE 1024
#define MAX_BUFFER 4096

#ifdef BAIL_ON_NT_STATUS
#undef BAIL_ON_NT_STATUS
#endif

#define BAIL_ON_NT_STATUS(status) \
    if ((status) != STATUS_SUCCESS) goto error;

CENTERROR
GPALocalOpenFile(
    IN PCSTR pszFileName,
    IN INT dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    );


CENTERROR
GPALocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );


CENTERROR
GPALocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    );


CENTERROR
GPALocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );


CENTERROR
GPALocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    );


CENTERROR
GPALocalRemoveDir(
    IN PCSTR pszPath
    );


CENTERROR
GPALocalRemoveFile(
    IN PCSTR pszPath
    );

CENTERROR
GPARemoteOpenFile(
    IN  PCSTR           pszFileName,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    );

CENTERROR
GPARemoteWriteFile(
    IN  IO_FILE_HANDLE hFile,
    IN  PVOID          pBuffer,
    IN  DWORD          dwNumBytesToWrite,
    OUT PDWORD         pdwNumBytesWritten
    );

CENTERROR
GPARemoteReadFile(
    IN  IO_FILE_HANDLE hFile,
    OUT PVOID          pBuffer,
    IN  DWORD          dwNumberOfBytesToRead,
    OUT PDWORD         pdwBytesRead
    );

CENTERROR
GPACopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

CENTERROR
GPACopyMultipleFilesFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

CENTERROR
GPACopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

CENTERROR
GPACheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

CENTERROR
GPACheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

#endif
