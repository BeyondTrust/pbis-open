/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwiocopy.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Tool to copy files/directories
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

NTSTATUS
ResolveFile(
    PCSTR pszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pPhysicalPath = NULL;
    IO_FILE_HANDLE pHandle = NULL;
    size_t dummy = 0;

    status = LwioRemoteOpenFile(
        pszPath,
        FILE_READ_ATTRIBUTES, /* Desired access mask */
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,     /* Share access */
        FILE_OPEN,           /* Create disposition */
        0, /* Create options */
        &pHandle);

    status = LwIoRdrGetPhysicalPath(pHandle, &pPhysicalPath);
    BAIL_ON_NT_STATUS(status);

    LwPrintfStdout(&dummy, "%ws\n", pPhysicalPath);

error:

    LwIoRdrFreePhysicalPath(pPhysicalPath);

    if (pHandle)
    {
        LwNtCloseFile(pHandle);
    }

    return status;
}

NTSTATUS
CopyFile(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (IsPathRemote(pszSrcPath))
    {
        if (IsPathRemote(pszDestPath))
        {
            // Remote to Remote
            ntStatus = CopyFile_RemoteToRemote(
                            pszSrcPath,
                            pszDestPath,
                            bCopyRecursive);
        }
        else
        {
            // Remote to Local
            ntStatus = CopyFile_RemoteToLocal(
                            pszSrcPath,
                            pszDestPath,
                            bCopyRecursive);
        }
    }
    else
    {
        if (IsPathRemote(pszDestPath))
        {
            // Local to Remote
            ntStatus = CopyFile_LocalToRemote(
                            pszSrcPath,
                            pszDestPath,
                            bCopyRecursive);
        }
        else
        {
            // Local to Local
            ntStatus = CopyFile_LocalToLocal(
                            pszSrcPath,
                            pszDestPath,
                            bCopyRecursive);
        }
    }

    return ntStatus;
}


NTSTATUS
CopyFile_RemoteToRemote(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bIsDirectory = FALSE;

    ntStatus = LwioCheckRemotePathIsDirectory(pszSrcPath, &bIsDirectory);
    BAIL_ON_NT_STATUS(ntStatus);

    if (bIsDirectory)
    {
        ntStatus = LwioCopyDirFromRemoteToRemote(pszSrcPath, pszDestPath);
    }
    else
    {
        ntStatus = LwioCopyFileFromRemoteToRemote(pszSrcPath, pszDestPath);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
CopyFile_RemoteToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bIsDirectory = FALSE;

    ntStatus = LwioCheckRemotePathIsDirectory(pszSrcPath, &bIsDirectory);
    BAIL_ON_NT_STATUS(ntStatus);

    if (bIsDirectory)
    {
        ntStatus = LwioCopyDirFromRemote(pszSrcPath, pszDestPath);
    }
    else
    {
        ntStatus = LwioCopyFileFromRemote(pszSrcPath, pszDestPath);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
CopyFile_LocalToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bExists = FALSE;

    BAIL_ON_NULL_POINTER(pszSrcPath);
    BAIL_ON_NULL_POINTER(pszDestPath);

    status = LwioCheckLocalDirectoryExists(pszSrcPath, &bExists);
    BAIL_ON_NT_STATUS(status);

    if (bExists)
    {
        status = LwioCopyDirFromLocalToLocal(pszSrcPath, pszDestPath);
        BAIL_ON_NT_STATUS(status);

        goto done;
    }

    status = LwioCheckLocalFileExists(pszSrcPath, &bExists);
    BAIL_ON_NT_STATUS(status);

    if (bExists)
    {
        status = LwioCopyFileFromLocalToLocal(pszSrcPath, pszDestPath);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = STATUS_NO_SUCH_FILE;
        BAIL_ON_NT_STATUS(status);
    }

done:
cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
CopyFile_LocalToRemote(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bExists = FALSE;

    ntStatus = LwioCheckLocalDirectoryExists(pszSrcPath, &bExists);
    BAIL_ON_NT_STATUS(ntStatus);

    if (bExists)
    {
        ntStatus = LwioCopyDirToRemote(pszSrcPath, pszDestPath);
        BAIL_ON_NT_STATUS(ntStatus);

        goto done;
    }

    ntStatus = LwioCheckLocalFileExists(pszSrcPath, &bExists);
    BAIL_ON_NT_STATUS(ntStatus);

    if (bExists)
    {
        ntStatus = LwioCopyFileToRemote(pszSrcPath, pszDestPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = STATUS_NO_SUCH_FILE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

done:
cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
LwioCopyFileFromLocalToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CHAR szBuf[BUFF_SIZE];
    int hSrcFile = -1;
    int hDestFile = -1;
    DWORD dwBytesRead = 0;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    BAIL_ON_NULL_POINTER(pszSrcPath);
    BAIL_ON_NULL_POINTER(pszDestPath);

    status = LwioLocalOpenFile(
                (PCSTR)pszSrcPath,
                O_RDONLY,
                0,
                &hSrcFile);
    BAIL_ON_NT_STATUS(status);

    //Get UID GID and mode
    status = LwioGetLocalFileOwnerAndPerms( 
                 pszSrcPath,
                 &uid,
                 &gid,
                 &mode);
    BAIL_ON_NT_STATUS(status);

    status = LwioLocalOpenFile(
                (PCSTR)pszDestPath,
                O_WRONLY|O_TRUNC|O_CREAT,
                0666,
                &hDestFile);
    BAIL_ON_NT_STATUS(status);


    do
    {
        DWORD dwWrote = 0;

        memset (szBuf,0,BUFF_SIZE);

        if ((dwBytesRead = read(hSrcFile, szBuf, sizeof(szBuf))) == -1)
        {
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        if (dwBytesRead == 0)
        {
            break;
        }

        if ((dwWrote = write(hDestFile, szBuf, dwBytesRead)) == -1)
        {
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

   } while(1);

   status = LwioChangeLocalFileOwnerAndPerms( 
                 pszDestPath,
                 uid,
                 gid,
                 mode);
   BAIL_ON_NT_STATUS(status);

cleanup:

    if (hSrcFile >= 0)
    {
        close(hSrcFile);
    }
    if (hDestFile >= 0)
    {
        close(hDestFile);
    }

    return (status);

error:

    goto cleanup;
}

NTSTATUS
LwioCopyFileFromRemoteToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_HANDLE hRemSrcFile = NULL;
    IO_FILE_HANDLE hRemDstFile = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    status = LwioRemoteOpenFile(
                    pszSourcePath,
                    FILE_READ_DATA,          /* Desired access mask */
                    FILE_SHARE_READ,         /* Share access */
                    FILE_OPEN,               /* Create disposition */
                    FILE_NON_DIRECTORY_FILE, /* Create options */
                    &hRemSrcFile);
    BAIL_ON_NT_STATUS(status);

    status = LwioRemoteOpenFile(
                    pszTargetPath,
                    FILE_WRITE_DATA,
                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE,
                    &hRemDstFile);
    BAIL_ON_NT_STATUS(status);

    do
    {
        BYTE  szBuff[BUFF_SIZE];
        DWORD dwRead = 0;
        DWORD dwWrote = 0;

        status = LwioRemoteReadFile(
                        hRemSrcFile,
                        szBuff,
                        sizeof(szBuff),
                        &dwRead);
        BAIL_ON_NT_STATUS(status);

        if (!dwRead)
        {
            break;
        }

        status  = LwioRemoteWriteFile(
                            hRemDstFile,
                            szBuff,
                            dwRead,
                            &dwWrote);

        BAIL_ON_NT_STATUS(status);

    } while(1);


cleanup:

    if (hRemSrcFile)
    {
        LwNtCloseFile(hRemSrcFile);
    }

    if (hRemDstFile)
    {
        LwNtCloseFile(hRemDstFile);
    }

    return (status);

error:

    goto cleanup;

}

NTSTATUS
LwioCopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_HANDLE hRemoteFile = NULL;
    int hLocalFile = -1;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    status = LwioRemoteOpenFile(
                    pszSourcePath,
                    FILE_READ_DATA,          /* Desired access mask */
                    FILE_SHARE_READ,         /* Share access */
                    FILE_OPEN,               /* Create disposition */
                    FILE_NON_DIRECTORY_FILE, /* Create options */
                    &hRemoteFile);
    BAIL_ON_NT_STATUS(status);

    status = LwioLocalOpenFile(
                (PCSTR)pszTargetPath,
                O_WRONLY|O_TRUNC|O_CREAT,
                0666,
                &hLocalFile);
    BAIL_ON_NT_STATUS(status);

    do
    {
        BYTE  szBuff[BUFF_SIZE];
        DWORD dwRead = 0;
        DWORD dwWrote = 0;

        status = LwioRemoteReadFile(
                        hRemoteFile,
                        szBuff,
                        sizeof(szBuff),
                        &dwRead);
        BAIL_ON_NT_STATUS(status);

        if (!dwRead)
        {
            break;
        }

        if ((dwWrote = write(hLocalFile, szBuff, dwRead)) == -1)
        {
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

    } while(1);


cleanup:

    if (hRemoteFile)
    {
        LwNtCloseFile(hRemoteFile);
    }

    if (hLocalFile >= 0)
    {
        close(hLocalFile);
    }

    return (status);

error:

    goto cleanup;

}

NTSTATUS
LwioCopyDirFromRemoteToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL bRestart = TRUE;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE hRemSrcFile = NULL;
    IO_FILE_HANDLE hRemDstFile = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszEntryFilename = NULL;
    BYTE buffer[MAX_BUFFER];
    PFILE_BOTH_DIR_INFORMATION pInfo = NULL;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    status = LwioRemoteOpenFile(
                           pszSourcePath,
                           FILE_LIST_DIRECTORY, /* Desired access mask */
                           FILE_SHARE_READ,     /* Share access */
                           FILE_OPEN,           /* Create disposition */
                           FILE_DIRECTORY_FILE, /* Create options */
                           &hRemSrcFile);
    BAIL_ON_NT_STATUS(status);

    status = LwioRemoteOpenFile(
                    pszTargetPath,
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ |FILE_SHARE_WRITE |FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_DIRECTORY_FILE,
                    &hRemDstFile);
    BAIL_ON_NT_STATUS(status);

    for (;;)
    {
        status = LwNtQueryDirectoryFile(
            hRemSrcFile,                        /* File handle */
            NULL,                               /* Async control block */
            &ioStatus,                          /* IO status block */
            buffer,                             /* Info structure */
            sizeof(buffer),                     /* Info structure size */
            FileBothDirectoryInformation,       /* Info level */
            FALSE,                              /* Do not return single entry */
            NULL,                               /* File spec */
            bRestart);                          /* Restart scan */

        switch (status)
        {
        case STATUS_NO_MORE_MATCHES:
            status = STATUS_SUCCESS;
            goto cleanup;
        default:
            BAIL_ON_NT_STATUS(status);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            RTL_FREE(&pszEntryFilename);
            RTL_FREE(&pszRemotePath);
            RTL_FREE(&pszLocalPath);

            status = LwRtlCStringAllocateFromWC16String(
                        &pszEntryFilename,
                        pInfo->FileName
                        );
            BAIL_ON_NT_STATUS(status);

            if (!strcmp(pszEntryFilename, "..") ||
                !strcmp(pszEntryFilename, "."))
                continue;

            status = LwRtlCStringAllocatePrintf(
                        &pszRemotePath,
                        "%s/%s",
                        pszSourcePath,
                        pszEntryFilename);
            BAIL_ON_NT_STATUS(status);

            status = LwRtlCStringAllocatePrintf(
                        &pszLocalPath,
                        "%s/%s",
                        pszTargetPath,
                        pszEntryFilename);
            BAIL_ON_NT_STATUS(status);

            if(pInfo->FileAttributes == FILE_ATTRIBUTE_DIRECTORY)
            {

                status = LwioCopyDirFromRemoteToRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_NT_STATUS(status);
            }
            else
            {
                status = LwioCopyFileFromRemoteToRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_NT_STATUS(status);
          }
        }
    }

cleanup:

    if (hRemSrcFile)
    {
        LwNtCloseFile(hRemSrcFile);
    }

    if (hRemDstFile)
    {
        LwNtCloseFile(hRemDstFile);
    }

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    RTL_FREE(&pszEntryFilename);
    RTL_UNICODE_STRING_FREE(&filename.Name);

    return status;

error:

    goto cleanup;
}

NTSTATUS
LwioCopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL bRestart = TRUE;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszEntryFilename = NULL;
    BYTE buffer[MAX_BUFFER];
    PFILE_BOTH_DIR_INFORMATION pInfo = NULL;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    status = LwioRemoteOpenFile(
                           pszSourcePath,
                           FILE_LIST_DIRECTORY, /* Desired access mask */
                           FILE_SHARE_READ,     /* Share access */
                           FILE_OPEN,           /* Create disposition */
                           FILE_DIRECTORY_FILE, /* Create options */
                           &handle);
    BAIL_ON_NT_STATUS(status);

    status = LwioLocalCreateDir(
                pszTargetPath,
                S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_NT_STATUS(status);

    for (;;)
    {
        status = LwNtQueryDirectoryFile(
            handle,                             /* File handle */
            NULL,                               /* Async control block */
            &ioStatus,                             /* IO status block */
            buffer,                             /* Info structure */
            sizeof(buffer),                     /* Info structure size */
            FileBothDirectoryInformation,         /* Info level */
            FALSE,                                 /* Do not return single entry */
            NULL,                                 /* File spec */
            bRestart);                             /* Restart scan */

        switch (status)
        {
        case STATUS_NO_MORE_MATCHES:
            status = STATUS_SUCCESS;
            goto cleanup;
        default:
            BAIL_ON_NT_STATUS(status);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            RTL_FREE(&pszEntryFilename);
            RTL_FREE(&pszRemotePath);
            RTL_FREE(&pszLocalPath);

            status = LwRtlCStringAllocateFromWC16String(
                        &pszEntryFilename,
                        pInfo->FileName
                        );
            BAIL_ON_NT_STATUS(status);

            if (!strcmp(pszEntryFilename, "..") ||
                !strcmp(pszEntryFilename, "."))
                continue;

            status = LwRtlCStringAllocatePrintf(
                        &pszRemotePath,
                        "%s/%s",
                        pszSourcePath,
                        pszEntryFilename);
            BAIL_ON_NT_STATUS(status);

            status = LwRtlCStringAllocatePrintf(
                        &pszLocalPath,
                        "%s/%s",
                        pszTargetPath,
                        pszEntryFilename);
            BAIL_ON_NT_STATUS(status);

            if(pInfo->FileAttributes == FILE_ATTRIBUTE_DIRECTORY)
            {

                status = LwioCopyDirFromRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_NT_STATUS(status);
            }
            else
            {
                status = LwioCopyFileFromRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_NT_STATUS(status);
          }
        }
    }

cleanup:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    RTL_FREE(&pszEntryFilename);
    RTL_UNICODE_STRING_FREE(&filename.Name);

    return status;

error:

    goto cleanup;
}


NTSTATUS
LwioCopyFileToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_HANDLE hRemoteFile = NULL;
    int hLocalFile = -1;
    DWORD dwBytesRead = 0;
    CHAR szBuf[BUFF_SIZE];

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    status = LwioLocalOpenFile(
                (PCSTR)pszSourcePath,
                O_RDONLY,
                0,
                &hLocalFile);

    BAIL_ON_NT_STATUS(status);

    status = LwioRemoteOpenFile(
                    pszTargetPath,
                    FILE_WRITE_DATA,
                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE,
                    &hRemoteFile);
    BAIL_ON_NT_STATUS(status);

    do
    {
        DWORD dwWritten = 0;

        memset (szBuf,0,BUFF_SIZE);

        if ((dwBytesRead = read(hLocalFile, szBuf, sizeof(szBuf))) == -1)
        {
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        if (dwBytesRead == 0)
        {
            break;
        }

        status  = LwioRemoteWriteFile(
                            hRemoteFile,
                            szBuf,
                            dwBytesRead,
                            &dwWritten);

        BAIL_ON_NT_STATUS(status);

    } while (dwBytesRead != 0);

cleanup:

    if (hRemoteFile)
    {
        LwNtCloseFile(hRemoteFile);
    }

    if (hLocalFile >= 0)
    {
        close(hLocalFile);
    }

    return (status);

error:

    goto cleanup;

}

NTSTATUS
LwioCopyDirFromLocalToLocal(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    if ((pDir = opendir(pszSourcePath)) == NULL)
    {
        status = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    status = LwioLocalCreateDir(
                pszTargetPath,
                S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_NT_STATUS(status);

    while ((pDirEntry = readdir(pDir)) != NULL)
    {
        RTL_FREE(&pszRemotePath);
        RTL_FREE(&pszLocalPath);

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        status = LwRtlCStringAllocatePrintf(
                    &pszLocalPath,
                    "%s/%s",
                    pszSourcePath,
                    pDirEntry->d_name);
        BAIL_ON_NT_STATUS(status);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszLocalPath, &statbuf) < 0)
        {
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        status = LwRtlCStringAllocatePrintf(
                    &pszRemotePath,
                    "%s/%s",
                    pszTargetPath,
                    pDirEntry->d_name);
        BAIL_ON_NT_STATUS(status);

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            status = LwioCopyDirFromLocalToLocal(
                            pszLocalPath,
                            pszRemotePath);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            status = LwioCopyFileFromLocalToLocal(
                            pszLocalPath,
                            pszRemotePath);
            BAIL_ON_NT_STATUS(status);
        }
    }

    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        status = LwErrnoToNtStatus(status);
        BAIL_ON_NT_STATUS(status);
    }

    pDir = NULL;

cleanup:

    if (pDir)
        closedir(pDir);

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    return status;

error:

    goto cleanup;

}

NTSTATUS
LwioCopyDirToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    IO_FILE_HANDLE handle = NULL;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    if ((pDir = opendir(pszSourcePath)) == NULL)
    {
        status = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    status = LwioRemoteOpenFile(
                    pszTargetPath,
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ |FILE_SHARE_WRITE |FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_DIRECTORY_FILE,
                    &handle);
    BAIL_ON_NT_STATUS(status);

    while ((pDirEntry = readdir(pDir)) != NULL)
    {
        RTL_FREE(&pszRemotePath);
        RTL_FREE(&pszLocalPath);

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        status = LwRtlCStringAllocatePrintf(
                    &pszLocalPath,
                    "%s/%s",
                    pszSourcePath,
                    pDirEntry->d_name);
        BAIL_ON_NT_STATUS(status);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszLocalPath, &statbuf) < 0)
        {
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        status = LwRtlCStringAllocatePrintf(
                    &pszRemotePath,
                    "%s/%s",
                    pszTargetPath,
                    pDirEntry->d_name);
        BAIL_ON_NT_STATUS(status);

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            status = LwioCopyDirToRemote(
                            pszLocalPath,
                            pszRemotePath);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            status = LwioCopyFileToRemote(
                            pszLocalPath,
                            pszRemotePath);
            BAIL_ON_NT_STATUS(status);
        }
    }

    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        status = LwErrnoToNtStatus(status);
        BAIL_ON_NT_STATUS(status);
    }

    pDir = NULL;

cleanup:
    if (handle)
        LwNtCloseFile(handle);

    if (pDir)
        closedir(pDir);

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    return status;

error:
    goto cleanup;

}
