
#ifndef __LWIOCOPY_H__
#define __LWIOCOPY_H__

NTSTATUS
CopyFile(
    IN PCSTR pSrc,
    IN PCSTR pDest,
    BOOLEAN  bCopyRecursive
    );

NTSTATUS
ResolveFile(
    PCSTR pszPath
    );

NTSTATUS
LwioCopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCopyFileToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCopyDirToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
CopyFile_RemoteToRemote(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    );

NTSTATUS
CopyFile_RemoteToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    );

NTSTATUS
CopyFile_LocalToRemote(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    );

NTSTATUS
CopyFile_LocalToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    );

NTSTATUS
LwioCopyFileFromLocalToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath
    );

NTSTATUS
LwioCopyDirFromLocalToLocal(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCopyDirFromRemoteToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCopyFileFromRemoteToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

#endif
