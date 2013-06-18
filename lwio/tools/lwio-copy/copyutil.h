
#ifndef __COPYUTIL_H__
#define __COPYUTIL_H__

BOOLEAN
IsPathRemote(
    IN PCSTR pszPath
    );

NTSTATUS
LwioCheckRemotePathIsDirectory(
    IN     PCSTR    pszPath,
    IN OUT PBOOLEAN pbIsDirectory
    );

NTSTATUS
LwioLocalOpenFile(
    IN PCSTR pszFileName,
    IN INT  dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    );

NTSTATUS
LwioLocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );

NTSTATUS
LwioLocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    );

NTSTATUS
LwioLocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );

NTSTATUS
LwioLocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    );

NTSTATUS
LwioLocalRemoveDir(
    IN PCSTR pszPath
    );

NTSTATUS
LwioLocalRemoveFile(
    IN PCSTR pszPath
    );

NTSTATUS
LwioCheckLocalFileExists(
    IN  PCSTR pszPath,
    OUT PBOOLEAN pbFileExists
    );

NTSTATUS
LwioCheckLocalDirectoryExists(
    IN  PCSTR pszPath,
    OUT PBOOLEAN pbDirExists
    );

NTSTATUS
LwioGetLocalFileOwnerAndPerms(
    IN  PCSTR pszSrcPath,
    OUT uid_t * uid,
    OUT gid_t * gid,
    OUT mode_t * mode
    );

NTSTATUS
LwioChangeLocalFileOwnerAndPerms(
    IN PCSTR pszPath,
    IN uid_t uid,
    IN gid_t gid,
    IN mode_t dwFileMode
    );

NTSTATUS
LwioChangeLocalFileOwner(
    IN PCSTR pszPath,
    IN uid_t uid,
    IN gid_t gid
    );

NTSTATUS
LwioChangeLocalFilePerms(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );

NTSTATUS
LwioRemoteOpenFile(
    IN  PCSTR           pszFileName,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    );

NTSTATUS
LwioRemoteReadFile(
    IN HANDLE hFile,
    OUT PVOID pBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT PDWORD pdwBytesRead
    );

NTSTATUS
LwioRemoteWriteFile(
    IN HANDLE hFile,
    IN PVOID pBuffer,
    IN DWORD dwNumBytesToWrite,
    OUT PDWORD pdwNumBytesWritten
    );

#endif
